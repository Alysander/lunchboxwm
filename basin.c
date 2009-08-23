#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <X11/extensions/shape.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>  //This is used for the size hints structure
#include <X11/Xcursor/Xcursor.h>
#include <X11/extensions/shape.h>
#include <X11/Xatom.h>
#include <errno.h>
#include <unistd.h> //for sleep()

#include "basin.h"

#include "space.h"
#include "xcheck.h"
#include "theme.h"
#include "menus.h"
#include "frame.h"
#include "frame-actions.h"

//int main 
void list_properties  (Display *display, Window window);
void create_seperators(Display *display, struct Seperators *seps);
void create_cursors   (Display *display, struct Cursors *cursors);
void create_hints     (Display *display, struct Atoms *atoms);
void free_cursors     (Display *display, struct Cursors *cursors);
void set_icon_size    (Display *display, Window window, int new_size);

#include "focus.h"
#include "workspace.h"
int done = 0;

void end_event_loop(int sig) {
  #ifdef SHOW_SIG
  printf("Caught signal %d\n", sig);
  #endif
  done = 1;
}

int main (int argc, char* argv[]) {
  Display* display = NULL;
  XEvent event; /* 96 bytes per events */
  Window root;

  Window pulldown = 0; //this is a sort of "pointer" window.  
                   //it has the window ID of the currently open pop-up
                   //this reduces state and allows the currently open pop-up to be removed
                   //from the screen.

  struct Seperators seps;
  struct Menubar menubar;    
  struct Themes *themes = NULL;
  struct Cursors cursors;
  struct Workspace_list workspaces = {.used = 0, .max = 16, .list = NULL};
  struct Atoms atoms;

  struct Mode_menu mode_menu;  //this menu is created and reused for all framed windows.
  struct Popup_menu title_menu;//this menu is created and reused across workspaces and framed windows.
  
  int do_click_to_focus = 0;   //used by EnterNotify and ButtonPress to determine when to set focus
                               //TODO currently requires num_lock to be off

  int grab_move = 0;           //used by EnterNotfy, LeaveNotify and ButtonPress to allow alt+click moves of windows

  int pointer_start_x, pointer_start_y; //used by ButtonPress and motionNotify for window movement arithmetic
  int r_edge_dx, b_edge_dy;             //used by resize_frame_grip for maintaining cursor distance from RHS and bottom edges.
  
  int clicked_frame = -1;     //identifies the window being moved/resized by frame index and the frame the title menu was opened on.
  int current_workspace = -1; //determines the workspace the clicked_frame was in, if any.
  Window clicked_widget = 0;  //clicked_widget is used to determine if close buttons etc. should be activated
  int resize_x_direction = 0; //used in motionNotify, 1 means LHS, -1 means RHS 
  int resize_y_direction = 0; //used in motionNotify, 1 means top, -1 means bottom
  int i;                     


  Time last_click_time = CurrentTime;  //this is used for implementing double click
  Window last_click_window = -1;       //this is used for implementing double click
  
  if(signal(SIGINT, end_event_loop) == SIG_ERR) {
    #ifdef SHOW_STARTUP
    printf("\nError: Could not set the error handler\n Is this a POSIX conformant system?\n");
    #endif
    return -1;
  }
  
  #ifdef SHOW_STARTUP
  printf("\nOpening Basin using the DISPLAY environment variable\n");
  printf("This can hang if the wrong screen number is given\n");
  printf("Send SIGINT by pressing ctrl+c to close carefully\n");
  printf("or run the command: \"pkill -INT basin\" \n");
  #endif

  display = XOpenDisplay(NULL);
  if(display == NULL)  {
    printf("Error: Could not open display.\n\n");
    printf("USAGE: basin [theme_name]\n");
    printf("Where theme_name is an optional theme name\n");
    printf("\nSet the DISPLAY environmental variable: export DISPLAY=\":screen_num\"\n");
    printf("Where screen_num is the correct screen number\n");
    return -1;
  }
  
  #if 0
  XSynchronize(display, True);
  #endif
  #ifndef CRASH_ON_BUG  
  XSetErrorHandler(supress_xerror);  
  #endif
  root = DefaultRootWindow(display);
  
  XSelectInput(display, root, SubstructureRedirectMask | ButtonMotionMask 
  | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask | FocusChangeMask);

  if(argc <= 1) themes = create_themes(display, "original");
  else themes = create_themes(display, argv[1]);
  
  if(themes == NULL) {
    printf("Error: Could not load theme \"original\".\n\n");
    return -1;
  }

  create_cursors (display, &cursors); //load a bunch of XCursors
  create_hints(display, &atoms);      //Create EWMH hints which are atoms
  create_seperators(display, &seps);
  create_menubar(display, &menubar, &seps, themes, &cursors);
  create_mode_menu(display, &mode_menu, themes, &cursors);
  create_title_menu(display, &title_menu, themes, &cursors);
  create_startup_workspaces(display, &workspaces, &seps, &title_menu, themes, &cursors, &atoms);
  
  change_to_workspace(display, &workspaces, &current_workspace, -1, themes);

  /* Passive grab for alt+click moving windows Mod2Mask is assumed to be numlock. */

  XGrabButton(display, Button1, Mod1Mask | Mod2Mask, root, False, ButtonPressMask | ButtonMotionMask
  , GrabModeAsync, GrabModeAsync, None, cursors.grab);

  XGrabButton(display, Button1, Mod1Mask, root, False, ButtonPressMask | ButtonMotionMask
  , GrabModeAsync, GrabModeAsync, None, cursors.grab);
  //OR'ing Mod2mask/numlock prevents click to focus and causes some kind of lockup
  
  XFlush(display);

  while(!done) {
    //always look for windows that have been destroyed first
    /* Sometimes closing multiple windows (e.g., exiting GIMP) can cause BadWindows otherwise */
    if(XCheckTypedEvent(display, DestroyNotify, &event)) { ; }
    else if(XCheckTypedEvent(display, UnmapNotify, &event)){ ; }
    else XNextEvent(display, &event);

    if(done) break;
    //these are from the StructureNotifyMask on the reparented window
    /* Resets clicked_frame, pulldown and clicked_widget and uses frames to remove a frame */
    switch(event.type) { 
      case DestroyNotify:  
        #ifdef SHOW_UNMAP_NOTIFY_EVENT
        printf("Destroy window: %lu\n", (unsigned long)event.xdestroywindow.window);
        #endif
      case UnmapNotify:
        #ifdef SHOW_UNMAP_NOTIFY_EVENT
        printf("Unmapping window: %lu\n", (unsigned long)event.xunmap.window);
        #endif
        
        {
          Window removed;
          if(event.type == DestroyNotify) {
            removed = event.xdestroywindow.window;
          }
          else {
            removed = event.xunmap.window;
          }
          for(int k = 0; k < workspaces.used; k++) {
            struct Frame_list *frames = &workspaces.list[k];
            for(i = 0; i < frames->used; i++) {
              if(removed == frames->list[i].framed_window) {
                #ifdef SHOW_UNMAP_NOTIFY_EVENT
                printf("Unmapping window %s\n", frames->list[i].window_name);
                #endif
                XUngrabPointer(display, CurrentTime);
                if(clicked_frame != -1) {
                  #ifdef SHOW_UNMAP_NOTIFY_EVENT
                  printf("Cancelling resize because a window was destroyed\n");
                  #endif
                  clicked_frame = -1;
                }
                if(pulldown) {
                  #ifdef SHOW_UNMAP_NOTIFY_EVENT
                  printf("Closing popup and cancelling grab\n");
                  #endif
                  XUnmapWindow(display, pulldown);
                  pulldown = 0;
                }
                clicked_widget = 0;
                #ifdef SHOW_UNMAP_NOTIFY_EVENT
                printf("Removed frame i:%d, framed_window %lu\n", i, (unsigned long)removed);
                #endif
                if(frames->focus.used > 0  
                && frames->focus.list[frames->focus.used - 1] == frames->list[i].framed_window) { 
                  remove_focus(frames->list[i].framed_window, &frames->focus);
                  unfocus_frames(display, frames);
                }
                //don't bother the focussed window if it wasn't the window being unmapped
                else if(frames->focus.used > 0) remove_focus(frames->list[i].framed_window, &frames->focus);
                
                remove_frame(display, frames, i, themes);
                if(frames->used == 0) {
                  #ifdef SHOW_UNMAP_NOTIFY_EVENT
                  printf("Removed workspace %d, name %s\n", k, workspaces.list[k].workspace_name);
                  #endif
                  remove_frame_list(display, &workspaces, k, themes);
                  if(workspaces.used != 0) change_to_workspace(display, &workspaces, &current_workspace, 0, themes);
                  else change_to_workspace(display, &workspaces, &current_workspace, -1, themes);
                }
                break;
              }
            }
            if(i < frames->used) break;
          }
        }
      break;
      
      /* modifies frames to create a new frame*/
      case MapRequest:
        #ifdef SHOW_MAP_REQUEST_EVENT
        printf("Mapping window %lu\n", (unsigned long)event.xmaprequest.window);
        #endif
        for(i = 0; i < workspaces.used; i++) {
          int k;
          struct Frame_list *frames = &workspaces.list[i];
          for(k = 0; k < frames->used; k++) if(frames->list[k].framed_window == event.xmaprequest.window) break;
          if(k < frames->used) break; //already exists
        }
        if(i == workspaces.used) {
          int new_workspace;
          XWindowAttributes attributes;
          XGetWindowAttributes(display, event.xmaprequest.window, &attributes);
          if(attributes.override_redirect == False) { //TODO investigate whether opening the first workspaces causes it to change to the workspace
            int used = workspaces.used;
            new_workspace = add_frame_to_workspace(display, &workspaces, event.xmaprequest.window, current_workspace
            , &title_menu, &seps, themes, &cursors, &atoms);
            #ifdef SHOW_MAP_REQUEST_EVENT
            printf("new workspace %d\n", new_workspace);
            #endif
            if(used < workspaces.used) { //if it is a newly created workspace
              #ifdef SHOW_MAP_REQUEST_EVENT
              printf("going to new workspace\n");
              #endif
              change_to_workspace(display, &workspaces, &current_workspace, new_workspace, themes);
            }
          }
        }
      break;
            
      /* uses grab_move, pointer_start_x, pointer_start_y, resize_x_direction,
         resize_y_direction and do_click_to_focus. */
      case ButtonPress:
        #ifdef SHOW_BUTTON_PRESS_EVENT
        printf("ButtonPress %lu, subwindow %lu\n", (unsigned long)event.xbutton.window, (unsigned long)event.xbutton.subwindow);
        #endif
        /* Ignore all button presses except for a button1 press */
        #ifdef SHOW_BUTTON_PRESS_EVENT
        printf("state is %d\n", event.xbutton.state);
        #endif          
        if(event.xbutton.button != Button1  &&  !do_click_to_focus && !grab_move) {
          #ifdef SHOW_BUTTON_PRESS_EVENT
          printf("Cancelling click\n");
          #endif
          break;
        }
        
        resize_x_direction = 0;
        resize_y_direction = 0;
        if(grab_move) {
          for(int k = 0; k < workspaces.used; k++) {
            struct Frame_list *frames = &workspaces.list[k];
            for(i = 0; i < frames->used; i++) if(frames->list[i].widgets[frame_parent].widget == event.xbutton.subwindow) {
              #ifdef SHOW_BUTTON_PRESS_EVENT
              printf("started grab_move\n");
              #endif
              clicked_frame = i;
              clicked_widget = 0;
              current_workspace = k;
              pointer_start_x = event.xbutton.x - frames->list[i].x;
              pointer_start_y = event.xbutton.y - frames->list[i].y;

              if(frames->list[i].state == fullscreen) //cancel the fullscreen
                change_frame_state(display, &frames->list[i], none, &seps, themes, &atoms);
                
              break;
            }
            if(i != frames->used) break;
          }
          break; //end of grab move
        }
        
        if(pulldown) {
          break;  //Since the pulldown has been created, ignore button presses.  Only button releases activate menuitems.
        }
        
        /* Menubar menu setup for activation */
        if(event.xbutton.window == menubar.widgets[window_menu].widget) {
          #ifdef SHOW_BUTTON_PRESS_EVENT
          printf("Starting to activate the Window menu\n");
          #endif

          //A menu is being opened so grab the pointer and intercept the events so that it works.
          XGrabPointer(display,  root, True
          , PointerMotionMask | ButtonPressMask | ButtonReleaseMask
          , GrabModeAsync,  GrabModeAsync, None, cursors.normal, CurrentTime);

          clicked_frame = -1;
          clicked_widget = menubar.widgets[window_menu].widget;
          break;
        }
        else if(event.xbutton.window == menubar.widgets[program_menu].widget) {
          #ifdef SHOW_BUTTON_PRESS_EVENT
          printf("Starting to activate the Program menu\n");
          #endif

          //A menu is being opened so grab the pointer and intercept the events so that it works.
          XGrabPointer(display,  root, True
          , PointerMotionMask | ButtonPressMask | ButtonReleaseMask
          , GrabModeAsync,  GrabModeAsync, None, cursors.normal, CurrentTime);

          clicked_frame = -1;
          clicked_widget = menubar.widgets[program_menu].widget;
          break;
        }

        /** Focus and raising policy, as well as search for the correct frame **/
        else for(int k = 0; k < workspaces.used; k++) {
          struct Frame_list *frames = &workspaces.list[k];
          for(i = 0; i < frames->used; i++) {
            if(event.xbutton.window ==  frames->list[i].widgets[frame_parent].widget
            || event.xbutton.window ==  frames->list[i].widgets[mode_dropdown_hotspot].widget
            || event.xbutton.window ==  frames->list[i].widgets[close_button_hotspot].widget
            || event.xbutton.window ==  frames->list[i].widgets[title_menu_hotspot].widget
            || event.xbutton.window ==  frames->list[i].widgets[t_edge].widget
            || event.xbutton.window ==  frames->list[i].widgets[b_edge].widget            
            || event.xbutton.window ==  frames->list[i].widgets[l_edge].widget  //need to show that these have been disabled for sinking windows.
            || event.xbutton.window ==  frames->list[i].widgets[r_edge].widget
            || event.xbutton.window ==  frames->list[i].widgets[tl_corner].widget
            || event.xbutton.window ==  frames->list[i].widgets[tr_corner].widget
            || event.xbutton.window ==  frames->list[i].widgets[bl_corner].widget
            || event.xbutton.window ==  frames->list[i].widgets[br_corner].widget
            || (event.xbutton.window == frames->list[i].widgets[window].widget && do_click_to_focus)) {
              #ifdef SHOW_BUTTON_PRESS_EVENT
              printf("Got a click\n");
              #endif

              stack_frame(display, &frames->list[i], &seps);

              if(do_click_to_focus) { 
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("clicked inside framed window %d - now focussed\n", i);
                #endif
                //EnterNotify on the framed window triggered a grab which has now intercepted a click.
                //pass on the event
                do_click_to_focus = 0;
              }

              if(frames->list[i].mode != hidden ) {
                //FOCUS
                add_focus(frames->list[i].framed_window, &frames->focus);
                unfocus_frames(display, frames);
                recover_focus(display, frames, themes);
              }

              if(!last_click_window  ||  last_click_window != event.xbutton.window) {  //this is the first click
                last_click_time = event.xbutton.time;
                last_click_window = event.xbutton.window;
              }
              else {  //this is the second click, reset
                if( last_click_time   != CurrentTime  
                &&  last_click_window == event.xbutton.window 
                &&  event.xbutton.window != frames->list[i].widgets[window].widget //ignore double click on these
                &&  event.xbutton.window != frames->list[i].framed_window ) {      //ignore double click on these
                  maximize_frame(display, frames, i, themes);
                  XFlush(display);
                }
                last_click_time = CurrentTime;
                last_click_window = 0;
              }
              break;
            }
          }
          /** Frame widget press registration. **/
          if(i < frames->used) {
            if(event.xbutton.window == frames->list[i].widgets[frame_parent].widget
            || event.xbutton.window == frames->list[i].widgets[mode_dropdown_hotspot].widget
            || event.xbutton.window == frames->list[i].widgets[title_menu_hotspot].widget)  {

              #ifdef SHOW_BUTTON_PRESS_EVENT
              printf("Preparing for frame move\n");
              #endif

              //these are in case the mouse moves and the menu is cancelled.
              //If the mouse is release without moving, the menu's change
              //the grab using XChangeActivePointerGrab.
              XGrabPointer(display,  root, True
              , PointerMotionMask | ButtonPressMask | ButtonReleaseMask
              , GrabModeAsync,  GrabModeAsync, None, cursors.grab, CurrentTime);

              pointer_start_x = event.xbutton.x;
              pointer_start_y = event.xbutton.y;
              clicked_frame = i;
              
              if(event.xbutton.window == frames->list[i].widgets[mode_dropdown_hotspot].widget) {
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("pressed mode pulldown %lu on window %lu\n", frames->list[i].widgets[mode_dropdown_hotspot].widget
                , frames->list[i].framed_window);
                #endif
                clicked_widget = frames->list[i].widgets[mode_dropdown_hotspot].widget;
                /**** These are used when the popup is cancelled and a frame move starts ***/

                if(themes->window_type[frames->list[i].theme_type][mode_dropdown_hotspot].x < 0) pointer_start_x += frames->list[i].w;
                if(themes->window_type[frames->list[i].theme_type][mode_dropdown_hotspot].y < 0) pointer_start_y += frames->list[i].h;
                pointer_start_x += themes->window_type[frames->list[i].theme_type][mode_dropdown_hotspot].x;
                pointer_start_y += themes->window_type[frames->list[i].theme_type][mode_dropdown_hotspot].y;


                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("changing mode pulldown pixmaps\n");
                #endif
                xcheck_raisewin(display, frames->list[i].widgets[mode_dropdown_lhs].state[active]);
                xcheck_raisewin(display, frames->list[i].widgets[mode_dropdown_text].state[active]);
                xcheck_raisewin(display, frames->list[i].widgets[mode_dropdown_rhs].state[active]);
              }
              else if(event.xbutton.window == frames->list[i].widgets[title_menu_hotspot].widget) {
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("button press on title menu window %lu\n", event.xbutton.window);
                #endif

                /*** These are used when the popup is cancelled and a frame move starts ***/
                if(themes->window_type[frames->list[i].theme_type][title_menu_hotspot].x < 0) 
                  pointer_start_x += frames->list[i].w;
                if(themes->window_type[frames->list[i].theme_type][title_menu_hotspot].y < 0) 
                  pointer_start_y += frames->list[i].h;
                  
                pointer_start_x += themes->window_type[frames->list[i].theme_type][title_menu_hotspot].x;
                pointer_start_y += themes->window_type[frames->list[i].theme_type][title_menu_hotspot].y;

                clicked_widget =  frames->list[i].widgets[title_menu_hotspot].widget;

                xcheck_raisewin(display, frames->list[i].widgets[title_menu_lhs].state[active]);
//                xcheck_raisewin(display, frames->list[i].widgets[title_menu_icon].state[active]);
                xcheck_raisewin(display, frames->list[i].widgets[title_menu_text].state[active]);
                xcheck_raisewin(display, frames->list[i].widgets[title_menu_rhs].state[active]);
              }
              XFlush(display);
              break;
            }

            else if(event.xbutton.window == frames->list[i].widgets[close_button_hotspot].widget) {
              #ifdef SHOW_BUTTON_PRESS_EVENT
              printf("pressed closebutton %lu on window %lu\n", frames->list[i].widgets[close_button_hotspot].widget
              , frames->list[i].widgets[frame_parent].widget);
              #endif

              clicked_widget = frames->list[i].widgets[close_button_hotspot].widget;
              //XSelectInput(display, frames->list[i].close_button.close_hotspot, 0);
              XFlush(display);
              XSync(display, False);              
              XGrabPointer(display,  root, True
              , PointerMotionMask | ButtonReleaseMask
              , GrabModeAsync,  GrabModeAsync, None, None, CurrentTime);
              XSync(display, True); //this is required in order to supress the leavenotify event from the grab window
              //XSelectInput(display, frames->list[i].close_button.close_hotspot,  ButtonPressMask | ButtonReleaseMask 
              //| EnterWindowMask | LeaveWindowMask);              
              #ifdef SHOW_BUTTON_PRESS_EVENT
              printf("raising window\n");
              #endif
              xcheck_raisewin(display, frames->list[i].widgets[close_button].state[active]);
              XFlush(display);
              break;
            }
            else 
            if(event.xbutton.window == frames->list[i].widgets[l_edge].widget
            || event.xbutton.window == frames->list[i].widgets[r_edge].widget
            || event.xbutton.window == frames->list[i].widgets[t_edge].widget
            || event.xbutton.window == frames->list[i].widgets[tl_corner].widget
            || event.xbutton.window == frames->list[i].widgets[tr_corner].widget
            || event.xbutton.window == frames->list[i].widgets[bl_corner].widget
            || event.xbutton.window == frames->list[i].widgets[br_corner].widget
            || event.xbutton.window == frames->list[i].widgets[b_edge].widget) {
              Cursor resize_cursor;

              #ifdef SHOW_BUTTON_PRESS_EVENT
              printf("Click was on an edge or a corner\n");
              #endif

              //these are for when the mouse moves.
              pointer_start_x = event.xbutton.x;
              pointer_start_y = event.xbutton.y;
              r_edge_dx = frames->list[i].x + frames->list[i].w - event.xbutton.x_root;
              b_edge_dy = frames->list[i].y + frames->list[i].h - event.xbutton.y_root;
              clicked_frame = i;
              //current_workspace = k;
              //  pointer_start_y += TITLEBAR_HEIGHT + 1 + EDGE_WIDTH*2;
              //TODO remove macros and replace with theme position information
              if(event.xbutton.window == frames->list[i].widgets[l_edge].widget) {
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("pressed l_edge on window %lu\n", frames->list[i].widgets[frame_parent].widget);
                #endif
                clicked_widget = frames->list[i].widgets[l_edge].widget;
                // TODO  pointer_start_y += frames->list[i].h + themes->window_type[frames->list[i].theme_type][bl_corner].y;
                resize_cursor = cursors.resize_h;
              }
              else if(event.xbutton.window == frames->list[i].widgets[t_edge].widget) {
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("pressed t_edge on window %lu\n", (unsigned long)frames->list[i].widgets[frame_parent].widget);
                #endif
                clicked_widget = frames->list[i].widgets[t_edge].widget;
                resize_cursor = cursors.resize_v;
              }
              else if(event.xbutton.window == frames->list[i].widgets[tl_corner].widget) {
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("pressed tl_corner on window %lu\n", (unsigned long)frames->list[i].widgets[frame_parent].widget);
                #endif
                clicked_widget = frames->list[i].widgets[tl_corner].widget;
                resize_cursor = cursors.resize_tl_br;
              }
              else if(event.xbutton.window == frames->list[i].widgets[tr_corner].widget) {
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("pressed tr_corner on window %lu\n", (unsigned long)frames->list[i].widgets[frame_parent].widget);
                #endif
                clicked_widget = frames->list[i].widgets[tr_corner].widget;
                resize_cursor = cursors.resize_tr_bl;
              }
              else if(event.xbutton.window == frames->list[i].widgets[r_edge].widget) {
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("pressed r_edge on window %lu\n", (unsigned long)frames->list[i].widgets[frame_parent].widget);
                #endif
                clicked_widget = frames->list[i].widgets[r_edge].widget;
                resize_cursor = cursors.resize_h;
              }
              else if(event.xbutton.window == frames->list[i].widgets[bl_corner].widget) {
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("pressed bl_edge on window %lu\n", frames->list[i].widgets[frame_parent].widget);
                #endif
                clicked_widget = frames->list[i].widgets[bl_corner].widget;
                resize_cursor = cursors.resize_tr_bl;
              }
              else if(event.xbutton.window == frames->list[i].widgets[br_corner].widget) {
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("pressed bl_edge on window %lu\n", frames->list[i].widgets[frame_parent].widget);
                #endif
                clicked_widget = frames->list[i].widgets[br_corner].widget;
                resize_cursor = cursors.resize_tl_br;
              }
              else if(event.xbutton.window == frames->list[i].widgets[b_edge].widget) {
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("pressed bl_edge on window %lu\n", frames->list[i].widgets[frame_parent].widget);
                #endif
                clicked_widget = frames->list[i].widgets[b_edge].widget;
                resize_cursor = cursors.resize_v;
              }
              XGrabPointer(display, frames->list[i].widgets[l_edge].widget, False, PointerMotionMask|ButtonReleaseMask
              , GrabModeAsync,  GrabModeAsync, None, resize_cursor, CurrentTime);
            }
            break;
          }
        }
        //would like to know when these are being used.
        XFlush(display);
        if(pulldown) {
          XAllowEvents(display, AsyncPointer, event.xbutton.time);
          break;
        }
        XAllowEvents(display, ReplayPointer, event.xbutton.time);

      break;

      /*modifies grab_move and do_click_to focus */
      case EnterNotify:
        #ifdef SHOW_ENTER_NOTIFY_EVENTS
        printf("EnterNotify on Window %lu, Subwindow %lu, root is %lu\n", event.xcrossing.window, event.xcrossing.subwindow, root);
        #endif
        /* This makes click to focus work and alt click dragging to move windows */
        if(event.xcrossing.mode == NotifyGrab) {
          if((event.xcrossing.window == root)
          && ((event.xcrossing.state & Mod1Mask) || (event.xcrossing.state & (Mod1Mask | Mod2Mask)))) { //allows numlock to be on or off
            #ifdef SHOW_ENTER_NOTIFY_EVENTS
            printf("set grab_move\n");
            #endif
            grab_move = 1;
          }
          else {
            //this means that a mouse click on the framed window has been detected
            //set the window focus
            #ifdef SHOW_ENTER_NOTIFY_EVENTS
            printf("set do_click_to_focus through enternotify\n");
            #endif
            do_click_to_focus = 1;
            XAllowEvents(display, ReplayPointer, event.xbutton.time);
          }
          break;
        }
      /*this continues from above.  grab_move, clicked_frame. background_window, clicked_widget, frames and thems*/
      /*It ends the alt grab move and resets pixmaps on widgets */
      case LeaveNotify:
        #ifdef SHOW_ENTER_NOTIFY_EVENTS
        if(event.type == LeaveNotify)
          printf("LeaveNotify on Window %lu, Subwindow %lu\n"
          , event.xcrossing.window, event.xcrossing.subwindow);
        #endif
        if(event.type == LeaveNotify  &&  event.xcrossing.mode == NotifyUngrab  &&  grab_move) {
          //ending the grab_move
          struct Frame_list *frames;
          if(current_workspace != -1) frames = &workspaces.list[current_workspace];

          grab_move = 0;

          if(clicked_frame != -1
          &&  current_workspace != -1
          &&  frames->list[clicked_frame].mode == tiling) {
            drop_frame(display, frames, clicked_frame, themes);
          }
          clicked_frame = -1;
        }

        if(clicked_widget &&  event.xcrossing.window == clicked_widget  &&  pulldown)  { //is this mutually exclusive with pulldown code bellow?
          #ifdef SHOW_LEAVE_NOTIFY_EVENTS
          printf("Enter or exit.  Window %lu, Subwindow %lu\n", event.xcrossing.window, event.xcrossing.subwindow);
          #endif
          for(int k = 0; k < workspaces.used; k++) {
            struct Frame_list *frames = &workspaces.list[k];
            for (i = 0; i < frames->used; i++) {
              if(clicked_widget == frames->list[i].widgets[close_button_hotspot].widget) {
                //XUnmapWindow(display, frames->list[i].close_button.close_button);
                //do not need to unselect for enter/leave notify events on the close button graphic because the hotspot window isn't effected
                if(event.type == EnterNotify)
                  xcheck_raisewin(display,  frames->list[i].widgets[close_button].state[active]);
                else if (event.type == LeaveNotify)
                  xcheck_raisewin(display,  frames->list[i].widgets[close_button].state[normal]);
                XFlush(display);
                break;
              }
            }
            if(i != frames->used) break;
          }
        }
        else if(clicked_widget  &&  current_workspace != -1  &&  pulldown) { //clicked frame checked?
          struct Frame_list *frames = &workspaces.list[current_workspace];
          if(event.xcrossing.window == mode_menu.items[floating].item  &&  clicked_frame != -1) {
            //this prevents enter/leave notify events from being generated
            if(frames->list[clicked_frame].mode == floating) {
              if(event.type == EnterNotify) xcheck_raisewin(display, mode_menu.items[floating].state[active_hover]);
              else /* type == LeaveNotify*/ xcheck_raisewin(display, mode_menu.items[floating].state[active]);
            }
            else {
              if(event.type == EnterNotify) xcheck_raisewin(display, mode_menu.items[floating].state[normal_hover]);
              else /* type == LeaveNotify*/ xcheck_raisewin(display, mode_menu.items[floating].state[normal]);
            }
            XFlush(display);
          }
          else if(event.xcrossing.window == mode_menu.items[tiling].item  && clicked_frame != -1) {
            if(frames->list[clicked_frame].mode == tiling) {
              if(event.type == EnterNotify) xcheck_raisewin(display, mode_menu.items[tiling].state[active_hover]);
              else /* type == LeaveNotify*/ xcheck_raisewin(display, mode_menu.items[tiling].state[active]);
            }
            else {
              if(event.type == EnterNotify) xcheck_raisewin(display, mode_menu.items[tiling].state[normal_hover]);
              else /* type == LeaveNotify*/ xcheck_raisewin(display, mode_menu.items[tiling].state[normal]);
            }
            XFlush(display);
          }
          else if(event.xcrossing.window == mode_menu.items[desktop].item &&  clicked_frame != -1) {
            if(frames->list[clicked_frame].mode == desktop) {
              if(event.type == EnterNotify) xcheck_raisewin(display, mode_menu.items[desktop].state[active_hover]);
              else /* type == LeaveNotify*/ xcheck_raisewin(display, mode_menu.items[desktop].state[active]);
            }
            else {
              if(event.type == EnterNotify) xcheck_raisewin(display, mode_menu.items[desktop].state[normal_hover]);
              else /* type == LeaveNotify*/ xcheck_raisewin(display, mode_menu.items[desktop].state[normal]);
            }
            XFlush(display);
          }
          else if(event.xcrossing.window == mode_menu.items[hidden].item &&  clicked_frame != -1) {
            if(frames->list[clicked_frame].mode == hidden) {
              if(event.type == EnterNotify) xcheck_raisewin(display, mode_menu.items[hidden].state[active_hover]);
              else /* type == LeaveNotify*/ xcheck_raisewin(display, mode_menu.items[hidden].state[active]);
            }
            else {
              if(event.type == EnterNotify) xcheck_raisewin(display, mode_menu.items[hidden].state[normal_hover]);
              else /* type == LeaveNotify*/ xcheck_raisewin(display, mode_menu.items[hidden].state[normal]);
            }
            XFlush(display);
          }
          else {
            for(i = 0; i < frames->used; i++) { //if not the mode_pulldown, try the title_menu
              if(event.xcrossing.window == frames->list[i].menu.item) { //this was hotspot to cover icons
                Window hover_w, normal_w;
                //TODO make entries which don't fit look disabled
                //TODO add icons
                if(i == clicked_frame) {
                  hover_w = frames->list[i].menu.state[active_hover];
                  normal_w = frames->list[i].menu.state[active];
                }
                else { //make all the windows normal again too
                  hover_w = frames->list[i].menu.state[normal_hover];
                  normal_w = frames->list[i].menu.state[normal];
                }
                if(event.type == EnterNotify)       xcheck_raisewin(display, hover_w);
                else if (event.type == LeaveNotify) xcheck_raisewin(display, normal_w);
                XFlush(display);
              }
            }
            for(i = 0; i < workspaces.used; i++) {
              if(event.xcrossing.window == workspaces.list[i].workspace_menu.item) { //this was hotspot to cover icons
                Window hover_w, normal_w;
                //TODO add icons
                if(current_workspace == i) { //Set the current workspace title bold
                  hover_w = workspaces.list[i].workspace_menu.state[active_hover];
                  normal_w = workspaces.list[i].workspace_menu.state[active];
                }
                else {
                  hover_w = workspaces.list[i].workspace_menu.state[normal_hover];
                  normal_w = workspaces.list[i].workspace_menu.state[normal];                
                }
                if(event.type == EnterNotify)      xcheck_raisewin(display, hover_w);
                else if (event.type == LeaveNotify)xcheck_raisewin(display, normal_w);
                XFlush(display);
              }
            }
          }
        }
      break;

      case ButtonRelease:
        #ifdef SHOW_BUTTON_RELEASE_EVENT
        printf("ButtonRelease. Window %lu, subwindow %lu, root %lu\n", event.xbutton.window, event.xbutton.subwindow, event.xbutton.root);
        #endif

        /* Close pop-up menu and maybe activate a menu item */
        if(pulldown) {
          #ifdef SHOW_BUTTON_RELEASE_EVENT
          printf("closed pulldown\n");
          printf("clicked_widget %lu\n", clicked_widget);
          #endif
          XUnmapWindow(display, pulldown);

          /* Recover a window with the window menu */
          if(clicked_widget == menubar.widgets[window_menu].widget) {
            #ifdef SHOW_BUTTON_RELEASE_EVENT
            printf("Clicked window menu\n");
            #endif
            xcheck_raisewin(display, menubar.widgets[window_menu].state[normal]);
            XFlush(display);
            for(int k = 0; k < workspaces.used; k++) {
              struct Frame_list *frames = &workspaces.list[k];
              for(i = 0; i < frames->used; i++) if(event.xbutton.window == frames->list[i].menu.item) { //hotspot
                #ifdef SHOW_BUTTON_RELEASE_EVENT
                printf("Recovering window %s\n", frames->list[i].window_name);
                #endif
                if(frames->list[i].mode != floating) {
                  drop_frame(display, frames, i, themes);
                }
                stack_frame(display, &frames->list[i], &seps);
                XFlush(display);
                break;
              }
              if(i != frames->used) break;
            }
          }
          /* Change the workspace with the program menu */
          else if(clicked_widget == menubar.widgets[program_menu].widget) {
            #ifdef SHOW_BUTTON_RELEASE_EVENT
            printf("Clicked program menu item\n");
            #endif
            xcheck_raisewin(display, menubar.widgets[program_menu].state[normal]);
            XFlush(display);
            for(int k = 0; k < workspaces.used; k++) {
              if(event.xbutton.window == workspaces.list[k].workspace_menu.item) { //hotspot
                #ifdef SHOW_BUTTON_RELEASE_EVENT
                printf("Changing to workspace %s\n", workspaces.list[k].workspace_name);
                #endif
                change_to_workspace(display, &workspaces, &current_workspace, k, themes);
                break;
              }
            }
          }
          //This loop determines if any frame's mode_dropdown or title_menu items have been pressed
          else for(int k = 0; k < workspaces.used; k++) {
            struct Frame_list *frames = &workspaces.list[k];
            for(i = 0; i < frames->used; i++) {
              if(clicked_widget == frames->list[i].widgets[mode_dropdown_hotspot].widget) {
                if(event.xbutton.window == mode_menu.items[floating].item) 
                  change_frame_mode(display, &frames->list[i], floating, themes);
                else if(event.xbutton.window == mode_menu.items[hidden].item) {
                  change_frame_mode(display, &frames->list[i], hidden, themes); //Redrawing mode pulldown
                  //FOCUS    
                  remove_focus(frames->list[i].framed_window, &frames->focus);
                  unfocus_frames(display, frames);
                }
                else if(event.xbutton.window == mode_menu.items[tiling].item) {
                  #ifdef SHOW_BUTTON_RELEASE_EVENT
                  printf("retiling frame\n");
                  #endif
                  //this function calls change_frame_mode conditionally.
                  drop_frame(display, frames, clicked_frame, themes);
                }
                else if(event.xbutton.window == mode_menu.items[desktop].item) {
                  change_frame_mode(display, &frames->list[i], desktop, themes); //Redrawing mode pulldown
                }
                else change_frame_mode(display, &frames->list[i], frames->list[i].mode, themes);
                stack_frame(display, &frames->list[i], &seps);
                break;
              }
              /* Handle title menu. This replaces frame with the user's chosen frame */
                else if(clicked_widget == frames->list[i].widgets[title_menu_hotspot].widget) {
                xcheck_raisewin(display, frames->list[i].widgets[title_menu_lhs].state[normal]);
                xcheck_raisewin(display, frames->list[i].widgets[title_menu_text].state[normal]);
                xcheck_raisewin(display, frames->list[i].widgets[title_menu_rhs].state[normal]);
                //the first loop find the frame that was clicked.
                //Now we need to identify which window to put here.
                for(int j = 0; j < frames->used; j++) { //this
                  if(event.xbutton.window == frames->list[j].menu.item) { //hotspot
                    replace_frame(display, &frames->list[i], &frames->list[j], &seps, themes);
                    remove_focus(frames->list[i].framed_window, &frames->focus);
                    add_focus(frames->list[i].framed_window, &frames->focus);
                    unfocus_frames(display, frames);
                    if(k == current_workspace) recover_focus(display, frames, themes);
                    break;
                  }
                }
                break; //end workspace search
              }
            }

          }
          
          pulldown = 0;
          clicked_widget = 0;
          XUngrabPointer(display, CurrentTime);
          XFlush(display);
          clicked_frame = -1;
          break;
        }
        /*** End pulldown handling ***/
        
        /*** ButtonRelease.  If no pulldown was open, try and activate a widget. ***/
        else if(clicked_widget &&  clicked_widget == event.xbutton.window) {
          //need to make sure that opening the menu also records the current workspace TODO
          struct Frame_list *frames = &workspaces.list[current_workspace];
          if(clicked_widget == menubar.widgets[window_menu].widget) {
            pulldown = title_menu.widgets[popup_menu_parent].widget;
            xcheck_raisewin(display, menubar.widgets[window_menu].state[active]);
            XChangeActivePointerGrab(display, PointerMotionMask | ButtonPressMask | ButtonReleaseMask
            , cursors.normal, CurrentTime);
            show_title_menu(display, &title_menu, menubar.widgets[window_menu].widget, frames, -1
            , event.xbutton.x_root - event.xbutton.x, event.xbutton.y_root - event.xbutton.y, themes);
            XFlush(display);
          }
          else if (clicked_widget == menubar.widgets[program_menu].widget  &&  current_workspace != -1) {
            #ifdef SHOW_BUTTON_RELEASE_EVENT
            printf("Program menu opening\n");
            printf("current_workspace %d, workspaces.used %d\n", current_workspace, workspaces.used);
            #endif
            pulldown = workspaces.workspace_menu.widgets[popup_menu_parent].widget;
            xcheck_raisewin(display, menubar.widgets[program_menu].state[active]);

            XChangeActivePointerGrab(display, PointerMotionMask | ButtonPressMask | ButtonReleaseMask
            , cursors.normal, CurrentTime);
            XFlush(display);
            show_workspace_menu(display, menubar.widgets[program_menu].widget, &workspaces, current_workspace
            , event.xbutton.x_root - event.xbutton.x, event.xbutton.y_root - event.xbutton.y, themes);
          }
          else for(int k = 0; k < workspaces.used; k++) {
            struct Frame_list *frames = &workspaces.list[k];
            for(i = 0; i < frames->used; i++) {
              if(clicked_widget == frames->list[i].widgets[close_button_hotspot].widget) {
                #ifdef SHOW_BUTTON_RELEASE_EVENT
                printf("released closebutton %lu, window %lu\n", frames->list[i].widgets[close_button_hotspot].widget
                , frames->list[i].widgets[frame_parent].widget);
                #endif
                xcheck_raisewin(display, frames->list[i].widgets[close_button].state[normal]);
                close_window(display, frames->list[i].framed_window);
                clicked_widget = 0;
                break;
              }
              else if(clicked_widget == frames->list[i].widgets[mode_dropdown_hotspot].widget) {
                #ifdef SHOW_BUTTON_RELEASE_EVENT
                printf("Pressed the mode_pulldown on window %lu\n", frames->list[i].framed_window);
                #endif
                XChangeActivePointerGrab(display, PointerMotionMask | ButtonPressMask | ButtonReleaseMask
                , cursors.normal, CurrentTime);
                pulldown = mode_menu.menu.widgets[popup_menu_parent].widget;
                show_mode_menu(display, frames->list[i].widgets[mode_dropdown_hotspot].widget, &mode_menu, &frames->list[i]
                , event.xbutton.x_root - event.xbutton.x, event.xbutton.y_root - event.xbutton.y);
                break;
              }
              else if(clicked_widget == frames->list[i].widgets[title_menu_hotspot].widget) {
                #ifdef SHOW_BUTTON_RELEASE_EVENT
                printf("Opening the title menu from: %lu\n", clicked_widget);
                #endif

                XChangeActivePointerGrab(display
                , PointerMotionMask | ButtonPressMask | ButtonReleaseMask
                , cursors.normal, CurrentTime);
                pulldown = title_menu.widgets[popup_menu_parent].widget;
                show_title_menu(display, &title_menu, frames->list[i].widgets[title_menu_hotspot].widget, frames, i
                , event.xbutton.x_root - event.xbutton.x, event.xbutton.y_root - event.xbutton.y, themes);

                XFlush(display);
                break;
              }
            }
            if(i != frames->used) break;
          }
        }

        /* If the frame was moved and released */
        if(!clicked_widget
        && clicked_frame != -1
        && current_workspace  != -1
        && workspaces.list[current_workspace].list[clicked_frame].mode == tiling) { //TODO
          struct Frame_list *frames = &workspaces.list[current_workspace];
          #ifdef SHOW_BUTTON_RELEASE_EVENT
          printf("retiling frame\n");
          #endif
          drop_frame(display, frames, clicked_frame, themes);
        }

        if(clicked_widget) for(int k = 0; k < workspaces.used; k++) {
          struct Frame_list *frames = &workspaces.list[k];
          for(i = 0; i < frames->used; i++) {
            if(clicked_widget == frames->list[i].widgets[l_edge].widget
            || clicked_widget == frames->list[i].widgets[bl_corner].widget
            || clicked_widget == frames->list[i].widgets[b_edge].widget
            || clicked_widget == frames->list[i].widgets[br_corner].widget
            || clicked_widget == frames->list[i].widgets[r_edge].widget
            || clicked_widget == frames->list[i].widgets[tl_corner].widget
            || clicked_widget == frames->list[i].widgets[t_edge].widget
            || clicked_widget == frames->list[i].widgets[tr_corner].widget
            || clicked_widget == frames->list[i].widgets[close_button_hotspot].widget) { //anything except a frame menu
              #ifdef SHOW_BUTTON_RELEASE_EVENT
              printf("Cancelled click\n");
              #endif
              if(clicked_widget == frames->list[i].widgets[close_button_hotspot].widget) {
                xcheck_raisewin(display, frames->list[i].widgets[close_button].state[normal]);
                XFlush(display);
              }
              clicked_widget = 0;
              break;
            }
          }
        }

        if((clicked_frame != -1  &&  current_workspace != -1  &&  !pulldown)
        || !clicked_widget) {  //Don't ungrab the pointer after opening the pop-up menus
          #ifdef SHOW_BUTTON_RELEASE_EVENT
          printf("Frame move or frame edge resize ended\n");
          #endif
          XUngrabPointer(display, CurrentTime);
          XFlush(display);
          clicked_frame = -1;
        }
      break;

      case MotionNotify:
        if(clicked_frame != -1  &&  current_workspace != -1  &&  !pulldown) {
          //these variables will hold the discarded return values from XQueryPointer
          Window mouse_root, mouse_child;

          int mouse_child_x, mouse_child_y;
          int mouse_root_x, mouse_root_y;
          //considering making a deadzone
          //also, if a move is large, cancel the last_click_window and last_click_time so it isn't considered a double click
          // (makes a resize flash)
          
          unsigned int mask;
          struct Frame_list *frames = &workspaces.list[current_workspace];
          //If a menu on the titlebar is dragged, cancel the menu and move the window.
          if(clicked_widget == frames->list[clicked_frame].widgets[mode_dropdown_hotspot].widget) {  //cancel the pulldown lists opening
            change_frame_mode(display, &frames->list[clicked_frame], frames->list[clicked_frame].mode, themes);
            clicked_widget = 0;
          }
          else
          if(clicked_widget == frames->list[clicked_frame].widgets[title_menu_hotspot].widget) { //cancel the pulldown lists opening
            xcheck_raisewin(display, frames->list[clicked_frame].widgets[title_menu_lhs].state[normal]);
            xcheck_raisewin(display, frames->list[clicked_frame].widgets[title_menu_icon].state[normal]);
            xcheck_raisewin(display, frames->list[clicked_frame].widgets[title_menu_text].state[normal]);
            xcheck_raisewin(display, frames->list[clicked_frame].widgets[title_menu_rhs].state[normal]);

            XFlush(display);
            clicked_widget = 0;
          }

          while(XCheckTypedEvent(display, MotionNotify, &event)); //skip foward to the latest move event

          XQueryPointer(display, root, &mouse_root, &mouse_child, &mouse_root_x, &mouse_root_y, &mouse_child_x, &mouse_child_y, &mask);
          if(!clicked_widget) { /*** Move ***/
            move_frame(display, &frames->list[clicked_frame]
            , &pointer_start_x, &pointer_start_y, mouse_root_x, mouse_root_y
            , &resize_x_direction, &resize_y_direction, themes);
            //TODO consider deadzone here too
            last_click_window = 0;
            last_click_time = CurrentTime;
          }
          else {  /*** Resize grips are being dragged ***/
            //clicked_widget is set to one of the grips.
            resize_using_frame_grip(display, frames, clicked_frame
            , pointer_start_x, pointer_start_y, mouse_root_x, mouse_root_y
            , r_edge_dx, b_edge_dy, clicked_widget, themes);
            //TODO consider deadzone here too
            last_click_window = 0;
            last_click_time = CurrentTime;
          }
        }
      break;

      case PropertyNotify:
        for(int k = 0; k < workspaces.used; k++) {
          struct Frame_list *frames = &workspaces.list[k];
          for(i = 0; i < frames->used; i++) {
            if(event.xproperty.window == frames->list[i].framed_window) {
              if(event.xproperty.atom == XA_WM_NAME  ||  event.xproperty.atom == atoms.name) {
                create_frame_name(display, &title_menu, &frames->list[i], themes, &atoms);
                resize_frame(display, &frames->list[i], themes);
              }
              else if (event.xproperty.atom == XA_WM_NORMAL_HINTS) {
                /* Ignore normal hints notification for a resizing window. */
                /* For some reason the gimp 2.6.3 on intrepid kept on resetting it's size hints for the toolbox 
                   This lead to the window moving and resizing unpredictably.  */
                if(clicked_frame != i) {
                  get_frame_hints(display, &frames->list[i]); 
                  if(frames->list[i].mode == tiling) { 
                    drop_frame (display, frames, i, themes);
                    resize_frame(display, &frames->list[i], themes);
                    check_frame_limits(display, &frames->list[i], themes);
                  }
                }
              }
              break;
            }
          }
        }
      break;

      case ConfigureRequest:
        #ifdef SHOW_CONFIGURE_REQUEST_EVENT
        printf("ConfigureRequest window %lu, x: %d: y: %d, w: %d, h %d, ser %lu, send %d\n", 
          event.xconfigurerequest.window,
          event.xconfigurerequest.x,
          event.xconfigurerequest.y,
          event.xconfigurerequest.width,
          event.xconfigurerequest.height,
          event.xconfigurerequest.serial, //last event processed
          event.xconfigurerequest.send_event);
        #endif
        {  //declare these outside so we can check if the frame was found or not in any workspace
          int k = 0;
          struct Frame_list *frames = &workspaces.list[k];        
          for(k = 0; k < workspaces.used; k++) {
            frames = &workspaces.list[k];
            for(i = 0; i < frames->used; i++) {
              if(event.xconfigurerequest.window == frames->list[i].framed_window) {
                if(frames->list[i].state == fullscreen) {
                  #ifdef SHOW_CONFIGURE_REQUEST_EVENT
                  printf("Skipping Configure on fullscreen window: %s\n", frames->list[i].window_name);
                  #endif
                  break;
                }

                #ifdef SHOW_CONFIGURE_REQUEST_EVENT
                printf("Configure window: %s\n", frames->list[i].window_name);
                #endif
                  /*
                if ( clicked_frame == i  &&  !pulldown ) { //this window is being resized
                  #ifdef SHOW_CONFIGURE_REQUEST_EVENT
                  printf("Ignoring config req., due to ongoing resize operation or tiled window\n");
                  #endif
                  break;
                }
                  */
                /* TODO, should we allow programs to change their position? */
                //frames->list[i].y = event.xconfigurerequest.x; //leads to gimp-toolbox jumpiness
                //frames->list[i].x = event.xconfigurerequest.y; //leads to gimp-toolbox jumpiness
                frames->list[i].w = event.xconfigurerequest.width + frames->list[i].hspace;
                frames->list[i].h = event.xconfigurerequest.height + frames->list[i].vspace;
                if(frames->list[i].mode == tiling) { 
                  drop_frame (display, frames, i, themes);
                  check_frame_limits(display, &frames->list[i], themes);
                  resize_frame(display, &frames->list[i], themes);
                } 
                else {
                  check_frame_limits(display, &frames->list[i], themes);
                  resize_frame(display, &frames->list[i], themes);
                }
                #ifdef SHOW_CONFIGURE_REQUEST_EVENT
                printf("new width %d, new height %d\n", frames->list[i].w, frames->list[i].h);
                #endif
                /* Raise Window if there has been a restack request */
                if(frames->list[i].mode != tiling
                && k == current_workspace
                && (event.xconfigurerequest.detail == Above || event.xconfigurerequest.detail == TopIf)) {
                  #ifdef SHOW_CONFIGURE_REQUEST_EVENT
                  printf("Recovering window in response to possible restack request\n");
                  #endif

                  if(frames->list[i].mode == hidden) change_frame_mode(display, &frames->list[i], floating, themes);
                  stack_frame(display, &frames->list[i], &seps);
                }
                resize_frame(display, &frames->list[i], themes);
                check_frame_limits(display, &frames->list[i], themes);
                XFlush(display);
                break;
              }
            }
            if(i != frames->used) break; //break out of workspace search
            //this window hasn't been mapped yet, let it update it's size and position
          }
          if(i == frames->used) { //frame not found in any workspace
            XWindowAttributes attributes;
            XWindowChanges premap_config; 

            #ifdef CRASH_ON_BUG
            XGrabServer(display);
            XSetErrorHandler(supress_xerror);
            #endif
            XGetWindowAttributes(display, event.xconfigurerequest.window, &attributes);
            /** Apparently firefox and open office seem have these bogus 200x200 config requests after the "real" ones **/
            if(!(event.xcreatewindow.width == 200  &&  event.xcreatewindow.height == 200)) {
              premap_config.width = event.xconfigurerequest.width;
              premap_config.height = event.xconfigurerequest.height;
            }
            else {
              #ifdef SHOW_CONFIGURE_REQUEST_EVENT
              printf("Bogus 200x200 premap config request\n");
              #endif
              premap_config.width = attributes.width;
              premap_config.height = attributes.height;
            }

            premap_config.x = event.xconfigurerequest.x;
            premap_config.y = event.xconfigurerequest.y;
            premap_config.border_width = 0;

            #ifdef SHOW_CONFIGURE_REQUEST_EVENT
            printf("premap config (%d, %d) width %d, height %d\n"
            , premap_config.x, premap_config.y, premap_config.width, premap_config.height);
            #endif
            XConfigureWindow(display, event.xconfigurerequest.window
            , CWX | CWY | CWWidth | CWHeight | CWBorderWidth, &premap_config);
            XSync(display, False);
            #ifdef CRASH_ON_BUG
            XSetErrorHandler(NULL);
            XUngrabServer(display);
            #endif
            XFlush(display);
          }
        }
      break;
      case FocusIn:
        XCheckTypedEvent(display, FocusIn, &event); //if root has focus
        #ifdef SHOW_FOCUS_EVENT
        printf("Recovering and resetting focus \n");
        #endif
        recover_focus(display, &workspaces.list[current_workspace], themes);
        break;
        case FocusOut:
        #ifdef SHOW_FOCUS_EVENT
        printf("Warning: Unhandled FocusOut event\n");
        #endif
        break;
      case MappingNotify:

      break;
      case ClientMessage:
        /*  FULLSCREEN REQUEST RESPONSE
          window  = the respective client window
          message_type = _NET_WM_STATE
          format = 32
          data.l[0] = the action, as listed below
          data.l[1] = first property to alter
          data.l[2] = second property to alter
          data.l[3] = source indication
          other data.l[] elements = 0
          
          _NET_WM_STATE_REMOVE        0     remove/unset property 
          _NET_WM_STATE_ADD           1     add/set property
          _NET_WM_STATE_TOGGLE        2     toggle property  
          ////
          There should be only one window in fullscreen as overrideredirect windows often appear.
        */
        if(event.xclient.message_type == atoms.wm_state) {
          printf("It is a state change\n");
          for(int k = 0; k < workspaces.used; k++) {
            struct Frame_list *frames = &workspaces.list[k];
            for(i = 0; i < frames->used; i++) {
              if(event.xclient.window == frames->list[i].framed_window) {
                if(event.xclient.data.l[0] == 1) {
                  if((Atom)event.xclient.data.l[1] == atoms.wm_state_fullscreen) {
                    change_frame_state(display, &frames->list[i], fullscreen, &seps, themes, &atoms);
                  }
                  #ifdef SHOW_CLIENT_MESSAGE
                  printf("Adding state\n");
                  #endif
                }
                else if(event.xclient.data.l[0] == 0) {
                  if((Atom)event.xclient.data.l[1] == atoms.wm_state_fullscreen) {
                    change_frame_state(display, &frames->list[i], none, &seps, themes, &atoms);
                  }
                  #ifdef SHOW_CLIENT_MESSAGE
                  printf("Removing state\n");
                  #endif
                }
                else if(event.xclient.data.l[0] == 2) {
                  if((Atom)event.xclient.data.l[1] == atoms.wm_state_fullscreen) {
                    if(frames->list[i].state == none) 
                    change_frame_state(display, &frames->list[i], fullscreen, &seps, themes, &atoms);
                    else if (frames->list[i].state == fullscreen) 
                    change_frame_state(display, &frames->list[i], none, &seps, themes, &atoms);
                  }
                  #ifdef SHOW_CLIENT_MESSAGE
                  printf("Toggling state\n");
                  #endif
                }
                break;
              }  
            }
            if(i < frames->used) break;
          }
        }
        else {
          #ifdef SHOW_CLIENT_MESSAGE
          printf("Warning: Unhandled client message.\n");
          #endif
          ;
        }
      break;
      //From [Sub]structureNotifyMask on the reparented window, typically self-generated
      case MapNotify:
      case ReparentNotify:
      case ConfigureNotify:
        #ifdef SHOW_CONFIGURE_NOTIFY_EVENT
        printf("ConfigureNotify window %lu, x: %d, y %d, w: %d, h %d, ser %lu, send %d\n", 
          event.xconfigure.window,
          event.xconfigure.x,
          event.xconfigure.y,
          event.xconfigure.width,
          event.xconfigure.height,
          event.xconfigure.serial, //last event processed
          event.xconfigure.send_event);
        #endif
      break;

      default:
        //printf("Warning: Unhandled event %d\n", event.type);
      break;
    }
  } //end main event loop

  if(pulldown) {
    XUnmapWindow(display, pulldown);
    pulldown = 0;
  }

  while( workspaces.used > 0 ) remove_frame_list(display, &workspaces, workspaces.used - 1, themes);
  free(workspaces.list);
  
  free_cursors(display, &cursors);

  remove_themes(display,themes);
  
  /* This will close all open windows, but not free any dangling pixmaps. Valgrind won't pickup on leaked pixmaps. */
  XCloseDisplay(display);

  printf(".......... \n");
  return 1;
}

/* When windows are created they are placed under one of these seperators (but on top of previous windows at that level
   The seperators are lowered in case of pre-exising override redirect windows which should be on top.
*/
void create_seperators(Display *display, struct Seperators *seps) {
  XSetWindowAttributes set_attributes;
  Window root = DefaultRootWindow(display);
  Screen *screen = DefaultScreenOfDisplay(display);

  seps->tiling_seperator = XCreateWindow(display, root, 0, 0
  , XWidthOfScreen(screen), XHeightOfScreen(screen), 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
  seps->sinking_seperator = XCreateWindow(display, root, 0, 0
  , XWidthOfScreen(screen), XHeightOfScreen(screen), 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
  seps->floating_seperator = XCreateWindow(display, root, 0, 0
  , XWidthOfScreen(screen), XHeightOfScreen(screen), 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
  seps->panel_seperator = XCreateWindow(display, root, 0, 0
  , XWidthOfScreen(screen), XHeightOfScreen(screen), 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
  
  set_attributes.override_redirect = True;
  XChangeWindowAttributes(display, seps->sinking_seperator, CWOverrideRedirect,  &set_attributes);
  XChangeWindowAttributes(display, seps->tiling_seperator, CWOverrideRedirect,   &set_attributes);
  XChangeWindowAttributes(display, seps->floating_seperator, CWOverrideRedirect, &set_attributes);
  XChangeWindowAttributes(display, seps->panel_seperator, CWOverrideRedirect,    &set_attributes);  

  XRaiseWindow(display, seps->sinking_seperator);
  XRaiseWindow(display, seps->tiling_seperator);
  XRaiseWindow(display, seps->floating_seperator);
  XRaiseWindow(display, seps->panel_seperator);
  XFlush(display);
}


void create_cursors (Display *display, struct Cursors *cursors) {
  cursors->normal       = XcursorLibraryLoadCursor(display, "left_ptr");
  cursors->pressable    = XcursorLibraryLoadCursor(display, "hand2");
  //AFAIK  hand1 is the open hand only in DMZ-white/black
  cursors->hand         = XcursorLibraryLoadCursor(display, "hand1");
  //AFAIK  fleur is the grabbed hand only in DMZ-white/black
  cursors->grab         = XcursorLibraryLoadCursor(display, "fleur");
  cursors->resize_h     = XcursorLibraryLoadCursor(display, "h_double_arrow");
  cursors->resize_v     = XcursorLibraryLoadCursor(display, "double_arrow");
  cursors->resize_tr_bl = XcursorLibraryLoadCursor(display, "fd_double_arrow");
  cursors->resize_tl_br = XcursorLibraryLoadCursor(display, "bd_double_arrow");
}

void free_cursors (Display *display, struct Cursors *cursors) {
  XFreeCursor(display, cursors->normal);
  XFreeCursor(display, cursors->pressable);
  XFreeCursor(display, cursors->hand);
  XFreeCursor(display, cursors->grab);
  XFreeCursor(display, cursors->resize_h);
  XFreeCursor(display, cursors->resize_v);
  XFreeCursor(display, cursors->resize_tr_bl);
  XFreeCursor(display, cursors->resize_tl_br);
}

void list_properties(Display *display, Window window) {
  int total;
  Atom *list = XListProperties(display, window, &total);
  char *name;
  for(int i = 0; i < total; i++) {
    name = NULL;
    name = XGetAtomName(display, list[i]);
    if(name != NULL) { 
      //printf("Property: %s\n", name);  
      XFree(name);
    }
  }
  XFree(list);
}

void create_hints (Display *display, struct Atoms *atoms) {
  Window root = DefaultRootWindow(display);
  Screen* screen = DefaultScreenOfDisplay(display);

  Atom *ewmh_atoms = &(atoms->supporting_wm_check);
  int number_of_atoms = 0;
  static int desktops = 1;

  int32_t desktop_geometry[2] = {XWidthOfScreen(screen), XHeightOfScreen(screen)};

  //this window is closed automatically by X11 when the connection is closed.
  //this is supposed to be used to save the required flags. TODO review this
  Window program_instance = XCreateSimpleWindow(display, root, 0, 0, 1, 1, 0, BlackPixelOfScreen(screen), BlackPixelOfScreen(screen));
  atoms->name  = XInternAtom(display, "_NET_WM_NAME", False);
  number_of_atoms++; atoms->supported                  = XInternAtom(display, "_NET_SUPPORTED", False);
  number_of_atoms++; atoms->supporting_wm_check        = XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", False);
  number_of_atoms++; atoms->number_of_desktops         = XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", False);
  number_of_atoms++; atoms->desktop_geometry           = XInternAtom(display, "_NET_DESKTOP_GEOMETRY", False);
  //_NET_WORKAREA
  //number_of_atoms++; atoms->wm_full_placement          = XInternAtom(display, "_NET_WM_FULL_PLACEMENT", False);
  number_of_atoms++; atoms->frame_extents              = XInternAtom(display, "_NET_FRAME_EXTENTS", False);
  number_of_atoms++; atoms->wm_window_type             = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
  number_of_atoms++; atoms->wm_window_type_normal      = XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", False);
  number_of_atoms++; atoms->wm_window_type_dock        = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);
  number_of_atoms++; atoms->wm_window_type_desktop     = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
  number_of_atoms++; atoms->wm_window_type_splash      = XInternAtom(display, "_NET_WM_WINDOW_TYPE_SPLASH", False);  //no frame
  number_of_atoms++; atoms->wm_window_type_dialog      = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);  //can be transient
  number_of_atoms++; atoms->wm_window_type_utility     = XInternAtom(display, "_NET_WM_WINDOW_TYPE_UTILITY", False); //can be transient
  number_of_atoms++; atoms->wm_state                   = XInternAtom(display, "_NET_WM_STATE", False);
  number_of_atoms++; atoms->wm_state_demands_attention = XInternAtom(display, "_NET_WM_STATE_DEMANDS_ATTENTION", False);
  number_of_atoms++; atoms->wm_state_modal             = XInternAtom(display, "_NET_WM_STATE_MODAL", False);
  number_of_atoms++; atoms->wm_state_fullscreen        = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

  //list all ewmh hints supported.
  XChangeProperty(display, root, atoms->supported, XA_ATOM, 32, PropModeReplace, (unsigned char *)ewmh_atoms, number_of_atoms);
    
  //this is a type
  atoms->utf8 = XInternAtom(display, "UTF8_STRING", False);

  //let clients know that a ewmh complient window manager is running
  XChangeProperty(display, root, atoms->supporting_wm_check, XA_WINDOW, 32, PropModeReplace, (unsigned char *)&program_instance, 1);
  XChangeProperty(display, root, atoms->number_of_desktops, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&desktops, 1);
  XChangeProperty(display, root, atoms->desktop_geometry, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)desktop_geometry, 2);


  #ifdef SHOW_STARTUP
  list_properties(display, root);
  #endif
  
}
