#include <npapi.h>
#include <npfunctions.h>
#include <syslog.h>

#if !defined(XP_UNIX) || !defined(MOZ_X11)
#error "This plugin will only work using X11 on Unix"
#endif

#ifndef NP_EXPORT
#define NP_EXPORT(TYPE) TYPE
#endif

/*
 * 0. Browser detects the presence of the plugin by finding the .so file
 *    in one of the directories its scans, and calls the function
 *    NP_GetMIMEDescription to get a list of MIME types and file extensions
 *    supported by this plugin.
 *
 * 1. Browser initializes the plugin. This consists of calling the following
 *    functions in any order:
 *
 *   * NP_GetEntryPoints
 *   * NP_Initialize
 *
 * 2. Browser calls NPP_New each time an instance of the plugin is
 *    created (i.e. for every object element that uses this plugin).
 *
 * 3. Plugin does all its awesomeness.
 *
 * 4. Browser calls NPP_Destroy each time an instance of the plugin is
 *    destroyed (e.g. when the page on which an instance of this plugin is used
 *    is closed).
 *
 * 5. Browser calls NP_Shutdown just before unloading the plugin.
 *    This can only happen after all instances of the plugin have been
 *    destroyed.
 */

/*
 * Type declarations.
 */

/*
 * Forward declarations.
 */
NP_EXPORT(NPError) NP_GetValue(void *future,
			       NPPVariable variable,
			       void *value);
static void log_debug(const char *message);

static NPNetscapeFuncs *browser_functions;

/*
 * Internal functions.
 */

/**
 * Redraws (part of) the plugin's drawable.
 */
static void draw(XGraphicsExposeEvent *event) {
  /* Get display and drawable from event. */
  Display *display = event->display;
  Drawable drawable = event->drawable;

  /* Get default colormap for our drawable. */
  Colormap colormap = XDefaultColormap(display,
				       XDefaultScreen(display));

  /* Get some colors. */
  XColor black, red;
  black.flags = 0;
  black.red = 0;
  black.green = 0;
  black.blue = 0;
  red.flags = DoRed;
  red.red = 0xffff;
  red.green = 0;
  red.blue = 0;
  XAllocColor(display, colormap, &black);
  XAllocColor(display, colormap, &red);

  /* Create graphics context with foreground and background colors. */
  XGCValues gc_values;
  gc_values.foreground = red.pixel;
  gc_values.background = black.pixel;
  GC gc = XCreateGC(display, drawable, GCForeground | GCBackground, &gc_values);

  /* Fill the exposed area. */
  XFillRectangle(display,
		 drawable,
		 gc,
		 event->x,
		 event->y,
		 event->width,
		 event->height);
}

/**
 * Logs a debug message.
 */
static void log_debug(const char *message) {
  syslog(LOG_DEBUG, "%s", message);
}

/*
 * Exported functions.
 */

/**
 * Called by the browser to destroy an instance of this plugin.
 *
 * @param instance the instance being destroyed.
 * @param saved can be used to save data for use by a later plugin instance
 *        at the same URI. The value stored in saved will be passed to
 *        NPP_New.
 *
 * @return NPError value.
 */
NPError NPP_Destroy(NPP instance, NPSavedData** save) {
  return NPERR_NO_ERROR;
}

NPError NPP_DestroyStream(NPP instance,
			  NPStream *stream,
			  NPReason reason) {
  return NPERR_NO_ERROR;
}

/**
 * Called by the browser to obtain the value of a variable from a plugin
 * instance.
 *
 * @param instance the plugin instance.
 * @param variable the variable whose value to get.
 * @param value    store the value here.
 *
 * @return NPError value
 */
NPError NPP_GetValue(NPP instance,
		     NPPVariable variable,
		     void *value) {
  return NP_GetValue(instance, variable, value);
}

/**
 * Called by the browser to send events to plugin instances.
 *
 * @param instance the plugin instance the event is being sent to.
 * @param event    the event being sent
 *
 * @return kNPEventNotHandled if the plugin did not handle the event
 *         or kNPEventHandled if it did.
 */
int16_t NP_LOADDS NPP_HandleEvent(NPP instance, void *event_ptr) {
  XEvent *event = (XEvent*) event_ptr;
  switch(event->type) {
  case GraphicsExpose:
    draw((XGraphicsExposeEvent*) event);
    return kNPEventHandled;
  }

  return kNPEventNotHandled;
}

/**
 * Called by the browser to create a new instance of this plugin.
 *
 * @return NPError value.
 */
NPError NPP_New(NPMIMEType pluginType,
		NPP instance,
		uint16_t mode,
		int16_t argc,
		char *argn[],
		char *argv[],
		NPSavedData *saved) {

  /* Make instance windowless. */
  NPError result = browser_functions->setvalue(instance,
					       NPPVpluginWindowBool,
					       (void*) false);
  if(result != NPERR_NO_ERROR) return result;

  browser_functions->status(instance, "Hello, world!");

  return NPERR_NO_ERROR;
}

NPError NPP_NewStream(NPP instance,
		      NPMIMEType type,
		      NPStream *stream,
		      NPBool seekaple,
		      uint16_t *stype) {
  return NPERR_NO_ERROR;
}

NPError NPP_SetWindow(NPP instance, NPWindow *window) {
  return NPERR_GENERIC_ERROR;
}

/**
 * Called by the browser to get pointers to the functions this plugin provides.
 *
 * @param pFuncs struct of function pointers that this function is expected
 *               to fill out.
 *
 * @return NPError value.
 */
NPError OSCALL NP_GetEntryPoints(NPPluginFuncs* pFuncs) {
  pFuncs->newp = NPP_New;
  pFuncs->destroy = NPP_Destroy;
  pFuncs->newstream = NPP_NewStream;
  pFuncs->destroystream = NPP_DestroyStream;
  pFuncs->setwindow = NPP_SetWindow;
  pFuncs->getvalue = NPP_GetValue;
  pFuncs->event = NPP_HandleEvent;

  return NPERR_NO_ERROR;
}

/**
 * Called by the browser to get the list of MIME types and file extensions
 * supported by this plugin.
 *
 * @return a string of the form:
 *         <MIME type>:<extensions>:<description>
 *         where <extensions> is a comma-separated list of file name
 *         extensions.
 *         Multiple MIME-types may be listed, separated by semicolons.
 */
const char *NP_GetMIMEDescription() {
  return "application/x-npexample:example:Example plugin data";
}

/**
 * Called by the browser to get information about this plugin.
 *
 * @param future   Unused.
 * @param variable Name of the variable to get.
 * @param value    Store the value of the variable here.
 *
 * @return NPError value.
 */
NP_EXPORT(NPError) NP_GetValue(void *future,
			      NPPVariable variable,
			      void *value) {
  switch(variable) {
  case NPPVpluginNameString:
    value = "example";
    return NPERR_NO_ERROR;
  case NPPVpluginDescriptionString:
    value = "Example plugin";
    return NPERR_NO_ERROR;
  default:
    return NPERR_INVALID_PARAM;
  }
}

/**
 * Called by the browser with a structure containing pointers to browser
 * functions the plugin can call.
 *
 * @param bFuncs structure containing pointers to browser functions.
 * @param pFuncs (only used on Unix) struct which we should populate with
 *               pointers to plugin functions.
 *
 * @return NPError value.
 */
NP_EXPORT(NPError) NP_Initialize(NPNetscapeFuncs *bFuncs,
				 NPPluginFuncs *pFuncs) {

  /* On Unix, instead of calling NP_GetEntryPoints, the browser passes
   * a second argument to NP_Initialize. We can simply call NP_GetEntryPoints
   * on that argument to do the right thing.
   */
  NP_GetEntryPoints(pFuncs);

  browser_functions = bFuncs;

  return NPERR_NO_ERROR;
}

/**
 * Called by the browser once the last instance of this plugin is destroyed.
 * It should perform any cleanup that this plugin requires.
 *
 * @return NPError value.
 */
NP_EXPORT(NPError) NP_Shutdown() {
  return NPERR_NO_ERROR;
}
