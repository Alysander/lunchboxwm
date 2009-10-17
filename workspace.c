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
#include "frame.h"
#include "frame-actions.h"
#include "workspace.h"
#include "space.h"
#include "focus.h"

/*** Create Workspace ***/
//create_workspace
int 
create_frame_list(Display *display, struct Workspace_list* workspaces, char *workspace_name, struct Themes *themes, struct Cursors *cursors) {
  Window root = DefaultRootWindow(display);
  Screen* screen =  DefaultScreenOfDisplay(display);  
  int black = BlackPixelOfScreen(screen);
  XSetWindowAttributes attributes; 
  struct Frame_list *frames;

  if(workspaces->list == NULL) {
    workspaces->used = 0;
    workspaces->max = 16;
    workspaces->list = malloc(sizeof(struct Frame_list) * workspaces->max);
    if(workspaces->list == NULL) return -1;
  }
  else if(workspaces->used == workspaces->max) {
    //printf("reallocating, used %d, max%d\n", workspaces->used, workspaces->max);
    struct Frame_list* temp = NULL;
    temp = realloc(workspaces->list, sizeof(struct Frame_list) * workspaces->max * 2);
    if(temp != NULL) workspaces->list = temp;
    else {
      #ifdef SHOW_STARTUP
      printf("\nError: Not enough available memory\n");
      #endif    
      return -1;
    }
    workspaces->max *= 2;
  }

  //the frame list frames is the new workspace
  frames = &workspaces->list[workspaces->used];
  frames->workspace_name = workspace_name;

  //Create the background window
  frames->virtual_desktop = XCreateSimpleWindow(display, root, 0, 0
  , XWidthOfScreen(screen), XHeightOfScreen(screen), 0, black, black);
  attributes.cursor = cursors->normal;
  attributes.override_redirect = True;
  attributes.background_pixmap = ParentRelative;
  XChangeWindowAttributes(display, frames->virtual_desktop 
  , CWOverrideRedirect | CWBackPixmap | CWCursor , &attributes);
  XLowerWindow(display, frames->virtual_desktop);

  //TODO when a workspace is added to the list, it must also be resized to the width of that list.
  unsigned int width = workspaces->workspace_menu.inner_width;

  frames->workspace_menu.item = XCreateSimpleWindow(display
  , workspaces->workspace_menu.widgets[popup_menu_parent].widget
  , themes->popup_menu[popup_l_edge].w, themes->popup_menu[popup_t_edge].h
  , width, themes->popup_menu[menu_item_mid].h
  , 0, black, black);

  XSelectInput(display, frames->workspace_menu.item,  ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
  
  for(int i = 0; i <= inactive; i++) { //if(themes->popup_menu[menu_item].state_p[i])
    frames->workspace_menu.state[i] = XCreateSimpleWindow(display
    , frames->workspace_menu.item
    , 0, 0
    , XWidthOfScreen(screen), themes->popup_menu[menu_item_mid].h
    , 0, black, black);

    create_text_background(display, frames->workspace_menu.state[i], frames->workspace_name
    , &themes->font_theme[i], themes->popup_menu[menu_item_mid].state_p[i]
    , themes->popup_menu[menu_item_mid].w, themes->popup_menu[menu_item_mid].h);

    XMapWindow(display, frames->workspace_menu.state[i]);
  }

  XMapWindow(display, frames->workspace_menu.item);

  frames->workspace_menu.width = get_text_width(display, frames->workspace_name, &themes->font_theme[active]);

  //Create the frame_list
  frames->used = 0;
  frames->max  = 16;
  frames->list = malloc(sizeof(struct Frame) * frames->max);
  if(frames->list == NULL) {
    #ifdef SHOW_STARTUP
    printf("\nError: Not enough available memory\n");
    #endif
    return -1;
  }

  frames->focus.used = 0;
  frames->focus.max  = 8; //must be divisible by 2
  frames->focus.list = malloc(sizeof(struct Focus_list) * frames->focus.max); //ok if it fails.
  #ifdef SHOW_WORKSPACE
  printf("Created workspace %d\n", workspaces->used);
  #endif

  workspaces->used++;
  XSync(display, False);
  return workspaces->used - 1;
}

/*  This is called when the wm is exiting, it doesn't close the open windows. */
void 
remove_frame_list(Display *display, struct Workspace_list* workspaces, int index, struct Themes *themes) {

  if(index >= workspaces->used) return;
  struct Frame_list *frames = &workspaces->list[index];
  
  //close all the frames
  for(int k = frames->used - 1; k >= 0; k--) remove_frame(display, frames, k, themes);
  free(frames->list);
  if(frames->workspace_name != NULL) {
    XFree(frames->workspace_name);
    frames->workspace_name = NULL;
  }
  if(frames->focus.list !=  NULL) {
    free(frames->focus.list);
    frames->focus.list = NULL;
  }
  //remove the focus list
  //close the background window
  //keep the open workspaces in order
  workspaces->used--;
  XDestroyWindow(display, frames->virtual_desktop);
  //TODO figure out how to update the window menu.
  //XDestroyWindow(display, frames->title_menu);  title menu will be global.
  XDestroyWindow(display, frames->workspace_menu.item);

  for(int i = index ; i < workspaces->used; i++) workspaces->list[i] = workspaces->list[i + 1];
}

//returned pointer must be freed with XFree.
char *
load_program_name(Display* display, Window window) {
  XClassHint program_hint;
  if(XGetClassHint(display, window, &program_hint)) {
   // printf("res_name %s, res_class %s\n", program_hint.res_name, program_hint.res_class);
    if(program_hint.res_name != NULL) XFree(program_hint.res_name);    
    if(program_hint.res_class != NULL)
      return program_hint.res_class;
  }
  return NULL;
}

//TODO does the name actually need to be allocated with Xmalloc or something?
void 
make_default_program_name(Display *display, Window window, char *name) {
  XClassHint program_hint;
  program_hint.res_name = name;
  program_hint.res_class = name;
  XSetClassHint(display, window, &program_hint);
  XFlush(display);  
}


/* creates the workspace */
int 
add_frame_to_workspace(Display *display, struct Workspace_list *workspaces, Window framed_window, int *current_workspace
, struct Popup_menu *window_menu
, struct Seperators *seps
, struct Themes *themes, struct Cursors *cursors, struct Atoms *atoms) {
  char *program_name = load_program_name(display, framed_window);
  int k;
  int frame_index = -1;
  
  if(program_name == NULL) {
    #ifdef SHOW_MAP_REQUEST_EVENT
    printf("Warning, could not load program name for window %lu. ", framed_window);  
    printf("Creating default workspace\n");
    #endif
    make_default_program_name(display, framed_window, "Other Programs");
    program_name = load_program_name(display, framed_window);
  }
  
  if(program_name == NULL) return -1; //probably out of memory
  
  for(k = 0; k < workspaces->used; k++) {
    if(strcmp(program_name, workspaces->list[k].workspace_name) == 0) {
      XFree(program_name);
      break;
    }
  }
  if(k == workspaces->used) { //create_statup workspaces only creates a workspace if there is at least one
    k = create_frame_list(display, workspaces, program_name, themes, cursors);
    if(k < 0) {
      #ifdef SHOW_MAP_REQUEST_EVENT   
      printf("Error, could not create new workspace\n");
      #endif
      return -1;
    }
  }
  frame_index = create_frame(display, &workspaces->list[k], framed_window, window_menu, seps, themes, cursors, atoms);
  if(frame_index != -1) {
    check_new_frame_focus (display, &workspaces->list[k], frame_index);
    if(k == *current_workspace  &&  *current_workspace != -1) {
      //printf("Created and mapped window in workspace %d\n", k);

      XMapWindow(display, workspaces->list[k].list[frame_index].widgets[frame_parent].widget);
      XMapWindow(display, workspaces->list[k].list[frame_index].menu.item);
      if(workspaces->list[k].list[frame_index].selected != 0) {
        recover_focus(display, &workspaces->list[k], themes);
      }
      if(workspaces->list[k].list[frame_index].mode == tiling) {
        drop_frame(display, &workspaces->list[k], frame_index, themes);
      }
    }
    else if(k != *current_workspace  &&  *current_workspace != -1) {
      change_to_workspace(display, workspaces, current_workspace, k, themes);
    }
    
    XFlush(display);
  }
  else if(!workspaces->list[k].used) { //if the window wasn't created, and the workspace is now empty, remove the workspace
    remove_frame_list(display, workspaces, k, themes);
    return -1;
  }
  return k;
}

int 
create_startup_workspaces(Display *display, struct Workspace_list *workspaces
, int *current_workspace
, struct Seperators *seps
, struct Popup_menu *window_menu, struct Themes *themes, struct Cursors *cursors, struct Atoms *atoms) {

  unsigned int windows_length;
  Window root, parent, children, *windows;
  XWindowAttributes attributes;
  root = DefaultRootWindow(display);

  create_workspaces_menu(display, workspaces, themes, cursors);  

  XQueryTree(display, root, &parent, &children, &windows, &windows_length);  

  if(windows != NULL) for(unsigned int i = 0; i < windows_length; i++)  {
    XGetWindowAttributes(display, windows[i], &attributes);
    if(attributes.map_state == IsViewable && !attributes.override_redirect)  {

      add_frame_to_workspace(display, workspaces, windows[i], current_workspace, window_menu
      , seps, themes, cursors, atoms);
    }
  }
  XFree(windows);
  return 1;
}

/*
Prec: all parameters intitalized and allocated properly.
Desc: This function changes the user's workspace to the workspace at the specified index.  
      If a negative index is passed, it changes to a default workspace which is currently 0.
      If a negative index is passed but no workspace is open nothing happens.
      Generally, it is expected that at least one workspace is open.
      Windows from other workspaces are unmapped.
Post: The user's workspace has visibly changed.
*/
void 
change_to_workspace(Display *display, struct Workspace_list *workspaces, int *current_workspace, int index, struct Themes *themes) {

  if(index < workspaces->used) {
    struct Frame_list *frames;
    if(workspaces->used == 0) return; //TODO change index to UINT_MAX or something
    if(*current_workspace < workspaces->used  &&  *current_workspace >= 0) {
      frames = &workspaces->list[*current_workspace];
      XUnmapWindow(display, frames->virtual_desktop);
      for(int i = 0; i < frames->used; i++) {
        XUnmapWindow(display, frames->list[i].widgets[frame_parent].widget);
        XUnmapWindow(display, frames->list[i].menu.item);
      }
    }
    //if index == -1, change to default workspace
    if(index < 0  &&  workspaces->used > 0) index = 0;

    frames = &workspaces->list[index];
    XMapWindow(display, frames->virtual_desktop);
    for(int i = 0; i < frames->used; i++) { 
      if(frames->list[i].mode != hidden) XMapWindow(display, frames->list[i].widgets[frame_parent].widget); 
      XMapWindow(display, frames->list[i].menu.item);
      //TODO Map other "shared" window names here
    }
    // printf("changing focus to one in new workspace\n");
    recover_focus(display, frames, themes);
    XFlush(display);
    *current_workspace = index;
  }
  else *current_workspace = -1;
}
