#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>  //This is used for the size hints structure
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <string.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/Xatom.h> //defines XA_ATOM, an atom that stores the text ATOM

#define M_PI 3.14159265359

//#define SHARP_SYMBOLS //turns off anti-aliasing on symbols

#define MINWIDTH 240 
#define MINHEIGHT 60

#define FLOATING 1
#define TILING 2
#define SINKING 3

/****THEME DERIVED CONSTANTS******/
#define LIGHT_EDGE_HEIGHT 7
#define PULLDOWN_WIDTH 100

#define BUTTON_SIZE 20
#define V_SPACING 4
#define H_SPACING 4
#define TITLEBAR_HEIGHT (BUTTON_SIZE + (V_SPACING * 2))
#define EDGE_WIDTH 1
#define PIXMAP_SIZE 16
#define SPOT_SIZE 14
#define CORNER_GRIP_SIZE 32
#define MENU_ITEM_HEIGHT 24
#define MENU_ITEM_VS_BUTTON_Y_DIFF 2

//this is the number of pixels the frame takes up from the top and bottom together of a window
#define FRAME_VSPACE (TITLEBAR_HEIGHT + EDGE_WIDTH*4 + H_SPACING)
//this is the number of pixels the frame takes up from either side of a window together
#define FRAME_HSPACE (EDGE_WIDTH*2 + H_SPACING)*2

#define TITLEBAR_USED_WIDTH (H_SPACING*5 + PULLDOWN_WIDTH + BUTTON_SIZE + EDGE_WIDTH*2 + BUTTON_SIZE)
#define TITLE_MAX_HEIGHT (BUTTON_SIZE - EDGE_WIDTH*4)
//subtract this from the width of the window to find the max title width.
#define TITLE_MAX_WIDTH_DIFF (TITLEBAR_USED_WIDTH + EDGE_WIDTH*2 + BUTTON_SIZE)
#define MENUBAR_HEIGHT (TITLEBAR_HEIGHT - V_SPACING + EDGE_WIDTH)
#define PUSH_PULL_RESIZE_MARGIN 1

/***** Colours for cairo as rgba amounts between 0 and 1 *******/
#define SPOT            0.235294118, 0.549019608, 0.99, 1
#define SPOT_EDGE       0.235294118, 0.549019608, 0.99, 0.35
#define BACKGROUND      0.4, 0.4, 0.4, 1
#define TEXT            1.00, 1.00, 1.00, 1
#define TEXT_DEACTIVATED   0.6, 0.6, 0.6, 1
#define SHADOW          0.0, 0.0, 0.0, 1

#define BORDER          0.13, 0.13, 0.13, 1
#define LIGHT_EDGE      0.34, 0.34, 0.34, 1
#define BODY            0.27, 0.27, 0.27, 1

/*** Green theme at Fraser's suggestion ***/
/*
#define BORDER          0.01, 0.35, 0.0, 1
#define LIGHT_EDGE      0.01, 0.78, 0.0, 1
#define BODY            0.01, 0.6, 0.0, 1
*/

#define INTERSECTS_BEFORE(x1, w1, x2, w2) (x1 + w1 > x2  &&  x1 <= x2)
#define INTERSECTS_AFTER(x1, w1, x2, w2)  (x1 < x2 + w2  &&  x1 + w1 >= x2 + w2)
#define INTERSECTS(x1, w1, x2, w2) (INTERSECTS_BEFORE(x1, w1, x2, w2) || INTERSECTS_AFTER(x1, w1, x2, w2))

/****
Workspaces
   - status windows are "global windows". They simply remain on the screen across workspaces.
   - file windows can be shared.  However, they are technically reopened in different workspaces.
     -  how is the "opening program" determined?
        - open in current program?

Dialog boxes should not have a title menu - this could too easily result in them being lost.
Splash screens should not have close button.

Program/workspace can be a variable in frame.  
(utility windows do and therefore can have the mode list too - but no close button)
   
****/


struct Frame {
  Window window;
  char *window_name;

  int x,y,w,h;
  int mode; //FLOATING || TILING || SINKING
  int selected;
  int min_width, max_width;
  int min_height, max_height;
  
  Window frame, body, innerframe, titlebar, close_button, mode_pulldown, selection_indicator;
  Window mode_hotspot, close_hotspot;
  Window backing;   //backing is the same dimensions as the framed window.  
                    //It is used so that the resize grips can cover the innerframe but still be below the framed window.
  struct {
    Window frame,   //frame is the outline, 
           body,    //body is the inner border, 
           title,   //title has the background pixmap and 
           arrow,   //arrow is the pulldownarrow on the title menu
           hotspot; //hotspot is an input_only window to make the events easier to identify

    //these pixmaps include the bevel, background and  text
    Pixmap title_normal_p,
           title_pressed_p,
           title_deactivated_p,

           item_title_p,
           item_title_hover_p;
    
    Window entry;
    int width; //this is the width of the individual title
  } title_menu;
 
  struct {
     int new_position;
     int new_size;
  } indirect_resize;
  
  //InputOnly resize grips  for the bottom left, top right etc.  
  Window l_grip, bl_grip, b_grip, br_grip, r_grip;
};

struct Framelist {
  unsigned int used, max;
  struct Frame* list;  
  Window title_menu;
};


struct rectangle {
  int x,y,w,h;
};

struct rectangle_list {
  unsigned int used, max;
  struct rectangle *list;  
};

struct mode_pulldown_list {
  Window frame, floating, tiling, sinking;
};

struct mouse_cursors {
  Cursor normal, hand, grab, pressable,
         resize_h, resize_v, resize_tr_bl, resize_tl_br;
};

struct frame_pixmaps {
  Pixmap border_p, light_border_p, body_p, titlebar_background_p,
         close_button_normal_p, close_button_pressed_p, close_button_deactivated_p,

         //these are have a textured background
         pulldown_floating_normal_p, pulldown_floating_pressed_p,
         pulldown_tiling_normal_p, pulldown_tiling_pressed_p,
  
         pulldown_deactivated_p,

         //these don't have a textured background
         item_floating_p, item_floating_hover_p, 
         item_sinking_p, item_sinking_hover_p, 
         item_tiling_p, item_tiling_hover_p, 
         
         selection_p, 
         arrow_normal_p, arrow_pressed_p, arrow_deactivated_p,
         arrow_clipping_normal_p, arrow_clipping_pressed_p, arrow_clipping_deactivated_p;
};

struct hints {
    
    Atom name,                // "WM_NAME"
    normal_hints,             // "WM_NORMAL_HINTS"

    //make sure this is the first Extended window manager hint
    supported,                // "_NET_SUPPORTED"
    supporting_wm_check,      // "_NET_SUPPORTING_WM_CHECK"    
    number_of_desktops,       // "_NET_NUMBER_OF_DESKTOPS" //always 1
    desktop_geometry,         // "_NET_DESKTOP_GEOMETRY" //this is currently the same size as the screen
    
    wm_full_placement,        // "_NET_WM_FULL_PLACEMENT"
    frame_extents,            // "_NET_FRAME_EXTENTS"
    wm_window_type,           // "_NET_WM_WINDOW_TYPE"
    wm_window_type_normal,    // "_NET_WM_WINDOW_TYPE_NORMAL"
    wm_window_type_dock,      // "_NET_WM_WINDOW_TYPE_DOCK"
    wm_window_type_splash,    // "_NET_WM_WINDOW_TYPE_SPLASH"  //no frame
    wm_window_type_dialog,    // "_NET_WM_WINDOW_TYPE_DIALOG"  //can be transient
    wm_window_type_utility,   // "_NET_WM_WINDOW_TYPE_UTILITY" //can be transient
    wm_state,                 // "_NET_WM_STATE"
    wm_state_fullscreen;      // "_NET_WM_STATE_FULLSCREEN"

    //make sure this comes last
    
};

  
/** This enum is passed as an argument to create_pixmap to select which one to draw **/
enum main_pixmap {
  background,
  body,
  border,
  light_border,
  titlebar,
  selection,
  close_button_normal,  close_button_pressed,  close_button_deactivated,
  pulldown_floating_normal,  pulldown_floating_pressed,  
  pulldown_tiling_normal, pulldown_tiling_pressed,
  //the pulldowns deactivated mode is when the sinking mode is pressed
  pulldown_deactivated, 
  
  //remove active modes 
  item_floating, item_floating_hover, 
  item_sinking, item_sinking_hover, 
  item_tiling, item_tiling_hover, 
  
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
  title_deactivated,

  item_title,
  item_title_hover,
  //no inactive as all available choices in the menu are valid
};
