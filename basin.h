#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>  //This is used for the size hints structure
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <string.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/extensions/shape.h>
#include <X11/Xatom.h>

#define M_PI 3.14159265359

//#define SHARP_SYMBOLS //turns off anti-aliasing on symbols

#define MINWIDTH 240 
#define MINHEIGHT 80

/****THEME DERIVED CONSTANTS******/
#define LIGHT_EDGE_HEIGHT 7
//#define PULLDOWN_WIDTH 100
#define PULLDOWN_WIDTH 120

#define BUTTON_SIZE 20
#define V_SPACING 4
#define H_SPACING 4
#define TITLEBAR_HEIGHT (BUTTON_SIZE + (V_SPACING * 2))
#define EDGE_WIDTH 1
#define PIXMAP_SIZE 16
#define SPOT_SIZE 14
#define CORNER_GRIP_SIZE 32
#define MENUBAR_ITEM_WIDTH 80 /* this needs to be larger than PIXMAP_SIZE */
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

/*** Convenience Macros ***/
#define INTERSECTS_BEFORE(x1, w1, x2, w2) (x1 + w1 > x2  &&  x1 <= x2)
#define INTERSECTS_AFTER(x1, w1, x2, w2)  (x1 < x2 + w2  &&  x1 + w1 >= x2 + w2)
#define INTERSECTS(x1, w1, x2, w2)        (INTERSECTS_BEFORE(x1, w1, x2, w2) || INTERSECTS_AFTER(x1, w1, x2, w2))

enum Window_mode {
  floating,
  tiling,
  desktop,
  hidden,
  unset //this is required for the first change_frame_mode
};

enum Window_type {
  unknown,
  program,
  splash,
  dialog,
  modal_dialog,
  file,
  utility,
  status, 
  system_program
};

enum Window_state {
  fullscreen,
  demands_attention,
  lurking,       /* The lurking modes are used when the window attemps to tile and fails*/  
  none
};

struct Frame {
  char *window_name;

  int x,y,w,h;
  enum Window_mode mode;
  enum Window_type type; 
  enum Window_state state;
  int selected;
  int min_width, max_width;
  int min_height, max_height;
  Window transient; //the calling window of this dialog box - not structural
  
  
  Window window;    //the reparented window 
  Window frame, body, innerframe, titlebar;

  t_edge,
  l_edge,
  b_edge,
  r_edge,
  tl_corner,
  tr_corner,
  bl_corner,
  br_corner,
  selection_indicator,
  //start middle end to allow filling and changing of widgets width
  //state windows are all children of a single parent window.
  title_menu_lhs,
  title_menu_middle, //fill
  title_menu_rhs,    //includes arrow
  mode_dropdown_lhs, //no fill needed because contents are constant.
  mode_dropdown_rhs, //includes arrow
    

  Window backing;   //backing is the same dimensions as the framed window.  
                    //It is used so that the resize grips can cover the innerframe but still be below the framed window.

  /* InputOnly resize grips for the bottom left, top right etc. */
  Window l_grip, bl_grip, b_grip, br_grip, r_grip;

  /*The following structures are required because every widget has a window for each state */
  struct {  
    Window backing, close_hotspot;
    Window close_button_normal, close_button_pressed;  
  } close_button;
  
  struct {
    Window backing, hide_hotspot;
    Window hide_button_normal, hide_button_pressed;
  } hide_button;  //this is used in minimal mode
  
  struct {
    Window backing, mode_hotspot;
    Window floating_normal, floating_pressed, floating_deactivated
    ,tiling_normal,  tiling_pressed,   tiling_deactivated
    ,desktop_normal, desktop_pressed,  desktop_deactivated //desktop is sometimes visible
    ,hidden_normal,  hidden_pressed,   hidden_deactivated; //hidden won't be shown
  } mode_dropdown;
  
  struct { //this is the selection indicator and it's different states
    Window backing;
    Window indicator_normal, indicator_active;
  } selection_indicator;
  
  struct {
    Window frame    //frame is the outline, 
    , body          //body is the inner border, 
    , hotspot;      //hotspot is an input_only window to make the events easier to identify

    Window backing, title_normal, title_pressed, title_deactivated;
    
    struct {
      Window backing, arrow_normal, arrow_pressed, arrow_deactivated, no_arrow;
    } arrow;
        
    int width;
  } title_menu;
 
  struct {
    //these are the items for the menu
    Window backing;
    Window item_title, item_title_active, item_title_deactivated
    , item_title_hover, item_title_active_hover, item_title_deactivated_hover;
    
    struct {
      Window item_icon, item_icon_hover;
    } icon;

    int width;
  } menu_item;
  
  struct { //these is used during tiling resize operations.
    int new_position;
    int new_size;
  } indirect_resize;
};

struct Focus_list {
  unsigned int used, max;
  Window* list;
};

struct Frame_list {
  unsigned int used, max;
  struct Frame* list;
  struct Focus_list focus;
  
  char *workspace_name;
  
  struct {
    Window item_title
    , item_title_active
    , item_title_deactivated
    , item_title_hover
    , item_title_active_hover
    , item_title_deactivated_hover;
    
    Window backing;
    int width;
  } workspace_menu;
  
  Window virtual_desktop;
  Window title_menu;
};

struct Workspace_list {
  unsigned int used, max;
  struct Frame_list* list;
  Window workspace_menu;
};

struct Rectangle {
  int x,y,w,h;
};

struct Rectangle_list {
  unsigned int used, max;
  struct Rectangle *list;  
};

struct Mode_menu {
  Window frame, floating, tiling, desktop, blank, hidden;
  Window
  item_floating, item_floating_hover, item_floating_active, item_floating_active_hover, item_floating_deactivated,
  item_tiling,   item_tiling_hover,   item_tiling_active,   item_tiling_active_hover,   item_tiling_deactivated,
  item_desktop,  item_desktop_hover,  item_desktop_active,  item_desktop_active_hover,  item_desktop_deactivated,
  item_hidden,   item_hidden_hover,   item_hidden_active,   item_hidden_active_hover,   item_hidden_deactivated;
};

struct Cursors {
  Cursor normal, hand, grab, pressable
  , resize_h, resize_v, resize_tr_bl, resize_tl_br;
};

struct Pixmaps {
  Pixmap border_p, light_border_p, body_p, titlebar_background_p
  , selection_indicator_active_p, selection_indicator_normal_p
  , arrow_normal_p,             arrow_pressed_p,             arrow_deactivated_p
  , pulldown_floating_normal_p, pulldown_floating_pressed_p, pulldown_floating_deactivated_p
  , pulldown_tiling_normal_p,   pulldown_tiling_pressed_p,   pulldown_tiling_deactivated_p
  , pulldown_desktop_normal_p,  pulldown_desktop_pressed_p,  pulldown_desktop_deactivated_p
  , hide_button_normal_p,       hide_button_pressed_p,       hide_button_deactivated_p
  , close_button_normal_p,      close_button_pressed_p /*, close_button_deactivated_p*/ ;
  //maybe make a close button deactivated mode (but still allow it to work) as a transitionary measure
  //since dialog boxes and utility windows shouldn't need it (but might since compatibility isn't ensured)
};

struct Atoms {    
  Atom name                    // "WM_NAME"
  , normal_hints               // "WM_NORMAL_HINTS"

  //make sure this is the first Extended window manager hint
  , supported                  // "_NET_SUPPORTED"
  , supporting_wm_check        // "_NET_SUPPORTING_WM_CHECK"    
  , number_of_desktops         // "_NET_NUMBER_OF_DESKTOPS" //always 1
  , desktop_geometry           // "_NET_DESKTOP_GEOMETRY" //this is currently the same size as the screen
  
  , wm_full_placement          // "_NET_WM_FULL_PLACEMENT"
  , frame_extents              // "_NET_FRAME_EXTENTS"
  , wm_window_type             // "_NET_WM_WINDOW_TYPE"
  , wm_window_type_normal      // "_NET_WM_WINDOW_TYPE_NORMAL"
  , wm_window_type_dock        // "_NET_WM_WINDOW_TYPE_DOCK"
  , wm_window_type_desktop     // "_NET_WM_WINDOW_TYPE_DESKTOP"  
  , wm_window_type_splash      // "_NET_WM_WINDOW_TYPE_SPLASH"  
  , wm_window_type_dialog      // "_NET_WM_WINDOW_TYPE_DIALOG"  //can be transient
  , wm_window_type_utility     // "_NET_WM_WINDOW_TYPE_UTILITY" //can be transient
  , wm_state                   // "_NET_WM_STATE"
  , wm_state_demands_attention // "_NET_WM_STATE_DEMANDS_ATTENTION"
  , wm_state_modal             // "_NET_WM_STATE_MODAL"  //can be transient - for the specified window
  , wm_state_fullscreen;       // "_NET_WM_STATE_FULLSCREEN"

  //make sure this comes last  
};

  
/** This enum is passed as an argument to create_pixmap to select which one to draw **/
enum Main_pixmap {
  body,
  border,
  light_border,
  titlebar
};

enum Widget_pixmap {
  selection_indicator,

  arrow,

  hide, 
   
  close_button,

  pulldown_floating,
  pulldown_tiling,
  pulldown_desktop,

  program_menu,
  window_menu,
  options_menu,
  links_menu,
  tool_menu,

  item_floating,
  item_tiling,
  item_desktop,
  item_hidden
};

enum Widget_state {
  normal,
  pressed,
  deactivated,
  hover,
  active,
  active_hover
};

enum Title_pixmap {
  title_normal, 
  title_pressed,
  title_deactivated,
  
  item_title,
  item_title_active,
  item_title_deactivated,
  item_title_hover,
  item_title_active_hover,
  item_title_deactivated_hover 
};

struct Menubar {
  Window border, body;
  Window program_menu, window_menu, options_menu, links_menu, tool_menu; //these are the parents of their respective entries below:
  Window program_menu_normal, window_menu_normal, options_menu_normal, links_menu_normal, tool_menu_normal
  , program_menu_pressed, window_menu_pressed, options_menu_pressed, links_menu_pressed, tool_menu_pressed
  , program_menu_deactivated, window_menu_deactivated, options_menu_deactivated, links_menu_deactivated, tool_menu_deactivated;  
};
