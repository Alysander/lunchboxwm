char *strnadd(char *restrict s0, char *restrict s1, char *restrict s2, size_t limit);

struct Themes *create_themes(Display *display, char *theme_name);

void remove_themes(Display *display, struct Themes *themes);

void create_text_background(Display *display, Window window, const char *restrict text
, const struct Font_theme *restrict font_theme, Pixmap background_p, int b_w, int b_h);

unsigned int get_text_width(Display * display, const char *title, struct Font_theme *font_theme);
