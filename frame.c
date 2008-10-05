
/* This function reparents a framed window to root and then destroys the frame as well as cleaning up the frames drawing surfaces */
/* It is used when the framed window has been unmapped or destroyed, or is about to be*/
void remove_frame(Display* display, struct Framelist* frames, int index) {
  
  XGrabServer(display);
  XSetErrorHandler(supress_xerror);
  XReparentWindow(display, frames->list[index].window, DefaultRootWindow(display), 0, 0);
  XRemoveFromSaveSet (display, frames->list[index].window); //this will not destroy the window because it has been reparented to root
  XDestroyWindow(display, frames->list[index].frame);
  XSync(display, False);
  XSetErrorHandler(NULL);    
  XUngrabServer(display);
  
  XFreePixmap(display, frames->list[index].title_menu.title_normal_p);
  XFreePixmap(display, frames->list[index].title_menu.title_pressed_p);
  XFreePixmap(display, frames->list[index].title_menu.title_deactivated_p);
  
  if(frames->list[index].window_name != NULL) XFree(frames->list[index].window_name);
   
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
    struct Frame* temp = NULL;
    temp = realloc(frames->list, sizeof(struct Frame) * frames->max * 2);
    if(temp != NULL) frames->list = temp;
    else return -1;
    frames->max *= 2;
  }

  XAddToSaveSet(display, framed_window); //add this window to the save set as soon as possible so that if an error occurs it is still available
  XGetWindowAttributes(display, framed_window, &attributes);
  
  /*** Set up defaults ***/
  if(attributes.width >= MINWIDTH) frame.w = attributes.width;
  else frame.w = 600;

  if(attributes.height >= MINHEIGHT) frame.h = attributes.height;
  else frame.h = 400;

  frame.x = attributes.x;
  frame.y = attributes.y;
  frame.min_width = MINWIDTH;
  frame.min_height = MINHEIGHT;
  frame.max_width = XWidthOfScreen(screen);
  frame.max_height = XHeightOfScreen(screen);
  frame.selected = 1;
  frame.window_name = NULL;
  frame.mode = FLOATING;
  frame.window = framed_window;    
    
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
  
  frame.title_menu.hotspot =   XCreateWindow(display, frame.titlebar, H_SPACING*2 + BUTTON_SIZE, V_SPACING,
                                      frame.w - TITLEBAR_USED_WIDTH, BUTTON_SIZE, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
  
  //doesn't matter if the width of the grips is a bit bigger as it will be under the frame_window anyway
  frame.l_grip = XCreateWindow(display, frame.frame, 0, TITLEBAR_HEIGHT,
                                      CORNER_GRIP_SIZE, frame.h - TITLEBAR_HEIGHT - CORNER_GRIP_SIZE, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
                                      
  frame.bl_grip = XCreateWindow(display, frame.frame, 0, frame.h - CORNER_GRIP_SIZE,
                                      CORNER_GRIP_SIZE, CORNER_GRIP_SIZE, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);

  frame.b_grip = XCreateWindow(display, frame.frame, CORNER_GRIP_SIZE, frame.h - CORNER_GRIP_SIZE,
                                      frame.w - CORNER_GRIP_SIZE*2, CORNER_GRIP_SIZE, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);

  frame.br_grip = XCreateWindow(display, frame.frame, frame.w - CORNER_GRIP_SIZE, frame.h - CORNER_GRIP_SIZE,
                                      CORNER_GRIP_SIZE, CORNER_GRIP_SIZE, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
                                      
  frame.r_grip = XCreateWindow(display, frame.frame, frame.w - CORNER_GRIP_SIZE, TITLEBAR_HEIGHT,
                                      CORNER_GRIP_SIZE, frame.h - TITLEBAR_HEIGHT - CORNER_GRIP_SIZE, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);  
  
  frame.backing = XCreateSimpleWindow(display, frame.frame, EDGE_WIDTH*2 + H_SPACING, TITLEBAR_HEIGHT + 1 + EDGE_WIDTH, //same y as body, with a constant width as the sides (so H_SPACING)
                                       frame.w - FRAME_HSPACE, frame.h - FRAME_VSPACE, 0, black, black); 
  
  get_frame_name(display, &frame);
  //TODO: add resize hotspots
  
  XSelectInput(display, frame.window, StructureNotifyMask | PropertyChangeMask);  //Property notify is used to update titles  

  resize_frame(display, &frame); //resize the title menu if it isn't at it's minimum
    
  XReparentWindow(display, frame.window, frame.backing, 0, 0);
  XFlush(display);
  frame.skip_reparent_unmap = 1;
      
  XSetWindowBorderWidth(display, frame.window, 0);  
  XResizeWindow(display, frame.window, frame.w - FRAME_HSPACE, frame.h - FRAME_VSPACE);
  frame.skip_resize_configure = 1;
  
  XSetWindowBackgroundPixmap(display, frame.frame, pixmaps->border_p );
  XSetWindowBackgroundPixmap(display, frame.title_menu.frame, pixmaps->border_p );
  XSetWindowBackgroundPixmap(display, frame.innerframe, pixmaps->border_p );
  XSetWindowBackgroundPixmap(display, frame.body, pixmaps->body_p );
  XSetWindowBackgroundPixmap(display, frame.title_menu.body, pixmaps->light_border_p );
  
  XSetWindowBackgroundPixmap(display, frame.titlebar,  pixmaps->titlebar_background_p );
  if(frame.mode == FLOATING) {
    XSetWindowBackgroundPixmap(display, frame.close_button, pixmaps->close_button_normal_p );
    XSetWindowBackgroundPixmap(display, frame.mode_pulldown, pixmaps->pulldown_floating_normal_p );
    XSetWindowBackgroundPixmap(display, frame.title_menu.arrow, pixmaps->arrow_normal_p);    
  }
  else if (frame.mode == TILING) {
    XSetWindowBackgroundPixmap(display, frame.close_button, pixmaps->close_button_normal_p );
    XSetWindowBackgroundPixmap(display, frame.mode_pulldown, pixmaps->pulldown_tiling_normal_p );
    XSetWindowBackgroundPixmap(display, frame.title_menu.arrow, pixmaps->arrow_normal_p);      
  }
  else if (frame.mode == SINKING) {
    XSetWindowBackgroundPixmap(display, frame.close_button, pixmaps->close_button_deactivated_p );
    XSetWindowBackgroundPixmap(display, frame.mode_pulldown, pixmaps->pulldown_deactivated_p );
    XSetWindowBackgroundPixmap(display, frame.title_menu.arrow, pixmaps->arrow_deactivated_p);        
  }

  if(frame.selected != 0) XSetWindowBackgroundPixmap(display, frame.selection_indicator, pixmaps->selection_p);
  else XSetWindowBackgroundPixmap(display, frame.selection_indicator, ParentRelative);
  
  XSelectInput(display, frame.frame,   Button1MotionMask | ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame.close_button,  ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
  XSelectInput(display, frame.mode_pulldown, ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
  XSelectInput(display, frame.title_menu.hotspot, ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
  XSelectInput(display, frame.l_grip,  ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame.bl_grip, ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame.b_grip,  ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame.br_grip, ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame.r_grip,  ButtonPressMask | ButtonReleaseMask);    

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

  //EMD DEFINES    
  XMapWindow(display, frame.frame); 
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
  XMapWindow(display, frame.backing );
  XMapWindow(display, frame.window);
  XMapWindow(display, frame.l_grip);
  XMapWindow(display, frame.bl_grip); 
  XMapWindow(display, frame.b_grip);
  XMapWindow(display, frame.br_grip);
  XMapWindow(display, frame.r_grip);
  
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
    XResizeWindow(display, frame->title_menu.hotspot, frame->title_menu.width + EDGE_WIDTH*4 + BUTTON_SIZE, BUTTON_SIZE);    
  }
  else {
    XResizeWindow(display, frame->title_menu.frame, frame->w - TITLEBAR_USED_WIDTH, BUTTON_SIZE);
    XResizeWindow(display, frame->title_menu.hotspot, frame->w - TITLEBAR_USED_WIDTH, BUTTON_SIZE);
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
  frame->skip_resize_configure++;

  XResizeWindow(display, frame->l_grip, CORNER_GRIP_SIZE, frame->h - TITLEBAR_HEIGHT - CORNER_GRIP_SIZE);
  XMoveResizeWindow(display, frame->bl_grip, 0, frame->h - CORNER_GRIP_SIZE, CORNER_GRIP_SIZE, CORNER_GRIP_SIZE);
  XMoveResizeWindow(display, frame->b_grip, CORNER_GRIP_SIZE, frame->h - CORNER_GRIP_SIZE, frame->w - CORNER_GRIP_SIZE*2, CORNER_GRIP_SIZE);
  XMoveResizeWindow(display, frame->br_grip, frame->w - CORNER_GRIP_SIZE, frame->h - CORNER_GRIP_SIZE, CORNER_GRIP_SIZE, CORNER_GRIP_SIZE);
  XMoveResizeWindow(display, frame->r_grip, frame->w - CORNER_GRIP_SIZE, TITLEBAR_HEIGHT, CORNER_GRIP_SIZE, frame->h - TITLEBAR_HEIGHT - CORNER_GRIP_SIZE);
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
      
  if(frame->mode == SINKING) XSetWindowBackgroundPixmap(display, frame->title_menu.title, frame->title_menu.title_deactivated_p);  
  else XSetWindowBackgroundPixmap(display, frame->title_menu.title, frame->title_menu.title_normal_p);  

  frame->title_menu.width = get_title_width(display, frame->window_name);
  XMapWindow(display, frame->title_menu.title);
  XFlush(display);
}

/*** Update with specified values if they are available ***/
void get_frame_hints(Display* display, struct Frame* frame) {
  XSizeHints specified;
  
  long pre_ICCCM; //pre ICCCM recovered values.  This isn't used but still needs to be passed on.
  
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
      frame->min_width = specified.min_width + FRAME_HSPACE;
      frame->min_height = specified.min_height + FRAME_VSPACE;
    }
    
    if(specified.flags & PMaxSize) {
      printf("Maximum size specified\n");
      frame->max_width = specified.max_width + FRAME_HSPACE;
      frame->max_height = specified.max_height + FRAME_VSPACE;
    }
  }

  frame->w += FRAME_HSPACE;
  frame->h += FRAME_VSPACE;
}
