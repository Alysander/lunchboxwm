#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/extensions/shape.h>
#include <X11/Xlib.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/Xatom.h>

#include "xcheck.h"
#include "basin.h"
#include "menus.h"
#include "theme.h"

/******************* 
Prec:  display is valid, themes is valid, cursors is valid.
Post:  Menubar created and mapped.
Desc:  This function uses the menubar member of the themes pointer to generate all the windows 
       that comprise the menubar with appropriate pixmap backgrounds.
todo:  Text needs to be drawn on the theme pixmaps, rater than having text on the menubar.
todo:  Possibly have an extra parameter that determines whether the menubar is a popup menu and use the shape
       extention.
********************/
void create_menubar(Display *display, struct Menubar *menubar, struct Themes *themes, struct Cursors *cursors) {
  /* create new window structure, using theme pixmaps */
  XSetWindowAttributes set_attributes;
  Window root = DefaultRootWindow(display);
  Screen* screen = DefaultScreenOfDisplay(display);
  int black = BlackPixelOfScreen(screen);
  unsigned int spacing;

  spacing = (XWidthOfScreen(screen) - themes->menubar[program_menu].w )/4;
  
  menubar->widgets[menubar_parent].widget = XCreateSimpleWindow(display, root
  , 0, XHeightOfScreen(screen) - themes->menubar[menubar_parent].h, XWidthOfScreen(screen)
  , themes->menubar[menubar_parent].h, 0, black, black);
  
  xcheck_setpixmap(display, menubar->widgets[menubar_parent].widget
  , themes->menubar[menubar_parent].state_p[normal]);
  
  for(int i = 0; i < menubar_parent; i++) {
    menubar->widgets[i].widget = XCreateSimpleWindow(display, menubar->widgets[menubar_parent].widget
    , spacing*i, 0, themes->menubar[i].w, themes->menubar[i].h, 0, black, black);
    XSelectInput(display, menubar->widgets[i].widget,  Button1MotionMask | ButtonPressMask | ButtonReleaseMask);

    for(int j = 0; j <= inactive; j++) {
      if(themes->menubar[i].state_p[j]) {
        menubar->widgets[i].state[j] = XCreateSimpleWindow(display, menubar->widgets[i].widget
        , 0, 0, themes->menubar[i].w, themes->menubar[i].h, 0, black, black);
        
        xcheck_setpixmap(display, menubar->widgets[i].state[j], themes->menubar[i].state_p[j]);
        xcheck_map(display, menubar->widgets[i].state[j]);
      }
      else printf("Warning:  Skipping state pixmap\n");
    }
    xcheck_map(display, menubar->widgets[i].widget);
  }

  set_attributes.override_redirect = True; 
  
  XChangeWindowAttributes(display, menubar->widgets[menubar_parent].widget
  , CWOverrideRedirect, &set_attributes);

  /* Show initial state. */
  xcheck_raisewin(display, menubar->widgets[program_menu].state[normal]);
  xcheck_raisewin(display, menubar->widgets[window_menu].state[normal]);
  XFlush(display);
  
  /* Show everything */
  XMapWindow(display, menubar->widgets[menubar_parent].widget);
  XFlush(display);
}

/*******************
Prec: display is valid, themes is valid, cursors is valid.
      all widgets are zero'd.
      inner_width and inner_height members of menu are initialized to a nonzero value.
      themes is valid and has loaded at least a background for the popup_menu_parent. 
Post: Menu with borders and background but no items is created but not mapped.
Desc: This function is used to create a blank and generic menu.  Items must be added by caller.
********************/
void create_popup_menu(Display *display, struct Popup_menu *menu, struct Themes *themes, struct Cursors *cursors) {

  XSetWindowAttributes set_attributes;
  Window root = DefaultRootWindow(display);
  Screen* screen = DefaultScreenOfDisplay(display);
  int black = BlackPixelOfScreen(screen);
  
  const int width = menu->inner_width + themes->popup_menu[popup_l_edge].w + themes->popup_menu[popup_r_edge].w;
  const int height = menu->inner_height + themes->popup_menu[popup_t_edge].h + themes->popup_menu[popup_b_edge].h;
  
  menu->widgets[popup_menu_parent].widget = XCreateSimpleWindow(display, root
  , 0, 0
  , width, height, 0, black, black);

  XSetWindowBackgroundPixmap(display, menu->widgets[popup_menu_parent].widget
  , themes->popup_menu[popup_menu_parent].state_p[normal]);
   
  //Currently, this is the "base" of the popup menu.
  //A similar loop will be needed for the actual menu items but not in this function
  for(int i = popup_t_edge; i < popup_menu_parent; i++) { //popup_menu_parent already done

    int x = themes->popup_menu[i].x;
    int y = themes->popup_menu[i].y;
    int w = themes->popup_menu[i].w;
    int h = themes->popup_menu[i].h;
    
    if(x < 0)  x += width;
    if(y < 0)  y += height; 
    if(w <= 0) w += width;
    if(h <= 0) h += height;
  
    menu->widgets[i].widget = XCreateSimpleWindow(display
    , menu->widgets[popup_menu_parent].widget
    , x, y, w, h, 0, black, black);
    
    for(int j = 0; j <= inactive; j++) {
      if(themes->popup_menu[i].state_p[j]) {
        menu->widgets[i].state[j] = XCreateSimpleWindow(display, menu->widgets[i].widget
        , 0, 0, w, h, 0, black, black);

        XSetWindowBackgroundPixmap(display, menu->widgets[i].state[j]
        , themes->popup_menu[i].state_p[j]);

        //TODO
        if(j == normal) XMapWindow(display, menu->widgets[i].state[j]);
      }
      else printf("Warning:  Skipping state pixmap\n");
    }

    XMapWindow(display, menu->widgets[i].widget);
  } 

  set_attributes.override_redirect = True; 
  XChangeWindowAttributes(display, menu->widgets[popup_menu_parent].widget, CWOverrideRedirect, &set_attributes);

//  XMapWindow(display, menu->widgets[popup_menu_parent].widget);
  XFlush(display);


}

void create_mode_menu(Display *display, struct Mode_menu *mode_menu
, struct Themes *themes, struct Cursors *cursors) {

  XSetWindowAttributes set_attributes;
  Window root = DefaultRootWindow(display);
  Screen* screen = DefaultScreenOfDisplay(display);
  int black = BlackPixelOfScreen(screen);

  //TODO get this from the theme somehow
  const int menu_item = medium_menu_item_mid; /*Change the mode menu item size here */

  mode_menu->menu.inner_width = DEFAULT_MENU_ITEM_WIDTH;
  mode_menu->menu.inner_height = themes->popup_menu[menu_item].h * (hidden + 1);
  
  create_popup_menu(display, &mode_menu->menu, themes, cursors);

  //Create the menu items
  for(int i = 0; i <= hidden; i++) {

    mode_menu->items[i].item = XCreateSimpleWindow(display, mode_menu->menu.widgets[popup_menu_parent].widget
    , themes->popup_menu[popup_l_edge].w, themes->popup_menu[popup_t_edge].h + themes->popup_menu[menu_item].h * i
    , mode_menu->menu.inner_width, themes->popup_menu[menu_item].h, 0, black, black);

    XSelectInput(display, mode_menu->items[i].item,  ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
    for(int j = 0; j <= inactive; j++) {
      if(themes->popup_menu[i].state_p[j]) {
        char *label = NULL;
        char Floating[] = "Floating";
        char Tiling[] = "Tiling";
        char Hidden[] = "Hidden";
        char Desktop[] = "Desktop";

        switch(i) {
          case floating: label = Floating;
          break;
          case tiling: label = Tiling;
          break;
          case hidden: label = Hidden;
          break;
          case desktop: label = Desktop;
          break;
        }

        mode_menu->items[i].state[j] = XCreateSimpleWindow(display, mode_menu->items[i].item
        , 0, 0, mode_menu->menu.inner_width, themes->popup_menu[menu_item].h, 0, black, black);

        create_text_background(display, mode_menu->items[i].state[j], label, &themes->small_font_theme[j]
        , themes->popup_menu[menu_item].state_p[j]
        , themes->popup_menu[menu_item].w, themes->popup_menu[menu_item].h);
        XMapWindow(display, mode_menu->items[i].state[j]);
      }
      else printf("Warning:  Skipping state pixmap\n");
    }
    XMapWindow(display, mode_menu->items[i].item);
  }

}

/********************
   This function only creates the frame and background for the workspace menu.
   When a workspace is created it must add itself to this menu.
   it should also check to see it it is bigger than the current width and enlarge it
   if required.
*********************/
void create_workspaces_menu(Display *display, struct Workspace_list *workspaces
, struct Themes *themes, struct Cursors *cursors) {

  const int menu_item = medium_menu_item_mid;  //TODO this must come from the theme
  
  workspaces->workspace_menu.inner_width = DEFAULT_MENU_ITEM_WIDTH; //TODO this must come from the theme
  workspaces->workspace_menu.inner_height = themes->popup_menu[menu_item].h;

  create_popup_menu(display, &workspaces->workspace_menu, themes, cursors);
}

/**************
The title menu is used for both the window menu and the dropdown list that appears at the top of the window.
Does it assume that titles have already been created? 
***************/
void create_title_menu(Display *display, struct Popup_menu *window_menu
, struct Themes *themes, struct Cursors *cursors) {

  const int menu_item = medium_menu_item_mid; //TODO this must come from the theme

  window_menu->inner_width = DEFAULT_MENU_ITEM_WIDTH;        //TODO this must come from the theme
  window_menu->inner_height = themes->popup_menu[menu_item].h;

  create_popup_menu(display, window_menu, themes, cursors);
}

void show_workspace_menu(Display *display, Window calling_widget, struct Workspace_list* workspaces
, int index, int x, int y, struct Themes *themes) {
  int max_length = 100;


  //TODO, do a loop and a resize things as in resize_frame

  //TOOD medium_menu_item_mid + lhs + rhs
  int menu_item = medium_menu_item_mid;

  for(int i = 0; i < workspaces->used; i++)  
  if(workspaces->list[i].workspace_menu.width > max_length) 
    max_length = workspaces->list[i].workspace_menu.width;

  //If this is the title menu show the title of the window that the menu appeared on in bold.
  for(int i = 0; i < workspaces->used; i++) {
    if(i == index) xcheck_raisewin(display, workspaces->list[i].workspace_menu.state[active]);
    else  xcheck_raisewin(display, workspaces->list[i].workspace_menu.state[normal]);
  }
  
  //Make all the menu items the same width and height.
  for(int i = 0; i < workspaces->used; i++) if(workspaces->list[i].workspace_menu.item) {
    //TODO menu_item_mid + lhs +rhs
    XMoveWindow(display,  workspaces->list[i].workspace_menu.item
    , themes->popup_menu[popup_l_edge].w, themes->popup_menu[popup_t_edge].h + themes->popup_menu[menu_item].h * i);
   // XResizeWindow(display, workspaces->list[i].workspace_menu.item, max_length, themes->popup_menu[menu_item].h);
  }

  workspaces->workspace_menu.inner_width = max_length;
  workspaces->workspace_menu.inner_height = themes->popup_menu[menu_item].h * workspaces->used;
  
  int width    = workspaces->workspace_menu.inner_width
   + themes->popup_menu[popup_l_edge].w + themes->popup_menu[popup_r_edge].w;
   
  int height   = workspaces->workspace_menu.inner_height 
  + themes->popup_menu[popup_t_edge].h + themes->popup_menu[popup_b_edge].h;
  
  resize_popup_menu(display, &workspaces->workspace_menu, themes);

  place_popup_menu(display, calling_widget, workspaces->workspace_menu.widgets[popup_menu_parent].widget
  , 0, y, themes);

}

/*********************
  This function pops-up the title menu in the titlebar or the window menu.
  the index parameter is the window which was clicked, so that it can be shown in bold.
  This function is also the window menu, which is done by setting the index to -1. 
**********************/

void show_title_menu(Display *display, struct Popup_menu *title_menu, Window calling_widget, struct Frame_list* frames
, int index, int x, int y, struct Themes *themes) {

  //TODO this does not consider what to do if a window is created when the menu is open
  int max_length = DEFAULT_MENU_ITEM_WIDTH;

  int height = themes->popup_menu[medium_menu_item_mid].h * frames->used 
  + themes->popup_menu[popup_t_edge].h + themes->popup_menu[popup_b_edge].h;

  for(int i = 0; i < frames->used; i++)  if(frames->list[i].menu.width > max_length) {
    max_length = frames->list[i].menu.width;
  }
  
  if(index == -1) for(int i = 0; i < frames->used; i++) {
//TODO check that the tiled window can actually fit on the screen
//    if(frames->list[i].mode == tiling) xcheck_raisewin(display, frames->list[i].menu.state[inactive]); 
    xcheck_raisewin(display, frames->list[i].menu.state[normal]);
  }
  //If this is the title menu show the title of the window that the menu appeared on in bold.
  else for(int i = 0; i < frames->used; i++) {
    if(i == index) xcheck_raisewin(display, frames->list[i].menu.state[active]);
    else xcheck_raisewin(display, frames->list[i].menu.state[normal]);
  }

  //Make all the menu items the same width and height.

  for(int i = 0; i < frames->used; i++) {
    XMoveWindow(display, frames->list[i].menu.item
    , themes->popup_menu[popup_l_edge].w, themes->popup_menu[popup_t_edge].h + themes->popup_menu[medium_menu_item_mid].h * i);
    XResizeWindow(display, frames->list[i].menu.item, max_length, themes->popup_menu[medium_menu_item_mid].h);
  }
  

  printf("Showing title menu at %d %d\n", x,y);

  title_menu->inner_width = max_length;
  title_menu->inner_height = themes->popup_menu[medium_menu_item_mid].h * frames->used;

  resize_popup_menu(display, title_menu, themes);
  place_popup_menu(display, calling_widget, title_menu->widgets[popup_menu_parent].widget, x, y, themes);
  XFlush(display);
}

void show_mode_menu(Display *display, Window calling_widget, struct Mode_menu *mode_menu
, struct Frame *active_frame, struct Themes *themes, int x, int y) {

  //these amounts are from create_mode_menu.
  //TODO menu_item_mid + lhs +rhs  
  int width = themes->popup_menu[popup_l_edge].w 
  + DEFAULT_MENU_ITEM_WIDTH + themes->popup_menu[popup_r_edge].w;

  int height = themes->popup_menu[medium_menu_item_mid].h * (hidden + 1) 
  + themes->popup_menu[popup_t_edge].h + themes->popup_menu[popup_b_edge].h;

  printf("Showing mode mode\n");
  if(active_frame->mode == floating) 
    xcheck_raisewin(display, mode_menu->items[floating].state[active]);
  else xcheck_raisewin(display, mode_menu->items[floating].state[normal]);

  if(active_frame->mode == tiling) 
    xcheck_raisewin(display, mode_menu->items[tiling].state[active]);
  else  xcheck_raisewin(display, mode_menu->items[tiling].state[normal]);

  if(active_frame->mode == desktop) 
    xcheck_raisewin(display, mode_menu->items[desktop].state[active]);
  else  xcheck_raisewin(display, mode_menu->items[desktop].state[normal]);

  xcheck_raisewin(display, mode_menu->items[hidden].state[normal]);

  XFlush(display);

  place_popup_menu(display, calling_widget, mode_menu->menu.widgets[popup_menu_parent].widget, x, y, themes);
}

void resize_popup_menu(Display *display, struct Popup_menu *menu, struct Themes *themes) {

  const int width  = menu->inner_width  + themes->popup_menu[popup_l_edge].w + themes->popup_menu[popup_r_edge].w;
  const int height = menu->inner_height + themes->popup_menu[popup_t_edge].h + themes->popup_menu[popup_b_edge].h;

  XResizeWindow(display, menu->widgets[popup_menu_parent].widget, width, height);

  //Currently, this is the "base" of the popup menu.
  //A similar loop will be needed for the actual menu items but not in this function
  for(int i = popup_t_edge; i < popup_menu_parent; i++) { //popup_menu_parent already done

    int x = themes->popup_menu[i].x;
    int y = themes->popup_menu[i].y;
    int w = themes->popup_menu[i].w;
    int h = themes->popup_menu[i].h;

    //only move or resize those which are dependent on the width.
    if(x < 0  ||  y < 0  ||  w <= 0  ||  h <= 0) {
      if(x < 0)  x += width;
      if(y < 0)  y += height; 
      if(w <= 0) w += width;
      if(h <= 0) h += height;

      XMoveResizeWindow(display, menu->widgets[i].widget, x, y, w, h);

      for(int j = 0; j <= inactive; j++) {
        if(themes->popup_menu[i].state_p[j]) {
          XResizeWindow(display, menu->widgets[i].state[j], w, h);
        }
        else printf("Warning:  Skipping state pixmap\n");
      }
    }
  } 

  XFlush(display);
}

/*
This function places a popup menu either above or below a particular widget.
It is placed above the widget to prevent it going off the bottom of the screen.
The x,y needs to be supplied because the x,y of the calling widget will be relative to its parent
, not the screen.
*/
void place_popup_menu(Display *display, Window calling_widget, Window popup_menu
, int x, int y, struct Themes *themes) {
  Screen* screen = DefaultScreenOfDisplay(display);
  XWindowAttributes details;
  XWindowAttributes popup_details;

  XGetWindowAttributes(display, calling_widget, &details);
  XGetWindowAttributes(display, popup_menu, &popup_details);  
  
  int width  = popup_details.width;
  int height = popup_details.height;

  y += details.height;

  if(y + height > XHeightOfScreen(screen)) y = y - (details.height + height); //either side of the widget
  if(y < 0) y = XHeightOfScreen(screen) - height;  
  if(x + width > XWidthOfScreen(screen)) x = XWidthOfScreen(screen) - width;
  printf("width is %d\n", width);
  XMoveWindow(display, popup_menu, x, y);
  XRaiseWindow(display, popup_menu);
//  xcheck_raisewin(display, popup_menu);
  XMapWindow(display, popup_menu);
  XFlush(display);
  printf("placed popup\n");
}
