struct Focuslist {
  int used, max;
  Window* list;
}

//caller allocates and frees the focus list.
int add_focus(Window new, Focuslist* focus) {
  if(focus->used == focus->max) {
    Window *temp = NULL;
    temp = realloc(focus->list, sizeof(Window) * focus->max * 2);
    if(temp != NULL) focus->list = temp;
    else return -1;
    focus->max *= 2;
  }
  focus->list[focus->used] = new;
  focus->used++;
  return 1;
}

void remove_focus(Window old, Focuslist* focus) {
  int i;
  for( i = 0; i < focus->used; i++) if(focus->list[i] == old) break;

  if(i == focus->used) return;

  focus->used--;
    
  if(!focus->used) return;

  for( ; i < focus->used; i++) focus->list[i] = focus->list[i + 1];
}

Window pop_focus(Window root, Focuslist* focus) {
  
  if(focus->used == 0) return root;
  focus->used--;
  return focus->list[focus->used];
}
