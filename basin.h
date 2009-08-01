#include <cairo/cairo.h> //because the Font_theme struct uses cairo types
#include <cairo/cairo-xlib.h>

#define M_PI 3.14159265359

/*** Convenience Macros ***/

#define PIXMAP_SIZE 16
#define MINWIDTH 240 
#define MINHEIGHT 80

/****THEME DERIVED CONSTANTS******/
#define LIGHT_EDGE_HEIGHT 7
//#define PULLDOWN_WIDTH 100
#define PULLDOWN_WIDTH 120

#define BUTTON_SIZE 20
#define V_SPACING 3
#define H_SPACING 3
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

#define MENUBAR_HEIGHT (TITLEBAR_HEIGHT - V_SPACING + EDGE_WIDTH)


enum Splash_widget {
  splash_parent
};

enum Splash_background_tile {
  tile_splash_parent
};

enum Menubar_widget {
  program_menu,
  window_menu,
  options_menu,
  links_menu,
  tool_menu,
  menubar_parent      /* menubar_parent must be last      */
}; 

enum Menubar_background_tile {
  tile_menubar_parent /* tile_menubar_parent must be last */
};

enum Popup_menu_widget {
  small_menu_item_lhs,
  small_menu_item_mid,  /* the middle will be tiled for wider or thinner popups */
  small_menu_item_rhs,
  medium_menu_item_lhs,
  medium_menu_item_mid, /* the middle will be tiled for wider or thinner popups */
  medium_menu_item_rhs,
  large_menu_item_lhs,
  large_menu_item_mid,  /* the middle will be tiled for wider or thinner popups */
  large_menu_item_rhs,  
  popup_t_edge,
  popup_l_edge,
  popup_b_edge,
  popup_r_edge,
  popup_tl_corner,
  popup_tr_corner,
  popup_bl_corner,
  popup_br_corner,    
  popup_menu_parent /* popup_menu_parent must be last */
};

enum Popup_menu_background_tile {
  tile_popup_t_edge,
  tile_popup_l_edge,
  tile_popup_b_edge,
  tile_popup_r_edge,
  tile_popup_parent /*tile_popup_parent must be last */
};

enum Frame_widget {
  window,
  t_edge,
  l_edge,
  b_edge,
  r_edge,
  tl_corner,
  tr_corner,
  bl_corner,
  br_corner,
  selection_indicator,
  selection_indicator_hotspot,  
  title_menu_lhs,
  title_menu_text,   //fill
  title_menu_rhs,    //includes arrow
  title_menu_hotspot,

  mode_dropdown_lhs_floating,
  mode_dropdown_lhs_tiling,
  mode_dropdown_lhs_desktop,

  mode_dropdown_rhs, //includes arrow
  mode_dropdown_hotspot,
  close_button,
  close_button_hotspot,

  frame_parent  /*frame parent must be last */
};

enum Frame_background_tile {
  tile_t_edge,
  tile_l_edge,
  tile_b_edge,
  tile_r_edge,
  tile_title_menu_text, 
  tile_title_menu_icon, //TODO
  tile_frame_parent /* tile_frame_parent must be last */
};

enum Window_mode {
  floating,
  tiling,
  desktop,
  hidden, /* add new modes above this line (which is hidden mode) */
  unset   /* must be after hidden */ /* This is required for the first change_frame_mode */
};

enum Window_state {
  fullscreen,
  demands_attention,
  lurking,       /* The lurking modes are used when the window attemps to tile and fails*/  
  none
};

/***********************
  Some clarification of widget state terminology.
  For a button, "active" is its pressed state.
  For a checkbox, "active" is its checked state.
  For a menu, "active" means that it is the already chosen item, or that the submenu is open.
  Therefore, the text from a drop down list should be look similar to the "active" element in the list.
  For example bold.
************************/

enum Widget_state {
  normal,
  active,  
  normal_hover,
  active_hover,  
  normal_focussed,
  active_focussed,
  normal_focussed_hover,
  active_focussed_hover,
  inactive,    //widget is unresponsive - must be last in this list
};

struct Widget {
  Window widget;
  /* The following windows can be raised to change the visible window state */
  Window state[inactive + 1];
  //Note that the reparented window is the "normal" state of the window widget
};

struct Widget_theme {
  char exists;
  int x,y,w,h; //values wrap around window.  w or h of zero is frame width or height.
  Pixmap state_p[inactive + 1];
};


struct Font_theme {
  char font_name[20];
  float size;
  float r,g,b,a;
  unsigned int x,y;
  cairo_font_slant_t slant;
  cairo_font_weight_t weight;
};


enum Window_type {
  unknown,
  splash,
  file,
  program,
  dialog,  
  modal_dialog,
  utility,
  status, 
  system_program,
  popup_menu,            
  popup_menubar,  
  menubar /* must be last */
};

struct Themes {
  struct Widget_theme *window_type[menubar + 1]; 
  struct Widget_theme *popup_menu; //this  may be implemented in the future
  struct Widget_theme *menubar;
  struct Font_theme small_font_theme[inactive + 1]; 
  struct Font_theme medium_font_theme[inactive + 1];
  struct Font_theme large_font_theme[inactive + 1]; 
};

struct Popup_menu {
  struct Widget widgets[popup_menu_parent + 1];
  unsigned int inner_width;
  unsigned int inner_height;
};

struct Menu_item {
  //these are the items for the title menu and window menu
  Window hotspot;
  Window item;
  Window state[inactive + 1];

  int width;
};

struct Mode_menu {
  struct Popup_menu menu;
  struct Menu_item items[hidden + 1];
};

struct Menubar {
  struct Widget widgets[menubar_parent + 1];
};

struct Frame {
  char *window_name;

  int x,y,w,h;
  int frame_hspace, frame_vspace; //amount used by the frame theme
  enum Window_mode mode;
  enum Window_type type; 
  enum Window_state state;
  int selected;
  int min_width, max_width;
  int min_height, max_height;
  int vspace; //dependent on the window type.
  int hspace;
  Window transient; //the calling window of this dialog box - not structural
  Window framed_window; //the window which is reparented.
  struct Widget widgets[frame_parent + 1];

  struct Menu_item menu; //this contains icons used in the window menu and the title menu

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
  int used, max;
  struct Frame* list;
  struct Focus_list focus;
  
  char *workspace_name;
  
  struct Menu_item workspace_menu; //this is a menu item
  
  Window virtual_desktop;
//  struct Popup_menu title_menu;
};

struct Workspace_list {
  int used, max; //this must be an int because index may intially be set as -1
  struct Frame_list* list;
  struct Popup_menu workspace_menu;
  //Window menu; this will be the popup_menu_parent 
};

struct Cursors {
  Cursor normal, hand, grab, pressable, resize_h, resize_v, resize_tr_bl, resize_tl_br;
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
