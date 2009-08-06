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
#include "space.h"
#include "frame-actions.h"

/*****
This function places moves the frame so that it is on-screen.
Or if it is off the screen, has it's bottom right hand corner visible (for dialog boxes).
It doesn't resize the frame.
******/
void 
check_frame_limits(Display *display, struct Frame *frame) {
  Screen* screen = DefaultScreenOfDisplay(display);  

  if(frame->w < frame->min_width)  frame->w = frame->min_width;
  else 
  if(frame->w > frame->max_width)  frame->w = frame->max_width;
  
  if(frame->h < frame->min_height) frame->h = frame->min_height;
  else 
  if(frame->h > frame->max_height) frame->h = frame->max_height;  

  if(frame->mode != desktop) {  
    if(frame->x < 0 ) frame->x = 0;
    if(frame->y < 0 ) frame->y = 0;
  
    if(frame->x + frame->w > XWidthOfScreen(screen)) 
      frame->x = XWidthOfScreen(screen) - frame->w;
    if(frame->y + frame->h > XHeightOfScreen(screen) - MENUBAR_HEIGHT) 
      frame->y = XHeightOfScreen(screen)- frame->h - MENUBAR_HEIGHT;
  }
}

/****** 
This function changes the frames mode to the desired mode. 
It shows the appropriate mode menu on the frame and resizes/moves the frame if appropriate.
If the mode is "tiling" it cannot ensure that the windows do not overlap because it doesn't have access to the frame list.
If the mode is set to "unset" then it sets the frame to whatever frame->mode currently is.
In the create frame function, this must be after create_frame_subwindows is called.
It is also after the get_frame_mode function, which sets the frame->mode directly before the create_frame_subwindows.
It should then be called with the mode "unset"
******/
void 
change_frame_mode(Display *display, struct Frame *frame, enum Window_mode mode, struct Themes *themes) {
  Screen* screen = DefaultScreenOfDisplay(display);
  if(frame->selected != 0) xcheck_raisewin(display, frame->widgets[selection_indicator].state[active]);
  else xcheck_raisewin(display, frame->widgets[selection_indicator].state[normal]);


  xcheck_raisewin(display, frame->widgets[close_button].state[normal]);
  xcheck_raisewin(display, frame->widgets[title_menu_lhs].state[normal]);
  xcheck_raisewin(display, frame->widgets[title_menu_text].state[normal]);
  xcheck_raisewin(display, frame->widgets[title_menu_rhs].state[normal]);

  if(mode == frame->mode  &&  mode == desktop) return; //This is causing dekstop windows to snap around needlessly.
  
  /**** Set the initial frame mode to whatever frame mode currently. This is done when the frame is created. ***/
  if(mode == unset) {
    mode = frame->mode;
  }
  /**** Undo state changes from current frame mode before settings new mode ****/
  else {
    if(frame->mode == hidden) XMapWindow(display, frame->widgets[frame_parent].widget);
    else 
    if(frame->mode == desktop) {
      check_frame_limits(display, frame); //bring the mode back
      resize_frame(display, frame, themes);
    }
  }
  
  XFlush(display);
  /*** Change the state of the frame to the new mode ***/
  if(mode == hidden) {
    XUnmapWindow(display, frame->widgets[frame_parent].widget);
    frame->mode = hidden;
  }
  else 
  if(mode == floating) {
    xcheck_raisewin(display, frame->widgets[mode_dropdown_lhs_floating].widget);
    xcheck_raisewin(display, frame->widgets[mode_dropdown_lhs_floating].state[normal]);
    xcheck_raisewin(display, frame->widgets[mode_dropdown_rhs].state[normal]);
    frame->mode = floating;
  }
  else
  if(mode == tiling) {
    xcheck_raisewin(display, frame->widgets[mode_dropdown_lhs_tiling].widget);
    xcheck_raisewin(display, frame->widgets[mode_dropdown_lhs_tiling].state[normal]);
    xcheck_raisewin(display, frame->widgets[mode_dropdown_rhs].state[normal]);
    frame->mode = tiling;
    //cannot drop frame here because it requires access to the whole frame list
  }

  else 
  if(mode == desktop) {
    xcheck_raisewin(display, frame->widgets[mode_dropdown_lhs_desktop].widget);
    xcheck_raisewin(display, frame->widgets[mode_dropdown_lhs_desktop].state[normal]);
    xcheck_raisewin(display, frame->widgets[mode_dropdown_rhs].state[normal]);
    frame->x = 0 - themes->window_type[frame->type][window].x; //(EDGE_WIDTH*2 + H_SPACING);  TODO use themes coordinates
    frame->y = 0 - themes->window_type[frame->type][window].y; //(TITLEBAR_HEIGHT + EDGE_WIDTH);

    if(frame->max_width >= XWidthOfScreen(screen)) 
      frame->w = XWidthOfScreen(screen) + frame->hspace;
    else 
      frame->w = frame->max_width;

    if(frame->max_height >= XHeightOfScreen(screen) - MENUBAR_HEIGHT)
      frame->h = XHeightOfScreen(screen) - MENUBAR_HEIGHT + frame->vspace;
    else 
      frame->h = frame->max_height;
    
    frame->mode = desktop;
    resize_frame(display, frame, themes);
  }
  xcheck_raisewin(display, frame->widgets[mode_dropdown_hotspot].widget);
  XFlush(display);
}

/*******
This function moves the dropped window to the nearest available space.
If the window has been enlarged so that it exceeds all available sizes,
a best-fit algorithm is used to determine the closest size.
If all spaces are smaller than the window's minimum size 
(which can only happen if the window's mode is being changed) the window
remains in it's previous mode. Otherwise the window's mode is changed to tiling.
********/
void 
drop_frame (Display *display, struct Frame_list *frames, int clicked_frame, struct Themes *themes) {
  
  struct Frame *frame = &frames->list[clicked_frame]; 
  struct Rectangle_list free_spaces = {0, 8, NULL};
  double min_displacement = -1; 
  int min = -1;
  int min_dx = 0;
  int min_dy = 0;
  //make the frame into a rectangle for calculating displacement function
  struct Rectangle window =  {frame->x
  , frame->y
  , frame->w
  , frame->h };
  
  #ifdef SHOW_FRAME_DROP  
  printf("Attempting to tile dropped window\n");
  #endif
  if(frame->mode == tiling) {
    frame->mode = hidden;
    free_spaces = get_free_screen_spaces (display, frames);
    frame->mode = tiling;
  }
  else free_spaces = get_free_screen_spaces (display, frames);

  if(free_spaces.list == NULL) return;
  
  for(unsigned int k = 0; k < free_spaces.used; k++) {
    double displacement = 0;
    int dx = 0;
    int dy = 0;
    displacement = calculate_displacement(window, free_spaces.list[k], &dx, &dy);
    #ifdef SHOW_FRAME_DROP  
    printf("Free space:space %d, x %d, y %d, w %d, h %d, distance %f\n", k
    , free_spaces.list[k].x, free_spaces.list[k].y
    , free_spaces.list[k].w, free_spaces.list[k].h, (float)displacement);
    #endif
    if(displacement >= 0  &&  (min_displacement == -1  ||  displacement < min_displacement)) {
      min_displacement = displacement;
      min = k;
      min_dx = dx;
      min_dy = dy;
    }
  }
  
  //the window is too large to fit in any current spaces.
  //find the space which is the closest size
  if(min == -1) { 
    #ifdef SHOW_FRAME_DROP  
    printf("move failed - finding the nearest size\n");
    #endif
    double w_proportion, h_proportion, current_fit;
    double best_fit = -1;
    int best_space = -1;
    for(unsigned int k = 0; k < free_spaces.used; k++) {
      if(free_spaces.list[k].w == 0
      || free_spaces.list[k].h == 0) {
        //printf("Error: FOUND ZERO AREA FREE SPACE\n");
        continue;
      }
      w_proportion = (double)frame->w / free_spaces.list[k].w;
      h_proportion = (double)frame->h / free_spaces.list[k].h;
      if(w_proportion > 1) w_proportion = 1;
      if(h_proportion > 1) h_proportion = 1;
      current_fit = w_proportion * h_proportion;
      if(current_fit > best_fit
      && free_spaces.list[k].w >= frame->min_width
      && free_spaces.list[k].h >= frame->min_height) {
        best_fit = current_fit;
        best_space = k;
      }
    }
    
    if(best_space != -1) {
      change_frame_mode(display, frame, tiling, themes);
      if(free_spaces.list[best_space].w < frame->w) {  
        frame->x = free_spaces.list[best_space].x;
        frame->w = free_spaces.list[best_space].w;
        if(frame->y + frame->h > free_spaces.list[best_space].y + free_spaces.list[best_space].h) 
          frame->y = free_spaces.list[best_space].y + free_spaces.list[best_space].h - frame->h;
        else if(frame->y < free_spaces.list[best_space].y)
          frame->y = free_spaces.list[best_space].y;
      }
      if(free_spaces.list[best_space].h < frame->h) {
        frame->y = free_spaces.list[best_space].y;
        frame->h = free_spaces.list[best_space].h; 
        if(frame->x + frame->w > free_spaces.list[best_space].x + free_spaces.list[best_space].w) 
          frame->x = free_spaces.list[best_space].x + free_spaces.list[best_space].w - frame->w;
        else if(frame->x < free_spaces.list[best_space].x)
          frame->x = free_spaces.list[best_space].x;
      }
      if(frame->w > frame->max_width) frame->w = frame->max_width;
      if(frame->h > frame->max_height) frame->h = frame->max_height;
      resize_frame(display, frame, themes);
    }
    else change_frame_mode(display, frame, frame->mode, themes);
  }
  else {
    #ifdef SHOW_FRAME_DROP  
    printf("Found min_dx %d, min_dy %d, distance %f\n", min_dx, min_dy, (float)min_displacement);
    #endif
    change_frame_mode(display, frame, tiling, themes);
    frame->x += min_dx;
    frame->y += min_dy;
    XMoveWindow(display, frame->widgets[frame_parent].widget, frame->x, frame->y);
    XFlush(display);
  }

  if(free_spaces.list != NULL) free(free_spaces.list);
  return;
}

/****
This function handles responding to the users click and drag on the resize grips of the window.
*****/
void 
resize_using_frame_grip (Display *display, struct Frame_list *frames, int clicked_frame
, int pointer_start_x, int pointer_start_y, int mouse_root_x, int mouse_root_y
, int r_edge_dx, int b_edge_dy, Window clicked_widget, struct Themes *themes) {
  
  Screen* screen = DefaultScreenOfDisplay(display);

  struct Frame *frame = &frames->list[clicked_frame];  

  int new_width = frame->w;
  int new_height = frame->h;
  int new_x = frame->x; 
  int new_y = frame->y; 
  /* precalculated potential values for x and y */ 
  /* This is done so that the they can be tested and altered in one place */

  int pot_x = mouse_root_x - pointer_start_x;
  int pot_y = mouse_root_y - pointer_start_y;
  if(frame->mode != desktop) {
    if(pot_x < 0) pot_x = 0;
    if(pot_y < 0) pot_y = 0;
  }
  //  printf("resize_nontiling_frame %s\n, x %d, y %d\n", frame->window_name, frame->x, frame->y);
  /* We need to consider more widgets */
  if(clicked_widget == frame->widgets[l_edge].widget) {
    new_x = pot_x;
    new_width = frame->w + (frame->x - new_x);
  }
  else if(clicked_widget == frame->widgets[t_edge].widget) {
    new_y = pot_y;
    new_height = frame->h + (frame->y - new_y);
  }
  else if(clicked_widget == frame->widgets[tl_corner].widget) {
    new_y = pot_y;
    new_height = frame->h + (frame->y - new_y);
    new_x = pot_x;
    new_width = frame->w + (frame->x - new_x);
  }
  else if(clicked_widget == frame->widgets[tr_corner].widget) {
    new_y = pot_y;
    new_height = frame->h + (frame->y - new_y);
    new_width = mouse_root_x - frame->x + r_edge_dx;    
  }
  else if(clicked_widget == frame->widgets[r_edge].widget) {
    new_width = mouse_root_x - frame->x + r_edge_dx;
  }
  else if(clicked_widget == frame->widgets[bl_corner].widget) {
    new_x = pot_x;
    new_width = frame->w + (frame->x - new_x);
    new_height = mouse_root_y - frame->y;
  }
  else if(clicked_widget == frame->widgets[br_corner].widget) {
    new_width = mouse_root_x - frame->x + r_edge_dx;
    new_height = mouse_root_y - frame->y + b_edge_dy;
  }
  else if(clicked_widget == frame->widgets[b_edge].widget) {
    new_height = mouse_root_y - frame->y + b_edge_dy;
  }

  //check that the frame is not outside its min or max sizes
  if(new_height < frame->min_height) new_height = frame->min_height;
  if(new_height > frame->max_height) new_height = frame->max_height;
  if(new_width < frame->min_width) new_width = frame->min_width;
  if(new_width > frame->max_width) new_width = frame->max_width;

  //undo movement if the height hasn't changed overall
  if(new_height == frame->h) new_y = frame->y;
  if(new_width == frame->w)  new_x = frame->x;
    
  if(frame->mode != desktop) {
    if(new_x + new_width > XWidthOfScreen(screen))
      new_width = XWidthOfScreen(screen) - new_x;
    if(new_y + new_height > XHeightOfScreen(screen) - MENUBAR_HEIGHT)
      new_height = XHeightOfScreen(screen)- new_y - MENUBAR_HEIGHT;
  }
  //commit height changes
  if(frame->mode == tiling) {
    if(new_height != frame->h) resize_tiling_frame(display, frames, clicked_frame, 'y', new_y, new_height, themes);
    if(new_width != frame->w)  resize_tiling_frame(display, frames, clicked_frame, 'x', new_x, new_width, themes);
  }
  else {  
    frame->x = new_x;  //for l_grip and bl_grip
    frame->y = new_y;  //in case top grip is added later
    frame->w = new_width;
    frame->h = new_height;    
  } 
  resize_frame(display, frame, themes);
  XFlush(display);
}


/******
This handles moving/resizing the window when the titlebar is dragged.  
It resizes windows that are pushed against the edge of the screen,
to sizes between the defined min and max.
*******/
void 
move_frame (Display *display, struct Frame *frame
, int *pointer_start_x, int *pointer_start_y, int mouse_root_x, int mouse_root_y
, int *resize_x_direction, int *resize_y_direction, struct Themes *themes) {
  
  //the pointer start variables may be updated as the window is squished against the LHS or top of the screen
  //because of the changing relative co-ordinates of the pointer on the window.
  
  //resize_x_direction - 1 means LHS, -1 means RHS , 0 means unset
  //resize_y_direction - 1 means top, -1 means bottom, 0 means unset
  // the resize_x/y_direction variables are used to identify when a squish resize has occured
  // and this is then used to decide when to stretch the window from the edge.

  Screen* screen = DefaultScreenOfDisplay(display);
  printf("Got a move frame\n");
  int new_width = 0;
  int new_height = 0;
  int new_x = mouse_root_x - *pointer_start_x;
  int new_y = mouse_root_y - *pointer_start_y;
  printf("move frame %s\n", frame->window_name);
  //do not attempt to resize if the window is larger than the screen
  if(frame->min_width <= XWidthOfScreen(screen)
  && frame->min_height <= XHeightOfScreen(screen) - MENUBAR_HEIGHT
  && frame->mode != desktop) {
  
    if((new_x + frame->w > XWidthOfScreen(screen)) //window moving off RHS
    || (*resize_x_direction == -1)) {  
      *resize_x_direction = -1;
      new_width = XWidthOfScreen(screen) - new_x;
    }
    
    if((new_x < 0) //window moving off LHS
    || (*resize_x_direction == 1)) { 
      *resize_x_direction = 1;
      new_width = frame->w + new_x;
      new_x = 0;
      *pointer_start_x = mouse_root_x;
    }

    if((new_y + frame->h > XHeightOfScreen(screen) - MENUBAR_HEIGHT) //window moving off the bottom
    || (*resize_y_direction == -1)) { 
      *resize_y_direction = -1;
      new_height = XHeightOfScreen(screen) - MENUBAR_HEIGHT - new_y;
    }
    
    if((new_y < 0) //window moving off the top of the screen
    || (*resize_y_direction == 1)) { 
      *resize_y_direction = 1;
      new_height = frame->h + new_y;
      new_y = 0;
      *pointer_start_y = mouse_root_y;
    }
    
    if(new_width != 0  &&  new_width < frame->min_width) {
      new_width = frame->min_width;
      if(*resize_x_direction == -1) new_x = XWidthOfScreen(screen) - frame->min_width;
      //don't move the window off the RHS if it has reached it's minimum size
      //LHS not considered because x has already been set to 0

    }
    
    if(new_height != 0  &&  new_height < frame->min_height) {
      new_height = frame->min_height;    
      //don't move the window off the bottom if it has reached it's minimum size
      //Top not considered because y has already been set to 0
      if(*resize_y_direction == -1) new_y = XHeightOfScreen(screen) - frame->min_height - MENUBAR_HEIGHT;
    }

    //limit resizes to max width
    if((new_width  != 0  &&  new_width  > frame->max_width) 
    || (new_height != 0  &&  new_height > frame->max_height)) {
      //these have been changed from zero to max values..
      if(new_width > frame->max_width) new_width = frame->max_width;
      if(new_height > frame->max_height) new_height = frame->max_height;
    } 
      
    //do not attempt to resize windows that cannot be resized
    if(frame->min_width == frame->max_width) *resize_x_direction = 0;
    if(frame->min_height == frame->max_height) *resize_y_direction = 0;
  } 
       
  if(new_width != 0  ||  new_height != 0) {   //resize window if required
    //allow movement in axis if it hasn't been resized
    frame->x = new_x;
    frame->y = new_y;
    if(new_width != 0) frame->w = new_width;
    
    if(new_height != 0) frame->h = new_height;
    //allow movement in axis if it hasn't been resized
    
    resize_frame(display, frame, themes);
    
  }
  else {
    //Moves the window to the specified location if there is no resizing going on.
    frame->x = new_x;
    frame->y = new_y;
    XMoveWindow(display, frame->widgets[frame_parent].widget, frame->x, frame->y);
  }
  XFlush(display);  
}
/*****

*****/
int 
replace_frame(Display *display, struct Frame *target, struct Frame *replacement
, struct Seperators *seps, struct Themes *themes) {
  XWindowChanges changes;
  XWindowChanges changes2;
  enum Window_mode mode;
  unsigned int mask = CWX | CWY | CWWidth | CWHeight;

  #ifdef SHOW_BUTTON_PRESS_EVENT
  printf("replace frame %s\n", replacement->window_name);
  #endif
  if(replacement->framed_window == target->framed_window) return 0;  //this can be chosen from the title menu
  if(target->w < replacement->min_width
  || target->h < replacement->min_height) {
    #ifdef SHOW_BUTTON_PRESS_EVENT
    printf("The requested window doesn't fit on the target window\n");
    #endif
    return 0;
  }
  if(replacement->mode != hidden && (target->min_width > replacement->w
  || target->min_height > replacement->h)) {
    #ifdef SHOW_BUTTON_PRESS_EVENT
    printf("The requested window doesn't fit on the target window\n");
    #endif
    return 0;  
  }
  
  changes.x = target->x;
  changes.y = target->y;
  
  if(target->w < replacement->max_width) changes.width = target->w;  //save info for request
  else changes.width = replacement->max_width;
      
  if(target->h < replacement->max_height) changes.height = target->h; 
  else changes.height = replacement->max_height;
  
  mode = replacement->mode;
  if(mode != hidden) {
    changes2.x = replacement->x;
    changes2.y = replacement->y;
    
    if(replacement->w < target->max_width) changes2.width = replacement->w;  //save info for request
    else changes2.width = target->max_width;
    
    if(replacement->h < target->max_height) changes2.height = replacement->h; 
    else changes2.height = target->max_height;
      
    target->x = changes2.x;
    target->y = changes2.y;
    target->w = changes2.width;
    target->h = changes2.height;  
    //printf("Target mode %d, x %d, y %d, w %d, h %d\n", target->mode, target->x, target->y, target->w, target->h);
    XConfigureWindow(display, target->widgets[frame_parent].widget, mask, &changes2);
  }
  
  replacement->x = changes.x;
  replacement->y = changes.y;
  replacement->w = changes.width;
  replacement->h = changes.height;
  //printf("Target mode %d, x %d, y %d, w %d, h %d\n", mode, replacement->x, replacement->y, replacement->w, replacement->h);
  XConfigureWindow(display, replacement->widgets[frame_parent].widget, mask, &changes);
  
  resize_frame(display, replacement, themes);
  resize_frame(display, target, themes);  

  change_frame_mode(display, replacement, target->mode, themes);
  change_frame_mode(display, target, mode, themes);
  stack_frame(display, target,      seps);
  stack_frame(display, replacement, seps);
  
  return 1;
}

/**** 
Implements stacking and focus policy 
*****/
void 
stack_frame(Display *display, struct Frame *frame, struct Seperators *seps) {
  XWindowChanges changes;
  
  unsigned int mask = CWSibling | CWStackMode;  
  changes.stack_mode = Below;

  printf("stacking window %s\n", frame->window_name);
  
  if(frame->mode == tiling) 
  changes.sibling = seps->tiling_seperator;
  else if(frame->mode == floating) 
  changes.sibling = seps->floating_seperator;  
  else 
  changes.sibling = seps->sinking_seperator;
  
  XConfigureWindow(display, frame->widgets[frame_parent].widget, mask, &changes);
  XFlush(display);
}

/******
maybe remove overlap variable and instead simply set the indirect resize parameters
and just reset them is something goes wrong?
******/
void resize_tiling_frame(Display *display, struct Frame_list *frames, int index, char axis, int position, int size, struct Themes *themes) {
  printf("Got a resize_tilting_frame\n");
  /******
  Purpose:  
    Resizes a window and enlarges any adjacent tiled windows in either axis
     up to a maximum size for the adjacent windows or to a minimum size for the shrinking window.
  Preconditions:  
    axis is 'x' or 'y',
    frames is a valid Frame_list,
    index is a valid index to frames,
    size is the new width or height 
    position is the new x or y co-ordinate and is within a valid range.
  *****/
  
  int shrink_margin = 1;
  
  int size_change;
  int overlap;
  int adj_position, adj_size;
  int frame_space;
      
  //variables for the index frame and variables for the iterated frame
  int *min_size, *fmin_size;
  int *max_size, *fmax_size;
  int *s, *fs;  //size
  int *p, *fp;  //position
  int *fs_adj;  //size of range of values of adjacent frames in perpendicular axis
  int *fp_adj;  //position of range of values of range of adjacent frames in perpendicular axis
  
  #ifdef SHOW_EDGE_RESIZE
  printf("resize tiling frame %s\n", frames->list[index].window_name);
  #endif
  if(axis == 'x') {
    min_size = &frames->list[index].min_width;
    max_size = &frames->list[index].max_width;
    s = &frames->list[index].w;
    p = &frames->list[index].x;
    adj_position = frames->list[index].y;
    adj_size = frames->list[index].h;
    frame_space = frames->list[index].hspace;
  }
  else if(axis == 'y') {
    min_size = &frames->list[index].min_height;
    max_size = &frames->list[index].max_height;
    s = &frames->list[index].h;
    p = &frames->list[index].y;
    adj_position = frames->list[index].x;
    adj_size = frames->list[index].w;
    frame_space = frames->list[index].vspace;
  }

  size_change = size - *s; //the size difference for the specified frame
  
  if(size < *min_size + frame_space  
  || size > *max_size
  || size_change == 0) return;
  
  if(size_change > 0) shrink_margin = 0;

  #ifdef SHOW_EDGE_RESIZE
  printf("\n\nResize: %c, position %d, size %d\n", axis, position, size);
  #endif
  while(True) {  
    int i = 0;
    if(size < *min_size + frame_space  ||  size > *max_size) return;
    
    for(; i < frames->used; i++) {
      if(i == index) {
        //skipping index frame
        frames->list[index].indirect_resize.new_size = 0; 
        continue;
      }

      if(frames->list[i].mode == tiling) {

        if(axis == 'x') {
          fs = &frames->list[i].w;
          fp = &frames->list[i].x;
          fmin_size = &frames->list[i].min_width;
          fmax_size = &frames->list[i].max_width;
          fs_adj = &frames->list[i].h;
          fp_adj = &frames->list[i].y;
        }
        else if(axis == 'y') {
          fs = &frames->list[i].h;
          fp = &frames->list[i].y;
          fmin_size = &frames->list[i].min_height;
          fmax_size = &frames->list[i].max_height;
          fs_adj = &frames->list[i].w;
          fp_adj = &frames->list[i].x;
        }
        //if within perpendicular range
        if((adj_position + adj_size > *fp_adj  &&  adj_position <= *fp_adj)
        || (adj_position < *fp_adj + *fs_adj   &&  adj_position >= *fp_adj)
        || (adj_position <= *fp_adj  &&  adj_position + adj_size >= *fp_adj + *fs_adj)
        ) {
          
          #ifdef SHOW_EDGE_RESIZE
          printf("Frame \" %s \" inside perp range. \n", frames->list[i].window_name);
          #endif
          //the size_change < 0 test determines the direction of the drag and which side is affected
          if( *p == position
          &&  *p + *s >= *fp + *fs
          &&  *p + *s <= *fp + *fs - size_change
          &&  size_change < 0 //shrinking
          ) {
            overlap = (*fp + *fs) - (size + position);
            #ifdef SHOW_EDGE_RESIZE
            printf("above/below RHS aligned, shrinking %d\n", overlap);
            #endif
            frames->list[i].indirect_resize.new_position = *fp;
          }
          
          else if(*p < position
          &&  *p <= *fp
          &&  *p - size_change >= *fp
          &&  size_change < 0 //shrinking
          ) {           
            overlap = position - *fp;
            #ifdef SHOW_EDGE_RESIZE
            printf("above/below LHS aligned, shrinking %d\n", overlap);
            #endif
            frames->list[i].indirect_resize.new_position = *fp + overlap;
          }

         else if( *p == position //find adjacent for enlarging (indirect enlarge)
          &&  *p + *s == *fp + *fs
          &&  *fp <= *p + *s
          &&  size_change > 0 //enlarging
          ) {
            overlap = -size_change;
            #ifdef SHOW_EDGE_RESIZE
            printf("above/below RHS aligned, enlarging %d \n", overlap);
            #endif
            frames->list[i].indirect_resize.new_position = *fp;
          }
          
          else if( *p > position //find adjacent for enlarging
          &&  *p == *fp
          &&  size_change > 0 //enlarging
          ) {
                 
            overlap = -size_change;
            #ifdef SHOW_EDGE_RESIZE
            printf("above/below LHS aligned, enlarging %d HEY\n", overlap);
            #endif
            frames->list[i].indirect_resize.new_position = position;
          }
          
          else if(*p + *s + shrink_margin > *fp //find adjacent for shrinking
          &&  *p + *s <= *fp 
          &&  size_change < 0 //shrinking
          &&  *p == position) {

            overlap = position + size - *fp;
            #ifdef SHOW_EDGE_RESIZE
            printf("found window adjacent to RHS, shrinking %d\n", overlap);
            #endif
            frames->list[i].indirect_resize.new_position = *fp + overlap;
          }
          else if(position + size  > *fp //find overlapped for enlarging
          &&  position < *fp
          &&  size_change > 0 //enlarging       
          &&  *p == position) {
            overlap = position + size - *fp;
            #ifdef SHOW_EDGE_RESIZE
            printf("found window overlapped on RHS, enlarging %d\n", overlap);
            #endif
            frames->list[i].indirect_resize.new_position = *fp + overlap;          
          }
          else if(*p < *fp + *fs + shrink_margin //find adjacent for shrinking
          &&  *p > *fp
          &&  size_change < 0 //shrinking
          &&  *p < position) { 
            overlap = *fp + *fs - position;
            #ifdef SHOW_EDGE_RESIZE
            printf("found window adjacent to LHS shrinking %d\n", overlap);
            #endif
            frames->list[i].indirect_resize.new_position = *fp;                  
          }
          else if(position < *fp + *fs //find overlapped for enlarging
          &&  position > *fp  //new position on other side of window
          &&  *p > position   //moving left
          &&  *fp < *p        //initial position on RHS of window
          &&  size_change > 0 //enlarging
          ) {
            overlap = *fp + *fs - position;
            #ifdef SHOW_EDGE_RESIZE
            printf("found window overlapped on LHS, enlarging %d\n", overlap);
            #endif
            frames->list[i].indirect_resize.new_position = *fp;                              
          }
          else {
            frames->list[i].indirect_resize.new_size = 0; //horizontally out of the way
            continue;
          }

         /* if windows are adjacent and being affected, 
             we need to check if we need to increase the size of opposing axis potentential range
             in order to get indirect resizes of other windows */            
          if(*fp_adj < adj_position  &&  *fp_adj + *fs_adj - adj_position > adj_size) {
            #ifdef SHOW_EDGE_RESIZE
            printf("enlarging adjacency area\n");
            #endif
            //completely encloses the area that encloses the adjacent windows
            
            adj_position = *fp_adj;
            adj_size = *fs_adj;
            break; //this will cause the loop to reset with the new values
          }
          else if(*fp_adj + *fs_adj - adj_position > adj_size) {
            //extends below the area that encloses the adjacent windows
            #ifdef SHOW_EDGE_RESIZE
            printf("enlarging adjacency area in h\n");
            #endif
            adj_size = *fp_adj + *fs_adj - adj_position;
            #ifdef SHOW_EDGE_RESIZE
            printf("adj_position %d, adj_size %d \n", adj_position, adj_size);
            #endif
            break; //this will cause the loop to reset with the new value
          }
          else if(*fp_adj < adj_position) {
            //extends above the adjacency area
            #ifdef SHOW_EDGE_RESIZE
            printf("enlarging adjacency area in position \n");
            #endif
            adj_size = adj_position + adj_size - *fp_adj;
            adj_position = *fp_adj;
            break; //this will cause the loop to reset with the new values
          }

          
          if(*fs - overlap >= *fmin_size + frame_space &&
             *fs - overlap <= *fmax_size) {
            frames->list[i].indirect_resize.new_size = *fs - overlap;
          }
          else if (overlap > 0 ) {
            #ifdef SHOW_EDGE_RESIZE
            printf("not within min/max adjusting.. finding max allowed value \n");
            #endif
            int new_size;
            if(position == *p) {
              new_size = *fp + *fmin_size + frame_space - *p;
            }
            else if(position < *p) {
              new_size = *p + *s - position;
              position = *fp + *fmin_size + frame_space;
            }

            if(new_size >= size) {      
              #ifdef SHOW_EDGE_RESIZE        
              printf("Cannot resize, cancelling resize.");
              #endif
              return;
            }
            size = new_size;
            break;
          }
          else if(frames->list[i].indirect_resize.new_size > *fmax_size) { //too big
            #ifdef SHOW_EDGE_RESIZE 
            printf("Setting size to max\n");
            #endif
            frames->list[i].indirect_resize.new_size = *fmax_size; //make it the maximum
          }              
        }
        else {
          frames->list[i].indirect_resize.new_size = 0; //vertically out of the way
          continue;
        }
      }
    }
    
    if(i == frames->used) break;
  }
 
  if(True) { //resize all modified windows.
    *p = position;
    *s = size;
    for(int i = 0; i < frames->used; i++) {
      if(frames->list[i].mode == tiling  &&  frames->list[i].indirect_resize.new_size) {
        if(axis == 'x') {
          frames->list[i].x = frames->list[i].indirect_resize.new_position;
          frames->list[i].w = frames->list[i].indirect_resize.new_size;
        } 
        else if(axis == 'y') {
          frames->list[i].y = frames->list[i].indirect_resize.new_position; 
          frames->list[i].h = frames->list[i].indirect_resize.new_size;
        }
        resize_frame(display, &frames->list[i], themes);
      }
    }
  }
  
  resize_frame(display, &frames->list[index], themes);
  return;
}