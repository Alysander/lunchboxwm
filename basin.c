#include <errno.h>
#include <signal.h>
#include "basin.h"

/*** basin.c ***/
//int main 
int supress_xerror        (Display *display, XErrorEvent *event);

/*** draw.c ***/
extern Pixmap create_pixmap(Display* display, enum main_pixmap type);
extern Pixmap create_title_pixmap(Display* display, const char* title, enum title_pixmap type); //separate function for the title as it needs the string to draw, returns width with the pointer

/*** frame.c ***/

extern void find_adjacent        (struct Framelist* frames, int frame_index);
extern int create_frame          (Display* display, struct Framelist* frames, Window framed_window, struct frame_pixmaps *pixmaps); 
extern void create_startup_frames(Display *display, struct Framelist* frames, struct frame_pixmaps *pixmaps);
extern void remove_frame         (Display* display, struct Framelist* frames, int index);
extern void remove_window        (Display* display, Window framed_window);
extern void get_frame_name       (Display* display, struct Frame* frame);
extern void get_frame_hints      (Display* display, struct Frame* frame);
extern void resize_frame         (Display* display, struct Frame* frame);

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
  Window grab_window;
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
    printf("Error: Could not open display.\n\n");
    printf("USAGE: basin\n");
    printf("Set the display variable using: export DISPLAY=\":foo\"\n");
    printf("Where foo is the correct screen\n");
    return -1;
  }
    
  root = DefaultRootWindow(display);
  last_pressed = root; //last_pressed is used to determine if close buttons etc. should be activated
  screen = DefaultScreenOfDisplay(display);
  int screen_number = DefaultScreen (display);
  
  XSelectInput(display, root, SubstructureRedirectMask | ButtonMotionMask | ButtonPressMask | ButtonReleaseMask);

/*  
  Button press mask for raising/setting focus,
  Motion mask for moves/resizes
  ButtonReleaseMask for when the close button has been pressed but release on the wrong window
  printf("grab returned %d\n",
  XGrabButton(display, 1, AnyMask, root, True, ButtonPressMask | ButtonReleaseMask, GrabModeSync, GrabModeAsync, None, None)
  );
*/
  pixmaps.border_p                    = create_pixmap(display, border);  
  pixmaps.light_border_p              = create_pixmap(display, light_border);  //this pixmap is only used for the pressed state of the title_menu
  pixmaps.body_p                      = create_pixmap(display, body);
  pixmaps.titlebar_background_p       = create_pixmap(display, titlebar);
  pixmaps.selection_p                 = create_pixmap(display, selection);
  
  pixmaps.close_button_normal_p       = create_pixmap(display, close_button_normal);
  pixmaps.close_button_pressed_p      = create_pixmap(display, close_button_pressed);
  pixmaps.close_button_deactivated_p  = create_pixmap(display, close_button_deactivated);
  
  pixmaps.pulldown_floating_normal_p  = create_pixmap(display, pulldown_floating_normal);
  pixmaps.pulldown_floating_pressed_p = create_pixmap(display, pulldown_floating_pressed);
  
  pixmaps.pulldown_sinking_normal_p   = create_pixmap(display, pulldown_sinking_normal);
  pixmaps.pulldown_sinking_pressed_p  = create_pixmap(display, pulldown_sinking_pressed);
    
  pixmaps.pulldown_tiling_normal_p    = create_pixmap(display, pulldown_tiling_normal);
  pixmaps.pulldown_tiling_pressed_p   = create_pixmap(display, pulldown_tiling_pressed);
    
  pixmaps.arrow_normal_p              = create_pixmap(display, arrow_normal);
  pixmaps.arrow_pressed_p             = create_pixmap(display, arrow_pressed);
  pixmaps.arrow_deactivated_p         = create_pixmap(display, arrow_deactivated);
  
  create_startup_frames(display, &frames, &pixmaps);  
  
  /* Create the background window and double buffer it*/
  background_window = XCreateSimpleWindow(display, root, 0, 0, XWidthOfScreen(screen), XHeightOfScreen(screen), 0, BlackPixelOfScreen(screen), BlackPixelOfScreen(screen));
  background_window_p = create_pixmap(display, background);
  XLowerWindow(display, background_window);
  XSetWindowBackgroundPixmap (display, background_window, background_window_p );
  XMapWindow(display, background_window);
  
  //XSynchronize(display, True);  //Turns on synchronized debugging
  int i;
  while(!done) {
    //always look for windows that have been destroyed first
    if(XCheckTypedEvent(display, DestroyNotify, &event)) ;
    //then look for unmaps
    else if(XCheckTypedEvent(display, UnmapNotify, &event)) ; 
    else XNextEvent(display, &event);

    if(done) break;
    
    switch(event.type) {   
      case DestroyNotify:
        printf("Destroyed window: %d\n", event.xdestroywindow.window);
        //sometimes a window gets away with being reparented and somehow not sending an unmap event.
        //then when the window is destroyed an empty frame is left.  This prevents that.
        for(i = 0; i < frames.used; i++) {
          if(event.xdestroywindow.window == frames.list[i].window) {
            printf("Removed the framed window %d\n", event.xdestroywindow.window);
            if(start_win != -1) { 
              printf("Cancelling resize because a window was destroyed\n");
              start_win = -1;
            }
            XUngrabPointer(display, CurrentTime);
            remove_frame(display, &frames, i);
            break;
          }
        }
      break;
      case UnmapNotify:
        printf("Unmapping window: %d\n", event.xunmap.window);  
        
        for(i = 0; i < frames.used; i++) {
          //printf("Event window: %d, framed window: %d\n", event.xunmap.window, frames.list[i].window);
          if(event.xunmap.window == frames.list[i].window) {
            if(frames.list[i].skip_reparent_unmap != 0) {
              printf("Ignored unmap for %d\n", event.xunmap.window);
              frames.list[i].skip_reparent_unmap = 0;
            }
            else {
              printf("Removed the framed window %d\n", event.xunmap.window);
              if(start_win != -1) {
                printf("Cancelling resize because a window was unmapped\n");
                start_win = -1;
              }
              XUngrabPointer(display, CurrentTime);
              remove_frame(display, &frames, i);
              break;
            }
          }
        }
      break;
      //handle resize requests, e.g. the download dialog box folding out in synaptic
      
      case MapRequest:
        printf("Mapping window %d\n", event.xmaprequest.window);
        for(i = 0; i < frames.used; i++) if(frames.list[i].window == event.xmaprequest.window) break;
        if(i == frames.used) create_frame(display, &frames, event.xmaprequest.window, &pixmaps);
        XMapWindow(display, event.xmaprequest.window);
        XFlush(display);
      break;
      case ConfigureNotify:
        for(i = 0; i < frames.used; i++) if(event.xconfigurerequest.window == frames.list[i].window) {
          if(frames.list[i].skip_resize_configure == 0  &&  start_win != i) { //gedit struggles against the resize unfortunately.
            if(event.xconfigurerequest.width + FRAME_HSPACE >= frames.list[i].min_width) frames.list[i].w = event.xconfigurerequest.width + FRAME_HSPACE;
            if(event.xconfigurerequest.width + FRAME_VSPACE >= frames.list[i].max_width) frames.list[i].h = event.xconfigurerequest.width + FRAME_VSPACE;
            printf("Adjusted width,height: %d %d\n", frames.list[i].w, frames.list[i].h);
            resize_frame(display, &frames.list[i]);
            frames.list[i].skip_resize_configure++;
          }
          else if(start_win != i) frames.list[i].skip_resize_configure--;
        }
      break;
      case ButtonPress:
        printf("ButtonPress %d, subwindow %d\n", event.xbutton.window, event.xbutton.subwindow);
        //TODO: Change the cursor
        last_pressed = root;
        for(i = 0; i < frames.used; i++) {
          //start the move
          if(event.xbutton.window == frames.list[i].frame  
            ||  event.xbutton.window == frames.list[i].mode_pulldown 
            ||  event.xbutton.window == frames.list[i].title_menu.hotspot) { 
            int initial_x, initial_y;
            
            if(frames.list[i].mode != SINKING) 
              XSetInputFocus(display, frames.list[i].window, RevertToPointerRoot, CurrentTime);
              //If a sinking window is clicked on and is raised then it can get focus
              //Probably the focus history should be in a stack
              
            if(frames.list[i].mode == FLOATING) {
              XRaiseWindow(display, frames.list[i].frame);
            }
            
            start_move_x = event.xbutton.x;
            start_move_y = event.xbutton.y;
            start_win = i;
            resize_x = 0;
            resize_y = 0;

//            grab_window = XCreateWindow(display, root, 0, 0, XWidthOfScreen(screen), XHeightOfScreen(screen), 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
//            XMapWindow(display, grab_window);

            XGrabPointer(display,  root, True, PointerMotionMask|ButtonReleaseMask, GrabModeAsync,  GrabModeAsync, None, None, CurrentTime);

            //Make the move still work even if they have activated a pulldown list
            if(event.xbutton.window == frames.list[i].mode_pulldown) {
              printf("pressed mode pulldown %d on window %d\n", frames.list[i].mode_pulldown, frames.list[i].frame);
              last_pressed = frames.list[i].mode_pulldown;
              start_move_x += 1; //compensate for relative co-ordinates of window and subwindow
              start_move_y += 1;              
              start_move_x += frames.list[i].w - H_SPACING*2 - PULLDOWN_WIDTH - BUTTON_SIZE - EDGE_WIDTH - 1;
              start_move_y += V_SPACING;
              XGrabPointer(display,  root, True, PointerMotionMask|ButtonReleaseMask, GrabModeAsync,  GrabModeAsync, None, None, CurrentTime);
              XUnmapWindow(display, frames.list[i].mode_pulldown);
              XMapWindow(display, frames.list[i].mode_pulldown);
            }
            else if(event.xbutton.window == frames.list[i].title_menu.hotspot) {
              printf("pressed title menu window %d\n", event.xbutton.window);
              last_pressed =  frames.list[i].title_menu.hotspot;       
              start_move_x += 1;
              start_move_y += 1;                        
              start_move_x += H_SPACING*2 + BUTTON_SIZE;
              start_move_y += V_SPACING;
              XGrabPointer(display,  root, True, PointerMotionMask|ButtonReleaseMask, GrabModeAsync,  GrabModeAsync, None, None, CurrentTime);
              XUnmapWindow(display, frames.list[i].title_menu.hotspot);                            
              XMapWindow(display, frames.list[i].title_menu.hotspot);           
            }
            XFlush(display);
            break;
          }
          
          /* These windows are already selected for leavenotify and enternotify events.  The grab generates a leave event, the map generates an enternotify event*/
          /* So we don't explicitly have to redraw :) */
          else if(event.xbutton.window == frames.list[i].close_button) {
            printf("pressed closebutton %d on window %d\n", frames.list[i].close_button, frames.list[i].frame);
            last_pressed = frames.list[i].close_button;
            XGrabPointer(display,  root, True, PointerMotionMask|ButtonReleaseMask, GrabModeAsync,  GrabModeAsync, None, None, CurrentTime);
            XUnmapWindow(display, frames.list[i].close_button);
            XMapWindow(display, frames.list[i].close_button);
            XFlush(display);
            break;
          }
        }
      break;
      case EnterNotify:     
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
            else if(last_pressed == frames.list[i].title_menu.hotspot) {
              XSelectInput(display, frames.list[i].title_menu.hotspot, 0); 
              XUnmapWindow(display, frames.list[i].title_menu.frame);
              if(event.type == EnterNotify)  { 
                printf("Enter notify, pressing down\n");
                XSetWindowBackgroundPixmap(display, frames.list[i].title_menu.arrow, pixmaps.arrow_pressed_p);
                XSetWindowBackgroundPixmap(display, frames.list[i].title_menu.title, frames.list[i].title_menu.title_pressed_p);
              }
              else if (event.type == LeaveNotify) {
                printf("Leave notify, pressing up %d\n", last_pressed);
                XSetWindowBackgroundPixmap(display, frames.list[i].title_menu.arrow, pixmaps.arrow_normal_p);
                XSetWindowBackgroundPixmap(display, frames.list[i].title_menu.title, frames.list[i].title_menu.title_normal_p);
              }
              XMapWindow(display, frames.list[i].title_menu.frame);
              XSelectInput(display, frames.list[i].title_menu.hotspot, ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
              XFlush(display);
              break;    
            }
          }
        }
      break;
      case ButtonRelease:
        printf("release %d, subwindow %d, root %d\n", event.xbutton.window, event.xbutton.subwindow, event.xbutton.root);
        
        if(start_win != -1) {
//          XDestroyWindow(display, grab_window);
          start_win = -1;
          resize_x = 0;
          resize_y = 0;
        }
        XUngrabPointer(display, CurrentTime);        
        
        if((last_pressed != root) && (last_pressed == event.xbutton.window)) {
          for(i = 0; i < frames.used; i++) {
            if(last_pressed == frames.list[i].close_button) {
              printf("released closebutton %d, window %d\n", frames.list[i].close_button, frames.list[i].frame);
              XSelectInput(display, frames.list[i].window, 0);
              XUnmapWindow(display, frames.list[i].window);            
              remove_window(display, frames.list[i].window);
              remove_frame(display, &frames, i);
              break;
            }
            else if(last_pressed == frames.list[i].mode_pulldown) {
              printf("Pressed the mode_pulldown on window %d\n", frames.list[i].window);
              XUnmapWindow(display, frames.list[i].mode_pulldown);
              if(frames.list[i].mode == FLOATING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_floating_normal_p );
              else if(frames.list[i].mode == TILING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_tiling_normal_p );
              else if(frames.list[i].mode == SINKING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_sinking_normal_p );
              XMapWindow(display, frames.list[i].mode_pulldown);
              XFlush(display);
              break;
            }
            else if(last_pressed == frames.list[i].title_menu.hotspot) {
              printf("Pressed the title_menu on window %d\n", frames.list[i].window);
              XUnmapWindow(display, frames.list[i].title_menu.frame);
              XSetWindowBackgroundPixmap(display, frames.list[i].title_menu.arrow, pixmaps.arrow_normal_p);
              XSetWindowBackgroundPixmap(display, frames.list[i].title_menu.title, frames.list[i].title_menu.title_normal_p);
              XMapWindow(display, frames.list[i].title_menu.frame);          
              XFlush(display);
              break;
            }
          }
        }
        last_pressed = root;
      break;
      
      //for window moves and resizes
      case MotionNotify:
        /*** Move algorithm ***/
        if(start_win != -1) { 
          while(XCheckTypedEvent(display, MotionNotify, &event));
          Window mouse_root, mouse_child;
          int mouse_root_x, mouse_root_y;
          int mouse_child_x, mouse_child_y;
          unsigned int mask;
          
          int new_width = 0;
          int new_height = 0;
          int new_x, new_y;
          
          if(last_pressed != root) {
            for(i = 0; i < frames.used; i++) {
              if(last_pressed == frames.list[i].mode_pulldown) {
                printf("Pressed the mode_pulldown on window %d\n", frames.list[i].window);
                XUnmapWindow(display, frames.list[i].mode_pulldown);
                if(frames.list[i].mode == FLOATING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_floating_normal_p );
                else if(frames.list[i].mode == TILING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_tiling_normal_p );
                else if(frames.list[i].mode == SINKING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_sinking_normal_p );
                XMapWindow(display, frames.list[i].mode_pulldown);
                XFlush(display);
                break;
              }
              else if(last_pressed == frames.list[i].title_menu.hotspot) {
                printf("Pressed the title_menu on window %d\n", frames.list[i].window);
                XUnmapWindow(display, frames.list[i].title_menu.frame);
                XSetWindowBackgroundPixmap(display, frames.list[i].title_menu.arrow, pixmaps.arrow_normal_p);
                XSetWindowBackgroundPixmap(display, frames.list[i].title_menu.title, frames.list[i].title_menu.title_normal_p);
                XMapWindow(display, frames.list[i].title_menu.frame);          
                XFlush(display);
                break;
              }
            }
            last_pressed = root; //this will cancel the pulldown lists opening
          }
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

            resize_frame(display, &frames.list[start_win]);
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
              get_frame_name(display, &frames.list[i]);
              resize_frame(display, &frames.list[i]);
            }
            else if ( strcmp(XGetAtomName(display, event.xproperty.atom), "WM_NORMAL_HINTS") == 0) {
              get_frame_hints(display, &frames.list[i]);
            }
            break;
          }
      break;
      default:
      break;
    }
  }

  XUngrabPointer(display, CurrentTime);  //doesn't hurt to do an extra one
  XDestroyWindow(display, background_window);  
  XFreePixmap(display, background_window_p);
  XFreePixmap(display, pixmaps.border_p);
  XFreePixmap(display, pixmaps.body_p);
  XFreePixmap(display, pixmaps.selection_p);
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
  XFreePixmap(display, pixmaps.arrow_deactivated_p);
  
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
