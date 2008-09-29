
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
  case arrow_pressed:
  case arrow_normal:
    pixmap = XCreatePixmap(display, root, BUTTON_SIZE - EDGE_WIDTH, BUTTON_SIZE -EDGE_WIDTH*4, XDefaultDepth(display, screen_number));
    surface = cairo_xlib_surface_create(display, pixmap, colours, BUTTON_SIZE - EDGE_WIDTH, BUTTON_SIZE -EDGE_WIDTH*4);
    cr = cairo_create(surface);

    //draw_light and dark background
    cairo_set_source_rgba(cr, LIGHT_EDGE);  
    cairo_rectangle(cr, 0, 0, BUTTON_SIZE - EDGE_WIDTH, BUTTON_SIZE - EDGE_WIDTH*4);
    cairo_fill(cr);
      
    cairo_set_source_rgba(cr, BODY);  
    cairo_rectangle(cr, 0, LIGHT_EDGE_HEIGHT, BUTTON_SIZE - EDGE_WIDTH, BUTTON_SIZE - EDGE_WIDTH*4 - LIGHT_EDGE_HEIGHT);
    cairo_fill(cr);  
    
    #ifdef SHARP_SYMBOLS
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    #endif 
    
    //draw arrow
    cairo_set_source_rgba(cr, TEXT);
    cairo_move_to(cr, BUTTON_SIZE - EDGE_WIDTH - 14, 6);
    cairo_line_to(cr, BUTTON_SIZE - EDGE_WIDTH - 5, 6);
    cairo_line_to(cr, BUTTON_SIZE - EDGE_WIDTH - 10, 11);
    cairo_close_path(cr);
    cairo_fill(cr); 
  break;
  case close_button_normal:
  case close_button_pressed:
    pixmap = XCreatePixmap(display, root, BUTTON_SIZE, BUTTON_SIZE, XDefaultDepth(display, screen_number));    
    surface = cairo_xlib_surface_create(display, pixmap, colours, BUTTON_SIZE, BUTTON_SIZE);
    cr = cairo_create(surface);
  
    //draw the button background
    cairo_set_source_rgba(cr, INNER_BORDER);
    cairo_rectangle(cr, 0, 0, 20, 20);
    cairo_fill(cr);

    cairo_set_source_rgba(cr, LIGHT_EDGE);  
    cairo_rectangle(cr, 1, 1, 18, 18);
    cairo_fill(cr);
    
    cairo_set_source_rgba(cr, BODY);  
    cairo_rectangle(cr, 2, 2+LIGHT_EDGE_HEIGHT, 16, 16 - LIGHT_EDGE_HEIGHT);
    cairo_fill(cr);
    
    #ifdef SHARP_SYMBOLS
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    #endif 
    //draw the close cross symbol
    cairo_set_source_rgba(cr, TEXT);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
          
    if(type == close_button_normal) {
      cairo_set_line_width(cr, 2);
      cairo_move_to(cr, 1+3+1+1, 3+2+1);
      cairo_line_to(cr, 1+3+1+9, 3+2+9);
      cairo_stroke(cr);
      cairo_move_to(cr, 1+3+1+1, 3+2+9);
      cairo_line_to(cr, 1+3+1+9, 3+2+1);  
      cairo_stroke(cr);
    }
    else {
      //Make the symbol look bigger if it is pressed
      cairo_set_line_width(cr, 3);
      cairo_move_to(cr, 1+3+1+1, 3+2+1);
      cairo_line_to(cr, 1+3+1+9, 3+2+9);
      cairo_stroke(cr);
      cairo_move_to(cr, 1+3+1+1, 3+2+9);
      cairo_line_to(cr, 1+3+1+9, 3+2+1);  
      cairo_stroke(cr);    
    }
  
  break;
  break;
  case selection:
    pixmap = XCreatePixmap(display, root, BUTTON_SIZE, BUTTON_SIZE, XDefaultDepth(display, screen_number));    
    surface = cairo_xlib_surface_create(display, pixmap, colours, BUTTON_SIZE, BUTTON_SIZE);
    cr = cairo_create(surface);
    //need to draw the background somehow
    //draw selection spot indicator
    cairo_set_source_rgba(cr, SHADOW);
    cairo_arc(cr, 15, 15.5, 6, 0, 2* M_PI);
    cairo_fill(cr);    
    cairo_set_source_rgba(cr, SPOT);
    cairo_arc(cr, 15, 14.5, 6, 0, 2* M_PI);
    cairo_fill_preserve(cr);
    cairo_set_line_width(cr, 1);
    cairo_set_source_rgba(cr, SPOT_EDGE);
    cairo_stroke(cr);
    cairo_set_line_width(cr, 1);  
      
  break;
  //if pressed make the arrow bigger
  case pulldown_floating_normal:
  case pulldown_floating_pressed:
  case pulldown_sinking_normal:
  case pulldown_sinking_pressed:
  case pulldown_tiling_normal:
  case pulldown_tiling_pressed:
  
    pixmap = XCreatePixmap(display, root, PULLDOWN_WIDTH, BUTTON_SIZE, XDefaultDepth(display, screen_number));    
    surface = cairo_xlib_surface_create(display, pixmap, colours, PULLDOWN_WIDTH, BUTTON_SIZE);
    cr = cairo_create(surface);
    
    cairo_set_source_rgba(cr, INNER_BORDER);
    cairo_rectangle(cr, 0, 0, PULLDOWN_WIDTH, BUTTON_SIZE);
    cairo_fill(cr);

    cairo_set_source_rgba(cr, LIGHT_EDGE);  
    cairo_rectangle(cr, EDGE_WIDTH, EDGE_WIDTH, PULLDOWN_WIDTH - EDGE_WIDTH*2, BUTTON_SIZE - EDGE_WIDTH*2);
    cairo_fill(cr);

    cairo_set_source_rgba(cr, BODY);  
    cairo_rectangle(cr, EDGE_WIDTH*2, EDGE_WIDTH*2, PULLDOWN_WIDTH - EDGE_WIDTH*4, 11); //11 will be the height of the darkened area
    cairo_fill(cr); 

    cairo_set_source_rgba(cr, TEXT);  
    cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 13.5); 
    cairo_move_to(cr, 22, 15); 
    
    if(type == pulldown_tiling_normal  ||  type == pulldown_tiling_pressed) {
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
    else if(type == pulldown_sinking_normal  ||  type == pulldown_sinking_pressed) {
      //TODO:
      cairo_show_text(cr, "Sinking");
    }
    else if(type == pulldown_floating_normal  ||  type == pulldown_floating_pressed) {
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
    #ifdef SHARP_SYMBOLS
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);    
    #endif
    if(type == pulldown_sinking_pressed  ||  type == pulldown_tiling_pressed  ||  type == pulldown_floating_pressed) {
      //draw arrow bigger
      cairo_move_to(cr, PULLDOWN_WIDTH - 16, 7);
      cairo_line_to(cr, PULLDOWN_WIDTH - 4, 7);
      cairo_line_to(cr, PULLDOWN_WIDTH - 10, 13);
      cairo_close_path(cr);
      cairo_fill(cr);     
    }
    else {
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
Pixmap create_title_pixmap(Display* display, char* title) {
  Window root = DefaultRootWindow(display); 
  int screen_number = DefaultScreen (display);
  Screen* screen = DefaultScreenOfDisplay(display);
  Visual *colours =  DefaultVisual(display, screen_number);
  Pixmap pixmap;
  cairo_surface_t *surface;
  cairo_t *cr;  

  pixmap = XCreatePixmap(display, root, XWidthOfScreen(screen), TITLE_MAX_HEIGHT,
                                XDefaultDepth(display, screen_number));

  surface = cairo_xlib_surface_create(display, pixmap, colours,  XWidthOfScreen(screen), TITLE_MAX_HEIGHT);
  cr = cairo_create(surface);

  //draw_light and dark background
  cairo_set_source_rgba(cr, LIGHT_EDGE);  
  cairo_rectangle(cr, 0, 0, XWidthOfScreen(screen), TITLE_MAX_HEIGHT);
  cairo_fill(cr);
    
  cairo_set_source_rgba(cr, BODY);  
  cairo_rectangle(cr, 0, LIGHT_EDGE_HEIGHT, XWidthOfScreen(screen), TITLE_MAX_HEIGHT - LIGHT_EDGE_HEIGHT);
  cairo_fill(cr);  

  //draw text    
  cairo_set_source_rgba(cr, TEXT);  
  cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, 13.5); 
  cairo_move_to(cr, 3, 13);
  if(title != NULL)  cairo_show_text(cr, title);
    
  cairo_fill(cr);  

  cairo_destroy (cr);  
  cairo_surface_destroy(surface);
  return pixmap;
};

 
/*** Draws the frame and maps the frame and the framed window ***/
void draw_frame(Display* display, struct Frame frame) {

  XMoveWindow(display, frame.close_button, frame.w - H_SPACING - BUTTON_SIZE - EDGE_WIDTH - 1, V_SPACING);
  XMoveWindow(display, frame.mode_pulldown, frame.w - H_SPACING*2 - PULLDOWN_WIDTH - BUTTON_SIZE - EDGE_WIDTH - 1, V_SPACING);
  XMoveWindow(display, frame.title_menu.arrow, frame.w - TITLEBAR_USED_WIDTH - EDGE_WIDTH*2 - BUTTON_SIZE, EDGE_WIDTH);
  XResizeWindow(display, frame.title_menu.frame, frame.w - TITLEBAR_USED_WIDTH, BUTTON_SIZE);
  XResizeWindow(display, frame.title_menu.body,frame.w - TITLEBAR_USED_WIDTH - EDGE_WIDTH*2, BUTTON_SIZE - EDGE_WIDTH*2);
  XResizeWindow(display, frame.title_menu.title,  frame.w - TITLE_MAX_WIDTH_DIFF, TITLE_MAX_HEIGHT);
//  if(frame.w - TITLE_MAX_WIDTH_DIFF <
//  XMoveWindow(display, frame.window, EDGE_WIDTH, EDGE_WIDTH);
  

  XFlush(display);
}
