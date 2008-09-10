#include <stdio.h>
#include <stdlib.h>
//#include <math.h>
#define M_PI 3.14159265359
#include <X11/Xlib.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#define WINSPACING 9

//this amount in pixels * 3 are used when the top level size cannot be established
#define MINWIDTH 80 
#define MINHEIGHT 60

#define FLOATING 1
#define TILING 2
#define SINKING 3

/****THEME DERIVED CONSTANTS******/
//this is the number of pixels the frame takes up from the top and bottom together of a window
#define FRAME_VSPACE 34

//this is the number of pixels the frame takes up from either side of a window together
#define FRAME_HSPACE 12

//this is the x,y position inside the frame for the reparented window
#define FRAME_XSTART 6
#define FRAME_YSTART 28

#define LIGHT_EDGE_HEIGHT 11

/***** Colours for cairo as rgba percentages *******/
#define SPOT            0.235294118, 0.549019608, 0.99, 1
#define SPOT_EDGE       0.235294118, 0.549019608, 0.99, 0.35
#define BACKGROUND         0.4, 0.4, 0.4, 1
//should background be background or desktop?
#define BORDER          0.13, 0.13, 0.13, 1
#define INNER_BORDER    0.0, 0.0, 0.0, 1
#define LIGHT_EDGE      0.34, 0.34, 0.34, 1
#define BODY            0.27, 0.27, 0.27, 1
#define SHADOW          0.0, 0.0, 0.0, 1
#define TEXT            100, 100, 100, 1
#define TEXT_DISABLED   0.6, 0.6, 0.6, 1
 

struct Frame {
  Window window;
  unsigned int x,y,w,h;
  int mode; //FLOATING || TILING || SINKING
  int min_width, max_width;
  int min_height, max_height; 
  Window frame; 
  Window pulldown;
  Window closebutton;
  Window NW_grip, N_grip, NE_grip, E_grip, SE_grip, S_grip, SW_grip, W_grip;  //InputOnly extends into frame
};

struct Framelist {
  struct Frame* list;
  int max, used;
};

//int find_frame_index (struct Framelist* frames, Window window);
//void find_adjacent (struct Framelist* frames, int frame_index);   

void draw_background(Display* display, Window backwin); //this draws a solid colour background
void draw_frame (Display* display, struct Frame frame);
int create_frame (Display* display, struct Framelist* frames, Window window); //returns the frame index of the newly created window or -1 if out of memory

int main (int argc, char* argv[]) {
  Display* display = NULL; 
  Screen* screen;  
  XEvent event;
  Window root, backwin;

  struct Framelist frames; //need a different frame for each window.
  //need to keep track of all the managed windows too

  printf("\n");
  if(argc > 1) {
    printf("Opening Basin on Display: %s\n", argv[1]);
    printf("This seems to hang if you give a wrong screen\n");
    display = XOpenDisplay(argv[1]);
  }
  else display = XOpenDisplay(NULL);
  
  if(display == NULL)  {
    printf("Could not open display.\n");
    printf("USAGE: basin [DISPLAY]\n");
    return -1;
  }

//  if(frames.list == NULL) {
    frames.used = 0;
    frames.max = 16;
    frames.list = malloc(sizeof(struct Framelist) * 16);
//  }
  
  root = DefaultRootWindow(display);
  //this is strangely different from DefaultScreen which returns int
  screen = DefaultScreenOfDisplay(display);  
  backwin = 
    XCreateSimpleWindow(display, root, 0, 0, XWidthOfScreen(screen), XHeightOfScreen(screen), 
      0, WhitePixelOfScreen(screen), BlackPixelOfScreen(screen)); 
  
  //TODO: configure this window to have stack mode below with an XWindowChanges structure
  XMapWindow(display, backwin);
  draw_background(display, backwin);
  
  XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask);
  /* TODO: Create an error handler to detect if a window manager is already running 
     with XSynchronize and XSetErrorHandler. */
  XSelectInput(display, backwin, PointerMotionMask|ExposureMask); 
  //do a select input for the backwin
  for( ; ;) {
    XNextEvent(display, &event);
    switch(event.type) {
      case MapRequest:
        printf("A top level window is being mapped: %d \n", event.xmaprequest.window); 

        //Do we need to check if the window is already mapped?
        int index = create_frame(display, &frames, event.xmaprequest.window);
        if(index != -1) {
          draw_frame(display, frames.list[index]);
        }
      break;
      case MotionNotify:
        //printf("(%d, %d)\n", event.xmotion.x, event.xmotion.y);
        /*
          Detect resize mode has started
          mouse moves,
          find the "touching windows"
          if they can't be resized then don't resize them
          otherwise go straight ahead
        */
      break;
      case Expose:
        if(event.xexpose.window == backwin)  draw_background(display, backwin);
        for(int i = 0; i < frames.used; i++) {
          if(event.xexpose.window == frames.list[i].frame) { 
            draw_frame(display, frames.list[i]);
            break;
          }
        }
      break;
      case UnmapNotify:
        printf("Unmap notify\n %d\n", event.xunmap.window);
        for(int i = 0; i < frames.used; i++) {
          if(event.xunmap.window == frames.list[i].window) XUnmapSubwindows (display, frames.list[i].frame);
          break;
        }
      break;
    }
  }
  XCloseDisplay(display);
  //go through and unmap the windows
  free(frames.list);
  return 1;  
}

void draw_background(Display* display, Window backwin) {
  Visual* colours = XDefaultVisual(display, DefaultScreen(display));
  Screen* screen = DefaultScreenOfDisplay(display);
  cairo_surface_t *background = cairo_xlib_surface_create(display, backwin, colours, XWidthOfScreen(screen), XHeightOfScreen(screen));
  
  cairo_t *cr = cairo_create(background);
  
  cairo_set_source_rgba(cr, BACKGROUND);
  cairo_paint(cr);
  printf("draw_background, backwin = %d\n", backwin);
  XMapWindow(display, backwin);
  XFlush(display);
  cairo_surface_destroy(background);
  cairo_destroy (cr);
}

void draw_closebutton(Display* display, Window window) {
  Visual* colours = XDefaultVisual(display, DefaultScreen(display));
  
  cairo_surface_t *background = cairo_xlib_surface_create(display, window, colours, 20, 20);
  cairo_t *cr = cairo_create (background);
  
  //draw the close button  
  cairo_set_source_rgba(cr, INNER_BORDER);
  cairo_rectangle(cr, 0, 0, 20, 20);
  cairo_fill(cr);

  cairo_set_source_rgba(cr, LIGHT_EDGE);  
  cairo_rectangle(cr, 1, 1, 18, 18);
  cairo_fill(cr);
  
  cairo_set_source_rgba(cr, BODY);  
  cairo_rectangle(cr, 2, 2+LIGHT_EDGE_HEIGHT, 16, 16 - LIGHT_EDGE_HEIGHT);
  cairo_fill(cr);
  
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
  //draw the close cross
  cairo_set_source_rgba(cr, TEXT);
  cairo_set_line_width(cr, 2);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
  cairo_move_to(cr, 1+3+1+1, 3+2+1);
  cairo_line_to(cr, 1+3+1+9, 3+2+9);
  cairo_stroke(cr);
  cairo_move_to(cr, 1+3+1+1, 3+2+9);
  cairo_line_to(cr, 1+3+1+9, 3+2+1);  
  cairo_stroke(cr);
  
  printf("draw_closebutton, frame.closebutton = %d\n", window);  
  XMapWindow(display, window);  
  XFlush(display);
  cairo_surface_destroy(background);
  cairo_destroy (cr);
}

void draw_frame(Display* display, struct Frame frame) {
  Visual* colours = XDefaultVisual(display, DefaultScreen(display));
  
  cairo_surface_t *background = cairo_xlib_surface_create(display, frame.frame, colours, frame.w, frame.h);
  cairo_t *cr = cairo_create (background);

  XMoveWindow(display, frame.closebutton, frame.w-20-8-1, 4);
  
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
  
  //draw selection spot indicator
  cairo_set_source_rgba(cr, SHADOW);
  cairo_arc(cr, 16, 14.5, 6, 0, 2* M_PI);
  cairo_fill(cr);
  cairo_set_source_rgba(cr, SPOT);
  cairo_arc(cr, 15, 13.5, 6, 0, 2* M_PI);
  cairo_fill_preserve(cr);
  cairo_set_line_width(cr, 2);
  cairo_set_source_rgba(cr, SPOT_EDGE);
  cairo_stroke(cr);
  cairo_set_line_width(cr, 1);  
  
  char *wm_name = NULL;
  printf("draw_frame, frame.window = %d\n", frame.window);
  XFetchName(display, frame.window, &wm_name);
  if(wm_name != NULL) {
    //this needs to be in a mask so that the text gets truncated  
    //draw text shadow
    cairo_set_source_rgba(cr, SHADOW);
    cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14.5); 
    cairo_move_to(cr, 27, 20);
    cairo_show_text(cr, wm_name);
    //draw text    
    cairo_set_source_rgba(cr, TEXT);
    cairo_move_to(cr, 26, 19);  
    cairo_show_text(cr, wm_name);
    XFree(wm_name);
    //end mask
  }
  
  //draw inner frame
  cairo_set_source_rgba(cr, BORDER);
  cairo_rectangle(cr, 5, 27, frame.w-10, frame.h-32);
  cairo_fill(cr);
 
  //draw an (probably unneeded) inner background
  cairo_set_source_rgba(cr, BACKGROUND);
  cairo_rectangle(cr, 6, 28, frame.w-12, frame.h-34);
  cairo_fill(cr);
  draw_closebutton(display, frame.closebutton);
  printf("draw_frame, frame.frame = %d\n", frame.frame);
  XMapWindow(display, frame.frame);
  printf("draw_frame, frame.window = %d\n", frame.window);  
  XMapWindow(display, frame.window);  
  XFlush(display);
  cairo_surface_destroy(background);
  cairo_destroy (cr);
}

int create_frame(Display* display, struct Framelist* frames, Window window) { 
  Window root = DefaultRootWindow(display);
  Screen* screen = DefaultScreenOfDisplay(display);
  //display is used to get screen width and height 
  
  if(frames->used == frames->max) {
    struct Frame* temp = NULL;
    temp = realloc(frames->list, 2*sizeof(struct Framelist)*frames->max);
    if(temp != NULL) frames->list = temp;
    else return -1;
  }
  
  //TODO: get details from WM hints if possible, these are defaults:
  struct Frame frame;
  frame.window = window;
  frame.w = MINWIDTH*3;
  frame.h = MINHEIGHT*3;
  frame.x = (XWidthOfScreen(screen) - frame.w); //RHS of the screen
  frame.y = 30; //below menubar
  frame.min_width = MINWIDTH;
  frame.min_height = MINHEIGHT;
  frame.max_width = XWidthOfScreen(screen);
  frame.max_height = XHeightOfScreen(screen);
  frame.frame =
    XCreateSimpleWindow(display, root, frame.x, frame.y, frame.w, frame.h, 0,
      WhitePixelOfScreen(screen), BlackPixelOfScreen(screen));
  printf("create_frame, frame.frame = %d\n", frame.frame);    
  printf("create_frame, frame.window = %d\n", frame.window);
  frame.closebutton =
    XCreateSimpleWindow(display, frame.frame, frame.w-20-8-1, 4, 20, 20, 0,
      WhitePixelOfScreen(screen), BlackPixelOfScreen(screen));
  printf("create_frame, frame.closebutton = %d\n", frame.closebutton);      
  
  XSetWindowBorderWidth(display, frame.window, 0);
  XResizeWindow(display, window, frame.w - FRAME_HSPACE, frame.h - FRAME_VSPACE);
  XReparentWindow(display, window, frame.frame, FRAME_XSTART, FRAME_YSTART);
  
  //and the input only hotspots    
  //and set the mode    
  //need to establish how to place a new window
  XMapWindow(display, frame.frame);
  XMapWindow(display, frame.closebutton);
  XMapWindow(display, frame.window);
  
  frames->list[frames->used] = frame;
  frames->used++;
  return (frames->used - 1);
  
 
} 

