
/*** Pixmap creation function, these pixmaps are used for double buffering reusable elements of the GUI ***/
Pixmap create_pixmap(Display* display, enum main_pixmap type) {
  Window root = DefaultRootWindow(display); 
  int screen_number = DefaultScreen (display);
  Screen* screen = DefaultScreenOfDisplay(display);
  Visual *colours =  DefaultVisual(display, screen_number);
  Pixmap pixmap;
  cairo_surface_t *surface;
  cairo_t *cr;  
  
  switch(type) {
  case background:
    pixmap = XCreatePixmap(display, root, PIXMAP_SIZE, PIXMAP_SIZE, XDefaultDepth(display, screen_number));    
    surface = cairo_xlib_surface_create(display, pixmap, colours, PIXMAP_SIZE, PIXMAP_SIZE);
    cr = cairo_create(surface);
    cairo_set_source_rgba(cr, BACKGROUND);
    cairo_paint(cr);    
  break;
  case body:
    pixmap = XCreatePixmap(display, root, PIXMAP_SIZE, PIXMAP_SIZE, XDefaultDepth(display, screen_number));    
    surface = cairo_xlib_surface_create(display, pixmap, colours, PIXMAP_SIZE, PIXMAP_SIZE);
    cr = cairo_create(surface);
    cairo_set_source_rgba(cr, BODY);
    cairo_paint(cr);  
  break;
  case border:
    pixmap = XCreatePixmap(display, root, PIXMAP_SIZE, PIXMAP_SIZE, XDefaultDepth(display, screen_number));    
    surface = cairo_xlib_surface_create(display, pixmap, colours, PIXMAP_SIZE, PIXMAP_SIZE);
    cr = cairo_create(surface);
    cairo_set_source_rgba(cr, BORDER);
    cairo_paint(cr);  
  break;
  case light_border:
    pixmap = XCreatePixmap(display, root, PIXMAP_SIZE, PIXMAP_SIZE, XDefaultDepth(display, screen_number));    
    surface = cairo_xlib_surface_create(display, pixmap, colours, PIXMAP_SIZE, PIXMAP_SIZE);
    cr = cairo_create(surface);
    cairo_set_source_rgba(cr, LIGHT_EDGE);
    cairo_paint(cr);  
  break;
  case titlebar: //this is just the titlebar background
    pixmap = XCreatePixmap(display, root, PIXMAP_SIZE, TITLEBAR_HEIGHT, XDefaultDepth(display, screen_number));
    surface = cairo_xlib_surface_create(display, pixmap, colours, PIXMAP_SIZE, TITLEBAR_HEIGHT);
    cr = cairo_create(surface);
    cairo_set_source_rgba(cr, LIGHT_EDGE);  
    cairo_rectangle(cr, 0, 0, PIXMAP_SIZE, LIGHT_EDGE_HEIGHT);
    cairo_fill(cr); 
    cairo_set_source_rgba(cr, BODY);  
    cairo_rectangle(cr, 0, LIGHT_EDGE_HEIGHT, PIXMAP_SIZE, TITLEBAR_HEIGHT - LIGHT_EDGE_HEIGHT);
    cairo_fill(cr);
  break;
  case selection:
    pixmap = XCreatePixmap(display, root, BUTTON_SIZE, BUTTON_SIZE, XDefaultDepth(display, screen_number));
    surface = cairo_xlib_surface_create(display, pixmap, colours, BUTTON_SIZE, BUTTON_SIZE);
    cr = cairo_create(surface);
    //this is the negative of the position of the selection_indicator window relative to the titlebar
    cairo_translate(cr, -H_SPACING, -V_SPACING);
    
    //drawing the background
    cairo_set_source_rgba(cr, LIGHT_EDGE);  
    cairo_rectangle(cr, 0, 0, BUTTON_SIZE + H_SPACING, LIGHT_EDGE_HEIGHT);
    cairo_fill(cr); 
    cairo_set_source_rgba(cr, BODY);  
    cairo_rectangle(cr, 0, LIGHT_EDGE_HEIGHT, BUTTON_SIZE + H_SPACING, TITLEBAR_HEIGHT - LIGHT_EDGE_HEIGHT);
    cairo_fill(cr);
    
    //draw selection spot indicator    
    cairo_set_source_rgba(cr, SPOT);
    cairo_arc(cr, 14, 14, 8, 0, 2* M_PI);
    cairo_fill(cr);
    cairo_arc(cr, 14, 14, 7.5, 0, 2* M_PI);
    cairo_set_line_width(cr, 1);
    cairo_set_source_rgba(cr, BORDER);
    cairo_stroke(cr);
  break;
  
  /*** Button background pixmaps start here ***/
  case arrow_normal:
  case arrow_pressed:
  case arrow_deactivated:
    pixmap = XCreatePixmap(display, root, BUTTON_SIZE - EDGE_WIDTH, BUTTON_SIZE -EDGE_WIDTH*4, XDefaultDepth(display, screen_number));
    surface = cairo_xlib_surface_create(display, pixmap, colours, BUTTON_SIZE - EDGE_WIDTH, BUTTON_SIZE -EDGE_WIDTH*4);
    cr = cairo_create(surface);

    //draw_light and dark background
    cairo_set_source_rgba(cr, LIGHT_EDGE);
    cairo_rectangle(cr, 0, 0, BUTTON_SIZE - EDGE_WIDTH, BUTTON_SIZE - EDGE_WIDTH*4);
    cairo_fill(cr);
      
    if(type == arrow_pressed)  cairo_set_source_rgba(cr, LIGHT_EDGE);
    else cairo_set_source_rgba(cr, BODY);
    
    cairo_rectangle(cr, 0, LIGHT_EDGE_HEIGHT, BUTTON_SIZE - EDGE_WIDTH, BUTTON_SIZE - EDGE_WIDTH*4 - LIGHT_EDGE_HEIGHT);
    cairo_fill(cr);
    
    #ifdef SHARP_SYMBOLS
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    #endif 
    
    if(type == arrow_deactivated) cairo_set_source_rgba(cr, TEXT_DEACTIVATED);
    else cairo_set_source_rgba(cr, TEXT);
    //draw arrow
    cairo_move_to(cr, BUTTON_SIZE - EDGE_WIDTH - 14, 6);
    cairo_line_to(cr, BUTTON_SIZE - EDGE_WIDTH - 5, 6);
    cairo_line_to(cr, BUTTON_SIZE - EDGE_WIDTH - 10, 11);
    cairo_close_path(cr);
    cairo_fill(cr);    
    
  break;
  case close_button_normal:
  case close_button_pressed:
  case close_button_deactivated:
    pixmap = XCreatePixmap(display, root, BUTTON_SIZE, BUTTON_SIZE, XDefaultDepth(display, screen_number));    
    surface = cairo_xlib_surface_create(display, pixmap, colours, BUTTON_SIZE, BUTTON_SIZE);
    cr = cairo_create(surface);
  
    //draw the button background
    cairo_set_source_rgba(cr, BORDER);
    cairo_rectangle(cr, 0, 0, 20, 20);
    cairo_fill(cr);

    cairo_set_source_rgba(cr, LIGHT_EDGE);  
    cairo_rectangle(cr, 1, 1, 18, 18);
    cairo_fill(cr);
    
    if(type == close_button_pressed)  cairo_set_source_rgba(cr, LIGHT_EDGE);
    else cairo_set_source_rgba(cr, BODY);
    
    cairo_rectangle(cr, 2, 2+LIGHT_EDGE_HEIGHT, 16, 16 - LIGHT_EDGE_HEIGHT);
    cairo_fill(cr);
    
    #ifdef SHARP_SYMBOLS
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    #endif 
    //draw the close cross symbol
    if(type == close_button_deactivated) cairo_set_source_rgba(cr, TEXT_DEACTIVATED);
    else cairo_set_source_rgba(cr, TEXT);
    
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
          
    cairo_set_line_width(cr, 2);
    cairo_move_to(cr, 1+3+1+1, 3+2+1);
    cairo_line_to(cr, 1+3+1+9, 3+2+9);
    cairo_stroke(cr);
    cairo_move_to(cr, 1+3+1+1, 3+2+9);
    cairo_line_to(cr, 1+3+1+9, 3+2+1);  
    cairo_stroke(cr);
  
  break;
  case pulldown_floating_normal:
  case pulldown_floating_pressed:
  case pulldown_tiling_normal:
  case pulldown_tiling_pressed:
  case pulldown_deactivated:
    pixmap = XCreatePixmap(display, root, PULLDOWN_WIDTH, BUTTON_SIZE, XDefaultDepth(display, screen_number));    
  //same as above but plain colour backgrounds. 
  //inactive versions have normal weight font
  //hover have light background    
  case item_floating:
  case item_floating_hover:
  case item_sinking:
  case item_sinking_hover:
  case item_tiling:
  case item_tiling_hover:
    if (type == item_floating
      || type == item_tiling
      || type == item_sinking
      || type == item_floating_hover
      || type == item_tiling_hover
      || type == item_sinking_hover)
      pixmap = XCreatePixmap(display, root, PULLDOWN_WIDTH, MENU_ITEM_HEIGHT, XDefaultDepth(display, screen_number));    

    surface = cairo_xlib_surface_create(display, pixmap, colours, PULLDOWN_WIDTH, BUTTON_SIZE);
    cr = cairo_create(surface);
    
    
    if(type == pulldown_floating_normal
        || type == pulldown_floating_pressed
        || type == pulldown_tiling_normal
        || type == pulldown_tiling_pressed
        || type == pulldown_deactivated) {
      cairo_set_source_rgba(cr, BORDER);
      
    }  
    else if(type == item_floating
             || type == item_tiling
             || type == item_sinking) {
      cairo_set_source_rgba(cr, BODY); 
    }
    else //for the hover modes
      cairo_set_source_rgba(cr, LIGHT_EDGE);


    cairo_rectangle(cr, 0, 0, PULLDOWN_WIDTH, BUTTON_SIZE + MENU_ITEM_HEIGHT);
    cairo_fill(cr);

    if (type == item_floating
      || type == item_tiling
      || type == item_sinking
      || type == item_floating_hover
      || type == item_tiling_hover
      || type == item_sinking_hover)
      cairo_translate(cr, 0, MENU_ITEM_VS_BUTTON_Y_DIFF);

    if(type == item_floating
       || type == item_tiling
       || type == item_sinking)
      cairo_set_source_rgba(cr, BODY);    
    else 
      cairo_set_source_rgba(cr, LIGHT_EDGE); 
    
    cairo_rectangle(cr, EDGE_WIDTH, EDGE_WIDTH, PULLDOWN_WIDTH - EDGE_WIDTH*2, BUTTON_SIZE - EDGE_WIDTH*2);
    cairo_fill(cr);

    if(type == pulldown_tiling_pressed  
       || type == pulldown_floating_pressed
       || type == item_floating_hover
       || type == item_tiling_hover
       || type == item_sinking_hover)
      cairo_set_source_rgba(cr, LIGHT_EDGE);  
    else cairo_set_source_rgba(cr, BODY);  
    
    cairo_rectangle(cr, EDGE_WIDTH*2, EDGE_WIDTH*2, PULLDOWN_WIDTH - EDGE_WIDTH*4, 11); //11 will be the height of the darkened area
    cairo_fill(cr); 

    cairo_set_source_rgba(cr, TEXT);
    if(type == item_floating
       || type == item_floating
       || type == item_tiling
       || type == item_sinking
       || type == item_floating_hover
       || type == item_tiling_hover
       || type == item_sinking_hover)
      cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL); 
      
    else 
      cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD); 

    cairo_set_font_size(cr, 13.5); 
    cairo_move_to(cr, 22, 15); 
    
    if(type == pulldown_tiling_normal  
       ||  type == pulldown_tiling_pressed  
       ||  type == item_tiling
       ||  type == item_tiling_hover) {
      cairo_show_text(cr, "Tiling");
      
      cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
      cairo_rectangle(cr, 5, 7, 4, 7);
      cairo_rectangle(cr, 4, 5, 6, 10);
      cairo_fill(cr);

      cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
      cairo_rectangle(cr, 12, 7, 4, 7);
      cairo_rectangle(cr, 11, 5, 6, 10);
      cairo_fill(cr);    
    }
    else if(type == pulldown_deactivated
             ||  type == item_sinking
             ||  type == item_sinking_hover) {
      if(type == pulldown_deactivated)
        cairo_set_source_rgba(cr, TEXT_DEACTIVATED);
        
      cairo_show_text(cr, "Sinking");
              
      cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD); //means don't fill areas that are filled twice.
      cairo_rectangle(cr, 4, 4, 11, 12);
      cairo_rectangle(cr, 5, 6, 9, 9);
      cairo_fill(cr); 

      #ifdef SHARP_SYMBOLS
      cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);    
      #endif

      cairo_move_to(cr, 7, 10);
      cairo_line_to(cr, 12, 10);
      cairo_line_to(cr, 9, 13);
      cairo_close_path(cr);
      cairo_fill(cr);    
      
      cairo_set_fill_rule (cr, CAIRO_FILL_RULE_WINDING);
      
      cairo_rectangle(cr, 9, 8, 1, 4);
      cairo_fill(cr);
      cairo_stroke(cr);
      
    }
    else if(type == pulldown_floating_normal  
             ||  type == pulldown_floating_pressed
             ||  type == item_floating
             ||  type == item_floating_hover) {
      cairo_show_text(cr, "Floating");
      
      cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD); //means don't fill areas that are filled twice.
      cairo_rectangle(cr, 4, 4, 9, 9);
      cairo_rectangle(cr, 5, 6, 7, 6);
      
      cairo_rectangle(cr, 8, 8, 5, 5); //cut off the corner for the 2nd window icon
      cairo_rectangle(cr, 8, 8, 4, 4);
      cairo_fill(cr);

      cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
      cairo_rectangle(cr, 8, 8, 9, 9);
      cairo_rectangle(cr, 9, 10, 7, 6);
      cairo_fill(cr); 
      
      
    }
    if(type == pulldown_floating_normal
        || type == pulldown_floating_pressed
        || type == pulldown_tiling_normal
        || type == pulldown_tiling_pressed
        || type == pulldown_deactivated) { 
      #ifdef SHARP_SYMBOLS
      cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);    
      #endif

      //draw arrow
      cairo_move_to(cr, PULLDOWN_WIDTH - 14, 8);
      cairo_line_to(cr, PULLDOWN_WIDTH - 5, 8);
      cairo_line_to(cr, PULLDOWN_WIDTH - 10, 13);
      cairo_close_path(cr);
      cairo_fill(cr); 
    }
  break;
  }

  cairo_destroy (cr);  
  cairo_surface_destroy(surface);
  return pixmap;
}

//Width and height arguments cannot be negative or zero
//This function draws the title pop-up menu
Pixmap create_title_pixmap(Display* display, const char* title, enum title_pixmap type) {
  Window root = DefaultRootWindow(display); 
  int screen_number = DefaultScreen (display);
  Screen* screen = DefaultScreenOfDisplay(display);
  Visual *colours =  DefaultVisual(display, screen_number);
  Pixmap pixmap;
  cairo_surface_t *surface;
  cairo_t *cr;  
  if(type == title_normal  ||  type == title_pressed  ||  type == title_deactivated)
    pixmap = XCreatePixmap(display, root, XWidthOfScreen(screen), TITLE_MAX_HEIGHT, XDefaultDepth(display, screen_number));
  else pixmap = XCreatePixmap(display, root, XWidthOfScreen(screen), MENU_ITEM_HEIGHT, XDefaultDepth(display, screen_number));

  surface = cairo_xlib_surface_create(display, pixmap, colours,  XWidthOfScreen(screen), TITLE_MAX_HEIGHT);
  cr = cairo_create(surface);
  switch(type) {
    case title_normal:
    case title_pressed:
    case title_deactivated: 
      //draw_light and dark background
      cairo_set_source_rgba(cr, LIGHT_EDGE);  
      cairo_rectangle(cr, 0, 0, XWidthOfScreen(screen), TITLE_MAX_HEIGHT);
      cairo_fill(cr);
      
      if(type == title_pressed)  cairo_set_source_rgba(cr, LIGHT_EDGE);
      else cairo_set_source_rgba(cr, BODY);
      
      cairo_rectangle(cr, 0, LIGHT_EDGE_HEIGHT, XWidthOfScreen(screen), TITLE_MAX_HEIGHT - LIGHT_EDGE_HEIGHT);
      cairo_fill(cr);  

      //draw text    
      if(type == title_deactivated) {
        cairo_set_source_rgba(cr, TEXT_DEACTIVATED);
        cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
      }
      else  {
        cairo_set_source_rgba(cr, TEXT);  
        cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
      }
      cairo_set_font_size(cr, 13.5); 
      cairo_move_to(cr, 3, 13);
      if(title != NULL)  cairo_show_text(cr, title);
      cairo_fill(cr);  
    break;
    case item_title:
    case item_title_hover:
    
      if(type == item_title_hover)
        cairo_set_source_rgba(cr, LIGHT_EDGE);
      else cairo_set_source_rgba(cr, BODY);

      cairo_rectangle(cr, 0, 0, XWidthOfScreen(screen), MENU_ITEM_HEIGHT);      
      cairo_fill(cr);
      cairo_translate(cr, 0, MENU_ITEM_VS_BUTTON_Y_DIFF);      
      cairo_set_source_rgba(cr, TEXT);  
      
      cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
      
      cairo_set_font_size(cr, 13.5);
      cairo_move_to(cr, 3, 15);
      if(title != NULL)  cairo_show_text(cr, title);
    break;
  }
  cairo_destroy (cr);  
  cairo_surface_destroy(surface);
  return pixmap;
};

int get_title_width(Display* display, const char *title) {
  Window root = DefaultRootWindow(display); 
  int screen_number = DefaultScreen (display);
  Screen* screen = DefaultScreenOfDisplay(display);
  Visual *colours =  DefaultVisual(display, screen_number);
  Window temp;
  cairo_surface_t *surface;
  cairo_t *cr;    
  cairo_text_extents_t extents;
  
  temp = XCreateSimpleWindow(display, root, 0, 0, PIXMAP_SIZE, PIXMAP_SIZE, 0, WhitePixelOfScreen(screen), BlackPixelOfScreen(screen));
  XFlush(display);
  surface = cairo_xlib_surface_create(display, temp, colours,  XWidthOfScreen(screen), TITLE_MAX_HEIGHT);
  cr = cairo_create(surface);
  
  cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, 13.5); 
  cairo_text_extents(cr, title, &extents);
  
  XDestroyWindow(display, temp); 
  XFlush(display);
  cairo_destroy (cr);  
  cairo_surface_destroy(surface);
  
  if(extents.x_bearing < 0) extents.x_advance -=  extents.x_bearing;
  
  return (int)extents.x_advance + H_SPACING;
}

