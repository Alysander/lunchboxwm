void check_frame_limits     (Display *display, struct Frame *frame);
void change_frame_mode      (Display *display, struct Frame *frame, enum Window_mode mode, struct Themes *themes);
void unsink_frame           (Display *display, struct Frame_list *frames, int index, struct Themes *themes);
void drop_frame             (Display *display, struct Frame_list *frames, int clicked_frame, struct Themes *themes);

void resize_nontiling_frame (Display *display, struct Frame_list *frames, int clicked_frame
, int pointer_start_x, int pointer_start_y, int mouse_root_x, int mouse_root_y
, int r_edge_dx, int b_edge_dy, Window clicked_widget, struct Themes *themes);

void move_frame             (Display *display, struct Frame *frame
, int *pointer_start_x, int *pointer_start_y, int mouse_root_x, int mouse_root_y
, int *resize_x_direction, int *resize_y_direction, struct Themes *themes);

void resize_tiling_frame    (Display *display, struct Frame_list* frames, int index, char axis, int position, int size, struct Themes *themes);
void stack_frame            (Display *display, struct Frame *frame, Window sinking_seperator, Window tiling_seperator, Window floating_seperator);

int replace_frame           (Display *display, struct Frame *target
, struct Frame *replacement, Window sinking_seperator, Window tiling_seperator, Window floating_seperator, struct Themes *themes);  

