/*** workspaces.c ***/
int create_frame_list        (Display *display, struct Workspace_list* workspaces, char *workspace_name, struct Themes *themes, struct Cursors *cursors);
void remove_frame_list       (Display *display, struct Workspace_list* workspaces, int index);
int create_startup_workspaces(Display *display, struct Workspace_list *workspaces
, Window sinking_seperator, Window tiling_seperator, Window floating_seperator
, struct Popup_menu *window_menu, struct Themes *themes, struct Cursors *cursors, struct Atoms *atoms);

int add_frame_to_workspace(Display *display, struct Workspace_list *workspaces, Window window, int current_workspace
, struct Popup_menu *window_menu
, Window sinking_seperator, Window tiling_seperator, Window floating_seperator
, struct Themes *themes, struct Cursors *cursors, struct Atoms *atoms);

void change_to_workspace     (Display *display, struct Workspace_list *workspaces, int *current_workspace, int index, struct Themes *themes);
