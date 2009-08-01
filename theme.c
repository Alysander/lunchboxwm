
#include <stdio.h>

#include <string.h>
#include <pwd.h>
#include <unistd.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include "basin.h"
#include "theme.h"
#include "xcheck.h"

#define PATH_SIZE 400
/* strnadd concatinates s1 and s2 and writes the result into s0 provided the length of s1 and s2 
is less that the limit, which is usually defined as the length of s0.  If any of the passed strings are NULL
s0 is returned unmodified.  If the limit is less than the length of s2, s0 is returned unmodified. 
All strings must be NULL terminated and this function ensures that s0 will always be null terminated */
char *strnadd(char *restrict s0, size_t limit, char *restrict s1, char *restrict s2) {
  size_t length;
  if(!s0  ||  !s1  ||  !s2)  return s0;
  length = strlen(s1) + strlen(s2) + 1;
  if(length > limit) return s0;
  if(s0 != s1) strcpy(s0, s1);
  strcat(s0, s2);
  s0[length-1] = '\0';
  //printf("s0: %s\n", s0);
  return s0;
}

/* create_component_themes opens the theme in the theme folder with the name specified by theme_name.
It changes the current working directory before calling create_component_theme each window type.
If an error occurs when opening the theme a NULL pointer is returned. */
struct Themes *create_themes(Display *display, char *theme_name) {
  struct Themes *themes = NULL;
  struct passwd *wd = NULL;
  char *path = NULL;
  char *login_name = NULL;

  login_name = getlogin();  
  path = malloc(PATH_SIZE * sizeof(char));

  if(!path) return NULL;

  if(!login_name) { 
    fprintf(stderr, "Error: getlogin returned NULL\n");
    goto error;
  }

  wd = getpwnam(login_name);
  if(!wd) goto error;

  strnadd(path, 200, wd->pw_dir, "/.basin/themes/");
  strcat(path, theme_name);
  printf("The theme path is: %s\n", path);
  if(chdir(path)) {
    fprintf(stderr, "Error opening theme path: %s\n", path);
    goto error;
  }

  themes = calloc(1, sizeof(struct Themes));
  if(themes == NULL) {
    fprintf(stderr, "Error: insufficient memory\n");
    goto error;
  }

//TODO check if file exists, otherwise use program_frame theme
  themes->window_type[unknown]        = create_component_theme(display, "program_frame");

/*****
  themes->window_type[splash]         = create_component_theme(display, "program_frame");
  themes->window_type[file]           = create_component_theme(display, "program_frame");
  themes->window_type[program]        = create_component_theme(display, "program_frame");
  themes->window_type[dialog]         = create_component_theme(display, "program_frame");
  themes->window_type[modal_dialog]   = create_component_theme(display, "program_frame");
  themes->window_type[utility]        = create_component_theme(display, "program_frame"); 
  themes->window_type[status]         = create_component_theme(display, "program_frame");
  themes->window_type[system_program] = create_component_theme(display, "program_frame");
  themes->window_type[popup_menu]     = create_component_theme(display, "program_frame");
  themes->window_type[popup_menubar]  = create_component_theme(display, "program_frame");  
  themes->window_type[menubar]        = create_component_theme(display, "program_frame");
*****/
/****
TODO Verify that: 
t_edge                         x >= 0, y >= 0, w <=0, h>0
l_edge                         x >= 0, y >= 0, w > 0, h<=0
b_edge                         x > 0,  y < 0, w <= 0, h>0
r_edge                         x < 0,  y >= 0, w > 0, h <= 0
****/
  themes->popup_menu = create_component_theme(display, "popup_menu");
  themes->menubar = create_component_theme(display, "menubar");

  if(!themes->window_type[unknown]) goto error;
  if(!themes->popup_menu) goto error;
  if(!themes->menubar) goto error;

  free(path);
  create_font_themes(themes);
  return themes;

  error:
  free(path);
  if(themes) free(themes);
  return NULL;
}

static struct Widget_theme *create_component_theme(Display *display, char *type) {
  struct Widget_theme *themes = NULL;
  struct Widget_theme *tiles  = NULL;
  char *filename = NULL;
  FILE *regions = NULL;

  unsigned int nwidgets = 0;
  unsigned int ntiles = 0;
  cairo_surface_t *theme_images[inactive + 1];

  /*Check that the theme set name is valid */
  if(strcmp(type, "program_frame")
  && strcmp(type, "file_frame")
  && strcmp(type, "dialog_frame")
  && strcmp(type, "modal_dialog_frame")
  && strcmp(type, "utility_frame")
  && strcmp(type, "unknown_frame")
  && strcmp(type, "popup_menu")
  && strcmp(type, "popup_menubar")
  && strcmp(type, "splash")   
  && strcmp(type, "menubar"))  {
    fprintf(stderr, "Unknown theme component \"%s\" specified\n", type);
    return NULL;
  }

  /* Establish the number of widgets in set */
  if(strstr(type, "frame")) {
    nwidgets = frame_parent + 1;
    ntiles = tile_frame_parent + 1;
  }
  else if(strstr(type, "menubar")) {
    nwidgets = menubar_parent + 1; 
    ntiles =   tile_menubar_parent + 1;
  }
  else if(!strcmp(type, "popup_menu")) {
    nwidgets = popup_menu_parent + 1; 
    ntiles =   tile_popup_parent + 1;
  }
  //TODO splash ?

  /* Allocate memory and open files */
  themes = calloc(nwidgets, sizeof(struct Widget_theme));
  if(!themes)   goto error;

  tiles =  calloc(ntiles,   sizeof(struct Widget_theme));  
  if(!tiles)    goto error;

  filename = calloc(100, sizeof(char));
  if(!filename) goto error;

  regions = fopen(strnadd(filename, 50, type, "_regions"), "r");
  if(regions == NULL) {
    fprintf(stderr, "Error:  A required theme file \"%s_regions\" could not be accessed\n", type);
    goto error;
  }

  /***
  To add another tiled background the following steps need to be followed:
  1) Add item to enumerators.
  2) Add tile name in strcmp below.  This should be the widget's name prepended with "tile_".
  3) Add in appropriate swap_widget theme commands before and after the pixmaps have been created.
  ***/
  
  /* Read in entries from the theme's region file into the theme array. */
  while(!feof(regions)) {
    int returned = 0;
    unsigned int current_widget;
    unsigned int current_tile;
    char widget_name[60];
    int x,y,w,h;
    int was_tile = 0;
    returned = fscanf(regions, "%s %d %d %d %d\n", widget_name, &x, &y, &w, &h);
    if(returned != 5) {
      fprintf(stderr, "Error in theme format, required \"string int int int int\", which represent the widget name, x position, y position, width and height");
      goto error;
    }
    else fprintf(stderr, "Loading theme for %s\n", widget_name);
    if(strstr(type, "frame")) {    
      if(!strcmp(widget_name, "window"))                   current_widget = window;
      else if(!strcmp(widget_name, "t_edge"))              current_widget = t_edge;
      else if(!strcmp(widget_name, "l_edge"))              current_widget = l_edge;
      else if(!strcmp(widget_name, "b_edge"))              current_widget = b_edge;
      else if(!strcmp(widget_name, "r_edge"))              current_widget = r_edge;
      else if(!strcmp(widget_name, "tl_corner"))           current_widget = tl_corner;
      else if(!strcmp(widget_name, "tr_corner"))           current_widget = tr_corner;
      else if(!strcmp(widget_name, "bl_corner"))           current_widget = bl_corner;
      else if(!strcmp(widget_name, "br_corner"))           current_widget = br_corner;
      else if(!strcmp(widget_name, "selection_indicator")) current_widget = selection_indicator;
      else if(!strcmp(widget_name, "selection_indicator_hotspot")) current_widget = selection_indicator_hotspot;
      else if(!strcmp(widget_name, "title_menu_lhs"))      current_widget = title_menu_lhs;
//      else if(!strcmp(widget_name, "title_menu_icon"))     current_widget = title_menu_icon;
      else if(!strcmp(widget_name, "title_menu_text"))     current_widget = title_menu_text;
      else if(!strcmp(widget_name, "title_menu_rhs"))      current_widget = title_menu_rhs;
      else if(!strcmp(widget_name, "title_menu_hotspot"))  current_widget = title_menu_hotspot;
      else if(!strcmp(widget_name, "mode_dropdown_lhs_floating")) current_widget = mode_dropdown_lhs_floating;
      else if(!strcmp(widget_name, "mode_dropdown_lhs_tiling"))   current_widget = mode_dropdown_lhs_tiling;
      else if(!strcmp(widget_name, "mode_dropdown_lhs_desktop"))  current_widget = mode_dropdown_lhs_desktop;
      else if(!strcmp(widget_name, "mode_dropdown_rhs"))          current_widget = mode_dropdown_rhs;
      else if(!strcmp(widget_name, "mode_dropdown_hotspot"))      current_widget = mode_dropdown_hotspot;
      else if(!strcmp(widget_name, "close_button"))               current_widget = close_button;
      else if(!strcmp(widget_name, "close_button_hotspot"))       current_widget = close_button_hotspot ;
      else if(!strcmp(widget_name, "frame_parent"))               current_widget = frame_parent;
      else if(!strcmp(widget_name, "tile_t_edge")) {           was_tile = 1; current_tile = tile_t_edge;    }
      else if(!strcmp(widget_name, "tile_l_edge")) {           was_tile = 1; current_tile = tile_l_edge;    }
      else if(!strcmp(widget_name, "tile_b_edge")) {           was_tile = 1; current_tile = tile_b_edge;    }
      else if(!strcmp(widget_name, "tile_r_edge")) {           was_tile = 1; current_tile = tile_r_edge;    }
      else if(!strcmp(widget_name, "tile_title_menu_text")) {  was_tile = 1; current_tile = tile_title_menu_text; }
      else if(!strcmp(widget_name, "tile_frame_parent")) {     was_tile = 1; current_tile = tile_frame_parent;    }
      else goto name_error;
    }
    else if(strstr(type, "menubar")) {
      if(!strcmp(widget_name, "program_menu"))        current_widget = program_menu;
      else if(!strcmp(widget_name, "window_menu"))    current_widget = window_menu;
      else if(!strcmp(widget_name, "options_menu"))   current_widget = options_menu;
      else if(!strcmp(widget_name, "links_menu"))     current_widget = links_menu;
      else if(!strcmp(widget_name, "tool_menu"))      current_widget = tool_menu;
      else if(!strcmp(widget_name, "menubar_parent")) current_widget = menubar_parent;
      else if(!strcmp(widget_name, "tile_menubar_parent")) {was_tile = 1; current_tile = tile_menubar_parent;}
      else goto name_error;
    }
    else if(!strcmp(type, "popup_menu")) {
//      printf("%s x %d, y %d, w %d, h %d\n", widget_name, x, y, w, h);
           if(!strcmp(widget_name, "small_menu_item_lhs"))   current_widget = small_menu_item_lhs;
      else if(!strcmp(widget_name, "small_menu_item_mid"))   current_widget = small_menu_item_mid;
      else if(!strcmp(widget_name, "small_menu_item_rhs"))   current_widget = small_menu_item_rhs;
      else if(!strcmp(widget_name, "medium_menu_item_lhs"))  current_widget = medium_menu_item_lhs;
      else if(!strcmp(widget_name, "medium_menu_item_mid"))  current_widget = medium_menu_item_mid;
      else if(!strcmp(widget_name, "medium_menu_item_rhs"))  current_widget = medium_menu_item_rhs;
      else if(!strcmp(widget_name, "large_menu_item_lhs"))   current_widget = large_menu_item_lhs;
      else if(!strcmp(widget_name, "large_menu_item_mid"))   current_widget = large_menu_item_mid;
      else if(!strcmp(widget_name, "large_menu_item_rhs"))   current_widget = large_menu_item_rhs;
      else if(!strcmp(widget_name, "popup_t_edge"))      current_widget = popup_t_edge;
      else if(!strcmp(widget_name, "popup_l_edge"))      current_widget = popup_l_edge;
      else if(!strcmp(widget_name, "popup_b_edge"))      current_widget = popup_b_edge;
      else if(!strcmp(widget_name, "popup_r_edge"))      current_widget = popup_r_edge;
      else if(!strcmp(widget_name, "popup_tl_corner"))   current_widget = popup_tl_corner;
      else if(!strcmp(widget_name, "popup_tr_corner"))   current_widget = popup_tr_corner;
      else if(!strcmp(widget_name, "popup_bl_corner"))   current_widget = popup_bl_corner;
      else if(!strcmp(widget_name, "popup_br_corner"))   current_widget = popup_br_corner;
      else if(!strcmp(widget_name, "popup_menu_parent")) current_widget = popup_menu_parent;
      else if(!strcmp(widget_name, "tile_popup_t_edge")) { was_tile = 1; current_tile = tile_popup_t_edge; }
      else if(!strcmp(widget_name, "tile_popup_l_edge")) { was_tile = 1; current_tile = tile_popup_l_edge; }
      else if(!strcmp(widget_name, "tile_popup_b_edge")) { was_tile = 1; current_tile = tile_popup_b_edge; }
      else if(!strcmp(widget_name, "tile_popup_r_edge")) { was_tile = 1; current_tile = tile_popup_r_edge; }
      else if(!strcmp(widget_name, "tile_popup_parent")) { was_tile = 1; current_tile = tile_popup_parent; }
      else goto name_error;
    }
    
    if(strstr(widget_name, "tile_")) {
      tiles[current_tile].exists = 1;
      tiles[current_tile].x = x;
      tiles[current_tile].y = y;
      tiles[current_tile].w = w;
      tiles[current_tile].h = h;
    }
    else {
      if(strstr(widget_name, "_hotspot")) themes[current_widget].exists = -1;
      else themes[current_widget].exists = 1;
      themes[current_widget].x = x;
      themes[current_widget].y = y;
      themes[current_widget].w = w;
      themes[current_widget].h = h;
    }
    continue;
    name_error:
    fprintf(stderr, "Error loading theme - widget name \"\%s\" not recognized\n", widget_name);        
    goto error;
  }
  fclose(regions); regions = NULL;
  
  /* Load the different state image files */
  theme_images[normal]  
  = cairo_image_surface_create_from_png(strnadd(filename, 50, type, "_normal.png"));
  printf("filename %s\n", filename);
  theme_images[active]  
  = cairo_image_surface_create_from_png(strnadd(filename, 50, type, "_active.png"));
  theme_images[normal_hover] 
  = cairo_image_surface_create_from_png(strnadd(filename, 50, type, "_normal_hover.png"));
  theme_images[active_hover] 
  = cairo_image_surface_create_from_png(strnadd(filename, 50, type, "_active_hover.png"));
  theme_images[normal_focussed] 
  = cairo_image_surface_create_from_png(strnadd(filename, 50, type, "_normal_focussed.png"));
  theme_images[active_focussed] 
  = cairo_image_surface_create_from_png(strnadd(filename, 50, type, "_active_focussed.png"));
  theme_images[normal_focussed_hover] 
  = cairo_image_surface_create_from_png(strnadd(filename, 50, type, "_normal_focussed_hover.png"));
  theme_images[active_focussed_hover] 
  = cairo_image_surface_create_from_png(strnadd(filename, 50, type, "_active_focussed_hover.png"));
  theme_images[inactive] 
  = cairo_image_surface_create_from_png(strnadd(filename, 50, type, "_inactive.png"));

  //TODO perhaps check to make sure that they have the same dimensions
  for(int i = 0; i <= inactive; i++) {
    if(cairo_surface_status(theme_images[i]) == CAIRO_STATUS_FILE_NOT_FOUND) {
      if(i != normal  &&  i != active  &&  i != inactive) {  /* Focussed variations are optional */
        cairo_surface_destroy(theme_images[i]);
        theme_images[i] == NULL;
      }
      else {
        fprintf(stderr, "Error:  Image file for theme component %s - %d\n not found\n", type, i);
        goto error;
      }
    }
    else 
    if(cairo_surface_status(theme_images[i]) == CAIRO_STATUS_NO_MEMORY ) fprintf(stderr, "surface %d no memory\n", i);
    else 
    if(cairo_surface_status(theme_images[i]) == CAIRO_STATUS_READ_ERROR) fprintf(stderr, "surface %d read error\n", i);
  }

  //backup the position and size of the widget before replacing with the corresponding tiles region
  swap_tiled_widget_themes(type, themes, tiles);

  //create all the widget pixmaps
  for(int i = 0; i < nwidgets; i++) {
    create_widget_theme_pixmap(display, &themes[i], theme_images);
  }

/*  if(strstr(type, "frame")) {
    Window root = DefaultRootWindow(display); 
    int screen_number = DefaultScreen (display);
    Screen *screen    = DefaultScreenOfDisplay(display);
    Visual *colours   = DefaultVisual(display, screen_number);    
    int black = BlackPixelOfScreen(screen);	  
    Window temp = XCreateSimpleWindow(display, root
    , 20, 20
    , 6, 2, 0, black, black); 

    XSetWindowBackgroundPixmap(display, temp, themes[t_edge].state_p[normal]);
    XMapWindow(display, temp);
    XFlush(display);    
  }*/
  //save the widgets region data and cleanup the tiled background region data as it isn't required anymore.
  swap_tiled_widget_themes(type, themes, tiles);

  //destroy the images of the whole frame sine they've been copied into images for individual widgets.
  for(int i = 0; i <= inactive; i++) if(theme_images[i]) cairo_surface_destroy(theme_images[i]);

  free(tiles);
  free(filename);

  return themes;

  error:
  if(tiles)       free(tiles);
  if(regions)     fclose(regions);  
  if(filename)    free(filename);
  if(themes)      free(themes);
  for(int i = 0; i <= inactive; i++) if(theme_images[i]) cairo_surface_destroy(theme_images[i]);
  return NULL;
}

//This loads the various font settings that are used by functions that draw the text.
//eventually it will load these from a file.
static void create_font_themes(struct Themes *restrict themes) {

  struct Font_theme font_theme = { .font_name = "Sans", .size = 13.5, .r = 1, .g = 1, .b = 1, .a = 1
  , .x = 3, .y = 15, .slant = CAIRO_FONT_SLANT_NORMAL, .weight = CAIRO_FONT_WEIGHT_NORMAL };

  for(int i = 0; i <= inactive; i++) {
    memcpy(&themes->medium_font_theme[i], &font_theme, sizeof(struct Font_theme));
  }

  font_theme.size = 11.5;
  font_theme.y = 13;
  font_theme.x = 3;
  for(int i = 0; i <= inactive; i++) {
    memcpy(&themes->small_font_theme[i], &font_theme, sizeof(struct Font_theme));
  }

  font_theme.size = 14.5;
  font_theme.y = 15;
  font_theme.x = 3;
  for(int i = 0; i <= inactive; i++) {  
    memcpy(&themes->large_font_theme[i], &font_theme, sizeof(struct Font_theme));
  }

  themes->large_font_theme[active].weight = CAIRO_FONT_WEIGHT_BOLD;
  themes->large_font_theme[active_hover].weight = CAIRO_FONT_WEIGHT_BOLD;
  themes->large_font_theme[inactive].r = 0.17;
  themes->large_font_theme[inactive].g = 0.17;
  themes->large_font_theme[inactive].b = 0.17;


  themes->small_font_theme[active].weight = CAIRO_FONT_WEIGHT_BOLD;
  themes->small_font_theme[active_hover].weight = CAIRO_FONT_WEIGHT_BOLD;
  themes->small_font_theme[inactive].r = 0.17;
  themes->small_font_theme[inactive].g = 0.17;
  themes->small_font_theme[inactive].b = 0.17;


  themes->medium_font_theme[active].weight = CAIRO_FONT_WEIGHT_BOLD;
  themes->medium_font_theme[active_hover].weight = CAIRO_FONT_WEIGHT_BOLD;
  themes->medium_font_theme[inactive].r = 0.17;
  themes->medium_font_theme[inactive].g = 0.17;
  themes->medium_font_theme[inactive].b = 0.17;

}

/*This copies all the details about the widget theme, but not the pixmaps.
This is so that the region of tile can be used to create the pixmaps for a widget that itself has a different region */
static void swap_widget_theme(struct Widget_theme *from, struct Widget_theme *to) {
  struct Widget_theme original_region;
  /*Problem, this also copies the pixmaps.*/
  if(from->exists &&  to->exists) {
    original_region = *to;
    to->x = from->x;
    to->y = from->y;
    to->w = from->w;
    to->h = from->h;
    from->x = original_region.x;
    from->y = original_region.y;
    from->w = original_region.w;
    from->h = original_region.h;
  }
  else fprintf(stderr, "Warning: background tile missing during theme load.\n");
}

/*****
This function is used just before and after creating the the pixmaps for the widgets
because some widgets need a tiled image (or image subsection) that isn't
the same region that the widget itself will be on.  So the region the widget
will be at and the image itself need to be temporarily swapped.
******/
static void swap_tiled_widget_themes(char *type, struct Widget_theme *themes, struct Widget_theme *tiles) {
  if(strstr(type, "frame")) {
    swap_widget_theme(&tiles[tile_t_edge], &themes[t_edge]);
    swap_widget_theme(&tiles[tile_r_edge], &themes[r_edge]);
    swap_widget_theme(&tiles[tile_b_edge], &themes[b_edge]);
    swap_widget_theme(&tiles[tile_l_edge], &themes[l_edge]);
    swap_widget_theme(&tiles[tile_title_menu_text], &themes[title_menu_text]);
    swap_widget_theme(&tiles[tile_frame_parent],    &themes[frame_parent]);
  }
  else if(strstr(type, "menubar")) {
    swap_widget_theme(&tiles[tile_menubar_parent], &themes[menubar_parent]);
  }
  else if(!strcmp(type, "popup_menu")) {
    swap_widget_theme(&tiles[tile_popup_t_edge], &themes[popup_t_edge]);
    swap_widget_theme(&tiles[tile_popup_l_edge], &themes[popup_l_edge]);
    swap_widget_theme(&tiles[tile_popup_b_edge], &themes[popup_b_edge]);
    swap_widget_theme(&tiles[tile_popup_r_edge], &themes[popup_r_edge]);
    swap_widget_theme(&tiles[tile_popup_parent], &themes[popup_menu_parent]);
  }
}

static void create_widget_theme_pixmap(Display *display,  struct Widget_theme *widget_theme, cairo_surface_t **theme_images) {
  Window root = DefaultRootWindow(display); 
  int screen_number = DefaultScreen (display);
  Screen *screen    = DefaultScreenOfDisplay(display);
  Visual *colours   = DefaultVisual(display, screen_number);

  cairo_surface_t *surface;
  cairo_t *cr;

  Window temp;
  int x = widget_theme->x;
  int y = widget_theme->y;
  int w = widget_theme->w;
  int h = widget_theme->h;
  //TODO THIS IS ONLY CREATING IT FOR ONE STATE
  int surface_width = cairo_image_surface_get_width(theme_images[0]);
  int surface_height = cairo_image_surface_get_height(theme_images[0]);

  if(widget_theme->exists <= 0) return; //the exists variable is -1 for hotspots
  //TODO get the size of the window from somewhere else
  if(x < 0)  x +=  surface_width;
  if(y < 0)  y +=  surface_height; 
  if(w <= 0) w +=  surface_width;
  if(h <= 0) h +=  surface_height;

  if(x < 0
  || y < 0
  || w <= 0
  || h <= 0
  || w > surface_width
  || h > surface_height) return;

  for(int i = 0; i <= inactive; i++) if(theme_images[i]) {
    widget_theme->state_p[i] = XCreatePixmap(display, root, w, h, XDefaultDepth(display, screen_number));
    surface = cairo_xlib_surface_create(display, widget_theme->state_p[i], colours, w, h);
    cr = cairo_create(surface);

    //paint a default background
    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
    cairo_rectangle(cr, 0, 0, w, h);
    cairo_fill(cr);

    //paint the section of the image    
    cairo_set_source_surface(cr, theme_images[i], -x, -y);
    cairo_rectangle(cr, 0, 0, w, h);
    cairo_fill(cr);
    cairo_surface_flush(surface);
    cairo_destroy (cr);  
    cairo_surface_destroy(surface);
  }
}


/* This function frees the pixmaps in an array of widget_theme s.  length is the number of elements in the array. */
static void remove_widget_themes (Display *display, struct Widget_theme *themes, int length) {
  if(themes != NULL) {
    for(int i = 0; i < length; i++)
    if(themes[i].exists) {
      for(int j = 0; j <= inactive; j++) if(themes[i].state_p[j]) XFreePixmap(display, themes[i].state_p[j]);
    }
    free(themes);
  }
}


/* This is dependent on struct Themes, so if that changes make corresponding updates here */
void remove_themes(Display *display, struct Themes *themes) {
  for(int i = 0; i <= menubar; i++)  remove_widget_themes(display, themes->window_type[i], frame_parent + 1);
  remove_widget_themes(display, themes->popup_menu, popup_menu_parent + 1);
  remove_widget_themes(display, themes->menubar, menubar_parent + 1);
  XFlush(display);
}



/* It needs to know where on the pixmap to start drawing - no more default values. */
/* It will eventually need to have the font passed in from the theme. */
/* Need to consider the different displacement for each type. */

/****************
Preconditions:   Display is open, font_theme is not NULL, background_p is valid
Postconditions:  pixmap using supplied background with the text in the specified style set as background
                 to the window.
Description:     this function uses the supplied pixmap as the background and draw the supplied text over it.
*****************/

/* this doesn't use the themes struct so that the caller can more easily specify which pixmap to use */

void create_text_background(Display *display, Window window, const char *restrict text
, const struct Font_theme *restrict font_theme, Pixmap background_p, int b_w, int b_h) {

  Window root = DefaultRootWindow(display); 
  int screen_number = DefaultScreen (display);
  Screen* screen = DefaultScreenOfDisplay(display);
  Visual *colours =  DefaultVisual(display, screen_number);
  if(b_w <=0 ) return;
  if(b_h <= 0) return;
  if(!background_p || !font_theme) return;

  //printf("Creating text pixmap %s\n", text);

  unsigned int width = XWidthOfScreen(screen);
  unsigned int height = b_h;  
  Pixmap pixmap = XCreatePixmap(display, root, width, height, XDefaultDepth(display, screen_number));
  cairo_surface_t *surface = cairo_xlib_surface_create(display, pixmap, colours,  width, height);
  cairo_surface_t *background = cairo_xlib_surface_create(display, background_p, colours, b_w, b_h);
  cairo_t *cr = cairo_create(surface);

  cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 1);
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_fill(cr);

  cairo_pattern_t *background_pattern = cairo_pattern_create_for_surface(background);
  cairo_pattern_set_extend(background_pattern, CAIRO_EXTEND_REPEAT);
  cairo_set_source(cr, background_pattern);
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_fill(cr);
  cairo_surface_flush(surface);
  cairo_surface_destroy(background);

  cairo_select_font_face(cr, font_theme->font_name, font_theme->slant, font_theme->weight);
  cairo_set_font_size(cr, font_theme->size);
  cairo_set_source_rgba(cr, font_theme->r, font_theme->g, font_theme->b, font_theme->a);
  cairo_move_to(cr, font_theme->x, font_theme->y);
  cairo_show_text(cr, text);
  cairo_destroy (cr);  
  cairo_surface_destroy(surface);
  XFlush(display);
  XSetWindowBackgroundPixmap(display, window, pixmap);
  XSync(display, False); 
  XFreePixmap(display, pixmap);
  XFlush(display);

}

//TODO, specify face and size.
//also specify weight?
/*
This is called when the workspace menu is called and when the window menu is shown
, it should be done whenever an item is added

int get_title_width(Display* display, const char *title, char *face_name, float size) {
  Window root = DefaultRootWindow(display); 
  int screen_number = DefaultScreen (display);
  Screen* screen = DefaultScreenOfDisplay(display);
  Visual *colours =  DefaultVisual(display, screen_number);
  Window temp;
  cairo_surface_t *surface;
  cairo_t *cr;
  cairo_text_extents_t extents;

  temp = XCreateSimpleWindow(display, root, 0, 0, PIXMAP_SIZE, PIXMAP_SIZE, 0, WhitePixelOfScreen(screen), BlackPixelOfScreen(screen));
  XFlush(display);
  surface = cairo_xlib_surface_create(display, temp, colours,  PIXMAP_SIZE, PIXMAP_SIZE);
  cr = cairo_create(surface);

  cairo_select_font_face (cr, face, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, size); 
  cairo_text_extents(cr, title, &extents);

  XDestroyWindow(display, temp); 
  XFlush(display);
  cairo_destroy (cr);  
  cairo_surface_destroy(surface);

  if(extents.x_bearing < 0) extents.x_advance -=  extents.x_bearing;

  return (int)extents.x_advance + 1; //have a extra pixel at the RHS just in case
}


Pixmap create_title_pixmap(Display* display, const char* title, enum Title_pixmap type) {
};
*/

