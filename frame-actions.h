void check_frame_limits     (struct Frame *frame, const struct Workarea *workarea,  struct Themes *themes);
void change_frame_mode      (Display *display, struct Frame *frame, enum Window_mode mode,  const struct Workarea *workarea,  struct Themes *themes);
void change_frame_state     (Display *display, struct Frame *frame, enum Window_state state, struct Separators *seps, const struct Workarea *workarea, struct Themes *themes, struct Atoms *atoms);
Bool redrop_frame           (Display *display, struct Workspace *frames, int clicked_frame, const struct Workarea *workarea,  struct Themes *themes);

Bool drop_frame             (struct Workspace *frames, int clicked_frame, Bool only_panels, const struct Workarea *workarea);

void change_frame_widget_state(Display* display, struct Frame* frame, enum Frame_widget widget, enum Widget_state state);
void resize_frame           (Display *display, struct Frame *frame, const struct Workarea *workarea, struct Themes *themes);

void resize_using_frame_grip (Display *display, struct Workspace *frames, int clicked_frame
, int pointer_start_x, int pointer_start_y, int mouse_root_x, int mouse_root_y
, int r_edge_dx, int b_edge_dy, Window clicked_widget, const struct Workarea *workarea, struct Themes *themes);

void move_frame             (Display *display, struct Frame *frame
, int *pointer_start_x, int *pointer_start_y, int mouse_root_x, int mouse_root_y
, int *resize_x_direction, int *resize_y_direction, const struct Workarea *workarea, struct Themes *themes);

void resize_tiling_frame    (Display *display, struct Workspace* frames, int index, char axis, int position, int size, const struct Workarea *workarea, struct Themes *themes);
void stack_frame            (Display *display, struct Frame *frame, struct Separators *seps);

int replace_frame           (Display *display, struct Frame *target
, struct Frame *replacement, struct Separators *seps, const struct Workarea *workarea, struct Themes *themes);

void maximize_frame         (Display *display, struct Workspace *frames, int clicked_frame, char direction, const struct Workarea *workarea, struct Themes *themes);

void reset_frame_titlebar   (Display *display, struct Frame *frame);
