Window create_menubar(Display *display, struct frame_pixmaps *pixmaps, struct mouse_cursors *cursors)  {
  
  Window root = DefaultRootWindow(display);
  Screen* screen = DefaultScreenOfDisplay(display);
  int black = BlackPixelOfScreen(screen);
  Window border, body;
    
  border = XCreateSimpleWindow(display, root, 0, XHeightOfScreen(screen) - MENUBAR_HEIGHT, XWidthOfScreen(screen), MENUBAR_HEIGHT, 0, black, black);
  body = XCreateSimpleWindow(display, border, 0, EDGE_WIDTH, XWidthOfScreen(screen), MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black);
  
  XSelectInput (display, border,  Button1MotionMask | ButtonPressMask | ButtonReleaseMask);
  XDefineCursor(display, border, cursors->normal);
  XDefineCursor(display, body, cursors->normal);  
  XSetWindowBackgroundPixmap(display, border, pixmaps->border_p );  
  XSetWindowBackgroundPixmap(display, body, pixmaps->titlebar_background_p );  
  XMapWindow   (display, body);
  XMapWindow   (display, border);  
  return border;
}
