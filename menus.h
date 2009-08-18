void create_menubar        (Display *display, struct Menubar *menubar, struct Seperators *seps,  struct Themes *themes, struct Cursors *cursors);
void create_mode_menu      (Display *display, struct Mode_menu *mode_menu,                       struct Themes *themes, struct Cursors *cursors);
void create_title_menu     (Display *display, struct Popup_menu *window_menu,                    struct Themes *themes, struct Cursors *cursors);
void create_workspaces_menu(Display *display, struct Workspace_list *workspaces,                 struct Themes *themes, struct Cursors *cursors);
void show_workspace_menu   (Display *display, Window calling_widget, struct Workspace_list* workspaces, int index, int x, int y, struct Themes *themes);
void show_title_menu       (Display *display, struct Popup_menu *window_menu, Window calling_widget, struct Frame_list *frames, int index, int x, int y, struct Themes *themes);
void show_mode_menu        (Display *display, Window calling_widget, struct Mode_menu *mode_menu, struct Frame *active_frame, int x, int y);  
void place_popup_menu      (Display *display, Window calling_widget, Window popup_menu, int x, int y);
void resize_popup_menu     (Display *display, struct Popup_menu *menu, struct Themes *themes);
void create_popup_menu     (Display *display, struct Popup_menu *menu, struct Themes *themes, struct Cursors *cursors);
