
/*** Create Workspace ***/
int create_frame_list(Display *display, struct Workspace_list* workspaces, char *workspace_name, struct Pixmaps *pixmaps, struct Cursors *cursors) {
  Window root = DefaultRootWindow(display);
  Screen* screen =  DefaultScreenOfDisplay(display);  
  int black = BlackPixelOfScreen(screen);
  XSetWindowAttributes attributes; 
  struct Frame_list *frames;

  Pixmap item_title_p
  , item_title_active_p
  , item_title_deactivated_p
  , item_title_hover_p
  , item_title_active_hover_p
  , item_title_deactivated_hover_p;
    
  
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
  , XWidthOfScreen(screen), XHeightOfScreen(screen) - MENUBAR_HEIGHT, 0, black, black);
  attributes.cursor = cursors->normal;
  attributes.override_redirect = True;
  attributes.background_pixmap = ParentRelative;
  XChangeWindowAttributes(display, frames->virtual_desktop 
  , CWOverrideRedirect | CWBackPixmap | CWCursor , &attributes);
  XLowerWindow(display, frames->virtual_desktop);

  frames->workspace_menu.backing = XCreateSimpleWindow(display, workspaces->workspace_menu, 10, 10
  , XWidthOfScreen(screen), MENU_ITEM_HEIGHT, 0, black, black);
  XSelectInput(display, frames->workspace_menu.backing, ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);
  XMapWindow(display, frames->workspace_menu.backing);
  
  //create the pixmaps
  item_title_p = create_title_pixmap(display, frames->workspace_name, item_title);
  item_title_active_p = create_title_pixmap(display, frames->workspace_name, item_title_active);
  item_title_deactivated_p = create_title_pixmap(display, frames->workspace_name, item_title_deactivated);
  item_title_hover_p = create_title_pixmap(display, frames->workspace_name, item_title_hover);
  item_title_active_hover_p = create_title_pixmap(display, frames->workspace_name, item_title_active_hover);
  item_title_deactivated_hover_p = create_title_pixmap(display, frames->workspace_name, item_title_deactivated_hover);

  //Create the windows for the different states of the menu item
  frames->workspace_menu.item_title = 
  XCreateSimpleWindow(display, frames->workspace_menu.backing, 0, 0, XWidthOfScreen(screen), MENU_ITEM_HEIGHT, 0, black, black);
  frames->workspace_menu.item_title_active =
  XCreateSimpleWindow(display, frames->workspace_menu.backing, 0, 0, XWidthOfScreen(screen), MENU_ITEM_HEIGHT, 0, black, black);
  frames->workspace_menu.item_title_deactivated =
  XCreateSimpleWindow(display, frames->workspace_menu.backing, 0, 0, XWidthOfScreen(screen), MENU_ITEM_HEIGHT, 0, black, black);
  frames->workspace_menu.item_title_hover =
  XCreateSimpleWindow(display, frames->workspace_menu.backing, 0, 0, XWidthOfScreen(screen), MENU_ITEM_HEIGHT, 0, black, black);
  frames->workspace_menu.item_title_active_hover =
  XCreateSimpleWindow(display, frames->workspace_menu.backing, 0, 0, XWidthOfScreen(screen), MENU_ITEM_HEIGHT, 0, black, black);
  frames->workspace_menu.item_title_deactivated_hover =
  XCreateSimpleWindow(display, frames->workspace_menu.backing, 0, 0, XWidthOfScreen(screen), MENU_ITEM_HEIGHT, 0, black, black);

  XSetWindowBackgroundPixmap(display, frames->workspace_menu.item_title, item_title_p);
  XSetWindowBackgroundPixmap(display, frames->workspace_menu.item_title_active, item_title_active_p);
  XSetWindowBackgroundPixmap(display, frames->workspace_menu.item_title_deactivated, item_title_deactivated_p);
  XSetWindowBackgroundPixmap(display, frames->workspace_menu.item_title_hover, item_title_hover_p);
  XSetWindowBackgroundPixmap(display, frames->workspace_menu.item_title_active_hover, item_title_active_hover_p);
  XSetWindowBackgroundPixmap(display, frames->workspace_menu.item_title_deactivated_hover, item_title_deactivated_hover_p);

  XRaiseWindow(display,  frames->workspace_menu.item_title);
  XMapWindow(display, frames->workspace_menu.item_title);
  XMapWindow(display, frames->workspace_menu.item_title_active);
  XMapWindow(display, frames->workspace_menu.item_title_deactivated);
  XMapWindow(display, frames->workspace_menu.item_title_hover);
  XMapWindow(display, frames->workspace_menu.item_title_active_hover);
  XMapWindow(display, frames->workspace_menu.item_title_deactivated_hover);

  XFreePixmap(display, item_title_p);
  XFreePixmap(display, item_title_active_p);
  XFreePixmap(display, item_title_deactivated_p);
  XFreePixmap(display, item_title_hover_p);
  XFreePixmap(display, item_title_active_hover_p);
  XFreePixmap(display, item_title_deactivated_hover_p);

  frames->workspace_menu.width = get_title_width(display, frames->workspace_name);
  
  //Create the window menu for this workspace
  create_title_menu(display, frames, pixmaps, cursors);

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
  frames->focus.max = 8; //must be divisible by 2
  frames->focus.list = NULL;
  frames->focus.list = malloc(sizeof(struct Focus_list) * frames->focus.max); //ok if it fails.
  #ifdef SHOW_STARTUP
  printf("Created workspace %d\n", workspaces->used);
  #endif
  
  workspaces->used++;
  XSync(display, False);
  return workspaces->used - 1;
}

/*  This is called when the wm is exiting, it doesn't close the open windows. */
void remove_frame_list(Display *display, struct Workspace_list* workspaces, int index) {

  if(index >= workspaces->used) return;
  struct Frame_list *frames = &workspaces->list[index];
  
  //close all the frames
  for(int k = frames->used - 1; k >= 0; k--) remove_frame(display, frames, k);
  free(frames->list);
  if(frames->workspace_name != NULL) {
    XFree(frames->workspace_name);
  }
  //remove the focus list
  //close the background window
  //keep the open workspaces in order
  workspaces->used--;
  XDestroyWindow(display, frames->virtual_desktop);
  XDestroyWindow(display, frames->title_menu);
  XDestroyWindow(display, frames->workspace_menu.backing);

  for(int i = index ; i < workspaces->used; i++) workspaces->list[i] = workspaces->list[i + 1];
}

//returned pointer must be freed with XFree.
char *load_program_name(Display* display, Window window) {
  XClassHint program_hint;
  if(XGetClassHint(display, window, &program_hint)) {
   // printf("res_name %s, res_class %s\n", program_hint.res_name, program_hint.res_class);
    if(program_hint.res_name != NULL) XFree(program_hint.res_name);    
    if(program_hint.res_class != NULL)
      return program_hint.res_class;
  }
  return NULL;
}

char *make_default_program_name(Display *display, Window window, char *name) {
  XClassHint program_hint;
  program_hint.res_name = name;
  program_hint.res_class = name;
  XSetClassHint(display, window, &program_hint);
  XFlush(display);
}

/* creates the workspace */
int add_frame_to_workspace(Display *display, struct Workspace_list *workspaces, Window window, int current_workspace
, Window sinking_seperator, Window tiling_seperator, Window floating_seperator
, struct Pixmaps *pixmaps, struct Cursors *cursors, struct Atoms *atoms) {
  char *program_name = load_program_name(display, window);
  int k;
  int frame_index = -1;  
  if(program_name == NULL) {
    #ifdef SHOW_MAP_REQUEST_EVENT
    printf("Error, could not load program name for window %lu\n", window);  
    printf("Creating default workspace\n");
    #endif
    make_default_program_name(display, window, "Other Programs");
    program_name = load_program_name(display, window);
  }
  
  for(k = 0; k < workspaces->used; k++) {
    if(strcmp(program_name, workspaces->list[k].workspace_name) == 0) {
      XFree(program_name);
      break;
    }
  }
  if(k == workspaces->used) {
    k = create_frame_list(display, workspaces, program_name, pixmaps, cursors);
    if(k < 0) {
      #ifdef SHOW_MAP_REQUEST_EVENT   
      printf("Error, could not create new workspace\n");
      #endif
      return -1;
    }
  }
  frame_index = create_frame(display, &workspaces->list[k], window, pixmaps, cursors, atoms);
  if(frame_index != -1) {
    check_new_frame_focus (display, &workspaces->list[k], frame_index);
    stack_frame(display, &workspaces->list[k].list[frame_index], sinking_seperator, tiling_seperator, floating_seperator);
    if(k == current_workspace  &&  current_workspace != -1) {
      //printf("Created and mapped window\n");
      XMapWindow(display, workspaces->list[k].list[frame_index].frame);
      if(workspaces->list[k].list[frame_index].selected != 0) {
       // printf("Set focus to window %s\n", workspaces->list[k].list[frame_index].window_name);
        XSetInputFocus(display, workspaces->list[k].list[frame_index].window, RevertToPointerRoot, CurrentTime);
      }
    }
    XFlush(display);
  }
  return k;
}

int create_startup_workspaces(Display *display, struct Workspace_list *workspaces
, Window sinking_seperator, Window tiling_seperator, Window floating_seperator
, struct Pixmaps *pixmaps, struct Cursors *cursors, struct Atoms *atoms) {
  unsigned int windows_length;
  Window root, parent, children, *windows;
  XWindowAttributes attributes;
  root = DefaultRootWindow(display);

  create_workspaces_menu(display, workspaces, pixmaps, cursors);  

  XQueryTree(display, root, &parent, &children, &windows, &windows_length);  
  
  if(windows != NULL) for(int i = 0; i < windows_length; i++)  {
    XGetWindowAttributes(display, windows[i], &attributes);
    if(attributes.map_state == IsViewable && !attributes.override_redirect)  {
      add_frame_to_workspace(display, workspaces, windows[i], -1
      , sinking_seperator, tiling_seperator, floating_seperator, pixmaps, cursors, atoms);
    }
  }
  XFree(windows);
  return 1;
}

void change_to_workspace(Display *display, struct Workspace_list *workspaces, int *current_workspace, int index, struct Pixmaps *pixmaps) {
  //Window sinking_seperator, Window tiling_seperator, Window floating_seperator
  if(index < workspaces->used) {
    struct Frame_list *frames;
    if(*current_workspace < workspaces->used  &&  *current_workspace >= 0) {
      frames = &workspaces->list[*current_workspace];
      XUnmapWindow(display, frames->virtual_desktop);
      for(int i = 0; i < frames->used; i++) XUnmapWindow(display, frames->list[i].frame);     
    }
    //if index == -1, change to default workspace
    if(index < 0  &&  workspaces->used >= 0) index = 0;
    
    frames = &workspaces->list[index];
    XMapWindow(display, frames->virtual_desktop);
    for(int i = 0; i < frames->used; i++) 
    if(frames->list[i].mode != hidden) XMapWindow(display, frames->list[i].frame); 
    
   // printf("changing focus to one in new workspace\n");
    recover_focus(display, frames);
    XFlush(display);
    *current_workspace = index;
  }
}
