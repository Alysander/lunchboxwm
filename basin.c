#include <errno.h>
#include <signal.h>
#include <math.h>
#include "basin.h"


/* these control which printfs are shown */
/***
#define SHOW_STARTUP                  
#define SHOW_DESTROY_NOTIFY_EVENT     
#define SHOW_UNMAP_NOTIFY_EVENT       
#define SHOW_MAP_REQUEST_EVENT        
#define SHOW_BUTTON_PRESS_EVENT       
#define SHOW_ENTER_NOTIFY_EVENTS      
#define SHOW_LEAVE_NOTIFY_EVENTS      
#define SHOW_CONFIGURE_REQUEST_EVENT  
#define SHOW_EDGE_RESIZE
***/
#define SHOW_BUTTON_RELEASE_EVENT     

/*** basin.c ***/
int supress_xerror (Display *display, XErrorEvent *event);
void list_properties (Display *display, Window window);
//int main 
void load_pixmaps  (Display *display, struct frame_pixmaps *pixmaps);
void load_cursors  (Display *display, struct mouse_cursors *cursors);
void load_hints    (Display *display, struct hints *atoms);
void free_pixmaps  (Display *display, struct frame_pixmaps *pixmaps);
void free_cursors  (Display *display, struct mouse_cursors *cursors);

void create_mode_pulldown_list(Display *display, struct mode_pulldown_list *mode_pulldown
, struct frame_pixmaps *pixmaps, struct mouse_cursors *cursors);
 
void create_title_menu        (Display *display, struct Framelist *frames, struct frame_pixmaps *pixmaps, struct mouse_cursors *cursors);
void show_title_menu          (Display *display, struct Framelist *frames, int x, int y);
void show_mode_pulldown_list  (Display *display, struct mode_pulldown_list *mode_pulldown, struct frame_pixmaps *pixmaps, int x, int y);

/*** events.c ***/
extern void handle_frame_resize (Display *display, struct Framelist *frames, int clicked_frame
, int pointer_start_x, int pointer_start_y, int mouse_root_x, int mouse_root_y, Window clicked_widget);

extern void handle_frame_move (Display *display, struct Framelist *frames, int clicked_frame
, int *pointer_start_x, int *pointer_start_y, int mouse_root_x, int mouse_root_y,  int *was_sunk
, struct frame_pixmaps *pixmaps, int *resize_x_direction, int *resize_y_direction);

extern void handle_frame_retile (Display *display, struct Framelist *frames, int clicked_frame
,  int pointer_start_x, int pointer_start_y, int mouse_root_x, int mouse_root_y);

/*** draw.c ***/
extern Pixmap create_pixmap(Display* display, enum main_pixmap type);
//separate function for the title as it needs the string to draw
extern Pixmap create_title_pixmap(Display* display, const char* title, enum title_pixmap type); 

/*** frame.c ***/
extern int create_frame (Display *display, struct Framelist* frames, Window framed_window
, struct frame_pixmaps *pixmaps, struct mouse_cursors *cursors);

extern void create_startup_frames (Display *display, struct Framelist* frames, struct frame_pixmaps *pixmaps, struct mouse_cursors *cursors);
extern void remove_frame          (Display *display, struct Framelist* frames, int index);
extern void resize_tiling_frame   (Display *display, struct Framelist *frames, int index, char axis, int position, int size);
extern void stack_frame           (Display *display, struct Frame *frame, Window sinking_seperator, Window tiling_seperator);

extern void remove_window         (Display *display, Window framed_window);
extern void get_frame_program_name(Display *display, struct Frame *frame);
extern void free_frame_name       (Display *display, struct Frame *frame);
extern void load_frame_name       (Display *display, struct Frame *frame);
extern void get_frame_hints       (Display *display, struct Frame *frame);
extern void show_frame_state      (Display *display, struct Frame *frame,  struct frame_pixmaps *pixmaps);
extern void resize_frame          (Display *display, struct Frame *frame);
extern int replace_frame          (Display *display, struct Frame *target, struct Frame *replacement, Window sinking_seperator, Window tiling_seperator, struct frame_pixmaps *pixmaps);  

/*** space.c ***/
extern void add_rectangle(struct rectangle_list *list, struct rectangle new);
extern void remove_rectangle(struct rectangle_list *list, struct rectangle old);
extern struct rectangle_list largest_available_spaces (struct rectangle_list *used_spaces, int w, int h);
extern struct rectangle_list get_free_screen_spaces (Display *display, struct Framelist *frames, int start);
extern double calculate_displacement(struct rectangle source, struct rectangle dest);

/*** menubar.c ***/
extern Window create_menubar(Display *display, struct frame_pixmaps *pixmaps, struct mouse_cursors *cursors);

#include "draw.c"
#include "frame.c"
#include "space.c"
#include "menubar.c"
#include "events.c"

int done = False;

void end_event_loop(int sig) {

  Display* display = XOpenDisplay(NULL); 
  Screen* screen = DefaultScreenOfDisplay(display);
  Window root = DefaultRootWindow(display);
  int black = BlackPixelOfScreen(screen);
  Window temp;
  
  done = True;
  
  //give the event loop something to do so that it unblocks    
  temp = XCreateSimpleWindow(display, root, 0, 0
  , XWidthOfScreen(screen), XHeightOfScreen(screen), 0, black, black);
  XMapWindow(display, temp);
  XFlush(display);
  XDestroyWindow(display, temp);
}

void list_properties(Display *display, Window window) {
  int total;
  Atom *list = XListProperties(display, window, &total);
  char *name;
  for(int i = 0; i < total; i++) {
    name = NULL;
    name = XGetAtomName(display, list[i]);
    if(name != NULL) printf("Property: %s\n", name);
    if(name != NULL) XFree(name);
  }
  XFree(list);
}

int main (int argc, char* argv[]) {
  Display* display = NULL; 
  Screen* screen;  
  int screen_number;
  int black;
    
  XEvent event;
  Window root, background_window;
  Window pulldown;
  Window menubar;
  Window sinking_seperator; //this window is always above sunk windows.
  Window tiling_seperator;  //this window is always below tiled windows.
  
  Pixmap background_window_p;   
  
  struct frame_pixmaps pixmaps;
  struct mouse_cursors cursors;
  struct hints atoms;
  struct Framelist frames = {NULL, 16, 0, 0};

  struct mode_pulldown_list mode_pulldown; //this window is created and reused.
  

  int do_click_to_focus = 0; //used by EnterNotify and ButtonPress to determine when to set focus
                             //currently requires num_lock to be off

  int grab_move = 0; //used by EnterNotfy, LeaveNotify and ButtonPress to allow alt+click moves of windows
  
  int pointer_start_x, pointer_start_y; //used by ButtonPress and motionNotify for window movement arithmetic
  int was_sunk = 0; //for showing and cancelling sinking state on extreme squish
  
  int clicked_frame = -1;      //identifies the window being moved/resized by frame index
  Window clicked_widget;       //clicked_widget is used to determine if close buttons etc. should be activated
  
  int resize_x_direction = 0; //used in motionNotify, 1 means LHS, -1 means RHS 
  int resize_y_direction = 0; //used in motionNotify, 1 means top, -1 means bottom
  
  int i; //i is the iterator for the window array 
         //and is also used to check how the loop terminated
    
  frames.list = malloc(sizeof(struct Frame) * frames.max);
  if(frames.list == NULL) {
    #ifdef SHOW_STARTUP
    printf("\nError: No available memory\n");
    #endif
    return -1;
  }
  
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

  //XSynchronize(display, True);  //Turns on synchronized debugging  
  root = DefaultRootWindow(display);
  screen = DefaultScreenOfDisplay(display);
  screen_number = DefaultScreen (display);
  black = BlackPixelOfScreen(screen);
  pulldown = root;
  clicked_widget = root;
  
  XSelectInput(display, root, SubstructureRedirectMask | ButtonMotionMask 
  | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
  
  load_pixmaps (display, &pixmaps);
  load_cursors (display, &cursors);
  load_hints(display, &atoms);

  tiling_seperator = XCreateWindow(display, root, 0, 0
  , XWidthOfScreen(screen), XHeightOfScreen(screen), 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
  XLowerWindow(display, tiling_seperator);

  sinking_seperator = XCreateWindow(display, root, 0, 0
  , XWidthOfScreen(screen), XHeightOfScreen(screen), 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
  XLowerWindow(display, sinking_seperator);

  XFlush(display);
    
  create_title_menu(display, &frames, &pixmaps, &cursors);
  create_startup_frames(display, &frames, &pixmaps, &cursors); 
  menubar = create_menubar(display, &pixmaps, &cursors);
  
  create_mode_pulldown_list(display, &mode_pulldown, &pixmaps, &cursors);
  
  /* Create the background window */
  background_window = XCreateSimpleWindow(display, root, 0, 0
  , XWidthOfScreen(screen), XHeightOfScreen(screen), 0, black, black);
  background_window_p = create_pixmap(display, background);
  XLowerWindow(display, background_window);
  XSetWindowBackgroundPixmap (display, background_window, background_window_p );
  XDefineCursor(display, background_window, cursors.normal);  
  XMapWindow(display, background_window);

  /* Passive alt+click grab for moving windows */
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
        printf("Unmapping: %d\n", event.xdestroywindow.window);
        #endif
        
        for(i = 0; i < frames.used; i++) {
          if(event.xany.window == frames.list[i].window) {
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
            if(clicked_widget != root) clicked_widget = root;
            #ifdef SHOW_UNMAP_NOTIFY_EVENT
            printf("Removed frame i:%d, framed_window %d\n", i, event.xany.window);
            #endif
            
            remove_frame(display, &frames, i);
            break;
          }
        }
      break;
      
      /* modifies frames to create a new frame*/
      case MapRequest:
        #ifdef SHOW_MAP_REQUEST_EVENT
        printf("Mapping window %d\n", event.xmaprequest.window);
        #endif
        
        for(i = 0; i < frames.used; i++) if(frames.list[i].window == event.xmaprequest.window) break;
        if(i == frames.used) create_frame(display, &frames, event.xmaprequest.window, &pixmaps, &cursors);
        XMapWindow(display, event.xmaprequest.window);
        XFlush(display);
      break;
      
      /* uses grab_move, pointer_start_x, pointer_start_y, resize_x_direction,
         resize_y_direction and do_click_to_focus. */
      case ButtonPress:
        #ifdef SHOW_BUTTON_PRESS_EVENT
        printf("ButtonPress %d, subwindow %d\n", event.xbutton.window, event.xbutton.subwindow);
        #endif
        
        if(grab_move) {
          for(i = 0; i < frames.used; i++) if(frames.list[i].frame == event.xbutton.subwindow) {
            #ifdef SHOW_BUTTON_PRESS_EVENT
            printf("started grab_move\n");
            #endif
            
            clicked_frame = i;
            clicked_widget = root;
            pointer_start_x = event.xbutton.x - frames.list[clicked_frame].x;
            pointer_start_y = event.xbutton.y - frames.list[clicked_frame].y;      
            resize_x_direction = 0;
            resize_y_direction = 0;
            break;
          }
          break;
        }
        
        if(pulldown != root) { //the pulldown has been created, ignore button press.  Only button releases activate menuitems.
          break;
        }
        
        /** Focus and raising policy **/
        //Probably the focus history should be in a stack
        for(i = 0; i < frames.used; i++) {
          if(event.xbutton.window == frames.list[i].frame  
          || event.xbutton.window == frames.list[i].mode_pulldown  
          || event.xbutton.window == frames.list[i].close_button   
          || event.xbutton.window == frames.list[i].title_menu.hotspot
          || event.xbutton.window == frames.list[i].l_grip  //need to show that these have been disabled for sinking windows.
          || event.xbutton.window == frames.list[i].r_grip
          || event.xbutton.window == frames.list[i].bl_grip
          || event.xbutton.window == frames.list[i].br_grip
          || event.xbutton.window == frames.list[i].b_grip
          || (event.xbutton.window == frames.list[i].backing && do_click_to_focus)) {
            stack_frame(display, &frames.list[i], sinking_seperator, tiling_seperator);

            //don't tile the sinking window if these are clicked.
            if(event.xbutton.window == frames.list[i].mode_pulldown  
            //|| event.xbutton.window == frames.list[i].title_menu.hotspot
            || event.xbutton.window == frames.list[i].close_button) break; 

            if(do_click_to_focus) { //EnterNotify on the framed window and has now been clicked.
              #ifdef SHOW_BUTTON_PRESS_EVENT            
              printf("clicked inside framed window %d - now focussed\n", i);
              #endif
              
              XAllowEvents(display, ReplayPointer, event.xbutton.time);
              do_click_to_focus = 0; //reset
            }

            if(frames.list[i].mode == SINKING) {
              struct rectangle_list free_spaces;
              int largest = 0;
              unsigned long int largest_area = 0;

              #ifdef SHOW_BUTTON_PRESS_EVENT
              printf("Tiling a previously sunk window\n");
              #endif
              free_spaces = get_free_screen_spaces (display, &frames, 0);
            
              for(int k = 0; k < free_spaces.used; k++) {
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("Free space: x %d, y %d, w %d, h %d\n"
                , free_spaces.list[k].x, free_spaces.list[k].y, free_spaces.list[k].w, free_spaces.list[k].h);
                #endif
                
                if(frames.list[i].min_width  <= free_spaces.list[k].w
                && frames.list[i].min_height <= free_spaces.list[k].h) {
                  if(free_spaces.list[k].w * free_spaces.list[k].h > largest_area) {
                    largest = k;
                    largest_area = free_spaces.list[k].w * free_spaces.list[k].h;
                  }
                }
              }
              if(largest_area != 0) {
                #ifdef SHOW_BUTTON_PRESS_EVENT
                printf("Tiling sinking window\n");
                #endif
                
                //max width might be wrong here
                if(frames.list[i].max_width > free_spaces.list[largest].w) frames.list[i].w = free_spaces.list[largest].w;
                else frames.list[i].w = frames.list[i].max_width;
                  
                if(frames.list[i].max_height > free_spaces.list[largest].h) frames.list[i].h = free_spaces.list[largest].h;
                else frames.list[i].h = frames.list[i].max_height;
                                      
                frames.list[i].x = free_spaces.list[largest].x;
                frames.list[i].y = free_spaces.list[largest].y;
                frames.list[i].mode = TILING;

                resize_frame(display, &frames.list[i]);
                show_frame_state(display, &frames.list[i], &pixmaps);
              }
              if(free_spaces.list != NULL) free(free_spaces.list);              
            }
            break;
          }
        }
        if(i < frames.used) {
          if(event.xbutton.window == frames.list[i].frame  
          || event.xbutton.window == frames.list[i].mode_pulldown 
          || event.xbutton.window == frames.list[i].title_menu.hotspot)  {    
            //A menu is being opened so grab the pointer and intercept the events so that it works.
            XGrabPointer(display,  root, True
            , PointerMotionMask | ButtonPressMask | ButtonReleaseMask
            , GrabModeAsync,  GrabModeAsync, None, cursors.grab, CurrentTime);
            
            //these are in case the mouse moves and the menu is cancelled.
            pointer_start_x = event.xbutton.x;
            pointer_start_y = event.xbutton.y;
            clicked_frame = i;   
            
            if(event.xbutton.window == frames.list[i].mode_pulldown  
            || event.xbutton.window == frames.list[i].title_menu.hotspot) {
              pointer_start_x += 1; //compensate for relative co-ordinates of window and subwindow
              pointer_start_y += 1;
            }
            
            if(event.xbutton.window == frames.list[i].mode_pulldown) {
              #ifdef SHOW_BUTTON_PRESS_EVENT
              printf("pressed mode pulldown %d on window %d\n", frames.list[i].mode_pulldown, frames.list[i].frame);
              #endif
              
              clicked_widget = frames.list[i].mode_pulldown;
              pointer_start_x += frames.list[i].w - H_SPACING*2 - PULLDOWN_WIDTH - BUTTON_SIZE - EDGE_WIDTH - 1;
              pointer_start_y += V_SPACING;
              XUnmapWindow(display, frames.list[i].mode_pulldown);
              XFlush(display);
              #ifdef SHOW_BUTTON_PRESS_EVENT
              printf("changing mode pulldown pixmaps\n");
              #endif
              
              if(frames.list[i].mode == FLOATING) 
                XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_floating_pressed_p );
              else if(frames.list[i].mode == TILING) 
                XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_tiling_pressed_p );
              else if(frames.list[i].mode == SINKING) 
                XSetWindowBackgroundPixmap(display, frames.list[i].mode_pulldown, pixmaps.pulldown_deactivated_p );
              XMapWindow(display, frames.list[i].mode_pulldown);
            }
            else if(event.xbutton.window == frames.list[i].title_menu.hotspot) {
              #ifdef SHOW_BUTTON_PRESS_EVENT
              printf("pressed title menu window %d\n", event.xbutton.window);
              #endif
              
              clicked_widget =  frames.list[i].title_menu.hotspot;                             
              pointer_start_x += H_SPACING*2 + BUTTON_SIZE;
              pointer_start_y += V_SPACING;
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
            #ifdef SHOW_BUTTON_PRESS_EVENT
            printf("pressed closebutton %d on window %d\n", frames.list[i].close_button, frames.list[i].frame);
            #endif
            
            clicked_widget = frames.list[i].close_button;
            XGrabPointer(display,  root, True
            , PointerMotionMask | ButtonReleaseMask
            , GrabModeAsync,  GrabModeAsync, None, None, CurrentTime);
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
           
            //these are for when the mouse moves.
            pointer_start_x = event.xbutton.x;
            pointer_start_y = event.xbutton.y;
            clicked_frame = i;

            if(event.xbutton.window == frames.list[i].l_grip) {
              #ifdef SHOW_BUTTON_PRESS_EVENT
              printf("pressed l_grip on window %d\n", frames.list[i].frame);
              #endif
              clicked_widget = frames.list[i].l_grip;
              pointer_start_y += TITLEBAR_HEIGHT + 1 + EDGE_WIDTH*2;
              XGrabPointer(display, frames.list[i].l_grip, False, PointerMotionMask|ButtonReleaseMask
              , GrabModeAsync,  GrabModeAsync, None, cursors.resize_h, CurrentTime);
            }
            else if(event.xbutton.window == frames.list[i].r_grip) {
              #ifdef SHOW_BUTTON_PRESS_EVENT
              printf("pressed r_grip on window %d\n", frames.list[i].frame);
              #endif
              clicked_widget = frames.list[i].r_grip;
              XGrabPointer(display, frames.list[i].l_grip, False, PointerMotionMask|ButtonReleaseMask
              , GrabModeAsync,  GrabModeAsync, None, cursors.resize_h, CurrentTime);
            }
            else if(event.xbutton.window == frames.list[i].bl_grip) {
              #ifdef SHOW_BUTTON_PRESS_EVENT
              printf("pressed bl_grip on window %d\n", frames.list[i].frame);
              #endif
              clicked_widget = frames.list[i].bl_grip;
              pointer_start_y += frames.list[i].h - CORNER_GRIP_SIZE;
              XGrabPointer(display, frames.list[i].l_grip, False, PointerMotionMask|ButtonReleaseMask
              , GrabModeAsync,  GrabModeAsync, None, cursors.resize_tr_bl, CurrentTime);
            }
            else if(event.xbutton.window == frames.list[i].br_grip) {
              #ifdef SHOW_BUTTON_PRESS_EVENT
              printf("pressed bl_grip on window %d\n", frames.list[i].frame);
              #endif
              clicked_widget = frames.list[i].br_grip;
              XGrabPointer(display, frames.list[i].l_grip, False, PointerMotionMask|ButtonReleaseMask
              , GrabModeAsync,  GrabModeAsync, None, cursors.resize_tl_br, CurrentTime);
            }
            else if(event.xbutton.window == frames.list[i].b_grip) {
              #ifdef SHOW_BUTTON_PRESS_EVENT
              printf("pressed bl_grip on window %d\n", frames.list[i].frame);
              #endif
              clicked_widget = frames.list[i].b_grip;
              XGrabPointer(display, frames.list[i].l_grip, False, PointerMotionMask|ButtonReleaseMask
              , GrabModeAsync,  GrabModeAsync, None, cursors.resize_v, CurrentTime);
            }          
          }
          XFlush(display);
          if(pulldown != root) {
            XAllowEvents(display, AsyncPointer, event.xbutton.time);
            break;
          }
          XAllowEvents(display, ReplayPointer, event.xbutton.time);
        }
        
      break;
      
      /*modifies grab_move and do_click_to focus */
      case EnterNotify:
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
      /*this continues from above.  grab_move, was_sunk, clicked_frame. background_window, clicked_widget, frames and pixmaps*/
      /*It ends the alt grab move and resets pixmaps on widgets */  
      case LeaveNotify:
        if(event.type == LeaveNotify  &&  event.xcrossing.mode == NotifyUngrab  &&  grab_move) {
          //ending the grab_move
          grab_move = 0;
          if(was_sunk  &&  clicked_frame != -1) {
            stack_frame(display, &frames.list[clicked_frame], sinking_seperator, tiling_seperator);
            was_sunk = 0; //reset was_sunk
          }
          if(clicked_frame != -1  &&  frames.list[clicked_frame].mode == TILING) {
            handle_frame_retile(display, &frames, clicked_frame, pointer_start_x, pointer_start_y, event.xbutton.x_root, event.xbutton.y_root);
          }
          clicked_frame = -1; 
        }
          
        if(clicked_widget != root  &&  event.xcrossing.window == clicked_widget)  {
          #ifdef SHOW_LEAVE_NOTIFY_EVENTS
          printf("Enter or exit.  Window %d, Subwindow %d\n", event.xcrossing.window, event.xcrossing.subwindow);
          #endif
          for (i = 0; i < frames.used; i++) {
            if(clicked_widget == frames.list[i].close_button) {
              XSelectInput(display, frames.list[i].close_button, 0);  
              XUnmapWindow(display, frames.list[i].close_button);
              if(event.type == EnterNotify) 
                XSetWindowBackgroundPixmap(display, frames.list[i].close_button, pixmaps.close_button_pressed_p );
              else if (event.type == LeaveNotify) 
                XSetWindowBackgroundPixmap(display, frames.list[i].close_button, pixmaps.close_button_normal_p );
              XMapWindow(display, frames.list[i].close_button);
              XSelectInput(display, frames.list[i].close_button, ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
              XFlush(display);
              break;
            }
          }
        }
        
        if(clicked_widget != root  &&  pulldown != root) {
          if(event.xcrossing.window == mode_pulldown.floating) {
            XSelectInput(display, mode_pulldown.floating, 0);  
            XFlush(display);
            XUnmapWindow(display, mode_pulldown.floating);
            if(event.type == EnterNotify) XSetWindowBackgroundPixmap(display, mode_pulldown.floating, pixmaps.item_floating_hover_p);
            else if (event.type == LeaveNotify) XSetWindowBackgroundPixmap(display, mode_pulldown.floating, pixmaps.item_floating_p);
            XMapWindow(display, mode_pulldown.floating);
            XSelectInput(display, mode_pulldown.floating,  
                  ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
            XFlush(display);
          }
          else if(event.xcrossing.window == mode_pulldown.tiling) {
            XSelectInput(display, mode_pulldown.tiling, 0);  
            XFlush(display);
            XUnmapWindow(display, mode_pulldown.tiling);
            if(event.type == EnterNotify) XSetWindowBackgroundPixmap(display, mode_pulldown.tiling, pixmaps.item_tiling_hover_p);
            else if (event.type == LeaveNotify) XSetWindowBackgroundPixmap(display, mode_pulldown.tiling, pixmaps.item_tiling_p);
            XMapWindow(display, mode_pulldown.tiling);
            XSelectInput(display, mode_pulldown.tiling,  
                  ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
            XFlush(display);
          }
          else if(event.xcrossing.window == mode_pulldown.sinking) {
            XSelectInput(display, mode_pulldown.sinking, 0);  
            XFlush(display);
            XUnmapWindow(display, mode_pulldown.sinking);            
            if(event.type == EnterNotify) XSetWindowBackgroundPixmap(display, mode_pulldown.sinking, pixmaps.item_sinking_hover_p);
            else if (event.type == LeaveNotify) XSetWindowBackgroundPixmap(display, mode_pulldown.sinking, pixmaps.item_sinking_p);
            XMapWindow(display, mode_pulldown.sinking);
            XSelectInput(display, mode_pulldown.sinking,  
                    ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
            XFlush(display);
          }
          else {
            for(i = 0; i < frames.used; i++) {
              if(event.xcrossing.window == frames.list[i].title_menu.entry) {
                XSelectInput(display, frames.list[i].title_menu.entry, 0);
                XFlush(display);
                XUnmapWindow(display, frames.list[i].title_menu.entry);
                if(event.type == EnterNotify) 
                  XSetWindowBackgroundPixmap(display, frames.list[i].title_menu.entry, frames.list[i].title_menu.item_title_hover_p);
                else if (event.type == LeaveNotify) 
                  XSetWindowBackgroundPixmap(display, frames.list[i].title_menu.entry, frames.list[i].title_menu.item_title_p);

                XMapWindow(display, frames.list[i].title_menu.entry);
                XSelectInput(display, frames.list[i].title_menu.entry,  
                        ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
                XFlush(display);                
              }
            }
          }
        }
      break;
      
      case ButtonRelease:
        #ifdef SHOW_BUTTON_RELEASE_EVENT      
        printf("ButtonRelease. Window %d, subwindow %d, root %d\n", event.xbutton.window, event.xbutton.subwindow, event.xbutton.root);        
        #endif
        
        if(was_sunk &&  clicked_frame != -1) {
          stack_frame(display, &frames.list[clicked_frame], sinking_seperator, tiling_seperator);
        }
        was_sunk = 0;
        
        /* Close pop-up menu and maybe activate a menu item */
        if(pulldown != root) { //
          #ifdef SHOW_BUTTON_RELEASE_EVENT
          printf("closed pulldown\n");
          printf("clicked_widget %d\n", clicked_widget);
          #endif
          XUnmapWindow(display, pulldown);
          
          for(i = 0; i < frames.used; i++) {
            if(clicked_widget == frames.list[i].mode_pulldown) {
              if(event.xbutton.window == mode_pulldown.floating) frames.list[i].mode = FLOATING;
              else if(event.xbutton.window == mode_pulldown.tiling) frames.list[i].mode = TILING;
              else if(event.xbutton.window == mode_pulldown.sinking) frames.list[i].mode = SINKING;
              stack_frame(display, &frames.list[i], sinking_seperator, tiling_seperator);
              show_frame_state(display, &frames.list[i], &pixmaps); //Redrawing mode pulldown
              break;
            }
            else if(clicked_widget == frames.list[i].title_menu.hotspot) {
              show_frame_state(display, &frames.list[i], &pixmaps);  //Redrawing title pulldown
              //the first loop find the frame that was clicked.  
              //Now we need to identify which window to put here.
              for(int j = 0; j < frames.used; j++) {               
                if(event.xbutton.window == frames.list[j].title_menu.entry) {
                  replace_frame(display, &frames.list[i], &frames.list[j], sinking_seperator, tiling_seperator, &pixmaps);
                  break;
                }
              }
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
              #ifdef SHOW_BUTTON_RELEASE_EVENT
              printf("released closebutton %d, window %d\n", frames.list[i].close_button, frames.list[i].frame);
              #endif
              XSelectInput(display, frames.list[i].close_button, 0);
              XUnmapWindow(display, frames.list[i].close_button);
              XSetWindowBackgroundPixmap(display, frames.list[i].close_button, pixmaps.close_button_normal_p );
              XMapWindow(display, frames.list[i].close_button);
              XSelectInput(display, frames.list[i].close_button, ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
              XFlush(display);
              //it is might never happen but this doesn't hurt
              if(pulldown != root) {
                XUnmapWindow(display, pulldown);
                pulldown = root;
              }
              remove_window(display, frames.list[i].window);
              clicked_widget = root;
              break;
            }
            else if(clicked_widget == frames.list[i].mode_pulldown) {
              #ifdef SHOW_BUTTON_RELEASE_EVENT
              printf("Pressed the mode_pulldown on window %d\n", frames.list[i].window);
              #endif
              XChangeActivePointerGrab(display, PointerMotionMask | ButtonPressMask | ButtonReleaseMask
              , cursors.normal, CurrentTime);
              pulldown = mode_pulldown.frame;
              show_mode_pulldown_list(display, &mode_pulldown, &pixmaps, event.xbutton.x_root, event.xbutton.y_root);
              
              clicked_frame = -1; //cancel move ability at this stage/don't ungrab
              break;
            }
            else if(clicked_widget == frames.list[i].title_menu.hotspot) {
              #ifdef SHOW_BUTTON_RELEASE_EVENT
              printf("Pressed the title menu on window %d\n", frames.list[i].window);
              #endif
              
              XChangeActivePointerGrab(display
              , PointerMotionMask | ButtonPressMask | ButtonReleaseMask
              , cursors.normal, CurrentTime);
              pulldown = frames.title_menu;
              show_title_menu(display, &frames, event.xbutton.x_root, event.xbutton.y_root);
              XMapWindow(display, pulldown);
              
              XFlush(display);
              clicked_frame = -1; //cancel move ability at this stage/don't ungrab              
              break;
            }
          }
        }
        
        if(clicked_widget == root 
        && clicked_frame != -1
        && frames.list[clicked_frame].mode == TILING) {
          #ifdef SHOW_BUTTON_RELEASE_EVENT
          printf("retiling frame\n");
          #endif    
          handle_frame_retile(display, &frames, clicked_frame, pointer_start_x, pointer_start_y, event.xbutton.x_root, event.xbutton.y_root);
        }
        
        if(clicked_widget != root) for(i = 0; i < frames.used; i++) {
          if(clicked_widget == frames.list[i].l_grip
          || clicked_widget == frames.list[i].bl_grip
          || clicked_widget == frames.list[i].b_grip
          || clicked_widget == frames.list[i].br_grip
          || clicked_widget == frames.list[i].r_grip
          || clicked_widget == frames.list[i].close_button) { //anything except a menu
            #ifdef SHOW_BUTTON_RELEASE_EVENT
            printf("Cancelled click\n");
            #endif
            clicked_widget = root;
            break;
          }
        }
        
        //Don't ungrab the pointer after opening the pop-up menus
        if(clicked_widget == root)  XUngrabPointer(display, CurrentTime);
        if(clicked_frame != -1) {
          #ifdef SHOW_BUTTON_RELEASE_EVENT
          printf("Frame move or frame edge resize ended\n");
          #endif
          XUngrabPointer(display, CurrentTime);
          XFlush(display);
          clicked_frame = -1;
          resize_x_direction = 0;
          resize_y_direction = 0; 
        }
      break;
      
      case MotionNotify:
        if(clicked_frame != -1) {
         //these variables will hold the discarded return values from XQueryPointer
          Window mouse_root, mouse_child;  
          
          int mouse_child_x, mouse_child_y; 
          int mouse_root_x, mouse_root_y;    
          
          unsigned int mask;   
        
          //If a menu on the titlebar is dragged, cancel the menu and move the window.
          if(clicked_widget == frames.list[clicked_frame].mode_pulldown) {  //cancel the pulldown lists opening
            show_frame_state(display, &frames.list[clicked_frame], &pixmaps);
            clicked_widget = root;
          }
          else 
          if(clicked_widget == frames.list[clicked_frame].title_menu.hotspot) { //cancel the pulldown lists opening
            XUnmapWindow(display, frames.list[clicked_frame].title_menu.frame);
            XSetWindowBackgroundPixmap(display, frames.list[clicked_frame].title_menu.arrow, pixmaps.arrow_normal_p);
            XSetWindowBackgroundPixmap(display, frames.list[clicked_frame].title_menu.title
            , frames.list[clicked_frame].title_menu.title_normal_p);
            XMapWindow(display, frames.list[clicked_frame].title_menu.frame);          
            XFlush(display);
            clicked_widget = root;                 
          }
          
          while(XCheckTypedEvent(display, MotionNotify, &event)); //skip foward to the latest move event

          XQueryPointer(display, root, &mouse_root, &mouse_child, &mouse_root_x, &mouse_root_y, &mouse_child_x, &mouse_child_y, &mask);    
                  
          if(clicked_widget == root) { /*** Move/Squish ***/
            handle_frame_move(display, &frames, clicked_frame
            , &pointer_start_x, &pointer_start_y, mouse_root_x, mouse_root_y
            , &was_sunk, &pixmaps, &resize_x_direction, &resize_y_direction);
          }
          else {  /*** Resize grips are being dragged ***/
            //clicked_widget is set to one of the grips.
            handle_frame_resize(display, &frames, clicked_frame
            , pointer_start_x, pointer_start_y, mouse_root_x, mouse_root_y, clicked_widget);
          }
        }
      break; 

      case PropertyNotify:
        for(i = 0; i < frames.used; i++) {
          if(event.xproperty.window == frames.list[i].window) {
            if( event.xproperty.atom == XA_WM_NAME) {
              free_frame_name(display, &frames.list[i]);
              load_frame_name(display, &frames.list[i]);
              resize_frame(display, &frames.list[i]);
            }
            else if ( event.xproperty.atom == XA_WM_NORMAL_HINTS) { 
              get_frame_hints(display, &frames.list[i]);
            }
            break;
          }
        }
      break;

      case ConfigureRequest:
        #ifdef SHOW_CONFIGURE_REQUEST_EVENT
        printf("ConfigureRequest window %d, w: %d, h %d, ser %d, send %d", 
          event.xconfigurerequest.window,
          event.xconfigurerequest.width,
          event.xconfigurerequest.height,
          event.xconfigurerequest.serial, //last event processed
          event.xconfigurerequest.send_event);
        #endif
        
        for(i = 0; i < frames.used; i++) { 
          if(event.xconfigurerequest.window == frames.list[i].window) {
            if(clicked_frame != i) {
              if( event.xconfigurerequest.width >= frames.list[i].min_width
               && event.xconfigurerequest.width <= frames.list[i].max_width) 
                frames.list[i].w = event.xconfigurerequest.width;

              if(  event.xconfigurerequest.height >= frames.list[i].min_height
               && event.xconfigurerequest.height  <= frames.list[i].max_height)
                frames.list[i].h = event.xconfigurerequest.height;
               
              #ifdef SHOW_CONFIGURE_REQUEST_EVENT
              printf(".. width %d, height %d", frames.list[i].w, frames.list[i].h);
              #endif
              resize_frame(display, &frames.list[i]);
              break;
            }
            else {
              #ifdef SHOW_CONFIGURE_REQUEST_EVENT
              printf("skipped");
              #endif
              break;
            }
          }
        }
        #ifdef SHOW_CONFIGURE_REQUEST_EVENT        
        printf("\n");
        #endif
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
  
          #ifdef SHOW_CONFIGURE_REQUEST_EVENT
          printf("premap config (%d, %d) width %d, height %d\n", 
            premap_config.x, premap_config.y, premap_config.width, premap_config.height);
          #endif
          XConfigureWindow(display, event.xconfigurerequest.window, 
            CWX | CWY | CWWidth | CWHeight | CWBorderWidth, &premap_config);
            
          XSetErrorHandler(NULL);
          XUngrabServer(display);
          XFlush(display);
        }
      break;
      case MappingNotify:      
        //printf("MappingNotify\n");
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

  if(pulldown != root) XUnmapWindow(display, pulldown);
  XDestroyWindow(display, background_window);  
  XDestroyWindow(display, mode_pulldown.frame);
  XDestroyWindow(display, frames.title_menu);
  
  XFreePixmap(display, background_window_p);
  free_pixmaps(display, &pixmaps);
  free_cursors(display, &cursors);
  
  for(int i = 0; i < frames.used; i++) remove_frame(display, &frames, i);

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
  //this pixmap is only used for the pressed state of the title_menu  
  pixmaps->light_border_p              = create_pixmap(display, light_border);
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
  
  pixmaps->item_floating_p       = create_pixmap(display, item_floating);
  pixmaps->item_floating_hover_p = create_pixmap(display, item_floating_hover);
  pixmaps->item_sinking_p        = create_pixmap(display, item_sinking);
  pixmaps->item_sinking_hover_p  = create_pixmap(display, item_sinking_hover);
  pixmaps->item_tiling_p         = create_pixmap(display, item_tiling);
  pixmaps->item_tiling_hover_p   = create_pixmap(display, item_tiling_hover);
  
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

  XFreePixmap(display, pixmaps->item_floating_p);
  XFreePixmap(display, pixmaps->item_floating_hover_p);
  XFreePixmap(display, pixmaps->item_sinking_p);
  XFreePixmap(display, pixmaps->item_sinking_hover_p);
  XFreePixmap(display, pixmaps->item_tiling_p);
  XFreePixmap(display, pixmaps->item_tiling_hover_p);

}

void load_cursors (Display *display, struct mouse_cursors *cursors) {
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

void load_hints (Display *display, struct hints *atoms) {
  Window root = DefaultRootWindow(display);
  Screen* screen = DefaultScreenOfDisplay(display);  

  unsigned char *ewmh_atoms = (unsigned char *)&(atoms->supporting_wm_check);
  int number_of_atoms = 0;
  static int desktops = 1;
  //this window is closed automatically by X11 when the connection is closed.
  Window program_instance = XCreateSimpleWindow(display, root, 0, 0, 1, 1, 0, BlackPixelOfScreen(screen), BlackPixelOfScreen(screen)); 
  
  number_of_atoms++; atoms->supported              = XInternAtom(display, "_NET_SUPPORTED", False);
  number_of_atoms++; atoms->supporting_wm_check    = XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", False); 
  number_of_atoms++; atoms->number_of_desktops     = XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", False);   
  number_of_atoms++; atoms->desktop_geometry       = XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", False);     
  number_of_atoms++; atoms->wm_full_placement      = XInternAtom(display, "_NET_WM_FULL_PLACEMENT", False);
  number_of_atoms++; atoms->frame_extents          = XInternAtom(display, "_NET_FRAME_EXTENTS", False);
  number_of_atoms++; atoms->wm_window_type         = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
  number_of_atoms++; atoms->wm_window_type_normal  = XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", False);
  number_of_atoms++; atoms->wm_window_type_dock    = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);
  number_of_atoms++; atoms->wm_window_type_splash  = XInternAtom(display, "_NET_WM_WINDOW_TYPE_SPLASH", False);  //no frame
  number_of_atoms++; atoms->wm_window_type_dialog  = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);  //can be transient
  number_of_atoms++; atoms->wm_window_type_utility = XInternAtom(display, "_NET_WM_WINDOW_TYPE_UTILITY", False); //can be transient
  number_of_atoms++; atoms->wm_state               = XInternAtom(display, "_NET_WM_STATE", False);
  number_of_atoms++; atoms->wm_state_fullscreen    = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
  
  //list all ewmh hints supported.
  XChangeProperty(display, root, atoms->supported, XA_ATOM, 32, PropModeReplace, ewmh_atoms, number_of_atoms);
  //let clients know that a ewmh comlpient window manager is running
  XChangeProperty(display, root, atoms->supporting_wm_check, XA_WINDOW, 32, PropModeReplace, (unsigned char *)&program_instance, 1);
  XChangeProperty(display, root, atoms->number_of_desktops, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&desktops, 1);  
  #ifdef SHOW_STARTUP
  list_properties(display, root);
  #endif
  
}

void show_mode_pulldown_list(Display *display, struct mode_pulldown_list *mode_pulldown, struct frame_pixmaps *pixmaps, int x, int y) {
  printf("showing mode pulldown\n");
  //when we map again the background of the last clicked item might be highlighted
  XSetWindowBackgroundPixmap(display, mode_pulldown->floating, pixmaps->item_floating_p);
  XSetWindowBackgroundPixmap(display, mode_pulldown->tiling,   pixmaps->item_tiling_p);
  XSetWindowBackgroundPixmap(display, mode_pulldown->sinking,  pixmaps->item_sinking_p);

  XFlush(display);

  //TODO: find a way to figure out if this is on screen
  XMoveWindow(display, mode_pulldown->frame, x, y);
  XRaiseWindow(display, mode_pulldown->frame);
  XMapWindow(display, mode_pulldown->frame);
  XFlush(display);
}

void create_mode_pulldown_list(Display *display, struct mode_pulldown_list *mode_pulldown,
 struct frame_pixmaps *pixmaps, struct mouse_cursors *cursors) {
 
  Window root = DefaultRootWindow(display);
  Screen* screen =  DefaultScreenOfDisplay(display);  
  int black = BlackPixelOfScreen(screen);

  mode_pulldown->frame = XCreateSimpleWindow(display, root, 0, 0, 
         PULLDOWN_WIDTH + EDGE_WIDTH*2, MENU_ITEM_HEIGHT * 3 + EDGE_WIDTH*2, 0, black, black);
         
  mode_pulldown->floating = XCreateSimpleWindow(display, mode_pulldown->frame, EDGE_WIDTH, EDGE_WIDTH, 
         PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
         
  mode_pulldown->tiling = XCreateSimpleWindow(display, mode_pulldown->frame, EDGE_WIDTH, MENU_ITEM_HEIGHT + EDGE_WIDTH,
         PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);

  mode_pulldown->sinking = XCreateSimpleWindow(display, mode_pulldown->frame, EDGE_WIDTH, MENU_ITEM_HEIGHT*2 + EDGE_WIDTH,
         PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);

  XSetWindowBackgroundPixmap(display, mode_pulldown->frame, pixmaps->border_p );
  XSetWindowBackgroundPixmap(display, mode_pulldown->floating, pixmaps->item_floating_p );
  XSetWindowBackgroundPixmap(display, mode_pulldown->tiling, pixmaps->item_tiling_p );
  XSetWindowBackgroundPixmap(display, mode_pulldown->sinking, pixmaps->item_sinking_p );
    
  XDefineCursor(display, mode_pulldown->frame, cursors->normal);
  XDefineCursor(display, mode_pulldown->floating, cursors->normal);
  XDefineCursor(display, mode_pulldown->tiling, cursors->normal);
  XDefineCursor(display, mode_pulldown->sinking, cursors->normal); 
  
  XSelectInput(display, mode_pulldown->floating,  
        ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
  XSelectInput(display, mode_pulldown->tiling,  
        ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
  XSelectInput(display, mode_pulldown->sinking,  
        ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);

  XMapWindow(display, mode_pulldown->floating);
  XMapWindow(display, mode_pulldown->tiling);
  XMapWindow(display, mode_pulldown->sinking);
}

void create_title_menu(Display *display, struct Framelist *frames, struct frame_pixmaps *pixmaps, struct mouse_cursors *cursors) {
  Window root = DefaultRootWindow(display);
  Screen* screen =  DefaultScreenOfDisplay(display);  
  int black = BlackPixelOfScreen(screen);
  
  frames->title_menu = XCreateSimpleWindow(display, root, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  XSetWindowBackgroundPixmap (display, frames->title_menu, pixmaps->border_p );
  XDefineCursor(display, frames->title_menu, cursors->normal);
}

void show_title_menu(Display *display, struct Framelist* frames, int x, int y) {
  int max_length = 100;
  XMoveResizeWindow(display, frames->title_menu, x, y, max_length, MENU_ITEM_HEIGHT*frames->used);

  for(int i = 0; i < frames->used; i++) {
    if(frames->list[i].title_menu.width > max_length) max_length = frames->list[i].title_menu.width;
    XMoveWindow(display, frames->list[i].title_menu.entry, EDGE_WIDTH, EDGE_WIDTH + MENU_ITEM_HEIGHT * i);
    XSetWindowBackgroundPixmap (display, frames->list[i].title_menu.entry, frames->list[i].title_menu.item_title_p );    
  }
  //make them all the same width.
  for(int i = 0; i < frames->used; i++) {
    XResizeWindow(display, frames->list[i].title_menu.entry, max_length, MENU_ITEM_HEIGHT);
  }
  
  XResizeWindow(display, frames->title_menu, max_length + EDGE_WIDTH*2, MENU_ITEM_HEIGHT*frames->used + EDGE_WIDTH*2);
  XRaiseWindow(display, frames->title_menu);
  XMapWindow(display, frames->title_menu);
}
