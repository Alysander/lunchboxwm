
/****THEME DERIVED CONSTANTS******/
//this is the number of pixels the frame takes up from the top and bottom together of a window
#define FRAME_VSPACE 34
//this is the number of pixels the frame takes up from either side of a window together
#define FRAME_HSPACE 12
//this is the x,y position inside the frame for the reparented window
#define FRAME_XSTART 6
#define FRAME_YSTART 28
#define LIGHT_EDGE_HEIGHT 7
#define PULLDOWN_WIDTH 100

/***** Colours for cairo as rgba percentages *******/
#define SPOT            0.235294118, 0.549019608, 0.99, 1
#define SPOT_EDGE       0.235294118, 0.549019608, 0.99, 0.35
#define BACKGROUND      0.4, 0.4, 0.4, 1
#define BORDER          0.13, 0.13, 0.13, 1
#define INNER_BORDER    0.0, 0.0, 0.0, 1
#define LIGHT_EDGE      0.34, 0.34, 0.34, 1
#define BODY            0.27, 0.27, 0.27, 1
#define SHADOW          0.0, 0.0, 0.0, 1
#define TEXT            100, 100, 100, 1
#define TEXT_DISABLED   0.6, 0.6, 0.6, 1
 
//Draws a solid colour background over the root window
void draw_background(Display* display, Window backwin, cairo_surface_t *surface) {
  cairo_t *cr = cairo_create(surface);
  cairo_set_source_rgba(cr, BACKGROUND);
  
  cairo_paint(cr);
  
  XMapWindow(display, backwin);
  cairo_destroy (cr);
}

//Draws a close button on the closebutton window on the frame and then maps it
void draw_closebutton(Display* display, struct Frame frame) {
  cairo_t *cr = cairo_create (frame.closebutton_s);
  //draw the close button  
  cairo_set_source_rgba(cr, INNER_BORDER);
  cairo_rectangle(cr, 0, 0, 20, 20);
  cairo_fill(cr);

  cairo_set_source_rgba(cr, LIGHT_EDGE);  
  cairo_rectangle(cr, 1, 1, 18, 18);
  cairo_fill(cr);
  
  cairo_set_source_rgba(cr, BODY);  
  cairo_rectangle(cr, 2, 2+5, 16, 16 - 5); //5 will be the height of the light edge
  cairo_fill(cr); 
 
  //draw the close cross
  //cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
  cairo_set_source_rgba(cr, TEXT);
  cairo_set_line_width(cr, 2);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
  cairo_move_to(cr, 1+3+1+1, 3+2+1);
  cairo_line_to(cr, 1+3+1+9, 3+2+9);
  cairo_stroke(cr);
  cairo_move_to(cr, 1+3+1+1, 3+2+9);
  cairo_line_to(cr, 1+3+1+9, 3+2+1);  
  cairo_stroke(cr);
  
  XMapWindow(display, frame.closebutton);  
  cairo_destroy (cr);
}

//Draws the pulldown list and maps it
void draw_pulldown(Display* display, struct Frame frame) {
  cairo_t *cr = cairo_create (frame.pulldown_s);
  cairo_set_source_rgba(cr, INNER_BORDER);
  cairo_rectangle(cr, 0, 0, PULLDOWN_WIDTH, 20);
  cairo_fill(cr);

  cairo_set_source_rgba(cr, LIGHT_EDGE);  
  cairo_rectangle(cr, 1, 1, PULLDOWN_WIDTH - 2, 18);
  cairo_fill(cr);
  
  cairo_set_source_rgba(cr, BODY);  
  cairo_rectangle(cr, 2, 2, PULLDOWN_WIDTH - 4, 11); //11 will be the height of the darkened area
  cairo_fill(cr); 
 
  cairo_set_source_rgba(cr, TEXT);  
  cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, 11.5); 
  cairo_move_to(cr, 22, 15);
    
  if(frame.mode == TILING) {
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
  else if(frame.mode == SINKING) {
    cairo_show_text(cr, "Sinking");
    
  }
  else if(frame.mode == FLOATING) {
 
    cairo_show_text(cr, "Floating");
    
    cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD); //dont fill areas that are filled twice.
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
  
  //draw arrow
  //cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
  cairo_move_to(cr, PULLDOWN_WIDTH - 14, 8);
  cairo_line_to(cr, PULLDOWN_WIDTH - 5, 8);
  cairo_line_to(cr, PULLDOWN_WIDTH - 10, 13);
  cairo_close_path(cr);
  cairo_fill(cr);
 
  XMapWindow(display, frame.closebutton);
  cairo_destroy (cr);
}

//Draws the frame and maps the frame and the framed window
void draw_frame(Display* display, struct Frame frame) {

  cairo_t *cr = cairo_create (frame.frame_s);

  XMoveWindow(display, frame.closebutton, frame.w-20-8-1, 4);
  XMoveWindow(display, frame.pulldown, frame.w-20-8-1-PULLDOWN_WIDTH-4, 4);
  XMoveWindow(display, frame.window, FRAME_XSTART, FRAME_YSTART);
  
  //from basin_black.c
  //draw dark grey border
  cairo_set_source_rgba(cr, BORDER);
  cairo_rectangle(cr, 0,0, frame.w, frame.h);  
  cairo_fill(cr);
 
  //draw frame highlight
  cairo_set_source_rgba(cr, LIGHT_EDGE);
  cairo_rectangle(cr, 1, 1, frame.w-2, LIGHT_EDGE_HEIGHT);
  cairo_fill(cr);
 
  //draw frame body
  cairo_set_source_rgba(cr, BODY);
  cairo_rectangle(cr, 1, LIGHT_EDGE_HEIGHT+1, frame.w-2, frame.h-LIGHT_EDGE_HEIGHT-2);
  cairo_fill(cr);  
  
  if(frame.selected > 0) {
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
  }
  
  //draw inner frame
  cairo_set_source_rgba(cr, BORDER);
  cairo_rectangle(cr, 5, 27, frame.w-10, frame.h-32);
  cairo_fill(cr);
 
  //draw an inner background
  cairo_set_source_rgba(cr, BACKGROUND);
  cairo_rectangle(cr, 6, 28, frame.w-12, frame.h-34);
  cairo_fill(cr);
  
  draw_closebutton(display, frame);
  draw_pulldown(display, frame);
    
  if(frame.window_name != NULL) {
    cairo_rectangle(cr, 25, 3, frame.w-20-8-1- 25- 4 - PULLDOWN_WIDTH- 4, 20); //subtract extra for pulldown list later
    cairo_clip(cr);

    cairo_set_source_rgba(cr, SHADOW);
    cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14.5); 
    cairo_move_to(cr, 27, 20);
    cairo_show_text(cr, frame.window_name);
    //draw text    
    cairo_set_source_rgba(cr, TEXT);
    cairo_move_to(cr, 26, 19);  
    cairo_show_text(cr, frame.window_name);
    //end mask
  }

  XMapWindow(display, frame.frame);
  XMapWindow(display, frame.window);  

  XFlush(display);
  cairo_destroy (cr);
}
