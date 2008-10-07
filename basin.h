#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>  //This is used for the size hints structure
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <string.h>
#include <X11/Xcursor/Xcursor.h>

#define M_PI 3.14159265359

//#define SHARP_SYMBOLS //turns off anti-aliasing on symbols

#define MINWIDTH 300 
#define MINHEIGHT 60

#define FLOATING 1
#define TILING 2
#define SINKING 3
#define TRANSIENT 4

/****THEME DERIVED CONSTANTS******/
#define LIGHT_EDGE_HEIGHT 7
#define PULLDOWN_WIDTH 100

#define BUTTON_SIZE 20
#define V_SPACING 4
#define H_SPACING 4
#define TITLEBAR_HEIGHT BUTTON_SIZE + (V_SPACING * 2)
#define EDGE_WIDTH 1
#define PIXMAP_SIZE 16
#define SPOT_SIZE 14
#define CORNER_GRIP_SIZE 32

//this is the number of pixels the frame takes up from the top and bottom together of a window
#define FRAME_VSPACE (TITLEBAR_HEIGHT + EDGE_WIDTH*4 + H_SPACING)
//this is the number of pixels the frame takes up from either side of a window together
#define FRAME_HSPACE (EDGE_WIDTH*2 + H_SPACING)*2

#define TITLEBAR_USED_WIDTH (H_SPACING*5 + PULLDOWN_WIDTH + BUTTON_SIZE + EDGE_WIDTH*2 + BUTTON_SIZE)
//subtract this from the width of the window to find the max title width.
#define TITLE_MAX_WIDTH_DIFF (TITLEBAR_USED_WIDTH + EDGE_WIDTH*2 + BUTTON_SIZE)
#define TITLE_MAX_HEIGHT (BUTTON_SIZE - EDGE_WIDTH*4)

/***** Colours for cairo as rgba amounts between 0 and 1 *******/
#define SPOT            0.235294118, 0.549019608, 0.99, 1
#define SPOT_EDGE       0.235294118, 0.549019608, 0.99, 0.35
#define BACKGROUND      0.4, 0.4, 0.4, 1
#define TEXT            1.00, 1.00, 1.00, 1
#define TEXT_DEACTIVATED   0.6, 0.6, 0.6, 1
//#define BORDER          0.13, 0.13, 0.13, 1
//#define LIGHT_EDGE      0.34, 0.34, 0.34, 1
//#define BODY            0.27, 0.27, 0.27, 1

/*** Green ***/
#define BORDER          0.01, 0.35, 0.0, 1
#define LIGHT_EDGE      0.01, 0.78, 0.0, 1
#define BODY            0.01, 0.6, 0.0, 1

#define SHADOW          0.0, 0.0, 0.0, 1

struct Frame {
  Window window;
  char *window_name;
  char *application_name;

  int x,y,w,h;
  int mode; //FLOATING || TILING || SINKING
  int selected;
  int min_width, max_width;
  int min_height, max_height;
  
  Window frame, body, innerframe, titlebar, close_button, mode_pulldown, selection_indicator; 
  Window backing;   //backing is the same dimensions as the framed window.  It is used so that the resize grips can cover the innerframe but still be below the framed window.

  //because the title menu needs to change size regularly we re-use the same tricks as used in the structure of window frame itself
  struct {
    Window frame, //frame is the outline, body is the inner border, title has the background pixmap and arrow is the pulldownarrow, 
           body,
           title,
           arrow,
           hotspot; //hotspot is an input_only window to make the events easier to handle (rather than lots of logical ORs)

    Pixmap title_normal_p, title_pressed_p, title_deactivated_p; //this draws the background "bevel" and the text.
    int width; //this is the width of the title
  } title_menu;
  
  Window l_grip, bl_grip, b_grip, br_grip, r_grip;  //InputOnly resize grips  for the bottom left, top right etc.
};

struct Framelist {
  struct Frame* list;
  unsigned int max, used;
};

struct mouse_cursors {
  Cursor normal, hand, grab, pressable,
  resize_h, resize_v, resize_tr_bl, resize_tl_br;
};

struct frame_pixmaps {
  Pixmap border_p, light_border_p, body_p, titlebar_background_p,
         close_button_normal_p, close_button_pressed_p, close_button_deactivated_p,
         pulldown_floating_normal_p, pulldown_floating_pressed_p,
         pulldown_tiling_normal_p, pulldown_tiling_pressed_p,
         pulldown_deactivated_p,
         selection_p, 
         arrow_normal_p, arrow_pressed_p, arrow_deactivated_p,
         arrow_clipping_normal_p, arrow_clipping_pressed_p, arrow_clipping_deactivated_p;
};

/** This enum is passed as an argument to create_pixmap to select which one to draw **/
enum main_pixmap {
  background,
  body,
  border,
  light_border,
  titlebar,
  selection,
  close_button_normal,
  close_button_pressed,
  close_button_deactivated,
  pulldown_floating_normal,
  pulldown_floating_pressed,
  pulldown_deactivated, //normal is shown when the window is deactivated
  //the pulldowns deactivated mode is when the sinking mode is pressed

  pulldown_tiling_normal,
  pulldown_tiling_pressed,
  arrow_normal,
  arrow_clipping_normal,
  arrow_pressed,
  arrow_clipping_pressed,
  arrow_deactivated,
  arrow_clipping_deactivated
};

enum title_pixmap {
  title_normal, 
  title_pressed,
  title_deactivated
};
