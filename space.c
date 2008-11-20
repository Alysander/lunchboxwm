/****
Workspaces
   - status windows are "global windows". They simply remain on the screen across workspaces.
   - file windows can be shared.  However, they are technically reopened in different workspaces.
     -  how is the "opening program" determined?
        - open in current program?

Dialog boxes should not have a title menu - this could too easily result in them being lost.
Splash screens should not have close button.

Program/workspace can be a variable in frame.  
Whenever free space is calculated, just skip the irrelevent ones.

(utility windows do and therefore can have the mode list too - but no close button)
   
****/


struct rectangle_list {
  int used;
  int max;
  rectangle *list;
}

/**
calling function identifies tiled windows in workspace and "global" windows - the "used_spaces".
It also identifies the initial space - the screen.

This function then returns the largest available spaces.
**/
struct *rectangle_list largest_available_spaces (struct rectangle_list *used_spaces, struct rectangle_list *initial_space) {
  struct rectangle_list new_spaces = {0, 16, NULL};
  
  if(used_spaces->list == NULL  ||  initial_space->list == NULL) {
    printf("Error:  find_largest_spaces called incorrectly\n");
    return;
  }  
  new_spaces.list = malloc(new_spaces.max * sizeof(rectangle_list));

  if(new_spaces.list == NULL) {
    //probably could do something more intelligent here
    printf("Error: out of memory for calculating free spaces on the screen.\n");
    return;
  }
  
}

void add_rectangle(struct rectangle_list *list, rectangle new) {
  if(list->max == list->used) {
    struct rectangle_list *temp = NULL;
    temp = realloc(list->list, sizeof(struct rectangle_list) * frames->max * 2);
    if(temp != NULL) frames->list = temp;
    else return;
    list->list = temp;
    list->max *= 2;
  }
  
  for(int i = 0; i < list.used; i++)
  if(list->list[i].x == new.x
  && list->list[i].y == new.y
  && list->list[i].w == new.w
  && list->list[i].h == new.h) break;

  list->list[used] = new;
  list->used++;
}

void remove_rectangle(struct rectangle_list *list, int i) {
  if((list->used != 1) && (i != list->used - 1)) { //not the first or the last
    list->list[i] = list->list[list->used - 1]; //swap the deleted item with the last item
  }
  list->used--;
}

/*

for every "used" space which is looked at, 
if it intersects then the original space is removed, and the new splits are added, otherwise look at next original space.
  - function to add space 
  - function to remove space.
It is inevitable that the "used" space intersects something - the total space doesn't grow.

*/

/*
new_spaces = empty - new list of rectangles
remove_spaces = empty  - list of indices
current_spaces = list of rectangles (just the screen initially)s

for all open tiled windows
  
  find current_spaces which are covered with the LHS of a window - add each to B
  if(( (x >= current_spaces[i].x  &&  x < current_spaces[i].x + current_spaces[i].w)
    || (x <  current_spaces[i].x  &&  x + w > current_spaces[i].x ))
  && ( (y >= current_spaces[i].y  &&  y < current_spaces[i].y + current_spaces[i].h)
    || (y <  current_spaces[i].y  &&  y + h > current_spaces[i].y ))
      
    modify_spaces(current_spaces, edge (x,y,w,h where h or w = 0, LHS, &new_spaces)
    
  repeat for LHS, RHS, Top, Bottom 
    

modify spaces:
     
     make the side of a window the "inner edge" of the space and add to new_spaces.


   
*/
