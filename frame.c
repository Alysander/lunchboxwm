extern int done;

/* This function reparents a framed window to root and then destroys the frame as well as cleaning up the frames drawing surfaces */
/* It is used when the framed window has been unmapped or destroyed, or is about to be*/
void remove_frame(Display* display, struct Frame_list* frames, int index) {
  XWindowChanges changes;
  Window root = DefaultRootWindow(display);
  unsigned int mask = CWSibling | CWStackMode;  

  changes.stack_mode = Below;
  changes.sibling = frames->list[index].frame;

  //free_frame_icon_size(display, &frames->list[index]);
  //always unmap the title pulldown before removing the frame or else it will crash
  XDestroyWindow(display, frames->list[index].menu_item.backing);
  
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
  //XGetWindowAttributes shouldn't be in get_frame_hints because it reports 0,0 after reparenting
  XGetWindowAttributes(display, framed_window, &get_attributes);
  
  /*** Set up defaults ***/
  frame.selected = 0;
  frame.window_name = NULL;
  frame.window = framed_window;
  frame.type = unknown;
  frame.mode = unset;
  frame.state = none;
  frame.transient = 0;
  frame.x = get_attributes.x;
  frame.y = get_attributes.y;
  frame.w = get_attributes.width;  
  frame.h = get_attributes.height;
  
  //TODO  get_frame_wm_hints(display, &frame); 
  
  frame.menu_item.backing = XCreateSimpleWindow(display, frames->title_menu, 10, 10
  , XWidthOfScreen(screen), MENU_ITEM_HEIGHT, 0, black, black);
  XMapWindow(display, frame.menu_item.backing);  
  
  get_frame_hints(display, &frame);
  create_frame_subwindows(display, &frame, pixmaps);
  create_frame_name(display, &frame);
  get_frame_type (display, &frame, atoms);
  get_frame_state(display, &frame, atoms);  

  resize_frame(display, &frame); //resize the title menu if it isn't at it's minimum 
  XMoveResizeWindow(display, frame.window, 0, 0, frame.w - FRAME_HSPACE, frame.h - FRAME_VSPACE);
  XSetWindowBorderWidth(display, frame.window, 0);
  XReparentWindow(display, frame.window, frame.backing, 0, 0);
  XSync(display, False);  //this prevents the Reparent unmap being reported.

  XSelectInput(display, frame.window,  StructureNotifyMask | PropertyChangeMask);
  //Property notify is used to update titles, structureNotify for destroyNotify events
  XSelectInput(display, frame.backing, SubstructureRedirectMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);

  XDefineCursor(display, frame.frame, cursors->normal);
  XDefineCursor(display, frame.l_grip, cursors->resize_h);
  XDefineCursor(display, frame.bl_grip, cursors->resize_tr_bl);
  XDefineCursor(display, frame.b_grip, cursors->resize_v);
  XDefineCursor(display, frame.br_grip, cursors->resize_tl_br);
  XDefineCursor(display, frame.r_grip, cursors->resize_h);
  
  //Intercept clicks so we can set the focus and possibly raise floating windows
  XGrabButton(display, Button1, 0, frame.backing, False, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
  
  frames->list[frames->used] = frame;
  frames->used++;
  return (frames->used - 1);
} 

void create_frame_subwindows (Display *display, struct Frame *frame, struct Pixmaps *pixmaps) {
  Window root = DefaultRootWindow(display);
  Screen* screen = DefaultScreenOfDisplay(display);
  int black = BlackPixelOfScreen(screen);
  
  frame->frame =         XCreateSimpleWindow(display, root
  , frame->x, frame->y
  , frame->w, frame->h, 0, black, black); 
  
  frame->body =          XCreateSimpleWindow(display
  , frame->frame, EDGE_WIDTH, TITLEBAR_HEIGHT
  , frame->w - EDGE_WIDTH*2, frame->h - (TITLEBAR_HEIGHT + EDGE_WIDTH) , 0, black, black);

  //same y as body, with a constant width as the sides (so H_SPACING)
  frame->innerframe =    XCreateSimpleWindow(display
  , frame->frame, EDGE_WIDTH + H_SPACING, TITLEBAR_HEIGHT
  , frame->w - (EDGE_WIDTH + H_SPACING)*2, frame->h - (TITLEBAR_HEIGHT + EDGE_WIDTH + H_SPACING), 0, black, black); 
                                                                           
  frame->titlebar =      XCreateSimpleWindow(display, frame->frame
  , EDGE_WIDTH, EDGE_WIDTH
  , frame->w - EDGE_WIDTH*2, TITLEBAR_HEIGHT, 0, black, black);
  
  frame->close_button.backing =  XCreateSimpleWindow(display, frame->titlebar
  , frame->w - H_SPACING - BUTTON_SIZE - EDGE_WIDTH*2, V_SPACING
  , BUTTON_SIZE, BUTTON_SIZE, 0, black, black);

  frame->close_button.close_button_normal =  XCreateSimpleWindow(display, frame->close_button.backing
  , 0,0
  , BUTTON_SIZE, BUTTON_SIZE, 0, black, black);
  frame->close_button.close_button_pressed =  XCreateSimpleWindow(display, frame->close_button.backing
  , 0,0
  , BUTTON_SIZE, BUTTON_SIZE, 0, black, black);
                                      
  frame->mode_dropdown.backing = XCreateSimpleWindow(display, frame->titlebar
  , frame->w - H_SPACING*2 - PULLDOWN_WIDTH - BUTTON_SIZE - EDGE_WIDTH*2, V_SPACING
  , PULLDOWN_WIDTH, BUTTON_SIZE, 0, black, black);

  //add mode dropdown states here
  frame->mode_dropdown.floating_normal = XCreateSimpleWindow(display, frame->mode_dropdown.backing
  , 0,0
  , PULLDOWN_WIDTH, BUTTON_SIZE, 0, black, black);
  frame->mode_dropdown.floating_pressed = XCreateSimpleWindow(display, frame->mode_dropdown.backing
  , 0,0
  , PULLDOWN_WIDTH, BUTTON_SIZE, 0, black, black);
  frame->mode_dropdown.floating_deactivated = XCreateSimpleWindow(display, frame->mode_dropdown.backing
  , 0,0
  , PULLDOWN_WIDTH, BUTTON_SIZE, 0, black, black);

  frame->mode_dropdown.tiling_normal = XCreateSimpleWindow(display, frame->mode_dropdown.backing
  , 0,0
  , PULLDOWN_WIDTH, BUTTON_SIZE, 0, black, black);
  frame->mode_dropdown.tiling_pressed = XCreateSimpleWindow(display, frame->mode_dropdown.backing
  , 0,0
  , PULLDOWN_WIDTH, BUTTON_SIZE, 0, black, black);
  frame->mode_dropdown.tiling_deactivated = XCreateSimpleWindow(display, frame->mode_dropdown.backing
  , 0,0
  , PULLDOWN_WIDTH, BUTTON_SIZE, 0, black, black);

  frame->mode_dropdown.desktop_normal = XCreateSimpleWindow(display, frame->mode_dropdown.backing
  , 0,0
  , PULLDOWN_WIDTH, BUTTON_SIZE, 0, black, black);
  frame->mode_dropdown.desktop_pressed = XCreateSimpleWindow(display, frame->mode_dropdown.backing
  , 0,0
  , PULLDOWN_WIDTH, BUTTON_SIZE, 0, black, black);
  frame->mode_dropdown.desktop_deactivated = XCreateSimpleWindow(display, frame->mode_dropdown.backing
  , 0,0
  , PULLDOWN_WIDTH, BUTTON_SIZE, 0, black, black);

  frame->selection_indicator.backing = XCreateSimpleWindow(display, frame->titlebar, H_SPACING, V_SPACING
  , BUTTON_SIZE, BUTTON_SIZE,  0, black, black);

  //selection states
  frame->selection_indicator.indicator_active = XCreateSimpleWindow(display, frame->selection_indicator.backing
  , 0,0
  , BUTTON_SIZE, BUTTON_SIZE,  0, black, black);
  frame->selection_indicator.indicator_normal = XCreateSimpleWindow(display, frame->selection_indicator.backing
  , 0,0
  , BUTTON_SIZE, BUTTON_SIZE,  0, black, black);
  
  frame->title_menu.frame =  XCreateSimpleWindow(display, frame->titlebar, H_SPACING*2 + BUTTON_SIZE, V_SPACING
  , frame->w - TITLEBAR_USED_WIDTH, BUTTON_SIZE, 0, black, black);

  frame->title_menu.body = XCreateSimpleWindow(display, frame->title_menu.frame, EDGE_WIDTH, EDGE_WIDTH
  , frame->w - TITLEBAR_USED_WIDTH - EDGE_WIDTH*2, BUTTON_SIZE - EDGE_WIDTH*2, 0, black, black);
  
  frame->title_menu.backing = XCreateSimpleWindow(display, frame->title_menu.body, EDGE_WIDTH, EDGE_WIDTH
  , frame->w - TITLE_MAX_WIDTH_DIFF, TITLE_MAX_HEIGHT, 0, black, black);   

  frame->title_menu.arrow.backing =  XCreateSimpleWindow(display, frame->title_menu.body
  , frame->w - TITLEBAR_USED_WIDTH - EDGE_WIDTH*2 - BUTTON_SIZE, EDGE_WIDTH
  , BUTTON_SIZE - EDGE_WIDTH , BUTTON_SIZE - EDGE_WIDTH*4, 0, black,  black);

  //arrow states
  frame->title_menu.arrow.arrow_normal =  XCreateSimpleWindow(display, frame->title_menu.arrow.backing
  , 0,0
  , BUTTON_SIZE - EDGE_WIDTH , BUTTON_SIZE - EDGE_WIDTH*4, 0, black,  black);
  frame->title_menu.arrow.arrow_pressed =  XCreateSimpleWindow(display, frame->title_menu.arrow.backing
  , 0,0
  , BUTTON_SIZE - EDGE_WIDTH , BUTTON_SIZE - EDGE_WIDTH*4, 0, black,  black);
  frame->title_menu.arrow.arrow_deactivated =  XCreateSimpleWindow(display, frame->title_menu.arrow.backing
  , 0,0
  , BUTTON_SIZE - EDGE_WIDTH , BUTTON_SIZE - EDGE_WIDTH*4, 0, black,  black);
   
  //doesn't matter if the width of the grips is a bit bigger as it will be under the frame_window anyway
  frame->l_grip = XCreateWindow(display, frame->frame, 0, TITLEBAR_HEIGHT + EDGE_WIDTH*2
  , CORNER_GRIP_SIZE, frame->h - TITLEBAR_HEIGHT - CORNER_GRIP_SIZE - EDGE_WIDTH*2, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
                                      
  frame->bl_grip = XCreateWindow(display, frame->frame, 0, frame->h - CORNER_GRIP_SIZE
  , CORNER_GRIP_SIZE, CORNER_GRIP_SIZE, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);

  frame->b_grip = XCreateWindow(display, frame->frame, CORNER_GRIP_SIZE, frame->h - CORNER_GRIP_SIZE
  , frame->w - CORNER_GRIP_SIZE*2, CORNER_GRIP_SIZE, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);

  frame->br_grip = XCreateWindow(display, frame->frame, frame->w - CORNER_GRIP_SIZE, frame->h - CORNER_GRIP_SIZE
  , CORNER_GRIP_SIZE, CORNER_GRIP_SIZE, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);
                                      
  frame->r_grip = XCreateWindow(display, frame->frame, frame->w - CORNER_GRIP_SIZE, TITLEBAR_HEIGHT + EDGE_WIDTH*2 
  , CORNER_GRIP_SIZE, frame->h - TITLEBAR_HEIGHT - CORNER_GRIP_SIZE - EDGE_WIDTH*2, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);  

  //same y as body, with a constant width as the sides (so H_SPACING)
  frame->backing = XCreateSimpleWindow(display, frame->frame, EDGE_WIDTH*2 + H_SPACING, TITLEBAR_HEIGHT + EDGE_WIDTH*2
  , frame->w - FRAME_HSPACE, frame->h - FRAME_VSPACE, 0, black, black);

  frame->title_menu.hotspot =   XCreateWindow(display, frame->frame
  , H_SPACING*2 + BUTTON_SIZE + EDGE_WIDTH, 0
  , frame->w - TITLEBAR_USED_WIDTH, BUTTON_SIZE + V_SPACING + EDGE_WIDTH*2, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);

  frame->mode_dropdown.mode_hotspot = XCreateWindow(display, frame->frame
  , frame->w - H_SPACING*2 - PULLDOWN_WIDTH - BUTTON_SIZE - EDGE_WIDTH*2, 0
  , PULLDOWN_WIDTH, BUTTON_SIZE + V_SPACING + EDGE_WIDTH*2, 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);

  frame->close_button.close_hotspot = XCreateWindow(display, frame->frame
  , frame->w - H_SPACING - BUTTON_SIZE - EDGE_WIDTH*2, 0
  , BUTTON_SIZE + H_SPACING + EDGE_WIDTH*2, BUTTON_SIZE + V_SPACING + EDGE_WIDTH*2
  , 0, CopyFromParent, InputOnly, CopyFromParent, 0, NULL);

  XSetWindowBackgroundPixmap(display, frame->frame, pixmaps->border_p );
  XSetWindowBackgroundPixmap(display, frame->title_menu.frame, pixmaps->border_p );
  XSetWindowBackgroundPixmap(display, frame->innerframe, pixmaps->border_p );
  XSetWindowBackgroundPixmap(display, frame->body, pixmaps->body_p );
  XSetWindowBackgroundPixmap(display, frame->title_menu.body, pixmaps->light_border_p );
  XSetWindowBackgroundPixmap(display, frame->titlebar,  pixmaps->titlebar_background_p );    

  XSetWindowBackgroundPixmap(display, frame->title_menu.arrow.arrow_normal, pixmaps->arrow_normal_p);
  XSetWindowBackgroundPixmap(display, frame->title_menu.arrow.arrow_pressed, pixmaps->arrow_pressed_p);
  XSetWindowBackgroundPixmap(display, frame->title_menu.arrow.arrow_deactivated, pixmaps->arrow_deactivated_p);

  XSetWindowBackgroundPixmap(display, frame->selection_indicator.indicator_active, pixmaps->selection_indicator_active_p);
  XSetWindowBackgroundPixmap(display, frame->selection_indicator.indicator_normal, pixmaps->selection_indicator_normal_p);

  XSetWindowBackgroundPixmap(display, frame->mode_dropdown.floating_normal, pixmaps->pulldown_floating_normal_p);
  XSetWindowBackgroundPixmap(display, frame->mode_dropdown.floating_pressed, pixmaps->pulldown_floating_pressed_p);
  XSetWindowBackgroundPixmap(display, frame->mode_dropdown.floating_deactivated, pixmaps->pulldown_floating_deactivated_p);

  XSetWindowBackgroundPixmap(display, frame->mode_dropdown.tiling_normal, pixmaps->pulldown_tiling_normal_p);
  XSetWindowBackgroundPixmap(display, frame->mode_dropdown.tiling_pressed, pixmaps->pulldown_tiling_pressed_p);
  XSetWindowBackgroundPixmap(display, frame->mode_dropdown.tiling_deactivated, pixmaps->pulldown_tiling_deactivated_p);

  XSetWindowBackgroundPixmap(display, frame->mode_dropdown.desktop_normal, pixmaps->pulldown_desktop_normal_p);
  XSetWindowBackgroundPixmap(display, frame->mode_dropdown.desktop_pressed, pixmaps->pulldown_desktop_pressed_p);
  XSetWindowBackgroundPixmap(display, frame->mode_dropdown.desktop_deactivated, pixmaps->pulldown_desktop_deactivated_p);
  
  XSetWindowBackgroundPixmap(display, frame->close_button.close_button_normal, pixmaps->close_button_normal_p);
  XSetWindowBackgroundPixmap(display, frame->close_button.close_button_pressed, pixmaps->close_button_pressed_p);

  //XMapWindow(display, frame.frame); //This window is now mapped in add_frame_to_workspace if required.
  XMapWindow(display, frame->window);
  XMapWindow(display, frame->backing);
  XMapWindow(display, frame->body);
  XMapWindow(display, frame->innerframe);
  XMapWindow(display, frame->titlebar);

  XMapWindow(display, frame->l_grip);
  XMapWindow(display, frame->bl_grip);
  XMapWindow(display, frame->b_grip);
  XMapWindow(display, frame->br_grip);
  XMapWindow(display, frame->r_grip);

  XMapWindow(display, frame->selection_indicator.backing);
  XMapWindow(display, frame->title_menu.frame);
  XMapWindow(display, frame->title_menu.body);
  XMapWindow(display, frame->title_menu.hotspot);
  XMapWindow(display, frame->title_menu.arrow.backing);
  XMapWindow(display, frame->title_menu.arrow.arrow_normal);
  XMapWindow(display, frame->title_menu.arrow.arrow_pressed);
  XMapWindow(display, frame->title_menu.arrow.arrow_deactivated);
    
  XMapWindow(display, frame->selection_indicator.indicator_active);
  XMapWindow(display, frame->selection_indicator.indicator_normal);
  XMapWindow(display, frame->title_menu.backing);

  XMapWindow(display, frame->mode_dropdown.backing);
  XMapWindow(display, frame->mode_dropdown.floating_normal);
  XMapWindow(display, frame->mode_dropdown.floating_pressed);
  XMapWindow(display, frame->mode_dropdown.floating_deactivated);
  XMapWindow(display, frame->mode_dropdown.tiling_normal);
  XMapWindow(display, frame->mode_dropdown.tiling_pressed);
  XMapWindow(display, frame->mode_dropdown.tiling_deactivated);
  XMapWindow(display, frame->mode_dropdown.desktop_normal);
  XMapWindow(display, frame->mode_dropdown.desktop_pressed);
  XMapWindow(display, frame->mode_dropdown.desktop_deactivated);
  XMapWindow(display, frame->mode_dropdown.mode_hotspot);
  
  XMapWindow(display, frame->close_button.backing);
  XMapWindow(display, frame->close_button.close_button_normal);
  XMapWindow(display, frame->close_button.close_button_pressed);
  XMapWindow(display, frame->close_button.close_hotspot);

  //XMapWindow(display, frame.menu_item.icon.item_icon);
  //XMapWindow(display, frame.menu_item.icon.item_icon_hover);

  XSelectInput(display, frame->frame,   Button1MotionMask | ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame->close_button.close_hotspot,  ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
  XSelectInput(display, frame->mode_dropdown.mode_hotspot, ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame->title_menu.hotspot, ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame->menu_item.backing, ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
  XSelectInput(display, frame->l_grip,  ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame->bl_grip, ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame->b_grip,  ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame->br_grip, ButtonPressMask | ButtonReleaseMask);
  XSelectInput(display, frame->r_grip,  ButtonPressMask | ButtonReleaseMask);
}

/*** Moves and resizes the subwindows of the frame ***/
void resize_frame(Display* display, struct Frame* frame) {

  //printf("resize frame, x %d, y %d, w %d, h %d\n", frame->x, frame->y, frame->w, frame->h);

  if(frame->mode == floating  
  || frame->mode == tiling  
  || frame->mode == desktop  
  || frame->mode == hidden) {
    XMoveResizeWindow(display, frame->frame, frame->x, frame->y,  frame->w, frame->h);
    //XResizeWindow(display, frame->titlebar, frame->w - EDGE_WIDTH*2, TITLEBAR_HEIGHT);
    XMoveResizeWindow(display, frame->titlebar, EDGE_WIDTH, EDGE_WIDTH, frame->w - EDGE_WIDTH*2, TITLEBAR_HEIGHT);
    XMoveWindow(display, frame->close_button.backing, frame->w - H_SPACING - BUTTON_SIZE - EDGE_WIDTH, V_SPACING);
    XMoveWindow(display, frame->close_button.close_hotspot, frame->w - H_SPACING - BUTTON_SIZE - EDGE_WIDTH , 0);
    XMoveWindow(display, frame->mode_dropdown.backing, frame->w - H_SPACING*2 - PULLDOWN_WIDTH - BUTTON_SIZE - EDGE_WIDTH, V_SPACING);
    XMoveWindow(display, frame->mode_dropdown.mode_hotspot, frame->w - H_SPACING*2 - PULLDOWN_WIDTH - BUTTON_SIZE - EDGE_WIDTH, 0);

    if(frame->title_menu.width + EDGE_WIDTH*4 + BUTTON_SIZE < frame->w - TITLEBAR_USED_WIDTH) {
      XResizeWindow(display, frame->title_menu.frame,   frame->title_menu.width + EDGE_WIDTH*4 + BUTTON_SIZE, BUTTON_SIZE);
      XMoveResizeWindow(display, frame->title_menu.hotspot, H_SPACING*2 + BUTTON_SIZE + EDGE_WIDTH, 0, frame->title_menu.width + EDGE_WIDTH*4 + BUTTON_SIZE, BUTTON_SIZE + V_SPACING + EDGE_WIDTH);
      //XResizeWindow(display, frame->title_menu.hotspot, frame->title_menu.width + EDGE_WIDTH*4 + BUTTON_SIZE, BUTTON_SIZE + V_SPACING + EDGE_WIDTH);
      XResizeWindow(display, frame->title_menu.body,    frame->title_menu.width + EDGE_WIDTH*2 + BUTTON_SIZE, BUTTON_SIZE - EDGE_WIDTH*2);
      //XResizeWindow(display, frame->title_menu.backing, frame->title_menu.width + EDGE_WIDTH, TITLE_MAX_HEIGHT);
      XMoveResizeWindow(display, frame->title_menu.backing
      , EDGE_WIDTH, EDGE_WIDTH
      , frame->w - TITLE_MAX_WIDTH_DIFF, TITLE_MAX_HEIGHT);
      XMoveWindow(display,   frame->title_menu.arrow.backing,   frame->title_menu.width + EDGE_WIDTH*2, EDGE_WIDTH);
    }
    else {
      XResizeWindow(display, frame->title_menu.frame,   frame->w - TITLEBAR_USED_WIDTH, BUTTON_SIZE);
      XResizeWindow(display, frame->title_menu.hotspot, frame->w - TITLEBAR_USED_WIDTH, BUTTON_SIZE + V_SPACING + EDGE_WIDTH);
      XResizeWindow(display, frame->title_menu.body,    frame->w - TITLEBAR_USED_WIDTH - EDGE_WIDTH*2, BUTTON_SIZE - EDGE_WIDTH*2);
      XResizeWindow(display, frame->title_menu.backing, frame->w - TITLE_MAX_WIDTH_DIFF, TITLE_MAX_HEIGHT);
      XMoveWindow(display,   frame->title_menu.arrow.backing,   frame->w - TITLEBAR_USED_WIDTH - EDGE_WIDTH*2 - BUTTON_SIZE, EDGE_WIDTH);
    }
    
    XResizeWindow(display, frame->body, frame->w - EDGE_WIDTH*2, frame->h - (TITLEBAR_HEIGHT + EDGE_WIDTH));
    XMoveResizeWindow(display, frame->innerframe
    , EDGE_WIDTH + H_SPACING, TITLEBAR_HEIGHT
    , frame->w - (EDGE_WIDTH + H_SPACING)*2, frame->h - (TITLEBAR_HEIGHT + EDGE_WIDTH + H_SPACING));
    
    XResizeWindow(display, frame->backing, frame->w - FRAME_HSPACE, frame->h - FRAME_VSPACE);
    XResizeWindow(display, frame->window, frame->w - FRAME_HSPACE, frame->h - FRAME_VSPACE);

    XMoveResizeWindow(display, frame->l_grip, 0, TITLEBAR_HEIGHT + EDGE_WIDTH*2, CORNER_GRIP_SIZE, frame->h - TITLEBAR_HEIGHT - CORNER_GRIP_SIZE - EDGE_WIDTH*2);
    XMoveResizeWindow(display, frame->bl_grip, 0, frame->h - CORNER_GRIP_SIZE, CORNER_GRIP_SIZE, CORNER_GRIP_SIZE);
    XMoveResizeWindow(display, frame->b_grip, CORNER_GRIP_SIZE, frame->h - CORNER_GRIP_SIZE, frame->w - CORNER_GRIP_SIZE*2, CORNER_GRIP_SIZE);
    XMoveResizeWindow(display, frame->br_grip, frame->w - CORNER_GRIP_SIZE, frame->h - CORNER_GRIP_SIZE, CORNER_GRIP_SIZE, CORNER_GRIP_SIZE);
    XMoveResizeWindow(display, frame->r_grip, frame->w - CORNER_GRIP_SIZE, TITLEBAR_HEIGHT + EDGE_WIDTH*2
    , CORNER_GRIP_SIZE, frame->h - TITLEBAR_HEIGHT - CORNER_GRIP_SIZE - EDGE_WIDTH*2);
    
    XMoveWindow(display, frame->window, 0,0);
    XFlush(display);
  }
  /*
  else if(frame->mode == minimal) {
    //need to add hide button.
    XMoveResizeWindow(display, frame->frame, frame->x, frame->y,  frame->w, frame->h);
    XMoveResizeWindow(display, frame->titlebar, EDGE_WIDTH, EDGE_WIDTH, frame->w - EDGE_WIDTH*2, MINIMAL_TITLEBAR_HEIGHT);

    XResizeWindow(display, frame->title_menu.arrow.backing, MINIMAL_BUTTON_SIZE, MINIMAL_BUTTON_SIZE);
    
    if(frame->title_menu.width + MINIMAL_BUTTON_SIZE < frame->w - MINIMAL_BUTTON_SIZE*2) {
      XMoveResizeWindow(display, frame->title_menu.frame,   0, 0, frame->title_menu.width + MINIMAL_BUTTON_SIZE, MINIMAL_BUTTON_SIZE);
      XMoveResizeWindow(display, frame->title_menu.hotspot, 0, 0, frame->title_menu.width + MINIMAL_BUTTON_SIZE, MINIMAL_BUTTON_SIZE);
      XMoveResizeWindow(display, frame->title_menu.body,    0, 0, frame->title_menu.width + MINIMAL_BUTTON_SIZE, MINIMAL_BUTTON_SIZE);
      XMoveResizeWindow(display, frame->title_menu.backing, 0, 0, frame->title_menu.width + MINIMAL_BUTTON_SIZE, MINIMAL_BUTTON_SIZE);
      XMoveWindow(display, frame->title_menu.arrow.backing,       frame->title_menu.width + MINIMAL_BUTTON_SIZE, 0);
    }
    else {
      XMoveResizeWindow(display, frame->title_menu.frame,   0, 0, frame->w - MINIMAL_BUTTON_SIZE*2, MINIMAL_BUTTON_SIZE);
      XMoveResizeWindow(display, frame->title_menu.hotspot, 0, 0, frame->w - MINIMAL_BUTTON_SIZE*2, MINIMAL_BUTTON_SIZE);
      XMoveResizeWindow(display, frame->title_menu.body,    0, 0, frame->w - MINIMAL_BUTTON_SIZE*2, MINIMAL_BUTTON_SIZE);
      XMoveResizeWindow(display, frame->title_menu.backing, 0, 0, frame->w - MINIMAL_BUTTON_SIZE*2, MINIMAL_BUTTON_SIZE);
      XMoveWindow(display,   frame->title_menu.arrow.backing,   frame->w - MINIMAL_BUTTON_SIZE, 0);
    }
    
    XMoveResizeWindow(display, frame->body,  EDGE_WIDTH, EDGE_WIDTH + MINIMAL_TITLEBAR_HEIGHT
    ,frame->w - EDGE_WIDTH*2, frame->h - (MINIMAL_TITLEBAR_HEIGHT + EDGE_WIDTH*2));

    XMoveResizeWindow(display, frame->innerframe
    , EDGE_WIDTH + MINIMAL_SPACING, EDGE_WIDTH + MINIMAL_TITLEBAR_HEIGHT
    , frame->w - (EDGE_WIDTH + MINIMAL_SPACING)*2, frame->h - (MINIMAL_TITLEBAR_HEIGHT + EDGE_WIDTH*2 + MINIMAL_SPACING));
    
    XMoveResizeWindow(display, frame->backing
    , EDGE_WIDTH*2 + MINIMAL_SPACING, MINIMAL_TITLEBAR_HEIGHT + EDGE_WIDTH*2
    , frame->w - (EDGE_WIDTH*2 + MINIMAL_SPACING)*2, frame->h - (MINIMAL_TITLEBAR_HEIGHT + EDGE_WIDTH*4 + MINIMAL_SPACING));

    XResizeWindow(display, frame->window
    , frame->w - (EDGE_WIDTH*2 + MINIMAL_SPACING)*2, frame->h - (MINIMAL_TITLEBAR_HEIGHT + EDGE_WIDTH*4 + MINIMAL_SPACING));

    XMoveResizeWindow(display, frame->l_grip
    , 0, MINIMAL_TITLEBAR_HEIGHT + EDGE_WIDTH*2
    , CORNER_GRIP_SIZE, frame->h - MINIMAL_TITLEBAR_HEIGHT - CORNER_GRIP_SIZE - EDGE_WIDTH*2);

    XMoveResizeWindow(display, frame->bl_grip
    , 0, frame->h - CORNER_GRIP_SIZE
    , CORNER_GRIP_SIZE, CORNER_GRIP_SIZE);

    XMoveResizeWindow(display, frame->b_grip
    ,  CORNER_GRIP_SIZE, frame->h - CORNER_GRIP_SIZE
    , frame->w - CORNER_GRIP_SIZE*2, CORNER_GRIP_SIZE);

    XMoveResizeWindow(display, frame->br_grip
    , frame->w - CORNER_GRIP_SIZE, frame->h - CORNER_GRIP_SIZE
    , CORNER_GRIP_SIZE, CORNER_GRIP_SIZE);

    XMoveResizeWindow(display, frame->r_grip
    , frame->w - CORNER_GRIP_SIZE, TITLEBAR_HEIGHT + EDGE_WIDTH*2
    , CORNER_GRIP_SIZE, frame->h - MINIMAL_TITLEBAR_HEIGHT - CORNER_GRIP_SIZE - EDGE_WIDTH*2);
    
    XMoveWindow(display, frame->window, 0,0);
    XFlush(display);

  } */
}

void free_frame_name(Display* display, struct Frame* frame) {
  if(frame->window_name != NULL) {
    XFree(frame->window_name);
    frame->window_name = NULL;
    XFlush(display);
  }
}

/*** create pixmaps with the specified name if it is available, otherwise use a default name
TODO check if the name is just whitespace  ***/
void create_frame_name(Display* display, struct Frame* frame) {
//  char untitled[10] = "noname";
  char untitled[10] = "   ";
  
  struct Frame temp = *frame; 
  //Only make changes if required and only unmap after pixmaps have been created.

  //these pixmaps include the bevel, background and  text
  Pixmap title_normal_p
  , title_pressed_p
  , title_deactivated_p;
  //these are the items for the menu
  Pixmap item_title_p
  , item_title_active_p
  , item_title_deactivated_p
  , item_title_hover_p
  , item_title_active_hover_p
  , item_title_deactivated_hover_p;

  Screen* screen = DefaultScreenOfDisplay(display);
  int black = BlackPixelOfScreen(screen);
    
  XFetchName(display, temp.window, &temp.window_name);

  if(temp.window_name == NULL 
  && frame->window_name != NULL
  && strcmp(frame->window_name, untitled) == 0) {
    //it was null and already has the name from untitled above.
    return;
  }
  else 
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
  
  title_normal_p             = create_title_pixmap(display, temp.window_name, title_normal);
  title_pressed_p            = create_title_pixmap(display, temp.window_name, title_pressed);
  title_deactivated_p        = create_title_pixmap(display, temp.window_name, title_deactivated);

  item_title_p               = create_title_pixmap(display, temp.window_name, item_title);
  item_title_active_p        = create_title_pixmap(display, temp.window_name, item_title_active);
  item_title_deactivated_p   = create_title_pixmap(display, temp.window_name, item_title_deactivated);
  item_title_hover_p         = create_title_pixmap(display, temp.window_name, item_title_hover);
  item_title_active_hover_p  = create_title_pixmap(display, temp.window_name, item_title_active_hover);
  item_title_deactivated_hover_p = create_title_pixmap(display, temp.window_name, item_title_deactivated_hover);

  //these are for the actual title_menu "button"
  temp.title_menu.title_normal  =   
  XCreateSimpleWindow(display, temp.title_menu.backing, 0, 0, XWidthOfScreen(screen), TITLE_MAX_HEIGHT, 0, black, black);   
  temp.title_menu.title_pressed =   
  XCreateSimpleWindow(display, temp.title_menu.backing, 0, 0, XWidthOfScreen(screen), TITLE_MAX_HEIGHT, 0, black, black); 
  temp.title_menu.title_deactivated = 
  XCreateSimpleWindow(display, temp.title_menu.backing, 0, 0, XWidthOfScreen(screen), TITLE_MAX_HEIGHT, 0, black, black); 
  //these are the items for inside the menu
  //need to create all these windows.
  temp.menu_item.item_title = 
  XCreateSimpleWindow(display, temp.menu_item.backing, 0, 0, XWidthOfScreen(screen), MENU_ITEM_HEIGHT, 0, black, black);
  temp.menu_item.item_title_active =
  XCreateSimpleWindow(display, temp.menu_item.backing, 0, 0, XWidthOfScreen(screen), MENU_ITEM_HEIGHT, 0, black, black);
  temp.menu_item.item_title_deactivated =
  XCreateSimpleWindow(display, temp.menu_item.backing, 0, 0, XWidthOfScreen(screen), MENU_ITEM_HEIGHT, 0, black, black);  
  temp.menu_item.item_title_hover = 
  XCreateSimpleWindow(display, temp.menu_item.backing, 0, 0, XWidthOfScreen(screen), MENU_ITEM_HEIGHT, 0, black, black);
  temp.menu_item.item_title_active_hover =
  XCreateSimpleWindow(display, temp.menu_item.backing, 0, 0, XWidthOfScreen(screen), MENU_ITEM_HEIGHT, 0, black, black);
  temp.menu_item.item_title_deactivated_hover =
  XCreateSimpleWindow(display, temp.menu_item.backing, 0, 0, XWidthOfScreen(screen), MENU_ITEM_HEIGHT, 0, black, black);
    
  //assign pixmaps
  XSetWindowBackgroundPixmap(display, temp.title_menu.title_normal, title_normal_p);
  XSetWindowBackgroundPixmap(display, temp.title_menu.title_pressed,  title_pressed_p);
  XSetWindowBackgroundPixmap(display, temp.title_menu.title_deactivated, title_deactivated_p);
  
  XSetWindowBackgroundPixmap(display, temp.menu_item.item_title, item_title_p);
  XSetWindowBackgroundPixmap(display, temp.menu_item.item_title_active, item_title_active_p);
  XSetWindowBackgroundPixmap(display, temp.menu_item.item_title_deactivated, item_title_deactivated_p);
  XSetWindowBackgroundPixmap(display, temp.menu_item.item_title_hover, item_title_hover_p);
  XSetWindowBackgroundPixmap(display, temp.menu_item.item_title_active_hover,  item_title_active_hover_p);
  XSetWindowBackgroundPixmap(display, temp.menu_item.item_title_deactivated_hover, item_title_deactivated_hover_p);

  XRaiseWindow(display, temp.title_menu.title_normal);
  XRaiseWindow(display, temp.menu_item.item_title);
  
  XMapWindow(display, temp.title_menu.title_normal);
  XMapWindow(display, temp.title_menu.title_pressed);
  XMapWindow(display, temp.title_menu.title_deactivated);
  
  XMapWindow(display, temp.menu_item.item_title);
  XMapWindow(display, temp.menu_item.item_title_active);
  XMapWindow(display, temp.menu_item.item_title_deactivated);
  XMapWindow(display, temp.menu_item.item_title_hover);
  XMapWindow(display, temp.menu_item.item_title_active_hover);
  XMapWindow(display, temp.menu_item.item_title_deactivated_hover);

  //TODO free pixmaps here.
  XFreePixmap(display, title_normal_p);
  XFreePixmap(display, title_pressed_p);
  XFreePixmap(display, title_deactivated_p);

  XFreePixmap(display, item_title_p);
  XFreePixmap(display, item_title_active_p);
  XFreePixmap(display, item_title_deactivated_p);
  XFreePixmap(display, item_title_hover_p);
  XFreePixmap(display, item_title_active_hover_p);
  XFreePixmap(display, item_title_deactivated_hover_p);
  //TODO map new titles 
  
  //destroy old title if it had one
  if(frame->window_name != NULL) {
    free_frame_name(display, frame);
    XDestroyWindow(display, frame->title_menu.title_pressed);
    XDestroyWindow(display, frame->title_menu.title_deactivated);
    XDestroyWindow(display, frame->title_menu.title_normal);
      
    XDestroyWindow(display, frame->menu_item.item_title);
    XDestroyWindow(display, frame->menu_item.item_title_active);
    XDestroyWindow(display, frame->menu_item.item_title_deactivated);
    XDestroyWindow(display, frame->menu_item.item_title_hover);
    XDestroyWindow(display, frame->menu_item.item_title_active_hover);
    XDestroyWindow(display, frame->menu_item.item_title_deactivated_hover);
  }
  XFlush(display);
  *frame = temp;
    
  frame->title_menu.width = get_title_width(display, frame->window_name);
  frame->menu_item.width = get_title_width(display, frame->window_name);
}

/*** Update frame with available resizing information ***/
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

/** Update frame with available wm hints (icon, window "group", focus wanted, urgency) **/
void get_frame_wm_hints(Display *display, struct Frame *frame) {
  XWMHints *wm_hints = XGetWMHints(display, frame->window);
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
      XShapeCombineMask (display, frame->menu_item.icon.item_icon, ShapeBounding /*ShapeClip or ShapeBounding */
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

void get_frame_type(Display *display, struct Frame *frame, struct Atoms *atoms) {
  unsigned char *contents = NULL;
  Atom return_type;
  int return_format;
  unsigned long items;
  unsigned long bytes;
  
  XGetTransientForHint(display, frame->window, &frame->transient);
  
  XGetWindowProperty(display, frame->window, atoms->wm_window_type, 0, 1 //long long_length?
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
        change_frame_mode(display, frame, desktop);
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
        change_frame_mode(display, frame, floating);
        //change_frame_mode(display, frame, minimal);
        frame->type = system_program; 
      }
      else if(window_type[i] == atoms->wm_window_type_splash) {
        #ifdef SHOW_PROPERTIES
        printf("type: splash\n");
        #endif
        change_frame_mode(display, frame, floating);        
        frame->type = splash;
      }
      else if(window_type[i] ==atoms->wm_window_type_dialog) {
        #ifdef SHOW_PROPERTIES
        printf("type: dialog\n");
        #endif
        change_frame_mode(display, frame, floating);        
        frame->type = dialog;
      }
      else if(window_type[i] == atoms->wm_window_type_utility) {
        #ifdef SHOW_PROPERTIES
        printf("type: utility\n");
        #endif
        change_frame_mode(display, frame, floating);
        //change_frame_mode(display, frame, minimal);        
        frame->type = utility;
      }
    }
  }
  if(frame->type == unknown) change_frame_mode(display, frame, floating);
  if(contents != NULL) XFree(contents);
}

void get_frame_state(Display *display, struct Frame *frame, struct Atoms *atoms) {
  unsigned char *contents = NULL;
  Atom return_type;
  int return_format;
  unsigned long items;
  unsigned long bytes;
  XGetWindowProperty(display, frame->window, atoms->wm_state, 0, 1
  , False, AnyPropertyType, &return_type, &return_format,  &items, &bytes, &contents);
  
  printf("loading state\n");
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

void free_frame_icon_size(Display *display, struct Frame *frame) {
  if(frame->icon_size != NULL) XFree(frame->icon_size);
  frame->icon_size = NULL;
}
*/
