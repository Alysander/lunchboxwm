#include "xcheck.h"

/**
@file     xcheck.c
@brief    Contains simple wrappers which enable calls to windows/pixmaps with an ID of 0 to be silently ignored.  This is useful for the theming system which does not require all pixmaps/windows to be present.
@author   Alysander Stanley
**/


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
