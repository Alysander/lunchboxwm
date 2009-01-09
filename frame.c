extern int done;

/* This function reparents a framed window to root and then destroys the frame as well as cleaning up the frames drawing surfaces */
/* It is used when the framed window has been unmapped or destroyed, or is about to be*/
void remove_frame(Display* display, struct Frame_list* frames, int index) {
  XWindowChanges changes;
  Window root = DefaultRootWindow(display);
  unsigned int mask = CWSibling | CWStackMode;  

  changes.stack_mode = Below;
  changes.sibling = frames->list[index].frame;
  
  //always unmap the title pulldown before removing the frame or else it will crash
  XDestroyWindow(display, frames->list[index].title_menu.entry);
  
  XGrabServer(display);
  XSetErrorHandler(supress_xerror);
  XReparentWindow(display, frames->list[index].window, root, frames->list[index].x, frames->list[index].y);
  XConfigureWindow(display, frames->list[index].window, mask, &changes);  //keep the stacking order
  XRemoveFromSaveSet (display, frames->list[index].window); //this will not destroy the window because it has been reparented to root
  XDestroyWindow(display, frames->list[index].frame);
  XSync(display, False);
  XSetErrorHandler(NULL);    
  XUngrabServer(display);

  free_frame_name(display, &frames->list[index]);
  
  if((frames->used != 1) && (index != frames->used - 1)) { //the frame is not the first or the last
    frames->list[index] = frames->list[frames->used - 1]; //swap the deleted frame with the last frame
  }
  frames->used--;
}

/*This function is called when the close button on the frame is pressed */
void close_window(Display* display, Window framed_window) {
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
int create_frame(Display *display, struct Frame_list* frames
, Window framed_window, struct Pixmaps *pixmaps, struct Cursors *cursors, struct Atoms *atoms) {
  Window root = DefaultRootWindow(display);
  Screen* screen = DefaultScreenOfDisplay(display);
  int black = BlackPixelOfScreen(screen);
  XWindowAttributes get_attributes;
  struct Frame frame;
  
  if(frames->used == frames->max) {
    printf("reallocating, used %d, max%d\n", frames->used, frames->max);
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
  XGetWindowAttributes(display, framed_window, &get_attributes);
  XGetTransientForHint(display, framed_window, &frame.transient);
  
  /*** Set up defaults ***/
  frame.selected = 0;
  frame.window_name = NULL;
  frame.window = framed_window;
  frame.mode = FLOATING;     
  frame.title_menu.entry = root;
  frame.x = get_attributes.x;
  frame.y = get_attributes.y;
  frame.w = get_attributes.width;  
  frame.h = get_attributes.height;

  get_frame_hints(display, &frame);
  get_frame_type (display, &frame, atoms);
  get_frame_state(display, &frame, atoms);
  
  if(frame.type == desktop) {
    XReparentWindow(display, frame.window, frames->virtual_desktop, 0, 0);
    XResizeWindow(display, frame.window
    , XWidthOfScreen(screen), XHeightOfScreen(screen) - MENUBAR_HEIGHT);
    XMapWindow(display, frame.window);
    XFlush(display);
    //don't allocate a frame structure for the desktop window because it doesn't need a frame.
    //It is highly unlikely a single program would have more than one desktop window but
    //if it doesn this will jut map one over the other. Which is what current ones do anyway.
    //TODO make sure the remove_frame_list checks for children and reparents them too
    return -1;
  }
    
  frame.frame =         XCreateSimpleWindow(display, root, frame.x, frame.y
  , frame.w, frame.h, 0, black, black); 
  
  frame.body =          XCreateSimpleWindow(display, frame.frame, EDGE_WIDTH, TITLEBAR_HEIGHT + 1
  , frame.w - EDGE_WIDTH*2, frame.h - (TITLEBAR_HEIGHT + EDGE_WIDTH + 1) , 0, black, black);

  //same y as body, with a constant width as the sides (so H_SPACING)
  frame.innerframe =    XCreateSimpleWindow(display, frame.frame, EDGE_WIDTH + H_SPACING, TITLEBAR_HEIGHT + 1
  , frame.w - (EDGE_WIDTH + H_SPACING)*2, frame.h - (TITLEBAR_HEIGHT + EDGE_WIDTH + 1 + H_SPACING), 0, black, black); 
                                                                           
  frame.titlebar =      XCreateSimpleWindow(display, frame.frame
  , EDGE_WIDTH, EDGE_WIDTH
  , frame.w - EDGE_WIDTH*2, TITLEBAR_HEIGHT, 0, black, black);
  
  frame.close_button =  XCreateSimpleWindow(display, frame.titlebar
  , frame.w - H_SPACING - BUTTON_SIZE - EDGE_WIDTH*2, V_SPACING
  , BUTTON_SIZE, BUTTON_SIZE, 0, black, black);

  frame.close_hotspot = XCreateWindow(display, frame.frame
  , frame.w - H_SPACING - BUTTON_SIZE - EDGE_WIDTH*2, 0
  , BUTTON_SIZE + H_SPACING + EDGE_WIDTH*2, BUTTON_SIZE + V_SPACING + EDGE_WIDTH*2
  , 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
                                      
  frame.mode_pulldown = XCreateSimpleWindow(display, frame.titlebar
  , frame.w - H_SPACING*2 - PULLDOWN_WIDTH - BUTTON_SIZE - EDGE_WIDTH*2, V_SPACING
  , PULLDOWN_WIDTH, BUTTON_SIZE, 0, black, black);

  frame.mode_hotspot = XCreateWindow(display, frame.frame
  , frame.w - H_SPACING*2 - PULLDOWN_WIDTH - BUTTON_SIZE - EDGE_WIDTH*2, 0
  , PULLDOWN_WIDTH, BUTTON_SIZE + V_SPACING + EDGE_WIDTH*2, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);

  frame.selection_indicator = XCreateSimpleWindow(display, frame.titlebar, H_SPACING, V_SPACING
  , BUTTON_SIZE, BUTTON_SIZE,  0, black, black);

  frame.title_menu.frame =  XCreateSimpleWindow(display, frame.titlebar, H_SPACING*2 + BUTTON_SIZE, V_SPACING
  , frame.w - TITLEBAR_USED_WIDTH, BUTTON_SIZE, 0, black, black);

  frame.title_menu.body = XCreateSimpleWindow(display, frame.title_menu.frame, EDGE_WIDTH, EDGE_WIDTH
  , frame.w - TITLEBAR_USED_WIDTH - EDGE_WIDTH*2, BUTTON_SIZE - EDGE_WIDTH*2, 0, black, black);

  frame.title_menu.title = XCreateSimpleWindow(display, frame.title_menu.body, EDGE_WIDTH, EDGE_WIDTH
  , frame.w - TITLE_MAX_WIDTH_DIFF, TITLE_MAX_HEIGHT, 0, black, black);
                                      
  frame.title_menu.arrow =  XCreateSimpleWindow(display, frame.title_menu.body
  , frame.w - TITLEBAR_USED_WIDTH - EDGE_WIDTH*2 - BUTTON_SIZE, EDGE_WIDTH
  , BUTTON_SIZE - EDGE_WIDTH , BUTTON_SIZE - EDGE_WIDTH*4, 0, black,  black);
  
  frame.title_menu.hotspot =   XCreateWindow(display, frame.frame
  , H_SPACING*2 + BUTTON_SIZE + EDGE_WIDTH, 0
  , frame.w - TITLEBAR_USED_WIDTH, BUTTON_SIZE + V_SPACING + EDGE_WIDTH*2, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);

  //doesn't matter if the width of the grips is a bit bigger as it will be under the frame_window anyway
  frame.l_grip = XCreateWindow(display, frame.frame, 0, TITLEBAR_HEIGHT + 1 + EDGE_WIDTH*2
  , CORNER_GRIP_SIZE, frame.h - TITLEBAR_HEIGHT - CORNER_GRIP_SIZE - EDGE_WIDTH*2 - 1, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
                                      
  frame.bl_grip = XCreateWindow(display, frame.frame, 0, frame.h - CORNER_GRIP_SIZE
  , CORNER_GRIP_SIZE, CORNER_GRIP_SIZE, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);

  frame.b_grip = XCreateWindow(display, frame.frame, CORNER_GRIP_SIZE, frame.h - CORNER_GRIP_SIZE
  , frame.w - CORNER_GRIP_SIZE*2, CORNER_GRIP_SIZE, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);

  frame.br_grip = XCreateWindow(display, frame.frame, frame.w - CORNER_GRIP_SIZE, frame.h - CORNER_GRIP_SIZE
  , CORNER_GRIP_SIZE, CORNER_GRIP_SIZE, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
                                      
  frame.r_grip = XCreateWindow(display, frame.frame, frame.w - CORNER_GRIP_SIZE, TITLEBAR_HEIGHT + EDGE_WIDTH*2 + 1
  , CORNER_GRIP_SIZE, frame.h - TITLEBAR_HEIGHT - CORNER_GRIP_SIZE - EDGE_WIDTH*2 - 1, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);  

  //same y as body, with a constant width as the sides (so H_SPACING)
  frame.backing = XCreateSimpleWindow(display, frame.frame, EDGE_WIDTH*2 + H_SPACING, TITLEBAR_HEIGHT + 1 + EDGE_WIDTH
  , frame.w - FRAME_HSPACE, frame.h - FRAME_VSPACE, 0, black, black); 
  
  //same y as body, with a constant width as the sides so H_SPACING
  frame.title_menu.entry = XCreateSimpleWindow(display, frames->title_menu, 10, 10
  , XWidthOfScreen(screen), MENU_ITEM_HEIGHT, 0, black, black); 
  
  load_frame_name(display, &frame);
  
  resize_frame(display, &frame); //resize the title menu if it isn't at it's minimum
  show_frame_state(display, &frame, pixmaps);
  
  XMoveWindow(display, frame.window, 0, 0);
  XResizeWindow(display, frame.window, frame.w - FRAME_HSPACE, frame.h - FRAME_VSPACE);
  XSetWindowBorderWidth(display, frame.window, 0);
  XReparentWindow(display, frame.window, frame.backing, 0, 0);
  
  XSetWindowBackgroundPixmap(display, frame.frame, pixmaps->border_p );
  XSetWindowBackgroundPixmap(display, frame.title_menu.frame, pixmaps->border_p );
  XSetWindowBackgroundPixmap(display, frame.innerframe, pixmaps->border_p );
  XSetWindowBackgroundPixmap(display, frame.body, pixmaps->body_p );
  XSetWindowBackgroundPixmap(display, frame.title_menu.body, pixmaps->light_border_p );
  XSetWindowBackgroundPixmap(display, frame.titlebar,  pixmaps->titlebar_background_p );

  XSync(display, False);  //this prevents the Reparent unmap being reported.
  
  XSelectInput(display, frame.frame,   Button1MotionMask | ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame.close_hotspot,  ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
  XSelectInput(display, frame.mode_hotspot, ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame.title_menu.hotspot, ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame.l_grip,  ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame.bl_grip, ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame.b_grip,  ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame.br_grip, ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame.r_grip,  ButtonPressMask | ButtonReleaseMask);    
  XSelectInput(display, frame.window,  StructureNotifyMask | PropertyChangeMask);
  //Property notify is used to update titles, structureNotify for destroyNotify events

  XSelectInput(display, frame.backing, SubstructureRedirectMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
  XSelectInput(display, frame.title_menu.entry, ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
  
  XDefineCursor(display, frame.frame, cursors->normal);
  XDefineCursor(display, frame.titlebar, cursors->normal);
  XDefineCursor(display, frame.body, cursors->normal);
  XDefineCursor(display, frame.close_hotspot, cursors->normal);
  XDefineCursor(display, frame.mode_hotspot, cursors->normal);
  XDefineCursor(display, frame.selection_indicator, cursors->normal);
  XDefineCursor(display, frame.title_menu.frame, cursors->normal);
  XDefineCursor(display, frame.title_menu.body, cursors->normal);
  XDefineCursor(display, frame.title_menu.arrow, cursors->normal);
  XDefineCursor(display, frame.title_menu.hotspot, cursors->normal);
  XDefineCursor(display, frame.innerframe, cursors->normal);
  XDefineCursor(display, frame.l_grip, cursors->resize_h);
  XDefineCursor(display, frame.bl_grip, cursors->resize_tr_bl); 
  XDefineCursor(display, frame.b_grip, cursors->resize_v);
  XDefineCursor(display, frame.br_grip, cursors->resize_tl_br);
  XDefineCursor(display, frame.r_grip, cursors->resize_h);
  XDefineCursor(display, frame.title_menu.entry, cursors->normal);

  XMapWindow(display, frame.titlebar);
  XMapWindow(display, frame.body);
  XMapWindow(display, frame.close_button);
  XMapWindow(display, frame.close_hotspot);  
  XMapWindow(display, frame.mode_hotspot);
  XMapWindow(display, frame.mode_pulldown);
  XMapWindow(display, frame.selection_indicator);
  XMapWindow(display, frame.title_menu.frame);
  XMapWindow(display, frame.title_menu.body);
  XMapWindow(display, frame.title_menu.arrow);
  XMapWindow(display, frame.title_menu.hotspot);
  XMapWindow(display, frame.innerframe);
  XMapWindow(display, frame.window);
  XMapWindow(display, frame.l_grip);
  XMapWindow(display, frame.bl_grip); 
  XMapWindow(display, frame.b_grip);
  XMapWindow(display, frame.br_grip);
  XMapWindow(display, frame.r_grip);
  XMapWindow(display, frame.title_menu.entry);
  XMapWindow(display, frame.backing );  
  XMapWindow(display, frame.frame);

  //Intercept clicks so we can set the focus and possibly raise floating windows
  XGrabButton(display, Button1, 0, frame.backing, False, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
  // printf("Passive click grab reported: %d\n", );
  
  frames->list[frames->used] = frame;
  frames->used++;
  return (frames->used - 1);
} 

void show_frame_state(Display *display, struct Frame *frame,  struct Pixmaps *pixmaps) {
  XUnmapWindow(display, frame->title_menu.frame);
  XUnmapWindow(display, frame->mode_pulldown);
  XUnmapWindow(display, frame->close_button);
  XUnmapWindow(display, frame->selection_indicator);

  XFlush(display);
  if(frame->mode == FLOATING) {
    XSetWindowBackgroundPixmap(display, frame->close_button, pixmaps->close_button_normal_p );
    XSetWindowBackgroundPixmap(display, frame->mode_pulldown, pixmaps->pulldown_floating_normal_p );
    XSetWindowBackgroundPixmap(display, frame->title_menu.arrow, pixmaps->arrow_normal_p);    
    XSetWindowBackgroundPixmap(display, frame->title_menu.title, frame->title_menu.title_normal_p);
  }
  else if (frame->mode == TILING) {
    XSetWindowBackgroundPixmap(display, frame->close_button, pixmaps->close_button_normal_p );
    XSetWindowBackgroundPixmap(display, frame->mode_pulldown, pixmaps->pulldown_tiling_normal_p );
    XSetWindowBackgroundPixmap(display, frame->title_menu.arrow, pixmaps->arrow_normal_p);      
    XSetWindowBackgroundPixmap(display, frame->title_menu.title, frame->title_menu.title_normal_p);
  }
  else if (frame->mode == SINKING) {
    XSetWindowBackgroundPixmap(display, frame->close_button, pixmaps->close_button_deactivated_p );
    XSetWindowBackgroundPixmap(display, frame->mode_pulldown, pixmaps->pulldown_deactivated_p );
    XSetWindowBackgroundPixmap(display, frame->title_menu.arrow, pixmaps->arrow_deactivated_p);
    XSetWindowBackgroundPixmap(display, frame->title_menu.title, frame->title_menu.title_deactivated_p);
  }

  if(frame->selected != 0) XSetWindowBackgroundPixmap(display, frame->selection_indicator, pixmaps->selection_p);
  else XSetWindowBackgroundPixmap(display, frame->selection_indicator, ParentRelative);
  
  XMapWindow(display, frame->title_menu.frame);
  XMapWindow(display, frame->mode_pulldown);
  XMapWindow(display, frame->close_button);
  XMapWindow(display, frame->selection_indicator);
  XFlush(display);
}

/*** Moves the frame and doesn't resize it **/
void move_frame(Display* display, struct Frame* frame) {
  XMoveWindow(display, frame->frame, frame->x, frame->y);
  XFlush(display);
}

/*** Moves and resizes the subwindows of the frame ***/
void resize_frame(Display* display, struct Frame* frame) {
  XMoveResizeWindow(display, frame->frame, frame->x, frame->y,  frame->w, frame->h);
  XResizeWindow(display, frame->titlebar, frame->w - EDGE_WIDTH*2, TITLEBAR_HEIGHT);
  XMoveWindow(display, frame->close_button, frame->w - H_SPACING - BUTTON_SIZE - EDGE_WIDTH - 1, V_SPACING);
  XMoveWindow(display, frame->close_hotspot, frame->w - H_SPACING - BUTTON_SIZE - EDGE_WIDTH - 1, 0);
  XMoveWindow(display, frame->mode_pulldown, frame->w - H_SPACING*2 - PULLDOWN_WIDTH - BUTTON_SIZE - EDGE_WIDTH - 1, V_SPACING);
  XMoveWindow(display, frame->mode_hotspot, frame->w - H_SPACING*2 - PULLDOWN_WIDTH - BUTTON_SIZE - EDGE_WIDTH - 1, 0);

  if(frame->title_menu.width + EDGE_WIDTH*4 + BUTTON_SIZE < frame->w - TITLEBAR_USED_WIDTH) {
    XResizeWindow(display, frame->title_menu.frame, frame->title_menu.width + EDGE_WIDTH*4 + BUTTON_SIZE, BUTTON_SIZE);
    XMoveWindow(display, frame->title_menu.arrow,   frame->title_menu.width + EDGE_WIDTH*2, EDGE_WIDTH);        
    XResizeWindow(display, frame->title_menu.body,  frame->title_menu.width + EDGE_WIDTH*2 + BUTTON_SIZE, BUTTON_SIZE - EDGE_WIDTH*2);
    XResizeWindow(display, frame->title_menu.title, frame->title_menu.width + EDGE_WIDTH, TITLE_MAX_HEIGHT);
    XResizeWindow(display, frame->title_menu.hotspot, frame->title_menu.width + EDGE_WIDTH*4 + BUTTON_SIZE, BUTTON_SIZE + V_SPACING + EDGE_WIDTH);
  }
  else {
    XResizeWindow(display, frame->title_menu.frame, frame->w - TITLEBAR_USED_WIDTH, BUTTON_SIZE);
    XResizeWindow(display, frame->title_menu.hotspot, frame->w - TITLEBAR_USED_WIDTH, BUTTON_SIZE + V_SPACING + EDGE_WIDTH);
    XResizeWindow(display, frame->title_menu.body,  frame->w - TITLEBAR_USED_WIDTH - EDGE_WIDTH*2, BUTTON_SIZE - EDGE_WIDTH*2);
    XResizeWindow(display, frame->title_menu.title, frame->w - TITLE_MAX_WIDTH_DIFF, TITLE_MAX_HEIGHT);
    XMoveWindow(display, frame->title_menu.arrow, frame->w - TITLEBAR_USED_WIDTH - EDGE_WIDTH*2 - BUTTON_SIZE, EDGE_WIDTH);
  }
  
  XResizeWindow(display, frame->body, frame->w - EDGE_WIDTH*2, frame->h - (TITLEBAR_HEIGHT + EDGE_WIDTH + 1));
  XResizeWindow(display, frame->innerframe, 
                         frame->w - (EDGE_WIDTH + H_SPACING)*2,
                         frame->h - (TITLEBAR_HEIGHT + EDGE_WIDTH + 1 + H_SPACING));
  XResizeWindow(display, frame->backing, frame->w - FRAME_HSPACE, frame->h - FRAME_VSPACE);
  XResizeWindow(display, frame->window, frame->w - FRAME_HSPACE, frame->h - FRAME_VSPACE);

  XResizeWindow(display, frame->l_grip, CORNER_GRIP_SIZE, frame->h - TITLEBAR_HEIGHT - CORNER_GRIP_SIZE - EDGE_WIDTH*2 - 1);
  XMoveResizeWindow(display, frame->bl_grip, 0, frame->h - CORNER_GRIP_SIZE, CORNER_GRIP_SIZE, CORNER_GRIP_SIZE);
  XMoveResizeWindow(display, frame->b_grip, CORNER_GRIP_SIZE, frame->h - CORNER_GRIP_SIZE, frame->w - CORNER_GRIP_SIZE*2, CORNER_GRIP_SIZE);
  XMoveResizeWindow(display, frame->br_grip, frame->w - CORNER_GRIP_SIZE, frame->h - CORNER_GRIP_SIZE, CORNER_GRIP_SIZE, CORNER_GRIP_SIZE);
  XMoveResizeWindow(display, frame->r_grip, frame->w - CORNER_GRIP_SIZE, TITLEBAR_HEIGHT + EDGE_WIDTH*2 + 1
  , CORNER_GRIP_SIZE, frame->h - TITLEBAR_HEIGHT - CORNER_GRIP_SIZE - EDGE_WIDTH*2 - 1);
  
  XMoveWindow(display, frame->window, 0,0);
  XFlush(display);
}

void free_frame_name(Display* display, struct Frame* frame) {
  if(frame->window_name != NULL) {
    XFree(frame->window_name);
    frame->window_name = NULL;
    
    XFreePixmap(display, frame->title_menu.title_normal_p);
    XFreePixmap(display, frame->title_menu.title_pressed_p);    
    XFreePixmap(display, frame->title_menu.title_deactivated_p);

    XFreePixmap(display, frame->title_menu.item_title_p);
    XFreePixmap(display, frame->title_menu.item_title_hover_p);
    XFlush(display);
  }
}

/*** Update pixmaps with the specified name if it is available.  ***/
void load_frame_name(Display* display, struct Frame* frame) {
  char untitled[10] = "noname";
  
  struct Frame temp = *frame; 
  //Only make changes if required and only unmap after pixmaps have been created.
  
  XFetchName(display, temp.window, &temp.window_name);

  if(temp.window_name == NULL) {
    printf("Warning: unnamed window\n");
    XStoreName(display, temp.window, untitled);
    XFlush(display);
    XFetchName(display, temp.window, &temp.window_name);
  }
  else 
  if(frame->window_name != NULL
  && strcmp(temp.window_name, frame->window_name) == 0) {
    XFree(temp.window_name);
    //skip this if the name hasn't changed
    return;
  }

  temp.title_menu.title_normal_p = create_title_pixmap(display, temp.window_name, title_normal);
  temp.title_menu.title_pressed_p = create_title_pixmap(display, temp.window_name, title_pressed);
  temp.title_menu.title_deactivated_p = create_title_pixmap(display, temp.window_name, title_deactivated);

  temp.title_menu.item_title_p = create_title_pixmap(display, temp.window_name, item_title);
  temp.title_menu.item_title_active_p = create_title_pixmap(display, temp.window_name, item_title_active);

  temp.title_menu.item_title_deactivated_p = create_title_pixmap(display, temp.window_name, item_title_deactivated);
  temp.title_menu.item_title_hover_p = create_title_pixmap(display, temp.window_name, item_title_hover);
  temp.title_menu.item_title_active_hover_p = create_title_pixmap(display, temp.window_name, item_title_active_hover);
  temp.title_menu.item_title_deactivated_hover_p = create_title_pixmap(display, temp.window_name, item_title_deactivated_hover);
   
  XUnmapWindow(display, frame->title_menu.title);
  XUnmapWindow(display, frame->title_menu.entry);
  XFlush(display);
  free_frame_name(display, frame);
  *frame = temp;
  
  if(frame->mode == SINKING) XSetWindowBackgroundPixmap(display, frame->title_menu.title, frame->title_menu.title_deactivated_p);
  else XSetWindowBackgroundPixmap(display, frame->title_menu.title, frame->title_menu.title_normal_p);    
  XSetWindowBackgroundPixmap(display, frame->title_menu.entry, frame->title_menu.item_title_p);
  XMapWindow(display, frame->title_menu.entry);  
  XMapWindow(display, frame->title_menu.title);
    
  frame->title_menu.width = get_title_width(display, frame->window_name);
  if(frame->window_name == untitled) frame->window_name = NULL;

  XFlush(display);
}

/*** Update with specified values if they are available.  
These values are for the framed_window and do not include the frame ***/
void get_frame_hints(Display* display, struct Frame* frame) {
  Screen* screen = DefaultScreenOfDisplay(display);
  Window root = DefaultRootWindow(display);
  
  XSizeHints specified;
  long pre_ICCCM; //pre ICCCM recovered values which are ignored.

  printf("BEFORE: width %d, height %d, x %d, y %d\n", 
        frame->w, frame->h, frame->x, frame->y);

  frame->max_width  = XWidthOfScreen(screen) - FRAME_HSPACE;
  frame->max_height = XHeightOfScreen(screen) - MENUBAR_HEIGHT - FRAME_VSPACE;  
  frame->min_width  = MINWIDTH - FRAME_HSPACE;
  frame->min_height = MINHEIGHT - FRAME_VSPACE;

  #ifdef ALLOW_OVERSIZE_WINDOWS_WITHOUT_MINIMUM_HINTS
  /* Ugh Horrible.  */
  /* Many apps that are resizeable like gedit ask to be the size of the screen
     and then become much more difficult to be used. */
     
  if(frame->w > XWidthOfScreen(screen)) {
    frame->min_width = frame->w;
    frame->max_width = frame->min_width;      
  }
  if(frame->h > XHeightOfScreen(screen)) {
    frame->min_height = frame->h;
    frame->max_height = frame->min_height;
  }
  #endif

  if(XGetWMNormalHints(display, frame->window, &specified, &pre_ICCCM) != 0) {
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
  frame->w += FRAME_HSPACE; 
  frame->h += FRAME_VSPACE; 

  frame->max_width  += FRAME_HSPACE; 
  frame->max_height += FRAME_VSPACE; 

  frame->min_width  += FRAME_HSPACE; 
  frame->min_height += FRAME_VSPACE; 

  #ifdef SHOW_FRAME_HINTS      
  printf("width %d, height %d, min_width %d, max_width %d, min_height %d, max_height %d, x %d, y %d\n"
  , frame->w, frame->h, frame->min_width, frame->max_width, frame->min_height, frame->max_height, frame->x, frame->y);
  #endif
  check_frame_limits(display, frame);
}

int replace_frame(Display *display, struct Frame *target, struct Frame *replacement, Window sinking_seperator, Window tiling_seperator, Window floating_seperator, struct Pixmaps *pixmaps) {
  XWindowChanges changes;
  unsigned int mask = CWX | CWY | CWWidth | CWHeight;

  if(replacement->window == target->window) return 0;  //this can be chosen from the title menu
  if(target->w < replacement->min_width
  || target->h < replacement->min_height) {
    #ifdef SHOW_BUTTON_PRESS_EVENT
    printf("The requested window doesn't fit on the target window\n");
    #endif
    return 0;
  }  
  changes.x = target->x;
  changes.y = target->y;
  
  if(target->w < replacement->max_width) changes.width = target->w;  //save info for request
  else changes.width = replacement->max_width;
      
  if(target->h < replacement->max_height) changes.height = target->h; 
  else changes.height = replacement->max_height;
  
  replacement->mode = target->mode;
  replacement->x = changes.x;
  replacement->y = changes.y;
  replacement->w = changes.width;
  replacement->h = changes.height;  
  target->mode = SINKING;
  
  XConfigureWindow(display, replacement->frame, mask, &changes);
  resize_frame(display, replacement);
  show_frame_state(display, target, pixmaps);
  show_frame_state(display, replacement, pixmaps);
  stack_frame(display, target, sinking_seperator, tiling_seperator, floating_seperator);
  stack_frame(display, replacement, sinking_seperator, tiling_seperator, floating_seperator);  
  
  return 1;
}

/* Implements stacking and focus policy */
void stack_frame(Display *display, struct Frame *frame, Window sinking_seperator, Window tiling_seperator, Window floating_seperator) {
  XWindowChanges changes;
  
  unsigned int mask = CWSibling | CWStackMode;  
  changes.stack_mode = Below;
    
  if(frame->mode == TILING)        changes.sibling = tiling_seperator;
  else if(frame->mode == SINKING)  changes.sibling = sinking_seperator;
  else if(frame->mode == FLOATING) changes.sibling = floating_seperator;  

  XSetInputFocus(display, frame->window, RevertToPointerRoot, CurrentTime);   //TODO
  XConfigureWindow(display, frame->frame, mask, &changes);
  XFlush(display);
}

//maybe remove overlap variable and instead simply set the indirect resize parameters and just reset them is something goes wrong?
void resize_tiling_frame(Display *display, struct Frame_list *frames, int index, char axis, int position, int size) {
  /******
  Purpose:  
    Resizes a window and enlarges any adjacent tiled windows in either axis
     up to a maximum size for the adjacent windows or to a minimum size for the shrinking window.
  Preconditions:  
    axis is 'x' or 'y',
    frames is a valid Frame_list,
    index is a valid index to frames,
    display is valid x11 connection,
    size is the new width or height 
    position is the new x or y co-ordinate and is within a valid range.
  *****/
  
  int shrink_margin = 1;
  
  int size_change;
  int overlap;
  int adj_position, adj_size;
  int frame_space;
      
  //variables for the index frame and variables for the iterated frame
  int *min_size, *fmin_size;
  int *max_size, *fmax_size;
  int *s, *fs;  //size
  int *p, *fp;  //position
  int *fs_adj;  //size of range of values of adjacent frames in perpendicular axis
  int *fp_adj;  //position of range of values of range of adjacent frames in perpendicular axis
      
  if(axis == 'x') {
    min_size = &frames->list[index].min_width;
    max_size = &frames->list[index].max_width;
    s = &frames->list[index].w;
    p = &frames->list[index].x;
    adj_position = frames->list[index].y;
    adj_size = frames->list[index].h;
    frame_space = FRAME_HSPACE;
  }
  else if(axis == 'y') {
    min_size = &frames->list[index].min_height;
    max_size = &frames->list[index].max_height;
    s = &frames->list[index].h;
    p = &frames->list[index].y;
    adj_position = frames->list[index].x;
    adj_size = frames->list[index].w;
    frame_space = FRAME_VSPACE;
  }

  size_change = size - *s; //the size difference for the specified frame
  
  if(size < *min_size + frame_space  
  || size > *max_size
  || size_change == 0) return;
  
  if(size_change > 0) shrink_margin = 0;

  #ifdef SHOW_EDGE_RESIZE
  printf("\n\nResize: %c, position %d, size %d\n", axis, position, size);
  #endif
  while(!done) {  
    int i = 0;
    if(size < *min_size + frame_space  ||  size > *max_size) return;
    
    for(; i < frames->used; i++) {
      if(i == index) {
        //skipping index frame
        frames->list[index].indirect_resize.new_size = 0; 
        continue;
      }

      if(frames->list[i].mode == TILING) {

        if(axis == 'x') {
          fs = &frames->list[i].w;
          fp = &frames->list[i].x;
          fmin_size = &frames->list[i].min_width;
          fmax_size = &frames->list[i].max_width;
          fs_adj = &frames->list[i].h;
          fp_adj = &frames->list[i].y;
        }
        else if(axis == 'y') {
          fs = &frames->list[i].h;
          fp = &frames->list[i].y;
          fmin_size = &frames->list[i].min_height;
          fmax_size = &frames->list[i].max_height;
          fs_adj = &frames->list[i].w;
          fp_adj = &frames->list[i].x;
        }
        //if within perpendicular range
        if((adj_position + adj_size > *fp_adj  &&  adj_position <= *fp_adj)
        || (adj_position < *fp_adj + *fs_adj   &&  adj_position >= *fp_adj)
        || (adj_position <= *fp_adj  &&  adj_position + adj_size >= *fp_adj + *fs_adj)
        ) {
          
          #ifdef SHOW_EDGE_RESIZE
          printf("Frame \" %s \" inside perp range. \n", frames->list[i].window_name);
          #endif
          //the size_change < 0 test determines the direction of the drag and which side is affected
          if( *p == position
          &&  *p + *s >= *fp + *fs
          &&  *p + *s <= *fp + *fs - size_change
          &&  size_change < 0 //shrinking
          ) {
            overlap = (*fp + *fs) - (size + position);
            #ifdef SHOW_EDGE_RESIZE
            printf("above/below RHS aligned, shrinking %d\n", overlap);
            #endif
            frames->list[i].indirect_resize.new_position = *fp;
          }
          
          else if(*p < position
          &&  *p <= *fp
          &&  *p - size_change >= *fp
          &&  size_change < 0 //shrinking
          ) {           
            overlap = position - *fp;
            #ifdef SHOW_EDGE_RESIZE
            printf("above/below LHS aligned, shrinking %d\n", overlap);
            #endif
            frames->list[i].indirect_resize.new_position = *fp + overlap;
          }

         else if( *p == position //find adjacent for enlarging (indirect enlarge)
          &&  *p + *s == *fp + *fs
          &&  *fp <= *p + *s
          &&  size_change > 0 //enlarging
          ) {
            overlap = -size_change;
            #ifdef SHOW_EDGE_RESIZE
            printf("above/below RHS aligned, enlarging %d \n", overlap);
            #endif
            frames->list[i].indirect_resize.new_position = *fp;
          }
          
          else if( *p > position //find adjacent for enlarging
          &&  *p == *fp
          &&  size_change > 0 //enlarging
          ) {
                 
            overlap = -size_change;
            #ifdef SHOW_EDGE_RESIZE
            printf("above/below LHS aligned, enlarging %d HEY\n", overlap);
            #endif
            frames->list[i].indirect_resize.new_position = position;
          }
          
          else if(*p + *s + shrink_margin > *fp //find adjacent for shrinking
          &&  *p + *s <= *fp 
          &&  size_change < 0 //shrinking
          &&  *p == position) {

            overlap = position + size - *fp;
            #ifdef SHOW_EDGE_RESIZE
            printf("found window adjacent to RHS, shrinking %d\n", overlap);
            #endif
            frames->list[i].indirect_resize.new_position = *fp + overlap;
          }
          else if(position + size  > *fp //find overlapped for enlarging
          &&  position < *fp
          &&  size_change > 0 //enlarging       
          &&  *p == position) {
            overlap = position + size - *fp;
            #ifdef SHOW_EDGE_RESIZE
            printf("found window overlapped on RHS, enlarging %d\n", overlap);
            #endif
            frames->list[i].indirect_resize.new_position = *fp + overlap;          
          }
          else if(*p < *fp + *fs + shrink_margin //find adjacent for shrinking
          &&  *p > *fp
          &&  size_change < 0 //shrinking
          &&  *p < position) { 
            overlap = *fp + *fs - position;
            #ifdef SHOW_EDGE_RESIZE
            printf("found window adjacent to LHS shrinking %d\n", overlap);
            #endif
            frames->list[i].indirect_resize.new_position = *fp;                  
          }
          else if(position < *fp + *fs //find overlapped for enlarging
          &&  position > *fp  //new position on other side of window
          &&  *p > position   //moving left
          &&  *fp < *p        //initial position on RHS of window
          &&  size_change > 0 //enlarging
          ) {
            overlap = *fp + *fs - position;
            #ifdef SHOW_EDGE_RESIZE
            printf("found window overlapped on LHS, enlarging %d\n", overlap);
            #endif
            frames->list[i].indirect_resize.new_position = *fp;                              
          }
          else {
            frames->list[i].indirect_resize.new_size = 0; //horizontally out of the way
            continue;
          }

         /* if windows are adjacent and being affected, 
             we need to check if we need to increase the size of opposing axis potentential range
             in order to get indirect resizes of other windows */            
          if(*fp_adj < adj_position  &&  *fp_adj + *fs_adj - adj_position > adj_size) {
            #ifdef SHOW_EDGE_RESIZE
            printf("enlarging adjacency area\n");
            #endif
            //completely encloses the area that encloses the adjacent windows
            
            adj_position = *fp_adj;
            adj_size = *fs_adj;
            break; //this will cause the loop to reset with the new values
          }
          else if(*fp_adj + *fs_adj - adj_position > adj_size) {
            //extends below the area that encloses the adjacent windows
            #ifdef SHOW_EDGE_RESIZE
            printf("enlarging adjacency area in h\n");
            #endif
            adj_size = *fp_adj + *fs_adj - adj_position;
            #ifdef SHOW_EDGE_RESIZE
            printf("adj_position %d, adj_size %d \n", adj_position, adj_size);
            #endif
            break; //this will cause the loop to reset with the new value
          }
          else if(*fp_adj < adj_position) {
            //extends above the adjacency area
            #ifdef SHOW_EDGE_RESIZE
            printf("enlarging adjacency area in position \n");
            #endif
            adj_size = adj_position + adj_size - *fp_adj;
            adj_position = *fp_adj;
            break; //this will cause the loop to reset with the new values
          }

          
          if(*fs - overlap >= *fmin_size + frame_space &&
             *fs - overlap <= *fmax_size) {
            frames->list[i].indirect_resize.new_size = *fs - overlap;
          }
          else if (overlap > 0 ) {
            #ifdef SHOW_EDGE_RESIZE
            printf("not within min/max adjusting.. finding max allowed value \n");
            #endif
            int new_size;
            if(position == *p) {
              new_size = *fp + *fmin_size + frame_space - *p;
            }
            else if(position < *p) {
              new_size = *p + *s - position;
              position = *fp + *fmin_size + frame_space;
            }

            if(new_size >= size) {      
              #ifdef SHOW_EDGE_RESIZE        
              printf("Cannot resize, cancelling resize.");
              #endif
              return;
            }
            size = new_size;
            break;
          }
          else if(frames->list[i].indirect_resize.new_size > *fmax_size) { //too big
            #ifdef SHOW_EDGE_RESIZE 
            printf("Setting size to max\n");
            #endif
            frames->list[i].indirect_resize.new_size = *fmax_size; //make it the maximum
          }              
        }
        else {
          frames->list[i].indirect_resize.new_size = 0; //vertically out of the way
          continue;
        }
      }
    }
    
    if(i == frames->used) break;
  }
 
  if(!done) { //resize all modified windows.
    *p = position;
    *s = size;
    for(int i = 0; i < frames->used; i++) {
      if(frames->list[i].mode == TILING  &&  frames->list[i].indirect_resize.new_size) {
        if(axis == 'x') {
          frames->list[i].x = frames->list[i].indirect_resize.new_position;
          frames->list[i].w = frames->list[i].indirect_resize.new_size;
        } 
        else if(axis == 'y') {
          frames->list[i].y = frames->list[i].indirect_resize.new_position; 
          frames->list[i].h = frames->list[i].indirect_resize.new_size;
        }
        resize_frame(display, &frames->list[i]);      
      }
    }
  }
  
  resize_frame(display, &frames->list[index]);
  return;
}

void get_frame_type(Display *display, struct Frame *frame, struct Atoms *atoms) {
  unsigned char *contents = NULL;
  Atom return_type;
  int return_format;
  unsigned long items;
  unsigned long bytes;
  XGetWindowProperty(display, frame->window, atoms->wm_window_type, 0, 1 //long long_length?
  , False, AnyPropertyType, &return_type, &return_format,  &items, &bytes, &contents);

  frame->type = normal; 
  if(return_type == XA_ATOM  && contents != NULL) {
    Atom *window_type = (Atom*)contents;
    #ifdef SHOW_PROPERTIES
    printf("Number of atoms %lu\n", items);
    #endif
    for(int i =0; i < items; i++) {
    //these are fairly mutually exclusive so be suprised if there are others
      if(window_type[i] == atoms->wm_window_type_desktop) {
        #ifdef SHOW_PROPERTIES
        printf("type: desktop\n");
        #endif
        frame->type = desktop;
      }
      else if(window_type[i] == atoms->wm_window_type_normal) {
        #ifdef SHOW_PROPERTIES
        printf("type: normal\n");
        #endif
        frame->type = normal;
      }
      else if(window_type[i] == atoms->wm_window_type_dock) {
        #ifdef SHOW_PROPERTIES
        printf("type: dock\n");
        #endif
        frame->type = dock;
      }
      else if(window_type[i] == atoms->wm_window_type_splash) {
        #ifdef SHOW_PROPERTIES
        printf("type: splash\n");
        #endif
        frame->type = splash;
      }
      else if(window_type[i] ==atoms->wm_window_type_dialog) {
        #ifdef SHOW_PROPERTIES
        printf("type: dialog\n");
        #endif
        frame->type = dialog;
      }
      else if(window_type[i] == atoms->wm_window_type_utility) {
        #ifdef SHOW_PROPERTIES
        printf("type: utility\n");
        #endif
        frame->type = utility;
      }
    }
  }
  if(contents != NULL) XFree(contents);
}

void get_frame_state(Display *display, struct Frame *frame, struct Atoms *atoms) {
  unsigned char *contents = NULL;
  Atom return_type;
  int return_format;
  unsigned long items;
  unsigned long bytes;
  XGetWindowProperty(display, frame->window, atoms->wm_state, 0, 1 //long long_length?
  , False, AnyPropertyType, &return_type, &return_format,  &items, &bytes, &contents);
  
  printf("loading state\n");
  frame->state = unstated; 
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

void check_frame_limits(Display *display, struct Frame *frame) {
  Screen* screen = DefaultScreenOfDisplay(display);  
  if(frame->w < frame->min_width)  frame->w = frame->min_width;
  else if(frame->w > frame->max_width) frame->w = frame->max_width;

  if(frame->h < frame->min_height) frame->h = frame->min_height;
  else if(frame->h > frame->max_height) frame->h = frame->max_height;

  if(frame->x + frame->w > XWidthOfScreen(screen)) 
    frame->x = XWidthOfScreen(screen) - frame->w;
  if(frame->y + frame->h > XHeightOfScreen(screen) - MENUBAR_HEIGHT) 
    frame->y = XHeightOfScreen(screen)- frame->h - MENUBAR_HEIGHT;
  if(frame->x < 0) frame->x = 0;
  if(frame->y < 0) frame->y = 0;

}
