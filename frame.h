
int supress_xerror          (Display *display, XErrorEvent *event);
int create_frame            (Display *display, struct Frame_list* frames, Window framed_window
, struct Popup_menu *window_menu, struct Themes *themes, struct Cursors *cursors, struct Atoms *atoms);

void create_frame_subwindows(Display *display, struct Frame *frame, struct Themes *themes, struct Cursors *cursors);
void remove_frame           (Display *display, struct Frame_list* frames, int index);
void close_window           (Display *display, Window framed_window);
void free_frame_name        (Display *display, struct Frame *frame);
void create_frame_name      (Display *display, struct Popup_menu *window_menu, struct Frame* frame, struct Themes *themes);
void get_frame_hints        (Display *display, struct Frame *frame);
void get_frame_wm_hints     (Display *display, struct Frame *frame);
void get_frame_type         (Display *display, struct Frame *frame, struct Atoms *atoms, struct Themes *themes);
void get_frame_state        (Display *display, struct Frame *frame, struct Atoms *atoms);
void resize_frame           (Display *display, struct Frame *frame, struct Themes *themes);
//void create_frame_icon_size (Display *display, struct Frame *frame, int new_size);
//void free_frame_icon_size   (Display *display, struct Frame *frame);

