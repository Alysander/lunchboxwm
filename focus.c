#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/extensions/shape.h>
#include <X11/Xlib.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/Xatom.h>

#include "lunchbox.h"

#include "frame.h"
#include "frame-actions.h"
#include "focus.h"
#include "xcheck.h"

/****

(implemented in add_frame_to_workspace called from main)
Give focus to new windows if
 1) It is the only window in the workspace
 2) It is a transient window and it's parent has focus.

(implemented in UnmapNotify case in main)
Remove focus when any frame is closed 
Remove focus and change it to previously focussed window when:
 1) The currently focussed frame is closed.

(implemented in ButtonRelease case in main)
 2) The currently focussed frame is sunk
 3) A window is replaced using the title menu with another window.

(same section as "window is replaced" above)
Remove focus if any window is hidden - because giving focus shouldn't have to raise a window.

(implemented in ButtonPress) 
Change focus to another window whenever the user clicks on it an
****/

void add_focus(Window new, struct Focus_list* focus) {
  remove_focus(new, focus); //remove duplicates
  if(focus->used == focus->max  ||  focus->list == NULL) {
    Window *temp = NULL;
    if(focus->list != NULL) temp = realloc(focus->list, sizeof(Window) * focus->max * 2);
    else {
      temp = malloc(sizeof(Window) * focus->max);
      focus->max = focus->max / 2;
    }
    
    if(temp != NULL) {
      focus->list = temp;
      focus->max *= 2;
    }
    else if (focus->list == NULL) return;      //it doesn't really matter if there is no focus history
    else remove_focus(focus->list[0], focus);  //if out of memory overwrite oldest value
  }
  focus->list[focus->used] = new;
  focus->used++;
}

void remove_focus(Window old, struct Focus_list* focus) {
  int i;
  if(focus->list == NULL || focus->used == 0) return;
  //recently added windows are more likely to be removed
  for( i = focus->used - 1; i >= 0; i--) if(focus->list[i] == old) break;
  if(i < 0) return; //not found
  focus->used--;
  //we know that i is unsigned at this stage
  for( ; (unsigned int)i < focus->used; i++) focus->list[i] = focus->list[i + 1];
}

//this doesn't actually focus the window in case it is in the wrong workspace.
//caller must determine that and then run recover focus.
void check_new_frame_focus (Display *display, struct Frame_list *frames, int index) {
  struct Frame *frame = &frames->list[index];
  int set_focus = 0;
  
  if(frames->focus.used == 0) set_focus=1;
  else 
  if(frames->focus.used > 0  
  && frame->transient == frames->focus.list[frames->focus.used - 1]) { //parent has focus
    unfocus_frames(display, frames); //make frames look normal
    set_focus=1;
  }
  
  if(set_focus) {
    add_focus(frame->framed_window, &frames->focus);
    frame->selected = 1;    
    xcheck_raisewin(display, frames->list[index].widgets[selection_indicator].state[active]);
    XFlush(display);
  }
}

//deselect all the frames.
void unfocus_frames(Display *display, struct Frame_list *frames) {
  for(int i = 0; i < frames->used; i++) 
  if(frames->list[i].selected) {
    frames->list[i].selected = 0;
    xcheck_raisewin(display, frames->list[i].widgets[selection_indicator].state[normal]);
    XFlush(display);
  }
}


//So it turns out that if selection indicators are present this introduces a linear behaviour
//rather than a constant time one.  The alternative would be to always keep the frame list
//in focus order, but copying around reasonably large structs seems more expensive than
//zipping through them once in a while.
void recover_focus(Display *display, struct Frame_list *frames, struct Themes *themes) {
  if(frames->focus.used == 0) return;
  //printf("Recovering focus\n");
  for(int i = frames->used - 1; i >= 0; i--) 
  if(frames->list[i].framed_window == frames->focus.list[frames->focus.used - 1]) {
    //_NET_ACTIVE_WINDOW
#ifdef CRASH_ON_BUG
    XGrabServer(display);
    XSetErrorHandler(supress_xerror);
#endif
    //seems excessive but closing windows can cause bad window errors
    XSetInputFocus(display, frames->list[i].framed_window, RevertToPointerRoot, CurrentTime);
#ifdef CRASH_ON_BUG
    XSync(display, False);
    XSetErrorHandler(NULL);    
    XUngrabServer(display);
#endif
    XFlush(display);
    frames->list[i].selected = 1;
    change_frame_mode(display, &frames->list[i], frames->list[i].mode, themes);
    break;
  }
  
}
