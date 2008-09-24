#include <errno.h>
#include <signal.h>
#include "basin.h"

/*** basin.c ***/
//int main 
void create_backwin       (Display *display, Window *backwin, Pixmap *backwin_p);
int supress_xerror        (Display *display, XErrorEvent *event);
void print_frames         (struct Framelist* frames);

/*** draw.c ***/
extern void draw_background(Display* display, cairo_surface_t *surface); 
extern void draw_frame (Display* display, struct Frame frame);
extern void draw_closebutton(Display* display, struct Frame frame);

/*** frame.c ***/
extern void find_adjacent        (struct Framelist* frames, int frame_index);
extern int create_frame          (Display* display, struct Framelist* frames, Window framed_window); 
extern void create_startup_frames(Display *display, struct Framelist* frames);
extern void remove_frame         (Display* display, struct Framelist* frames, int index);
extern void remove_window        (Display* display, Window framed_window);

#include "draw.c"
#include "frame.c"

int done = False;

void end_event_loop(int sig) {

  Display* display = XOpenDisplay(NULL); 
  Screen* screen = DefaultScreenOfDisplay(display);
  Window root = DefaultRootWindow(display);
  Window temp;
  
  done = True;
  
  //give the event loop something to do so that it unblocks    
  temp = XCreateSimpleWindow(display, root, 0, 0, XWidthOfScreen(screen), XHeightOfScreen(screen), 0, WhitePixelOfScreen(screen), BlackPixelOfScreen(screen));
  XMapWindow(display, temp);
  XFlush(display);
  XDestroyWindow(display, temp);
}

int main (int argc, char* argv[]) {
  Display* display = NULL; 
  Screen* screen;  
  XEvent event;
  Window root, backwin;
  Pixmap backwin_p;
  struct Framelist frames = {NULL, 16, 0};

  Window last_pressed;
  int start_move_x, start_move_y, start_win;
  int resize_x = 0; //-1 means LHS, 1 means RHS
  int resize_y = 0; //-1 means top, 1 means bottom
  start_win = -1;
  
  printf("\n");
  
  frames.list = malloc(sizeof(struct Frame) * frames.max);
  if(frames.list == NULL) {
    printf("No available memory\n");
    return -1;
  }
  
  if (signal(SIGINT, end_event_loop) == SIG_ERR) {
    printf("Could not set the error handler\n");
    return -1;
  }
  
  printf("Opening Basin using the DISPLAY environment variable\n");
  printf("This can hang if the wrong screen number is given\n");
  display = XOpenDisplay(NULL);
  
  if(display == NULL)  {
    printf("Could not open display.");
    printf("USAGE: basin\n");
    printf("Set the display variable using: export DISPLAY=\":foo\"\n");
    printf("Where foo is the correct screen\n");
    return -1;
  }
    
  root = DefaultRootWindow(display);
  last_pressed = root; //last_pressed is used to determine if close buttons etc. should be activated
  screen = DefaultScreenOfDisplay(display);

  XSelectInput(display, root, SubstructureRedirectMask | ButtonPressMask);
  
  XGrabButton(display, 1, AnyButton, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
  create_startup_frames(display, &frames);  
  
  create_backwin(display, &backwin, &backwin_p);
  XSelectInput(display, backwin, ButtonPressMask | ButtonReleaseMask); 
  //Press for deselecting windows, Release for ending resize mode, dont need expose
  //if it is the background
  
  //XSynchronize(display, True);  //Turns on synchronized debugging
  
  while(!done) {
    //always look for windows that have been destroyed first
    if(XCheckTypedEvent(display, UnmapNotify, &event)) ; 
    else XNextEvent(display, &event);

    if(done) break;
    
    switch(event.type) {   
      case UnmapNotify:
        //printf("Unmapping window: %d\n", event.xunmap.window);  
        print_frames(&frames);
        
        if(start_win != -1) {
          printf("Cancelling resize because a window was unmapped\n");
          start_win = -1;
        }
        
        for(int i = 0; i < frames.used; i++) {
          //printf("Event window: %d, framed window: %d\n", event.xunmap.window, frames.list[i].window);
          if(event.xunmap.window == frames.list[i].window) {
            printf("removed the window %d\n", event.xunmap.window);
            remove_frame(display, &frames, i);
            break;
          }
        }
      break;
    
      case MapRequest:
        printf("\n");
        int index;
        index = create_frame(display, &frames, event.xmaprequest.window);
        if(index != -1) draw_frame(display, frames.list[index]);
        XMapWindow(display, event.xmaprequest.window);
      break;
      
      case Expose:
        //printf("Expose window: %d \n", event.xexpose.window);
        for(int i = 0; i < frames.used; i++) {
          if(event.xexpose.window == frames.list[i].frame) { 
            draw_frame(display, frames.list[i]);
            break;
          }
          else if (event.xexpose.window == frames.list[i].closebutton) { 
            draw_closebutton(display, frames.list[i]);
            break;
          }
        }
      break;
      
      case ButtonPress:
        printf("ButtonPress %d\n", event.xbutton.window);
        //TODO: Change the cursor
        last_pressed = root;
        for(int i = 0; i < frames.used; i++) {
          //start the move
          if(event.xbutton.window == frames.list[i].frame) { 
            int initial_x, initial_y;
            
            if(frames.list[i].mode != SINKING) 
              XSetInputFocus(display, frames.list[i].window, RevertToPointerRoot, CurrentTime);
              //If a sinking window is clicked on and is raised then it can get focus
              //Probably the focus history should be in a stack
              
            if(frames.list[i].mode == FLOATING) {
              XRaiseWindow(display, frames.list[i].frame);
            }
            
            XGrabPointer(display, event.xbutton.window, True, PointerMotionMask|ButtonReleaseMask, GrabModeAsync,  GrabModeAsync, None, None, CurrentTime);
            start_move_x = event.xbutton.x;
            start_move_y = event.xbutton.y;
            start_win = i;
            resize_x = 0;
            resize_y = 0;
            break;
          }
          else if(event.xbutton.window == frames.list[i].closebutton) {
            printf("pressed closebutton %d on window %d\n", frames.list[i].closebutton, frames.list[i].frame);
            last_pressed = frames.list[i].closebutton;
            break;
          }
        }
      break;
      
      case ButtonRelease:
        start_win = -1;
        resize_x = 0;
        resize_y = 0;
        XUngrabPointer(display, CurrentTime);
        //printf("release %d, subwindow %d, root %d\n", event.xbutton.window, event.xbutton.subwindow, event.xbutton.root);
        if(last_pressed != root) for(int i = 0; i < frames.used; i++) {
          if((last_pressed == event.xbutton.window) && (last_pressed == frames.list[i].closebutton)) {
            //printf("released closebutton %d, window %d\n", frames.list[i].closebutton, frames.list[i].frame);
            last_pressed = root;
            XSelectInput(display, frames.list[i].window, 0);
            XUnmapWindow(display, frames.list[i].window);            
            remove_window(display, frames.list[i].window);
            remove_frame(display, &frames, i);
            break;
          }
       }
      break;
      
      case MotionNotify: 
        if((event.xmotion.window != root) && (event.xmotion.window != backwin) && (start_win != -1)) { 
          while(XCheckTypedEvent(display, MotionNotify, &event));
          Window mouse_root, mouse_child;
          int mouse_root_x, mouse_root_y;
          int mouse_child_x, mouse_child_y;
          unsigned int mask;
          
          int new_width = 0;
          int new_height = 0;
          
          XQueryPointer(display, root, &mouse_root, &mouse_child, &mouse_root_x, &mouse_root_y, &mouse_child_x, &mouse_child_y, &mask);
          frames.list[start_win].x = mouse_root_x - start_move_x;
          frames.list[start_win].y = mouse_root_y - start_move_y;
         
          if((frames.list[start_win].x + frames.list[start_win].w > XWidthOfScreen(screen)) //window moving off RHS
           ||(resize_x == -1)) {  
            new_width = XWidthOfScreen(screen) - frames.list[start_win].x;
            resize_x = -1;
          }
          if((frames.list[start_win].x < 0) //window moving off LHS
           ||(resize_x == 1)) { 
            new_width = frames.list[start_win].w + frames.list[start_win].x;
            frames.list[start_win].x = 0;
            start_move_x = mouse_root_x;
            resize_x = 1;
          }

          if((frames.list[start_win].y + frames.list[start_win].h > XHeightOfScreen(screen)) //window moving off the bottom
           ||(resize_y == -1)) { 
            new_height = XHeightOfScreen(screen) - frames.list[start_win].y;
            resize_y = -1;
          }
          if((frames.list[start_win].y < 0) //window moving off the top of the screen
            ||(resize_y == 1)) { 
            new_height = frames.list[start_win].h + frames.list[start_win].y;
            frames.list[start_win].y = 0;
            start_move_y = mouse_root_y;
            resize_y = 1;
          }

          if((new_width != 0  &&  new_width < frames.list[start_win].min_width) 
           ||(new_height != 0  &&  new_height < frames.list[start_win].min_height)) {
             
            if(new_width != 0) new_width = 0;
            if(new_height != 0) new_height = 0;    
            //Set the state to Sinking
            //Don't retile
          }

          if((new_width != 0  &&  new_width > frames.list[start_win].max_width) //Don't allow too large resizes
           ||(new_height != 0  &&  new_height > frames.list[start_win].max_height)) {
            if(new_width > frames.list[start_win].max_width)   new_width = 0;
            if(new_height > frames.list[start_win].max_height) new_height = 0;
          }          
          
          if(new_width != 0  ||  new_height != 0) {   //resize window if required        
            if(new_width != 0) frames.list[start_win].w = new_width;
            if(new_height != 0) frames.list[start_win].h = new_height;
            
            XMoveResizeWindow(display, frames.list[start_win].frame, frames.list[start_win].x, frames.list[start_win].y,  frames.list[start_win].w, frames.list[start_win].h);
            cairo_xlib_surface_set_size(frames.list[start_win].frame_s, frames.list[start_win].w, frames.list[start_win].h);
            XResizeWindow(display, frames.list[start_win].window, frames.list[start_win].w - FRAME_HSPACE, frames.list[start_win].h - FRAME_VSPACE);
            
          }
          else {
            XMoveWindow(display, frames.list[start_win].frame, frames.list[start_win].x, frames.list[start_win].y);
          }
        }
        //add resize ability here  
      break; 

      case PropertyNotify:
        for(int i = 0; i < frames.used; i++) 
          if(event.xproperty.window == frames.list[i].window) {
            if( strcmp(XGetAtomName(display, event.xproperty.atom), "WM_NAME") == 0) {
             //NET_WM property is now preffered because it is UTF8
              XFetchName(display, frames.list[i].window, &frames.list[i].window_name);
              draw_frame(display, frames.list[i]);
            }
            break;
          }
      break;
      default:
      break;
    }
  }
  
  printf(".......... \n");
  XFreePixmap(display, backwin_p);
  XDestroyWindow(display, backwin);
  //destroy the backgoround window
  for(int i = 0; i < frames.used; i++) {
    //remove_window(display, frames.list[i].window);
    remove_frame(display, &frames, i);
  }
  XCloseDisplay(display);
  free(frames.list);
  return 1;  
}

void print_frames(struct Framelist* frames) {
  for(int i = 0; i < frames->used; i++) printf("Framed window %d is %d\n", i, frames->list[i].window);
}

/* Sometimes a client window is killed before it gets unmapped, we only get the unmapnotify event,
 but there is no way to tell so we just supress the error. No harm done. */
int supress_xerror(Display *display, XErrorEvent *event) {
  (void) display;
  (void) event;
  return 0;
}

/* This function creates the background underneath all the other windows */
void create_backwin(Display *display, Window *backwin, Pixmap *backwin_p) {
  Screen* screen = DefaultScreenOfDisplay(display);
  Visual* colours = XDefaultVisual(display, DefaultScreen(display));
  Window root = DefaultRootWindow(display);
  cairo_surface_t *backwin_s;
  cairo_surface_t *backwin_s_initial;
  
  *backwin = XCreateSimpleWindow(display, root, 0, 0, XWidthOfScreen(screen), XHeightOfScreen(screen), 0, WhitePixelOfScreen(screen), BlackPixelOfScreen(screen));
  XLowerWindow(display, *backwin);
  XMapWindow(display, *backwin);

  *backwin_p = XCreatePixmap(display, *backwin, XWidthOfScreen(screen), XHeightOfScreen(screen), XDefaultDepth(display, DefaultScreen(display)));

  //the pixmap is only draw on expose events so drawing this once to prevent a black background from showing up.
  backwin_s_initial = cairo_xlib_surface_create(display, *backwin, colours, XWidthOfScreen(screen), XHeightOfScreen(screen)); 
  draw_background(display, backwin_s_initial);
  
  backwin_s = cairo_xlib_surface_create(display, *backwin_p, colours, XWidthOfScreen(screen), XHeightOfScreen(screen));
  draw_background(display, backwin_s);
  
  XSetWindowBackgroundPixmap (display, *backwin, *backwin_p );
  XMapWindow(display, *backwin);
  cairo_surface_destroy(backwin_s);  
  cairo_surface_destroy(backwin_s_initial);    
}
