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

/* Sometimes a client window is killed before it gets unmapped, we only get the unmapnotify event,
 but there is no way to tell so we just supress the error. */
int 
supress_xerror(Display *display, XErrorEvent *event) {
  (void) display;
  (void) event;
  //printf("Caught an error\n");
  return 0;
}

/* This function reparents a framed window to root and then destroys the frame as well as cleaning up the frames drawing surfaces */
/* It is used when the framed window has been unmapped or destroyed, or is about to be*/
void 
remove_frame(Display* display, struct Frame_list* frames, int index) {
  XWindowChanges changes;
  Window root = DefaultRootWindow(display);
  unsigned int mask = CWSibling | CWStackMode;  

  changes.stack_mode = Below;
  changes.sibling = frames->list[index].widgets[frame_parent].widget;

  //free_frame_icon_size(display, &frames->list[index]);
  //always unmap the title pulldown before removing the frame or else it will crash
  //TODO destroy sequence (what is this todo for again?)
  //XDestroyWindow(display, frames->list[index].menu_item.backing);
  
  XGrabServer(display);
  XSetErrorHandler(supress_xerror);  
  XReparentWindow(display, frames->list[index].framed_window, root, frames->list[index].x, frames->list[index].y);
  XConfigureWindow(display, frames->list[index].framed_window, mask, &changes);  //keep the stacking order
  XRemoveFromSaveSet (display, frames->list[index].framed_window); //this will not destroy the window because it has been reparented to root
  XDestroyWindow(display, frames->list[index].widgets[frame_parent].widget);
  XSync(display, False);
  XSetErrorHandler(NULL);    
  XUngrabServer(display);

  free_frame_name(display, &frames->list[index]);
  
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

/*returns the frame index of the newly created window or -1 if out of memory */
int
create_frame (Display *display, struct Frame_list* frames
, Window framed_window, struct Popup_menu *window_menu, struct Themes *themes, struct Cursors *cursors, struct Atoms *atoms) {

  Screen* screen = DefaultScreenOfDisplay(display);
  int black = BlackPixelOfScreen(screen);
  XWindowAttributes get_attributes;
  struct Frame frame;

  if(frames->used == frames->max) {
    //printf("reallocating, used %d, max%d\n", frames->used, frames->max);
    struct Frame* temp = NULL;
    temp = realloc(frames->list, sizeof(struct Frame) * frames->max * 2);
    if(temp != NULL) frames->list = temp;
    else return -1;
    frames->max *= 2;
  }
  printf("Creating frames->list[%d] with window %lu, connection %lu\n", frames->used, (unsigned long)framed_window, (unsigned long)display);
  //add this window to the save set as soon as possible so that if an error occurs it is still available

  XAddToSaveSet(display, framed_window); 
  XSync(display, False);
  //XGetWindowAttributes shouldn't be in get_frame_hints because it reports 0,0 after reparenting
  XGetWindowAttributes(display, framed_window, &get_attributes);

  /*** Set up defaults ***/
  frame.selected = 0;
  frame.window_name = NULL;
  frame.framed_window = framed_window;
  frame.type = unknown;
  frame.mode = unset;
  frame.state = none;
  frame.transient = 0;
  frame.x = get_attributes.x;
  frame.y = get_attributes.y;
  frame.w = get_attributes.width;  
  frame.h = get_attributes.height;

  frame.hspace = themes->window_type[unknown][frame_parent].w - themes->window_type[unknown][window].w;
  frame.vspace = themes->window_type[unknown][frame_parent].h - themes->window_type[unknown][window].h;
  printf("frame hspace = %d\n", frame.hspace);
  printf("frame vspace = %d\n", frame.vspace);

  //TODO icon support  get_frame_wm_hints(display, &frame); 

  //frame.menu_item.backing = XCreateSimpleWindow(display, frames->title_menu, 10, 10
  //, XWidthOfScreen(screen), MENU_ITEM_HEIGHT, 0, black, black);
  //XMapWindow(display, frame.menu_item.backing);  

  get_frame_hints(display, &frame);
  create_frame_subwindows(display, &frame, themes, cursors);
  create_frame_name(display, window_menu, &frame, themes);
  get_frame_type (display, &frame, atoms, themes);
  get_frame_state(display, &frame, atoms);

  resize_frame(display, &frame, themes); //resize the title menu if it isn't at it's minimum 
  XMoveResizeWindow(display, framed_window, 0, 0, frame.w - frame.hspace, frame.h - frame.vspace);
  XSetWindowBorderWidth(display, framed_window, 0);
  XGrabServer(display);
  XSetErrorHandler(supress_xerror);
  XSelectInput(display, framed_window,  0);

  //reparent the framed_window to frame.widgets[window].widget
  XReparentWindow(display, framed_window, frame.widgets[window].widget, 0, 0);
  //for some odd reason the reparent only reports an extra unmap event if the window was already unmapped
  XRaiseWindow(display, framed_window);
  XMapWindow(display, frame.widgets[window].widget);
  XSync(display, False);
  XSetErrorHandler(NULL);
  XUngrabServer(display);

  //reparent the framed_window to frame.widgets[window].widget
  XSelectInput(display, framed_window,  StructureNotifyMask | PropertyChangeMask);
  //Property notify is used to update titles, structureNotify for destroyNotify events
  XSelectInput(display, frame.widgets[window].widget
  , SubstructureRedirectMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);

  //Intercept clicks so we can set the focus and raise window if they are floating.
  XGrabButton(display, Button1, 0, framed_window, False, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);

  XMapWindow(display, framed_window);  
  frames->list[frames->used] = frame;
  frames->used++;
  return (frames->used - 1);
}

void
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

    int x = themes->window_type[unknown][i].x;
    int y = themes->window_type[unknown][i].y;
    int w = themes->window_type[unknown][i].w;
    int h = themes->window_type[unknown][i].h;

    if(!themes->window_type[unknown][i].exists) {
      printf("Skipping window component %d\n", i);
      continue;
    }
    //TODO get the size of the window from somewhere else
    if(x <  0) x += frame->w;
    if(y <  0) y += frame->h; 
    if(w <= 0) w += frame->w;
    if(h <= 0) h += frame->h;

    if(themes->window_type[unknown][i].exists < 0) { //the exists variable is -1 for hotspots
      frame->widgets[i].widget = XCreateWindow(display, frame->widgets[frame_parent].widget
      , x, y, w, h, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
      XMapWindow(display, frame->widgets[i].widget);
    }
    else if (themes->window_type[unknown][i].exists) { //otherwise if it isn't an inputonly hotspot
      frame->widgets[i].widget = XCreateSimpleWindow(display, frame->widgets[frame_parent].widget
      , x, y, w, h, 0, black, black);

      if(i != window) //dont create state windows for the framed window 
      for(int j = 0; j <= inactive; j++) {
        frame->widgets[i].state[j] = XCreateSimpleWindow(display, frame->widgets[i].widget
        , 0, 0, w, h, 0, black, black);
        XSetWindowBackgroundPixmap(display, frame->widgets[i].state[j], themes->window_type[unknown][i].state_p[j]);
        XMapWindow(display, frame->widgets[i].state[j]);
      }
      XMapWindow(display, frame->widgets[i].widget);
    }
    //map windows
  }
  //select input
  XMapWindow(display, frame->widgets[frame_parent].widget); //remove this later and get change_to_workspace to map it properly
  XSelectInput(display, frame->widgets[frame_parent].widget,   Button1MotionMask | ButtonPressMask | ButtonReleaseMask);

  XDefineCursor(display, frame->widgets[frame_parent].widget, cursors->normal);

  if(themes->window_type[unknown][close_button_hotspot].exists) {
    XSelectInput(display, frame->widgets[close_button_hotspot].widget,  ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
    XDefineCursor(display, frame->widgets[close_button_hotspot].widget, cursors->pressable);
  }

  if(themes->window_type[unknown][mode_dropdown_hotspot].exists) {
    XSelectInput(display, frame->widgets[mode_dropdown_hotspot].widget, ButtonPressMask | ButtonReleaseMask);
    XDefineCursor(display, frame->widgets[mode_dropdown_hotspot].widget, cursors->pressable);
    XRaiseWindow(display, frame->widgets[mode_dropdown_hotspot].widget);
  }

  if(themes->window_type[unknown][title_menu_hotspot].exists) {
    XSelectInput(display, frame->widgets[title_menu_hotspot].widget, ButtonPressMask | ButtonReleaseMask);
    XDefineCursor(display, frame->widgets[title_menu_hotspot].widget , cursors->pressable);
  }

  if(themes->window_type[unknown][tl_corner].exists) {
    XSelectInput(display, frame->widgets[tl_corner].widget,  ButtonPressMask | ButtonReleaseMask);
    XDefineCursor(display, frame->widgets[tl_corner].widget, cursors->resize_tl_br);
  }
  if(themes->window_type[unknown][t_edge].exists) {
// TODO I don't current want the top edge to be used as a resize, perhaps this should be a seperate widget
//    XSelectInput(display, frame->widgets[t_edge].widget,     ButtonPressMask | ButtonReleaseMask);
//    XDefineCursor(display, frame->widgets[t_edge].widget , cursors->resize_v);
  }
  if(themes->window_type[unknown][tr_corner].exists) {
    XSelectInput(display, frame->widgets[tr_corner].widget,  ButtonPressMask | ButtonReleaseMask);
    XDefineCursor(display, frame->widgets[tr_corner].widget, cursors->resize_tr_bl);  
  }
  if(themes->window_type[unknown][l_edge].exists) {
    XSelectInput(display, frame->widgets[l_edge].widget,     ButtonPressMask | ButtonReleaseMask);
    XDefineCursor(display, frame->widgets[l_edge].widget , cursors->resize_h);
  }
  if(themes->window_type[unknown][bl_corner].exists) {
    XSelectInput(display, frame->widgets[bl_corner].widget,  ButtonPressMask | ButtonReleaseMask);
    XDefineCursor(display, frame->widgets[bl_corner].widget, cursors->resize_tr_bl);
  }
  if(themes->window_type[unknown][b_edge].exists) {
    XSelectInput(display, frame->widgets[b_edge].widget,     ButtonPressMask | ButtonReleaseMask);
    XDefineCursor(display, frame->widgets[b_edge].widget , cursors->resize_v);
  }
  if(themes->window_type[unknown][br_corner].exists) {
    XSelectInput(display, frame->widgets[br_corner].widget,  ButtonPressMask | ButtonReleaseMask);
    XDefineCursor(display, frame->widgets[br_corner].widget, cursors->resize_tl_br);
  }
  if(themes->window_type[unknown][r_edge].exists) {
    XSelectInput(display, frame->widgets[r_edge].widget,     ButtonPressMask | ButtonReleaseMask);
    XDefineCursor(display, frame->widgets[r_edge].widget , cursors->resize_h);
  }
  XFlush(display);
}

/*** Moves and resizes the subwindows of the frame ***/
void resize_frame(Display* display, struct Frame* frame, struct Themes *themes) {
  XMoveResizeWindow(display, frame->widgets[frame_parent].widget, frame->x, frame->y, frame->w, frame->h);

  XResizeWindow(display, frame->framed_window, frame->w - frame->hspace, frame->h - frame->vspace);

  for(int i = 0; i < frame_parent; i++) {
    int x = themes->window_type[frame->type][i].x;
    int y = themes->window_type[frame->type][i].y;
    int w = themes->window_type[frame->type][i].w;
    int h = themes->window_type[frame->type][i].h;
    if(!themes->window_type[frame->type][i].exists) continue; //the exists variable is -1 for hotspots

    if(x < 0  ||  y < 0  ||  w <= 0  ||  h <= 0) { //only resize those which are dependent on the width
      if(x <  0) x += frame->w;
      if(y <  0) y += frame->h;
      if(w <= 0) w += frame->w;
      if(h <= 0) h += frame->h;

      XMoveResizeWindow(display, frame->widgets[i].widget, x, y, w, h);
      for(int j = 0; j <= inactive; j++) {
        //OPTIMIZATION: these could be a default large size in the create_subwindows
        //perhaps make the screen width or height if theme component width/height is variable 
        if(frame->widgets[i].state[j]) XResizeWindow(display, frame->widgets[i].state[j], w, h);        
      }
    }
  }
  if(frame->widgets[mode_dropdown_hotspot].widget) {
    XRaiseWindow(display, frame->widgets[mode_dropdown_hotspot].widget);
  }
  XFlush(display);
}

void
free_frame_name(Display* display, struct Frame* frame) {
  if(frame->window_name != NULL) {
    XFree(frame->window_name);
    frame->window_name = NULL;
    XDestroyWindow(display, frame->menu.item);
    XFlush(display);
  }
}

/*** create pixmaps with the specified name if it is available, otherwise use a default name
TODO check if the name is just whitespace  ***/

/* Problem:  this should create the name for the window itself, and one for the title menu
   but this function doesn't know parent window of the title menu.  Also, it may need to be in many
   different title menus so perhaps this should just make a pixmap. */

void
create_frame_name(Display* display, struct Popup_menu *window_menu, struct Frame *frame, struct Themes *themes) {
  char untitled[10] = "noname";
//  char untitled[10] = "untitled";

  struct Frame temp = *frame; 

  Screen* screen = DefaultScreenOfDisplay(display);
  int black = BlackPixelOfScreen(screen);
  const int menu_item = medium_menu_item_mid; /*Change the mode menu item size here */
  XFetchName(display, temp.framed_window, &temp.window_name);

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

  temp.menu.item = XCreateSimpleWindow(display
  , window_menu->widgets[popup_menu_parent].widget //what is the parent?
  , themes->popup_menu[l_edge].w, 0
  , XWidthOfScreen(screen), themes->popup_menu[menu_item].h
  , 0, black, black);

  //create corresponding title menu item for this frame
  for(int i = 0; i <= inactive; i++) {
    temp.menu.state[i] = XCreateSimpleWindow(display
    , temp.menu.item
    , 0, 0
    , window_menu->inner_width, themes->popup_menu[menu_item].h
    , 0, black, black);

    create_text_background(display, temp.menu.state[i], temp.window_name
    , &themes->medium_font_theme[i], themes->popup_menu[menu_item].state_p[i]
    , themes->popup_menu[menu_item].w, themes->popup_menu[menu_item].h);

    XMapWindow(display, temp.menu.state[i]);
  }
  //these are the items for inside the menu
  //need to create all these windows.

  //TODO map new titles 

  //destroy old title if it had one
  if(frame->window_name != NULL) {
    free_frame_name(display, frame);
  }
  XFlush(display);

  frame->title_width = 90;
  frame->menu_width = 90;
/*  frame->title_width = get_title_width(display, frame->window_name);
  frame->menu_width =  get_title_width(display, frame->window_name);  */
  *frame = temp;
}

/*** Update frame with available resizing information ***/
void 
get_frame_hints(Display* display, struct Frame* frame) { //use themes
  Screen* screen = DefaultScreenOfDisplay(display);
  Window root = DefaultRootWindow(display);

  XSizeHints specified;
  long pre_ICCCM; //pre ICCCM recovered values which are ignored.

  /*  printf("BEFORE: width %d, height %d, x %d, y %d\n", frame->w, frame->h, frame->x, frame->y); */
  //TODO, need to calculate this using the theme...
  frame->max_width  = XWidthOfScreen(screen) - frame->hspace;
  frame->max_height = XHeightOfScreen(screen) - frame->vspace; //- MENUBAR_HEIGHT
  frame->min_width  = MINWIDTH - frame->hspace;
  frame->min_height = MINHEIGHT - frame->vspace;


  #ifdef ALLOW_OVERSIZE_WINDOWS_WITHOUT_MINIMUM_HINTS
  /* Ugh Horrible.  */
  /* Many apps that are resizeable ask to be the size of the screen and since windows
     often don't specifiy their minimum size, we have no way of knowing if they 
     really need to be that size or not.  In case they do, this specifies that their
     current width is their minimum size, in the hope that it is overridden by the
     size hints. This kind of behaviour causes problems on small screens like the
     eee pc. */

  if(frame->w > XWidthOfScreen(screen)) {
    frame->min_width = frame->w;
    frame->max_width = frame->min_width;      
  }
  if(frame->h > XHeightOfScreen(screen)) {
    frame->min_height = frame->h;
    frame->max_height = frame->min_height;
  }
  #endif

  if(XGetWMNormalHints(display, frame->framed_window, &specified, &pre_ICCCM) != 0) {
    #ifdef SHOW_FRAME_HINTS
    printf("Managed to recover size hints\n");
    #endif
    #ifdef ALLOW_POSITION_HINTS
    if((specified.flags & PPosition) 
    || (specified.flags & USPosition)) {
      #ifdef SHOW_FRAME_HINTS
      if(specified.flags & PPosition) printf("PPosition specified\n");
      else printf("USPosition specified\n");
      #endif
      frame->x = specified.x;
      frame->y = specified.y;
    }
    #endif
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

  #ifdef SHOW_FRAME_HINTS      
  printf("width %d, height %d, min_width %d, max_width %d, min_height %d, max_height %d, x %d, y %d\n"
  , frame->w, frame->h, frame->min_width, frame->max_width, frame->min_height, frame->max_height, frame->x, frame->y);
  #endif
  check_frame_limits(display, frame);
}

/** Update frame with available wm hints (icon, window "group", focus wanted, urgency) **/

  /*
void get_frame_wm_hints(Display *display, struct Frame *frame) {
  XWMHints *wm_hints = XGetWMHints(display, frame->framed_window);
  //WM_ICON_SIZE  in theory we can ask for set of specific icon sizes.

  //Set defaults
  Pixmap icon_p = 0;
  Pixmap icon_mask_p = 0;

  if(wm_hints != NULL) {
    if(wm_hints->flags & IconPixmapHint
    && wm_hints->icon_pixmap != 0) {
      icon_p = wm_hints->icon_pixmap;
      XSetWindowBackgroundPixmap(display, frame->menu_item.icon.item_icon, icon_p);
    }

    if(wm_hints->flags & IconMaskHint  
    && wm_hints->icon_pixmap != 0) {
      icon_mask_p = wm_hints->icon_mask;
      XShapeCombineMask (display, frame->menu_item.icon.item_icon, ShapeBounding //ShapeClip or ShapeBounding
      ,0, 0, icon_mask_p, ShapeSet); 
      //Shapeset or ShapeUnion, ShapeIntersect, ShapeSubtract, ShapeInvert
      XMapWindow(display, frame->menu_item.icon.item_icon);
    }
    XSync(display, False);
    //get the icon sizes
    //find out it is urgent
    //get the icon if it has one.
    //icon window is for the systray
    //window group is for mass minimization
    XFree(wm_hints);
  }
}
*/

void get_frame_type(Display *display, struct Frame *frame, struct Atoms *atoms, struct Themes *themes) {
  unsigned char *contents = NULL;
  Atom return_type;
  int return_format;
  unsigned long items;
  unsigned long bytes;

  XGetTransientForHint(display, frame->framed_window, &frame->transient);

  XGetWindowProperty(display, frame->framed_window, atoms->wm_window_type, 0, 1 //long long_length?
  , False, AnyPropertyType, &return_type, &return_format,  &items, &bytes, &contents);

  frame->type = unknown; 
  if(return_type == XA_ATOM  && contents != NULL) {
    Atom *window_type = (Atom*)contents;
    #ifdef SHOW_PROPERTIES
    printf("Number of atoms %lu\n", items);
    #endif
    for(int i =0; i < items; i++) {
    //these are fairly mutually exclusive so be suprised if there are others
      if(window_type[i] == atoms->wm_window_type_desktop) {
        #ifdef SHOW_PROPERTIES
        printf("mode/type: desktop\n");
        #endif
        change_frame_mode(display, frame, desktop, themes);
        frame->type = program; 
      }
      else if(window_type[i] == atoms->wm_window_type_normal) {
        #ifdef SHOW_PROPERTIES
        printf("type: normal/decidedly unknown\n");
        #endif

        frame->type = unknown;
      }
      else if(window_type[i] == atoms->wm_window_type_dock) {
        #ifdef SHOW_PROPERTIES
        printf("type: dock\n");
        #endif
        change_frame_mode(display, frame, floating, themes);
        //change_frame_mode(display, frame, minimal);
        frame->type = system_program; 
      }
      else if(window_type[i] == atoms->wm_window_type_splash) {
        #ifdef SHOW_PROPERTIES
        printf("type: splash\n");
        #endif
        change_frame_mode(display, frame, floating, themes);
        frame->type = splash;
      }
      else if(window_type[i] ==atoms->wm_window_type_dialog) {
        #ifdef SHOW_PROPERTIES
        printf("type: dialog\n");
        #endif
        change_frame_mode(display, frame, floating, themes);
        frame->type = dialog;
      }
      else if(window_type[i] == atoms->wm_window_type_utility) {
        #ifdef SHOW_PROPERTIES
        printf("type: utility\n");
        #endif
        change_frame_mode(display, frame, floating, themes);
        //change_frame_mode(display, frame, minimal);        
        frame->type = utility;
      }
    }
  }

  if(frame->type == unknown) change_frame_mode(display, frame, floating, themes);
  if(contents != NULL) XFree(contents);
}

void 
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
    for(int i =0; i < items; i++) {
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

/* This function sets the icon size property on the window so that the program that created it
knows what size to make the icon it provides when we call XGetWMHints.  But I don't understand
why XSetIconSize provides a method to specify a variety of sizes when only one size is returned.
So instead I set the size to one value and change it for each subsequent call to XGetWMHints.
If this fails, it doesn't really matter.  We will still get the icon and just resize it.
*/
/*
void create_frame_icon_size (Display *display, struct Frame *frame, int new_size) {
  if(frame->icon_size != NULL) {
    if(frame->icon_size->min_width = new_size) return;
    free_frame_icon_size(display, frame);
  }
  //XIconSize *icon_size; //this allows us to specify a size for icons when we request them

  frame->icon_size = XAllocIconSize();
  if(frame->icon_size == NULL) return;
  frame->icon_size->min_width = 16;
  frame->icon_size->max_width = 16;
  frame->icon_size->min_height = 16;
  frame->icon_size->max_height = 16;
  //inc amount are already zero from XAlloc
  XSetIconSizes(display, frame->window, frame->icon_size, 1); 
}
void 
free_frame_icon_size(Display *display, struct Frame *frame) {
  if(frame->icon_size != NULL) XFree(frame->icon_size);
  frame->icon_size = NULL;
}

*/
