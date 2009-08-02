#define DEFAULT_MENU_ITEM_WIDTH 90 /* This is the starting width for the mode menu */


enum Splash_background_tile {
  tile_splash_parent
};

enum Menubar_background_tile {
  tile_menubar_parent /* tile_menubar_parent must be last */
};

enum Popup_menu_background_tile {
  tile_popup_t_edge,
  tile_popup_l_edge,
  tile_popup_b_edge,
  tile_popup_r_edge,
  tile_popup_parent /*tile_popup_parent must be last */
};

enum Frame_background_tile {
  tile_t_edge,
  tile_titlebar,
  tile_l_edge,
  tile_b_edge,
  tile_r_edge,
  tile_title_menu_text, 
  tile_title_menu_icon, //TODO
  tile_frame_parent /* tile_frame_parent must be last */
};


char *strnadd(char *restrict s0, size_t limit, char *restrict s1, char *restrict s2);

static struct Widget_theme *create_component_theme(Display *display, char *type);

static void create_component_themes(Display *display, struct Themes *themes, char *theme_name);
static void create_font_themes(struct Themes *restrict themes);
static void swap_widget_theme(struct Widget_theme *from, struct Widget_theme *to);
static void swap_tiled_widget_themes(char *type, struct Widget_theme *themes, struct Widget_theme *tiles);

static void create_widget_theme_pixmap(Display *display, struct Widget_theme *widget_theme, cairo_surface_t **theme_images);
static void remove_widget_themes (Display *display, struct Widget_theme *themes, int length);

struct Themes *create_themes(Display *display, char *theme_name);

void remove_themes(Display *display, struct Themes *themes);

void create_text_background(Display *display, Window window, const char *restrict text
, const struct Font_theme *restrict font_theme, Pixmap background_p, int b_w, int b_h);

unsigned int get_text_width(Display * display, const char *title, struct Font_theme *font_theme);
