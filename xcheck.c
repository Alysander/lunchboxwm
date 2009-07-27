#include "xcheck.h"

Bool
xcheck_raisewin(Display *display, Window window) {
  if(window) {
    XRaiseWindow(display, window);
    return True;
  }
  return False;
}

Bool
xcheck_setpixmap (Display *display, Window window, Pixmap pixmap) {
  if(pixmap) {
    XSetWindowBackgroundPixmap(display, window, pixmap);
    return True;
  }
  return False;
}

Bool
xcheck_map(Display *display, Window window) {
  if(window) {
    XMapWindow(display, window);
    return True;
  }
  return False;
}
