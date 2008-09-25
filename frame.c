
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
  XRemoveFromSaveSet (display, frames->list[index].window); //this will not destroy the window because it has been reparented to root
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

/*This function is called when the close button on the frame is pressed */
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

/*This function is used to add frames to windows that are already open when the WM is starting*/
void create_startup_frames (Display *display, struct Framelist* frames) {
	unsigned int windows_length;
	Window parent, children, *windows, root;
	XWindowAttributes attributes;
	int index;
  root = DefaultRootWindow(display);
  
	XQueryTree(display, root, &parent, &children, &windows, &windows_length);
  if(windows != NULL) for (int i = 0; i < windows_length; i++)  {
    XGetWindowAttributes(display, windows[i], &attributes);
	  if (attributes.map_state == IsViewable && !attributes.override_redirect) {
	    index = create_frame(display, frames, windows[i]);
		  if( index != -1) draw_frame(display, frames->list[index]);
		}
	}
	XFree(windows);
	
}


/*returns the frame index of the newly created window or -1 if out of memory */
int create_frame(Display* display, struct Framelist* frames, Window framed_window) { 
  Window root = DefaultRootWindow(display);
  Screen* screen = DefaultScreenOfDisplay(display);
  Visual* colours = XDefaultVisual(display, DefaultScreen(display));
  
  XSizeHints specified;
  
  long pre_ICCCM; //pre ICCCM recovered values.  Not sure about this one.
  
 	XWindowAttributes attributes; //fallback if the specified size hints don't work
  struct Frame frame;
   
  Window transient; //this window is only used to store a return value
	
  if(frames->used == frames->max) {
    struct Frame* temp = NULL;
    temp = realloc(frames->list, sizeof(struct Frame) * frames->max * 2);
    if(temp != NULL) frames->list = temp;
    else return -1;
    frames->max *= 2;
  }

  XGetWindowAttributes(display, framed_window, &attributes);
  
  /*** Set up defaults ***/
  if(attributes.width >= MINWIDTH) frame.w = attributes.width;
  else frame.w = 600;

  if(attributes.height >= MINHEIGHT) frame.h = attributes.height;
  else frame.h = 400;

  frame.x = attributes.x;
  frame.y = attributes.y;
  frame.min_width = MINWIDTH;
  frame.min_height = MINHEIGHT;
  frame.max_width = XWidthOfScreen(screen);
  frame.max_height = XHeightOfScreen(screen);
  frame.width_inc = 1;
  frame.height_inc = 1;
  frame.selected = 1;
  
  /*** Update with specified values if they are available ***/
  if(XGetWMNormalHints(display, framed_window, &specified, &pre_ICCCM) != 0) {
    printf("Managed to recover size hints\n");
    
    if((specified.flags & PPosition != 0) || (specified.flags & USPosition != 0)) {
      printf("got position hints\n");
      frame.x = specified.x;
      frame.y = specified.y;
    }
    if((specified.flags & PSize) || (specified.flags & USSize)) {
      printf("got size hints\n");
      frame.w = specified.width;
      frame.h = specified.height;
    }
    
    if((specified.flags & PMinSize) && (specified.min_width >= MINWIDTH) && (specified.min_height >= MINHEIGHT)) {
      printf("got min size hints\n");
      frame.min_width = specified.min_width;
      frame.min_height = specified.min_height;
    }
    
    if(specified.flags & PMaxSize) {
      printf("got max size hints\n");
      frame.max_width = specified.max_width;
      frame.max_height = specified.max_height;
    }
    if(specified.flags & PResizeInc) {
      printf("got inc hints, w %d, h %d\n", specified.width_inc, specified.height_inc);
      frame.width_inc = specified.width_inc;
      frame.height_inc = specified.height_inc;
    }
  }

  frame.w += FRAME_HSPACE;
  frame.h += FRAME_VSPACE;
  frame.window_name = NULL;
  frame.mode = FLOATING;
  frame.window = framed_window;  
  
  frame.frame = XCreateSimpleWindow(display, root, frame.x, frame.y, frame.w, frame.h, 0,  WhitePixelOfScreen(screen), BlackPixelOfScreen(screen));
  frame.closebutton = XCreateSimpleWindow(display, frame.frame, frame.w-20-8-1, 4, 20, 20, 0, WhitePixelOfScreen(screen), BlackPixelOfScreen(screen));
  frame.pulldown = XCreateSimpleWindow(display, frame.frame, frame.w-20-8-1-PULLDOWN_WIDTH-4, 4, PULLDOWN_WIDTH, 20, 0, WhitePixelOfScreen(screen), BlackPixelOfScreen(screen));

  XAddToSaveSet(display, frame.window); //this means the window will survive after this programs connection is closed
  
  XSetWindowBorderWidth(display, frame.window, 0);
  XReparentWindow(display, framed_window, frame.frame, FRAME_XSTART, FRAME_YSTART);
  XFetchName(display, frame.window, &frame.window_name);
  //and the input only hotspots 
  
  //need to map the windows before creating the cairo surfaces
  XMapWindow(display, frame.frame);
  XMapWindow(display, frame.closebutton);
  XMapWindow(display, frame.pulldown);
  XMapWindow(display, frame.window);
  
  XResizeWindow(display, frame.window, frame.w - FRAME_HSPACE, frame.h - FRAME_VSPACE);
  //need to change this so that the background pixmap is used instead.
  frame.frame_s = cairo_xlib_surface_create(display, frame.frame, colours, frame.w, frame.h);
  frame.closebutton_s = cairo_xlib_surface_create(display, frame.closebutton, colours, 20, 20);
  frame.pulldown_s = cairo_xlib_surface_create(display, frame.pulldown, colours, 100, 20);
  
  XSelectInput(display, frame.window, StructureNotifyMask | PropertyChangeMask);  //use the property notify to update titles  
  XSelectInput(display, frame.frame, Button1MotionMask | ButtonPressMask | ButtonReleaseMask | ExposureMask);
  XSelectInput(display, frame.closebutton, ButtonPressMask | ButtonReleaseMask | ExposureMask | EnterWindowMask | LeaveWindowMask);

	XGetTransientForHint(display, framed_window, &transient);
  if(transient != 0) {
    printf("Transient window detected\n");
    //set pop-windows to have focus
    XSetInputFocus(display, frame.window, RevertToPointerRoot, CurrentTime);
	}   
  
  frames->list[frames->used] = frame;
  frames->used++;
  return (frames->used - 1);
} 

