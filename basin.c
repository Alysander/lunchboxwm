#include <errno.h>
#include <signal.h>

#include "basin.h"
#include "draw.c"

/*** draw.c ***/
extern void draw_background(Display* display, Window backwin, cairo_surface_t *surface); 
extern void draw_frame (Display* display, struct Frame frame);
extern void draw_closebutton(Display* display, struct Frame frame);

/*** basin.c ***/
void find_adjacent (struct Framelist* frames, int frame_index);
int create_frame (Display* display, struct Framelist* frames, Window framed_window); 
void create_startup_frames(Display *display, struct Framelist* frames);
void remove_frame(Display* display, struct Framelist* frames, int index);
void remove_window(Display* display, Window framed_window);
int supress_xerror(Display *display, XErrorEvent *event);
void print_frames(struct Framelist* frames);

int done = 0;
void end_event_loop(int sig) {
  printf("Quiting the window manager.\n Generate an event to continue.\n");
  /* set the global variable done to 1, this will terminate the main event loop */
  done = 1;
}

int main (int argc, char* argv[]) {
  Display* display = NULL; 
  Screen* screen;  
  Visual* colours;
  XEvent event;
  Window root, backwin;
  struct Framelist frames;

  frames.list = NULL;
  frames.max = 16;
  frames.used = 0;
  
  cairo_surface_t *backwin_s;

  int start_move_x, start_move_y, start_win;
  start_win = -1;
  int resize_x = 0; //-1 means LHS, 1 means RHS
  int resize_y = 0; //-1 means top, 1 means bottom
  frames.list = malloc(sizeof(struct Frame) * frames.max);

  printf("\n");
  if (signal(SIGINT, end_event_loop) == SIG_ERR) {
    printf("Could not set the error handler\n");
    return -1;
  }
  
  if(argc > 1) {
    printf("Opening Basin on Display: %s\n", argv[1]);
    printf("This can hang if you give the wrong screen\n");
    display = XOpenDisplay(argv[1]);
  }
  else display = XOpenDisplay(NULL);
  
  if(display == NULL)  {
    printf("Could not open display.\n");
    printf("USAGE: basin [DISPLAY]\n");
    return -1;
  }

  create_startup_frames(display, &frames);
    
  root = DefaultRootWindow(display);
  screen = DefaultScreenOfDisplay(display);
  colours = XDefaultVisual(display, DefaultScreen(display));

  backwin = XCreateSimpleWindow(display, root, 0, 0, XWidthOfScreen(screen), XHeightOfScreen(screen), 0, WhitePixelOfScreen(screen), BlackPixelOfScreen(screen)); 
  XLowerWindow(display, backwin);
  XMapWindow(display, backwin);
  backwin_s = cairo_xlib_surface_create(display, backwin, colours, XWidthOfScreen(screen), XHeightOfScreen(screen));
  draw_background(display, backwin, backwin_s);
  
  XSelectInput(display, root, SubstructureRedirectMask);
  XSelectInput(display, backwin, ExposureMask | ButtonPressMask | ButtonReleaseMask); //ButtonPress for deselecting windows, Button release for ending resize mode
  
  XSynchronize(display, True);
  while(!done) {
    if(XCheckTypedEvent(display, UnmapNotify, &event)) {
      printf("Doing an unmap first.\n"); //always check for unmaps first
    }
    else {
      XNextEvent(display, &event);
    }
    
    switch(event.type) {
    
      case MapRequest:
        printf("Mapping window: %d \n", event.xmaprequest.window); 
        //Do we need to check if the window is already mapped?
        int index;
        index = create_frame(display, &frames, event.xmaprequest.window);
        if(index != -1) draw_frame(display, frames.list[index]);
        
      break;
      
      case Expose:
  //      printf("Expose window: %d \n", event.xexpose.window);
        if(event.xexpose.window == backwin) draw_background(display, backwin, backwin_s);
        else {
          for(int i = 0; i < frames.used; i++) {
            if(event.xexpose.window == frames.list[i].frame) { 
              draw_frame(display, frames.list[i]);
              break;
            }
            else if (event.xexpose.window == frames.list[i].closebutton) { 
              draw_closebutton(display, frames.list[i]);
              break;
            }
          }
        }
      break;

      /*
      case DestroyNotify:
        printf("A Window window was destroyed: %d\n", event.xdestroywindow.window);
        for(int i = 0; i < frames.used; i++) {
          if(event.xdestroywindow.window == frames.list[i].window) {
            remove_frame(display, &frames, i);
          }
        }
      break;
      */
      
      case ButtonPress:
//        printf("ButtonPress %d\n", event.xbutton.window);
        //TODO: Change the cursor
        for(int i = 0; i < frames.used; i++) {
          //start the move
          if(event.xbutton.window == frames.list[i].frame) { 
            int initial_x, initial_y;
            XGrabPointer(display, event.xbutton.window, True, PointerMotionMask|ButtonReleaseMask, GrabModeAsync,  GrabModeAsync, None, None, CurrentTime);
            start_move_x = event.xbutton.x;
            start_move_y = event.xbutton.y;
            start_win = i;
            resize_x = 0;
            resize_y = 0;
            break;
          }
          else if(event.xbutton.window == frames.list[i].closebutton) {
            XSelectInput(display, frames.list[i].window, 0);
            XUnmapWindow(display, frames.list[i].window);            
            remove_window(display, frames.list[i].window);
            remove_frame(display, &frames, i);
            break;
          }
        }
      break;
      
      case MotionNotify: 
        if((event.xmotion.window != root)&& (event.xmotion.window != backwin) && (start_win != -1)) { 
          while(XCheckTypedEvent(display, MotionNotify, &event));
          Window mouse_root, mouse_child;
          int mouse_root_x, mouse_root_y;
          int mouse_child_x, mouse_child_y;
          unsigned int mask;
          
          XQueryPointer(display, root, &mouse_root, &mouse_child, &mouse_root_x, &mouse_root_y, &mouse_child_x, &mouse_child_y, &mask);
          frames.list[start_win].x = mouse_root_x - start_move_x;
          frames.list[start_win].y = mouse_root_y - start_move_y;
          
          int new_width = 0;
          int new_height = 0;
          
          if((frames.list[start_win].x + frames.list[start_win].w > XWidthOfScreen(screen)) //window moving off RHS
            ||(resize_x == -1)) {  
            new_width = XWidthOfScreen(screen) - frames.list[start_win].x;
            resize_x = -1;
          }
          if((frames.list[start_win].x < 0) //window moving off LHS
                 ||(resize_x == 1)) { 
            new_width = frames.list[start_win].w + frames.list[start_win].x;
            frames.list[start_win].x = 0;
            start_move_x = mouse_root_x;
            resize_x = 1;
          }

          if((frames.list[start_win].y + frames.list[start_win].h > XHeightOfScreen(screen)) //window moving off the bottom
           ||(resize_y == -1)) { 
            new_height = XHeightOfScreen(screen) - frames.list[start_win].y;
            resize_y = -1;
          }
          if((frames.list[start_win].y < 0) //window moving off the top of the screen
                ||(resize_y == 1)) { 
            new_height = frames.list[start_win].h + frames.list[start_win].y;
            frames.list[start_win].y = 0;
            start_move_y = mouse_root_y;
            resize_y = 1;
          }

          if((new_width != 0  &&  new_width < frames.list[start_win].min_width) 
           ||(new_height != 0  &&  new_height < frames.list[start_win].min_height)) {
             
            if(new_width != 0) new_width = 0;
            if(new_height != 0) new_height = 0;    
            //Set the state to Sinking
            //Don't retile
          }

          if((new_width != 0  &&  new_width > frames.list[start_win].max_width) //Don't allow too large resizes
           ||(new_height != 0  &&  new_height > frames.list[start_win].max_height)) {
            if(new_width > frames.list[start_win].max_width)   new_width = 0;
            if(new_height > frames.list[start_win].max_height) new_height = 0;
          }          
          
          if(new_width != 0  ||  new_height != 0) {
              
              if(new_width != 0) frames.list[start_win].w = new_width;
              if(new_height != 0) frames.list[start_win].h = new_height;
              
              XMoveResizeWindow(display, frames.list[start_win].frame, frames.list[start_win].x, frames.list[start_win].y,  frames.list[start_win].w, frames.list[start_win].h);
              cairo_xlib_surface_set_size(frames.list[start_win].frame_s, frames.list[start_win].w, frames.list[start_win].h);
              XResizeWindow(display, frames.list[start_win].window, frames.list[start_win].w - FRAME_HSPACE, frames.list[start_win].h - FRAME_VSPACE);
              
          }
          else {
            XMoveWindow(display, frames.list[start_win].frame, frames.list[start_win].x, frames.list[start_win].y);
          }
        }
        //add resize ability here
        
      break;
      
      case ButtonRelease:
         start_win = -1;
         resize_x = 0;
         resize_y = 0;
         XUngrabPointer(display, CurrentTime);
      break;

      case PropertyNotify:
        for(int i = 0; i < frames.used; i++) 
          if(event.xproperty.window == frames.list[i].window) {
            if( strcmp(XGetAtomName(display, event.xproperty.atom), "WM_NAME") == 0) {
             //NET_WM property is now preffered because it is UTF8
              XFetchName(display, frames.list[i].window, &frames.list[i].window_name);
              draw_frame(display, frames.list[i]);
            }
            break;
          }
      break;
      
      case UnmapNotify:      
        start_win = -1;
        printf("Unmap notify for window: %d\n", event.xunmap.window);
        print_frames(&frames);
        Window remove = event.xunmap.window;
        for(int i = 0; i < frames.used; i++) {
          printf("Event w: %d, frame window: %d\n", remove, frames.list[i].window);
          if(remove == frames.list[i].window) {
            printf("removed the window\n");
            remove_frame(display, &frames, i);
          }
          break;
        }
      break;
    }
  }
  
  printf(".......... \n");
  cairo_surface_destroy(backwin_s);
  for(int i = 0; i < frames.used; i++) {
    remove_window(display, frames.list[i].window);
    remove_frame(display, &frames, i);
  }
  XCloseDisplay(display);
  free(frames.list);
  return 1;  
}

void print_frames(struct Framelist* frames) {
  for(int i = 0; i < frames->used; i++) printf("Framed window %d is %d\n", i, frames->list[i].window);
}

/* Sometimes a client window is killed before it gets unmapped, but there is no way to tell so we just supress the error */
int supress_xerror(Display *display, XErrorEvent *event) {
  (void) display;
  (void) event;
  return 0;
}

/* This function reparents a framed window to root and then destroys the frame as well as cleaning up the frames drawing surfaces */
/* It is used when the framed window has been unmapped or destroyed, or is about to be*/
void remove_frame(Display* display, struct Framelist* frames, int index) {

  printf("removing window\n");
  cairo_surface_destroy(frames->list[index].frame_s);
  cairo_surface_destroy(frames->list[index].closebutton_s);  
  cairo_surface_destroy(frames->list[index].pulldown_s);  
  
  XGrabServer(display);
  XSetErrorHandler(supress_xerror);
  XReparentWindow(display, frames->list[index].window, DefaultRootWindow(display), 0, 0);
  XRemoveFromSaveSet (display, frames->list[index].window); 
  XDestroyWindow(display, frames->list[index].frame);
  XSync(display, False);
  XSetErrorHandler(NULL);    
  XUngrabServer(display);
  
  if(frames->list[index].window_name != NULL) XFree(frames->list[index].window_name);
   
  if((frames->used != 1) && (index != frames->used - 1)) { //the frame is not the first or the last
    frames->list[index] = frames->list[frames->used - 1]; //swap the deleted frame with the last frame
  }
  frames->used--;
}

/*This function is used to add frames to windows that are already open when the WM is starting*/
void create_startup_frames (Display *display, struct Framelist* frames) {
	unsigned int windows_length;
	Window parent, children, *windows, root;
	XWindowAttributes attr;
	int index;
  root = DefaultRootWindow(display);
  
	XQueryTree(display, root, &parent, &children, &windows, &windows_length);
  if(windows != NULL) for (int i = 0; i < windows_length; i++)  {
    XGetWindowAttributes(display, windows[i], &attr);
	  if (attr.map_state == IsViewable && !attr.override_redirect) 
	    index = create_frame(display, frames, windows[i]);
//		  if( index != -1) draw_frame(display, frames->list[index]);;
	}
	XFree(windows);
	
}

void remove_window(Display* display, Window framed_window) {
  int n, found = 0;
  Atom *protocols;
  
  if (XGetWMProtocols(display, framed_window, &protocols, &n)) {
  	for (int i = 0; i < n; i++)
  		if (protocols[i] == XInternAtom(display, "WM_DELETE_WINDOW", False)) {
  			found++;
  			break;
  	  }
  	XFree(protocols);
  }
  
  if(found)  {
  	XClientMessageEvent event;
  	event.type = ClientMessage;
  	event.window = framed_window;
  	event.format = 32;
  	event.message_type = XInternAtom(display, "WM_PROTOCOLS", False);
  	event.data.l[0] = (long)XInternAtom(display, "WM_DELETE_WINDOW", False);
	  event.data.l[1] = CurrentTime;
    XSendEvent(display, framed_window, False, NoEventMask, (XEvent *)&event);
  }
  else {
    printf("Killed client %d\n", framed_window);
    XKillClient(display, framed_window);
  }
  
}

/*returns the frame index of the newly created window or -1 if out of memory */
int create_frame(Display* display, struct Framelist* frames, Window framed_window) { 
  Window root = DefaultRootWindow(display);
  Screen* screen = DefaultScreenOfDisplay(display);
  Visual* colours = XDefaultVisual(display, DefaultScreen(display));
  
  if(frames->used == frames->max) {
    struct Frame* temp = NULL;
    temp = realloc(frames->list, sizeof(struct Frame) * frames->max * 2);
    if(temp != NULL) frames->list = temp;
    else return -1;
    frames->max *= 2;
  }
  
  //TODO: get details from WM hints if possible, these are defaults:
  struct Frame frame;
  frame.window_name = NULL;
  frame.mode = FLOATING;
  frame.window = framed_window;
  frame.w = 600; // MINWIDTH*3;
  frame.h = 400; //MINHEIGHT*3;
  frame.x = (XWidthOfScreen(screen) - frame.w); //RHS of the screen
  frame.y = 30; //below menubar
  frame.min_width = MINWIDTH;
  frame.min_height = MINHEIGHT;
  frame.max_width = XWidthOfScreen(screen);
  frame.max_height = XHeightOfScreen(screen);
  frame.selected = 1;
  frame.frame = XCreateSimpleWindow(display, root, frame.x, frame.y, frame.w, frame.h, 0,  WhitePixelOfScreen(screen), BlackPixelOfScreen(screen));
  frame.closebutton = XCreateSimpleWindow(display, frame.frame, frame.w-20-8-1, 4, 20, 20, 0, WhitePixelOfScreen(screen), BlackPixelOfScreen(screen));
  frame.pulldown = XCreateSimpleWindow(display, frame.frame, frame.w-20-8-1-PULLDOWN_WIDTH-4, 4, PULLDOWN_WIDTH, 20, 0, WhitePixelOfScreen(screen), BlackPixelOfScreen(screen));

  XAddToSaveSet(display, frame.window); //this means the window will survive after the connection is closed
  
  XSetWindowBorderWidth(display, frame.window, 0);
  XResizeWindow(display, frame.window, frame.w - FRAME_HSPACE, frame.h - FRAME_VSPACE);
  XReparentWindow(display, framed_window, frame.frame, FRAME_XSTART, FRAME_YSTART);
  XFetchName(display, frame.window, &frame.window_name);
  //and the input only hotspots    
  //and set the mode    
  //need to establish how to place a new window
  
  //need to map the windows before creating the cairo surfaces
  XMapWindow(display, frame.frame);
  XMapWindow(display, frame.closebutton);
  XMapWindow(display, frame.pulldown);
  XMapWindow(display, frame.window);
  
  frame.frame_s = cairo_xlib_surface_create(display, frame.frame, colours, frame.w, frame.h);
  frame.closebutton_s = cairo_xlib_surface_create(display, frame.closebutton, colours, 20, 20);
  frame.pulldown_s = cairo_xlib_surface_create(display, frame.pulldown, colours, 100, 20);
  
  //use the property notify to update titles  
  XSelectInput(display, frame.window, StructureNotifyMask | PropertyChangeMask);
  XSelectInput(display, frame.frame, Button1MotionMask | ButtonPressMask | ButtonReleaseMask | ExposureMask);
  XSelectInput(display, frame.closebutton, ButtonPressMask | ButtonReleaseMask | ExposureMask);  
  
  frames->list[frames->used] = frame;
  frames->used++;
  return (frames->used - 1);
} 

