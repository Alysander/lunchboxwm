void create_menubar(Display *display, struct Menubar *menubar, struct Pixmaps *pixmaps, struct Cursors *cursors) {
  Window root = DefaultRootWindow(display);
  Screen* screen = DefaultScreenOfDisplay(display);
  int black = BlackPixelOfScreen(screen);
  unsigned int spacing = (XWidthOfScreen(screen) - MENUBAR_ITEM_WIDTH)/ 4;
  
  menubar->border = XCreateSimpleWindow(display, root
  , 0, XHeightOfScreen(screen) - MENUBAR_HEIGHT, XWidthOfScreen(screen), MENUBAR_HEIGHT, 0, black, black);
  menubar->body = XCreateSimpleWindow(display, menubar->border
  , 0, EDGE_WIDTH, XWidthOfScreen(screen), MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black);

  menubar->program_menu = XCreateSimpleWindow(display, menubar->border
  , spacing*0, EDGE_WIDTH, MENUBAR_ITEM_WIDTH, MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black);
  menubar->window_menu = XCreateSimpleWindow(display, menubar->border
  , spacing*1, EDGE_WIDTH, MENUBAR_ITEM_WIDTH, MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black);
  menubar->options_menu = XCreateSimpleWindow(display, menubar->border
  , spacing*2, EDGE_WIDTH, MENUBAR_ITEM_WIDTH, MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black); 
  menubar->links_menu = XCreateSimpleWindow(display, menubar->border
  , spacing*3, EDGE_WIDTH, MENUBAR_ITEM_WIDTH, MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black);
  menubar->tool_menu = XCreateSimpleWindow(display, menubar->border
  , spacing*4, EDGE_WIDTH, MENUBAR_ITEM_WIDTH, MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black);

  XSelectInput (display, menubar->program_menu,  Button1MotionMask | ButtonPressMask | ButtonReleaseMask);
  XSelectInput (display, menubar->window_menu,  Button1MotionMask | ButtonPressMask | ButtonReleaseMask);
  XDefineCursor(display, menubar->border, cursors->normal);
  XDefineCursor(display, menubar->body, cursors->normal);  

  XSetWindowBackgroundPixmap(display, menubar->border, pixmaps->border_p );  
  XSetWindowBackgroundPixmap(display, menubar->body, pixmaps->titlebar_background_p );
  XSetWindowBackgroundPixmap(display, menubar->program_menu, pixmaps->program_menu_normal_p );
  XSetWindowBackgroundPixmap(display, menubar->window_menu, pixmaps->window_menu_normal_p );
  XSetWindowBackgroundPixmap(display, menubar->options_menu, pixmaps->options_menu_normal_p );
  XSetWindowBackgroundPixmap(display, menubar->links_menu, pixmaps->links_menu_normal_p );
  XSetWindowBackgroundPixmap(display, menubar->tool_menu, pixmaps->tool_menu_normal_p );
  
  XMapWindow(display, menubar->body);
  XMapWindow(display, menubar->border);
  XMapWindow(display, menubar->program_menu);
  XMapWindow(display, menubar->window_menu);  
  XMapWindow(display, menubar->options_menu);  
  XMapWindow(display, menubar->links_menu);  
  XMapWindow(display, menubar->tool_menu);  
}

void create_mode_menu(Display *display, struct Mode_menu *mode_menu
, struct Pixmaps *pixmaps, struct Cursors *cursors) {
 
  Window root = DefaultRootWindow(display);
  Screen* screen =  DefaultScreenOfDisplay(display);  
  int black = BlackPixelOfScreen(screen);

  mode_menu->frame = XCreateSimpleWindow(display, root, 0, 0
  , PULLDOWN_WIDTH + EDGE_WIDTH*2, MENU_ITEM_HEIGHT * 3 + EDGE_WIDTH*2, 0, black, black);
         
  mode_menu->floating = XCreateSimpleWindow(display, mode_menu->frame, EDGE_WIDTH, EDGE_WIDTH
  , PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
         
  mode_menu->tiling = XCreateSimpleWindow(display, mode_menu->frame, EDGE_WIDTH, MENU_ITEM_HEIGHT + EDGE_WIDTH
  , PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);

  mode_menu->sinking = XCreateSimpleWindow(display, mode_menu->frame, EDGE_WIDTH, MENU_ITEM_HEIGHT*2 + EDGE_WIDTH
  , PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);

  XSetWindowBackgroundPixmap(display, mode_menu->frame, pixmaps->border_p );
  XSetWindowBackgroundPixmap(display, mode_menu->floating, pixmaps->item_floating_p );
  XSetWindowBackgroundPixmap(display, mode_menu->tiling, pixmaps->item_tiling_p );
  XSetWindowBackgroundPixmap(display, mode_menu->sinking, pixmaps->item_sinking_p );
    
  XDefineCursor(display, mode_menu->frame, cursors->normal);
  XDefineCursor(display, mode_menu->floating, cursors->normal);
  XDefineCursor(display, mode_menu->tiling, cursors->normal);
  XDefineCursor(display, mode_menu->sinking, cursors->normal); 
  
  XSelectInput(display, mode_menu->floating
  , ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
  
  XSelectInput(display, mode_menu->tiling
  , ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
  
  XSelectInput(display, mode_menu->sinking
  , ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);

  XMapWindow(display, mode_menu->floating);
  XMapWindow(display, mode_menu->tiling);
  XMapWindow(display, mode_menu->sinking);
}

void create_workspaces_menu(Display *display, struct Workspace_list *workspaces, struct Pixmaps *pixmaps, struct Cursors *cursors) {
  Window root = DefaultRootWindow(display);
  Screen* screen =  DefaultScreenOfDisplay(display);  
  int black = BlackPixelOfScreen(screen);
  
  workspaces->workspace_menu = XCreateSimpleWindow(display, root, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  XSetWindowBackgroundPixmap (display, workspaces->workspace_menu, pixmaps->border_p );
  XDefineCursor(display, workspaces->workspace_menu, cursors->normal);
}

void create_title_menu(Display *display, struct Frame_list *frames, struct Pixmaps *pixmaps, struct Cursors *cursors) {
  Window root = DefaultRootWindow(display);
  Screen* screen =  DefaultScreenOfDisplay(display);  
  int black = BlackPixelOfScreen(screen);
  
  frames->title_menu = XCreateSimpleWindow(display, root, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  XSetWindowBackgroundPixmap (display, frames->title_menu, pixmaps->border_p );
  XDefineCursor(display, frames->title_menu, cursors->normal);
}

void show_workspace_menu(Display *display, Window calling_widget, struct Workspace_list* workspaces, int index, int x, int y) {
  int max_length = 100;
  int width;
  int height = MENU_ITEM_HEIGHT * workspaces->used + EDGE_WIDTH*2;

  for(int i = 0; i < workspaces->used; i++)  
  if(workspaces->list[i].workspace_menu.width > max_length) 
    max_length = workspaces->list[i].workspace_menu.width;

  //If this is the title menu show the title of the window that the menu appeared on in bold.
  for(int i = 0; i < workspaces->used; i++) {
    if(i == index)  XSetWindowBackgroundPixmap (display, workspaces->list[i].workspace_menu.entry, workspaces->list[i].workspace_menu.item_title_active_p );
    else XSetWindowBackgroundPixmap (display, workspaces->list[i].workspace_menu.entry, workspaces->list[i].workspace_menu.item_title_p );
  }
  
  //Make all the menu items the same width and height.
  for(int i = 0; i < workspaces->used; i++) {
    XMoveWindow(display, workspaces->list[i].workspace_menu.entry, EDGE_WIDTH, EDGE_WIDTH + MENU_ITEM_HEIGHT * i);
    XResizeWindow(display, workspaces->list[i].workspace_menu.entry, max_length, MENU_ITEM_HEIGHT);    
  }

  width = max_length + EDGE_WIDTH*2; //this is now the total width of the pop-up menu
  place_popup_menu(display, calling_widget, workspaces->workspace_menu, x, y, width, height);

}

/*This function pops-up the title menu in the titlebar or the window menu.
  the index parameter is the window which was clicked, so that it can be shown in bold.
  This function is also the window menu, which is done by setting the index to -1. */
void show_title_menu(Display *display, Window calling_widget, struct Frame_list* frames, int index, int x, int y) {
  int max_length = 100;
  int width;
  int height = MENU_ITEM_HEIGHT*frames->used + EDGE_WIDTH*2;
  
  for(int i = 0; i < frames->used; i++)  if(frames->list[i].title_menu.width > max_length) max_length = frames->list[i].title_menu.width;
    
  //If this is the Window menu, disable the tiling windows because they are already tiled.
  if(index == -1) for(int i = 0; i < frames->used; i++) {
    if(frames->list[i].mode == TILING)  XSetWindowBackgroundPixmap (display, frames->list[i].title_menu.entry, frames->list[i].title_menu.item_title_deactivated_p);
    else XSetWindowBackgroundPixmap (display, frames->list[i].title_menu.entry, frames->list[i].title_menu.item_title_p );
  }
  //If this is the title menu show the title of the window that the menu appeared on in bold.
  else for(int i = 0; i < frames->used; i++) {
    if(i == index)  XSetWindowBackgroundPixmap (display, frames->list[i].title_menu.entry, frames->list[i].title_menu.item_title_active_p );
    else XSetWindowBackgroundPixmap (display, frames->list[i].title_menu.entry, frames->list[i].title_menu.item_title_p );
  }
  //Make all the menu items the same width and height.
  for(int i = 0; i < frames->used; i++) {
    XMoveWindow(display, frames->list[i].title_menu.entry, EDGE_WIDTH, EDGE_WIDTH + MENU_ITEM_HEIGHT * i);  
    XResizeWindow(display, frames->list[i].title_menu.entry, max_length, MENU_ITEM_HEIGHT);    
  }
  
  width = max_length + EDGE_WIDTH*2; //this is now the total width of the pop-up menu
  place_popup_menu(display, calling_widget, frames->title_menu, x, y, width, height);
}

void show_mode_menu(Display *display, Window calling_widget, struct Mode_menu *mode_menu, struct Frame *active_frame, struct Pixmaps *pixmaps, int x, int y) {

  //these amounts are from create_mode_menu.
  int width = PULLDOWN_WIDTH + EDGE_WIDTH*2;  
  int height = MENU_ITEM_HEIGHT * 3 + EDGE_WIDTH*2; 
  
  printf("showing mode pulldown\n");  
  if(active_frame->mode == FLOATING) 
    XSetWindowBackgroundPixmap(display, mode_menu->floating, pixmaps->item_floating_active_p);
  else  XSetWindowBackgroundPixmap(display, mode_menu->floating, pixmaps->item_floating_p);
  
  if(active_frame->mode == TILING)
    XSetWindowBackgroundPixmap(display, mode_menu->tiling,   pixmaps->item_tiling_active_p);
  else  XSetWindowBackgroundPixmap(display, mode_menu->tiling,   pixmaps->item_tiling_p);
  
  if(active_frame->mode == SINKING)  
    XSetWindowBackgroundPixmap(display, mode_menu->sinking,  pixmaps->item_sinking_active_p);
  else  XSetWindowBackgroundPixmap(display, mode_menu->sinking,  pixmaps->item_sinking_p);

  XFlush(display);

  place_popup_menu(display, calling_widget, mode_menu->frame, x, y, width, height);
}

void place_popup_menu(Display *display, Window calling_widget, Window popup_menu, int x, int y, int width, int height) {
  Screen* screen = DefaultScreenOfDisplay(display);
  //calling widget is currently not being used, but an XGetWindowAttributes could be used to get its size
  //and position and then ensure that the popup is not overlapping it.
  //however, since this code is currently simpler and a bit faster to use, I prefer it.
  if(x + width > XWidthOfScreen(screen)) x -= width;
  if(y + height > XHeightOfScreen(screen)) y -= height;
  if(x < 0) x = XWidthOfScreen(screen) - width;  
  if(y < 0) y = XHeightOfScreen(screen) - height;
  XMoveResizeWindow(display, popup_menu, x, y, width, height);
  XRaiseWindow(display, popup_menu);
  XMapWindow(display, popup_menu);
  XFlush(display);
}
