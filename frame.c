
/* This function reparents a framed window to root and then destroys the frame as well as cleaning up the frames drawing surfaces */
/* It is used when the framed window has been unmapped or destroyed, or is about to be*/
void remove_frame(Display* display, struct Framelist* frames, int index) {
  //always unmap the title pulldown before removing the frame or else it will crash
  XDestroyWindow(display, frames->list[index].title_menu.entry);
  
  XGrabServer(display);
  XSetErrorHandler(supress_xerror);
  XReparentWindow(display, frames->list[index].window, DefaultRootWindow(display), frames->list[index].x, frames->list[index].y);
  XRemoveFromSaveSet (display, frames->list[index].window); //this will not destroy the window because it has been reparented to root
  XDestroyWindow(display, frames->list[index].frame);
  XSync(display, False);
  XSetErrorHandler(NULL);    
  XUngrabServer(display);

  XFreePixmap(display, frames->list[index].title_menu.title_normal_p);
  XFreePixmap(display, frames->list[index].title_menu.title_pressed_p);
  XFreePixmap(display, frames->list[index].title_menu.title_deactivated_p);
  
  XFreePixmap(display, frames->list[index].title_menu.item_title_p);
  XFreePixmap(display, frames->list[index].title_menu.item_title_hover_p);
  
  if(frames->list[index].window_name != NULL) XFree(frames->list[index].window_name);
  if(frames->list[index].program_name != NULL) XFree(frames->list[index].program_name);
  
  if((frames->used != 1) && (index != frames->used - 1)) { //the frame is not the first or the last
    frames->list[index] = frames->list[frames->used - 1]; //swap the deleted frame with the last frame
  }
  frames->used--;
}

/*This function is called when the close button on the frame is pressed */
void remove_window(Display* display, Window framed_window) {
  int n, found = 0;
  Atom *protocols;
  
  if (XGetWMProtocols(display, framed_window, &protocols, &n)) {
    for (int i = 0; i < n; i++)
      if (protocols[i] == XInternAtom(display, "WM_DELETE_WINDOW", False)) {
        found++;
        break;
      }
    XFree(protocols);
  }
  
  if(found)  {
    XClientMessageEvent event;
    event.type = ClientMessage;
    event.window = framed_window;
    event.format = 32;
    event.message_type = XInternAtom(display, "WM_PROTOCOLS", False);
    event.data.l[0] = (long)XInternAtom(display, "WM_DELETE_WINDOW", False);
    event.data.l[1] = CurrentTime;
    XSendEvent(display, framed_window, False, NoEventMask, (XEvent *)&event);
  }
  else {
    printf("Killed client %d\n", framed_window);
    XKillClient(display, framed_window);
  }
  
}

/*This function is used to add frames to windows that are already open when the WM is starting*/
void create_startup_frames (Display *display, struct Framelist* frames, struct frame_pixmaps *pixmaps, struct mouse_cursors *cursors) {
  unsigned int windows_length;
  Window root, parent, children, *windows;
  XWindowAttributes attributes;
  int index;
  root = DefaultRootWindow(display);
  
  XQueryTree(display, root, &parent, &children, &windows, &windows_length);
  if(windows != NULL) for (int i = 0; i < windows_length; i++)  {
    XGetWindowAttributes(display, windows[i], &attributes);
    if (attributes.map_state == IsViewable && !attributes.override_redirect) {
      create_frame(display, frames, windows[i], pixmaps, cursors);
    }
  }
  XFree(windows);
}


/*returns the frame index of the newly created window or -1 if out of memory */
int create_frame(Display* display, struct Framelist* frames, Window framed_window, struct frame_pixmaps *pixmaps, struct mouse_cursors *cursors) { 
  Window root = DefaultRootWindow(display);
  Screen* screen = DefaultScreenOfDisplay(display);
  Visual* colours = XDefaultVisual(display, DefaultScreen(display));
  int black = BlackPixelOfScreen(screen);
  
  XWindowAttributes attributes; //fallback if the specified size hints don't work
  struct Frame frame;
   
  Window transient; //this window is actually used to store a return value
  
  if(frames->used == frames->max) {
    printf("reallocating, used %d, max%d\n", frames->used, frames->max);
    struct Frame* temp = NULL;
    temp = realloc(frames->list, sizeof(struct Frame) * frames->max * 2);
    if(temp != NULL) frames->list = temp;
    else return -1;
    frames->max *= 2;
  }
  printf("Creating new frame at %d with window %d\n", frames->used, framed_window);
  XAddToSaveSet(display, framed_window); //add this window to the save set as soon as possible so that if an error occurs it is still available
  XSync(display, False);
  XGetWindowAttributes(display, framed_window, &attributes);
  
  /*** Set up defaults ***/
  printf("attributes width %d, height %d\n", attributes.width, attributes.height);

  frame.selected = 1;
  frame.window_name = NULL;
  frame.program_name = NULL;
  frame.mode = FLOATING;
  frame.window = framed_window;      
  frame.title_menu.entry = root;
  get_frame_hints(display, &frame);
    
  frame.frame =         XCreateSimpleWindow(display, root, frame.x, frame.y, 
                                      frame.w, frame.h, 0, black, black); 
  
  frame.body =          XCreateSimpleWindow(display, frame.frame, EDGE_WIDTH, TITLEBAR_HEIGHT + 1, 
                                      frame.w - EDGE_WIDTH*2, frame.h - (TITLEBAR_HEIGHT + EDGE_WIDTH + 1) , 0, black, black);

  frame.innerframe =    XCreateSimpleWindow(display, frame.frame, EDGE_WIDTH + H_SPACING, TITLEBAR_HEIGHT + 1, //same y as body, with a constant width as the sides (so H_SPACING)
                                      frame.w - (EDGE_WIDTH + H_SPACING)*2, frame.h - (TITLEBAR_HEIGHT + EDGE_WIDTH + 1 + H_SPACING), 0, black, black); 
                                                                           
  frame.titlebar =      XCreateSimpleWindow(display, frame.frame, EDGE_WIDTH, EDGE_WIDTH, 
                                      frame.w - EDGE_WIDTH*2, TITLEBAR_HEIGHT, 0, black, black);
  
  frame.close_button =  XCreateSimpleWindow(display, frame.titlebar, frame.w - H_SPACING - BUTTON_SIZE - EDGE_WIDTH*2, V_SPACING,
                                      BUTTON_SIZE, BUTTON_SIZE, 0, black, black);
                                      
  frame.mode_pulldown = XCreateSimpleWindow(display, frame.titlebar, frame.w - H_SPACING*2 - PULLDOWN_WIDTH - BUTTON_SIZE - EDGE_WIDTH*2, V_SPACING,
                                      PULLDOWN_WIDTH, BUTTON_SIZE, 0, black, black);

  frame.selection_indicator = XCreateSimpleWindow(display, frame.titlebar, H_SPACING, V_SPACING,
                                      BUTTON_SIZE, BUTTON_SIZE,  0, black, black);


  frame.title_menu.frame =  XCreateSimpleWindow(display, frame.titlebar, H_SPACING*2 + BUTTON_SIZE, V_SPACING, 
                                      frame.w - TITLEBAR_USED_WIDTH, BUTTON_SIZE, 0, black, black);

  frame.title_menu.body = XCreateSimpleWindow(display, frame.title_menu.frame, EDGE_WIDTH, EDGE_WIDTH, 
                                      frame.w - TITLEBAR_USED_WIDTH - EDGE_WIDTH*2, BUTTON_SIZE - EDGE_WIDTH*2, 0, black, black);

  frame.title_menu.title = XCreateSimpleWindow(display, frame.title_menu.body, EDGE_WIDTH, EDGE_WIDTH, 
                                      frame.w - TITLE_MAX_WIDTH_DIFF, TITLE_MAX_HEIGHT, 0, black, black);
                                      
  frame.title_menu.arrow =  XCreateSimpleWindow(display, frame.title_menu.body, frame.w - TITLEBAR_USED_WIDTH - EDGE_WIDTH*2 - BUTTON_SIZE, EDGE_WIDTH, 
                                      BUTTON_SIZE - EDGE_WIDTH , BUTTON_SIZE - EDGE_WIDTH*4, 0, black,  black);
  
  frame.title_menu.hotspot =   XCreateWindow(display, frame.frame, H_SPACING*2 + BUTTON_SIZE + EDGE_WIDTH, 0,
                                      frame.w - TITLEBAR_USED_WIDTH, BUTTON_SIZE + V_SPACING + EDGE_WIDTH, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
  
  //doesn't matter if the width of the grips is a bit bigger as it will be under the frame_window anyway
  frame.l_grip = XCreateWindow(display, frame.frame, 0, TITLEBAR_HEIGHT + 1 + EDGE_WIDTH*2,
                                      CORNER_GRIP_SIZE, frame.h - TITLEBAR_HEIGHT - CORNER_GRIP_SIZE - EDGE_WIDTH*2 - 1, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
                                      
  frame.bl_grip = XCreateWindow(display, frame.frame, 0, frame.h - CORNER_GRIP_SIZE,
                                      CORNER_GRIP_SIZE, CORNER_GRIP_SIZE, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);

  frame.b_grip = XCreateWindow(display, frame.frame, CORNER_GRIP_SIZE, frame.h - CORNER_GRIP_SIZE,
                                      frame.w - CORNER_GRIP_SIZE*2, CORNER_GRIP_SIZE, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);

  frame.br_grip = XCreateWindow(display, frame.frame, frame.w - CORNER_GRIP_SIZE, frame.h - CORNER_GRIP_SIZE,
                                      CORNER_GRIP_SIZE, CORNER_GRIP_SIZE, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
                                      
  frame.r_grip = XCreateWindow(display, frame.frame, frame.w - CORNER_GRIP_SIZE, TITLEBAR_HEIGHT + EDGE_WIDTH*2 + 1,
                                      CORNER_GRIP_SIZE, frame.h - TITLEBAR_HEIGHT - CORNER_GRIP_SIZE - EDGE_WIDTH*2 - 1, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);  
  
  frame.backing = XCreateSimpleWindow(display, frame.frame, EDGE_WIDTH*2 + H_SPACING, TITLEBAR_HEIGHT + 1 + EDGE_WIDTH, //same y as body, with a constant width as the sides (so H_SPACING)
                                       frame.w - FRAME_HSPACE, frame.h - FRAME_VSPACE, 0, black, black); 
  
  //same y as body, with a constant width as the sides so H_SPACING
  frame.title_menu.entry = XCreateSimpleWindow(display, frames->title_menu, 10, 10, 
                                         XWidthOfScreen(screen), MENU_ITEM_HEIGHT, 0, black, black); 
                                       
  get_frame_name(display, &frame);
  get_frame_program_name(display, &frame);
  
  resize_frame(display, &frame); //resize the title menu if it isn't at it's minimum
  show_frame_state(display, &frame, pixmaps);
  
  XMoveWindow(display, frame.window, 0, 0);    
  XReparentWindow(display, frame.window, frame.backing, 0, 0);
  XFlush(display);
      
  XSetWindowBorderWidth(display, frame.window, 0);  
  XResizeWindow(display, frame.window, frame.w - FRAME_HSPACE, frame.h - FRAME_VSPACE);
  
  XSetWindowBackgroundPixmap(display, frame.frame, pixmaps->border_p );
  XSetWindowBackgroundPixmap(display, frame.title_menu.frame, pixmaps->border_p );
  XSetWindowBackgroundPixmap(display, frame.innerframe, pixmaps->border_p );
  XSetWindowBackgroundPixmap(display, frame.body, pixmaps->body_p );
  XSetWindowBackgroundPixmap(display, frame.title_menu.body, pixmaps->light_border_p );
  XSetWindowBackgroundPixmap(display, frame.titlebar,  pixmaps->titlebar_background_p );

  XSync(display, False);  //this prevents the Reparent unmap being reported.
  XSelectInput(display, frame.frame,   Button1MotionMask | ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame.backing, SubstructureRedirectMask);  
  XSelectInput(display, frame.close_button,  ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
  XSelectInput(display, frame.mode_pulldown, ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame.title_menu.hotspot, ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame.l_grip,  ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame.bl_grip, ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame.b_grip,  ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame.br_grip, ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame.r_grip,  ButtonPressMask | ButtonReleaseMask);    
  XSelectInput(display, frame.window,  StructureNotifyMask |PropertyChangeMask);  
  //Property notify is used to update titles, structureNotify for destroyNotify events
  //why not use substructure redirect all the time????
  XSelectInput(display, frame.backing, ButtonPressMask | EnterWindowMask | LeaveWindowMask);
  XSelectInput(display, frame.title_menu.entry, ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);  
  
  XDefineCursor(display, frame.frame, cursors->normal);
  XDefineCursor(display, frame.titlebar, cursors->normal);
  XDefineCursor(display, frame.body, cursors->normal);
  XDefineCursor(display, frame.close_button, cursors->normal);
  XDefineCursor(display, frame.mode_pulldown, cursors->normal);
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
   printf("Passive click grab reported: %d\n", XGrabButton(display, Button1, 0, frame.backing, False, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None));
     
  XGetTransientForHint(display, framed_window, &transient);
  if(transient != 0) {
    printf("Transient window detected\n");
    //set pop-windows to have focus
    XSetInputFocus(display, frame.window, RevertToPointerRoot, CurrentTime);
  }
  
  frames->list[frames->used] = frame;
  frames->used++;
  return (frames->used - 1);
} 

void show_frame_state(Display *display, struct Frame *frame,  struct frame_pixmaps *pixmaps) {
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

/*** Moves and resizes the subwindows of the frame ***/
void resize_frame(Display* display, struct Frame* frame) {
  XMoveResizeWindow(display, frame->frame, frame->x, frame->y,  frame->w, frame->h);
  XResizeWindow(display, frame->titlebar, frame->w - EDGE_WIDTH*2, TITLEBAR_HEIGHT);
  XMoveWindow(display, frame->close_button, frame->w - H_SPACING - BUTTON_SIZE - EDGE_WIDTH - 1, V_SPACING);
  XMoveWindow(display, frame->mode_pulldown, frame->w - H_SPACING*2 - PULLDOWN_WIDTH - BUTTON_SIZE - EDGE_WIDTH - 1, V_SPACING);

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
  XMoveResizeWindow(display, frame->r_grip, frame->w - CORNER_GRIP_SIZE, TITLEBAR_HEIGHT + EDGE_WIDTH*2 + 1, CORNER_GRIP_SIZE, frame->h - TITLEBAR_HEIGHT - CORNER_GRIP_SIZE - EDGE_WIDTH*2 - 1);
  XMoveWindow(display, frame->window, 0,0);
  //had an XSynch here in a vain attempt to stop getting bogus configure requests
  //XSync(display, False);
  XFlush(display);
}

void get_frame_program_name(Display* display, struct Frame* frame) {
  XClassHint program_hint;
  if(XGetClassHint(display, frame->window, &program_hint)) {
    printf("res_name %s, res_class %s\n", program_hint.res_name, program_hint.res_class);
    if(program_hint.res_name != NULL) XFree(program_hint.res_name);
    frame->program_name = program_hint.res_class;
  }
  XFlush(display);
}

/*** Update with the specified name if it is available ***/
void get_frame_name(Display* display, struct Frame* frame) {
  
  XFetchName(display, frame->window, &frame->window_name);
  
  XUnmapWindow(display, frame->title_menu.title);
  XFlush(display);

  frame->title_menu.title_normal_p = create_title_pixmap(display, frame->window_name, title_normal);
  frame->title_menu.title_pressed_p = create_title_pixmap(display, frame->window_name, title_pressed);
  frame->title_menu.title_deactivated_p = create_title_pixmap(display, frame->window_name, title_deactivated);

  frame->title_menu.item_title_p = create_title_pixmap(display, frame->window_name, item_title);
  frame->title_menu.item_title_hover_p = create_title_pixmap(display, frame->window_name, item_title_hover);

  if(frame->mode == SINKING) XSetWindowBackgroundPixmap(display, frame->title_menu.title, frame->title_menu.title_deactivated_p);
  else XSetWindowBackgroundPixmap(display, frame->title_menu.title, frame->title_menu.title_normal_p);  
  
  XUnmapWindow(display, frame->title_menu.entry);
  XFlush(display);
  XSetWindowBackgroundPixmap(display, frame->title_menu.entry, frame->title_menu.item_title_p);
  XMapWindow(display, frame->title_menu.entry);  
  
  frame->title_menu.width = get_title_width(display, frame->window_name);
  XMapWindow(display, frame->title_menu.title);
  XFlush(display);
}

/*** Update with specified values if they are available.  
These values are for the framed_window and do not include the frame ***/
void get_frame_hints(Display* display, struct Frame* frame) {
  Screen* screen;
  Window root;
  
  XWindowAttributes attributes;
  XSizeHints specified;
  long pre_ICCCM; //pre ICCCM recovered values which are ignored.
  
  root = DefaultRootWindow(display);
  screen = DefaultScreenOfDisplay(display);

  XGetWindowAttributes(display, frame->window, &attributes);
  frame->x = attributes.x;
  frame->y = attributes.y;  
  printf("existing x,y: %d, %d\n", frame->x, frame->y);
  frame->min_width = MINWIDTH;
  frame->min_height = MINHEIGHT;
  frame->max_width = XWidthOfScreen(screen) -  FRAME_HSPACE;
  frame->max_height = XHeightOfScreen(screen) -  FRAME_VSPACE;  
   
  if(XGetWMNormalHints(display, frame->window, &specified, &pre_ICCCM) != 0) {
    printf("Managed to recover size hints\n");
    
    if((specified.flags & PPosition != 0) || (specified.flags & USPosition != 0)) {
      printf("Position specified\n");
      frame->x = specified.x;
      frame->y = specified.y;
    }
    if((specified.flags & PSize) || (specified.flags & USSize)) {
      printf("Size specified\n");
      frame->w = specified.width;
      frame->h = specified.height;
    }
    if((specified.flags & PMinSize) && (specified.min_width >= MINWIDTH) && (specified.min_height >= MINHEIGHT)) {
      printf("Minimum size specified\n");
      frame->min_width = specified.min_width;
      frame->min_height = specified.min_height;
    }
    if(specified.flags & PMaxSize) {
      printf("Maximum size specified\n");
      frame->max_width = specified.max_width;
      frame->max_height = specified.max_height;
    }
  }
  
  if(attributes.width < frame->min_width) frame->w = frame->min_width;
  else frame->w = attributes.width;

  if(attributes.height < frame->min_height) frame->h = frame->min_height;
  else frame->h = attributes.height;
  
  if(frame->w > frame->max_width) frame->w = frame->max_width;
  if(frame->h > frame->max_height) frame->h = frame->max_height;
    
  frame->w += FRAME_HSPACE; //increase the size of the window for the frame to be drawn in
  frame->h += FRAME_VSPACE; 
  printf("width %d, height %d, min_width %d, max_width %d, min_height %d, max_height %d, x %d, y %d\n", 
        frame->w, frame->h, frame->min_width, frame->max_width, frame->min_height, frame->max_height, frame->x, frame->y);
        
  //put splash screens where they specify, not off-centre
  if(frame->x - (H_SPACING + EDGE_WIDTH*2)  >  0) frame->x -= H_SPACING + EDGE_WIDTH*2;
  if(frame->y - (TITLEBAR_HEIGHT + EDGE_WIDTH*2) > 0) frame->y -= TITLEBAR_HEIGHT + EDGE_WIDTH*2; 
}

int replace_frame(Display *display, struct Frame *target, struct Frame *replacement, struct frame_pixmaps *pixmaps) {
  XWindowChanges changes;
  unsigned int mask = CWX | CWY | CWSibling | CWStackMode;

  if(replacement->window == target->window) return 0;
  changes.x = target->x;
  changes.y = target->y;
  changes.sibling = target->frame;
  changes.stack_mode = Above;
 
  if(target->w < replacement->max_width) {
    if(target->w < replacement->min_width) {
      printf("The requested window is too wide to fit on target window\n");
      return 0;
    }
    changes.width = target->w;
    replacement->w = target->w;
    mask |= CWWidth;
  }
  
  if(target->h < replacement->max_height) {
    if(target->h < replacement->min_height) {
      printf("The requested window is too tall to fit on target window\n");
      return 0;
    }  
    changes.height = target->h; 
    replacement->h = target->h;
    mask |= CWHeight;
  }
  
  replacement->mode = target->mode;
  replacement->x = changes.x;
  replacement->y = changes.y;
  target->mode = SINKING;
  
  XConfigureWindow(display, replacement->frame, mask, &changes);
  XSetInputFocus(display, replacement->window, RevertToPointerRoot, CurrentTime);
  resize_frame(display, replacement);
  show_frame_state(display, target, pixmaps);
  show_frame_state(display, replacement, pixmaps);
      
  return 1;
}

//maybe remove overlap variable and instead simply set the indirect resize parameters and just reset them is something goes wrong?
void enlarge_frame(Display *display, struct Framelist *frames, int index, char axis, int position, int size) {  
  /******
  Purpose:  
    Enlargens a window and any adjacent tiled windows in either axis or does nothing
    if the adjacent windows reach a minimum size.
  Preconditions:  
    axis is 'x' or 'y',
    frames is a valid Framelist,
    index is a valid index to frames,
    display is valid x11 connection,
    size is the new width or height and is within a valid range
    position is the new x or y co-ordinate and is within a valid range.
  *****/
  if((axis == 'x') && (size < frames->list[index].min_width + FRAME_HSPACE
    || size > frames->list[index].max_width)) return;
  if((axis == 'y') && (size < frames->list[index].min_height + FRAME_VSPACE
    || size > frames->list[index].max_height)) return;
  
  printf("En: %c, position %d, size %d\n", axis, position, size);
  for(int i = 0; i < frames->used; i++) {
    if(i == index) {
      frames->list[index].indirect_resize.new_width = 0;
      frames->list[index].indirect_resize.new_height = 0;
      continue;
    }
    
    if(frames->list[i].mode == TILING) {
      int overlap;
      if(axis == 'x') {
        if((frames->list[index].y + frames->list[index].h > frames->list[i].y  &&  frames->list[index].y < frames->list[i].y)
            || (frames->list[index].y < frames->list[i].y + frames->list[i].h  &&  frames->list[index].y > frames->list[i].y)) {
          if(position + size > frames->list[i].x  &&  position < frames->list[i].x) {
            //RHS now overlaps other windows LHS
            overlap = position + size - frames->list[i].x;
            printf("RHS overlaps other window LHS by %d\n", overlap);
            frames->list[i].indirect_resize.new_x = frames->list[i].x + overlap;
          }
          else if(position < frames->list[i].x + frames->list[i].w  &&  position > frames->list[i].x) {
            //LHS now overlaps other windows RHS
            overlap =  frames->list[i].x + frames->list[i].w - position;
            printf("LHS overlaps other window RHS by %d\n", overlap);
            frames->list[i].indirect_resize.new_x = frames->list[i].x;
          }
          else {
            frames->list[i].indirect_resize.new_width = 0; //horizontally out of the way
            continue;
          }
        }
        else {
          frames->list[i].indirect_resize.new_width = 0; //vertically out of the way
          continue;
        }
        
        if(frames->list[i].w - overlap >= frames->list[i].min_width) {
          frames->list[i].indirect_resize.new_width = frames->list[i].w - overlap;
        }
        else {
          int new_overlap = frames->list[i].w - (frames->list[i].min_width + FRAME_HSPACE);
          printf("trying to find a happy medium in x\n");
          if((new_overlap > 0)  &&  (new_overlap != overlap)) {
            //need to adjust position in some cases.
            if(position < frames->list[index].x) position += overlap - new_overlap;
            enlarge_frame(display, frames, index, 'x', position, size - (overlap - new_overlap));
          }
          else printf("Nope: Adjacent windows minimum width reached\n");
          return;
        }
      }
      else if(axis == 'y') {
        if((frames->list[index].x + frames->list[index].w > frames->list[i].x  &&  frames->list[index].x < frames->list[i].x)
            || (frames->list[index].x < frames->list[i].x + frames->list[i].w  &&  frames->list[index].x > frames->list[i].x)) {

          if(position + size > frames->list[i].y  &&  position < frames->list[i].y) {
            //bottom now overlaps other windows top
            overlap = position + size - frames->list[i].y; 
            frames->list[i].indirect_resize.new_y = frames->list[i].y + overlap;
          }
          else if(position < frames->list[i].y + frames->list[i].h  &&  position > frames->list[i].y) {
            //top now overlaps other windows bottom
            overlap =  frames->list[i].y + frames->list[i].h - position;
            frames->list[i].indirect_resize.new_y = frames->list[i].y;
          }
          else { //vertically out of the way
            frames->list[i].indirect_resize.new_height = 0;
            continue;
          }
        }
        else { //horizontally out of the way
          frames->list[i].indirect_resize.new_height = 0;
          continue;
        }
        
        if(frames->list[i].h - overlap > frames->list[i].min_height) {
          frames->list[i].indirect_resize.new_height = frames->list[i].h - overlap;
        }
        else { 
          int new_overlap = frames->list[i].h - (frames->list[i].min_height + FRAME_VSPACE);
          if((new_overlap > 0)  && ( new_overlap != overlap)) {
            printf("trying to find a happy medium in y\n");
            if(position < frames->list[index].y) position += overlap - new_overlap;
            enlarge_frame(display, frames, index, 'y', position, size - (overlap - new_overlap));
          }
          else printf("Nope: Adjacent windows minimum height reached\n");
          return;
        }
      }
    }
  }

  if(axis == 'x') {
    frames->list[index].x = position;
    frames->list[index].w = size;
    for(int i = 0; i < frames->used; i++) {
      if(frames->list[i].mode == TILING  &&  frames->list[i].indirect_resize.new_width) {
        frames->list[i].x = frames->list[i].indirect_resize.new_x;
        frames->list[i].w = frames->list[i].indirect_resize.new_width;        
        resize_frame(display, &frames->list[i]);
      }
    }
  }
  else if(axis == 'y') {
    frames->list[index].y = position;
    frames->list[index].h = size;
    for(int i = 0; i < frames->used; i++) {
      if(frames->list[i].mode == TILING  &&  frames->list[i].indirect_resize.new_height) {
        frames->list[i].y = frames->list[i].indirect_resize.new_y; 
        frames->list[i].h = frames->list[i].indirect_resize.new_height;
        resize_frame(display, &frames->list[i]);
      }
    }
  }
  resize_frame(display, &frames->list[index]);
  
  return;
}


void shrink_frame(Display *display, struct Framelist *frames, int index, char axis, int position, int size) {
  /******
  Purpose:  
    Shrinks an window and enlarges any adjacent tiled windows in either axis
     up to a maximum size for the adjacent windows or to a minimum size for the shrinking window.
  Preconditions:  
    axis is 'x' or 'y',
    frames is a valid Framelist,
    index is a valid index to frames,
    display is valid x11 connection,
    size is the new width or height 
    position is the new x or y co-ordinate and is within a valid range.
  *****/
  
  int increase; //convenience variable notes how much to increase adjacent windows by.
  if(axis == 'x') {
    if(size < frames->list[index].min_width + FRAME_HSPACE || size > frames->list[index].max_width) return;
    increase = frames->list[index].w - size;
    if(increase <= 0) return;
  }
  if(axis == 'y') {
    if(size < frames->list[index].min_height + FRAME_VSPACE || size > frames->list[index].max_height) return;
    increase = frames->list[index].h - size;
    if(increase <= 0) return;
  }
  
  printf("Shrink: %c, position %d, size %d\n", axis, position, size);
  for(int i = 0; i < frames->used; i++) {
    if(i == index) {
      frames->list[index].indirect_resize.new_width = 0;
      frames->list[index].indirect_resize.new_height = 0;
      continue;
    }
    

    if(frames->list[i].mode == TILING) {
      if(axis == 'x') {
        if((frames->list[index].y + frames->list[index].h > frames->list[i].y  &&  frames->list[index].y < frames->list[i].y)
            || (frames->list[index].y < frames->list[i].y + frames->list[i].h  &&  frames->list[index].y > frames->list[i].y)) {
          
          if(frames->list[index].x + frames->list[index].w + SHRINK_GRIP_MARGIN > frames->list[i].x  &&  frames->list[index].x < frames->list[i].x) {
            //window is adjacent to this windows RHS
            frames->list[i].indirect_resize.new_x = frames->list[i].x - increase;
            frames->list[i].indirect_resize.new_width = frames->list[i].w + increase;
          }
          else if(frames->list[index].x < frames->list[i].x + frames->list[i].w + SHRINK_GRIP_MARGIN  &&  frames->list[index].x > frames->list[i].x) {
            //window is adjacent to this windows LHS
            frames->list[i].indirect_resize.new_width = frames->list[i].w + increase;           
            frames->list[i].indirect_resize.new_x = frames->list[i].x;
          }
          else {
            frames->list[i].indirect_resize.new_width = 0; //horizontally out of the way
            continue;
          }
        }
        else {
          frames->list[i].indirect_resize.new_width = 0; //vertically out of the way
          continue;
        }
        
        if(frames->list[i].indirect_resize.new_width > frames->list[i].max_width) { //too big
          frames->list[i].indirect_resize.new_width = frames->list[i].max_width; //make it the maximum
        }
      }
      else if(axis == 'y') {
        if((frames->list[index].x + frames->list[index].w > frames->list[i].x  &&  frames->list[index].x < frames->list[i].x)
            || (frames->list[index].x < frames->list[i].x + frames->list[i].w  &&  frames->list[index].x > frames->list[i].x)) {

          if(frames->list[index].x + frames->list[index].w + SHRINK_GRIP_MARGIN > frames->list[i].y  &&  frames->list[index].x < frames->list[i].y) {
            //window is adjacent this windows bottom
            frames->list[i].indirect_resize.new_y = frames->list[i].h - increase;
            frames->list[i].indirect_resize.new_height = frames->list[i].h + increase;
          }
          else if(frames->list[index].x < frames->list[i].y + frames->list[i].h + SHRINK_GRIP_MARGIN  &&  frames->list[index].x > frames->list[i].y) {
            //window is adjacent to this windows top
            frames->list[i].indirect_resize.new_y = frames->list[i].y;
            frames->list[i].indirect_resize.new_height = frames->list[i].h + increase;
          }
          else { //vertically out of the way
            frames->list[i].indirect_resize.new_height = 0;
            continue;
          }
        }
        else { //horizontally out of the way
          frames->list[i].indirect_resize.new_height = 0;
          continue;
        }
        
        if(frames->list[i].indirect_resize.new_height > frames->list[i].max_height) { //too big
          frames->list[i].indirect_resize.new_height = frames->list[i].max_height; //make it the maximum
        }
      }
    }
  }

  if(axis == 'x') {
    frames->list[index].x = position;
    frames->list[index].w = size;
    for(int i = 0; i < frames->used; i++) {
      if(frames->list[i].mode == TILING  &&  frames->list[i].indirect_resize.new_width) {
        frames->list[i].x = frames->list[i].indirect_resize.new_x;
        frames->list[i].w = frames->list[i].indirect_resize.new_width;        
        resize_frame(display, &frames->list[i]);
      }
    }
  }
  else if(axis == 'y') {
    frames->list[index].y = position;
    frames->list[index].h = size;
    for(int i = 0; i < frames->used; i++) {
      if(frames->list[i].mode == TILING  &&  frames->list[i].indirect_resize.new_height) {
        frames->list[i].y = frames->list[i].indirect_resize.new_y; 
        frames->list[i].h = frames->list[i].indirect_resize.new_height;
        resize_frame(display, &frames->list[i]);
      }
    }
  }
  resize_frame(display, &frames->list[index]);
  
  return;
}


