
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
    new_height = XHeightOfScreen(screen) - new_y - MENUBAR_HEIGHT;
  }
  
  //commit height changes
  if(frame->mode == TILING) {
    resize_tiling_frame(display, frames, clicked_frame, 'y', frame->y, new_height);
  }
  else 
  if(new_height >= frame->min_height + FRAME_VSPACE
  && new_height <= frame->max_height + FRAME_VSPACE) {
    frame->h = new_height;              
  }

  //commit width and x position changes                             
  if(frame->mode == TILING) {
    resize_tiling_frame(display, frames, clicked_frame, 'x', new_x, new_width);
  }
  else 
  if(new_width >= frame->min_width + FRAME_HSPACE
  && new_width <= frame->max_width + FRAME_HSPACE) {
    frame->x = new_x;  //for l_grip and bl_grip
    frame->w = new_width;
  }            
  
  if(frame->mode != TILING) resize_frame(display, frame);
  XFlush(display);
}

extern void handle_frame_retile (Display *display, struct Framelist *frames, int clicked_frame
,  int pointer_start_x, int pointer_start_y, int mouse_root_x, int mouse_root_y) {

  struct Frame *frame = &frames->list[clicked_frame]; 
  struct Frame temp;
  int start = -1;
  struct rectangle_list free_spaces = {0, 8, NULL};
  int largest = 0;
  unsigned long int largest_area = 0;
  
  printf("retiling a frame\n");
  //Find tiled windows that intersect the placed window and seperate them at the start of the list
  for(int i = 0; i < frames->used; i++) { //put the intersecting windows first.
    if(i == clicked_frame) continue;
    if(INTERSECTS(frame->x, frame->w, frames->list[i].x, frames->list[i].w) 
    && INTERSECTS(frame->y, frame->h, frames->list[i].y, frames->list[i].h)) {
      start++;
      if(start == i) continue;
      temp = frames->list[start];
      frames->list[start] = frames->list[i];
      frames->list[i] = temp;
    }
  }

  //return if there were no overlapping windows
  if(start == -1) return;
  
  //Calculate free space but skip the intersected windows and include the placed window.
  //there will be at least one other window apart from the intersecting windows - the clicked_frame
  //so start + 1 doesn't need to be checked
  free_spaces = get_free_screen_spaces (display, frames, start + 1);
  
//  for(int k = 0; k < free_spaces.used; k++) {
//    printf("Free space: x %d, y %d, w %d, h %d\n"
//    , free_spaces.list[k].x, free_spaces.list[k].y, free_spaces.list[k].w, free_spaces.list[k].h);
//  }
  
  //iterate over the overlapping frames,  placing them each in the nearest available position.
  for(int i = 0; i <= start; i++) {
    for(int k = 0; k < free_spaces.used; k++) {
      struct rectangle window;
      double displacement;
      window.x = frames->list[i].x;
      window.y = frames->list[i].y;
      window.w = frames->list[i].w;      
      window.h = frames->list[i].h;
      displacement = calculate_displacement(window, free_spaces.list[k]);
      printf("intersecting w %d, space %d, distance %f\n", i, k, displacement);
    }
  }
  
  if(free_spaces.list != NULL) free(free_spaces.list);
  return;
}

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

  if((new_width != 0  &&  new_width < frame->min_width + FRAME_HSPACE) 
  ||(new_height != 0  &&  new_height < frame->min_height + FRAME_VSPACE)) {
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
  if((new_width  != 0  &&  new_width  > frame->max_width  + FRAME_HSPACE) 
  || (new_height != 0  &&  new_height > frame->max_height + FRAME_VSPACE)) {
    //investigate if this has similar situations as above where it moves instead of stopping
    //once limit is reached
    if(new_width > frame->max_width + FRAME_HSPACE) new_width = 0;
    if(new_height > frame->max_height + FRAME_VSPACE) new_height = 0;
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

