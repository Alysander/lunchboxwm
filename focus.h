void add_focus             (Window new, struct Focus_list *focus);
void remove_focus          (Window old, struct Focus_list *focus);
void check_new_frame_focus (Display *display, struct Frame_list *frames, int index);
void unfocus_frames        (Display *display, struct Frame_list *frames);
void recover_focus         (Display *display, struct Frame_list *frames, struct Themes *themes);
