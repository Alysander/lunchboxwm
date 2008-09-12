#include "basin.h"
#include "draw.c"

void find_adjacent (struct Framelist* frames, int frame_index);   

extern void draw_background(Display* display, Window backwin); //this draws a solid colour background
extern void draw_frame (Display* display, struct Frame frame);
extern void draw_closebutton(Display* display, Window window);
int create_frame (Display* display, struct Framelist* frames, Window window); //returns the frame index of the newly created window or -1 if out of memory

int main (int argc, char* argv[]) {
  Display* display = NULL; 
  Screen* screen;  
  XEvent event;
  Window root, backwin;
  int start_move_x, start_move_y, start_move_index;
  start_move_index = -1;
  struct Framelist frames = {NULL, 16, 0};
  frames.list = malloc(sizeof(struct Framelist) * 16);

  printf("\n");
  if(argc > 1) {
    printf("Opening Basin on Display: %s\n", argv[1]);
    printf("This seems to hang if you give a wrong screen\n");
    display = XOpenDisplay(argv[1]);
  }
  else display = XOpenDisplay(NULL);
  
  if(display == NULL)  {
    printf("Could not open display.\n");
    printf("USAGE: basin [DISPLAY]\n");
    return -1;
  }
  
  root = DefaultRootWindow(display);
  screen = DefaultScreenOfDisplay(display);  
  backwin = 
    XCreateSimpleWindow(display, root, 0, 0, XWidthOfScreen(screen), XHeightOfScreen(screen), 
      0, WhitePixelOfScreen(screen), BlackPixelOfScreen(screen)); 
      
  XMapWindow(display, backwin);
  draw_background(display, backwin);
  
  XSelectInput(display, root, SubstructureRedirectMask);
  XSelectInput(display, backwin, ExposureMask);

  for( ; ;) {
    XNextEvent(display, &event);
    switch(event.type) {
    
      case MapRequest:
        printf("A top level window is being mapped: %d \n", event.xmaprequest.window); 
        //Do we need to check if the window is already mapped?
        int index = create_frame(display, &frames, event.xmaprequest.window);
        if(index != -1) {
          draw_frame(display, frames.list[index]);
        }
      break;
      
      case Expose:
        if(event.xexpose.window != backwin) printf("Expose event on window: %d\n", event.xexpose.window);
        if(event.xexpose.window == backwin)  draw_background(display, backwin);
        else {
          for(int i = 0; i < frames.used; i++) {
            if(event.xexpose.window == frames.list[i].frame) { 
              draw_frame(display, frames.list[i]);
              break;
            }
            else if (event.xexpose.window == frames.list[i].closebutton) { 
              draw_closebutton(display, frames.list[i].closebutton);
              break;
            }
          }
        }
      break;
      
      case ButtonPress:
        //Change the cursor
        for(int i = 0; i < frames.used; i++) {
          if(event.xbutton.window == frames.list[i].frame) { //start the move
          int initial_x, initial_y;
            XGrabPointer(display, event.xbutton.window, True, PointerMotionMask|ButtonReleaseMask, GrabModeAsync,  GrabModeAsync, None, None, CurrentTime);
            start_move_x = event.xbutton.x;
            start_move_y = event.xbutton.y;
            start_move_index = i;
            break;
          }
        }
      break;
      
      case MotionNotify: 
        if(event.xmotion.window != root) { //actually need to check that it isn't the resize grips
          while(XCheckTypedEvent(display, MotionNotify, &event));
          Window mouse_root, mouse_child;
          int mouse_root_x, mouse_root_y;
          int mouse_child_x, mouse_child_y;
          unsigned int mask;
          
          XQueryPointer(display, root, &mouse_root, &mouse_child, &mouse_root_x, &mouse_root_y, &mouse_child_x, &mouse_child_y, &mask);        
          frames.list[start_move_index].x = mouse_root_x - start_move_x;
          frames.list[start_move_index].y = mouse_root_y - start_move_y;
          XMoveWindow(display, event.xmotion.window, frames.list[start_move_index].x, frames.list[start_move_index].y);
        }
      break;
      
      case ButtonRelease:
         XUngrabPointer(display, CurrentTime);
         start_move_index = -1;
      break;

      case UnmapNotify:
        printf("Unmap notify %d\n", event.xunmap.window);
        for(int i = 0; i < frames.used; i++) {
          if(event.xunmap.window == frames.list[i].window) {
            XUnmapWindow (display, frames.list[i].frame);
            XUnmapWindow (display, frames.list[i].closebutton);
          }
          break;
        }
      break;
    }
  }
  XCloseDisplay(display);
  //go through and unmap the windows
  free(frames.list);
  return 1;  
}

int create_frame(Display* display, struct Framelist* frames, Window window) { 
  Window root = DefaultRootWindow(display);
  Screen* screen = DefaultScreenOfDisplay(display);
  //display is used to get screen width and height 
  
  if(frames->used == frames->max) {
    struct Frame* temp = NULL;
    temp = realloc(frames->list, 2*sizeof(struct Framelist)*frames->max);
    if(temp != NULL) frames->list = temp;
    else return -1;
  }
  
  //TODO: get details from WM hints if possible, these are defaults:
  struct Frame frame;
  frame.window = window;
  frame.w = MINWIDTH*3;
  frame.h = MINHEIGHT*3;
  frame.x = (XWidthOfScreen(screen) - frame.w); //RHS of the screen
  frame.y = 30; //below menubar
  frame.min_width = MINWIDTH;
  frame.min_height = MINHEIGHT;
  frame.max_width = XWidthOfScreen(screen);
  frame.max_height = XHeightOfScreen(screen);

  frame.frame =
    XCreateSimpleWindow(display, root, frame.x, frame.y, frame.w, frame.h, 0,
      WhitePixelOfScreen(screen), BlackPixelOfScreen(screen));
  printf("create_frame, frame.frame = %d\n", frame.frame);    
  printf("create_frame, frame.window = %d\n", frame.window);
  frame.closebutton =
    XCreateSimpleWindow(display, frame.frame, frame.w-20-8-1, 4, 20, 20, 0,
      WhitePixelOfScreen(screen), BlackPixelOfScreen(screen));
  printf("create_frame, frame.closebutton = %d\n", frame.closebutton);      
  
  XSetWindowBorderWidth(display, frame.window, 0);
  XResizeWindow(display, frame.window, frame.w - FRAME_HSPACE, frame.h - FRAME_VSPACE);
  XAddToSaveSet(display, frame.window); //this means the window will survive after the connection is closed
  XReparentWindow(display, window, frame.frame, FRAME_XSTART, FRAME_YSTART);
  
  //and the input only hotspots    
  //and set the mode    
  //need to establish how to place a new window
  XMapWindow(display, frame.frame);
  XMapWindow(display, frame.closebutton);
  XMapWindow(display, frame.window);

  //use the property notify to update titles  
  XSelectInput(display, frame.window, StructureNotifyMask);
  XSelectInput(display, frame.frame, Button1MotionMask | ButtonPressMask | ExposureMask);
  XSelectInput(display, frame.closebutton, ButtonPressMask | ExposureMask);  
  frames->list[frames->used] = frame;
  frames->used++;
  return (frames->used - 1);
} 

