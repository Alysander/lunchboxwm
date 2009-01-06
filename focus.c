void add_focus(Window new, struct Focus_list* focus) {
  remove_focus(new, focus); //remove duplicates
  if(focus->used == focus->max  ||  focus->list == NULL) {
    Window *temp = NULL;
    if(focus->list != NULL) temp = realloc(focus->list, sizeof(Window) * focus->max * 2);
    else {
      focus->used = 0;
      focus->max = 4;
      temp = malloc(sizeof(Window) * focus->max * 2);
    }
    
    if(temp != NULL) {
      focus->list = temp;
      focus->max *= 2;
    }
    else remove_focus(focus->list[0], focus);  //if out of memory start overwriting old values
  }
  focus->list[focus->used] = new;
  focus->used++;
}

void remove_focus(Window old, struct Focus_list* focus) {
  int i;

  //recently added windows are more likely to be removed
  for( i = focus->used - 1; i >= 0; i--) if(focus->list[i] == old) break;
  if(i < 0) return; //not found
  focus->used--;
  for( ; i < focus->used; i++) focus->list[i] = focus->list[i + 1];
}
