void create_menubar(Display *display, struct Menubar *menubar, struct frame_pixmaps *pixmaps, struct mouse_cursors *cursors)  {
  
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
