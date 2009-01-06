/* Design Rationale.  These functions were previously included in the main function and have been seperated to enhance readability.
Therefore, they accept lots of parameters and look messy, beause there are inter-relationships with other events. */

/*
This function responds to a user's request to unsink a window by changing its mode from sinking
*/
void handle_frame_unsink (Display *display, struct Frame_list *frames, int index, struct Pixmaps *pixmaps) {
  handle_frame_drop(display, frames, index);
  show_frame_state(display, &frames->list[index], pixmaps); 
}

/*
This function handles responding to the users click and drag on the resize grips of the window.
*/
void handle_frame_resize (Display *display, struct Frame_list *frames, int clicked_frame
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
This function moves the dropped window to the nearest available space.
If the window has been enlarged so that it exceeds all available sizes,
a best-fit algorithm is used to determine the closest size.
If all spaces are smaller than the window's minimum size 
(which can only happen if the window's mode is being changed) the window
remains in it's previous mode. Otherwise the window's mode is changed to tiling.

*/
void handle_frame_drop (Display *display, struct Frame_list *frames, int clicked_frame) {

  struct Frame *frame = &frames->list[clicked_frame]; 
  struct Rectangle_list free_spaces = {0, 8, NULL};
  double min_displacement = 0; 
  int min = -1;
  int min_dx = 0;
  int min_dy = 0;
  
  //make the frame into a rectangle for calculating displacement function
  struct Rectangle window =  {frame->x
  , frame->y
  , frame->w
  , frame->h };
  
  #ifdef SHOW_FRAME_DROP  
  printf("Retiling dropped window\n");
  #endif
  if(frame->mode == TILING) {
    frame->mode = SINKING;
    free_spaces = get_free_screen_spaces (display, frames);
    frame->mode = TILING;
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
    if(displacement >= 0  &&  (min_displacement == 0  ||  displacement < min_displacement)) {
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
        printf("Error: FOUND ZERO AREA FREE SPACE\n");
        continue;
      }
      w_proportion = (double)frame->w / free_spaces.list[k].w;
      h_proportion = (double)frame->h / free_spaces.list[k].h;
      if(w_proportion > 1) free_spaces.list[k].w / frame->w;
      if(h_proportion > 1) free_spaces.list[k].h / frame->h;
      current_fit = w_proportion * h_proportion;
      if(current_fit > best_fit
      && free_spaces.list[k].w >= frame->min_width
      && free_spaces.list[k].h >= frame->min_height) {
        best_fit = current_fit;
        best_space = k;
      }
    }
    
    if(best_space != -1) {
      frame->mode = TILING;    
      frame->x = free_spaces.list[best_space].x;
      frame->y = free_spaces.list[best_space].y;
      frame->w = free_spaces.list[best_space].w;
      frame->h = free_spaces.list[best_space].h;
      
      if(frame->w > frame->max_width) frame->w = frame->max_width;
      if(frame->h > frame->max_height) frame->h = frame->max_height;
      resize_frame(display, frame);
    }
  }
  else {
    #ifdef SHOW_FRAME_DROP  
    printf("Found min_dx %d, min_dy %d, distance %f\n", min_dx, min_dy, (float)min_displacement);
    #endif
    frame->mode = TILING;
    frame->x += min_dx;
    frame->y += min_dy;
    move_frame(display, frame);
  }

  if(free_spaces.list != NULL) free(free_spaces.list);
  return;
}

/*
This handles moving/resizing the window when the titlebar is dragged.  
It resizes windows that are pushed against the edge of the screen,
to sizes between the defined min and max.  If the users attemps to resize below a
minimum size the window is sunk (after a disable look is shown and the user releases the mouse button).
 
*/
void handle_frame_move (Display *display, struct Frame *frame
, int *pointer_start_x, int *pointer_start_y, int mouse_root_x, int mouse_root_y
, int *was_sunk, struct Pixmaps *pixmaps, int *resize_x_direction, int *resize_y_direction) {
  
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

  //do not attempt to resize if the window is larger than the screen
  if(frame->min_width <= XWidthOfScreen(screen)
  && frame->min_height <= XHeightOfScreen(screen) - MENUBAR_HEIGHT) {
  
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

      #ifdef ALLOW_INTERPRETIVE_SINKING
      /** Show Sinking state**/
      if(!*was_sunk) {
        *was_sunk = frame->mode;
        printf("Setting sinking mode from %d\n", *was_sunk);
        frame->mode = SINKING;
        show_frame_state(display, frame, pixmaps);
      }
      #endif
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
    if(new_width != 0) {
      frame->w = new_width; 
      //Cancel sinking look
      #ifdef ALLOW_INTERPRETIVE_SINKING      
      if(*was_sunk) {
        printf("Cancel edge slam by width, new mode %d\n", *was_sunk);
        frame->mode = *was_sunk;
        show_frame_state(display, frame, pixmaps);
        XSync(display, False);
        *was_sunk = 0;
      }
      #endif
    }
    
    if(new_height != 0) frame->h = new_height;
    //allow movement in axis if it hasn't been resized
    
    resize_frame(display, frame);
    
  }
  else {
    //Moves the window to the specified location if there is no resizing going on.
    #ifdef ALLOW_INTERPRETIVE_SINKING
    if(frame->x != new_x  &&  *was_sunk) {
      printf("Cancel edge slam by movement in x, new mode is %d\n", *was_sunk);
      frame->mode = *was_sunk;
      show_frame_state(display, frame, pixmaps);
      XSync(display, False);
      *was_sunk = 0;
    }
    #endif
    frame->x = new_x;
    frame->y = new_y;
    move_frame(display, frame);
  }
  XFlush(display);  
}

