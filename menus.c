void create_menubar(Display *display, struct Menubar *menubar, struct Pixmaps *pixmaps, struct Cursors *cursors) {
  XSetWindowAttributes set_attributes;
  Window root = DefaultRootWindow(display);
  Screen* screen = DefaultScreenOfDisplay(display);
  int black = BlackPixelOfScreen(screen);
  unsigned int spacing = (XWidthOfScreen(screen) - MENUBAR_ITEM_WIDTH)/ 4;

  Pixmap program_menu_normal_p, window_menu_normal_p, options_menu_normal_p, links_menu_normal_p, tool_menu_normal_p
  ,program_menu_pressed_p, window_menu_pressed_p, options_menu_pressed_p, links_menu_pressed_p, tool_menu_pressed_p
  ,program_menu_deactivated_p, window_menu_deactivated_p, options_menu_deactivated_p, links_menu_deactivated_p, tool_menu_deactivated_p;

  menubar->border = XCreateSimpleWindow(display, root
  , 0, XHeightOfScreen(screen) - MENUBAR_HEIGHT, XWidthOfScreen(screen), MENUBAR_HEIGHT, 0, black, black);
  menubar->body = XCreateSimpleWindow(display, menubar->border
  , 0, EDGE_WIDTH, XWidthOfScreen(screen), MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black);

  XSetWindowBackgroundPixmap(display, menubar->border, pixmaps->border_p );  
  XSetWindowBackgroundPixmap(display, menubar->body, pixmaps->titlebar_background_p );

  XDefineCursor(display, menubar->border, cursors->normal);
  XDefineCursor(display, menubar->body, cursors->normal);

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

  menubar->program_menu_normal = XCreateSimpleWindow(display,  menubar->program_menu
  , 0, 0, MENUBAR_ITEM_WIDTH, MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black);
  menubar->program_menu_pressed = XCreateSimpleWindow(display, menubar->program_menu
  , 0, 0, MENUBAR_ITEM_WIDTH, MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black);
  menubar->window_menu_normal = XCreateSimpleWindow(display,   menubar->window_menu
  , 0, 0, MENUBAR_ITEM_WIDTH, MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black);
  menubar->window_menu_pressed = XCreateSimpleWindow(display,  menubar->window_menu
  , 0, 0, MENUBAR_ITEM_WIDTH, MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black);
  menubar->options_menu_normal = XCreateSimpleWindow(display,  menubar->options_menu
  , 0, 0, MENUBAR_ITEM_WIDTH, MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black); 
  menubar->options_menu_pressed = XCreateSimpleWindow(display, menubar->options_menu
  , 0, 0, MENUBAR_ITEM_WIDTH, MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black); 
  menubar->links_menu_normal = XCreateSimpleWindow(display,    menubar->links_menu
  , 0, 0, MENUBAR_ITEM_WIDTH, MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black);
  menubar->links_menu_pressed = XCreateSimpleWindow(display,   menubar->links_menu
  , 0, 0, MENUBAR_ITEM_WIDTH, MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black);
  menubar->tool_menu_normal = XCreateSimpleWindow(display,     menubar->tool_menu
  , 0, 0, MENUBAR_ITEM_WIDTH, MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black);
  menubar->tool_menu_pressed = XCreateSimpleWindow(display,    menubar->tool_menu
  , 0, 0, MENUBAR_ITEM_WIDTH, MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black);

  menubar->program_menu_deactivated = XCreateSimpleWindow(display,  menubar->program_menu
  , 0, 0, MENUBAR_ITEM_WIDTH, MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black);
  menubar->window_menu_deactivated = XCreateSimpleWindow(display,   menubar->window_menu
  , 0, 0, MENUBAR_ITEM_WIDTH, MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black);
  menubar->options_menu_deactivated = XCreateSimpleWindow(display,  menubar->options_menu
  , 0, 0, MENUBAR_ITEM_WIDTH, MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black); 
  menubar->links_menu_deactivated = XCreateSimpleWindow(display,    menubar->links_menu
  , 0, 0, MENUBAR_ITEM_WIDTH, MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black);
  menubar->tool_menu_deactivated = XCreateSimpleWindow(display,     menubar->tool_menu
  , 0, 0, MENUBAR_ITEM_WIDTH, MENUBAR_HEIGHT - EDGE_WIDTH, 0, black, black);


  //make a hotspot called program menu and window_menu
  XSelectInput (display, menubar->program_menu,  Button1MotionMask | ButtonPressMask | ButtonReleaseMask);
  XSelectInput (display, menubar->window_menu,  Button1MotionMask | ButtonPressMask | ButtonReleaseMask);

  program_menu_normal_p  = create_widget_pixmap(display, program_menu, normal);
  program_menu_pressed_p = create_widget_pixmap(display, program_menu, pressed);
  window_menu_normal_p   = create_widget_pixmap(display, window_menu, normal);
  window_menu_pressed_p  = create_widget_pixmap(display, window_menu, pressed);
  options_menu_normal_p  = create_widget_pixmap(display, options_menu, normal);
  options_menu_pressed_p = create_widget_pixmap(display, options_menu, pressed);
  links_menu_normal_p    = create_widget_pixmap(display, links_menu, normal);
  links_menu_pressed_p   = create_widget_pixmap(display, links_menu, pressed);
  tool_menu_normal_p     = create_widget_pixmap(display, tool_menu, normal);
  tool_menu_pressed_p    = create_widget_pixmap(display, tool_menu, pressed);

  program_menu_deactivated_p  = create_widget_pixmap(display, program_menu, deactivated);
  window_menu_deactivated_p   = create_widget_pixmap(display, window_menu, deactivated);
  options_menu_deactivated_p  = create_widget_pixmap(display, options_menu, deactivated);
  links_menu_deactivated_p    = create_widget_pixmap(display, links_menu, deactivated);
  tool_menu_deactivated_p     = create_widget_pixmap(display, tool_menu, deactivated);

  XSetWindowBackgroundPixmap(display, menubar->program_menu_normal,  program_menu_normal_p );
  XSetWindowBackgroundPixmap(display, menubar->program_menu_pressed, program_menu_pressed_p );
  XSetWindowBackgroundPixmap(display, menubar->window_menu_normal,   window_menu_normal_p );
  XSetWindowBackgroundPixmap(display, menubar->window_menu_pressed,  window_menu_pressed_p );
  XSetWindowBackgroundPixmap(display, menubar->options_menu_normal,  options_menu_normal_p );
  XSetWindowBackgroundPixmap(display, menubar->options_menu_pressed, options_menu_pressed_p );
  XSetWindowBackgroundPixmap(display, menubar->links_menu_normal,    links_menu_normal_p );
  XSetWindowBackgroundPixmap(display, menubar->links_menu_pressed,   links_menu_pressed_p );
  XSetWindowBackgroundPixmap(display, menubar->tool_menu_normal,     tool_menu_normal_p );
  XSetWindowBackgroundPixmap(display, menubar->tool_menu_pressed,    tool_menu_pressed_p );

  XSetWindowBackgroundPixmap(display, menubar->program_menu_deactivated,  program_menu_deactivated_p );
  XSetWindowBackgroundPixmap(display, menubar->window_menu_deactivated,   window_menu_deactivated_p );
  XSetWindowBackgroundPixmap(display, menubar->options_menu_deactivated,  options_menu_deactivated_p );
  XSetWindowBackgroundPixmap(display, menubar->links_menu_deactivated,    links_menu_deactivated_p );
  XSetWindowBackgroundPixmap(display, menubar->tool_menu_deactivated,     tool_menu_deactivated_p );

  set_attributes.override_redirect = True; 
  XChangeWindowAttributes(display, menubar->border, CWOverrideRedirect, &set_attributes);

  XMapWindow(display, menubar->body);
  XMapWindow(display, menubar->border);
  XMapWindow(display, menubar->program_menu);
  XMapWindow(display, menubar->window_menu);
  XMapWindow(display, menubar->options_menu);
  XMapWindow(display, menubar->links_menu);
  XMapWindow(display, menubar->tool_menu);

  XMapWindow(display, menubar->program_menu_normal);
  XMapWindow(display, menubar->program_menu_pressed);
  XMapWindow(display, menubar->window_menu_normal);
  XMapWindow(display, menubar->window_menu_pressed);
  XMapWindow(display, menubar->options_menu_normal);
  XMapWindow(display, menubar->options_menu_pressed);
  XMapWindow(display, menubar->links_menu_normal);
  XMapWindow(display, menubar->links_menu_pressed);
  XMapWindow(display, menubar->tool_menu_normal);
  XMapWindow(display, menubar->tool_menu_pressed);

  XMapWindow(display, menubar->program_menu_deactivated);
  XMapWindow(display, menubar->window_menu_deactivated);
  XMapWindow(display, menubar->options_menu_deactivated);
  XMapWindow(display, menubar->links_menu_deactivated);
  XMapWindow(display, menubar->tool_menu_deactivated);

  XRaiseWindow(display, menubar->program_menu_normal);
  XRaiseWindow(display, menubar->window_menu_normal);
  XRaiseWindow(display, menubar->options_menu_deactivated);
  XRaiseWindow(display, menubar->links_menu_deactivated);
  XRaiseWindow(display, menubar->tool_menu_deactivated);
      
  XFreePixmap(display, program_menu_normal_p);
  XFreePixmap(display, program_menu_pressed_p);
  XFreePixmap(display, window_menu_normal_p);
  XFreePixmap(display, window_menu_pressed_p);
  XFreePixmap(display, options_menu_normal_p);
  XFreePixmap(display, options_menu_pressed_p);
  XFreePixmap(display, links_menu_normal_p);
  XFreePixmap(display, links_menu_pressed_p);
  XFreePixmap(display, tool_menu_normal_p );
  XFreePixmap(display, tool_menu_pressed_p);

  XFreePixmap(display, program_menu_deactivated_p);
  XFreePixmap(display, window_menu_deactivated_p);
  XFreePixmap(display, options_menu_deactivated_p);
  XFreePixmap(display, links_menu_deactivated_p);
  XFreePixmap(display, tool_menu_deactivated_p ); 
  
  XFlush(display);
}

void create_mode_menu(Display *display, struct Mode_menu *mode_menu
, struct Pixmaps *pixmaps, struct Cursors *cursors) {
  XSetWindowAttributes set_attributes;
  Window root = DefaultRootWindow(display);
  Screen* screen =  DefaultScreenOfDisplay(display);  
  int black = BlackPixelOfScreen(screen);

  Pixmap   
  //these don't have a textured background
  item_floating_p, item_floating_hover_p, item_floating_active_p, item_floating_active_hover_p, item_floating_deactivated_p
  ,item_tiling_p,  item_tiling_hover_p,   item_tiling_active_p,   item_tiling_active_hover_p,   item_tiling_deactivated_p  
  ,item_desktop_p, item_desktop_hover_p,  item_desktop_active_p,  item_desktop_active_hover_p,  item_desktop_deactivated_p
  ,item_hidden_p,  item_hidden_hover_p,   item_hidden_active_p,   item_hidden_active_hover_p,   item_hidden_deactivated_p;
  
  mode_menu->frame = XCreateSimpleWindow(display, root, 0, 0
  , PULLDOWN_WIDTH + EDGE_WIDTH*2, MENU_ITEM_HEIGHT * 5 + EDGE_WIDTH*2, 0, black, black);

  XSetWindowBackgroundPixmap(display, mode_menu->frame,    pixmaps->border_p );
         
  mode_menu->floating = XCreateSimpleWindow(display, mode_menu->frame, EDGE_WIDTH, MENU_ITEM_HEIGHT*0 + EDGE_WIDTH
  , PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
         
  mode_menu->tiling = XCreateSimpleWindow(display, mode_menu->frame, EDGE_WIDTH, MENU_ITEM_HEIGHT*1 + EDGE_WIDTH
  , PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  
  mode_menu->desktop = XCreateSimpleWindow(display, mode_menu->frame, EDGE_WIDTH, MENU_ITEM_HEIGHT*2 + EDGE_WIDTH
  , PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);

  mode_menu->blank = XCreateSimpleWindow(display, mode_menu->frame, EDGE_WIDTH, MENU_ITEM_HEIGHT*3 + EDGE_WIDTH
  , PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);  
  
  mode_menu->hidden = XCreateSimpleWindow(display, mode_menu->frame, EDGE_WIDTH, MENU_ITEM_HEIGHT*4 + EDGE_WIDTH
  , PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  
  XDefineCursor(display, mode_menu->frame, cursors->normal);
  
  XSelectInput(display, mode_menu->floating
  , ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
  
  XSelectInput(display, mode_menu->tiling
  , ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);

  XSelectInput(display, mode_menu->desktop
  , ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);

    XSelectInput(display, mode_menu->hidden
  , ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
  
  set_attributes.override_redirect = True; 
  XChangeWindowAttributes(display, mode_menu->frame,    CWOverrideRedirect, &set_attributes);
  
  mode_menu->item_floating =               XCreateSimpleWindow(display, mode_menu->floating, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_floating_hover =         XCreateSimpleWindow(display, mode_menu->floating, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_floating_active =        XCreateSimpleWindow(display, mode_menu->floating, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_floating_active_hover  = XCreateSimpleWindow(display, mode_menu->floating, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_floating_deactivated =   XCreateSimpleWindow(display, mode_menu->floating, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  
  mode_menu->item_tiling =                 XCreateSimpleWindow(display, mode_menu->tiling, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_tiling_hover =           XCreateSimpleWindow(display, mode_menu->tiling, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_tiling_active =          XCreateSimpleWindow(display, mode_menu->tiling, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_tiling_active_hover =    XCreateSimpleWindow(display, mode_menu->tiling, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_tiling_deactivated =     XCreateSimpleWindow(display, mode_menu->tiling, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  
  mode_menu->item_desktop =                XCreateSimpleWindow(display, mode_menu->desktop, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_desktop_hover =          XCreateSimpleWindow(display, mode_menu->desktop, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_desktop_active =         XCreateSimpleWindow(display, mode_menu->desktop, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_desktop_active_hover =   XCreateSimpleWindow(display, mode_menu->desktop, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_desktop_deactivated =    XCreateSimpleWindow(display, mode_menu->desktop, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);

  mode_menu->item_hidden =                 XCreateSimpleWindow(display, mode_menu->hidden, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_hidden_hover =           XCreateSimpleWindow(display, mode_menu->hidden, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_hidden_active =          XCreateSimpleWindow(display, mode_menu->hidden, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_hidden_active_hover =    XCreateSimpleWindow(display, mode_menu->hidden, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_hidden_deactivated =     XCreateSimpleWindow(display, mode_menu->hidden, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);  
      
  item_floating_p              = create_widget_pixmap(display, item_floating, normal);
  item_floating_hover_p        = create_widget_pixmap(display, item_floating, hover);
  item_tiling_p                = create_widget_pixmap(display, item_tiling, normal);
  item_tiling_hover_p          = create_widget_pixmap(display, item_tiling, hover);
  item_desktop_p               = create_widget_pixmap(display, item_desktop, normal);
  item_desktop_hover_p         = create_widget_pixmap(display, item_desktop, hover);
  item_hidden_p                = create_widget_pixmap(display,  item_hidden, normal);
  item_hidden_hover_p          = create_widget_pixmap(display,  item_hidden, hover);
  
  item_floating_active_p       = create_widget_pixmap(display, item_floating, active);
  item_floating_active_hover_p = create_widget_pixmap(display, item_floating, active_hover);
  item_tiling_active_p         = create_widget_pixmap(display, item_tiling, active);
  item_tiling_active_hover_p   = create_widget_pixmap(display, item_tiling, active_hover);
  item_desktop_active_p        = create_widget_pixmap(display, item_desktop, active);
  item_desktop_active_hover_p  = create_widget_pixmap(display, item_desktop, active_hover);
  item_hidden_active_p         = create_widget_pixmap(display, item_hidden, active);
  item_hidden_active_hover_p   = create_widget_pixmap(display, item_hidden, active_hover);
  
  item_floating_deactivated_p  = create_widget_pixmap(display, item_floating, deactivated);
  item_tiling_deactivated_p    = create_widget_pixmap(display, item_tiling, deactivated);
  item_desktop_deactivated_p   = create_widget_pixmap(display, item_desktop, deactivated);
  item_hidden_deactivated_p    = create_widget_pixmap(display, item_hidden, deactivated);
  
  mode_menu->item_floating =               XCreateSimpleWindow(display, mode_menu->floating, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_floating_hover =         XCreateSimpleWindow(display, mode_menu->floating, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_floating_active =        XCreateSimpleWindow(display, mode_menu->floating, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_floating_active_hover =  XCreateSimpleWindow(display, mode_menu->floating, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_floating_deactivated =   XCreateSimpleWindow(display, mode_menu->floating, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  
  mode_menu->item_tiling =                 XCreateSimpleWindow(display, mode_menu->tiling, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_tiling_hover =           XCreateSimpleWindow(display, mode_menu->tiling, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_tiling_active =          XCreateSimpleWindow(display, mode_menu->tiling, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_tiling_active_hover =    XCreateSimpleWindow(display, mode_menu->tiling, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_tiling_deactivated =     XCreateSimpleWindow(display, mode_menu->tiling, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  
  mode_menu->item_desktop =                XCreateSimpleWindow(display, mode_menu->desktop, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_desktop_hover =          XCreateSimpleWindow(display, mode_menu->desktop, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_desktop_active =         XCreateSimpleWindow(display, mode_menu->desktop, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_desktop_active_hover =   XCreateSimpleWindow(display, mode_menu->desktop, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_desktop_deactivated =    XCreateSimpleWindow(display, mode_menu->desktop, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);

  mode_menu->item_hidden =                 XCreateSimpleWindow(display, mode_menu->hidden, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_hidden_hover =           XCreateSimpleWindow(display, mode_menu->hidden, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_hidden_active =          XCreateSimpleWindow(display, mode_menu->hidden, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_hidden_active_hover =    XCreateSimpleWindow(display, mode_menu->hidden, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  mode_menu->item_hidden_deactivated =     XCreateSimpleWindow(display, mode_menu->hidden, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);  

  XSetWindowBackgroundPixmap(display, mode_menu->blank, pixmaps->body_p);
  XMapWindow(display, mode_menu->blank);
  
  XSetWindowBackgroundPixmap(display, mode_menu->item_floating, item_floating_p);
  XSetWindowBackgroundPixmap(display, mode_menu->item_floating_hover, item_floating_hover_p);
  XSetWindowBackgroundPixmap(display, mode_menu->item_floating_active, item_floating_active_p);
  XSetWindowBackgroundPixmap(display, mode_menu->item_floating_active_hover, item_floating_active_hover_p);
  XSetWindowBackgroundPixmap(display, mode_menu->item_floating_deactivated, item_floating_deactivated_p);
  
  XSetWindowBackgroundPixmap(display, mode_menu->item_tiling, item_tiling_p);
  XSetWindowBackgroundPixmap(display, mode_menu->item_tiling_hover, item_tiling_hover_p);
  XSetWindowBackgroundPixmap(display, mode_menu->item_tiling_active, item_tiling_active_p);
  XSetWindowBackgroundPixmap(display, mode_menu->item_tiling_active_hover, item_tiling_active_hover_p);
  XSetWindowBackgroundPixmap(display, mode_menu->item_tiling_deactivated, item_tiling_deactivated_p);
  
  XSetWindowBackgroundPixmap(display, mode_menu->item_desktop , item_desktop_p);
  XSetWindowBackgroundPixmap(display, mode_menu->item_desktop_hover, item_desktop_hover_p);
  XSetWindowBackgroundPixmap(display, mode_menu->item_desktop_active , item_desktop_active_p);
  XSetWindowBackgroundPixmap(display, mode_menu->item_desktop_active_hover, item_desktop_active_hover_p);
  XSetWindowBackgroundPixmap(display, mode_menu->item_desktop_deactivated , item_desktop_deactivated_p);

  XSetWindowBackgroundPixmap(display, mode_menu->item_hidden, item_hidden_p);
  XSetWindowBackgroundPixmap(display, mode_menu->item_hidden_hover , item_hidden_hover_p);
  XSetWindowBackgroundPixmap(display, mode_menu->item_hidden_active, item_hidden_active_p);
  XSetWindowBackgroundPixmap(display, mode_menu->item_hidden_active_hover, item_hidden_active_hover_p);
  XSetWindowBackgroundPixmap(display, mode_menu->item_hidden_deactivated , item_hidden_deactivated_p);
  
  XRaiseWindow(display, mode_menu->item_floating_active);

  XMapWindow(display, mode_menu->floating);
  XMapWindow(display, mode_menu->tiling);
  XMapWindow(display, mode_menu->desktop);
  XMapWindow(display, mode_menu->hidden);
    
  XMapWindow(display, mode_menu->item_floating);
  XMapWindow(display, mode_menu->item_floating_hover);
  XMapWindow(display, mode_menu->item_floating_active);
  XMapWindow(display, mode_menu->item_floating_active_hover);
  XMapWindow(display, mode_menu->item_floating_deactivated);
  
  XMapWindow(display, mode_menu->item_tiling);
  XMapWindow(display, mode_menu->item_tiling_hover);
  XMapWindow(display, mode_menu->item_tiling_active);
  XMapWindow(display, mode_menu->item_tiling_active_hover);
  XMapWindow(display, mode_menu->item_tiling_deactivated);
  
  XMapWindow(display, mode_menu->item_desktop);
  XMapWindow(display, mode_menu->item_desktop_hover);
  XMapWindow(display, mode_menu->item_desktop_active);
  XMapWindow(display, mode_menu->item_desktop_active_hover);
  XMapWindow(display, mode_menu->item_desktop_deactivated);

  XMapWindow(display, mode_menu->item_hidden);
  XMapWindow(display, mode_menu->item_hidden_hover);
  XMapWindow(display, mode_menu->item_hidden_active);
  XMapWindow(display, mode_menu->item_hidden_active_hover);
  XMapWindow(display, mode_menu->item_hidden_deactivated);

  XFreePixmap(display, item_floating_p);
  XFreePixmap(display, item_floating_hover_p);
  XFreePixmap(display, item_tiling_p);
  XFreePixmap(display, item_tiling_hover_p);
  XFreePixmap(display, item_desktop_p);
  XFreePixmap(display, item_desktop_hover_p);
  XFreePixmap(display, item_hidden_p);
  XFreePixmap(display, item_hidden_hover_p);
  
  XFreePixmap(display, item_floating_active_p);
  XFreePixmap(display, item_floating_active_hover_p);
  XFreePixmap(display, item_tiling_active_p);
  XFreePixmap(display, item_tiling_active_hover_p);
  XFreePixmap(display, item_desktop_active_p);
  XFreePixmap(display, item_desktop_active_hover_p);
  XFreePixmap(display, item_hidden_active_p);
  XFreePixmap(display, item_hidden_active_hover_p);
  
  XFreePixmap(display, item_floating_deactivated_p);
  XFreePixmap(display, item_tiling_deactivated_p);
  XFreePixmap(display, item_desktop_deactivated_p);
  XFreePixmap(display, item_hidden_deactivated_p);
  
  XFlush(display);
}

void create_workspaces_menu(Display *display, struct Workspace_list *workspaces, struct Pixmaps *pixmaps, struct Cursors *cursors) {
  Window root = DefaultRootWindow(display);
  Screen* screen =  DefaultScreenOfDisplay(display);  
  int black = BlackPixelOfScreen(screen);
  XSetWindowAttributes set_attributes;

  set_attributes.override_redirect = True;     
  workspaces->workspace_menu = XCreateSimpleWindow(display, root, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  XSetWindowBackgroundPixmap (display, workspaces->workspace_menu, pixmaps->border_p );
  XDefineCursor(display, workspaces->workspace_menu, cursors->normal);
  XChangeWindowAttributes(display, workspaces->workspace_menu, CWOverrideRedirect, &set_attributes);
}

void create_title_menu(Display *display, struct Frame_list *frames, struct Pixmaps *pixmaps, struct Cursors *cursors) {
  Window root = DefaultRootWindow(display);
  Screen* screen =  DefaultScreenOfDisplay(display);  
  int black = BlackPixelOfScreen(screen);
  XSetWindowAttributes set_attributes;
  
  set_attributes.override_redirect = True; 
  frames->title_menu = XCreateSimpleWindow(display, root, 0, 0, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, 0, black, black);
  XSetWindowBackgroundPixmap (display, frames->title_menu, pixmaps->border_p );
  XDefineCursor(display, frames->title_menu, cursors->normal);
  XChangeWindowAttributes(display, frames->title_menu, CWOverrideRedirect, &set_attributes);
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
    if(i == index)  XRaiseWindow(display, workspaces->list[i].workspace_menu.item_title_active );
    else XRaiseWindow(display, workspaces->list[i].workspace_menu.item_title);
  }
  
  //Make all the menu items the same width and height.
  for(int i = 0; i < workspaces->used; i++) {
    XMoveWindow(display, workspaces->list[i].workspace_menu.backing, EDGE_WIDTH, EDGE_WIDTH + MENU_ITEM_HEIGHT * i);
    XResizeWindow(display, workspaces->list[i].workspace_menu.backing, max_length, MENU_ITEM_HEIGHT);    
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
    if(frames->list[i].mode == tiling) XRaiseWindow(display, frames->list[i].menu_item.item_title_deactivated);
    else XRaiseWindow(display, frames->list[i].menu_item.item_title);
  }
  //If this is the title menu show the title of the window that the menu appeared on in bold.
  else for(int i = 0; i < frames->used; i++) {
    if(i == index) XRaiseWindow(display, frames->list[i].menu_item.item_title_active);
    else XRaiseWindow(display, frames->list[i].menu_item.item_title);
  }
  //Make all the menu items the same width and height.
  for(int i = 0; i < frames->used; i++) {
    XMoveWindow(display, frames->list[i].menu_item.backing, EDGE_WIDTH, EDGE_WIDTH + MENU_ITEM_HEIGHT * i);  
    XResizeWindow(display, frames->list[i].menu_item.backing, max_length, MENU_ITEM_HEIGHT);    
  }
  
  width = max_length + EDGE_WIDTH*2; //this is now the total width of the pop-up menu
  place_popup_menu(display, calling_widget, frames->title_menu, x, y, width, height);
}

void show_mode_menu(Display *display, Window calling_widget, struct Mode_menu *mode_menu, struct Frame *active_frame, struct Pixmaps *pixmaps, int x, int y) {

  //these amounts are from create_mode_menu.
  int width = PULLDOWN_WIDTH + EDGE_WIDTH*2;  
  int height = MENU_ITEM_HEIGHT * 5 + EDGE_WIDTH*2; 
  
  printf("showing mode dropdown\n");  
  if(active_frame->mode == floating) 
    XRaiseWindow(display, mode_menu->item_floating_active);
  else XRaiseWindow(display, mode_menu->item_floating);
  
  if(active_frame->mode == tiling)
    XRaiseWindow(display, mode_menu->item_tiling_active);
  else  XRaiseWindow(display, mode_menu->item_tiling);
  
  if(active_frame->mode == desktop)  
    XRaiseWindow(display, mode_menu->item_desktop_active);
  else  XRaiseWindow(display, mode_menu->item_desktop);
  printf("showing mode dropdown..2 \n");  
  
  XRaiseWindow(display, mode_menu->item_hidden);
    
  XFlush(display);

  place_popup_menu(display, calling_widget, mode_menu->frame, x, y, width, height);
}

void place_popup_menu(Display *display, Window calling_widget, Window popup_menu, int x, int y, int width, int height) {
  Screen* screen = DefaultScreenOfDisplay(display);
  XWindowAttributes details;
  
  XGetWindowAttributes(display, calling_widget, &details);
  y += details.height;
  if(y + height > XHeightOfScreen(screen)) y = y - (details.height + height); //either side of the widget
  if(y < 0) y = XHeightOfScreen(screen) - height;  
  if(x + width > XWidthOfScreen(screen)) x = XWidthOfScreen(screen) - width;
  
  XMoveResizeWindow(display, popup_menu, x, y, width, height);
  XRaiseWindow(display, popup_menu);
  XMapWindow(display, popup_menu);
  XFlush(display);
  printf("placed popup\n");
}
