void check_frame_limits(Display *display, struct Frame *frame) {
  Screen* screen = DefaultScreenOfDisplay(display);  

  if(frame->w < frame->min_width)  frame->w = frame->min_width;
  else 
  if(frame->w > frame->max_width)  frame->w = frame->max_width;
  
  if(frame->h < frame->min_height) frame->h = frame->min_height;
  else 
  if(frame->h > frame->max_height) frame->h = frame->max_height;  
  
  if(frame->x < 0) frame->x = 0;
  if(frame->y < 0) frame->y = 0;
  
  if(frame->x + frame->w > XWidthOfScreen(screen)) 
    frame->x = XWidthOfScreen(screen) - frame->w;
  if(frame->y + frame->h > XHeightOfScreen(screen) - MENUBAR_HEIGHT) 
    frame->y = XHeightOfScreen(screen)- frame->h - MENUBAR_HEIGHT;
}

void change_frame_mode(Display *display, struct Frame *frame, enum Window_mode mode) {
  Screen* screen = DefaultScreenOfDisplay(display);
  if(frame->selected != 0) XRaiseWindow(display, frame->selection_indicator.indicator_active);
  else XRaiseWindow(display, frame->selection_indicator.indicator_normal);      
  
  XRaiseWindow(display, frame->close_button.close_button_normal);
  XRaiseWindow(display, frame->title_menu.title_normal);
  XRaiseWindow(display, frame->title_menu.arrow.arrow_normal);
  
  XFlush(display);
  if(frame->mode == unset  &&  mode == unset) {
    //printf("This frame did not have a stacking mode set\n");
    mode = floating;
  }
    
  //if(frame->mode == mode) return;
  
  if(frame->mode == hidden) XMapWindow(display, frame->frame);
  else 
  if(frame->mode == desktop  &&  mode != desktop) {
    check_frame_limits(display, frame);
    resize_frame(display, frame);
  }
  
  if(mode == hidden) {
    XUnmapWindow(display, frame->frame);
    frame->mode = hidden;
  }
  else 
  if(mode == floating) {
    XRaiseWindow(display, frame->mode_dropdown.floating_normal);
    frame->mode = floating;
  }
  else
  if(mode == tiling) {
    XRaiseWindow(display, frame->mode_dropdown.tiling_normal);
    frame->mode = tiling;
  }

  else 
  if(mode == desktop  &&  frame->mode != desktop) {
    XRaiseWindow(display, frame->mode_dropdown.desktop_normal);
    frame->x = -(EDGE_WIDTH*2 + H_SPACING);
    frame->y = -(TITLEBAR_HEIGHT + EDGE_WIDTH);

    if(frame->max_width >= XWidthOfScreen(screen)) 
      frame->w = XWidthOfScreen(screen) + FRAME_HSPACE;
    else 
      frame->w = frame->max_width;

    if(frame->max_height >= XHeightOfScreen(screen) - MENUBAR_HEIGHT)
      frame->h = XHeightOfScreen(screen) - MENUBAR_HEIGHT + FRAME_VSPACE;
    else 
      frame->h = frame->max_height;
    
    frame->mode = desktop;
    resize_frame(display, frame);
  }
  XFlush(display);
}

/*
This function responds to a user's request to unsink a window by changing its mode from sinking
*/
void unsink_frame (Display *display, struct Frame_list *frames, int index) {
  //printf("unsink frame\n");
  drop_frame(display, frames, index);
}

/*
This function moves the dropped window to the nearest available space.
If the window has been enlarged so that it exceeds all available sizes,
a best-fit algorithm is used to determine the closest size.
If all spaces are smaller than the window's minimum size 
(which can only happen if the window's mode is being changed) the window
remains in it's previous mode. Otherwise the window's mode is changed to tiling.
*/
void drop_frame (Display *display, struct Frame_list *frames, int clicked_frame) {

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
  
  for(int k = 0; k < free_spaces.used; k++) {
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
    for(int k = 0; k < free_spaces.used; k++) {
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
      change_frame_mode(display, frame, tiling);
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
      resize_frame(display, frame);
    }
    else change_frame_mode(display, frame, frame->mode);
  }
  else {
    #ifdef SHOW_FRAME_DROP  
    printf("Found min_dx %d, min_dy %d, distance %f\n", min_dx, min_dy, (float)min_displacement);
    #endif
    change_frame_mode(display, frame, tiling);
    frame->x += min_dx;
    frame->y += min_dy;
    XMoveWindow(display, frame->frame, frame->x, frame->y);
    XFlush(display);
  }

  if(free_spaces.list != NULL) free(free_spaces.list);
  return;
}

/*
This function handles responding to the users click and drag on the resize grips of the window.
*/
void resize_nontiling_frame (Display *display, struct Frame_list *frames, int clicked_frame
, int pointer_start_x, int pointer_start_y, int mouse_root_x, int mouse_root_y
, int r_edge_dx, int b_edge_dy, Window clicked_widget) {
  
  Window root = DefaultRootWindow(display);
  Screen* screen = DefaultScreenOfDisplay(display);

  struct Frame *frame = &frames->list[clicked_frame];  
  
  int new_width = frame->w;
  int new_height = frame->h;
  int new_x = mouse_root_x - pointer_start_x;
  int new_y = frame->y; 
  //int new_y = mouse_root_y - pointer_start_y; //no top grip
  //printf("resize_nontiling_frame %s\n", frames->list[clicked_frame].window_name);
  if(frame->mode != desktop) {
    if(new_x < 0) new_x = 0;
    if(new_y < 0) new_y = 0;
  }
  if(clicked_widget == frame->l_grip) {
    new_width = frame->w + (frame->x - new_x);
  }
  else if(clicked_widget == frame->r_grip) {
    new_width = mouse_root_x - frame->x + r_edge_dx;
    new_x = frame->x;
  }
  else if(clicked_widget == frame->bl_grip) {
    new_width = frame->w + (frame->x - new_x);
    new_height = mouse_root_y - frame->y;
  }
  else if(clicked_widget == frame->br_grip) {
    new_width = mouse_root_x - frame->x + r_edge_dx;
    new_height = mouse_root_y - frame->y + b_edge_dy;
    new_x = frame->x;
  }
  else if(clicked_widget == frame->b_grip) {
    new_height = mouse_root_y - frame->y + b_edge_dy;
    new_x = frame->x;
  }
  
  if(new_height < frame->min_height) new_height = frame->min_height;
  if(new_height > frame->max_height) new_height = frame->max_height;
  if(new_width < frame->min_width) new_width = frame->min_width;
  if(new_width > frame->max_width) new_width = frame->max_width;
  if(frame->mode != desktop) {  
    if(new_x + new_width > XWidthOfScreen(screen))
      new_width = XWidthOfScreen(screen) - new_x;
    if(new_y + new_height > XHeightOfScreen(screen) - MENUBAR_HEIGHT)
      new_height = XHeightOfScreen(screen)- new_y - MENUBAR_HEIGHT;
  }
  //commit height changes
  if(frame->mode == tiling) {
    if(new_height != frame->h) resize_tiling_frame(display, frames, clicked_frame, 'y', new_y, new_height);
    if(new_width != frame->w)  resize_tiling_frame(display, frames, clicked_frame, 'x', new_x, new_width);
  }
  else {  
    frame->x = new_x;  //for l_grip and bl_grip
    frame->y = new_y;  //in case top grip is added later
    frame->w = new_width;
    frame->h = new_height;    
  } 
  resize_frame(display, frame);
  XFlush(display);
}


/*
This handles moving/resizing the window when the titlebar is dragged.  
It resizes windows that are pushed against the edge of the screen,
to sizes between the defined min and max.  If the users attemps to resize below a
minimum size the window is sunk (after a disable look is shown and the user releases the mouse button).
*/
void move_frame (Display *display, struct Frame *frame
, int *pointer_start_x, int *pointer_start_y, int mouse_root_x, int mouse_root_y
, struct Pixmaps *pixmaps, int *resize_x_direction, int *resize_y_direction) {
  
  //the pointer start variables may be updated as the window is squished against the LHS or top of the screen
  //because of the changing relative co-ordinates of the pointer on the window.
  
  //resize_x_direction - 1 means LHS, -1 means RHS , 0 means unset
  //resize_y_direction - 1 means top, -1 means bottom, 0 means unset
  // the resize_x/y_direction variables are used to identify when a squish resize has occured
  // and this is then used to decide when to stretch the window from the edge.

  Window root = DefaultRootWindow(display);
  Screen* screen = DefaultScreenOfDisplay(display);

  int new_width = 0;
  int new_height = 0;
  int new_x = mouse_root_x - *pointer_start_x;
  int new_y = mouse_root_y - *pointer_start_y;
  //printf("move frame %s\n", frame->window_name);
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
    
    resize_frame(display, frame);
    
  }
  else {
    //Moves the window to the specified location if there is no resizing going on.
    frame->x = new_x;
    frame->y = new_y;
    XMoveWindow(display, frame->frame, frame->x, frame->y);
  }
  XFlush(display);  
}

int replace_frame(Display *display, struct Frame *target, struct Frame *replacement
, Window sinking_seperator, Window tiling_seperator, Window floating_seperator, struct Pixmaps *pixmaps) {
  XWindowChanges changes;
  XWindowChanges changes2;
  enum Window_mode mode;
  unsigned int mask = CWX | CWY | CWWidth | CWHeight;

  #ifdef SHOW_BUTTON_PRESS_EVENT
  printf("replace frame %s\n", replacement->window_name);
  #endif
  if(replacement->window == target->window) return 0;  //this can be chosen from the title menu
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
    XConfigureWindow(display, target->frame, mask, &changes2);
  }
  
  replacement->x = changes.x;
  replacement->y = changes.y;
  replacement->w = changes.width;
  replacement->h = changes.height;
  //printf("Target mode %d, x %d, y %d, w %d, h %d\n", mode, replacement->x, replacement->y, replacement->w, replacement->h);
  XConfigureWindow(display, replacement->frame, mask, &changes);
  
  resize_frame(display, replacement);
  resize_frame(display, target);  

  change_frame_mode(display, replacement, target->mode);
  change_frame_mode(display, target, mode);
  stack_frame(display, target,      sinking_seperator, tiling_seperator, floating_seperator);
  stack_frame(display, replacement, sinking_seperator, tiling_seperator, floating_seperator);
  
  return 1;
}

/* Implements stacking and focus policy */
void stack_frame(Display *display, struct Frame *frame, Window sinking_seperator, Window tiling_seperator, Window floating_seperator) {
  XWindowChanges changes;
  
  unsigned int mask = CWSibling | CWStackMode;  
  changes.stack_mode = Below;

  //printf("stacking window %s\n", frame->window_name);
  
  if(frame->mode == tiling) 
  changes.sibling = tiling_seperator;
  else if(frame->mode == floating) 
  changes.sibling = floating_seperator;  
  else 
  changes.sibling = sinking_seperator;
  
  XConfigureWindow(display, frame->frame, mask, &changes);
  XFlush(display);
}

//maybe remove overlap variable and instead simply set the indirect resize parameters and just reset them is something goes wrong?
void resize_tiling_frame(Display *display, struct Frame_list *frames, int index, char axis, int position, int size) {
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
    frame_space = FRAME_HSPACE;
  }
  else if(axis == 'y') {
    min_size = &frames->list[index].min_height;
    max_size = &frames->list[index].max_height;
    s = &frames->list[index].h;
    p = &frames->list[index].y;
    adj_position = frames->list[index].x;
    adj_size = frames->list[index].w;
    frame_space = FRAME_VSPACE;
  }

  size_change = size - *s; //the size difference for the specified frame
  
  if(size < *min_size + frame_space  
  || size > *max_size
  || size_change == 0) return;
  
  if(size_change > 0) shrink_margin = 0;

  #ifdef SHOW_EDGE_RESIZE
  printf("\n\nResize: %c, position %d, size %d\n", axis, position, size);
  #endif
  while(!done) {  
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
 
  if(!done) { //resize all modified windows.
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
        resize_frame(display, &frames->list[i]);      
      }
    }
  }
  
  resize_frame(display, &frames->list[index]);
  return;
}
