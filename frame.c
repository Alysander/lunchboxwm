#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/extensions/shape.h>
#include <X11/Xlib.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/Xatom.h>

#include "xcheck.h"
#include "basin.h"
#include "menus.h"
#include "theme.h"
#include "frame.h"
#include "frame-actions.h"

static void create_frame_subwindows (Display *display, struct Frame *frame, struct Themes *themes, struct Cursors *cursors);
static void get_frame_type_and_mode (Display *display, struct Frame *frame, struct Atoms *atoms, struct Themes *themes);
static void get_frame_state         (Display *display, struct Frame *frame, struct Atoms *atoms);


/* Sometimes a client window is killed before it gets unmapped, we only get the unmapnotify event,
 but there is no way to tell so we just supress the error. */
int 
supress_xerror(Display *display, XErrorEvent *event) {
  (void) display;
  (void) event;
  //printf("Caught an error\n");
  return 0;
}

/*returns the frame index of the newly created window or -1 if out of memory */
int
create_frame (Display *display, struct Frame_list* frames
, Window framed_window, struct Popup_menu *window_menu, struct Themes *themes, struct Cursors *cursors, struct Atoms *atoms) {
  
  XWindowAttributes get_attributes;
  struct Frame frame;

  if(frames->used == frames->max) {
    struct Frame* temp = NULL;
    temp = realloc(frames->list, sizeof(struct Frame) * frames->max * 2);
    if(temp != NULL) frames->list = temp;
    else return -1;
    frames->max *= 2;
  }
  //printf("Creating frames->list[%d] with window %lu, connection %lu\n", frames->used, (unsigned long)framed_window, (unsigned long)display);
  //add this window to the save set as soon as possible so that if an error occurs it is still available

  XAddToSaveSet(display, framed_window); 
  XSync(display, False);
  XGetWindowAttributes(display, framed_window, &get_attributes);

  /*** Set up defaults ***/
  frame.selected = 0;
  frame.window_name = NULL;
  frame.framed_window = framed_window;
  frame.type = unknown;
  frame.theme_type = unknown;
  frame.mode = unset;
  frame.state = none;
  frame.transient = 0;
  frame.x = get_attributes.x - themes->window_type[frame.theme_type][window].x;
  frame.y = get_attributes.y - themes->window_type[frame.theme_type][window].y;
  frame.width_inc = 1;
  frame.height_inc = 1;
  frame.hspace = 0 - themes->window_type[frame.theme_type][window].w;
  frame.vspace = 0 - themes->window_type[frame.theme_type][window].h;

  frame.w = get_attributes.width; 
  frame.h = get_attributes.height;

  //TODO icon support  get_frame_wm_hints(display, &frame); 
  
  get_frame_hints(display, &frame);
  get_frame_type_and_mode (display, &frame, atoms, themes);
  get_frame_state(display, &frame, atoms);
  create_frame_subwindows(display, &frame, themes, cursors);
  create_frame_name(display, window_menu, &frame, themes, atoms);
  change_frame_mode(display, &frame, unset, themes);
 
  //_NET_FRAME_EXTENTS, left, right, top, bottom, CARDINAL[4]/32 - done per window!      
  Window ewmh_frame_extents[4] = { themes->window_type[frame.theme_type][window].x
  , themes->window_type[frame.theme_type][window].y
  , - themes->window_type[frame.theme_type][window].x - themes->window_type[frame.theme_type][window].w
  , - themes->window_type[frame.theme_type][window].y - themes->window_type[frame.theme_type][window].h
  };
  XChangeProperty(display, framed_window, atoms->frame_extents, XA_CARDINAL
  , 32, PropModeReplace, (unsigned char *)ewmh_frame_extents, 4);
  
  XSetWindowBorderWidth(display, framed_window, 0);
  XGrabServer(display);
  XSetErrorHandler(supress_xerror);
  XSelectInput(display, framed_window,  0);

  //reparent the framed_window to frame.widgets[window].widget
  XReparentWindow(display, framed_window, frame.widgets[window].widget, 0, 0);
  //for some odd reason the reparent only reports an extra unmap event if the window was already unmapped
  XRaiseWindow(display, framed_window);
  XMapWindow(display, frame.widgets[window].widget);
  XSetErrorHandler(NULL);
  XUngrabServer(display);

  XSelectInput(display, framed_window,  PropertyChangeMask);
  XSelectInput(display, frame.widgets[window].widget
  , SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
  //Property notify is used to update titles, structureNotify for destroyNotify events. 
  //Some windows only send the destroy event (e.g., gimp splash screen)
  XSync(display, False);  
  
  //Intercept clicks so we can set the focus and possibly raise floating windows
  XGrabButton(display, Button1, 0, frame.widgets[window].widget, False, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
  //do it for numlock as well
  XGrabButton(display, Button1, Mod2Mask, frame.widgets[window].widget, False, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
  
  check_frame_limits(display, &frame, themes);
  resize_frame(display, &frame, themes);
  
  XMoveResizeWindow(display, framed_window, 0, 0, frame.w - frame.hspace, frame.h - frame.vspace);  
  XMoveWindow(display, framed_window, 0, 0);
  XMapWindow(display, framed_window);  
  XFlush(display);
  frames->list[frames->used] = frame;
  frames->used++;
  return (frames->used - 1);
}

/* This function reparents a framed window to root and then destroys the frame as well as cleaning up the frames drawing surfaces */
/* It is used when the framed window has been unmapped or destroyed, or is about to be*/
void 
remove_frame(Display* display, struct Frame_list* frames, int index, struct Themes *themes) {
  XWindowChanges changes;
  Window root = DefaultRootWindow(display);
  unsigned int mask = CWSibling | CWStackMode;  

  changes.stack_mode = Below;
  changes.sibling = frames->list[index].widgets[frame_parent].widget;
  struct Frame *frame = &frames->list[index];
  //free_frame_icon_size(display, &frames->list[index]);
  //always unmap the title pulldown before removing the frame or else it will crash
  //XDestroyWindow(display, frames->list[index].menu_item.backing);

  XDestroyWindow(display, frames->list[index].menu.item);  
  XGrabServer(display);
  XSetErrorHandler(supress_xerror);  
  XReparentWindow(display, frame->framed_window, root
  , frame->x + themes->window_type[frame->theme_type][window].x
  , frame->y + themes->window_type[frame->theme_type][window].y);
  //TODO need to change the frame w,h attributes here
  XConfigureWindow(display, frame->framed_window, mask, &changes);  //keep the stacking order
  XRemoveFromSaveSet (display, frame->framed_window); //this will not destroy the window because it has been reparented to root
  XDestroyWindow(display, frame->widgets[frame_parent].widget);
  XSync(display, False);
  XSetErrorHandler(NULL);    
  XUngrabServer(display);

  free_frame_name(&frames->list[index]);
  if((frames->used != 1) && (index != frames->used - 1)) { //the frame is not alone or the last
    frames->list[index] = frames->list[frames->used - 1]; //swap the deleted frame with the last frame
  }
  frames->used--;
}

/*This function is called when the close button on the frame is pressed */
void
close_window(Display* display, Window framed_window) {
  int n, found = 0;
  Atom *protocols;

  //based on windowlab/aewm
  if (XGetWMProtocols(display, framed_window, &protocols, &n)) {
    Atom delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    for (int i = 0; i < n; i++)
      if (protocols[i] == delete_window) {
        found++;
        break;
      }
    XFree(protocols);
  }

  if(found)  {
   //from windowlab/aewm
    XClientMessageEvent event;
    event.type = ClientMessage;
    event.window = framed_window;
    event.format = 32;
    event.message_type = XInternAtom(display, "WM_PROTOCOLS", False);
    event.data.l[0] = (long)XInternAtom(display, "WM_DELETE_WINDOW", False);
    event.data.l[1] = CurrentTime;
    XSendEvent(display, framed_window, False, NoEventMask, (XEvent *)&event);
    printf("Sent wm_delete_window message\n");
  }
  else {
    printf("Killed window %lu\n", (unsigned long)framed_window);
    XUnmapWindow(display, framed_window);
    XFlush(display);
    XKillClient(display, framed_window);
  } 
}


/*** Update frame with available resizing information ***/
void 
get_frame_hints(Display* display, struct Frame* frame) { //use themes
  Screen* screen = DefaultScreenOfDisplay(display);
  XSizeHints specified;
  long pre_ICCCM; //pre ICCCM recovered values which are ignored.

/*  printf("BEFORE: width %d, height %d, x %d, y %d\n", frame->w, frame->h, frame->x, frame->y); */
  
  //prevent overly large windows, but allow one larger than the screen by default (think eee pc)
  frame->max_width  = XWidthOfScreen(screen); 
  frame->max_height = XHeightOfScreen(screen);
  frame->min_width  = MINWIDTH  ;
  frame->min_height = MINHEIGHT ;

  #ifdef ALLOW_OVERSIZE_WINDOWS_WITHOUT_MINIMUM_HINTS
  /* Ugh Horrible.  */
  /* Many apps that are resizeable ask to be the size of the screen and since windows
     often don't specifiy their minimum size, we have no way of knowing if they 
     really need to be that size or not.  In case they do, this specifies that their
     current width is their minimum size, in the hope that it is overridden by the
     size hints. This kind of behaviour causes problems on small screens like the
     eee pc. */
  if(frame->w > XWidthOfScreen(screen))  frame->min_width = frame->w;
  if(frame->h > XHeightOfScreen(screen)) frame->min_height = frame->h;
  #endif

  if(XGetWMNormalHints(display, frame->framed_window, &specified, &pre_ICCCM) != 0) {
    #ifdef SHOW_FRAME_HINTS
    printf("Managed to recover size hints\n");
    #endif
    
    if((specified.flags & PPosition) 
    || (specified.flags & USPosition)) {
      #ifdef SHOW_FRAME_HINTS
      if(specified.flags & PPosition) printf("PPosition specified\n");
      else printf("USPosition specified\n");
      #endif
      frame->x = specified.x;
      frame->y = specified.y;
    }

    if((specified.flags & PSize) 
    || (specified.flags & USSize)) {
      #ifdef SHOW_FRAME_HINTS    
      printf("Size specified\n");
      #endif
      frame->w = specified.width ;
      frame->h = specified.height;
    }
    if((specified.flags & PMinSize)
    && (specified.min_width >= frame->min_width)
    && (specified.min_height >= frame->min_height)) {
      #ifdef SHOW_FRAME_HINTS
      printf("Minimum size specified\n");
      #endif
      frame->min_width = specified.min_width;
      frame->min_height = specified.min_height;
    }
    if((specified.flags & PMaxSize) 
    && (specified.max_width >= frame->min_width)
    && (specified.max_height >= frame->min_height)) {
      #ifdef SHOW_FRAME_HINTS
      printf("Maximum size specified\n");
      #endif
      frame->max_width = specified.max_width;
      frame->max_height = specified.max_height;
    }
    if(specified.flags & PResizeInc) {
      printf("got inc hints, w %d, h %d\n", specified.width_inc, specified.height_inc);
      frame->width_inc = specified.width_inc;
      frame->height_inc = specified.height_inc;
    }
  }
  
  //all of the initial values are sans the frame 
  //increase the size of the window for the frame to be drawn in 
  //this means that the attributes must be saved without the extra width and height
  frame->w += frame->hspace; 
  frame->h += frame->vspace; 

  frame->max_width  += frame->hspace; 
  frame->max_height += frame->vspace; 

  frame->min_width  += frame->hspace; 
  frame->min_height += frame->vspace; 

  /* Ensure that the specified sizes all correspond to usable sizes for the client.
     If they set incremental resize hints */
  
  /* */ 

  if(frame->width_inc != 1) {
    if((frame->min_width  - frame->hspace)  % frame->width_inc)
    frame->min_width  += frame->width_inc  - ((frame->min_width  - frame->hspace)% frame->width_inc);
  
    if((frame->max_width  - frame->hspace) % frame->width_inc)
    frame->max_width  += frame->width_inc  - ((frame->max_width  - frame->hspace)% frame->width_inc);
    
    if((frame->w - frame->hspace)  % frame->width_inc)
    frame->w          += frame->width_inc  - ((frame->w          - frame->hspace)% frame->width_inc);
  }

  if(frame->height_inc != 1) {
    if((frame->min_height - frame->vspace) % frame->height_inc)
    frame->min_height += frame->height_inc - ((frame->min_height - frame->vspace)% frame->height_inc);

    if((frame->max_height - frame->vspace) % frame->height_inc)
    frame->max_height += frame->height_inc - ((frame->max_height - frame->vspace)% frame->height_inc);
    
    if((frame->h          - frame->vspace) % frame->width_inc)
    frame->h          += frame->height_inc - ((frame->h          - frame->vspace)% frame->height_inc);  
  }

  #ifdef SHOW_FRAME_HINTS      
  printf("width %d, height %d, min_width %d, max_width %d, min_height %d, max_height %d, x %d, y %d\n"
  , frame->w, frame->h, frame->min_width, frame->max_width, frame->min_height, frame->max_height, frame->x, frame->y);
  #endif
  
}

static void 
get_frame_type_and_mode(Display *display, struct Frame *frame, struct Atoms *atoms, struct Themes *themes) {
  unsigned char *contents = NULL;
  Atom return_type;
  int return_format;
  unsigned long items;
  unsigned long bytes;

  XGetTransientForHint(display, frame->framed_window, &frame->transient);

  XGetWindowProperty(display, frame->framed_window, atoms->wm_window_type, 0, 1 //long long_length?
  , False, AnyPropertyType, &return_type, &return_format,  &items, &bytes, &contents);

  frame->type = unknown;
  frame->mode = floating;
  
  if(return_type == XA_ATOM  && contents != NULL) {
    Atom *window_type = (Atom*)contents;
    #ifdef SHOW_PROPERTIES
    printf("Number of atoms %lu\n", items);
    #endif
    for(unsigned int i =0; i < items; i++) {
    //these are fairly mutually exclusive so be suprised if there are others
      if(window_type[i] == atoms->wm_window_type_desktop) {
        #ifdef SHOW_PROPERTIES
        printf("mode/type: desktop\n");
        #endif
        frame->mode = desktop;
        if(themes->window_type[program]) frame->type = program;  //There are no desktop windows - desktops are normal "programs"
      }
      else if(window_type[i] == atoms->wm_window_type_normal) {
        #ifdef SHOW_PROPERTIES
        printf("type: normal/definately unknown\n");
        #endif
        frame->mode = floating;
        frame->type = unknown;
      }
      else if(window_type[i] == atoms->wm_window_type_dock) {
        #ifdef SHOW_PROPERTIES
        printf("type: dock\n");
        #endif
        frame->mode = floating;
        if(themes->window_type[system_program]) frame->type = system_program; 
      }
      else if(window_type[i] == atoms->wm_window_type_splash) {
        #ifdef SHOW_PROPERTIES
        printf("type: splash\n");
        #endif
        frame->mode = floating;
        if(themes->window_type[splash]) frame->type = splash;
      }
      else if(window_type[i] ==atoms->wm_window_type_dialog) {
        #ifdef SHOW_PROPERTIES
        printf("type: dialog\n");
        #endif
        frame->mode = floating;
        if(themes->window_type[dialog]) frame->type = dialog;
      }
      else if(window_type[i] == atoms->wm_window_type_utility) {
        #ifdef SHOW_PROPERTIES
        printf("type: utility\n");
        #endif
        frame->mode = floating;
        if(themes->window_type[utility]) frame->type = utility;
      }
    }
  }

  if(contents != NULL) XFree(contents);
}

/*This function gets the frame state, which is either, none, demands attention or fullscreen.  
  Since EWMH considers "modal dialog box" a state, but I consider it a type, it can also change the type
  and therefor must be called after get frame type. */
static void 
get_frame_state(Display *display, struct Frame *frame, struct Atoms *atoms) {
  unsigned char *contents = NULL;
  Atom return_type;
  int return_format;
  unsigned long items;
  unsigned long bytes;
  XGetWindowProperty(display, frame->framed_window, atoms->wm_state, 0, 1
  , False, AnyPropertyType, &return_type, &return_format,  &items, &bytes, &contents);

  //printf("loading state\n");
  frame->state = none; 
  if(return_type == XA_ATOM  && contents != NULL) {
    Atom *window_state = (Atom*)contents;
    #ifdef SHOW_PROPERTIES
    printf("Number of atoms %lu\n", items);
    #endif
    for(unsigned int i =0; i < items; i++) {
      if(window_state[i] == atoms->wm_state_demands_attention) {
        #ifdef SHOW_PROPERTIES
        printf("state: urgent\n");
        #endif
        frame->state = demands_attention;
      }
      else if(window_state[i] == atoms->wm_state_modal) {
        #ifdef SHOW_PROPERTIES
        printf("type/state: modal dialog\n");
        #endif
        frame->type = modal_dialog;
        frame->mode = floating;
      }
      else if(window_state[i] == atoms->wm_state_fullscreen) {
        #ifdef SHOW_PROPERTIES
        printf("state: fullscreen\n");
        #endif
        frame->state = fullscreen;
      }
    }
  }
  if(contents != NULL) XFree(contents);
}

static void
create_frame_subwindows (Display *display, struct Frame *frame, struct Themes *themes, struct Cursors *cursors) {
  Window root = DefaultRootWindow(display);
  Screen* screen = DefaultScreenOfDisplay(display);
  int black = BlackPixelOfScreen(screen);

  frame->widgets[frame_parent].widget = XCreateSimpleWindow(display, root, frame->x, frame->y,  frame->w, frame->h, 0, black, black);

  for(int i = 0; i < frame_parent; i++) {
    frame->widgets[i].widget = 0;
    for(int j = 0; j <= inactive; j++)  frame->widgets[i].state[j] = 0;
  }
  for(int i = 0; i < frame_parent; i++) {

    int x = themes->window_type[frame->theme_type][i].x;
    int y = themes->window_type[frame->theme_type][i].y;
    int w = themes->window_type[frame->theme_type][i].w;
    int h = themes->window_type[frame->theme_type][i].h;

    if(!themes->window_type[frame->theme_type][i].exists) {
      //printf("Skipping window component %d\n", i);
      continue;
    }
    //TODO get the size of the window from somewhere else
    if(x <  0) x += frame->w;
    if(y <  0) y += frame->h; 
    if(w <= 0) w += frame->w;
    if(h <= 0) h += frame->h;

    if(themes->window_type[frame->theme_type][i].exists < 0) { //the exists variable is -1 for hotspots
      frame->widgets[i].widget = XCreateWindow(display, frame->widgets[frame_parent].widget
      , x, y, w, h, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
      XMapWindow(display, frame->widgets[i].widget);
    }
    else if (themes->window_type[frame->theme_type][i].exists) { //otherwise if it isn't an inputonly hotspot
      frame->widgets[i].widget = XCreateSimpleWindow(display, frame->widgets[frame_parent].widget
      , x, y, w, h, 0, black, black);

      //OPTIMIZATION: This makes the cropped state windows large so that they don't need to be resized
      if(themes->window_type[frame->theme_type][i].w <= 0) w = XWidthOfScreen(screen);
      if(themes->window_type[frame->theme_type][i].h <= 0) h = XWidthOfScreen(screen);       
      
      if(i != window) //dont create state windows for the framed window 
      for(int j = 0; j <= inactive; j++) {
        frame->widgets[i].state[j] = XCreateSimpleWindow(display, frame->widgets[i].widget
        , 0, 0, w, h, 0, black, black);
        XSetWindowBackgroundPixmap(display, frame->widgets[i].state[j], themes->window_type[frame->theme_type][i].state_p[j]);
        XMapWindow(display, frame->widgets[i].state[j]);
      }
      XMapWindow(display, frame->widgets[i].widget);
    }
    //map windows
  }
  frame->menu.item = 0;
  //select input
  XSelectInput(display, frame->widgets[frame_parent].widget,   Button1MotionMask | ButtonPressMask | ButtonReleaseMask);

  XDefineCursor(display, frame->widgets[frame_parent].widget, cursors->normal);

  if(themes->window_type[frame->theme_type][close_button_hotspot].exists) {
    XSelectInput(display, frame->widgets[close_button_hotspot].widget,  ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
    XDefineCursor(display, frame->widgets[close_button_hotspot].widget, cursors->pressable);
  }

  if(themes->window_type[frame->theme_type][mode_dropdown_hotspot].exists) {
    XSelectInput(display, frame->widgets[mode_dropdown_hotspot].widget, ButtonPressMask | ButtonReleaseMask);
    XDefineCursor(display, frame->widgets[mode_dropdown_hotspot].widget, cursors->pressable);
  }

  if(themes->window_type[frame->theme_type][title_menu_hotspot].exists) {
    XSelectInput(display, frame->widgets[title_menu_hotspot].widget, ButtonPressMask | ButtonReleaseMask);
    XDefineCursor(display, frame->widgets[title_menu_hotspot].widget , cursors->pressable);
  }

  if(themes->window_type[frame->theme_type][tl_corner].exists) {
    XSelectInput(display, frame->widgets[tl_corner].widget,  ButtonPressMask | ButtonReleaseMask);
    XDefineCursor(display, frame->widgets[tl_corner].widget, cursors->resize_tl_br);
  }
  if(themes->window_type[frame->theme_type][t_edge].exists) {
    XSelectInput(display, frame->widgets[t_edge].widget,     ButtonPressMask | ButtonReleaseMask);
    XDefineCursor(display, frame->widgets[t_edge].widget , cursors->resize_v);
  }
  if(themes->window_type[frame->theme_type][tr_corner].exists) {
    XSelectInput(display, frame->widgets[tr_corner].widget,  ButtonPressMask | ButtonReleaseMask);
    XDefineCursor(display, frame->widgets[tr_corner].widget, cursors->resize_tr_bl);  
  }
  if(themes->window_type[frame->theme_type][l_edge].exists) {
    XSelectInput(display, frame->widgets[l_edge].widget,     ButtonPressMask | ButtonReleaseMask);
    XDefineCursor(display, frame->widgets[l_edge].widget , cursors->resize_h);
  }
  if(themes->window_type[frame->theme_type][bl_corner].exists) {
    XSelectInput(display, frame->widgets[bl_corner].widget,  ButtonPressMask | ButtonReleaseMask);
    XDefineCursor(display, frame->widgets[bl_corner].widget, cursors->resize_tr_bl);
  }
  if(themes->window_type[frame->theme_type][b_edge].exists) {
    XSelectInput(display, frame->widgets[b_edge].widget,     ButtonPressMask | ButtonReleaseMask);
    XDefineCursor(display, frame->widgets[b_edge].widget , cursors->resize_v);
  }
  if(themes->window_type[frame->theme_type][br_corner].exists) {
    XSelectInput(display, frame->widgets[br_corner].widget,  ButtonPressMask | ButtonReleaseMask);
    XDefineCursor(display, frame->widgets[br_corner].widget, cursors->resize_tl_br);
  }
  if(themes->window_type[frame->theme_type][r_edge].exists) {
    XSelectInput(display, frame->widgets[r_edge].widget,     ButtonPressMask | ButtonReleaseMask);
    XDefineCursor(display, frame->widgets[r_edge].widget , cursors->resize_h);
  }
  XFlush(display);
}

/*** create pixmaps with the specified name if it is available, otherwise use a default name
TODO check if the name is just whitespace  ***/

/* Problem:  this should create the name for the window itself, and one for the title menu
   but this function doesn't know parent window of the title menu.  Also, it may need to be in many
   different title menus so perhaps this should just make a pixmap. */

void
create_frame_name(Display* display, struct Popup_menu *window_menu, struct Frame *frame, struct Themes *themes, struct Atoms *atoms) {
  char untitled[] = "noname";

  struct Frame temp = *frame; 

  Screen* screen = DefaultScreenOfDisplay(display);
  int black = BlackPixelOfScreen(screen);

  { /* Recover EWMH UTF8 name first. If none exists, get the ICCCM ASCII name */
    /* If this succeeds, previous names will be freed if they exists later on */
    Atom ret_type;
    int  ret_format;
    unsigned long ret_nitems;
    unsigned long ret_trailing_bytes;
    
    XGetWindowProperty (display, temp.framed_window, atoms->name, (long)0, (long)MAX_WM_NAME_LENGTH
    , False, atoms->utf8
    , &ret_type, &ret_format, &ret_nitems, &ret_trailing_bytes
    , (unsigned char **)&temp.window_name );
    
    if(!temp.window_name) XFetchName(display, temp.framed_window, &temp.window_name);
  } 
  
  if(temp.window_name == NULL 
  && frame->window_name != NULL
  && strcmp(frame->window_name, untitled) == 0) {
    //it was null and already has the name from untitled above.
    return;
  }
  else 
  if(temp.window_name == NULL) {
    printf("Warning: unnamed window\n");
    XStoreName(display, temp.framed_window, untitled);
    XFlush(display);
    XFetchName(display, temp.framed_window, &temp.window_name);
  }
  else 
  if(frame->window_name != NULL
  && strcmp(temp.window_name, frame->window_name) == 0) {
    XFree(temp.window_name);
    //skip this if the name hasn't changed
    return;
  }

  if(!temp.menu.item) {
    temp.menu.item = XCreateSimpleWindow(display
    , window_menu->widgets[popup_menu_parent].widget
    , themes->popup_menu[l_edge].w, 0
    , XWidthOfScreen(screen), themes->popup_menu[menu_item_mid].h
    , 0, black, black);
    for(int i = 0; i <= inactive; i++) {
      temp.menu.state[i] = XCreateSimpleWindow(display
      , temp.menu.item
      , 0, 0
      , XWidthOfScreen(screen), themes->popup_menu[menu_item_mid].h
      , 0, black, black);
    }
    XSelectInput(display, temp.menu.item, ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
  }
  
  temp.menu.width = get_text_width(display, temp.window_name, &themes->font_theme[active]);
  
  //create corresponding title menu item for this frame
  for(int i = 0; i <= inactive; i++) {
    XUnmapWindow(display, temp.menu.state[i]);
    XUnmapWindow(display, temp.widgets[title_menu_text].state[i]);
    XFlush(display);
    create_text_background(display, temp.menu.state[i], temp.window_name
    , &themes->font_theme[i], themes->popup_menu[menu_item_mid].state_p[i]
    , XWidthOfScreen(screen), themes->popup_menu[menu_item_mid].h);
    
    create_text_background(display, temp.widgets[title_menu_text].state[i], temp.window_name
    , &themes->font_theme[active], themes->window_type[frame->theme_type][title_menu_text].state_p[i]
    , XWidthOfScreen(screen), themes->window_type[frame->theme_type][title_menu_text].h);
    
    //If this is mapped here, it might be shown in the wrong workspace, //XMapWindow(display, temp.menu.item);
    /* Show changes to background pixmaps */
    XMapWindow(display, temp.menu.state[i]);
    XMapWindow(display, temp.widgets[title_menu_text].state[i]);    
  }
  xcheck_raisewin(display, temp.menu.state[active]);
  //these are the items for inside the menu
  //need to create all these windows.
  
  {
    XWindowAttributes attr;
    XGetWindowAttributes(display, temp.menu.item, &attr);
    /* Destroy/Free old title if it had one */
    free_frame_name(frame);
    
    if(attr.map_state != IsUnmapped) { //remap all the state pixmaps
      XSelectInput(display, temp.menu.item, 0);
      XSync(display, False);
      XUnmapWindow(display, temp.menu.item);
      XSelectInput(display, temp.menu.item, ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
      XFlush(display);
      XMapWindow(display, temp.menu.item);
    }
  }
  XFlush(display);

  *frame = temp;
}

/*This is used to make the transition to the EWMH UTF8 names simpler */
//free_frame_name(Display* display, struct Frame* frame)
void
free_frame_name(struct Frame* frame) {
  if(frame->window_name != NULL) {
    XFree(frame->window_name);
    frame->window_name = NULL;
  }
} 
