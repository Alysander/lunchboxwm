#include <errno.h>
#include <signal.h>
#include "basin.h"

/*** basin.c ***/
//int main 
int supress_xerror        (Display *display, XErrorEvent *event);

/*** draw.c ***/
extern void draw_frame (Display* display, struct Frame frame);
extern Pixmap create_pixmap(Display* display, enum main_pixmap type);
extern Pixmap create_title_pixmap(Display* display, char* title);

/*** frame.c ***/
extern void find_adjacent        (struct Framelist* frames, int frame_index);
extern int create_frame          (Display* display, struct Framelist* frames, Window framed_window, struct frame_pixmaps *pixmaps); 
extern void create_startup_frames(Display *display, struct Framelist* frames, struct frame_pixmaps *pixmaps);
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
  Window root, background_window;
  Pixmap background_window_p;
  
  struct frame_pixmaps pixmaps;
         
  struct Framelist frames = {NULL, 16, 0};

  Window last_pressed;
  int start_move_x, start_move_y, start_win;

  //idintifies the resize directions
  int resize_x = 0; //-1 means LHS, 1 means RHS
  int resize_y = 0; //-1 means top, 1 means bottom
  start_win = -1;   //identifies the window being moved/resized
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

  XSelectInput(display, root, SubstructureRedirectMask | ButtonPressMask| ButtonReleaseMask);
  
  //Button press mask for raising/setting focus,
  //Motion mask for moves/resizes
  //ButtonReleaseMask for when the close button has been pressed but release on the wrong window
  //XGrabPointer is an active grab (continous)
  //may change modifier to either none or ALT somehow
  
 /* XGrabButton(display, AnyButton, AnyModifier, root, False,
    ButtonPressMask, GrabModeSync, GrabModeAsync, None, None); */
    
 // XGrabPointer(display, root, False, ButtonPressMask| ButtonReleaseMask, GrabModeAsync,  GrabModeAsync, None, None, CurrentTime);

  pixmaps.border_p                    = create_pixmap(display, border);  
  pixmaps.body_p                      = create_pixmap(display, body);
  pixmaps.titlebar_background_p       = create_pixmap(display, titlebar);
  pixmaps.close_button_normal_p       = create_pixmap(display, close_button_normal);
  pixmaps.close_button_pressed_p      = create_pixmap(display, close_button_pressed);
  pixmaps.pulldown_floating_normal_p  = create_pixmap(display, pulldown_floating_normal);
  pixmaps.pulldown_floating_pressed_p = create_pixmap(display, pulldown_floating_pressed);
  pixmaps.pulldown_sinking_normal_p   = create_pixmap(display, pulldown_sinking_normal);
  pixmaps.pulldown_sinking_pressed_p  = create_pixmap(display, pulldown_sinking_pressed);
  pixmaps.pulldown_tiling_normal_p    = create_pixmap(display, pulldown_tiling_normal);
  pixmaps.pulldown_tiling_pressed_p   = create_pixmap(display, pulldown_tiling_pressed);
  pixmaps.arrow_normal_p              = create_pixmap(display, arrow_normal);
  pixmaps.arrow_pressed_p             = create_pixmap(display, arrow_pressed);
  create_startup_frames(display, &frames, &pixmaps);  
  
  /* Create the background window and double buffer it*/
  background_window = XCreateSimpleWindow(display, root, 0, 0, XWidthOfScreen(screen), XHeightOfScreen(screen), 0, BlackPixelOfScreen(screen), BlackPixelOfScreen(screen));
  background_window_p = create_pixmap(display, background);
  XLowerWindow(display, background_window);
  XSetWindowBackgroundPixmap (display, background_window, background_window_p );
  XMapWindow(display, background_window);

  //XSelectInput(display, background_window, ButtonPressMask | ButtonReleaseMask);  //this can be the root window
  //Press for deselecting windows, Release for ending resize mode, dont need expose
  //if it is the background
  
//  XSynchronize(display, True);  //Turns on synchronized debugging
  int i;
  while(!done) {
    //always look for windows that have been destroyed first
    if(XCheckTypedEvent(display, UnmapNotify, &event)) ; 
    else XNextEvent(display, &event);

    if(done) break;
    
    switch(event.type) {   
      case UnmapNotify:
        printf("Unmapping window: %d\n", event.xunmap.window);  
        //print_frames(&frames);
        
        if(start_win != -1) {
          printf("Cancelling resize because a window was unmapped\n");
          start_win = -1;
        }
        
        for(i = 0; i < frames.used; i++) {
          //printf("Event window: %d, framed window: %d\n", event.xunmap.window, frames.list[i].window);
          if(event.xunmap.window == frames.list[i].window) {
            printf("removed the window %d\n", event.xunmap.window);
            remove_frame(display, &frames, i);
            break;
          }
        }
      break;
      //handle resize requests, e.g. the download dialog box folding out in synaptic
      case MapRequest:
        printf("Mapping window %d\n", event.xmaprequest.window);
        int index;
        index = create_frame(display, &frames, event.xmaprequest.window, &pixmaps);
        if(index != -1) draw_frame(display, frames.list[index]);
        XMapWindow(display, event.xmaprequest.window);
      break;

      case ButtonPress:
        printf("ButtonPress %d, subwindow %d\n", event.xbutton.window, event.xbutton.subwindow);
        //TODO: Change the cursor
        last_pressed = root;
        for(i = 0; i < frames.used; i++) {
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
          else if(event.xbutton.window == frames.list[i].close_button) {
            printf("pressed closebutton %d on window %d\n", frames.list[i].close_button, frames.list[i].frame);
            last_pressed = frames.list[i].close_button;
            XUngrabPointer(display, CurrentTime); //little idiosyncrasy of X11, it automatically grabs the pointer on mouse down.
            XGrabPointer(display, last_pressed, True, PointerMotionMask|ButtonReleaseMask, GrabModeSync,  GrabModeSync, None, None, CurrentTime);
            XUnmapWindow(display, frames.list[i].close_button);
            XSetWindowBackgroundPixmap(display, frames.list[i].close_button, pixmaps.close_button_pressed_p );
            XMapWindow(display, frames.list[i].close_button);
            XFlush(display);
            break;
          }
          else if(event.xbutton.window == frames.list[i].mode_pulldown) {
            printf("pressed mode pulldown %d on window %d\n", frames.list[i].mode_pulldown, frames.list[i].frame);
            last_pressed = frames.list[i].mode_pulldown;
            XUngrabPointer(display, CurrentTime); //little idiosyncrasy of X11, it automatically grabs the pointer on mouse down.
            XUnmapWindow(display, frames.list[i].mode_pulldown);
            if(frames.list[i].mode == FLOATING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_floating_pressed_p );
            else if(frames.list[i].mode == TILING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_tiling_pressed_p );
            else if(frames.list[i].mode == SINKING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_sinking_pressed_p );
            XMapWindow(display, frames.list[i].mode_pulldown);
            XFlush(display);
            break;
          }
          else if(event.xbutton.window == frames.list[i].title_menu.frame) {
            printf("pressed title menu %d on window %d\n", frames.list[i].close_button, frames.list[i].frame);
            last_pressed =  frames.list[i].title_menu.frame;
            XUngrabPointer(display, CurrentTime); //little idiosyncrasy of X11, it automatically grabs the pointer on mouse down.
            XUnmapWindow(display, frames.list[i].title_menu.frame);
            XSetWindowBackgroundPixmap(display, frames.list[i].title_menu.arrow, pixmaps.arrow_pressed_p );
            XMapWindow(display, frames.list[i].title_menu.frame);
            XFlush(display);
            break;
          }
        }
      break;
      case EnterNotify:     
        printf("Enter Notify "); 
      case LeaveNotify:
        if((last_pressed != root)  && (event.xcrossing.window == last_pressed))  {
          //need to loop again to determine the type
          printf("Enter or exit.  Window %d, Subwindow %d\n", event.xcrossing.window, event.xcrossing.subwindow);
          for (i = 0; i < frames.used; i++) {
            if(last_pressed == frames.list[i].close_button) {
              XSelectInput(display, frames.list[i].close_button, 0);  
              XUnmapWindow(display, frames.list[i].close_button);
              if(event.type == EnterNotify) XSetWindowBackgroundPixmap(display, frames.list[i].close_button, pixmaps.close_button_pressed_p );
              else if (event.type == LeaveNotify) XSetWindowBackgroundPixmap(display, frames.list[i].close_button, pixmaps.close_button_normal_p );
              XMapWindow(display, frames.list[i].close_button);
              XSelectInput(display, frames.list[i].close_button, ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
              XFlush(display);
              break;
            }
            else if(last_pressed == frames.list[i].mode_pulldown) {
              XSelectInput(display, frames.list[i].mode_pulldown, 0);  
              XUnmapWindow(display, frames.list[i].mode_pulldown);  
              if(event.type == EnterNotify) {
                if(frames.list[i].mode == FLOATING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_floating_pressed_p );
                else if(frames.list[i].mode == TILING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_tiling_pressed_p );
                else if(frames.list[i].mode == SINKING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_sinking_pressed_p );
              }
              else if (event.type == LeaveNotify) {
                if(frames.list[i].mode == FLOATING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_floating_normal_p );
                else if(frames.list[i].mode == TILING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_tiling_normal_p );
                else if(frames.list[i].mode == SINKING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_sinking_normal_p );
              }
              XMapWindow(display, frames.list[i].mode_pulldown);
              XSelectInput(display, frames.list[i].mode_pulldown, ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
              XFlush(display);
              break;
            }
            else if(last_pressed == frames.list[i].title_menu.frame) {
              XSelectInput(display, frames.list[i].title_menu.frame, 0);  
              XUnmapWindow(display, frames.list[i].title_menu.arrow);
              if(event.type == EnterNotify)  XSetWindowBackgroundPixmap(display, frames.list[i].title_menu.arrow, pixmaps.arrow_pressed_p );
              else if (event.type == LeaveNotify) XSetWindowBackgroundPixmap(display, frames.list[i].title_menu.arrow, pixmaps.arrow_normal_p);
              XMapWindow(display, frames.list[i].title_menu.arrow);          
              XSelectInput(display, frames.list[i].title_menu.frame, ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask); 
              XFlush(display);
              break;    
            }
          }
        }
      break;
      case ButtonRelease:
        printf("release %d, subwindow %d, root %d\n", event.xbutton.window, event.xbutton.subwindow, event.xbutton.root);
        
        if(start_win != -1) {
          XUngrabPointer(display, CurrentTime);        
          start_win = -1;
          resize_x = 0;
          resize_y = 0;
        }

        if((last_pressed != root) && (last_pressed == event.xbutton.window)) {
          for(i = 0; i < frames.used; i++) {
            if(last_pressed == frames.list[i].close_button) {
              //printf("released closebutton %d, window %d\n", frames.list[i].closebutton, frames.list[i].frame);
              last_pressed = root;
              XSelectInput(display, frames.list[i].window, 0);
              XUnmapWindow(display, frames.list[i].window);            
              remove_window(display, frames.list[i].window);
              remove_frame(display, &frames, i);
              break;
            }
            else if(last_pressed == frames.list[i].mode_pulldown) {
              last_pressed = root;
              printf("Pressed the mode_pulldown on window %d\n", frames.list[i].window);
              XUnmapWindow(display, frames.list[i].mode_pulldown);
              if(frames.list[i].mode == FLOATING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_floating_normal_p );
              else if(frames.list[i].mode == TILING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_tiling_normal_p );
              else if(frames.list[i].mode == SINKING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_sinking_normal_p );
              XMapWindow(display, frames.list[i].mode_pulldown);
              XFlush(display);
              break;
            }
            else if(last_pressed == frames.list[i].title_menu.frame) {
              last_pressed = root;
              printf("Pressed the title_menu on window %d\n", frames.list[i].window);
              XUnmapWindow(display, frames.list[i].title_menu.arrow);
              XSetWindowBackgroundPixmap(display, frames.list[i].title_menu.arrow, pixmaps.arrow_normal_p);
              XMapWindow(display, frames.list[i].title_menu.arrow);          
              XFlush(display);
              break;
            }
          }
        }
      break;
      
      //for window moves and resizes
      case MotionNotify:
        /*** Move algorithm ***/
        if((event.xmotion.window != root) && (event.xmotion.window != background_window) && (start_win != -1)) { 
          while(XCheckTypedEvent(display, MotionNotify, &event));
          Window mouse_root, mouse_child;
          int mouse_root_x, mouse_root_y;
          int mouse_child_x, mouse_child_y;
          unsigned int mask;
          
          int new_width = 0;
          int new_height = 0;
          int new_x, new_y;
          
          XQueryPointer(display, root, &mouse_root, &mouse_child, &mouse_root_x, &mouse_root_y, &mouse_child_x, &mouse_child_y, &mask);    
          new_x = mouse_root_x - start_move_x;
          new_y = mouse_root_y - start_move_y;
          
          if((new_x + frames.list[start_win].w > XWidthOfScreen(screen)) //window moving off RHS
           ||(resize_x == -1)) {  
            resize_x = -1;
            new_width = XWidthOfScreen(screen) - new_x;
          }
          
          if((new_x < 0) //window moving off LHS
           ||(resize_x == 1)) { 
            resize_x = 1;
            new_width = frames.list[start_win].w + new_x;
            new_x = 0;
            start_move_x = mouse_root_x;
          }

          if((new_y + frames.list[start_win].h > XHeightOfScreen(screen)) //window moving off the bottom
           ||(resize_y == -1)) { 
            resize_y = -1;
            new_height = XHeightOfScreen(screen) - new_y;
          }
          
          if((new_y < 0) //window moving off the top of the screen
            ||(resize_y == 1)) { 
            resize_y = 1;
            new_height = frames.list[start_win].h + new_y;
            new_y = 0;
            start_move_y = mouse_root_y;
          }

          if((new_width != 0  &&  new_width < frames.list[start_win].min_width) 
           ||(new_height != 0  &&  new_height < frames.list[start_win].min_height)) {
             
            if(new_width != 0) new_width = 0;
            if(new_height != 0) new_height = 0;    
            //Set the state to Sinking
            //Don't retile
            break;//double check this
          }

          if((new_width != 0  &&  new_width > frames.list[start_win].max_width) //Don't allow too large resizes
           ||(new_height != 0  &&  new_height > frames.list[start_win].max_height)) {
            if(new_width > frames.list[start_win].max_width)   new_width = 0;
            if(new_height > frames.list[start_win].max_height) new_height = 0;
          } 
          
          if(new_width != 0  ||  new_height != 0) {   //resize window if required
            if(new_width != 0) {
              frames.list[start_win].w = new_width;
              frames.list[start_win].x = new_x;
            }
            else frames.list[start_win].x = new_x; //allow movement in axis if it hasn't been resized
  
            if(new_height != 0) {
              frames.list[start_win].h = new_height;
              frames.list[start_win].y = new_y;
            }
            else frames.list[start_win].y = new_y; //allow movement in axis if it hasn't been resized

            draw_frame(display, frames.list[start_win]);
          }
          else {
            //Moves the window to the specified location if there is no resizing going on.
            frames.list[start_win].x = new_x;
            frames.list[start_win].y = new_y;
            XMoveWindow(display, frames.list[start_win].frame, frames.list[start_win].x, frames.list[start_win].y);
          }
        }
        /*** Resize Algorithm should start here ***/

      break; 

      case PropertyNotify:
        for(i = 0; i < frames.used; i++) 
          if(event.xproperty.window == frames.list[i].window) {
            if( strcmp(XGetAtomName(display, event.xproperty.atom), "WM_NAME") == 0) {
              XFetchName(display, frames.list[i].window, &frames.list[i].window_name);
              XUnmapWindow(display, frames.list[i].title_menu.title);
              frames.list[i].title_menu.title_p = create_title_pixmap(display, frames.list[i].window_name);                
              XMapWindow(display, frames.list[i].title_menu.title);
              XFlush(display);
            }
            break;
          }
      break;
      default:
      break;
    }
  }

  XUngrabPointer(display, CurrentTime);  //doesn't hurt to do an extra one
  XUngrabButton(display, AnyButton, AnyModifier, root);
  XDestroyWindow(display, background_window);  
  XFreePixmap(display, background_window_p);
  XFreePixmap(display, pixmaps.border_p);
  XFreePixmap(display, pixmaps.body_p);
  XFreePixmap(display, pixmaps.titlebar_background_p);
  XFreePixmap(display, pixmaps.close_button_normal_p);
  XFreePixmap(display, pixmaps.close_button_pressed_p);
  XFreePixmap(display, pixmaps.pulldown_floating_normal_p);
  XFreePixmap(display, pixmaps.pulldown_floating_pressed_p);
  XFreePixmap(display, pixmaps.pulldown_sinking_normal_p);
  XFreePixmap(display, pixmaps.pulldown_sinking_pressed_p);
  XFreePixmap(display, pixmaps.pulldown_tiling_normal_p);
  XFreePixmap(display, pixmaps.pulldown_tiling_pressed_p);
  XFreePixmap(display, pixmaps.arrow_normal_p);
  XFreePixmap(display, pixmaps.arrow_pressed_p);  
  
  //destroy the backgoround window
  for(int i = 0; i < frames.used; i++) {
    remove_frame(display, &frames, i);
  }
  XCloseDisplay(display);
  free(frames.list);
  printf(".......... \n");  
  return 1;  
}

/* Sometimes a client window is killed before it gets unmapped, we only get the unmapnotify event,
 but there is no way to tell so we just supress the error. */
int supress_xerror(Display *display, XErrorEvent *event) {
  (void) display;
  (void) event;
  return 0;
}
