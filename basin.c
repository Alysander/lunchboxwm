#include <errno.h>
#include <signal.h>
#include "basin.h"

/*** basin.c ***/
//int main 
int supress_xerror (Display *display, XErrorEvent *event);
void load_pixmaps (Display *display, struct frame_pixmaps *pixmaps);
void load_cursors (Display *display, struct mouse_cursors *cursors);
void free_pixmaps (Display *display, struct frame_pixmaps *pixmaps);
void free_cursors (Display *display, struct mouse_cursors *cursors);

void create_mode_pulldown_list(Display *display, struct mode_pulldown_list *mode_pulldown);
void remove_mode_pulldown_list(Display *display, struct mode_pulldown_list *mode_pulldown);

/*** draw.c ***/
extern Pixmap create_pixmap(Display* display, enum main_pixmap type);
//separate function for the title as it needs the string to draw
extern Pixmap create_title_pixmap(Display* display, const char* title, enum title_pixmap type); 

/*** frame.c ***/
extern void find_adjacent        (struct Framelist* frames, int frame_index);
extern int create_frame          (Display* display, struct Framelist* frames, Window framed_window, struct frame_pixmaps *pixmaps, struct mouse_cursors *cursors); 
extern void create_startup_frames(Display *display, struct Framelist* frames, struct frame_pixmaps *pixmaps, struct mouse_cursors *cursors);
extern void remove_frame         (Display* display, struct Framelist* frames, int index);
extern void remove_window        (Display* display, Window framed_window);
extern void get_frame_program_name(Display* display, struct Frame* frame);
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
  int screen_number;
  
  XEvent event;
  Window root, background_window, clicked_widget; 
  Window pulldown;
  Pixmap background_window_p;   
  
  struct frame_pixmaps pixmaps;
  struct mouse_cursors cursors; 
  struct Framelist frames = {NULL, 16, 0};

  struct mode_pulldown_list mode_pulldown; //this window is created and reused.
  
  int do_click_to_focus = 0;
  int grab_move = 0;
  int start_move_x, start_move_y;
  
  int clicked_frame = -1; //identifies the window being moved/resized by frame index
  int clicked_menu_item = -1;  //this can be a frame index or FLOATING/TILING/SINKING
  
  int resize_x_direction = 0; //1 means LHS, -1 means RHS
  int resize_y_direction = 0; //1 means top, -1 means bottom
  int i; //i is the iterator for the window array 
         //and is also used to check how the loop terminated
  
  frames.list = malloc(sizeof(struct Frame) * frames.max);
  if(frames.list == NULL) {
    printf("\nError: No available memory\n");
    return -1;
  }
  
  if(signal(SIGINT, end_event_loop) == SIG_ERR) {
    printf("\nError: Could not set the error handler\n Is this a POSIX conformant system?\n");
    return -1;
  }
  
  printf("\nOpening Basin using the DISPLAY environment variable\n");
  printf("This can hang if the wrong screen number is given\n");
  display = XOpenDisplay(NULL);
  
  if(display == NULL)  {
    printf("Error: Could not open display.\n\n");
    printf("USAGE: basin\n");
    printf("Set the display variable using: export DISPLAY=\":foo\"\n");
    printf("Where foo is the correct screen\n");
    return -1;
  }

  //XSynchronize(display, True);  //Turns on synchronized debugging  
  
  if(XcursorSetTheme(display, "DMZ-White") == True) { 
    XcursorSetDefaultSize (display, 24);
    XSync(display, False);
  }
  else {
    printf("Error: could not find the cursor theme\n");
    XCloseDisplay(display);
    return -1;
  }
  
  root = DefaultRootWindow(display);
  screen = DefaultScreenOfDisplay(display);
  screen_number = DefaultScreen (display);
  
  pulldown = root;
  clicked_widget = root; //clicked_widget is used to determine if close buttons etc. should be activated
  
  XSelectInput(display, root, SubstructureRedirectMask | ButtonMotionMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask); 
  load_pixmaps (display, &pixmaps);
  load_cursors (display, &cursors);
  XFlush(display);
  create_startup_frames(display, &frames, &pixmaps, &cursors);  
  
  /* Create the background window and double buffer it*/
  background_window = XCreateSimpleWindow(display, root, 0, 0, XWidthOfScreen(screen), XHeightOfScreen(screen), 0, BlackPixelOfScreen(screen), BlackPixelOfScreen(screen));
  background_window_p = create_pixmap(display, background);
  XLowerWindow(display, background_window);
  XSetWindowBackgroundPixmap (display, background_window, background_window_p );
  XDefineCursor(display, background_window, cursors.normal);  
  XMapWindow(display, background_window);

  XGrabButton(display, Button1, Mod1Mask, root, False, ButtonPressMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, cursors.grab);

  XFlush(display);
  
  while(!done) {
    //always look for windows that have been destroyed first
    if(XCheckTypedEvent(display, DestroyNotify, &event)) ;
    //then look for unmaps
    else if(XCheckTypedEvent(display, UnmapNotify, &event)) ; 
    else XNextEvent(display, &event);

    if(done) break;
    //these are from the StructureNotifyMask on the reparented window
    switch(event.type) { 
      case DestroyNotify:  
        printf("Destroyed window, ");
      case UnmapNotify:
        printf("Unmapping: %d\n", event.xdestroywindow.window);
        //The OpenOffice splash screen doesn't send an unmap event.
        for(i = 0; i < frames.used; i++) {
          if(event.xany.window == frames.list[i].window) {
            if(clicked_frame != -1) { 
              printf("Cancelling resize because a window was destroyed\n");
              clicked_frame = -1;
            }
            XUngrabPointer(display, CurrentTime);
            printf("Removed frame i:%d, frame window %d\n, framed_window %d\n",
             i, frames.list[i].frame, event.xany.window);
            remove_frame(display, &frames, i);
            break;
          }
        }
      break;
      
      case MapRequest:
        printf("Mapping window %d\n", event.xmaprequest.window);
        for(i = 0; i < frames.used; i++) if(frames.list[i].window == event.xmaprequest.window) break;
        if(i == frames.used) create_frame(display, &frames, event.xmaprequest.window, &pixmaps, &cursors);
        XMapWindow(display, event.xmaprequest.window);
        XFlush(display);
      break;
      
      case ButtonPress:
        printf("ButtonPress %d, subwindow %d\n", event.xbutton.window, event.xbutton.subwindow);

        if(grab_move) {
          for(i = 0; i < frames.used; i++) {
            if(frames.list[i].frame == event.xbutton.subwindow) {
              printf("started grab_move\n");
              clicked_frame = i;
              break;
            }
          }
          start_move_x = event.xbutton.x - frames.list[clicked_frame].x;
          start_move_y = event.xbutton.y - frames.list[clicked_frame].y;      
          resize_x_direction = 0;
          resize_y_direction = 0;
          break;
        }
        
        if(pulldown != root) { //the pulldown has been created, ignore button press
          break;
        }
        
        if(do_click_to_focus)  {
          for(i = 0; i < frames.used; i++) {
            if(frames.list[i].backing == event.xbutton.window) {
              clicked_frame = i;
              break;
            }
          }
        }
                
        for(i = 0; i < frames.used; i++) {
          
          if(event.xbutton.window == frames.list[i].frame  
            ||  event.xbutton.window == frames.list[i].mode_pulldown 
            ||  event.xbutton.window == frames.list[i].title_menu.hotspot){ 
            do_click_to_focus = 1; //this will raise the window, see code below
            start_move_x = event.xbutton.x;
            start_move_y = event.xbutton.y;
            clicked_frame = i;
            resize_x_direction = 0;
            resize_y_direction = 0;
            
            XGrabPointer(display,  root, True, 
                         PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
                         GrabModeAsync,  GrabModeAsync, None, cursors.grab, CurrentTime);

            if(event.xbutton.window == frames.list[i].mode_pulldown  
              ||  event.xbutton.window == frames.list[i].title_menu.hotspot) {
              start_move_x += 1; //compensate for relative co-ordinates of window and subwindow
              start_move_y += 1;
            }
            
            if(event.xbutton.window == frames.list[i].mode_pulldown) {
              printf("pressed mode pulldown %d on window %d\n", frames.list[i].mode_pulldown, frames.list[i].frame);
              clicked_widget = frames.list[i].mode_pulldown;
              start_move_x += frames.list[i].w - H_SPACING*2 - PULLDOWN_WIDTH - BUTTON_SIZE - EDGE_WIDTH - 1;
              start_move_y += V_SPACING;
              XUnmapWindow(display, frames.list[i].mode_pulldown);
              XFlush(display);
              printf("changing mode pulldown pixmaps\n");
              if(frames.list[i].mode == FLOATING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_floating_pressed_p );
              else if(frames.list[i].mode == TILING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_tiling_pressed_p );
              else if(frames.list[i].mode == SINKING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_deactivated_p );
              XMapWindow(display, frames.list[i].mode_pulldown);
            }
            else if(event.xbutton.window == frames.list[i].title_menu.hotspot) {
              printf("pressed title menu window %d\n", event.xbutton.window);
              clicked_widget =  frames.list[i].title_menu.hotspot;                             
              start_move_x += H_SPACING*2 + BUTTON_SIZE;
              start_move_y += V_SPACING;
              XUnmapWindow(display, frames.list[i].title_menu.frame);      
              XFlush(display);
              XSetWindowBackgroundPixmap(display, frames.list[i].title_menu.arrow, pixmaps.arrow_pressed_p);
              XSetWindowBackgroundPixmap(display, frames.list[i].title_menu.title, frames.list[i].title_menu.title_pressed_p);
              XMapWindow(display, frames.list[i].title_menu.frame);           
            }
            XFlush(display);
            break;
          }
          
          else if(event.xbutton.window == frames.list[i].close_button) {
            do_click_to_focus = 1;
            printf("pressed closebutton %d on window %d\n", frames.list[i].close_button, frames.list[i].frame);
            clicked_widget = frames.list[i].close_button;
            XGrabPointer(display,  root, True,
                         PointerMotionMask | ButtonReleaseMask,
                         GrabModeAsync,  GrabModeAsync, None, None, CurrentTime);
            XUnmapWindow(display, frames.list[i].close_button);
            /* This is already selected for leavenotify and enternotify events.  */
            /* The grab generates a leave event, the map generates an enternotify event*/
            /* So we don't explicitly have to redraw :) */
            XMapWindow(display, frames.list[i].close_button);
            XFlush(display);
            break;
          }
          else if (event.xbutton.window == frames.list[i].l_grip
                  ||  event.xbutton.window == frames.list[i].r_grip
                  ||  event.xbutton.window == frames.list[i].bl_grip
                  ||  event.xbutton.window == frames.list[i].br_grip
                  ||  event.xbutton.window == frames.list[i].b_grip) {
            do_click_to_focus = 1;      
            start_move_x = event.xbutton.x;
            start_move_y = event.xbutton.y;
            clicked_frame = i;
            resize_x_direction = 0;
            resize_y_direction = 0;

            if(event.xbutton.window == frames.list[i].l_grip) {
              printf("pressed l_grip on window %d\n", frames.list[i].frame);
              clicked_widget = frames.list[i].l_grip;
              start_move_y += TITLEBAR_HEIGHT + 1 + EDGE_WIDTH*2;
              XGrabPointer(display, frames.list[i].l_grip, False, PointerMotionMask|ButtonReleaseMask, GrabModeAsync,  GrabModeAsync, None, cursors.resize_h, CurrentTime);
            }
            else if(event.xbutton.window == frames.list[i].r_grip) {
              printf("pressed r_grip on window %d\n", frames.list[i].frame);
              clicked_widget = frames.list[i].r_grip;
              XGrabPointer(display, frames.list[i].l_grip, False, PointerMotionMask|ButtonReleaseMask, GrabModeAsync,  GrabModeAsync, None, cursors.resize_h, CurrentTime);
            }
            else if(event.xbutton.window == frames.list[i].bl_grip) {
              printf("pressed bl_grip on window %d\n", frames.list[i].frame);
              clicked_widget = frames.list[i].bl_grip;
              start_move_y += frames.list[i].h - CORNER_GRIP_SIZE;
              XGrabPointer(display, frames.list[i].l_grip, False, PointerMotionMask|ButtonReleaseMask, GrabModeAsync,  GrabModeAsync, None, cursors.resize_tr_bl, CurrentTime);
            }
            else if(event.xbutton.window == frames.list[i].br_grip) {
              printf("pressed bl_grip on window %d\n", frames.list[i].frame);
              clicked_widget = frames.list[i].br_grip;
              XGrabPointer(display, frames.list[i].l_grip, False, PointerMotionMask|ButtonReleaseMask, GrabModeAsync,  GrabModeAsync, None, cursors.resize_tl_br, CurrentTime);
            }
            else if(event.xbutton.window == frames.list[i].b_grip) {
              printf("pressed bl_grip on window %d\n", frames.list[i].frame);
              clicked_widget = frames.list[i].b_grip;
              XGrabPointer(display, frames.list[i].l_grip, False, PointerMotionMask|ButtonReleaseMask, GrabModeAsync,  GrabModeAsync, None, cursors.resize_v, CurrentTime);
            }          
          }
          XFlush(display);
        }
        
        /** Focus and raising policy **/
        //Probably the focus history should be in a stack
        if(do_click_to_focus) {
          //don't raise window if the pulldown is open, it has already been raised
          //and focussed and raising again hides the pop-up        
          if(pulldown != root) {
            XAllowEvents(display, AsyncPointer, event.xbutton.time);
            break;
          }
          
          printf("focussing frame %d\n", clicked_frame);
          if(clicked_frame != -1) {
            if(frames.list[clicked_frame].mode == FLOATING) 
              XRaiseWindow(display, frames.list[clicked_frame].frame);
            XSetInputFocus(display, frames.list[clicked_frame].window, RevertToPointerRoot, CurrentTime);
          }
          XAllowEvents(display, ReplayPointer, event.xbutton.time);
          do_click_to_focus = 0;
          break;
        }
      break;
      
      case EnterNotify:
        if(event.xcrossing.mode == NotifyGrab) {
          if(event.xcrossing.state & Mod1Mask) {
            printf("set grab_move\n");
            grab_move = 1;
          }
          else {
            printf("set do_click_to_focus through enternotify\n");
            do_click_to_focus = 1;
          }
          break;
        }
      case LeaveNotify:
        if(event.type == LeaveNotify  &&  event.xcrossing.mode == NotifyUngrab) {
          grab_move = 0;
          clicked_frame = -1;
        }
          
        if((clicked_widget != root)  && (event.xcrossing.window == clicked_widget))  {
          //need to loop again to determine the type
          printf("Enter or exit.  Window %d, Subwindow %d\n", event.xcrossing.window, event.xcrossing.subwindow);
          for (i = 0; i < frames.used; i++) {
            if(clicked_widget == frames.list[i].close_button) {
              XSelectInput(display, frames.list[i].close_button, 0);  
              XUnmapWindow(display, frames.list[i].close_button);
              if(event.type == EnterNotify) XSetWindowBackgroundPixmap(display, frames.list[i].close_button, pixmaps.close_button_pressed_p );
              else if (event.type == LeaveNotify) XSetWindowBackgroundPixmap(display, frames.list[i].close_button, pixmaps.close_button_normal_p );
              XMapWindow(display, frames.list[i].close_button);
              XSelectInput(display, frames.list[i].close_button, ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
              XFlush(display);
              break;
            }
          }
        }
      break;
      
      case ButtonRelease:
        printf("ButtonRelease. Window %d, subwindow %d, root %d\n", event.xbutton.window, event.xbutton.subwindow, event.xbutton.root);        

        /* Close pop-up menu and maybe activate a menu item */
        if(pulldown != root) { //
          printf("closed pulldown\n");
          XDestroyWindow(display, pulldown);
          printf("clicked_widget %d\n", clicked_widget);
          for(i = 0; i < frames.used; i++) {
            if(clicked_widget == frames.list[i].mode_pulldown) {
              printf("Redrawing mode pulldown\n");
              XUnmapWindow(display, frames.list[i].mode_pulldown);
              XFlush(display);
              if(frames.list[i].mode == FLOATING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_floating_normal_p );
              else if(frames.list[i].mode == TILING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_tiling_normal_p );
              else if(frames.list[i].mode == SINKING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_deactivated_p );
              XMapWindow(display, frames.list[i].mode_pulldown);
              /*Do some menu item identifying here */
              break;                
            }
            else if(clicked_widget == frames.list[i].title_menu.hotspot) {
              printf("Redrawing title pulldown\n");
              XUnmapWindow(display, frames.list[i].title_menu.frame);
              XFlush(display);
              XSetWindowBackgroundPixmap(display, frames.list[i].title_menu.arrow, pixmaps.arrow_normal_p);
              XSetWindowBackgroundPixmap(display, frames.list[i].title_menu.title, frames.list[i].title_menu.title_normal_p);
              XMapWindow(display, frames.list[i].title_menu.frame);          
              /*Do some menu item identifying here */
              break;
            }
          }
          XFlush(display);
          pulldown = root;          
          clicked_widget = root;
        }

        /* Activate a widget */
        if((clicked_widget != root) && (clicked_widget == event.xbutton.window)) {
          for(i = 0; i < frames.used; i++) {
            if(clicked_widget == frames.list[i].close_button) {
              printf("released closebutton %d, window %d\n", frames.list[i].close_button, frames.list[i].frame);
              XSelectInput(display, frames.list[i].window, 0);
              XUnmapWindow(display, frames.list[i].window);            
              remove_window(display, frames.list[i].window);
              remove_frame(display, &frames, i);
              clicked_widget = root;
              break;
            }
            else if(clicked_widget == frames.list[i].mode_pulldown) {
              printf("Pressed the mode_pulldown on window %d\n", frames.list[i].window);
              XChangeActivePointerGrab(display, 
                                       PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
                                       cursors.normal, CurrentTime);
              pulldown = XCreateSimpleWindow(display, root, event.xbutton.x_root, event.xbutton.y_root, 
                                      100, 200, 1, WhitePixelOfScreen(screen), BlackPixelOfScreen(screen));
              XMapWindow(display, pulldown);
              
              XFlush(display);
              clicked_frame = -1; //cancel move ability at this stage/don't ungrab
              break;
            }
            else if(clicked_widget == frames.list[i].title_menu.hotspot) {
              printf("Pressed the title menu on window %d\n", frames.list[i].window);            
              XChangeActivePointerGrab(display, 
                                       PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
                                       cursors.normal, CurrentTime);              
              pulldown = XCreateSimpleWindow(display, root, event.xbutton.x_root, event.xbutton.y_root, 
                                      100, 200, 1, WhitePixelOfScreen(screen), BlackPixelOfScreen(screen));
              XMapWindow(display, pulldown);
              
              XFlush(display);
              clicked_frame = -1; //cancel move ability at this stage/don't ungrab              
              break;
            }
          }
        }
        
        if(clicked_widget != root) for(i = 0; i < frames.used; i++) {
          if(clicked_widget == frames.list[i].l_grip
             ||  clicked_widget == frames.list[i].bl_grip
             ||  clicked_widget == frames.list[i].b_grip
             ||  clicked_widget == frames.list[i].br_grip
             ||  clicked_widget == frames.list[i].r_grip) {
            printf("cancelled resize\n");
            clicked_widget = root;
            clicked_frame = -1;
            break;
          }
        }
        
        if(clicked_widget == root)  XUngrabPointer(display, CurrentTime);
        if(clicked_frame != -1) {
          printf("cancelling frame click\n");
          XUngrabPointer(display, CurrentTime);
          clicked_frame = -1;
          break;
        }
      break;
      
      case MotionNotify:

        if(clicked_frame != -1) { 
          //skip on to the latest move event
          while(XCheckTypedEvent(display, MotionNotify, &event));
          Window mouse_root, mouse_child;
          int mouse_root_x, mouse_root_y;
          int mouse_child_x, mouse_child_y;
          unsigned int mask;
          
          int new_width = 0;
          int new_height = 0;
          int new_x, new_y;
          
          if(clicked_widget != root) { //just the frame was pressed, start the squish
            for(i = 0; i < frames.used; i++) {
              if(clicked_widget == frames.list[i].mode_pulldown) {
                XUnmapWindow(display, frames.list[i].mode_pulldown);
                if(frames.list[i].mode == FLOATING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_floating_normal_p );
                else if(frames.list[i].mode == TILING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_tiling_normal_p );
                else if(frames.list[i].mode == SINKING) XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_deactivated_p );
                XMapWindow(display, frames.list[i].mode_pulldown);
                XFlush(display);
                clicked_widget = root; //cancel the pulldown lists opening
                break;
              }
              else if(clicked_widget == frames.list[i].title_menu.hotspot) {
                XUnmapWindow(display, frames.list[i].title_menu.frame);
                XSetWindowBackgroundPixmap(display, frames.list[i].title_menu.arrow, pixmaps.arrow_normal_p);
                XSetWindowBackgroundPixmap(display, frames.list[i].title_menu.title, frames.list[i].title_menu.title_normal_p);
                XMapWindow(display, frames.list[i].title_menu.frame);          
                XFlush(display);
                clicked_widget = root; //cancel the pulldown lists opening                
                break;
              }
            }
          }
          XQueryPointer(display, root, &mouse_root, &mouse_child, &mouse_root_x, &mouse_root_y, &mouse_child_x, &mouse_child_y, &mask);    
          
          new_x = mouse_root_x - start_move_x;
          new_y = mouse_root_y - start_move_y;
          
          /*** Move/Squish ***/
          if(clicked_widget == root) { //no widget is active, only the frame or grips
            
            if((new_x + frames.list[clicked_frame].w > XWidthOfScreen(screen)) //window moving off RHS
             ||(resize_x_direction == -1)) {  
              resize_x_direction = -1;
              new_width = XWidthOfScreen(screen) - new_x;
            }
            
            if((new_x < 0) //window moving off LHS
             ||(resize_x_direction == 1)) { 
              resize_x_direction = 1;
              new_width = frames.list[clicked_frame].w + new_x;
              new_x = 0;
              start_move_x = mouse_root_x;
            }

            if((new_y + frames.list[clicked_frame].h > XHeightOfScreen(screen)) //window moving off the bottom
             ||(resize_y_direction == -1)) { 
              resize_y_direction = -1;
              new_height = XHeightOfScreen(screen) - new_y;
            }
            
            if((new_y < 0) //window moving off the top of the screen
              ||(resize_y_direction == 1)) { 
              resize_y_direction = 1;
              new_height = frames.list[clicked_frame].h + new_y;
              new_y = 0;
              start_move_y = mouse_root_y;
            }

            if((new_width != 0  &&  new_width < frames.list[clicked_frame].min_width + FRAME_HSPACE) 
             ||(new_height != 0  &&  new_height < frames.list[clicked_frame].min_height + FRAME_VSPACE)) {
              if(new_width != 0) {
                new_width = 0;
                //don't move the window off the RHS if it has reached it's minimum size
                //LHS not considered because x has already been set to 0
                if(resize_x_direction == -1) new_x = XWidthOfScreen(screen) - frames.list[clicked_frame].w;
                /*** TODO: Set sinking appearance here ***/
              }
              if(new_height != 0) {
                new_height = 0;    
                //don't move the window off the bottom if it has reached it's minimum size
                //Top not considered because y has already been set to 0
                if(resize_y_direction == -1) new_y = XHeightOfScreen(screen) - frames.list[clicked_frame].h;
              }
              
            }

            //limit resizes to max width
            if((new_width != 0  &&  new_width > frames.list[clicked_frame].max_width + FRAME_HSPACE) 
             ||(new_height != 0  &&  new_height > frames.list[clicked_frame].max_height + FRAME_VSPACE)) {
              //investigate if this has similar situations as above where it moves instead of stopping
              //once limit is reached
              if(new_width > frames.list[clicked_frame].max_width + FRAME_HSPACE) new_width = 0;
              if(new_height > frames.list[clicked_frame].max_height + FRAME_VSPACE) new_height = 0;
            } 

            //do not attempt to resize windows that cannot be resized
            if(frames.list[clicked_frame].min_width == frames.list[clicked_frame].max_width) resize_x_direction = 0;
            if(frames.list[clicked_frame].min_height == frames.list[clicked_frame].max_height) resize_y_direction = 0;
                        
            if(new_width != 0  ||  new_height != 0) {   //resize window if required
            
              if(new_width != 0) {
                frames.list[clicked_frame].w = new_width;              
                frames.list[clicked_frame].x = new_x;
              }
              else frames.list[clicked_frame].x = new_x; //allow movement in axis if it hasn't been resized
                
              if(new_height != 0) {
                frames.list[clicked_frame].h = new_height;
                frames.list[clicked_frame].y = new_y;
              }
              else frames.list[clicked_frame].y = new_y; //allow movement in axis if it hasn't been resized
              
              resize_frame(display, &frames.list[clicked_frame]);
              
            }
            else {
              //Moves the window to the specified location if there is no resizing going on.
              frames.list[clicked_frame].x = new_x;
              frames.list[clicked_frame].y = new_y;
              XMoveWindow(display, frames.list[clicked_frame].frame, frames.list[clicked_frame].x, frames.list[clicked_frame].y);
            }
          }
          /*** Resize grips come into effect here ***/
          else {
           //if mode == FLOATING
            if(clicked_widget == frames.list[clicked_frame].l_grip) {
              new_width = frames.list[clicked_frame].w + (frames.list[clicked_frame].x - new_x);
            }
            else if(clicked_widget == frames.list[clicked_frame].r_grip) {
              new_width = mouse_root_x - frames.list[clicked_frame].x ;
              new_x = frames.list[clicked_frame].x;
            }
            else if(clicked_widget == frames.list[clicked_frame].bl_grip) {
              new_width = frames.list[clicked_frame].w + (frames.list[clicked_frame].x - new_x);
              new_height = mouse_root_y - frames.list[clicked_frame].y;
            }
            else if(clicked_widget == frames.list[clicked_frame].br_grip) {
              new_width = mouse_root_x - frames.list[clicked_frame].x;
              new_height = mouse_root_y - frames.list[clicked_frame].y;
              new_x = frames.list[clicked_frame].x;
            }
            else if(clicked_widget == frames.list[clicked_frame].b_grip) {
              new_height = mouse_root_y - frames.list[clicked_frame].y;
              new_x = frames.list[clicked_frame].x;
            }
            
            if(new_height >= frames.list[clicked_frame].min_height + FRAME_VSPACE
               && new_height <= frames.list[clicked_frame].max_height + FRAME_VSPACE) {
              frames.list[clicked_frame].h = new_height; 
            }
              
            if(new_width >= frames.list[clicked_frame].min_width + FRAME_HSPACE
               && new_width <= frames.list[clicked_frame].max_width) {
              frames.list[clicked_frame].x = new_x;  //for l_grip and bl_grip
              frames.list[clicked_frame].w = new_width;
            }
            resize_frame(display, &frames.list[clicked_frame]);
          }
          XFlush(display);
        }
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
      case ConfigureRequest:        
        printf("ConfigureRequest window %d, w: %d, h %d, ser %d, send %d\n", 
          event.xconfigurerequest.window,
          event.xconfigurerequest.width,
          event.xconfigurerequest.height,
          event.xconfigurerequest.serial, //last event processed
          event.xconfigurerequest.send_event
          );
          
        for(i = 0; i < frames.used; i++) { 
          if(event.xconfigurerequest.window == frames.list[i].window) {
            if(clicked_frame != i) {
              if( event.xconfigurerequest.width >= frames.list[i].min_width + FRAME_HSPACE
               && event.xconfigurerequest.width <= frames.list[i].max_width + FRAME_HSPACE) 
                frames.list[i].w = event.xconfigurerequest.width + FRAME_HSPACE;

              if(  event.xconfigurerequest.height >= frames.list[i].min_height + FRAME_VSPACE
               && event.xconfigurerequest.height  <= frames.list[i].max_height + FRAME_VSPACE)
                frames.list[i].h = event.xconfigurerequest.height + FRAME_VSPACE;
               
              printf("Adjusted width,height: %d %d\n", frames.list[i].w, frames.list[i].h);
              resize_frame(display, &frames.list[i]);
              break;
            }
            else printf("skipped resize request");
          }
        }

        //this window hasn't been mapped yet, let it update it's size
        if(i == frames.used) {
          XWindowAttributes attributes;
          XWindowChanges premap_config; 
          
          XGrabServer(display);
          XSetErrorHandler(supress_xerror);        
          XGetWindowAttributes(display, event.xconfigurerequest.window, &attributes);
          /** This one has me stumped, firefox and open office seem have these bogus 200x200 config requests after the "real" ones **/
          if(event.xcreatewindow.width != 200  && event.xcreatewindow.height != 200) {
            premap_config.width = event.xconfigurerequest.width;
            premap_config.height = event.xconfigurerequest.height;
          }
          else {
            premap_config.width = attributes.width;
            premap_config.height = attributes.height;
          }

          premap_config.x = event.xconfigurerequest.x;
          premap_config.y = event.xconfigurerequest.y;
          premap_config.border_width = 0;
          printf("premap config (%d, %d) width %d, height %d\n", 
            premap_config.x, premap_config.y, premap_config.width, premap_config.height);

          XConfigureWindow(display, event.xconfigurerequest.window, 
            CWX | CWY | CWWidth | CWHeight | CWBorderWidth, &premap_config);
            
          XSetErrorHandler(NULL);
          XUngrabServer(display);
          XFlush(display);
        }
      break;
      case MappingNotify:      
        printf("MappingNotify\n");
      break;
      case ClientMessage:
        printf("Unhandled client message...\n");
      break;
      
      //From StructureNotifyMask on the reparented window, typically self-generated
      case MapNotify:
      case ReparentNotify:      
      case ConfigureNotify: 
      //ignore these events
      break;
      
      default:
        printf("Warning: Unhandled event %d\n", event.type);
      break;
    }
  }

  XDestroyWindow(display, background_window);  
  XFreePixmap(display, background_window_p);
  free_pixmaps(display, &pixmaps);
  free_cursors(display, &cursors);
  
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
void load_pixmaps (Display *display, struct frame_pixmaps *pixmaps) {
  pixmaps->border_p                    = create_pixmap(display, border);  
  pixmaps->light_border_p              = create_pixmap(display, light_border);  //this pixmap is only used for the pressed state of the title_menu
  pixmaps->body_p                      = create_pixmap(display, body);
  pixmaps->titlebar_background_p       = create_pixmap(display, titlebar);
  pixmaps->selection_p                 = create_pixmap(display, selection);
  
  pixmaps->close_button_normal_p       = create_pixmap(display, close_button_normal);
  pixmaps->close_button_pressed_p      = create_pixmap(display, close_button_pressed);
  pixmaps->close_button_deactivated_p  = create_pixmap(display, close_button_deactivated);
  
  pixmaps->pulldown_floating_normal_p  = create_pixmap(display, pulldown_floating_normal);
  pixmaps->pulldown_floating_pressed_p = create_pixmap(display, pulldown_floating_pressed);   
  pixmaps->pulldown_tiling_normal_p    = create_pixmap(display, pulldown_tiling_normal);
  pixmaps->pulldown_tiling_pressed_p   = create_pixmap(display, pulldown_tiling_pressed);
  pixmaps->pulldown_deactivated_p      = create_pixmap(display, pulldown_deactivated);
  
  pixmaps->arrow_normal_p              = create_pixmap(display, arrow_normal);
  pixmaps->arrow_pressed_p             = create_pixmap(display, arrow_pressed);
  pixmaps->arrow_deactivated_p         = create_pixmap(display, arrow_deactivated);
}
void free_pixmaps (Display *display, struct frame_pixmaps *pixmaps) {
  XFreePixmap(display, pixmaps->border_p);
  XFreePixmap(display, pixmaps->light_border_p);
  XFreePixmap(display, pixmaps->body_p);
  XFreePixmap(display, pixmaps->titlebar_background_p);
  XFreePixmap(display, pixmaps->selection_p);
    
  XFreePixmap(display, pixmaps->close_button_normal_p);
  XFreePixmap(display, pixmaps->close_button_pressed_p);
  XFreePixmap(display, pixmaps->close_button_deactivated_p);
    
  XFreePixmap(display, pixmaps->pulldown_floating_normal_p);
  XFreePixmap(display, pixmaps->pulldown_floating_pressed_p);
  XFreePixmap(display, pixmaps->pulldown_tiling_normal_p);
  XFreePixmap(display, pixmaps->pulldown_tiling_pressed_p);
  XFreePixmap(display, pixmaps->pulldown_deactivated_p);

  XFreePixmap(display, pixmaps->arrow_normal_p);
  XFreePixmap(display, pixmaps->arrow_pressed_p);  
  XFreePixmap(display, pixmaps->arrow_deactivated_p);
   
}
void load_cursors (Display *display, struct mouse_cursors *cursors) {
  cursors->normal = XcursorLibraryLoadCursor(display, "left_ptr"); 
  cursors->pressable = XcursorLibraryLoadCursor(display, "hand2"); 
  cursors->hand =    XcursorLibraryLoadCursor(display, "hand1");  //AFAIK  hand1 is the open hand only in DMZ-white/black
  cursors->grab =    XcursorLibraryLoadCursor(display, "fleur");  //AFAIK  fleur is the grabbed hand only in DMZ-white/black
  cursors->resize_h = XcursorLibraryLoadCursor(display, "h_double_arrow");
  cursors->resize_v = XcursorLibraryLoadCursor(display, "double_arrow");
  cursors->resize_tr_bl =  XcursorLibraryLoadCursor(display, "fd_double_arrow");
  cursors->resize_tl_br =  XcursorLibraryLoadCursor(display, "bd_double_arrow");

}
void free_cursors (Display *display, struct mouse_cursors *cursors) {
  XFreeCursor(display, cursors->normal);
  XFreeCursor(display, cursors->pressable);
  XFreeCursor(display, cursors->hand);
  XFreeCursor(display, cursors->grab);
  XFreeCursor(display, cursors->resize_h);
  XFreeCursor(display, cursors->resize_v);
  XFreeCursor(display, cursors->resize_tr_bl);
  XFreeCursor(display, cursors->resize_tl_br);

}
