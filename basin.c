#include <errno.h>
#include <signal.h>
#include <math.h>
#include <X11/extensions/shape.h>
#include "basin.h"

/* These control which printfs are shown */

//#define SHOW_STARTUP                  
//#define SHOW_DESTROY_NOTIFY_EVENT     
//#define SHOW_UNMAP_NOTIFY_EVENT       
//#define SHOW_MAP_REQUEST_EVENT       
//#define SHOW_FRAME_HINTS 
//#define SHOW_CONFIGURE_REQUEST_EVENT  
//#define SHOW_BUTTON_PRESS_EVENT       
//#define SHOW_ENTER_NOTIFY_EVENTS      
//#define SHOW_LEAVE_NOTIFY_EVENTS      
//#define SHOW_FRAME_DROP  
//#define SHOW_EDGE_RESIZE
//#define SHOW_FOCUS_EVENT
//#define SHOW_BUTTON_RELEASE_EVENT     
//#define SHOW_PROPERTIES
//#define SHOW_FRAME_DROP  
//#define SHOW_PROPERTY_NOTIFY
//#define SHOW_CLIENT_MESSAGE
//#define SHOW_FREE_SPACE_STEPS

/**** Configure behaviour *****/
/* This turns on a sort of "sinking" feature by */
//#define ALLOW_INTERPRETIVE_SINKING

/* Sadly, many windows do have a minimum size but do not specify it.  
Small screens then cause the window to be automatically resized below this unspecified amount,
and the window becomes unusable.  
This is a work around hack that incidently disables resizing of those windows. */

//#define ALLOW_OVERSIZE_WINDOWS_WITHOUT_MINIMUM_HINTS

/* This is an old hint which is considered obsolete by the ICCCM. */
//#define ALLOW_POSITION_HINTS 

/* Allows synchronized xlib calls - useful when debugging */
//#define ALLOW_XLIB_DEBUG

/*** basin.c ***/
int supress_xerror    (Display *display, XErrorEvent *event);
void list_properties  (Display *display, Window window);
//int main 
void create_seperators(Display *display, Window *sinking_seperator, Window *tiling_seperator, Window *floating_seperator);
void create_pixmaps   (Display *display, struct Pixmaps *pixmaps);
void create_cursors   (Display *display, struct Cursors *cursors);
void create_hints     (Display *display, struct Atoms *atoms);
void free_pixmaps     (Display *display, struct Pixmaps *pixmaps);
void free_cursors     (Display *display, struct Cursors *cursors);
void set_icon_size    (Display *display, Window window, int new_size);

/*** draw.c ***/
extern Pixmap create_pixmap(Display* display, enum Main_pixmap type);
extern Pixmap create_widget_pixmap(Display *display, enum Widget_pixmap type, enum Widget_state state);
extern Pixmap create_title_pixmap(Display* display, const char* title, enum Title_pixmap type); 

/*** frame-actions.c ***/
extern void check_frame_limits     (Display *display, struct Frame *frame);
extern void change_frame_mode      (Display *display, struct Frame *frame, enum Window_mode mode);
extern void unsink_frame           (Display *display, struct Frame_list *frames, int index);
extern void drop_frame             (Display *display, struct Frame_list *frames, int clicked_frame);

extern void resize_nontiling_frame (Display *display, struct Frame_list *frames, int clicked_frame
, int pointer_start_x, int pointer_start_y, int mouse_root_x, int mouse_root_y
, int r_edge_dx, int b_edge_dy, Window clicked_widget);

extern void move_frame             (Display *display, struct Frame *frame
, int *pointer_start_x, int *pointer_start_y, int mouse_root_x, int mouse_root_y
, struct Pixmaps *pixmaps, int *resize_x_direction, int *resize_y_direction);

extern void resize_tiling_frame    (Display *display, struct Frame_list* frames, int index, char axis, int position, int size);
extern void stack_frame            (Display *display, struct Frame *frame, Window sinking_seperator, Window tiling_seperator, Window floating_seperator);

extern int replace_frame           (Display *display, struct Frame *target
, struct Frame *replacement, Window sinking_seperator, Window tiling_seperator, Window floating_seperator, struct Pixmaps *pixmaps);  


/*** frame.c ***/
extern int create_frame            (Display *display, struct Frame_list* frames
, Window framed_window, struct Pixmaps *pixmaps, struct Cursors *cursors, struct Atoms *atoms);
extern void create_frame_subwindows(Display *display, struct Frame *frame, struct Pixmaps *pixmaps);
extern void remove_frame           (Display *display, struct Frame_list* frames, int index);
extern void close_window           (Display *display, Window framed_window);
extern void free_frame_name        (Display *display, struct Frame *frame);
extern void create_frame_name      (Display *display, struct Frame *frame);
extern void get_frame_hints        (Display *display, struct Frame *frame);
extern void get_frame_wm_hints     (Display *display, struct Frame *frame);
extern void get_frame_type         (Display *display, struct Frame *frame, struct Atoms *atoms);
extern void get_frame_state        (Display *display, struct Frame *frame, struct Atoms *atoms);
extern void resize_frame           (Display *display, struct Frame *frame);
//extern void create_frame_icon_size (Display *display, struct Frame *frame, int new_size);
//extern void free_frame_icon_size   (Display *display, struct Frame *frame);

/*** space.c ***/
extern void add_rectangle                             (struct Rectangle_list *list, struct Rectangle new);
extern void remove_rectangle                          (struct Rectangle_list *list, struct Rectangle old);
extern struct Rectangle_list largest_available_spaces (struct Rectangle_list *used_spaces, int w, int h);
extern double calculate_displacement                  (struct Rectangle source, struct Rectangle dest, int *dx, int *dy);
extern struct Rectangle_list get_free_screen_spaces   (Display *display, struct Frame_list *frames);

/*** menus.c ***/
extern void create_menubar      (Display *display, struct Menubar *menubar, struct Pixmaps *pixmaps, struct Cursors *cursors);
extern void create_mode_menu    (Display *display, struct Mode_menu *mode_menu
, struct Pixmaps *pixmaps, struct Cursors *cursors);
extern void create_title_menu   (Display *display, struct Frame_list *frames, struct Pixmaps *pixmaps, struct Cursors *cursors);
extern void show_title_menu     (Display *display, Window calling_widget, struct Frame_list *frames, int index, int x, int y);
extern void show_mode_menu      (Display *display, Window calling_widget, struct Mode_menu *mode_menu, struct Frame *active_frame, struct Pixmaps *pixmaps, int x, int y);
extern void place_popup_menu    (Display *display, Window calling_widget, Window popup_menu, int x, int y, int width, int height);

/*** focus.c ***/
extern void add_focus           (Window new, struct Focus_list *focus);
extern void remove_focus        (Window old, struct Focus_list *focus);
extern void check_new_frame_focus (Display *display, struct Frame_list *frames, int index);
extern void unfocus_frames      (Display *display, struct Frame_list *frames);
extern void recover_focus       (Display *display, struct Frame_list *frames);

/*** workspaces.c ***/
extern int create_frame_list        (Display *display, struct Workspace_list* workspaces, char *workspace_name, struct Pixmaps *pixmaps, struct Cursors *cursors);
extern void remove_frame_list       (Display *display, struct Workspace_list* workspaces, int index);
extern int create_startup_workspaces(Display *display, struct Workspace_list *workspaces
, Window sinking_seperator, Window tiling_seperator, Window floating_seperator
, struct Pixmaps *pixmaps, struct Cursors *cursors, struct Atoms *atoms);

extern int add_frame_to_workspace(Display *display, struct Workspace_list *workspaces, Window window, int current_workspace
, Window sinking_seperator, Window tiling_seperator, Window floating_seperator
, struct Pixmaps *pixmaps, struct Cursors *cursors, struct Atoms *atoms);

extern void change_to_workspace     (Display *display, struct Workspace_list *workspaces, int *current_workspace, int index, struct Pixmaps *pixmaps);

#include "draw.c"
#include "menus.c"
#include "frame.c"
#include "frame-actions.c"
#include "space.c"
#include "focus.c"
#include "workspace.c"

int done = 0;

void end_event_loop(int sig) {
  done = 1;
}

/* Sometimes a client window is killed before it gets unmapped, we only get the unmapnotify event,
 but there is no way to tell so we just supress the error. */
int supress_xerror(Display *display, XErrorEvent *event) {
  (void) display;
  (void) event;
  //printf("Caught an error\n");
  return 0;
}

int main (int argc, char* argv[]) {
  Display* display = NULL;
  XEvent event;
  Window root;

  Window pulldown; //this is a sort of "pointer" window.  
                   //it has the window ID of the currently open pop-up
                   //this reduces state and allows the currently open pop-up to be removed
                   //from the screen.
  Window sinking_seperator; //this window is always above sunk windows.
  Window tiling_seperator;  //this window is always above tiled windows.
  Window floating_seperator; //this window is always above floating windows.
  
  struct Menubar menubar;  
  struct Pixmaps pixmaps;
  struct Cursors cursors;
  struct Workspace_list workspaces= {0, 16, NULL};
  struct Atoms atoms;

  struct Mode_menu mode_menu; //this window is created and reused.
  
  int do_click_to_focus = 0; //used by EnterNotify and ButtonPress to determine when to set focus
                             //currently requires num_lock to be off

  int grab_move = 0;         //used by EnterNotfy, LeaveNotify and ButtonPress to allow alt+click moves of windows

  int pointer_start_x, pointer_start_y; //used by ButtonPress and motionNotify for window movement arithmetic
  int r_edge_dx, b_edge_dy;             //used by resize_nontiling_frame for maintaining cursor distance from RHS and bottom edges.
  
  int clicked_frame = -1;     //identifies the window being moved/resized by frame index and the frame the title menu was opened on.
  int current_workspace = -1; //determines the workspace the clicked_frame was in, if any.
  Window clicked_widget;      //clicked_widget is used to determine if close buttons etc. should be activated
  int resize_x_direction = 0; //used in motionNotify, 1 means LHS, -1 means RHS 
  int resize_y_direction = 0; //used in motionNotify, 1 means top, -1 means bottom
  int i;
  
  XIconSize *size;  
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
    #ifdef SHOW_STARTUP
    printf("Error: Could not open display.\n\n");
    printf("USAGE: basin\n");
    printf("Set the display variable using: export DISPLAY=\":foo\"\n");
    printf("Where foo is the correct screen\n");
    #endif
    return -1;
  }
  
  #ifdef ALLOW_XLIB_DEBUG
  XSynchronize(display, True);
  #endif
    
  root = DefaultRootWindow(display);

  pulldown = root;
  clicked_widget = root;
   
  XSelectInput(display, root, SubstructureRedirectMask | ButtonMotionMask 
  | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask | FocusChangeMask);
  
  create_pixmaps (display, &pixmaps);
  create_cursors (display, &cursors);
  create_hints(display, &atoms);
  
  create_seperators(display, &sinking_seperator, &tiling_seperator, &floating_seperator);
  create_menubar(display, &menubar, &pixmaps, &cursors);
  create_mode_menu(display, &mode_menu, &pixmaps, &cursors);
  create_startup_workspaces(display, &workspaces, sinking_seperator, tiling_seperator, floating_seperator, &pixmaps, &cursors, &atoms);
  
  change_to_workspace(display, &workspaces, &current_workspace, -1, &pixmaps);
  
  /* Passive grab for alt+click moving windows */
  XGrabButton(display, Button1, Mod1Mask, root, False, ButtonPressMask | ButtonMotionMask
  , GrabModeAsync, GrabModeAsync, None, cursors.grab);
  
  XFlush(display);

  while(!done) {
    //always look for windows that have been destroyed first
    if(XCheckTypedEvent(display, DestroyNotify, &event)) ;
    //then look for unmaps
    else if(XCheckTypedEvent(display, UnmapNotify, &event)) ; 
    else XNextEvent(display, &event);

    if(done) break;
    //these are from the StructureNotifyMask on the reparented window
    /* Resets clicked_frame, pulldown and clicked_widget and uses frames to remove a frame */
    switch(event.type) { 
      case DestroyNotify:  
        #ifdef SHOW_DESTROY_NOTIFY_EVENT
        printf("Destroyed window, ");
        #endif
      case UnmapNotify:
        #ifdef SHOW_UNMAP_NOTIFY_EVENT
        printf("Unmapping: %lu\n", (unsigned long)event.xdestroywindow.window);
        #endif
        
        for(int k = 0; k < workspaces.used; k++) {
          struct Frame_list *frames = &workspaces.list[k];
          for(i = 0; i < frames->used; i++) {
            if(event.xany.window == frames->list[i].window) {
              //printf("Unmapping window %s\n", frames->list[i].window_name);
              XUngrabPointer(display, CurrentTime);
              if(clicked_frame != -1) {
                #ifdef SHOW_UNMAP_NOTIFY_EVENT
                printf("Cancelling resize because a window was destroyed\n");
                #endif
                clicked_frame = -1;
              }
              if(pulldown != root) {
                #ifdef SHOW_UNMAP_NOTIFY_EVENT
                printf("Closing popup and cancelling grab\n");
                #endif
                XUnmapWindow(display, pulldown);
                pulldown = root;
              }
              clicked_widget = root;
              //if(clicked_widget != root) clicked_widget = root;
              #ifdef SHOW_UNMAP_NOTIFY_EVENT
              printf("Removed frame i:%d, framed_window %lu\n", i, (unsigned long)event.xany.window);
              #endif
              if(frames->focus.used > 0  
              && frames->focus.list[frames->focus.used -1] == frames->list[i].window) { 
                remove_focus(frames->list[i].window, &frames->focus);
                unfocus_frames(display, frames);
              }
              //don't bother the focussed window if it wasn't the window being unmapped
              else if(frames->focus.used > 0) remove_focus(frames->list[i].window, &frames->focus);
              
              remove_frame(display, frames, i);
              if(frames->used == 0) {
                #ifdef SHOW_UNMAP_NOTIFY_EVENT
                printf("Removed workspace %d, name %s\n", k, workspaces.list[k].workspace_name);
                #endif
                remove_frame_list(display, &workspaces, k);
                if(workspaces.used != 0) change_to_workspace(display, &workspaces, &current_workspace, 0, &pixmaps);
                else change_to_workspace(display, &workspaces, &current_workspace, -1, &pixmaps);
              }
              break;
            }
          }
          if(i < frames->used) break;
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
          for(k = 0; k < frames->used; k++) if(frames->list[k].window == event.xmaprequest.window) break;
          if(k < frames->used) break; //already exists
        }
        if(i == workspaces.used) {
          int new_workspace;
          XWindowAttributes attributes;
          XGetWindowAttributes(display, event.xmaprequest.window, &attributes);
          if(attributes.override_redirect == False) {
            int used = workspaces.used;
            new_workspace = add_frame_to_workspace(display, &workspaces, event.xmaprequest.window, current_workspace
            , sinking_seperator, tiling_seperator, floating_seperator, &pixmaps, &cursors, &atoms);
            #ifdef SHOW_MAP_REQUEST_EVENT
            printf("new workspace %d\n", new_workspace);
            #endif
            if(used < workspaces.used) { //if it is a newly created workspace
              #ifdef SHOW_MAP_REQUEST_EVENT
              printf("going to new workspace\n");
              #endif
              change_to_workspace(display, &workspaces, &current_workspace, new_workspace, &pixmaps);
            }
          }
          else XRaiseWindow(display, event.xmaprequest.window);
        }
      break;
            
      /* uses grab_move, pointer_start_x, pointer_start_y, resize_x_direction,
         resize_y_direction and do_click_to_focus. */
      case ButtonPress:
        #ifdef SHOW_BUTTON_PRESS_EVENT
        printf("ButtonPress %lu, subwindow %lu\n", (unsigned long)event.xbutton.window, (unsigned long)event.xbutton.subwindow);
        #endif
        resize_x_direction = 0;
        resize_y_direction = 0;
        if(grab_move) {
          for(int k = 0; k < workspaces.used; k++) {
            struct Frame_list *frames = &workspaces.list[k];
            for(i = 0; i < frames->used; i++) if(frames->list[i].frame == event.xbutton.subwindow) {
              #ifdef SHOW_BUTTON_PRESS_EVENT
              printf("started grab_move\n");
              #endif
              clicked_frame = i;
              clicked_widget = root;
              current_workspace = k;
              pointer_start_x = event.xbutton.x - frames->list[i].x;
              pointer_start_y = event.xbutton.y - frames->list[i].y;
              break;
            }
            if(i != frames->used) break;
          }
          break; //end of grab move
        }
        
        if(pulldown != root) {
          break;  //the pulldown has been created, ignore button press.  Only button releases activate menuitems.
        }
        
        /* Menubar menu setup for activation */
        if(event.xbutton.window == menubar.window_menu) {
          #ifdef SHOW_BUTTON_PRESS_EVENT
          printf("Starting to activate the Window menu\n", i);
          #endif

          //A menu is being opened so grab the pointer and intercept the events so that it works.
          XGrabPointer(display,  root, True
          , PointerMotionMask | ButtonPressMask | ButtonReleaseMask
          , GrabModeAsync,  GrabModeAsync, None, cursors.normal, CurrentTime);
          
          clicked_frame = -1;
          clicked_widget = menubar.window_menu;
          break;
        }
        else if(event.xbutton.window == menubar.program_menu) {
          #ifdef SHOW_BUTTON_PRESS_EVENT
          printf("Starting to activate the Program menu\n", i);
          #endif

          //A menu is being opened so grab the pointer and intercept the events so that it works.
          XGrabPointer(display,  root, True
          , PointerMotionMask | ButtonPressMask | ButtonReleaseMask
          , GrabModeAsync,  GrabModeAsync, None, cursors.normal, CurrentTime);
          
          clicked_frame = -1;
          clicked_widget = menubar.program_menu;
          break;
        }

        /** Focus and raising policy, as well as search for the correct frame **/
        else for(int k = 0; k < workspaces.used; k++) {
          struct Frame_list *frames = &workspaces.list[k];
          for(i = 0; i < frames->used; i++) {
            if(event.xbutton.window == frames->list[i].frame
            || event.xbutton.window == frames->list[i].mode_dropdown.mode_hotspot
            || event.xbutton.window == frames->list[i].close_button.close_hotspot
            || event.xbutton.window == frames->list[i].title_menu.hotspot
            || event.xbutton.window == frames->list[i].l_grip  //need to show that these have been disabled for sinking windows.
            || event.xbutton.window == frames->list[i].r_grip
            || event.xbutton.window == frames->list[i].bl_grip
            || event.xbutton.window == frames->list[i].br_grip
            || event.xbutton.window == frames->list[i].b_grip
            || (event.xbutton.window == frames->list[i].backing && do_click_to_focus)) {
            
              stack_frame(display, &frames->list[i], sinking_seperator, tiling_seperator, floating_seperator);

              if(do_click_to_focus) { 
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("clicked inside framed window %d - now focussed\n", i);
                #endif
                //EnterNotify on the framed window triggered a grab which has now intercepted a click.
                //pass on the event
                XAllowEvents(display, ReplayPointer, event.xbutton.time);
                do_click_to_focus = 0;
              }
              
              if(frames->list[i].state == lurking) {
                if(event.xbutton.window == frames->list[i].mode_dropdown.mode_hotspot
                || event.xbutton.window == frames->list[i].close_button.close_hotspot)
                  break; //allow the mode pulldown and close button to be used on lurking windows.
                
                unsink_frame (display, frames, i);
                i = frames->used;
                clicked_frame = -1; 
              }
              
              if(frames->list[i].mode != hidden  &&  frames->list[i].state != lurking) { //above code may not have been able to tile the window
                //FOCUS
                add_focus(frames->list[i].window, &frames->focus);
                unfocus_frames(display, frames);
                recover_focus(display, frames);
              }
              break;
            }
          }
          /** Frame widget press registration. **/
          if(i < frames->used) {
            if(event.xbutton.window == frames->list[i].frame
            || event.xbutton.window == frames->list[i].mode_dropdown.mode_hotspot
            || event.xbutton.window == frames->list[i].title_menu.hotspot)  {
            
              //these are in case the mouse moves and the menu is cancelled.
              //If the mouse is release without moving, the menu's change
              //the grab using XChangeActivePointerGrab.
              XGrabPointer(display,  root, True
              , PointerMotionMask | ButtonPressMask | ButtonReleaseMask
              , GrabModeAsync,  GrabModeAsync, None, cursors.grab, CurrentTime);
              
              pointer_start_x = event.xbutton.x;
              pointer_start_y = event.xbutton.y;
              clicked_frame = i;
              
              if(event.xbutton.window == frames->list[i].mode_dropdown.mode_hotspot
              || event.xbutton.window == frames->list[i].title_menu.hotspot) {
                pointer_start_x += 1; //compensate for relative co-ordinates of window and subwindow
                pointer_start_y += 1;
              }
              
              if(event.xbutton.window == frames->list[i].mode_dropdown.mode_hotspot) {
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("pressed mode pulldown %lu on window %lu\n", frames->list[i].mode_dropdown.mode_hotspot, frames->list[i].frame);
                #endif
                
                clicked_widget = frames->list[i].mode_dropdown.mode_hotspot;
                pointer_start_x += frames->list[i].w - H_SPACING*2 - PULLDOWN_WIDTH - BUTTON_SIZE - EDGE_WIDTH - 1;
                pointer_start_y += V_SPACING;

                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("changing mode pulldown pixmaps\n");
                #endif
                
                if(frames->list[i].mode == floating)
                  XRaiseWindow(display, frames->list[i].mode_dropdown.floating_pressed);
                else if(frames->list[i].mode == tiling)
                  XRaiseWindow(display, frames->list[i].mode_dropdown.tiling_pressed);
                else if(frames->list[i].mode == desktop)
                  XRaiseWindow(display, frames->list[i].mode_dropdown.desktop_pressed);

              }
              else if(event.xbutton.window == frames->list[i].title_menu.hotspot) {
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("pressed title menu window %lu\n", event.xbutton.window);
                #endif
                
                clicked_widget =  frames->list[i].title_menu.hotspot;
                pointer_start_x += H_SPACING*2 + BUTTON_SIZE;
                pointer_start_y += V_SPACING;
                XRaiseWindow(display, frames->list[i].title_menu.arrow.arrow_pressed);
                XRaiseWindow(display, frames->list[i].title_menu.title_pressed);
              }
              XFlush(display);
              break;
            }
            
            else if(event.xbutton.window == frames->list[i].close_button.close_hotspot) {
              #ifdef SHOW_BUTTON_PRESS_EVENT
              printf("pressed closebutton %lu on window %lu\n", frames->list[i].close_button.close_hotspot, frames->list[i].frame);
              #endif
              
              clicked_widget = frames->list[i].close_button.close_hotspot;
              //XSelectInput(display, frames->list[i].close_button.close_hotspot, 0);
              XGrabPointer(display,  root, True
              , PointerMotionMask | ButtonReleaseMask
              , GrabModeAsync,  GrabModeAsync, None, None, CurrentTime);
              XSync(display, True); //this is required in order to supress the leavenotify event from the grab window
              //XSelectInput(display, frames->list[i].close_button.close_hotspot,  ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);              
              #ifdef SHOW_BUTTON_PRESS_EVENT
              printf("raising window\n");
              #endif
              XRaiseWindow(display, frames->list[i].close_button.close_button_pressed);
              XFlush(display);
              break;
            }
            else 
            if(event.xbutton.window == frames->list[i].l_grip
            || event.xbutton.window == frames->list[i].r_grip
            || event.xbutton.window == frames->list[i].bl_grip
            || event.xbutton.window == frames->list[i].br_grip
            || event.xbutton.window == frames->list[i].b_grip) {
              Cursor resize_cursor;
              
              //these are for when the mouse moves.
              pointer_start_x = event.xbutton.x;
              pointer_start_y = event.xbutton.y;
              r_edge_dx = frames->list[i].x + frames->list[i].w - event.xbutton.x_root;
              b_edge_dy = frames->list[i].y + frames->list[i].h - event.xbutton.y_root;
              clicked_frame = i;
              //current_workspace = k;
              
              if(event.xbutton.window == frames->list[i].l_grip) {
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("pressed l_grip on window %lu\n", frames->list[i].frame);
                #endif
                clicked_widget = frames->list[i].l_grip;
                pointer_start_y += TITLEBAR_HEIGHT + 1 + EDGE_WIDTH*2;
                resize_cursor = cursors.resize_h;
              }
              else if(event.xbutton.window == frames->list[i].r_grip) {
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("pressed r_grip on window %lu\n", (unsigned long)frames->list[i].frame);
                #endif
                clicked_widget = frames->list[i].r_grip;
                resize_cursor = cursors.resize_h;
              }
              else if(event.xbutton.window == frames->list[i].bl_grip) {
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("pressed bl_grip on window %lu\n", frames->list[i].frame);
                #endif
                clicked_widget = frames->list[i].bl_grip;
                pointer_start_y += frames->list[i].h - CORNER_GRIP_SIZE;
                resize_cursor = cursors.resize_tr_bl;
              }
              else if(event.xbutton.window == frames->list[i].br_grip) {
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("pressed bl_grip on window %lu\n", frames->list[i].frame);
                #endif
                clicked_widget = frames->list[i].br_grip;
                resize_cursor = cursors.resize_tl_br;
              }
              else if(event.xbutton.window == frames->list[i].b_grip) {
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("pressed bl_grip on window %lu\n", frames->list[i].frame);
                #endif
                clicked_widget = frames->list[i].b_grip;
                resize_cursor = cursors.resize_v;
              }
              XGrabPointer(display, frames->list[i].l_grip, False, PointerMotionMask|ButtonReleaseMask
              , GrabModeAsync,  GrabModeAsync, None, resize_cursor, CurrentTime);
            }
            break;
          }
        }
        //would like to know when these are being used.
        XFlush(display);
        if(pulldown != root) {
          XAllowEvents(display, AsyncPointer, event.xbutton.time);
          break;
        }
        XAllowEvents(display, ReplayPointer, event.xbutton.time);
        
      break;
      
      /*modifies grab_move and do_click_to focus */
      case EnterNotify:
        #ifdef SHOW_ENTER_NOTIFY_EVENTS
        printf("EnterNotify on Window %d, Subwindow %d\n", event.xcrossing.window, event.xcrossing.subwindow);
        #endif
        if(event.xcrossing.mode == NotifyGrab) {
          if(event.xcrossing.state & Mod1Mask) {
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
          }
          break;
        }
      /*this continues from above.  grab_move, clicked_frame. background_window, clicked_widget, frames and pixmaps*/
      /*It ends the alt grab move and resets pixmaps on widgets */
      case LeaveNotify:
        #ifdef SHOW_ENTER_NOTIFY_EVENTS
        if(event.type == LeaveNotify)
          printf("LeaveNotify on Window %d, Subwindow %d\n"
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
            drop_frame(display, frames, clicked_frame);
          }
          clicked_frame = -1;
        }
          
        if(clicked_widget != root  &&  event.xcrossing.window == clicked_widget  &&  pulldown == root)  { //is this mutually exclusive with pulldown code bellow?
          #ifdef SHOW_LEAVE_NOTIFY_EVENTS
          printf("Enter or exit.  Window %lu, Subwindow %lu\n", event.xcrossing.window, event.xcrossing.subwindow);
          #endif
          for(int k = 0; k < workspaces.used; k++) {
            struct Frame_list *frames = &workspaces.list[k];
            for (i = 0; i < frames->used; i++) {
              if(clicked_widget == frames->list[i].close_button.close_hotspot) {
                //XUnmapWindow(display, frames->list[i].close_button.close_button);
                //do not need to unselect for enter/leave notify events on the close button graphic because the hotspot window isn't effected
                if(event.type == EnterNotify)
                  XRaiseWindow(display,  frames->list[i].close_button.close_button_pressed);
                else if (event.type == LeaveNotify)
                  XRaiseWindow(display,  frames->list[i].close_button.close_button_normal);
                XFlush(display);
                break;
              }
            }
            if(i != frames->used) break;
          }
        }
        else if(clicked_widget != root  &&  current_workspace != -1  &&  pulldown != root) { //clicked frame checked?
          struct Frame_list *frames = &workspaces.list[current_workspace];
          if(event.xcrossing.window == mode_menu.floating  &&  clicked_frame != -1) {
            //this prevents enter/leave notify events from being generated
            if(frames->list[clicked_frame].mode == floating) {
              if(event.type == EnterNotify) XRaiseWindow(display, mode_menu.item_floating_active_hover);
              else /* type == LeaveNotify*/ XRaiseWindow(display, mode_menu.item_floating_active);
            }
            else {
              if(event.type == EnterNotify) XRaiseWindow(display, mode_menu.item_floating_hover);
              else /* type == LeaveNotify*/ XRaiseWindow(display, mode_menu.item_floating);
            }
            XFlush(display);
          }
          else if(event.xcrossing.window == mode_menu.tiling  && clicked_frame != -1) {
            if(frames->list[clicked_frame].mode == tiling) {
              if(event.type == EnterNotify) XRaiseWindow(display, mode_menu.item_tiling_active_hover);
              else /* type == LeaveNotify*/ XRaiseWindow(display, mode_menu.item_tiling_active);
            }
            else {
              if(event.type == EnterNotify) XRaiseWindow(display, mode_menu.item_tiling_hover);
              else /* type == LeaveNotify*/ XRaiseWindow(display, mode_menu.item_tiling);
            }
            XFlush(display);
          }
          else if(event.xcrossing.window == mode_menu.desktop &&  clicked_frame != -1) {
            if(frames->list[clicked_frame].mode == desktop) {
              if(event.type == EnterNotify) XRaiseWindow(display, mode_menu.item_desktop_active_hover);
              else /* type == LeaveNotify*/ XRaiseWindow(display, mode_menu.item_desktop_active);
            }
            else {
              if(event.type == EnterNotify) XRaiseWindow(display, mode_menu.item_desktop_hover);
              else /* type == LeaveNotify*/ XRaiseWindow(display, mode_menu.item_desktop);
            }
            XFlush(display);
          }
          else if(event.xcrossing.window == mode_menu.hidden &&  clicked_frame != -1) {
            if(frames->list[clicked_frame].mode == hidden) {
              if(event.type == EnterNotify) XRaiseWindow(display, mode_menu.item_hidden_active_hover);
              else /* type == LeaveNotify*/ XRaiseWindow(display, mode_menu.item_hidden_active);
            }
            else {
              if(event.type == EnterNotify) XRaiseWindow(display, mode_menu.item_hidden_hover);
              else /* type == LeaveNotify*/ XRaiseWindow(display, mode_menu.item_hidden);
            }
            XFlush(display);
          }
          else {
            for(i = 0; i < frames->used; i++) { //if not the mode_pulldown, try the title_menu
              if(event.xcrossing.window == frames->list[i].menu_item.backing) {
                Window hover, normal;
                if(clicked_frame == -1  &&  frames->list[i].mode == tiling) { //from window menu
                  hover = frames->list[i].menu_item.item_title_deactivated_hover;
                  normal = frames->list[i].menu_item.item_title_deactivated;
                }
                else if(i == clicked_frame) {
                  hover = frames->list[i].menu_item.item_title_active_hover;
                  normal = frames->list[i].menu_item.item_title_active;
                }
                else { //make all the windows normal again too
                  hover = frames->list[i].menu_item.item_title_hover;
                  normal = frames->list[i].menu_item.item_title;
                }
                if(event.type == EnterNotify)      XRaiseWindow(display, hover);
                else if (event.type == LeaveNotify)XRaiseWindow(display, normal);
                XFlush(display);
              }
            }
            for(i = 0; i < workspaces.used; i++) {
              if(event.xcrossing.window == workspaces.list[i].workspace_menu.backing) {
                Window hover, normal;
                if(current_workspace == i) { //Set the current workspace title bold
                  hover = workspaces.list[i].workspace_menu.item_title_active_hover;
                  normal = workspaces.list[i].workspace_menu.item_title_active;
                }
                else {
                  hover = workspaces.list[i].workspace_menu.item_title_hover;
                  normal = workspaces.list[i].workspace_menu.item_title;                
                }
                if(event.type == EnterNotify)      XRaiseWindow(display, hover);
                else if (event.type == LeaveNotify)XRaiseWindow(display, normal);
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
        if(pulldown != root) {
          #ifdef SHOW_BUTTON_RELEASE_EVENT
          printf("closed pulldown\n");
          printf("clicked_widget %lu\n", clicked_widget);
          #endif
          XUnmapWindow(display, pulldown);
          
          /* Recover a window with the window menu */
          if(clicked_widget == menubar.window_menu) {
            #ifdef SHOW_BUTTON_RELEASE_EVENT
            printf("Clicked window menu\n");
            #endif
            XRaiseWindow(display, menubar.window_menu_normal);
            XFlush(display);
            for(int k = 0; k < workspaces.used; k++) {
              struct Frame_list *frames = &workspaces.list[k];
              for(i = 0; i < frames->used; i++) if(event.xbutton.window == frames->list[i].menu_item.backing) {
                #ifdef SHOW_BUTTON_RELEASE_EVENT
                printf("Recovering window %s\n", frames->list[i].window_name);
                #endif
                if(frames->list[i].mode != floating) {
                  drop_frame(display, frames, i);
                }
                stack_frame(display, &frames->list[i], sinking_seperator, tiling_seperator, floating_seperator);
                XFlush(display);
                break;
              }
              if(i != frames->used) break;
            }
          }
          /* Change the workspace with the program menu */
          else if(clicked_widget == menubar.program_menu) {
            #ifdef SHOW_BUTTON_RELEASE_EVENT
            printf("Clicked program menu item\n");
            #endif
            XRaiseWindow(display, menubar.program_menu_normal);
            XFlush(display);
            for(int k = 0; k < workspaces.used; k++) {
              if(event.xbutton.window == workspaces.list[k].workspace_menu.backing) {
                #ifdef SHOW_BUTTON_RELEASE_EVENT
                printf("Changing to workspace %s\n", workspaces.list[k].workspace_name);
                #endif
                change_to_workspace(display, &workspaces, &current_workspace, k, &pixmaps);
                break;
              }
            }
          }
          //This crashes the WM if the loop isn't included (and the workspace is just assumed to be the current one)
          //This loop determines if any frame's mode_dropdown or title_menu items have been pressed
          else for(int k = 0; k < workspaces.used; k++) {
            struct Frame_list *frames = &workspaces.list[k];
            for(i = 0; i < frames->used; i++) {
              if(clicked_widget == frames->list[i].mode_dropdown.mode_hotspot) {
                if(event.xbutton.window == mode_menu.floating) 
                  change_frame_mode(display, &frames->list[i], floating);
                else if(event.xbutton.window == mode_menu.hidden) {
                  change_frame_mode(display, &frames->list[i], hidden); //Redrawing mode pulldown
                  //FOCUS    
                  remove_focus(frames->list[i].window, &frames->focus);
                  unfocus_frames(display, frames);
                }
                else if(event.xbutton.window == mode_menu.tiling) {
                  #ifdef SHOW_BUTTON_RELEASE_EVENT
                  printf("retiling frame\n");
                  #endif
                  //this function calls change_frame_mode conditionally.
                  drop_frame(display, frames, clicked_frame);
                }
                else if(event.xbutton.window == mode_menu.desktop) {
                  change_frame_mode(display, &frames->list[i], desktop); //Redrawing mode pulldown                  
                }
                else change_frame_mode(display, &frames->list[i], frames->list[i].mode);
                stack_frame(display, &frames->list[i], sinking_seperator, tiling_seperator, floating_seperator);
                break;
              }
              /* Replace frame with chosen frame*/
              else if(clicked_widget == frames->list[i].title_menu.hotspot) {
                change_frame_mode(display, &frames->list[i], frames->list[i].mode);  //Redrawing title pulldown
                //the first loop find the frame that was clicked.
                //Now we need to identify which window to put here.
                for(int j = 0; j < frames->used; j++) {
                  if(event.xbutton.window == frames->list[j].menu_item.backing) {
                    //FOCUS
                    replace_frame(display, &frames->list[i], &frames->list[j], sinking_seperator, tiling_seperator, floating_seperator, &pixmaps);
                    remove_focus(frames->list[i].window, &frames->focus);
                    add_focus(frames->list[j].window, &frames->focus);
                    unfocus_frames(display, frames);
                    if(k = current_workspace) recover_focus(display, frames);
                    break;
                  }
                }
                break;
              }
            }
            if(i != frames->used) break;
          }
          pulldown = root;
          clicked_widget = root;
          XUngrabPointer(display, CurrentTime);
          XFlush(display);
          clicked_frame = -1;
          break;
        }
        
        /* Activate a widget */
        if((clicked_widget != root) && (clicked_widget == event.xbutton.window)) {
          //need to make sure that opening the menu also records the current workspace TODO
          struct Frame_list *frames = &workspaces.list[current_workspace];
          if(clicked_widget == menubar.window_menu) {
            pulldown = frames->title_menu;
            XRaiseWindow(display, menubar.window_menu_pressed);
            XChangeActivePointerGrab(display, PointerMotionMask | ButtonPressMask | ButtonReleaseMask
            , cursors.normal, CurrentTime);
            XFlush(display);
            show_title_menu(display, menubar.window_menu, frames, -1
            , event.xbutton.x_root - event.xbutton.x, event.xbutton.y_root - event.xbutton.y);
          }
          else if (clicked_widget == menubar.program_menu) {
            #ifdef SHOW_BUTTON_RELEASE_EVENT
            printf("Program menu opening\n");
            printf("current_workspace %d, workspaces.used %d\n", current_workspace, workspaces.used);
            #endif
            pulldown = workspaces.workspace_menu;
            XRaiseWindow(display, menubar.program_menu_pressed);
            
            XChangeActivePointerGrab(display, PointerMotionMask | ButtonPressMask | ButtonReleaseMask
            , cursors.normal, CurrentTime);
            XFlush(display);
            show_workspace_menu(display, menubar.program_menu, &workspaces, current_workspace
            , event.xbutton.x_root - event.xbutton.x - EDGE_WIDTH, event.xbutton.y_root - event.xbutton.y);
          }
          else for(int k = 0; k < workspaces.used; k++) {
            struct Frame_list *frames = &workspaces.list[k];
            for(i = 0; i < frames->used; i++) {
              if(clicked_widget == frames->list[i].close_button.close_hotspot) {
                #ifdef SHOW_BUTTON_RELEASE_EVENT
                printf("released closebutton %lu, window %lu\n", frames->list[i].close_button.close_hotspot, frames->list[i].frame);
                #endif
                XRaiseWindow(display, frames->list[i].close_button.close_button_normal);
                //it is might never happen but this doesn't hurt
                if(pulldown != root) {
                  XUnmapWindow(display, pulldown);
                  pulldown = root;
                  clicked_frame = -1;
                }
                close_window(display, frames->list[i].window);
                clicked_widget = root;
                break;
              }
              else if(clicked_widget == frames->list[i].mode_dropdown.mode_hotspot) {
                #ifdef SHOW_BUTTON_RELEASE_EVENT
                printf("Pressed the mode_pulldown on window %lu\n", frames->list[i].window);
                #endif
                XChangeActivePointerGrab(display, PointerMotionMask | ButtonPressMask | ButtonReleaseMask
                , cursors.normal, CurrentTime);
                pulldown = mode_menu.frame;
                show_mode_menu(display, frames->list[i].mode_dropdown.mode_hotspot, &mode_menu, &frames->list[i], &pixmaps
                , event.xbutton.x_root - event.xbutton.x + EDGE_WIDTH, event.xbutton.y_root - event.xbutton.y - EDGE_WIDTH*2);
                break;
              }
              else if(clicked_widget == frames->list[i].title_menu.hotspot) {
                #ifdef SHOW_BUTTON_RELEASE_EVENT
                printf("Pressed the title menu on window %lu\n", frames->list[i].window);
                #endif
                
                XChangeActivePointerGrab(display
                , PointerMotionMask | ButtonPressMask | ButtonReleaseMask
                , cursors.normal, CurrentTime);
                pulldown = frames->title_menu;
                show_title_menu(display, frames->list[i].title_menu.hotspot, frames, i
                , event.xbutton.x_root - event.xbutton.x, event.xbutton.y_root - event.xbutton.y - EDGE_WIDTH);
                XMapWindow(display, pulldown);
                
                XFlush(display);
                break;
              }
            }
            if(i != frames->used) break;
          }
        }
        
        if(clicked_widget == root
        && clicked_frame != -1
        && current_workspace  != -1
        && workspaces.list[current_workspace].list[clicked_frame].mode == tiling) { //TODO
          struct Frame_list *frames = &workspaces.list[current_workspace];
          #ifdef SHOW_BUTTON_RELEASE_EVENT
          printf("retiling frame\n");
          #endif
          drop_frame(display, frames, clicked_frame);
        }
        
        if(clicked_widget != root) for(int k = 0; k < workspaces.used; k++) {
          struct Frame_list *frames = &workspaces.list[k];
          for(i = 0; i < frames->used; i++) {
            if(clicked_widget == frames->list[i].l_grip
            || clicked_widget == frames->list[i].bl_grip
            || clicked_widget == frames->list[i].b_grip
            || clicked_widget == frames->list[i].br_grip
            || clicked_widget == frames->list[i].r_grip
            || clicked_widget == frames->list[i].close_button.close_hotspot) { //anything except a frame menu
              #ifdef SHOW_BUTTON_RELEASE_EVENT
              printf("Cancelled click\n");
              #endif
              clicked_widget = root;
              break;
            }
          }
        }
        
        if((clicked_frame != -1  &&  current_workspace != -1  &&  pulldown == root)
        || (clicked_widget == root)) {  //Don't ungrab the pointer after opening the pop-up menus
          #ifdef SHOW_BUTTON_RELEASE_EVENT
          printf("Frame move or frame edge resize ended\n");
          #endif
          XUngrabPointer(display, CurrentTime);
          XFlush(display);
          clicked_frame = -1;
        }
      break;
      
      case MotionNotify:
        if(clicked_frame != -1  &&  current_workspace != -1  &&  pulldown == root) {
          //these variables will hold the discarded return values from XQueryPointer
          Window mouse_root, mouse_child;
          
          int mouse_child_x, mouse_child_y;
          int mouse_root_x, mouse_root_y;
          
          unsigned int mask;
          struct Frame_list *frames = &workspaces.list[current_workspace]; //TODO
          //If a menu on the titlebar is dragged, cancel the menu and move the window.
          if(clicked_widget == frames->list[clicked_frame].mode_dropdown.mode_hotspot) {  //cancel the pulldown lists opening
            change_frame_mode(display, &frames->list[clicked_frame], frames->list[clicked_frame].mode);
            clicked_widget = root;
          }
          else
          if(clicked_widget == frames->list[clicked_frame].title_menu.hotspot) { //cancel the pulldown lists opening
            XRaiseWindow(display, frames->list[clicked_frame].title_menu.arrow.arrow_normal);
            XRaiseWindow(display, frames->list[clicked_frame].title_menu.title_normal);
            XFlush(display);
            clicked_widget = root;
          }
          
          while(XCheckTypedEvent(display, MotionNotify, &event)); //skip foward to the latest move event

          XQueryPointer(display, root, &mouse_root, &mouse_child, &mouse_root_x, &mouse_root_y, &mouse_child_x, &mouse_child_y, &mask);
          if(clicked_widget == root) { /*** Move/Squish ***/
            move_frame(display, &frames->list[clicked_frame]
            , &pointer_start_x, &pointer_start_y, mouse_root_x, mouse_root_y
            , &pixmaps, &resize_x_direction, &resize_y_direction);
          }
          else {  /*** Resize grips are being dragged ***/
            //clicked_widget is set to one of the grips.
            resize_nontiling_frame(display, frames, clicked_frame
            , pointer_start_x, pointer_start_y, mouse_root_x, mouse_root_y
            , r_edge_dx, b_edge_dy, clicked_widget);
          }
        }
      break;

      case PropertyNotify:
        for(int k = 0; k < workspaces.used; k++) {
          struct Frame_list *frames = &workspaces.list[k];
          for(i = 0; i < frames->used; i++) {
            if(event.xproperty.window == frames->list[i].window) {
              if( event.xproperty.atom == XA_WM_NAME) {
                create_frame_name(display, &frames->list[i]);
                #ifdef SHOW_PROPERTY_NOTIFY
                printf("resize property\n");
                #endif
                resize_frame(display, &frames->list[i]);
              }
              else if ( event.xproperty.atom == XA_WM_NORMAL_HINTS) {
                get_frame_hints(display, &frames->list[i]);
              }
              break;
            }
          }
        }
      break;

      case ConfigureRequest:
        #ifdef SHOW_CONFIGURE_REQUEST_EVENT
        printf("ConfigureRequest window %lu, w: %d, h %d, ser %lu, send %d\n", 
          event.xconfigurerequest.window,
          event.xconfigurerequest.width,
          event.xconfigurerequest.height,
          event.xconfigurerequest.serial, //last event processed
          event.xconfigurerequest.send_event);
        #endif
        for(int k = 0; k < workspaces.used; k++) {
          struct Frame_list *frames = &workspaces.list[k];          
          for(i = 0; i < frames->used; i++) { 
            if(event.xconfigurerequest.window == frames->list[i].window) {
              //ignore programs resize request if
              #ifdef SHOW_CONFIGURE_REQUEST_EVENT
              printf("Configure window: %s\n", frames->list[i].window_name);
              #endif
              if ((clicked_frame == i  &&  pulldown == root) //this window is being resized or if
              || frames->list[i].mode == tiling) {
                //TODO  figure out how to handle tiled windows enlarging themselves.
                #ifdef SHOW_CONFIGURE_REQUEST_EVENT
                printf("Ignoring config req., due to ongoing resize operation or tiled window\n");
                #endif
                break;
              }

              frames->list[i].w = event.xconfigurerequest.width + FRAME_HSPACE;
              frames->list[i].h = event.xconfigurerequest.height + FRAME_VSPACE;
              if(frames->list[i].mode != desktop) check_frame_limits(display, &frames->list[i]);
                                            
              #ifdef SHOW_CONFIGURE_REQUEST_EVENT
              printf("new width %d, new height %d\n", frames->list[i].w, frames->list[i].h);
              #endif
              if(frames->list[i].mode != tiling
              && k == current_workspace              
              && (event.xconfigurerequest.detail == Above  
                 ||  event.xconfigurerequest.detail == TopIf)) {
                #ifdef SHOW_CONFIGURE_REQUEST_EVENT
                printf("Recovering window in response to possible restack request\n");
                //it would be better to try not to refocus unnecessaraly.
                #endif
                
                if(frames->list[i].mode == hidden) {
                  change_frame_mode(display, &frames->list[i], floating);
                }
                stack_frame(display, &frames->list[i], sinking_seperator, tiling_seperator, floating_seperator);
              }
              resize_frame(display, &frames->list[i]);
              break;
            }
          }
          //this window hasn't been mapped yet, let it update it's size
          if(i == frames->used) {
            XWindowAttributes attributes;
            XWindowChanges premap_config; 
            
            XGrabServer(display);
            XSetErrorHandler(supress_xerror);        
            XGetWindowAttributes(display, event.xconfigurerequest.window, &attributes);
            /** This one has me stumped, firefox and open office seem have these bogus 200x200 config requests after the "real" ones **/
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
            XSetErrorHandler(NULL);
            XUngrabServer(display);
            XFlush(display);
          }
          if(i != frames->used) break;
        }
      break;
      case FocusIn:
        XCheckTypedEvent(display, FocusIn, &event); //if root has focus
        #ifdef SHOW_FOCUS_EVENT
        printf("Recovering and resetting focus \n");
        #endif
        recover_focus(display, &workspaces.list[current_workspace]);
    	break;
    	case FocusOut:
    	  #ifdef SHOW_FOCUS_EVENT
        printf("Warning: Unhandled FocusOut event\n");    	
        #endif
    	break;
      case MappingNotify:
      
      break;
      case ClientMessage:
        #ifdef SHOW_CLIENT_MESSAGE
        printf("Warning: Unhandled client message.\n");
        #endif
      break;
      //From StructureNotifyMask on the reparented window, typically self-generated
      case MapNotify:
      case ReparentNotify:
      case ConfigureNotify:
      //ignore these events
      break;
      
      default:
        //printf("Warning: Unhandled event %d\n", event.type);
      break;
    }
  }

  if(pulldown != root) XUnmapWindow(display, pulldown);

  for(int k = 0; k < workspaces.used; k++) remove_frame_list(display, &workspaces, k);
    
  XDestroyWindow(display, mode_menu.frame);
  XDestroyWindow(display, menubar.border);
  XDestroyWindow(display, workspaces.workspace_menu);
  
  free_pixmaps(display, &pixmaps);
  free_cursors(display, &cursors);
  
    
  XCloseDisplay(display);
  
  printf(".......... \n");
  return 1;
}

void create_seperators(Display *display, Window *sinking_seperator, Window *tiling_seperator, Window *floating_seperator) {
  XSetWindowAttributes set_attributes;
  Window root = DefaultRootWindow(display);
  Screen *screen = DefaultScreenOfDisplay(display);

  *tiling_seperator = XCreateWindow(display, root, 0, 0
  , XWidthOfScreen(screen), XHeightOfScreen(screen), 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
  *sinking_seperator = XCreateWindow(display, root, 0, 0
  , XWidthOfScreen(screen), XHeightOfScreen(screen), 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
  *floating_seperator = XCreateWindow(display, root, 0, 0
  , XWidthOfScreen(screen), XHeightOfScreen(screen), 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);

  set_attributes.override_redirect = True;
  XChangeWindowAttributes(display, *sinking_seperator, CWOverrideRedirect, &set_attributes);
  XChangeWindowAttributes(display, *tiling_seperator, CWOverrideRedirect, &set_attributes);
  XChangeWindowAttributes(display, *floating_seperator, CWOverrideRedirect, &set_attributes);
  XRaiseWindow(display, *sinking_seperator);
  XRaiseWindow(display, *tiling_seperator);
  XRaiseWindow(display, *floating_seperator);
  XFlush(display);
}

void create_pixmaps (Display *display, struct Pixmaps *pixmaps) {
  pixmaps->border_p                     = create_pixmap(display, border);
  pixmaps->light_border_p               = create_pixmap(display, light_border);
  pixmaps->body_p                       = create_pixmap(display, body);
  pixmaps->titlebar_background_p        = create_pixmap(display, titlebar);
  
  pixmaps->selection_indicator_active_p = create_widget_pixmap(display, selection_indicator, active);
  pixmaps->selection_indicator_normal_p = create_widget_pixmap(display, selection_indicator, normal);

  pixmaps->arrow_normal_p               = create_widget_pixmap(display, arrow, normal);
  pixmaps->arrow_pressed_p              = create_widget_pixmap(display, arrow, pressed);
  pixmaps->arrow_deactivated_p          = create_widget_pixmap(display, arrow, deactivated);
    
  pixmaps->pulldown_floating_normal_p      = create_widget_pixmap(display, pulldown_floating, normal);
  pixmaps->pulldown_floating_pressed_p     = create_widget_pixmap(display, pulldown_floating, pressed);
  pixmaps->pulldown_floating_deactivated_p = create_widget_pixmap(display, pulldown_floating, deactivated);

  pixmaps->pulldown_tiling_normal_p        = create_widget_pixmap(display, pulldown_tiling, normal);
  pixmaps->pulldown_tiling_pressed_p       = create_widget_pixmap(display, pulldown_tiling, pressed);
  pixmaps->pulldown_tiling_deactivated_p   = create_widget_pixmap(display, pulldown_tiling, deactivated);

  pixmaps->pulldown_desktop_normal_p       = create_widget_pixmap(display, pulldown_desktop, normal);
  pixmaps->pulldown_desktop_pressed_p      = create_widget_pixmap(display, pulldown_desktop, pressed);
  pixmaps->pulldown_desktop_deactivated_p  = create_widget_pixmap(display, pulldown_desktop, deactivated);
  
  pixmaps->close_button_normal_p           = create_widget_pixmap(display, close_button, normal);
  pixmaps->close_button_pressed_p          = create_widget_pixmap(display, close_button, pressed);
  //pixmaps->close_button_deactivated_p      = create_widget_pixmap(display, close_button, deactivated);
  
}

void free_pixmaps (Display *display, struct Pixmaps *pixmaps) {
  XFreePixmap(display, pixmaps->border_p);
  XFreePixmap(display, pixmaps->light_border_p);
  XFreePixmap(display, pixmaps->body_p);
  XFreePixmap(display, pixmaps->titlebar_background_p);

  XFreePixmap(display, pixmaps->selection_indicator_active_p);
  XFreePixmap(display, pixmaps->selection_indicator_normal_p);
  
  XFreePixmap(display, pixmaps->arrow_normal_p);
  XFreePixmap(display, pixmaps->arrow_pressed_p);
  XFreePixmap(display, pixmaps->arrow_deactivated_p);

  XFreePixmap(display, pixmaps->pulldown_floating_normal_p);
  XFreePixmap(display, pixmaps->pulldown_floating_pressed_p);
  XFreePixmap(display, pixmaps->pulldown_floating_deactivated_p);
  
  XFreePixmap(display, pixmaps->pulldown_tiling_normal_p);
  XFreePixmap(display, pixmaps->pulldown_tiling_pressed_p);
  XFreePixmap(display, pixmaps->pulldown_tiling_deactivated_p);
  
  XFreePixmap(display, pixmaps->pulldown_desktop_normal_p);
  XFreePixmap(display, pixmaps->pulldown_desktop_pressed_p);
  XFreePixmap(display, pixmaps->pulldown_desktop_deactivated_p);

  XFreePixmap(display, pixmaps->close_button_normal_p);
  XFreePixmap(display, pixmaps->close_button_pressed_p);
  //XFreePixmap(display, pixmaps->close_button_deactivated_p);
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

  unsigned char *ewmh_atoms = (unsigned char *)&(atoms->supporting_wm_check);
  int number_of_atoms = 0;
  static int desktops = 1;
  //this window is closed automatically by X11 when the connection is closed.
  //this is supposed to be used to save the required flags. TODO review this
  Window program_instance = XCreateSimpleWindow(display, root, 0, 0, 1, 1, 0, BlackPixelOfScreen(screen), BlackPixelOfScreen(screen));
  
  number_of_atoms++; atoms->supported                  = XInternAtom(display, "_NET_SUPPORTED", False);
  number_of_atoms++; atoms->supporting_wm_check        = XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", False);
  number_of_atoms++; atoms->number_of_desktops         = XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", False);
  number_of_atoms++; atoms->desktop_geometry           = XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", False);
  number_of_atoms++; atoms->wm_full_placement          = XInternAtom(display, "_NET_WM_FULL_PLACEMENT", False);
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
  XChangeProperty(display, root, atoms->supported, XA_ATOM, 32, PropModeReplace, ewmh_atoms, number_of_atoms);
  //let clients know that a ewmh complient window manager is running
  XChangeProperty(display, root, atoms->supporting_wm_check, XA_WINDOW, 32, PropModeReplace, (unsigned char *)&program_instance, 1);
  XChangeProperty(display, root, atoms->number_of_desktops, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&desktops, 1);
  #ifdef SHOW_STARTUP
  list_properties(display, root);
  #endif
  
}
