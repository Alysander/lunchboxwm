
/* These control which printfs are shown */
#define ALLOW_XLIB_DEBUG
//#define CRASH_ON_BUG
#define SHOW_SIG
#define SHOW_STARTUP
//#define SHOW_FRAME_HINTS
#define SHOW_PROPERTIES
#define SHOW_CONFIGURE_REQUEST_EVENT
//#define SHOW_ENTER_NOTIFY_EVENTS
#define SHOW_CONFIGURE_NOTIFY_EVENT
//#define SHOW_UNMAP_NOTIFY_EVENT

//#define SHOW_DESTROY_NOTIFY_EVENT     
//#define SHOW_UNMAP_NOTIFY_EVENT       
//#define SHOW_MAP_REQUEST_EVENT       
#define SHOW_EDGE_RESIZE
#define SHOW_FRAME_HINTS 
//#define SHOW_BUTTON_PRESS_EVENT       
//#define SHOW_ENTER_NOTIFY_EVENTS      
//#define SHOW_LEAVE_NOTIFY_EVENTS      
//#define SHOW_FRAME_DROP  
//#define SHOW_FOCUS_EVENT
//#define SHOW_BUTTON_RELEASE_EVENT     
//#define SHOW_PROPERTIES
//#define SHOW_FRAME_DROP  
//#define SHOW_PROPERTY_NOTIFY
#define SHOW_CLIENT_MESSAGE
//#define SHOW_FREE_SPACE_STEPS

/**** Configure behaviour *****/

/* Sadly, many windows do have a minimum size but do not specify it.  
Small screens then cause the window to be automatically resized below this unspecified amount,
and the window becomes unusable.  
This is a work around hack that incidently disables resizing of those windows. */

//#define ALLOW_OVERSIZE_WINDOWS_WITHOUT_MINIMUM_HINTS

/* This is an old hint which is considered obsolete by the ICCCM. */
//#define ALLOW_POSITION_HINTS 

/* Allows synchronized xlib calls - useful when debugging */
//#define ALLOW_XLIB_DEBUG
