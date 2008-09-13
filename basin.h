#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#define M_PI 3.14159265359

//this amount in pixels * 3 are used when the top level size cannot be established
#define MINWIDTH 80 
#define MINHEIGHT 60

#define FLOATING 1
#define TILING 2
#define SINKING 3

struct Frame {
  Window window;
  unsigned int x,y,w,h;
  int mode; //FLOATING || TILING || SINKING
  int selected;
  int min_width, max_width;
  int min_height, max_height; 

  Window frame, pulldown, closebutton;
  cairo_surface_t *frame_s, *pulldown_s, *closebutton_s;  
  
  Window NW_grip, N_grip, NE_grip, E_grip, SE_grip, S_grip, SW_grip, W_grip;  //InputOnly extends into frame
};

struct Framelist {
  struct Frame* list;
  int max, used;
};
