
int supress_xerror          (Display *display, XErrorEvent *event);
int create_frame            (Display *display, struct Frame_list* frames, Window framed_window
, struct Popup_menu *window_menu, struct Themes *themes, struct Cursors *cursors, struct Atoms *atoms);
void get_frame_hints        (Display* display, struct Frame* frame); //there might be a property notify
void remove_frame           (Display *display, struct Frame_list* frames, int index, struct Themes *themes);
void close_window           (Display *display, Window framed_window);
void free_frame_name        (struct Frame *frame);
void create_frame_name      (Display *display, struct Popup_menu *window_menu, struct Frame* frame, struct Themes *themes);
void resize_frame           (Display *display, struct Frame *frame, struct Themes *themes);
//void create_frame_icon_size (Display *display, struct Frame *frame, int new_size);
//void free_frame_icon_size   (Display *display, struct Frame *frame);

