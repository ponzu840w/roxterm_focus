/**
 * roxterm_focus.c
 *
 * Detects new Roxterm windows (WM_CLASS.res_class == "Roxterm")
 * and activates them.
 * 新規Roxtermウィンドウをアクティベートする。
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#define MAX_WINDOWS 256

static Window seen_windows[MAX_WINDOWS];
static int    seen_count = 0;

/**
 * Ignores some errors, improving the survivability of demon.
 * エラーを無視するエラーハンドラ
 */
static XErrorHandler prev_x_error_handler;
static int
x_error_handler(Display *dpy, XErrorEvent *ev)
{
  if (ev->error_code == BadWindow)
  {
    fprintf(stderr, "ignore BadWindow.\n");
    return 0;                       // ignore
  }
  if (prev_x_error_handler)
  {
    prev_x_error_handler(dpy, ev);  // fallback
  }
  return 0;
}

/*
 * 既存ウィンドウか？
 */
static bool
is_seen(Window w)
{
  for (int i=0; i<seen_count; i++)
  {
    if (seen_windows[i] == w)
    {
      return true;
    }
  }
  return false;
}

/*
 * 既存ウィンドウとして配列に追加
 */
static void
record_seen(Window w)
{
  if (seen_count<MAX_WINDOWS)
  {
    seen_windows[seen_count++] = w;
  }
}

/*
 * 既存ウィンドウとしての記録を抹消
 */
static void
forget_seen(Window w)
{
  for (int i=0; i<seen_count; i++)
  {
    if (seen_windows[i] == w)
    {
#ifdef DEBUG
      fprintf(stderr, "Forgetting Roxterm window 0x%lx\n", w);
#endif
      seen_windows[i] = seen_windows[--seen_count];
      return;
    }
  }
}

/**
 * Send an EWMH client message to DefaultRootWindow.
 * msg_name is e.g. "_NET_CURRENT_DESKTOP"
 *               or "_NET_ACTIVE_WINDOW".
 * Returns 0 on success, -1 on failure.
 * EWMHクライアントメッセージの送信
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
 * Find the toplevel window
 * whose WM_CLASS.res_class == "Roxterm"
 * in the subtree rooted at w. Returns that Window or 0.
 * Roxtermのウィンドウかを再帰的にチェック
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
 * ウィンドウをアクティベート（キーボードのフォーカス）
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

#ifdef DEBUG
/*
 * 既存ウィンドウ記録配列のデバッグ表示
 */
static void
debug_print_seen(void)
{
    fprintf(stderr, "[DEBUG] seen_count = %d\n", seen_count);
    for (int i = 0; i < seen_count; i++) {
        fprintf(stderr, "[DEBUG] seen_windows[%d] = 0x%lx\n",
                i, (unsigned long)seen_windows[i]);
    }
}
#endif

static long
_get_desktop(Display *dpy, Window w, Atom atom)
{
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;
  long desktop = -1;

  if (XGetWindowProperty(
        dpy, w, atom, 0, 1, False,
        XA_CARDINAL, &actual_type, &actual_format, &nitems,
        &bytes_after, &prop) == Success && prop)
  {
    if (nitems > 0)
    {
      desktop = *((long *)prop);
    }
    XFree(prop);
  }
  return desktop;
}

/**
 * Get a window's desktop number (_NET_WM_DESKTOP).
 * Returns desktop number or -1 on error.
 * A value of 0xFFFFFFFF means the window is on all desktops.
 * ウィンドウが属するデスクトップの番号
 */
static long
get_window_desktop(Display *dpy, Window w)
{
  Atom atom = XInternAtom(dpy, "_NET_WM_DESKTOP", False);
  return _get_desktop(dpy, w, atom);
}

/**
 * Get the current desktop number (_NET_CURRENT_DESKTOP).
 * Returns desktop number or -1 on error.
 * 現在のデスクトップの番号
 */
static long
get_current_desktop(Display *dpy)
{
  Atom atom = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
  Window root = DefaultRootWindow(dpy);
  return _get_desktop(dpy, root, atom);
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

  // install custom error handler
  prev_x_error_handler = XSetErrorHandler(x_error_handler);

  Window root = DefaultRootWindow(dpy);

  // listen for new windows
  XSelectInput(dpy, root, SubstructureNotifyMask);

  fprintf(stderr, "roxterm_focus started\n");

  for (;;)
  {
    XEvent ev;
    XNextEvent(dpy, &ev);

    switch (ev.type)
    {
    case MapNotify:
      Window mapped = ev.xmap.window;
      Window target = find_roxterm_window(dpy, mapped);
      if (target && !is_seen(target))
      {
        long current_desktop = get_current_desktop(dpy);
        long target_desktop = get_window_desktop(dpy, target);
#ifdef DEBUG
        fprintf(stderr, "Current Desktop=0x%lx\n", current_desktop);
        fprintf(stderr, "Target Desktop=0x%lx\n", target_desktop);
#endif
        if (current_desktop == target_desktop ||
            target_desktop == 0xFFFFFFFF)
        {
          record_seen(target);
          XSelectInput(dpy, target, StructureNotifyMask);
          activate_roxterm(dpy, target);
#ifdef DEBUG
          debug_print_seen();
          fprintf(stderr, "Found Roxterm window 0x%lx, activating\n", target);
#endif
        }
      }
      break;

    case DestroyNotify:
      forget_seen(ev.xdestroywindow.window);
      break;

    default:
      break;
    }
  }

  // never reached
  XCloseDisplay(dpy);
  return EXIT_SUCCESS;
}
