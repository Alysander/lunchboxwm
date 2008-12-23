/* Design Rationale.    */

/*
This function attempts to tile the sinking window in available screen space.
*/
void handle_frame_unsink (Display *display, struct Framelist *frames, int index, struct frame_pixmaps *pixmaps) {
  struct rectangle_list free_spaces;
  int largest = 0;
  unsigned long int largest_area = 0;

  #ifdef SHOW_BUTTON_PRESS_EVENT
  printf("Tiling a previously sunk window\n");
  #endif
  free_spaces = get_free_screen_spaces (display, frames);

  for(int k = 0; k < free_spaces.used; k++) {
    #ifdef SHOW_BUTTON_PRESS_EVENT
    printf("Free space: x %d, y %d, w %d, h %d\n"
    , free_spaces.list[k].x, free_spaces.list[k].y, free_spaces.list[k].w, free_spaces.list[k].h);
    #endif
    
    if(frames->list[index].min_width  <= free_spaces.list[k].w
    && frames->list[index].min_height <= free_spaces.list[k].h) {
      if(free_spaces.list[k].w * free_spaces.list[k].h > largest_area) {
        largest = k;
        largest_area = free_spaces.list[k].w * free_spaces.list[k].h;
      }
    }
  }
  if(largest_area != 0) {
    if(frames->list[index].max_width > free_spaces.list[largest].w) frames->list[index].w = free_spaces.list[largest].w;
    else frames->list[index].w = frames->list[index].max_width;
      
    if(frames->list[index].max_height > free_spaces.list[largest].h) frames->list[index].h = free_spaces.list[largest].h;
    else frames->list[index].h = frames->list[index].max_height;
                          
    frames->list[index].x = free_spaces.list[largest].x;
    frames->list[index].y = free_spaces.list[largest].y;
    frames->list[index].mode = TILING;

    resize_frame(display, &frames->list[index]);
    show_frame_state(display, &frames->list[index], pixmaps);
  }
  if(free_spaces.list != NULL) free(free_spaces.list);
}

/*
This function handles responding to the users click and drag on the resize grips of the window.
*/
void handle_frame_resize (Display *display, struct Framelist *frames, int clicked_frame
, int pointer_start_x, int pointer_start_y, int mouse_root_x, int mouse_root_y, Window clicked_widget) {

  Window root = DefaultRootWindow(display);
  Screen* screen = DefaultScreenOfDisplay(display);
  
  int new_width = 0;
  int new_height = 0;
  int new_x = mouse_root_x - pointer_start_x;
  int new_y = mouse_root_y - pointer_start_y;

  struct Frame *frame = &frames->list[clicked_frame];  

  if(clicked_widget == frame->l_grip) {
    new_width = frame->w + (frame->x - new_x);
  }
  else if(clicked_widget == frame->r_grip) {
    new_width = mouse_root_x - frame->x ;
    new_x = frame->x;
  }
  else if(clicked_widget == frame->bl_grip) {
    new_width = frame->w + (frame->x - new_x);
    new_height = mouse_root_y - frame->y;
  }
  else if(clicked_widget == frame->br_grip) {
    new_width = mouse_root_x - frame->x;
    new_height = mouse_root_y - frame->y;
    new_x = frame->x;
  }
  else if(clicked_widget == frame->b_grip) {
    new_height = mouse_root_y - frame->y;
    new_x = frame->x;
  }
  
  //adjust for the toolbar
  new_y = frame->y;
  if(new_height + new_y > XHeightOfScreen(screen) - MENUBAR_HEIGHT) {
    #ifdef SHOW_EDGE_RESIZE
    printf("Adjusted for toolbar height %d ,", new_height);
    #endif
    new_height = XHeightOfScreen(screen) - new_y - MENUBAR_HEIGHT;
    #ifdef SHOW_EDGE_RESIZE
    printf("%d \n", new_height);
    #endif
  }
  
  //commit height changes
  if(frame->mode == TILING) {
    resize_tiling_frame(display, frames, clicked_frame, 'y', frame->y, new_height);
  }
  else 
  if(new_height >= frame->min_height
  && new_height <= frame->max_height) {
    frame->h = new_height;              
  }

  //commit width and x position changes                             
  if(frame->mode == TILING) {
    resize_tiling_frame(display, frames, clicked_frame, 'x', new_x, new_width);
  }
  else 
  if(new_width >= frame->min_width
  && new_width <= frame->max_width) {
    frame->x = new_x;  //for l_grip and bl_grip
    frame->w = new_width;
  }            
  
  if(frame->mode != TILING) resize_frame(display, frame);
  XFlush(display);
}

/*
This function moves the dropped tiling window to the nearest available space.
*/
extern void handle_frame_drop (Display *display, struct Framelist *frames, int clicked_frame
,  int pointer_start_x, int pointer_start_y, int mouse_root_x, int mouse_root_y) {

  struct Frame *frame = &frames->list[clicked_frame]; 
  struct rectangle_list free_spaces = {0, 8, NULL};
  double min_displacement = 0; 
  int min = -1;  //index of the closest free space
  int dx = 0;
  int dy = 0;
  
  //the frame needs to be converted to this alternative type for calculating displacement.
  struct rectangle window =  {frames->list[clicked_frame].x
  , frames->list[clicked_frame].y
  , frames->list[clicked_frame].w
  , frames->list[clicked_frame].h };
    
  if(frame->mode != TILING) return;
  
  frame->mode = FLOATING;
  free_spaces = get_free_screen_spaces (display, frames);
  frame->mode = TILING;  
  
  for(int k = 0; k < free_spaces.used; k++) {
    double displacement = 0;    
    displacement = calculate_displacement(window, free_spaces.list[k], &dx, &dy);
    if(min_displacement == 0  ||  displacement < min_displacement) {
      min_displacement = displacement;
      min = k;
    }
    printf("Free space:space %d, x %d, y %d, w %d, h %d, distance %f\n", displacement
    , k, free_spaces.list[k].x, free_spaces.list[k].y, free_spaces.list[k].w, free_spaces.list[k].h);
  }

  if(min == -1) { 
    //this should never occur as in the worst case the window can return to whence it came
    printf("Impossible:  Unable to find free space to move dropped window\n");
  }
  else {
    frame->x += dx;
    frame->y += dy;
    move_frame(display, frame);
  }

  if(free_spaces.list != NULL) free(free_spaces.list);
  return;
}

/*
This handles moving the window when the titlebar is dragged.  
It resizes windows that are pushed against the edge of the screen,
to sizes between the defined min and max.  If the users attemps to resize below a
minimum size the window is sunk 
(after a disable look is shown and the user releases the mouse button).

After this function is run, another window determines if the tiled window is overlapping
other tiled windows and then "handle_frame_retile" determines which windows need to be moved
 and where.
 
TODO:  Use the Framelist to determine the maximum available spaces (at the start of the move)
*/
void handle_frame_move (Display *display, struct Framelist *frames, int clicked_frame
, int *pointer_start_x, int *pointer_start_y, int mouse_root_x, int mouse_root_y
, int *was_sunk, struct frame_pixmaps *pixmaps, int *resize_x_direction, int *resize_y_direction) {
  
  //the pointer start variables may be updated as the window is squished against the LHS or top of the screen
  //because of the changing relative co-ordinates of the pointer on the window.
 
  //was_sunk is a pointer because the previous state of the window may need to be restored after several MotionNotify events. 
  
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
  struct Frame *frame = &frames->list[clicked_frame]; 
  
  //TODO if the window is tiled make the mode pulldown list look disabled.
  if((new_x + frame->w > XWidthOfScreen(screen)) //window moving off RHS
  || (*resize_x_direction == -1)) {  
    *resize_x_direction = -1;
    new_width = XWidthOfScreen(screen) - new_x;
  }
  
  if((new_x < 0) //window moving off LHS
  ||(*resize_x_direction == 1)) { 
    *resize_x_direction = 1;
    new_width = frame->w + new_x;
    new_x = 0;
    *pointer_start_x = mouse_root_x;
  }

  if((new_y + frame->h > XHeightOfScreen(screen) - MENUBAR_HEIGHT) //window moving off the bottom
  ||(*resize_y_direction == -1)) { 
    *resize_y_direction = -1;
    new_height = XHeightOfScreen(screen) - MENUBAR_HEIGHT - new_y;
  }
  
  if((new_y < 0) //window moving off the top of the screen
  ||(*resize_y_direction == 1)) { 
    *resize_y_direction = 1;
    new_height = frame->h + new_y;
    new_y = 0;
    *pointer_start_y = mouse_root_y;
  }

  if((new_width != 0  &&  new_width < frame->min_width) 
  ||(new_height != 0  &&  new_height < frame->min_height)) {
    if(new_width != 0) {
      new_width = 0;
      if(*resize_x_direction == -1) new_x = XWidthOfScreen(screen) - frame->w;
      //don't move the window off the RHS if it has reached it's minimum size
      //LHS not considered because x has already been set to 0
      
      /** Show Sinking state**/
      if(!*was_sunk) {
        *was_sunk = frame->mode;
        printf("setting sinking mode from %d\n", *was_sunk);
        frame->mode = SINKING;
        show_frame_state(display, frame, pixmaps);
      }
    }
    if(new_height != 0) {
      new_height = 0;    
      //don't move the window off the bottom if it has reached it's minimum size
      //Top not considered because y has already been set to 0
      if(*resize_y_direction == -1) new_y = XHeightOfScreen(screen) - frame->h;
    }
  }

  //limit resizes to max width
  if((new_width  != 0  &&  new_width  > frame->max_width) 
  || (new_height != 0  &&  new_height > frame->max_height)) {
    //investigate if this has similar situations as above where it moves instead of stopping
    //once limit is reached
    if(new_width > frame->max_width) new_width = 0;
    if(new_height > frame->max_height) new_height = 0;
  } 

  //do not attempt to resize windows that cannot be resized
  if(frame->min_width == frame->max_width) *resize_x_direction = 0;
  if(frame->min_height == frame->max_height) *resize_y_direction = 0;
              
  if(new_width != 0  ||  new_height != 0) {   //resize window if required
    if(new_width != 0) {
      frame->w = new_width;              
      frame->x = new_x;
      //Cancel sinking look
      if(*was_sunk) {
        printf("Cancel edge slam by width, new mode %d\n", *was_sunk);
        frame->mode = *was_sunk;
        show_frame_state(display, frame, pixmaps);
        XSync(display, False);
        *was_sunk = 0;
      }
    }
    else {
      frame->x = new_x; 
      //allow movement in axis if it hasn't been resized
    }
      
    if(new_height != 0) {
      frame->h = new_height;
      frame->y = new_y;
    }
    else frame->y = new_y; //allow movement in axis if it hasn't been resized
    
    resize_frame(display, frame);
    
  }
  else {
    //Moves the window to the specified location if there is no resizing going on.
    if(frame->x != new_x  &&  *was_sunk) {
      printf("Cancel edge slam by movement in x, new mode is %d\n", *was_sunk);
      frame->mode = *was_sunk;
      show_frame_state(display, frame, pixmaps);
      XSync(display, False);
      *was_sunk = 0;
    }
    frame->x = new_x;
    frame->y = new_y;
    XMoveWindow(display, frame->frame, frame->x, frame->y);
  }
  XFlush(display);  
}

