// roxterm_focus.c
//
// Detects new Roxterm windows (WM_CLASS.res_class == "Roxterm")
// and activates them.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

/**
 * Ignores some errors, improving the survivability of demon.
 */
int
error_handler(Display *dpy, XErrorEvent *ev)
{
  if (ev->error_code == BadWindow)
  {
    return 0; // ignore
  }
  return XSetErrorHandler(NULL)(dpy, ev); // fallback
}

/**
 * Send an EWMH client message to DefaultRootWindow.
 * msg_name is e.g. "_NET_CURRENT_DESKTOP" or "_NET_ACTIVE_WINDOW".
 * Returns 0 on success, -1 on failure.
 */
static int
client_msg(Display *dpy, Window target_win,
    const char *msg_name,
    unsigned long d0, unsigned long d1,
    unsigned long d2, unsigned long d3, unsigned long d4)
{
  XEvent ev;
  Atom msg = XInternAtom(dpy, msg_name, False);
  Window root = DefaultRootWindow(dpy);
  long mask = SubstructureRedirectMask | SubstructureNotifyMask;

  memset(&ev, 0, sizeof(ev));
  ev.xclient.type         = ClientMessage;
  ev.xclient.window       = target_win;
  ev.xclient.message_type = msg;
  ev.xclient.format       = 32;
  ev.xclient.data.l[0]    = d0;
  ev.xclient.data.l[1]    = d1;
  ev.xclient.data.l[2]    = d2;
  ev.xclient.data.l[3]    = d3;
  ev.xclient.data.l[4]    = d4;

  if (!XSendEvent(dpy, root, False, mask, &ev))
  {
    fprintf(stderr, "Cannot send %s ClientMessage\n", msg_name);
    return -1;
  }
  XFlush(dpy);
  return 0;
}

/**
 * Find the toplevel window whose WM_CLASS.res_class == "Roxterm"
 * in the subtree rooted at w. Returns that Window or 0.
 */
static Window
find_roxterm_window(Display *dpy, Window w)
{
  XClassHint hint;
  Window result = 0;

  if (XGetClassHint(dpy, w, &hint))
  {
    if (hint.res_class && strcasecmp(hint.res_class, "Roxterm") == 0)
    {
      XFree(hint.res_name);
      XFree(hint.res_class);
      return w;
    }
    XFree(hint.res_name);
    XFree(hint.res_class);
  }

  {
    Window root_ret, parent_ret, *kids;
    unsigned int nkids;
    if (XQueryTree(dpy, w, &root_ret, &parent_ret, &kids, &nkids))
    {
      for (unsigned int i = 0; i < nkids; i++)
      {
        result = find_roxterm_window(dpy, kids[i]);
        if (result) break;
      }
      XFree(kids);
    }
  }

  return result;
}

/**
 * Activate a Roxterm window:
 */
static void
activate_roxterm(Display *dpy, Window win)
{
  // activate the window
  client_msg(dpy, win, "_NET_ACTIVE_WINDOW", 0, 0, 0, 0, 0);

  // raise & map
  XMapRaised(dpy, win);
  XFlush(dpy);
}

int
main(void)
{
  Display *dpy = XOpenDisplay(NULL);
  if (!dpy)
  {
    fprintf(stderr, "Cannot open X display\n");
    return EXIT_FAILURE;
  }

  Window root = DefaultRootWindow(dpy);

  // listen for new windows
  XSelectInput(dpy, root, SubstructureNotifyMask);

  fprintf(stderr, "roxterm_focus started\n");

  for (;;)
  {
    XEvent ev;
    XNextEvent(dpy, &ev);
    if (ev.type == MapNotify)
    {
      Window mapped = ev.xmap.window;
      Window target = find_roxterm_window(dpy, mapped);
      if (target)
      {
        fprintf(stderr, "Found Roxterm window 0x%lx, activating\n", target);
        activate_roxterm(dpy, target);
      }
    }
  }

  XCloseDisplay(dpy);
  return EXIT_SUCCESS;
}
