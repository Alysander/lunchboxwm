
struct rectangle_list get_free_screen_spaces (Display *display, struct Framelist *frames) {
  struct rectangle_list used_spaces = {0, 8, NULL};
  struct rectangle_list free_spaces = {0, 8, NULL};
  Window root = DefaultRootWindow(display);
  Screen* screen = DefaultScreenOfDisplay(display);
  
  used_spaces.list = malloc(used_spaces.max * sizeof(struct rectangle_list));
  if(used_spaces.list == NULL) {
    free(used_spaces.list);
    //probably could do something more intelligent here
    printf("Error: out of memory for calculating free spaces on the screen.\n");
    return free_spaces;
  }
  
  for(int i = 0; i < frames->used; i++) {
    if(frames->list[i].mode == TILING) {
      struct rectangle current = 
        {frames->list[i].x, frames->list[i].y, frames->list[i].w, frames->list[i].h};

//      printf("Tiled window %d, x %d, y %d, w %d, h %d\n", i, frames->list[i].x, frames->list[i].y, frames->list[i].w, frames->list[i].h);
      add_rectangle(&used_spaces, current);
    }
  }
  free_spaces = largest_available_spaces(&used_spaces, XWidthOfScreen(screen), XHeightOfScreen(screen) - MENUBAR_HEIGHT);
  return free_spaces;
}

/* This implements "Free space modeling for placing rectangles without overlapping" by Marc Bernard and Francois Jacquenet */
struct rectangle_list largest_available_spaces (struct rectangle_list *used_spaces, int w, int h) {

  struct rectangle_list free_spaces = {0, 8, NULL};
  free_spaces.list = malloc(free_spaces.max * sizeof(struct rectangle_list));
  if(free_spaces.list == NULL) {
    free(used_spaces->list);
    //probably could do something more intelligent here
    printf("Error: out of memory for calculating free spaces on the screen.\n");
    return free_spaces;
  }
  
  struct rectangle screen_space = {0, 0, w, h};
  add_rectangle(&free_spaces, screen_space ); //define the intitial space.
  
  //for all used spaces (already tiled windows on the screen)
  for(int i = 0; i < used_spaces->used; i++) {  
    struct rectangle_list new_spaces = {0, 8, NULL};
    struct rectangle_list old_spaces = {0, 8, NULL};  
    new_spaces.list = malloc(new_spaces.max * sizeof(struct rectangle_list));
    old_spaces.list = malloc(old_spaces.max * sizeof(struct rectangle_list));

    if(new_spaces.list == NULL  ||  old_spaces.list == NULL) {
      free(used_spaces->list);
      free(free_spaces.list);
      used_spaces->list == NULL;
      free_spaces.list == NULL;          
      //probably could do something more intelligent here
      printf("Error: out of memory for calculating free spaces on the screen.\n");
      return free_spaces;
    }
    for(int j = 0; j < free_spaces.used; j++) {
      //if an edge intersects, modify opposing edge.  add modified to new_spaces and original to old_spaces
      struct rectangle new = {0, 0, 0, 0};
      if(INTERSECTS(free_spaces.list[j].y, free_spaces.list[j].h, used_spaces->list[i].y, used_spaces->list[i].h) ) {
        printf("i %d intersects j %d in y\n", i, j);
        if(INTERSECTS_BEFORE(free_spaces.list[j].x, free_spaces.list[j].w, used_spaces->list[i].x, used_spaces->list[i].w) ) {
          //the free space comes before the placed rectangle, modify it's RHS    
          new = free_spaces.list[j];
          new.w += new.x;
          new.x = used_spaces->list[i].x + used_spaces->list[i].w;
          new.w -= new.x;
          add_rectangle(&new_spaces, new);
          add_rectangle(&old_spaces, free_spaces.list[j]);          
          printf("LHS of free space j %d. w was %d, is %d\n", j, free_spaces.list[j].w, new.w);
        }
        if(INTERSECTS_AFTER(free_spaces.list[j].x, free_spaces.list[j].w, used_spaces->list[i].x, used_spaces->list[i].w) ) {
          //the free space comes after the placed rectangle, modify it's LHS
          new = free_spaces.list[j];
          new.w = used_spaces->list[i].x - new.x;
          add_rectangle(&new_spaces, new);
          add_rectangle(&old_spaces, free_spaces.list[j]);    
          printf("RHS of free space j %d. w was %d, is %d\n", j, free_spaces.list[j].w, new.w);
        }
      }
      if(INTERSECTS(free_spaces.list[j].x, free_spaces.list[j].w, used_spaces->list[i].x, used_spaces->list[i].w) ) {
        printf("i %d intersects j %d in x\n", i, j);
        if(INTERSECTS_BEFORE(free_spaces.list[j].y, free_spaces.list[j].h, used_spaces->list[i].y, used_spaces->list[i].h) ) {
          //the free space comes before the placed rectangle, modify it's Top
          new = free_spaces.list[j];
          new.h += new.y;
          new.y = used_spaces->list[i].y + used_spaces->list[i].h;
          new.h -= new.y;
          add_rectangle(&new_spaces, new);
          add_rectangle(&old_spaces, free_spaces.list[j]);          
          printf("bottom of free space j %d. h was %d, is %d\n", j, free_spaces.list[j].h, new.h);
        }
        if(INTERSECTS_AFTER(free_spaces.list[j].y, free_spaces.list[j].h, used_spaces->list[i].y, used_spaces->list[i].h) ) {
          //the free space comes before the placed rectangle, modify it's Bottom
          new = free_spaces.list[j];
          new.h = used_spaces->list[i].y - new.y;
          add_rectangle(&new_spaces, new);
          add_rectangle(&old_spaces, free_spaces.list[j]);          
          printf("top of free space j %d. h was %d, is %d\n", j, free_spaces.list[j].h, new.h);          
        }
      }

    }
    if(new_spaces.used != 0) {
      //remove the spaces which were modified. O(N*M)
      for(int k = 0; k < old_spaces.used; k++) remove_rectangle(&free_spaces, old_spaces.list[k]);
      
      //add new_spaces to the free spaces O(N*M)
      for(int k = 0; k < new_spaces.used; k++) add_rectangle(&free_spaces, new_spaces.list[k]);
      
      free(new_spaces.list);
      free(old_spaces.list);
    }
  }
  free(used_spaces->list);
  return free_spaces;
}

/* Adds rectangles to the list.  Rectangles are checked for validity and independence/inclusion. O(n) */
void add_rectangle(struct rectangle_list *list, struct rectangle new) {
  if(new.h <= 0  ||  new.w <= 0  ||  new.x < 0  ||  new.y < 0) return;
  if(list->max == list->used) {
    struct rectangle *temp = NULL;
    temp = realloc(list->list, sizeof(struct rectangle) * list->max * 2);
    if(temp != NULL) list->list = temp;
    else return;
    list->list = temp;
    list->max *= 2;
  }

  //start from the end in case it was just added. Don't add rectangles which are already covered/included
  for(int i = list->used - 1; i >= 0; i--) 
  if(list->list[i].x <= new.x 
  && list->list[i].y <= new.y
  && list->list[i].w + list->list[i].x >= new.w + new.x
  && list->list[i].h + list->list[i].y >= new.h + new.y) return; 

    
  list->list[list->used] = new;
  list->used++;
}

/* Removes the rectangle from the list if it exists. O(n)*/
void remove_rectangle(struct rectangle_list *list, struct rectangle old) {
  int i;
  for(i = 0; i < list->used; i++)
  if(list->list[i].x == old.x
  && list->list[i].y == old.y
  && list->list[i].w == old.w
  && list->list[i].h == old.h) break;  

  if(i == list->used) return;
  
  if((list->used != 1) && (i != list->used - 1)) { //not the first or the last
    list->list[i] = list->list[list->used - 1];    //swap the deleted item with the last item
  }
  list->used--;
}


