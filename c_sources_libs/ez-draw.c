/*
 * ez-draw.c: basic EZ-Draw module
 *
 * Edouard.Thiel@lif.univ-mrs.fr - 07/07/2013 - version 1.2
 *
 * This program is free software under the terms of the
 * GNU Lesser General Public License (LGPL) version 2.1.
*/

/* To debug, define environment variable EZ_IMAGE_DEBUG */

#define EZ_PRIVATE_DEFS 1
#include "ez-draw.h"

/* Contains internal parameters of ez-draw.c */
Ez_X ezx;

/* Allow to know if ez_init and ez_main_loop were called */
enum { EZ_PRE_INIT, EZ_INIT, EZ_POST_INIT, EZ_MAIN_LOOP, EZ_FINAL };
int ez_state = EZ_PRE_INIT;

/* Colors */
Ez_uint32 ez_black, ez_white, ez_grey, ez_red, ez_green, ez_blue,
          ez_yellow, ez_cyan, ez_magenta;


/*---------------------- P U B L I C   I N T E R F A C E --------------------*/

/*
 * Main initialization.
 * Return 0 on succes, -1 on error.
*/

int ez_init (void)
{
    if (ez_state != EZ_PRE_INIT) {
        ez_error ("ez_init: error, called several times\n");
        return -1;
    }
    ez_state = EZ_INIT;

    /* Set to zero all fields of ezx */
    memset (&ezx, 0, sizeof(Ez_X));

#ifdef EZ_BASE_XLIB

    /* Connect to the display which was set in environment variable DISPLAY,
       by default the local display. */
    ezx.display = XOpenDisplay ("");
    if (ezx.display == NULL) {
        ez_error ("ez_init: XOpenDisplay failed for \"%s\"\n",
            XDisplayName (""));
        return -1;
    }

    /* Store display parameters */
    ezx.screen_num = DefaultScreen (ezx.display);
    ezx.display_width = DisplayWidth (ezx.display, ezx.screen_num);
    ezx.display_height = DisplayHeight (ezx.display, ezx.screen_num);
    ezx.root_win = RootWindow (ezx.display, ezx.screen_num);
    ezx.depth = DefaultDepth (ezx.display, ezx.screen_num);

    /* Graphical context gc; suppress events NoExpose and GraphicsExpose */
    ezx.gc = DefaultGC (ezx.display, ezx.screen_num);
    XSetGraphicsExposures(ezx.display, ezx.gc, False);

    /* Create an xid to store data in a window */
    ezx.info_prop = XUniqueContext ();

    /* Atoms for the WM */
    ezx.atom_protoc = XInternAtom (ezx.display, "WM_PROTOCOLS", False);
    ezx.atom_delwin = XInternAtom (ezx.display, "WM_DELETE_WINDOW", False);

#elif defined EZ_BASE_WIN32

    /* Get the program Handle */
    ezx.hand_prog = GetModuleHandle(NULL);

    /* Prepare an extended window class */
    ezx.wnd_class.cbSize        = sizeof(WNDCLASSEX);
    ezx.wnd_class.style         = CS_CLASSDC | CS_HREDRAW | CS_VREDRAW;
    ezx.wnd_class.lpfnWndProc   = ez_win_proc;
    ezx.wnd_class.cbClsExtra    = 0;
    ezx.wnd_class.cbWndExtra    = 0;
    ezx.wnd_class.hInstance     = ezx.hand_prog;
    ezx.wnd_class.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
    ezx.wnd_class.hIconSm       = LoadIcon (NULL, IDI_APPLICATION);
    ezx.wnd_class.hCursor       = LoadCursor (NULL, IDC_ARROW);
    ezx.wnd_class.hbrBackground = (HBRUSH) (COLOR_WINDOW+1);
    ezx.wnd_class.lpszMenuName  = NULL;
    ezx.wnd_class.lpszClassName = "EZ_WND_CLASS";

    /* Store the window class */
    if (!RegisterClassEx (&ezx.wnd_class)) {
        ez_error ("ez_init: RegisterClassEx failed for class \"%s\"\n",
            ezx.wnd_class.lpszClassName);
        return -1;
    }

    /* Store screen parameters */
    ezx.display_width  = GetSystemMetrics(SM_CXSCREEN);
    ezx.display_height = GetSystemMetrics(SM_CYSCREEN);
    ezx.root_win = NULL;

    /* Property to store data in a window */
    ezx.info_prop = "EZ_INFO_PROP";

    /* Events */
    ezx.key_sym = 0;                   /* For KeyRelease */
    ezx.key_name = ezx.key_string = "";

    /* For drawings */
    ezx.dc_win = None;                 /* Current window associated to dc */
    ezx.hpen = NULL;                   /* Pen for color drawing */
    ezx.hbrush = NULL;                 /* Brush for color filling */

    /* Handling of time */
    ez_init_timeofday ();

#endif /* EZ_BASE_ */

    /* Initialize the double buffer for windows displaying */
    ez_dbuf_init ();

    /* Initialize fonts and default font */
    ez_font_init ();
    ez_set_nfont (0);

    /* Initialize colors, default color and thickness */
    ez_color_init ();
    ez_set_color (ez_black);
    ez_set_thick (1);

    /* Timers list */
    ezx.timer_nb = 0;

    /* Configure event loop */
    ezx.main_loop = 1;    /* Set to 0 to break the event loop */
    ezx.last_expose = 1;  /* Set to 0 to deactivate waiting of last Expose */
    ezx.auto_quit = 1;    /* Button Close will exit program */
    ezx.mouse_b = 0;      /* Used for MotionNotify */
    ezx.win_nb = 0;       /* Break also event loop */
    ezx.mv_win = None;    /* To filter mouse moves */

    /* Initialize random numbers generator */
    ez_random_init ();

    /* For safe exit */
    atexit (ez_close_disp);

    ez_state = EZ_POST_INIT;

    return 0;
}


/*
 * Create a main window, having width w and height h, a title name, and a
 * callback func that will be called for each event.
 * Return the window.
*/

Ez_window ez_window_create (int w, int h, const char *name, Ez_func func)
{
    Ez_window win;
    Ez_win_info *info;

    if (ez_check_state ("ez_window_create") < 0) return None;

#ifdef EZ_BASE_XLIB

    /* Create a main window, child of the root window */
    win = XCreateSimpleWindow (ezx.display, ezx.root_win,
        0, 0, w, h, 0, ez_black, ez_white);

   /* Set an event mask */
    XSelectInput (ezx.display, win,
        ExposureMask | KeyPressMask | KeyReleaseMask |
        ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
        StructureNotifyMask);

    /* Set the window title */
    XStoreName (ezx.display, win, name);

    /* Protect windows from destruction */
    XSetWMProtocols (ezx.display, win, &ezx.atom_delwin, 1);

    /* Palette of colors */
    if (ezx.visual->class == PseudoColor) {
        XSetWindowAttributes attr;
        attr.colormap = ezx.pseudoColor.colormap;
        XChangeWindowAttributes(ezx.display, win, CWColormap, &attr);
    }

#elif defined EZ_BASE_WIN32

    /* Create a main window = child of NULL on Windows */
    win = CreateWindowEx (
        WS_EX_CLIENTEDGE,
        ezx.wnd_class.lpszClassName,
        name,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        w + 4 + GetSystemMetrics (SM_CXSIZEFRAME)*2,
        h + 4 + GetSystemMetrics (SM_CYSIZEFRAME)*2
              + GetSystemMetrics (SM_CYCAPTION),
        NULL, NULL, ezx.hand_prog, NULL);

    if (win == NULL) {
        ez_error ("ez_window_create: failed for \"%s\"\n", name);
        return NULL;
    }

#endif /* EZ_BASE_ */

    /* To store data in the window */
    info = malloc (sizeof (Ez_win_info));
    if (info == NULL) {
        ez_error ("ez_window_create: malloc error for \"%s\"\n", name);
#ifdef EZ_BASE_XLIB
        XDestroyWindow (ezx.display, win);
#elif defined EZ_BASE_WIN32
        DestroyWindow (win);
#endif /* EZ_BASE_ */
        return None;
    }

    /* Associate data to the window by using a xid or a property */
    if (ez_prop_set (win, ezx.info_prop, info) < 0) {
        ez_error ("ez_window_create: malloc error for \"%s\"\n", name);
        free (info);
#ifdef EZ_BASE_XLIB
        XDestroyWindow (ezx.display, win);
#elif defined EZ_BASE_WIN32
        DestroyWindow (win);
#endif /* EZ_BASE_ */
        return None;
    }
    info->func = func;
    info->data = NULL;
    info->dbuf = None;
    ez_window_show (win, 1);

    /* Store the window */
    ez_win_list_insert (win);

    if (ez_draw_debug())
        printf ("ez_window_create 0x%x\n", ez_window_get_id(win));

    return win;
}


/*
 * Return the window id as an int.
*/

int ez_window_get_id (Ez_window win)
{
#ifdef EZ_BASE_XLIB
    return win;
#elif defined EZ_BASE_WIN32
    return PtrToInt (win);
#endif /* EZ_BASE_ */
}


/*
 * Destroy window win and all associated stuff.
*/

void ez_window_destroy (Ez_window win)
{
    if (ez_state < EZ_POST_INIT) {
        ez_error ("ez_window_destroy: error, called before ez_init\n");
        return;
    }

    if (ez_win_list_remove (win) < 0) return;

    ez_win_delete (win);
}


/*
 * Show (val = 1) or hide (val = 0) a window.
*/

void ez_window_show (Ez_window win, int val)
{
    if (ez_check_state ("ez_window_show") < 0) return;

    /* If not in main_loop, delay display */
    if (ez_state < EZ_MAIN_LOOP) {
        Ez_win_info *info;
        if (ez_info_get (win, &info) < 0) return;
        info->show = val;
        return;
    }

#ifdef EZ_BASE_XLIB
    if (val) {
        XMapRaised (ezx.display, win);
    } else {
        XUnmapWindow (ezx.display, win);
    }
#elif defined EZ_BASE_WIN32
    if (val) {
        ShowWindow (win, SW_SHOWNORMAL);
        PostMessage (win, WM_PAINT, 0, 0);
    } else {
        ShowWindow (win, SW_HIDE);
    }
#endif /* EZ_BASE_ */
}


/*
 * Change or retrieve the window size.
*/

void ez_window_set_size (Ez_window win, int w, int h)
{
#ifdef EZ_BASE_XLIB

    XWindowChanges wc;

    wc.width = w > 1 ? w : 1;
    wc.height = h > 1 ? h : 1;
    XConfigureWindow (ezx.display, win, CWWidth|CWHeight, &wc);

#elif defined EZ_BASE_WIN32

    if (w < 1) w = 1;
    if (h < 1) h = 1;
    SetWindowPos (win, HWND_TOP, 0, 0,
        w + 4 + GetSystemMetrics (SM_CXSIZEFRAME)*2,
        h + 4 + GetSystemMetrics (SM_CYSIZEFRAME)*2
              + GetSystemMetrics (SM_CYCAPTION),
        SWP_NOMOVE | SWP_NOZORDER );

#endif /* EZ_BASE_ */
}

void ez_window_get_size (Ez_window win, int *w, int *h)
{
#ifdef EZ_BASE_XLIB

    Ez_window root_ret;
    unsigned int w_ret, h_ret, b_ret, d_ret;
    int x_ret, y_ret;

    XGetGeometry (ezx.display, win, &root_ret, &x_ret, &y_ret,
        &w_ret, &h_ret, &b_ret, &d_ret);

#elif defined EZ_BASE_WIN32

    int w_ret, h_ret;
    RECT rect;

    GetWindowRect (win, &rect);

    w_ret = rect.right - rect.left - 4
            - GetSystemMetrics (SM_CXSIZEFRAME)*2;
    if (w_ret < 0) w_ret = 0;
    h_ret = rect.bottom - rect.top - 4
            - GetSystemMetrics (SM_CYSIZEFRAME)*2
            - GetSystemMetrics (SM_CYCAPTION);
    if (h_ret < 0) h_ret = 0;

#endif /* EZ_BASE_ */

    if (w) *w = w_ret;
    if (h) *h = h_ret;
}


/*
 * Clear a window and initialize again drawings parameters.
*/

void ez_window_clear (Ez_window win)
{
    int w, h;

    ez_window_get_size (win, &w, &h);
    ez_set_color (ez_white);
    ez_fill_rectangle (win, 0, 0, w, h);
    ez_set_color (ez_black);
    ez_set_thick (1);
    ez_set_nfont (0);
}


/*
 * Activate or unactivate the double buffer for a window.
*/

void ez_window_dbuf (Ez_window win, int val)
{
    XdbeBackBuffer dbuf;

    if (ez_dbuf_get (win, &dbuf) < 0) return;

    if (val) {

        if (dbuf != None) return;
#ifdef EZ_BASE_XLIB
        dbuf = XdbeAllocateBackBufferName (ezx.display, win, XdbeUndefined);
#elif defined EZ_BASE_WIN32
        ez_cur_win (win);
        dbuf = CreateCompatibleDC (ezx.hdc);
#endif /* EZ_BASE_ */
        ez_dbuf_set (win, dbuf);

    } else {

        if (dbuf == None) return;
#ifdef EZ_BASE_XLIB
        XdbeDeallocateBackBufferName (ezx.display, dbuf);
#elif defined EZ_BASE_WIN32
        ez_cur_win (None);
        DeleteDC (dbuf);
#endif /* EZ_BASE_ */
        ez_dbuf_set (win, None);
    }
}


/*
 * Associate a client data to a window.
*/

void ez_set_data (Ez_window win, void *data)
{
    Ez_win_info *info;
    if (ez_info_get (win, &info) < 0) return;
    info->data = data;
}


/*
 * Retrieve the client data associated to a window.
*/

void *ez_get_data (Ez_window win)
{
    Ez_win_info *info;
    if (ez_info_get (win, &info) < 0) return NULL;
    return info->data;
}


/*
 * Break ez_main_loop().
*/

void ez_quit (void)
{
    if (ez_check_state ("ez_quit") < 0) return;

#ifdef EZ_BASE_WIN32
    PostQuitMessage (0);
#endif /* EZ_BASE_ */

    ezx.main_loop = 0;
}


/*
 * Determine the behavior when the user click on the Close button :
 *   val = 1  ==> the program will end (default).
 *   val = 0  ==> the program will receive a WindowClose event.
*/

void ez_auto_quit (int val)
{
    ezx.auto_quit = val;
}


/*
 * Send an Expose event to the window, to force redraw.
*/

void ez_send_expose (Ez_window win)
{
#ifdef EZ_BASE_XLIB

    XEvent ev;

    ev.type = Expose;
    ev.xexpose.window = win;
    ev.xexpose.count = 0;

    XSendEvent (ezx.display, win, False, 0L, &ev);

#elif defined EZ_BASE_WIN32

    PostMessage (win, EZ_MSG_PAINT, 0, 0);

#endif /* EZ_BASE_ */
}


/*
 * Start a timer for the window win with the delay expressed in millisecs.
 * Any recall before timer expiration will cancel and replace the timer with
 * the new delay. Moreover, if delay is -1 then the timer is deleted.
*/

void ez_start_timer (Ez_window win, int delay)
{
    if (delay < 0) {
        ez_timer_remove (win);
    } else {
        if (ez_timer_add (win, delay) < 0)
            ez_error ("ez_start_timer: could not set timer delay"
                " = %d ms for win 0x%x\n", delay, ez_window_get_id(win));
    }
}


/*
 * Main loop. To break, just call ez_quit().
 * This function displays the windows, then wait for events and dispatch them
 * by calling the corresponding callbacks.
 * Once returned from ez_main_loop(), no more graphic call should be done.
*/

void ez_main_loop (void)
{
    int i;
#ifdef EZ_BASE_XLIB
    Ez_event ev;
#elif defined EZ_BASE_WIN32
    MSG msg;
#endif /* EZ_BASE_ */

    if (ez_state == EZ_PRE_INIT) {
        ez_error ("ez_main_loop: error, ez_init must be called first\n");
        return;
    }
     if (ez_state >= EZ_MAIN_LOOP) {
        ez_error ("ez_main_loop: error, called several times\n");
        return;
    }
    ez_state = EZ_MAIN_LOOP;

    /* Display all windows whose displaying was delayed */
    for (i = 0; i < ezx.win_nb; i++) {
        Ez_win_info *info;
        if (ez_info_get (ezx.win_l[i], &info) < 0) continue;
        if (info->show) ez_window_show (ezx.win_l[i], 1);
    }

    /* Wait for next event then call the callback */
    while (ezx.main_loop != 0 && ezx.win_nb > 0) {
#ifdef EZ_BASE_XLIB
        ez_event_next (&ev);
        ez_event_dispatch (&ev);
#elif defined EZ_BASE_WIN32
        ez_msg_next (&msg);
        DispatchMessage (&msg);
#endif /* EZ_BASE_ */
    }

    ez_state = EZ_FINAL;
}


/*
 * Return a random number between 0 and n-1
*/

int ez_random (int n)
{
#ifdef EZ_BASE_XLIB
    return (random() / (RAND_MAX + 1.0)) * n;
#elif defined EZ_BASE_WIN32
    return (rand() / (RAND_MAX + 1.0)) * n;
#endif /* EZ_BASE_ */
}


/*
 * Return the elapsed time since the Epoch in seconds,microseconds
*/

double ez_get_time (void)
{
    struct timeval tp;
    ez_gettimeofday (&tp);
    return (double) tp.tv_sec + (double) tp.tv_usec / (double) 1E6;
}


/*
 * Return a color computed according to the levels r,g,b between 0 and 255.
*/

Ez_uint32 (*ez_get_RGB)(Ez_uint8 r, Ez_uint8 g, Ez_uint8 b) =
#ifdef EZ_BASE_XLIB
    ez_get_RGB_true_color;
#elif defined EZ_BASE_WIN32
    ez_get_RGB_win32;
#endif /* EZ_BASE_ */


/*
 * Return a grey color computed according to the level g between 0 and 255.
*/

Ez_uint32 ez_get_grey (Ez_uint8 g)
{
    return ez_get_RGB (g, g, g);
}


/*
 * Convert a color from HSV to RGB ; h in [0..360], s,v in [0..1]
 * see http://en.wikipedia.org/wiki/Hue/saturation/value
*/

void ez_HSV_to_RGB (double h, double s, double v,
    Ez_uint8 *r, Ez_uint8 *g, Ez_uint8 *b)
{
    int    ti = ((int) h / 60) % 6;
    double f = h / 60 - ti;
    v *= 255;
    switch (ti) {
        case 0 : *r = v; *b = v*(1-s); *g = v*(1-(1-f)*s); break;
        case 1 : *g = v; *b = v*(1-s); *r = v*(1-   f *s); break;
        case 2 : *g = v; *r = v*(1-s); *b = v*(1-(1-f)*s); break;
        case 3 : *b = v; *r = v*(1-s); *g = v*(1   -f *s); break;
        case 4 : *b = v; *g = v*(1-s); *r = v*(1-(1-f)*s); break;
        case 5 : *r = v; *g = v*(1-s); *b = v*(1-   f *s); break;
        default : *r = *g = *b = 0;
    }
}


/*
 * Return a color defined in space Hue, Saturation, Value.
 * h in [0..360], s,v in [0..1]
*/

Ez_uint32 ez_get_HSV (double h, double s, double v)
{
    Ez_uint8 r, g, b;

    ez_HSV_to_RGB (h, s, v, &r, &g, &b);
    return ez_get_RGB (r, g, b);
}


/*
 * Store the color for the next drawings, as well as for text displaying.
*/

void ez_set_color (Ez_uint32 color)
{
    ezx.color = color;

#ifdef EZ_BASE_XLIB

    XSetForeground (ezx.display, ezx.gc, ezx.color);

#elif defined EZ_BASE_WIN32

    /* Pen for drawings */
    ez_update_pen ();

    /* Brush for collor filling */
    if (ezx.hbrush != NULL) DeleteObject (ezx.hbrush);
    ezx.hbrush = CreateSolidBrush (ezx.color);
    if (ezx.dc_win != None) SelectObject (ezx.hdc, ezx.hbrush);

    /* To display text */
    if (ezx.dc_win != None) SetTextColor (ezx.hdc, ezx.color);

#endif /* EZ_BASE_ */
}


/*
 * Set the thickness for next drawings.
 * relates : ez_draw_point, ez_draw_line, ez_draw_rectangle, ez_draw_triangle,
 *           ez_draw_circle.
*/

void ez_set_thick (int thick)
{
    ezx.thick = (thick <= 0) ? 1 : thick;

#ifdef EZ_BASE_XLIB
    XSetLineAttributes (ezx.display, ezx.gc, (ezx.thick == 1) ? 0 : ezx.thick,
        LineSolid, CapRound, JoinRound);
#elif defined EZ_BASE_WIN32
    ez_update_pen ();
#endif /* EZ_BASE_ */
}


/*
 * Basic drawings. x1,y1 and y2,y2 are the top left and bottom right
 * coordinates of the bounding box.
*/

#define EZ_MIN(x,y) ((x)<(y)?(x):(y))
#define EZ_MAX(x,y) ((x)>(y)?(x):(y))

void ez_draw_point (Ez_window win, int x1, int y1)
{
#ifdef EZ_BASE_XLIB
    if (win == ezx.dbuf_win) win = ezx.dbuf_pix;
    if (ezx.thick == 1)
         XDrawPoint (ezx.display, win, ezx.gc, x1, y1);
    else XFillArc (ezx.display, win, ezx.gc, x1-ezx.thick/2, y1-ezx.thick/2,
                   ezx.thick+1, ezx.thick+1, 0, 360*64);
#elif defined EZ_BASE_WIN32
    ez_cur_win (win);
    MoveToEx (ezx.hdc, x1, y1, NULL);
    LineTo (ezx.hdc, x1+1, y1);  /* final point excluded */
#endif /* EZ_BASE_ */
}

void ez_draw_line (Ez_window win, int x1, int y1, int x2, int y2)
{
#ifdef EZ_BASE_XLIB
    if (win == ezx.dbuf_win) win = ezx.dbuf_pix;
    XDrawLine (ezx.display, win, ezx.gc, x1, y1, x2, y2);
#elif defined EZ_BASE_WIN32
    ez_cur_win (win);
    MoveToEx (ezx.hdc, x1, y1, NULL);
    LineTo (ezx.hdc, x2, y2);
    if (ezx.thick == 1) LineTo (ezx.hdc, x2+1, y2);   /* final point excluded */
#endif /* EZ_BASE_ */
}

void ez_draw_rectangle (Ez_window win, int x1, int y1, int x2, int y2)
{
#ifdef EZ_BASE_XLIB
    if (win == ezx.dbuf_win) win = ezx.dbuf_pix;
    XDrawRectangle (ezx.display, win, ezx.gc,
        EZ_MIN(x1,x2), EZ_MIN(y1,y2), abs(x2-x1), abs(y2-y1));
#elif defined EZ_BASE_WIN32
    ez_cur_win (win);
    MoveToEx (ezx.hdc, x1, y1, NULL);
    LineTo (ezx.hdc, x2, y1);
    LineTo (ezx.hdc, x2, y2);
    LineTo (ezx.hdc, x1, y2);
    LineTo (ezx.hdc, x1, y1);
#endif /* EZ_BASE_ */
}

void ez_fill_rectangle (Ez_window win, int x1, int y1, int x2, int y2)
{
#ifdef EZ_BASE_XLIB
    if (win == ezx.dbuf_win) win = ezx.dbuf_pix;
    XFillRectangle (ezx.display, win, ezx.gc,
        EZ_MIN(x1,x2), EZ_MIN(y1,y2), abs(x2-x1)+1, abs(y2-y1)+1);
#elif defined EZ_BASE_WIN32
    int old_thick = ezx.thick;
    ez_cur_win (win);
    if (ezx.thick != 1) ez_set_thick (1);
    Rectangle (ezx.hdc, EZ_MIN(x1,x2)  , EZ_MIN(y1,y2)   ,
                        EZ_MAX(x1,x2)+1, EZ_MAX(y1,y2)+1 );
    if (ezx.thick != old_thick) ez_set_thick (old_thick);
#endif /* EZ_BASE_ */
}

void ez_draw_triangle (Ez_window win, int x1, int y1, int x2, int y2, int x3, int y3)
{
#ifdef EZ_BASE_XLIB
    if (win == ezx.dbuf_win) win = ezx.dbuf_pix;
    XDrawLine (ezx.display, win, ezx.gc, x1, y1, x2, y2);
    XDrawLine (ezx.display, win, ezx.gc, x2, y2, x3, y3);
    XDrawLine (ezx.display, win, ezx.gc, x3, y3, x1, y1);
#elif defined EZ_BASE_WIN32
    ez_cur_win (win);
    MoveToEx (ezx.hdc, x1, y1, NULL);
    LineTo (ezx.hdc, x2, y2);
    LineTo (ezx.hdc, x3, y3);
    LineTo (ezx.hdc, x1, y1);
#endif /* EZ_BASE_ */
}

void ez_fill_triangle (Ez_window win, int x1, int y1, int x2, int y2, int x3, int y3)
{
#ifdef EZ_BASE_XLIB
    XPoint points[3];
    points[0].x = x1; points[1].x = x2; points[2].x = x3;
    points[0].y = y1; points[1].y = y2; points[2].y = y3;
    if (win == ezx.dbuf_win) win = ezx.dbuf_pix;
    XFillPolygon (ezx.display, win, ezx.gc, points, 3, Convex, CoordModeOrigin);
#elif defined EZ_BASE_WIN32
    POINT points[3];
    int old_thick = ezx.thick;
    points[0].x = x1; points[1].x = x2; points[2].x = x3;
    points[0].y = y1; points[1].y = y2; points[2].y = y3;
    ez_cur_win (win);
    if (ezx.thick != 1) ez_set_thick (1);
    Polygon (ezx.hdc, points, 3 );
    if (ezx.thick != old_thick) ez_set_thick (old_thick);
#endif /* EZ_BASE_ */
}

void ez_draw_circle (Ez_window win, int x1, int y1, int x2, int y2)
{
#ifdef EZ_BASE_XLIB
    if (win == ezx.dbuf_win) win = ezx.dbuf_pix;
    XDrawArc (ezx.display, win, ezx.gc,
        EZ_MIN(x1,x2), EZ_MIN(y1,y2), abs(x2-x1), abs(y2-y1), 0, 360*64);
#elif defined EZ_BASE_WIN32
    int xa = EZ_MIN(x1,x2), ya = EZ_MIN(y1,y2),
        xb = EZ_MAX(x1,x2), yb = EZ_MAX(y1,y2),
        xc = (xa+xb)/2;
    ez_cur_win (win);
    Arc (ezx.hdc, xa, ya, xb, yb, xc, ya, xc, ya);
#endif /* EZ_BASE_ */
}

void ez_fill_circle (Ez_window win, int x1, int y1, int x2, int y2)
{
#ifdef EZ_BASE_XLIB
    if (win == ezx.dbuf_win) win = ezx.dbuf_pix;
    XFillArc (ezx.display, win, ezx.gc,
        EZ_MIN(x1,x2), EZ_MIN(y1,y2), abs(x2-x1)+1, abs(y2-y1)+1, 0, 360*64);
#elif defined EZ_BASE_WIN32
    int old_thick = ezx.thick;
    ez_cur_win (win);
    if (ezx.thick != 1) ez_set_thick (1);
    Ellipse (ezx.hdc, EZ_MIN(x1,x2)  , EZ_MIN(y1,y2)   ,
                      EZ_MAX(x1,x2)+1, EZ_MAX(y1,y2)+1 );
    if (ezx.thick != old_thick) ez_set_thick (old_thick);
#endif /* EZ_BASE_ */
}


/*
 * Load a font from its name (e.g. "6x13") and store it in ezx.font[num].
 * Return 0 on succes, -1 on error.
 *
 * On X11, the name can be in any fashion but must correspond to an existing
 * font. On Windows, the name must be in the form widthxheight; a matching font
 * of fixed size is obtained.
*/

int ez_font_load (int num, const char *name)
{
#ifdef EZ_BASE_WIN32
    int w, h;
#endif /* EZ_BASE_ */

    if (num < 0 || num >= EZ_FONT_MAX) {
        ez_error ("ez_font_load: bad num\n");
        return -1;
    }

#ifdef EZ_BASE_XLIB

    ezx.font[num] = XLoadQueryFont (ezx.display, name);

#elif defined EZ_BASE_WIN32

    if (sscanf (name, "%dx%d", &w, &h) != 2) {
        ez_error ("ez_font_load: could not get wxh in \"%s\"\n", name);
        return -1;
    }

    ezx.font[num] = CreateFont (
        -h, -w, 0, 0,               /* nHeight,nWidth,nEscapement,nOrientation */
        h <= 18 ? 500 : 600,        /* fnWeight */
        FALSE, FALSE, FALSE,        /* fdwItalic, fdwUnderline, fdwStrikeOut */
        ANSI_CHARSET,               /* fdwCharset. DEFAULT_CHARSET ? */
        OUT_DEFAULT_PRECIS,         /* fdwOutputPrecision */
        CLIP_DEFAULT_PRECIS,        /* fdwClipPrecision */
        DEFAULT_QUALITY,            /* fdwQuality */
        FIXED_PITCH | FF_MODERN,    /* fdwPitchAndFamily */
        ""                          /* lpszFace */
    );

#endif /* EZ_BASE_ */

    if (ezx.font[num] == NULL)  {
        ez_error ("ez_font_load: could not load font \"%s\"\n", name);
        return -1;
    }
    return 0;
}


/*
 * Set the font number num for the next text drawings.
*/

void ez_set_nfont (int num)
{
    if (num < 0 || num >= EZ_FONT_MAX || ezx.font[num] == NULL) {
        ez_error ("ez_set_nfont: bad num\n");
        return;
    }
    ezx.nfont = num;

#ifdef EZ_BASE_XLIB
    XSetFont (ezx.display, ezx.gc, ezx.font[ezx.nfont]->fid);
#elif defined EZ_BASE_WIN32
    if (ezx.dc_win != None) SelectObject (ezx.hdc, ezx.font[ezx.nfont]);
#endif /* EZ_BASE_ */
}


/*
 * Display text; same usage as printf.
 *
 * Example : ez_draw_text (win, EZ_TL, x, y,
 *                         "Width = %d\nHeight = %d", w, h);
 *
 * The coordinates x1,y1 are relative to align, see Ez_Align definition.
 * align also specifies if the background is filled or not.
 *
 * By default:
 *  - the text is displayed with the font number 0 ("6x13") ;
 *    can be changed with ez_set_nfont().
 *  - text is displayed in black; can be changed with ez_set_color().
*/

void ez_draw_text (Ez_window win, Ez_Align align, int x1, int y1,
    const char *format, ...)
{
    int valign, halign, fillbg;
    va_list (ap);
    char buf[16384];
    int i, j, k, n, x, y, a, b, c;

#ifdef EZ_BASE_XLIB
    XFontStruct *font = ezx.font[ezx.nfont];
#elif defined EZ_BASE_WIN32
    TEXTMETRIC text_metric;
#endif /* EZ_BASE_ */

    if (align <= EZ_AA || align == EZ_BB || align >= EZ_CC)
      { ez_error ("ez_draw_text: bad align\n"); return; }

    /* Decode align */
    fillbg = 0;
    if (align > EZ_BB) { fillbg = 1; align -= 10; }
    align -= EZ_AA + 1;
    halign = align % 3;
    valign = align / 3;

    /* Print the formated string in buf */
    va_start (ap, format);
    vsnprintf (buf, sizeof(buf)-1, format, ap);
    va_end (ap);
    buf[sizeof(buf)-1] = 0;
    if (buf[0] == 0) return;

    /* Count the number of lines */
    for (i = j = k = 0; ; i++)
    if (buf[i] == '\n' || buf[i] == 0) {
        k++; j = i+1;
        if (buf[i] == 0) break;
    }
    n = k;

#ifdef EZ_BASE_XLIB

    if (win == ezx.dbuf_win) win = ezx.dbuf_pix;
    a = font->ascent; b = font->descent; c = a+b+b;

    /* Display line by line */
    for (i = j = k = 0; ; i++)
    if (buf[i] == '\n' || buf[i] == 0) {
        x = x1 - XTextWidth (font, buf+j, i-j) * halign/2;
        y = y1 + a + c*k - (c*n-b) * valign/2;
        if (fillbg == 0)
             XDrawString      (ezx.display, win, ezx.gc, x, y, buf+j, i-j);
        else XDrawImageString (ezx.display, win, ezx.gc, x, y, buf+j, i-j);
        k++; j = i+1;
        if (buf[i] == 0) break;
    }

#elif defined EZ_BASE_WIN32

    ez_cur_win (win);
    GetTextMetrics (ezx.hdc, &text_metric);
    a = text_metric.tmAscent; b = text_metric.tmDescent; c = a+b;

    if (fillbg == 0) SetBkMode (ezx.hdc, TRANSPARENT);

    /* Display line by line */
    for (i = j = k = 0; ; i++)
    if (buf[i] == '\n' || buf[i] == 0) {
        x = x1 - (i-j)*text_metric.tmAveCharWidth * halign/2;
        y = y1 + c*k - (c*n-b) * valign/2 -2;
        TextOut (ezx.hdc, x, y, buf+j, i-j);
        k++; j = i+1;
        if (buf[i] == 0) break;
    }

    /* Restore background drawing mode */
    if (fillbg == 0) SetBkMode (ezx.hdc, OPAQUE);

#endif /* EZ_BASE_ */
}


/*-------------------- P R I V A T E   F U N C T I O N S --------------------*/

/*
 * Unique test of the definition of the environment variable EZ_DRAW_DEBUG
*/

int ez_draw_debug (void)
{
    static int debug = -1;
    if (debug < 0) debug = getenv ("EZ_DRAW_DEBUG") != NULL;
    return debug;
}


/*
 * Print errors.
 *
 * By default, ez_error prints on stderr. We can suppress error messages
 * by making: ez_error_handler = ez_error_ign
 * or intercept messages by changing ez_error_handler.
*/

int ez_error_dfl (const char *fmt, va_list ap)
{
    return vfprintf (stderr, fmt, ap);
}

int ez_error_ign (const char *fmt, va_list ap)
{
    (void) fmt; (void) ap;
    return 0;
}

int (*ez_error_handler) (const char *fmt, va_list ap) = ez_error_dfl;

int ez_error (const char *fmt, ...)
{
    va_list ap;
    int r;

    va_start (ap, fmt);
    r = (*ez_error_handler) (fmt, ap);
    va_end (ap);

    return r;
}


/*
 * Test if ez_state is EZ_POST_INIT or EZ_MAIN_LOOP
 * Return 0 on success, -1 on error. Verbose.
*/

int ez_check_state (const char *funcname)
{
    if (ez_state < EZ_POST_INIT) {
        ez_error ("%s: error, called before ez_init\n", funcname);
        return -1;
    }
    if (ez_state > EZ_MAIN_LOOP) {
        ez_error ("%s: error, called after ez_main_loop\n", funcname);
        return -1;
    }
    return 0;
}


/*
 * Delete a window (without suppressing it from the list).
 * Called by ez_window_destroy and ez_win_delete_all.
*/

void ez_win_delete (Ez_window win)
{
    Ez_win_info *info;

    if (ez_draw_debug())
        printf ("ez_win_delete 0x%x\n", ez_window_get_id(win));

    ez_window_dbuf (win, 0);
    ez_timer_remove (win);

    /* Destroy data _after_ ez_window_dbuf (which still uses them) */
    if (ez_info_get (win, &info) == 0) {
        free (info);
        ez_prop_destroy (win, ezx.info_prop);
    }

#ifdef EZ_BASE_XLIB

    XDestroyWindow (ezx.display, win);

#elif defined EZ_BASE_WIN32

    if (win == ezx.dc_win) ez_cur_win (None);
    if (win == ezx.mv_win) ezx.mv_win = None;
    DestroyWindow (win);

#endif /* EZ_BASE_ */
}


/* Delete remaining windows. Called at the end of the program by
 * ez_close_disp if ez_win_delete_final == 1.
*/

int ez_win_delete_final = 1;

void ez_win_delete_all (void)
{
    int i;

    for (i = 0; i < ezx.win_nb; i++)
        ez_win_delete (ezx.win_l[i]);
    ezx.win_nb = 0;
}


/*
 * Called after exit (atexit function set by ez_init)
*/

void ez_close_disp (void)
{
    if (ez_win_delete_final) ez_win_delete_all ();

    ez_font_delete ();

#ifdef EZ_BASE_XLIB
    if (ezx.visual->class == PseudoColor)
        XFreeColormap (ezx.display, ezx.pseudoColor.colormap);

    /* Close the display; from now on, do not call functions using it. */
    XCloseDisplay (ezx.display); ezx.display = NULL;
#endif /* EZ_BASE_ */
}


#ifdef EZ_BASE_WIN32

/*
 * Initialize time computation.
*/

void ez_init_timeofday (void)
{
    LARGE_INTEGER freq;

    ezx.perf_freq = 0;
    ez_get_systemtime (&ezx.start_time);
    QueryPerformanceCounter (&ezx.start_count);

    if (QueryPerformanceFrequency (&freq) && freq.QuadPart > 0)
         ezx.perf_freq = 1.0 / freq.QuadPart;
    if (ez_draw_debug())
        printf ("ez_init_timeofday: perf_freq = %.10f\n", ezx.perf_freq);
}


/*
 * Emulate gettimeofday() with GetSystemTimeAsFileTime().
 * GetSystemTimeAsFileTime() has an accuracy of 15ms (10ms on XP).
 * GetSystemTimePreciseAsFileTime() has an accuracy of 1ms but needs
 * windows >= 8, and seems rather expensive.
*/

void ez_get_systemtime (struct timeval *t)
{
    ULARGE_INTEGER ul;
    FILETIME ft;
    double dt;
    if (t == NULL) return;
    GetSystemTimeAsFileTime (&ft);
    ul.LowPart = ft.dwLowDateTime;
    ul.HighPart = ft.dwHighDateTime;
    dt = ul.QuadPart / 10000000.0 - 11644473600.0;
    t->tv_sec = dt;
    t->tv_usec = (dt - t->tv_sec) * 1000000.0;
}


/*
 * Emulate gettimeofday() with QueryPerformanceCounter().
 * This method seems to be more accurate than ez_get_systemtime()
 * but leads to a drift.
*/

void ez_get_countertime (struct timeval *t)
{
    LARGE_INTEGER now;
    double dt;
    QueryPerformanceCounter (&now);
    dt = (now.QuadPart - ezx.start_count.QuadPart) * ezx.perf_freq;
    *t = ezx.start_time;
    t->tv_sec += dt;
    t->tv_usec += (dt - (int)dt) * 1000000;
    t->tv_sec += t->tv_usec / 1000000;
    t->tv_usec %= 1000000;
}

#endif /* EZ_BASE_ */


/*
 * Compute elapsed time since the Epoch.
*/

void ez_gettimeofday (struct timeval *t)
{
    if (t == NULL) return;
#ifdef EZ_BASE_XLIB
    gettimeofday (t, NULL);
#elif defined EZ_BASE_WIN32
    if (ezx.perf_freq == 0)
         ez_get_systemtime (t);
    else ez_get_countertime (t);
#endif /* EZ_BASE_ */
}


/*
 * Insert a timer in the list. Return 0 on success, -1 on error.
*/

int ez_timer_add (Ez_window win, int delay)
{
    int m1, m2, mid;
    struct timeval t, *mt;

    ez_timer_remove (win);

    if (ezx.timer_nb >= EZ_TIMER_MAX) {
        ez_error ("ez_timer_add: too many timers\n");
        return -1;
    }

    /* Retrieve current date */
    ez_gettimeofday (&t);

    /* Compute expiration date */
    t.tv_usec += delay * 1000;   /* delay in milliseconds */
    if (t.tv_usec > 1000000) {
        t.tv_sec += t.tv_usec / 1000000;
        t.tv_usec %= 1000000;
    }

    /* Dichotomic search */
    m1 = 0; m2 = ezx.timer_nb;
    while (m2-m1 > 0) {
        mid = (m1+m2) / 2;    /* Middle s.t. m1 <= mid < m2 */
        mt = &ezx.timer_l[mid].expiration;
        if ( mt->tv_sec < t.tv_sec ||
            (mt->tv_sec == t.tv_sec && mt->tv_usec < t.tv_usec) )
             m1 = mid+1;   /* t goes in [mid+1 .. m2] */
        else m2 = mid;     /* t goes in [m1 .. mid]   */
    }

    /* Insert in position m1 */
    if (m1 < ezx.timer_nb)
        memmove ( ezx.timer_l+m1+1, ezx.timer_l+m1,
                 (ezx.timer_nb-m1)*sizeof(Ez_timer) );
    ezx.timer_nb++;
    ezx.timer_l[m1].win = win;
    ezx.timer_l[m1].expiration = t;

    return 0;
}


/*
 * Suppress a timer from the list. Return 0 on success, -1 on error.
*/

int ez_timer_remove (Ez_window win)
{
    int i;
    if (win == None) return 0;
    for (i = 0; i < ezx.timer_nb; i++)
        if (ezx.timer_l[i].win == win) {
            memmove ( ezx.timer_l+i, ezx.timer_l+i+1,
                     (ezx.timer_nb-i-1)*sizeof(Ez_timer) );
            ezx.timer_nb--;
            return 0;
        }
    return -1;
}


/*
 * Return delay between current date and next timer.
 * To pass directly to select().
*/

struct timeval *ez_timer_delay (void)
{
    static struct timeval t;

    /* No timer */
    if (ezx.timer_nb == 0) return NULL;

    /* Retrieve current date */
    ez_gettimeofday (&t);

    /* The next timer is ezx.timer_list[0].expiration ;
       we compute the difference with the current date */
    t.tv_sec  = ezx.timer_l[0].expiration.tv_sec  - t.tv_sec;
    t.tv_usec = ezx.timer_l[0].expiration.tv_usec - t.tv_usec;
    if (t.tv_usec < 0) { t.tv_usec += 1000000; t.tv_sec -= 1;}
    if (t.tv_sec  < 0) t.tv_sec = t.tv_usec = 0;

    /* printf ("Timeout in %d s  %6d us\n", (int)t.tv_sec, (int)t.tv_usec); */

    /* Return static address of the struct */
    return &t;
}


#ifdef EZ_BASE_XLIB

/*
 * Blocking waiting of en event.
 * Replace XNextEvent and add events (TimerNotify).
*/

void ez_event_next (Ez_event *ev)
{
    int n, res, fdx = ConnectionNumber(ezx.display);
    fd_set set1;

    /* Initialize ev */
    memset (ev, 0, sizeof(Ez_event));
    ev->type = EzLastEvent;
    ev->win = None;

    /* Label allowing to ignore an event and start again waiting */
    start_waiting:

    /* Do a XFlush and retrieve the number of events in the queue,
     * without reading and without blocking.
    */
    n = XEventsQueued (ezx.display, QueuedAfterFlush);

    /* If there is at least one event in the queue, we can read it without
     * blocking with XNextEvent(). We must do it here since a waiting with
     * select() would be blocking if the server did already send all events.
    */
    if (n > 0) {
        XNextEvent (ezx.display, &ev->xev);
        if ( (ev->xev.type == Expose) &&
             ezx.last_expose && ! ez_is_last_expose (&ev->xev))
            goto start_waiting;
        return;
    }

    /* The queue on the client side is empty, we start waiting */
    FD_ZERO (&set1);
    FD_SET (fdx, &set1);

    res = select (fdx+1, &set1, NULL, NULL, ez_timer_delay ());

    if (res > 0) {
        if (FD_ISSET (fdx, &set1)) {
            XNextEvent (ezx.display, &ev->xev);
            if ( (ev->xev.type == Expose) &&
                 ezx.last_expose && ! ez_is_last_expose (&ev->xev) )
                goto start_waiting;
            return;
        }

    } else if (res == 0) {

        if (ezx.timer_nb == 0) {
            ez_error ("ez_event_next: select() should not return 0\n");
            goto start_waiting;
        }
        ev->type = TimerNotify;
        ev->win = ezx.timer_l[0].win;
        ez_timer_remove (ev->win);

    } else {
        perror ("ez_event_next: select()");
    }
}


/* Type for XCheckIfEvent */
typedef struct { int later; Ez_window win; } Ez_predicat_Expose;

/* Prototype imposed by XCheckIfEvent */
Bool ez_predicat_expose (Display *display, XEvent *xev, XPointer arg)
{
    Ez_predicat_Expose *p = (Ez_predicat_Expose *) arg;
    (void) display;  /* unused variable. */
    if (xev->type == Expose && xev->xexpose.window == p->win)
        p->later = 1;
    return 0;
}

/*
 * Check if this is the last Expose in the queue, without modifying it to
 * preserve temporal order. Return boolean.
*/

int ez_is_last_expose (XEvent *xev)
{
    Ez_predicat_Expose p;
    XEvent ev_return;

    if (xev->xexpose.count > 0) return 0;

    p.later = 0;
    p.win = xev->xexpose.window;

    /* Check the whole queue without alteration, because the predicate
     * function ez_predicat_expose always returns FALSE */
    XCheckIfEvent (ezx.display, &ev_return, ez_predicat_expose, (XPointer) &p);

    return ! p.later;
}


/*
 * Analyse an event then call the callback.
*/

void ez_event_dispatch (Ez_event *ev)
{
    /* Double buffer */
    ezx.dbuf_pix = None; ezx.dbuf_win = None;

    /* We decode the event. To ignore, just make return */
    if (ev->type == EzLastEvent)
    switch (ev->xev.type) {

        /* The window must be redrawn. */
        case Expose :
            /* Some Expose will be ignored. */
            if (ezx.last_expose && ! ez_is_last_expose (&ev->xev))
                return;
            ev->type = ev->xev.type;
            ev->win  = ev->xev.xexpose.window;
            ez_dbuf_get (ev->win, &ezx.dbuf_pix);
            if (ezx.dbuf_pix != None) ez_dbuf_preswap (ev->win);
            ez_window_clear (ev->win);
            break;

        /* A mouse button was pressed or released. */
        case ButtonPress :
            ev->type = ev->xev.type;
            ev->win  = ev->xev.xbutton.window;
            ev->mx   = ev->xev.xbutton.x;
            ev->my   = ev->xev.xbutton.y;
            ev->mb   = ev->xev.xbutton.button;
            /* If another button is pressed, it is ignored. */
            if (ezx.mouse_b != 0) return;
            ezx.mouse_b = ev->mb;
            break;

        case ButtonRelease :
            ev->type = ev->xev.type;
            ev->win  = ev->xev.xbutton.window;
            ev->mx   = ev->xev.xbutton.x;
            ev->my   = ev->xev.xbutton.y;
            ev->mb   = ev->xev.xbutton.button;
            /* If another button is released, it is ignored. */
            if (ezx.mouse_b != ev->mb) return;
            ezx.mouse_b = 0;
            break;

        /* The mouse was moved in the window. */
        case MotionNotify :
            ev->type = ev->xev.type;
            ev->win  = ev->xev.xmotion.window;
            ev->mx   = ev->xev.xmotion.x;
            ev->my   = ev->xev.xmotion.y;
            ev->mb   = ezx.mouse_b;  /* because no xmotion.button ! */
            /* If the same event has already been sent, it is ignored. */
            if (ezx.mv_win == ev->win && ezx.mv_x == ev->mx && ezx.mv_y == ev->my)
                    return;
            ezx.mv_win = ev->win; ezx.mv_x = ev->mx; ezx.mv_y = ev->my;
            break;

        /* A key was pressed or released. */
        case KeyPress :
        case KeyRelease :
            ev->type = ev->xev.type;
            ev->win  = ev->xev.xkey.window;
            ev->mx   = ev->xev.xkey.x;
            ev->my   = ev->xev.xkey.y;

            ev->key_count = XLookupString (&ev->xev.xkey, ev->key_string,
                sizeof(ev->key_string)-1, &ev->key_sym, NULL);
            ev->key_string[ev->key_count] = 0;  /* add terminal '\0' */
            sprintf (ev->key_name, "XK_%s", XKeysymToString(ev->key_sym));
            break;

        /* Get new window size */
        case ConfigureNotify  :
            ev->type   = ev->xev.type;
            ev->win    = ev->xev.xconfigure.window;
            ev->width  = ev->xev.xconfigure.width;
            ev->height = ev->xev.xconfigure.height;
            break;

        /* Intercept window close: see ez_auto_quit() */
        case ClientMessage :
            if ((Atom) ev->xev.xclient.message_type == ezx.atom_protoc &&
                (Atom) ev->xev.xclient.data.l[0]    == ezx.atom_delwin)
            {
                if (ezx.auto_quit) {
                    ez_quit ();
                    return;
                } else {
                    ev->type  = WindowClose;
                    ev->win   = ev->xev.xclient.window;
                }
            } else return;
            break;

        default : return;

    } /* End event decoding */

    /* Call the window callback. */
    ez_func_call (ev);

    /* Swap double buffer */
    if (ezx.dbuf_pix != None) ez_dbuf_swap (ev->win);
}

#elif defined EZ_BASE_WIN32

/*
 * Blocking waiting of a message.
 * Replace GetMessage by adding the message WM_TIMER with a better accuracy
 * than with native timers (whose have an accuracy of 10 or 15ms).
*/

void ez_msg_next (MSG *msg)
{
    struct timeval *tv;
    double dt_ms;
    int k;

    start_waiting:
    tv = ez_timer_delay ();
    if (tv == NULL) dt_ms = INFINITE;
    else {
        dt_ms = tv->tv_sec*1000 + tv->tv_usec/1000;
        if (dt_ms < 0) dt_ms = 0;
    }

    k = MsgWaitForMultipleObjectsEx (0, NULL, dt_ms, QS_ALLINPUT,
            MWMO_INPUTAVAILABLE);  /* <-- very important! */

    if (k == WAIT_TIMEOUT) {
        memset (msg, 0, sizeof(MSG));
        msg->message = WM_TIMER;
        msg->hwnd = ezx.timer_l[0].win;
        ez_timer_remove (msg->hwnd);
    } else {
        if (! PeekMessage (msg, NULL, 0, 0, PM_REMOVE)) goto start_waiting;
        /* Add message WM_CHAR after a WM_KEYDOWN */
        TranslateMessage (msg);
    }
}


/*
 * The winproc called by DispatchMessage or by the system.
 * Return 0L to tell that the message is processed.
*/
LRESULT CALLBACK ez_win_proc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Ez_event ev;

    /* Initialize ev */
    memset (&ev, 0, sizeof(Ez_event));
    ev.win = None;  /* It it stays to None, the callback is not called */
    ev.xev.hwnd = hwnd; ev.xev.message = msg;
    ev.xev.wParam = wParam; ev.xev.lParam = lParam;

    /* Double buffer */
    ezx.dbuf_dc = None; ezx.dbuf_win = None;

    if (ez_draw_debug())
        if (msg != WM_SETCURSOR && msg != WM_NCHITTEST)
            printf ("    ez_win_proc  win = 0x%08x  MSG = 0x%04x = %-20s  wP = 0x%08x  lP = 0x%08x\n",
            PtrToInt(hwnd), (int) msg, ez_win_msg_name (msg), (int) wParam, (int) lParam);

    /* Decode message */
    switch (msg) {

        case WM_PAINT :
            ValidateRect (hwnd, NULL);
        case EZ_MSG_PAINT :
            ev.type = Expose;
            ev.win = hwnd;
            ez_dbuf_get (ev.win, &ezx.dbuf_dc);
            if (ezx.dbuf_dc != None) ez_dbuf_preswap (ev.win);
            if (ez_draw_debug())
                printf ("Expose  win 0x%x  dbuf 0x%x\n",
                    ez_window_get_id(ev.win), PtrToInt(ezx.dbuf_dc));
            ez_window_clear (ev.win);
            break;

        /* A mouse button was pressed or released. */
        case WM_LBUTTONDOWN :
        case WM_MBUTTONDOWN :
        case WM_RBUTTONDOWN :
            ev.type = ButtonPress;
            ev.win  = hwnd;
            ev.mx   = GET_X_LPARAM (lParam);
            ev.my   = GET_Y_LPARAM (lParam);
            ev.mb   = msg == WM_LBUTTONDOWN ? 1 :
                      msg == WM_MBUTTONDOWN ? 2 :
                      msg == WM_RBUTTONDOWN ? 3 : 4;
            /* If another button is pressed, it is ignored. */
            if (ezx.mouse_b != 0) return 0L;
            ezx.mouse_b = ev.mb;
            /* Capture the mouse to get the MotionNotify events when the
               mouse comes out while button pressed. */
            SetCapture (hwnd);
            break;

        case WM_LBUTTONUP :
        case WM_MBUTTONUP :
        case WM_RBUTTONUP :
            ev.type = ButtonRelease;
            ev.win  = hwnd;
            ev.mx   = GET_X_LPARAM (lParam);
            ev.my   = GET_Y_LPARAM (lParam);
            ev.mb   = msg == WM_LBUTTONUP ? 1 :
                      msg == WM_MBUTTONUP ? 2 :
                      msg == WM_RBUTTONUP ? 3 : 4;
            /* If another button is pressed, it is ignored. */
            if (ezx.mouse_b != ev.mb) return 0L;
            ezx.mouse_b = 0;
            ReleaseCapture ();
            break;

        /* The mouse has moved in the window. */
        case WM_MOUSEMOVE :
            ev.type = MotionNotify;
            ev.win  = hwnd;
            ev.mx   = GET_X_LPARAM (lParam);
            ev.my   = GET_Y_LPARAM (lParam);
            ev.mb   = ezx.mouse_b;
            /* If the same event has already been sent, it is ignored. */
            if (ezx.mv_win == ev.win && ezx.mv_x == ev.mx && ezx.mv_y == ev.my)
                    return 0L;
            ezx.mv_win = ev.win; ezx.mv_x = ev.mx; ezx.mv_y = ev.my;
            break;

        /* A key was pressed or released. */
        case WM_SYSKEYDOWN :
        case WM_KEYDOWN :
        case WM_CHAR :
        {
            KeySym ksym;
            char *kname, *kstring;

            /* Some keys generate two events, we filter. */
            if (msg == WM_CHAR) {
                if (ez_keychar_convert (wParam, &ksym, &kname, &kstring) < 0)
                    return 0L;
                /* Store the key for the next WM_KEYUP */
                ezx.key_sym = ksym;
                ezx.key_name = kname;
                ezx.key_string = kstring;
            } else {
                if (ez_keydown_convert (wParam, lParam, &ksym,&kname, &kstring) < 0)
                   return 0L;
            }

            ev.type    = KeyPress;
            ev.win     = hwnd;
            ev.mx      = ezx.mv_x;
            ev.my      = ezx.mv_y;
            ev.key_sym = ksym;
            strcpy (ev.key_name  , kname);
            strcpy (ev.key_string, kstring);
            ev.key_count = strlen (kstring);

            if (ez_is_repetition (lParam)) {
                if (ez_is_modifier (ksym)) {
                    /* Suppress repetitions for keys Ctrl, ALt and Shift */
                    return 0L;
                } else {
                    /* Simulate KeyRelease before the KeyPress */
                    ev.type = KeyRelease; ez_func_call (&ev); ev.type = KeyPress;
                }
            }
            break;
        }

        case WM_KEYUP :
        case WM_SYSKEYUP :
        {
            KeySym ksym;
            char *kname, *kstring;

            if (ez_keydown_convert (wParam, lParam, &ksym, &kname, &kstring) < 0) {
               /* This message contains the description of the corresponding
                  WM_KEYDOWN, but that is the description of the previous
                  WM_CHAR which is relevant. */
                ksym = ezx.key_sym;
                kname = ezx.key_name;
                kstring = ezx.key_string;
                /* Reset */
                ezx.key_sym = 0;
                ezx.key_name = ezx.key_string = "";
            }
            if (ksym == 0) return 0L;

            ev.type    = KeyRelease;
            ev.win     = hwnd;
            ev.mx      = ezx.mv_x;
            ev.my      = ezx.mv_y;
            ev.key_sym = ksym;
            strcpy (ev.key_name  , kname);
            strcpy (ev.key_string, kstring);
            ev.key_count = strlen (kstring);
            break;
        }

        case WM_SIZE :
            ev.type   = ConfigureNotify;
            ev.win    = hwnd;
            ev.width  = LOWORD(lParam);
            ev.height = HIWORD(lParam);
            break;

     case WM_TIMER :
            ev.type   = TimerNotify;
            ev.win    = hwnd;
            break;

        case WM_CLOSE :
            if (ezx.auto_quit) {
                ez_quit ();
                return 0L;
            } else {
                ev.type  = WindowClose;
                ev.win   = hwnd;
            }
            break;

        default:
            return DefWindowProc (hwnd, msg, wParam, lParam);

    } /* End message decoding */

    /* Call the window callback. */
    ez_func_call (&ev);

    /* Swap Double buffer */
    if (ezx.dbuf_dc != None) ez_dbuf_swap (ev.win);

    return 0L;
}


/*
 * Return 1 if key_sim is a Ctrl, Shift or Alt key, else 0.
*/

int ez_is_modifier (KeySym key_sym)
{
    switch (key_sym) {
        case XK_Control_L :
        case XK_Control_R :
        case XK_Shift_L :
        case XK_Shift_R :
        case XK_Alt_L :
        case XK_Alt_R :
            return 1;
        default : break;
    }
    return 0;
}


/*
 * Return 1 if the key is repeated, else 0.
*/

int ez_is_repetition (LPARAM lParam)
{
    return (lParam & (1L << 30)) ? 1 : 0;
}


/*
 * Set this window as the current window, and associate or free the
 * drawing Handles if necessary.
*/

void ez_cur_win (Ez_window win)
{
    if (ezx.dc_win == win) return;
    if (ezx.dc_win != None && ezx.dc_win != ezx.dbuf_win)
        ReleaseDC (ezx.dc_win, ezx.hdc);

    ezx.dc_win = win;
    if (ezx.dc_win == None) return;

    if (ezx.dc_win != ezx.dbuf_win)
         ezx.hdc = GetDC (ezx.dc_win);
    else ezx.hdc = ezx.dbuf_dc;

    if (ezx.hpen   != NULL) SelectObject (ezx.hdc, ezx.hpen);
    if (ezx.hbrush != NULL) SelectObject (ezx.hdc, ezx.hbrush);
    if (ezx.font[ezx.nfont] != NULL)
        SelectObject (ezx.hdc, ezx.font[ezx.nfont]);
    SetTextColor (ezx.hdc, ezx.color);
}


/*
 * Create a pen to draw with the current color and thickness.
*/

void ez_update_pen (void)
{
    if (ezx.hpen != NULL) DeleteObject (ezx.hpen);
    ezx.hpen = CreatePen (PS_SOLID, (ezx.thick == 1) ? 0 : ezx.thick, ezx.color);
    if (ezx.dc_win != None) SelectObject (ezx.hdc, ezx.hpen);
}

#endif /* EZ_BASE_ */


/*
 * Initialize the random numbers generator
*/

void ez_random_init (void)
{
#ifdef EZ_BASE_XLIB
    srandom ((int) time (NULL));
#elif defined EZ_BASE_WIN32
    srand ((int) time (NULL));
#endif /* EZ_BASE_ */
}


/*
 * Return insertion point in the sorted list ezx.win_l
*/

int ez_win_list_find (Ez_window win)
{
    int m1, m2, mid;

    m1 = 0; m2 = ezx.win_nb;
    while (m2-m1 > 0) {
        mid = (m1+m2) / 2;    /* Middle s.t. m1 <= mid < m2 */

        if (win > ezx.win_l[mid])
             m1 = mid+1;   /* Search in [mid+1 .. m2] */
        else m2 = mid;     /* Search in [m1 .. mid]   */
    }

    return m1;
}


/*
 * Insert a window in the sorted list ezx.win_l
 * (win != None, and win not already member of the list)
 * Return 0 on success, -1 on error.
*/

int ez_win_list_insert (Ez_window win)
{
    int k;

    if (ezx.win_nb >= EZ_WIN_MAX) {
        ez_error ("ez_win_list_insert: too many windows\n");
        return -1;
    }
    k = ez_win_list_find (win);

    /* Insert in position k */
    if (k < ezx.win_nb)
        memmove ( ezx.win_l+k+1, ezx.win_l+k,
                 (ezx.win_nb-k)*sizeof(Ez_window) );
    ezx.win_nb++;
    ezx.win_l[k] = win;
    return 0;
}


/*
 * Suppress the window from the sorted list ezx.win_l
 * Return 0 on success, -1 on error.
*/

int ez_win_list_remove (Ez_window win)
{
    int k = ez_win_list_find (win);

    if (k >= ezx.win_nb || ezx.win_l[k] != win) {
        ez_error ("ez_win_list_remove: can't find window\n");
        return -1;
    }

    ezx.win_nb--;
    if (k < ezx.win_nb)
        memmove ( ezx.win_l+k, ezx.win_l+k+1,
                 (ezx.win_nb-k)*sizeof(Ez_window) );
    return 0;
}


/*
 * Associate a property to a window.
 * Return 0 on success, -1 on error.
*/

int ez_prop_set (Ez_window win, XContext prop, void *value)
{
#ifdef EZ_BASE_XLIB
    if (XSaveContext (ezx.display, win, prop, (XPointer) value) != 0) {
        ez_error ("ez_prop_set: XSaveContext failed\n");
        return -1;
    }
#elif defined EZ_BASE_WIN32
    if (SetProp (win, prop, (HANDLE) value) == 0) {
        ez_error ("ez_prop_set: SetProp failed\n");
        return -1;
    }
#endif /* EZ_BASE_ */
    return 0;
}


/*
 * Retrieve a property associated to a window.
 * Return 0 on success, -1 on error.
*/

int ez_prop_get (Ez_window win, XContext prop, void **value)
{
    if (value == NULL) return -1;

#ifdef EZ_BASE_XLIB
    *value = NULL;
    if (XFindContext (ezx.display, win, prop, (XPointer*) value) != 0)
        return -1;
#elif defined EZ_BASE_WIN32
    *value = GetProp (win, prop);
#endif /* EZ_BASE_ */
    return 0;
}


/*
 * Delete the property associated to the window.
 * Return 0 on success, -1 on error.
*/

int ez_prop_destroy (Ez_window win, XContext prop)
{
#ifdef EZ_BASE_XLIB
    return XDeleteContext (ezx.display, win, prop) == 0 ? 0 : -1;
#elif defined EZ_BASE_WIN32
    return RemoveProp (win, prop) == NULL ? -1 : 0;
#endif /* EZ_BASE_ */
}


/*
 * Retrieve the data associated to a window.
 * Return 0 on success, -1 on error.
*/

int ez_info_get (Ez_window win, Ez_win_info **info)
{
    if (ez_prop_get (win, ezx.info_prop, (void **) info) < 0) return -1;
    if (*info == NULL) {
        ez_error ("ez_info_get: can't get Ez_win_info\n");
        return -1;
    }
    return 0;
}


/*
 * Associate a callback func to a window.
 * A new call overwrite the previous callback.
 * To deactivate the callback, call with func = NULL.
 * Return 0 on success, -1 on error.
*/

int ez_func_set (Ez_window win, Ez_func func)
{
    Ez_win_info *info;
    if (ez_info_get (win, &info) < 0) return -1;
    info->func = func;
    return 0;
}


/*
 * Retrieve the callback associated to the window.
 * Return 0 on success, -1 on error.
*/

int ez_func_get (Ez_window win, Ez_func *func)
{
    Ez_win_info *info;
    if (ez_info_get (win, &info) < 0) return -1;
    *func = info->func;
    return 0;
}


/*
 * Check if a callback has been defined for this window.
 * If yes, call the callback and return 0, else return -1.
*/

int ez_func_call (Ez_event *ev)
{
    Ez_func func;

    /* No drawable */
    if (ev->win == None) return -1;

#ifdef EZ_BASE_XLIB
    /* NoExpose and GraphicsExpose does not involve pixmaps */
    if (ev->type == NoExpose || ev->type == GraphicsExpose) return -1;
#endif /* EZ_BASE_ */

    /* Is there a callback? */
    if (ez_func_get (ev->win, &func) < 0) return -1;
    if (func == NULL) return -1;

    /* Call the callback */
    func (ev);

    return 0;
}


/*
 * Initialize the double buffer mode, which allows to redraw the pixels without
 * blinking.
*/

void ez_dbuf_init (void)
{
#ifdef EZ_BASE_XLIB
    int m1, m2;
    /* Load the DBE extension */
    if (XdbeQueryExtension (ezx.display, &m1, &m2) == 0)
        ez_error ("ez_dbuf_init: DBE extension failed\n");
    ezx.dbuf_pix = None;
#elif defined EZ_BASE_WIN32
    ezx.dbuf_dc  = None;
#endif /* EZ_BASE_ */
    ezx.dbuf_win = None;
}


/*
 * Store dbuf in win.
 * Return 0 on success, -1 on error.
*/

int ez_dbuf_set (Ez_window win, XdbeBackBuffer dbuf)
{
    Ez_win_info *info;
    if (ez_info_get (win, &info) < 0) return -1;
    info->dbuf = dbuf;
    return 0;
}


/*
 * Retrieve dbuf from win.
 * Return 0 on success, -1 on error.
*/

int ez_dbuf_get (Ez_window win, XdbeBackBuffer *dbuf)
{
    Ez_win_info *info;
    if (ez_info_get (win, &info) < 0) return -1;
    *dbuf = info->dbuf;
    return 0;
}


/*
 * Prepare the double buffer for the swap.
*/

void ez_dbuf_preswap (Ez_window win)
{
#ifdef EZ_BASE_XLIB
    ezx.dbuf_win = win;
#elif defined EZ_BASE_WIN32
    ez_cur_win (win);
    ez_window_get_size (win, &ezx.dbuf_w, &ezx.dbuf_h);
    ezx.hMemBmp = CreateCompatibleBitmap (ezx.hdc, ezx.dbuf_w, ezx.dbuf_h);
    ezx.hOldBmp = (HBITMAP) SelectObject (ezx.dbuf_dc, ezx.hMemBmp);
    ez_cur_win (None);
    ezx.dbuf_win = win;
#endif /* EZ_BASE_ */
}


/*
 * Swap the buffers for the window.
*/

void ez_dbuf_swap (Ez_window win)
{
#ifdef EZ_BASE_XLIB
    XdbeSwapInfo swap_info[1];
    swap_info[0].swap_window = win;
    swap_info[0].swap_action = XdbeUndefined;
    XdbeSwapBuffers (ezx.display, swap_info, 1);
#elif defined EZ_BASE_WIN32
    ez_cur_win (None);
    ezx.dbuf_win = None;
    ez_cur_win (win);
    BitBlt (ezx.hdc, 0, 0, ezx.dbuf_w, ezx.dbuf_h, ezx.dbuf_dc, 0, 0, SRCCOPY);
    SelectObject (ezx.dbuf_dc, ezx.hOldBmp);
    DeleteObject (ezx.hMemBmp);
#endif /* EZ_BASE_ */
}


/*
 * Initialize the fonts.
*/

void ez_font_init (void)
{
    int i;
    for (i = 0; i < EZ_FONT_MAX; i++)
        ezx.font[i] = NULL;
    ez_font_load (0, "6x13");
    ez_font_load (1, "8x16");
    ez_font_load (2, "10x20");
    ez_font_load (3, "12x24");
}


/*
 * Free the fonts
*/

void ez_font_delete (void)
{
#ifdef EZ_BASE_XLIB
    int i;
    for (i = 0; i < EZ_FONT_MAX; i++)
    if (ezx.font[i] != NULL)
      { XFreeFont (ezx.display, ezx.font[i]); ezx.font[i] = NULL; }
#elif defined EZ_BASE_WIN32
    int i;
    for (i = 0; i < EZ_FONT_MAX; i++)
    if (ezx.font[i] != 0)
       { DeleteObject (ezx.font[i]); ezx.font[i] = NULL; }
#endif /* EZ_BASE_ */
}


/*
 * Initialize colors.
 *
 * Author: Regis.Barbanchon@lif.univ-mrs.fr
 * This code was extracted from MyX_Color.c
*/

int ez_color_init (void)  {

#ifdef EZ_BASE_XLIB

    ezx.visual = DefaultVisual (ezx.display, ezx.screen_num);

    switch (ezx.visual->class) {
        case PseudoColor : ez_init_PseudoColor (); break;
        case TrueColor   : ez_init_TrueColor   (); break;
        default : ez_error ("ez_init_color: unsupported Visual\n");
                  return -1;
    }

#endif /* EZ_BASE_ */

    /* Predefined colors */
    ez_black   = ez_get_RGB (0, 0, 0);
    ez_white   = ez_get_RGB (255, 255, 255);
    ez_grey    = ez_get_RGB (150, 150, 150);
    ez_red     = ez_get_RGB (255, 0, 0);
    ez_green   = ez_get_RGB (0, 255, 0);
    ez_blue    = ez_get_RGB (0, 0, 255);
    ez_yellow  = ez_get_RGB (255, 255, 0);
    ez_cyan    = ez_get_RGB (0, 255, 255);
    ez_magenta = ez_get_RGB (255, 0, 255);
    return 0;
}


#ifdef EZ_BASE_XLIB

/*
 * Store a color in the palette.
*/

void ez_set_palette (Ez_uint32 pixel, int R, int G, int B,
    int max, int inCube)
{
    XColor c;

    c.pixel = pixel;
    c.red   = R / (double) max * 0xFFFF ;
    c.green = G / (double) max * 0xFFFF ;
    c.blue  = B / (double) max * 0xFFFF ;
    c.flags = DoRed | DoGreen | DoBlue;
    XStoreColor (ezx.display, ezx.pseudoColor.colormap, &c);
    if (inCube) ezx.pseudoColor.palette [R][G][B] = c.pixel;
    ezx.pseudoColor.samples[c.pixel] = c;
}


/*
 * Initialize color palette.
*/

void ez_init_PseudoColor (void)
{
    int i, j, k;
    Ez_uint32 pixel;

    ez_get_RGB = ez_get_RGB_pseudo_color;

    ezx.pseudoColor.colormap =
        XCreateColormap (ezx.display, ezx.root_win, ezx.visual, AllocAll);

    for (i = 0, pixel = 0; i < 3; i++)
    for (j = 0; j < 6; j++)
    for (k = 0; k < 6; k++, pixel++)
        ez_set_palette (pixel, i, j, k, 5, 1);

    for (i = 0 ; i < 40; i++, pixel++)
        ez_set_palette (pixel, i, i, i, 39, 0);

    for (i = 3; i < 6; i++)
    for (j = 0; j < 6; j++)
    for (k = 0; k < 6; k++, pixel++)
        ez_set_palette (pixel, i, j, k, 5, 1);
}


/*
 * Initialize the true color mode.
*/

void ez_init_TrueColor (void)
{
    ez_init_channel (&ezx.trueColor.blue , ezx.visual-> blue_mask);
    ez_init_channel (&ezx.trueColor.green, ezx.visual->green_mask);
    ez_init_channel (&ezx.trueColor.red  , ezx.visual->  red_mask);
}


/*
 * Initialize a color channel.
*/

void ez_init_channel (Ez_channel *channel, Ez_uint32 mask)
{
    channel->mask = mask;
    channel->shift = channel->length = channel->max = 0;
    if (! mask) return;
    while ( (mask & 1) == 0 ) { channel->shift  ++; mask >>= 1; }
    while ( (mask & 1) == 1 ) { channel->length ++; mask >>= 1; }
    channel->max = channel->mask >> channel->shift;
}


/*
 * Compute a color for R,G,B levels between 0 and 255.
*/

Ez_uint32 ez_get_RGB_true_color (Ez_uint8 r, Ez_uint8 g, Ez_uint8 b)
{
    return r >> (8 - ezx.trueColor.red  .length) << ezx.trueColor.red  .shift |
           g >> (8 - ezx.trueColor.green.length) << ezx.trueColor.green.shift |
           b >> (8 - ezx.trueColor.blue .length) << ezx.trueColor.blue .shift ;
}

Ez_uint32 ez_get_RGB_pseudo_color (Ez_uint8 r, Ez_uint8 g, Ez_uint8 b)
{
     if (r == g && g == b)
         return ezx.pseudoColor.samples[108 + (int)r * 40/256].pixel;
     return ezx.pseudoColor.palette[r / 51][g / 51][b / 51];
}

#elif defined EZ_BASE_WIN32

Ez_uint32 ez_get_RGB_win32 (Ez_uint8 r, Ez_uint8 g, Ez_uint8 b)
{
     return RGB (r, g, b);
}


/*
 * Convert keyboard events from win32 to X11
*/

int ez_keydown_convert (WPARAM wParam, LPARAM lParam, KeySym *k, char **n, char **s)
{
    char *es = "";

    switch (wParam) {
        case VK_PAUSE   : *k = XK_Pause  ; *n = "XK_Pause"  ; *s = es ; break;
        case VK_PRIOR   : *k = XK_Prior  ; *n = "XK_Prior"  ; *s = es ; break;
        case VK_NEXT    : *k = XK_Next   ; *n = "XK_Next"   ; *s = es ; break;
        case VK_END     : *k = XK_End    ; *n = "XK_End"    ; *s = es ; break;
        case VK_HOME    : *k = XK_Home   ; *n = "XK_Home"   ; *s = es ; break;
        case VK_LEFT    : *k = XK_Left   ; *n = "XK_Left"   ; *s = es ; break;
        case VK_UP      : *k = XK_Up     ; *n = "XK_Up"     ; *s = es ; break;
        case VK_RIGHT   : *k = XK_Right  ; *n = "XK_Right"  ; *s = es ; break;
        case VK_DOWN    : *k = XK_Down   ; *n = "XK_Down"   ; *s = es ; break;
        case VK_INSERT  : *k = XK_Insert ; *n = "XK_Insert" ; *s = es ; break;
        case VK_DELETE  : *k = XK_Delete ; *n = "XK_Delete" ; *s = es ; break;
        case VK_F1      : *k = XK_F1     ; *n = "XK_F1"     ; *s = es ; break;
        case VK_F2      : *k = XK_F2     ; *n = "XK_F2"     ; *s = es ; break;
        case VK_F3      : *k = XK_F3     ; *n = "XK_F3"     ; *s = es ; break;
        case VK_F4      : *k = XK_F4     ; *n = "XK_F4"     ; *s = es ; break;
        case VK_F5      : *k = XK_F5     ; *n = "XK_F5"     ; *s = es ; break;
        case VK_F6      : *k = XK_F6     ; *n = "XK_F6"     ; *s = es ; break;
        case VK_F7      : *k = XK_F7     ; *n = "XK_F7"     ; *s = es ; break;
        case VK_F8      : *k = XK_F8     ; *n = "XK_F8"     ; *s = es ; break;
        case VK_F9      : *k = XK_F9     ; *n = "XK_F9"     ; *s = es ; break;
        case VK_F10     : *k = XK_F10    ; *n = "XK_F10"    ; *s = es ; break;

        case VK_SHIFT   :
                if (lParam & (1L << 20))
                    { *k = XK_Shift_R   ; *n = "XK_Shift_R"   ; *s = es; }
               else { *k = XK_Shift_L   ; *n = "XK_Shift_L"   ; *s = es; } break;
        case VK_CONTROL :
                if (lParam & (1L << 24))
                    { *k = XK_Control_R ; *n = "XK_Control_R" ; *s = es; }
               else { *k = XK_Control_L ; *n = "XK_Control_L" ; *s = es; } break;
        case VK_MENU    :
                if (lParam & (1L << 24))
                    { *k = XK_Alt_R     ; *n = "XK_Alt_R"     ; *s = es; }
               else { *k = XK_Alt_L     ; *n = "XK_Alt_L"     ; *s = es; } break;
        /* BUG: AltGr generates Control_L + ALt_R */

        /* Symbols still to be processed, please help! */
     /* case VK_CAPITAL  : *k = XK_ ; *n = "XK_"; *s = es; break; */
     /* case VK_NUMLOCK  : *k = XK_ ; *n = "XK_"; *s = es; break; */
     /* case VK_SCROLL   : *k = XK_ ; *n = "XK_"; *s = es; break; */

        default : *k = 0; *n = es; *s = es; return -1;
    }

    return 0;
}

int ez_keychar_convert (WPARAM wParam, KeySym *k, char **n, char **s)
{
    char *es = "";

    switch (wParam) {
        case VK_BACK   : *k = XK_BackSpace ; *n = "XK_BackSpace" ; *s = es  ; break;
        case VK_TAB    : *k = XK_Tab       ; *n = "XK_Tab"       ; *s = es  ; break;
        case VK_RETURN : *k = XK_Return    ; *n = "XK_Return"    ; *s = es  ; break;
        case VK_ESCAPE : *k = XK_Escape    ; *n = "XK_Escape"    ; *s = es  ; break;
        case VK_SPACE  : *k = XK_space     ; *n = "XK_space"     ; *s = " " ; break;

        case 0x0021    : *k = XK_exclam    ; *n = "XK_exclam"    ; *s = "!" ; break;
        case 0x0022    : *k = XK_quotedbl  ; *n = "XK_quotedbl"  ; *s = "\""; break;
        case 0x0023    : *k = XK_numbersign; *n = "XK_numbersign"; *s = "#" ; break;
        case 0x0024    : *k = XK_dollar    ; *n = "XK_dollar"    ; *s = "$" ; break;
        case 0x0025    : *k = XK_percent   ; *n = "XK_percent"   ; *s = "%" ; break;
        case 0x0026    : *k = XK_ampersand ; *n = "XK_ampersand" ; *s = "&" ; break;
        case 0x0027    : *k = XK_apostrophe; *n = "XK_apostrophe"; *s = "'" ; break;
        case 0x0028    : *k = XK_parenleft ; *n = "XK_parenleft" ; *s = "(" ; break;
        case 0x0029    : *k = XK_parenright; *n = "XK_parenright"; *s = ")" ; break;
        case 0x002a    : *k = XK_asterisk  ; *n = "XK_asterisk"  ; *s = "*" ; break;
        case 0x002b    : *k = XK_plus      ; *n = "XK_plus"      ; *s = "+" ; break;
        case 0x002c    : *k = XK_comma     ; *n = "XK_comma"     ; *s = "," ; break;
        case 0x002d    : *k = XK_minus     ; *n = "XK_minus"     ; *s = "-" ; break;
        case 0x002e    : *k = XK_period    ; *n = "XK_period"    ; *s = "." ; break;
        case 0x002f    : *k = XK_slash     ; *n = "XK_slash"     ; *s = "/" ; break;

        case 0x0030 : *k = XK_0        ; *n = "XK_0"        ; *s = "0" ; break;
        case 0x0031 : *k = XK_1        ; *n = "XK_1"        ; *s = "1" ; break;
        case 0x0032 : *k = XK_2        ; *n = "XK_2"        ; *s = "2" ; break;
        case 0x0033 : *k = XK_3        ; *n = "XK_3"        ; *s = "3" ; break;
        case 0x0034 : *k = XK_4        ; *n = "XK_4"        ; *s = "4" ; break;
        case 0x0035 : *k = XK_5        ; *n = "XK_5"        ; *s = "5" ; break;
        case 0x0036 : *k = XK_6        ; *n = "XK_6"        ; *s = "6" ; break;
        case 0x0037 : *k = XK_7        ; *n = "XK_7"        ; *s = "7" ; break;
        case 0x0038 : *k = XK_8        ; *n = "XK_8"        ; *s = "8" ; break;
        case 0x0039 : *k = XK_9        ; *n = "XK_9"        ; *s = "9" ; break;
        case 0x003a : *k = XK_colon    ; *n = "XK_colon"    ; *s = ":" ; break;
        case 0x003b : *k = XK_semicolon; *n = "XK_semicolon"; *s = ";" ; break;
        case 0x003c : *k = XK_less     ; *n = "XK_less"     ; *s = "<" ; break;
        case 0x003d : *k = XK_equal    ; *n = "XK_equal"    ; *s = "=" ; break;
        case 0x003e : *k = XK_greater  ; *n = "XK_greater"  ; *s = ">" ; break;
        case 0x003f : *k = XK_question ; *n = "XK_question" ; *s = "?" ; break;
        case 0x0040 : *k = XK_at       ; *n = "XK_at"       ; *s = "@" ; break;
        case 0x0041 : *k = XK_A        ; *n = "XK_A"        ; *s = "A" ; break;
        case 0x0042 : *k = XK_B        ; *n = "XK_B"        ; *s = "B" ; break;
        case 0x0043 : *k = XK_C        ; *n = "XK_C"        ; *s = "C" ; break;
        case 0x0044 : *k = XK_D        ; *n = "XK_D"        ; *s = "D" ; break;
        case 0x0045 : *k = XK_E        ; *n = "XK_E"        ; *s = "E" ; break;
        case 0x0046 : *k = XK_F        ; *n = "XK_F"        ; *s = "F" ; break;
        case 0x0047 : *k = XK_G        ; *n = "XK_G"        ; *s = "G" ; break;
        case 0x0048 : *k = XK_H        ; *n = "XK_H"        ; *s = "H" ; break;
        case 0x0049 : *k = XK_I        ; *n = "XK_I"        ; *s = "I" ; break;
        case 0x004a : *k = XK_J        ; *n = "XK_J"        ; *s = "J" ; break;
        case 0x004b : *k = XK_K        ; *n = "XK_K"        ; *s = "K" ; break;
        case 0x004c : *k = XK_L        ; *n = "XK_L"        ; *s = "L" ; break;
        case 0x004d : *k = XK_M        ; *n = "XK_M"        ; *s = "M" ; break;
        case 0x004e : *k = XK_N        ; *n = "XK_N"        ; *s = "N" ; break;
        case 0x004f : *k = XK_O        ; *n = "XK_O"        ; *s = "O" ; break;
        case 0x0050 : *k = XK_P        ; *n = "XK_P"        ; *s = "P" ; break;
        case 0x0051 : *k = XK_Q        ; *n = "XK_Q"        ; *s = "Q" ; break;
        case 0x0052 : *k = XK_R        ; *n = "XK_R"        ; *s = "R" ; break;
        case 0x0053 : *k = XK_S        ; *n = "XK_S"        ; *s = "S" ; break;
        case 0x0054 : *k = XK_T        ; *n = "XK_T"        ; *s = "T" ; break;
        case 0x0055 : *k = XK_U        ; *n = "XK_U"        ; *s = "U" ; break;
        case 0x0056 : *k = XK_V        ; *n = "XK_V"        ; *s = "V" ; break;
        case 0x0057 : *k = XK_W        ; *n = "XK_W"        ; *s = "W" ; break;
        case 0x0058 : *k = XK_X        ; *n = "XK_X"        ; *s = "X" ; break;
        case 0x0059 : *k = XK_Y        ; *n = "XK_Y"        ; *s = "Y" ; break;
        case 0x005a : *k = XK_Z        ; *n = "XK_Z"        ; *s = "Z" ; break;

        case 0x005b : *k = XK_bracketleft  ; *n = "XK_bracketleft"  ; *s = "[" ; break;
        case 0x005c : *k = XK_backslash    ; *n = "XK_backslash"    ; *s = "\\"; break;
        case 0x005d : *k = XK_bracketright ; *n = "XK_bracketright" ; *s = "]" ; break;
        case 0x005e : *k = XK_asciicircum  ; *n = "XK_asciicircum"  ; *s = "^" ; break;
        case 0x005f : *k = XK_underscore   ; *n = "XK_underscore"   ; *s = "_" ; break;
        case 0x0060 : *k = XK_grave        ; *n = "XK_grave"        ; *s = "`" ; break;

        case 0x0061 : *k = XK_a        ; *n = "XK_a"        ; *s = "a" ; break;
        case 0x0062 : *k = XK_b        ; *n = "XK_b"        ; *s = "b" ; break;
        case 0x0063 : *k = XK_c        ; *n = "XK_c"        ; *s = "c" ; break;
        case 0x0064 : *k = XK_d        ; *n = "XK_d"        ; *s = "d" ; break;
        case 0x0065 : *k = XK_e        ; *n = "XK_e"        ; *s = "e" ; break;
        case 0x0066 : *k = XK_f        ; *n = "XK_f"        ; *s = "f" ; break;
        case 0x0067 : *k = XK_g        ; *n = "XK_g"        ; *s = "g" ; break;
        case 0x0068 : *k = XK_h        ; *n = "XK_h"        ; *s = "h" ; break;
        case 0x0069 : *k = XK_i        ; *n = "XK_i"        ; *s = "i" ; break;
        case 0x006a : *k = XK_j        ; *n = "XK_j"        ; *s = "j" ; break;
        case 0x006b : *k = XK_k        ; *n = "XK_k"        ; *s = "k" ; break;
        case 0x006c : *k = XK_l        ; *n = "XK_l"        ; *s = "l" ; break;
        case 0x006d : *k = XK_m        ; *n = "XK_m"        ; *s = "m" ; break;
        case 0x006e : *k = XK_n        ; *n = "XK_n"        ; *s = "n" ; break;
        case 0x006f : *k = XK_o        ; *n = "XK_o"        ; *s = "o" ; break;
        case 0x0070 : *k = XK_p        ; *n = "XK_p"        ; *s = "p" ; break;
        case 0x0071 : *k = XK_q        ; *n = "XK_q"        ; *s = "q" ; break;
        case 0x0072 : *k = XK_r        ; *n = "XK_r"        ; *s = "r" ; break;
        case 0x0073 : *k = XK_s        ; *n = "XK_s"        ; *s = "s" ; break;
        case 0x0074 : *k = XK_t        ; *n = "XK_t"        ; *s = "t" ; break;
        case 0x0075 : *k = XK_u        ; *n = "XK_u"        ; *s = "u" ; break;
        case 0x0076 : *k = XK_v        ; *n = "XK_v"        ; *s = "v" ; break;
        case 0x0077 : *k = XK_w        ; *n = "XK_w"        ; *s = "w" ; break;
        case 0x0078 : *k = XK_x        ; *n = "XK_x"        ; *s = "x" ; break;
        case 0x0079 : *k = XK_y        ; *n = "XK_y"        ; *s = "y" ; break;
        case 0x007a : *k = XK_z        ; *n = "XK_z"        ; *s = "z" ; break;

        case 0x007b : *k = XK_braceleft  ; *n = "XK_braceleft"  ; *s = "{" ; break;
        case 0x007c : *k = XK_bar        ; *n = "XK_bar"        ; *s = "|" ; break;
        case 0x007d : *k = XK_braceright ; *n = "XK_braceright" ; *s = "}" ; break;
        case 0x007e : *k = XK_asciitilde ; *n = "XK_asciitilde" ; *s = "~" ; break;

        case 0x0080 : *k = XK_EuroSign   ; *n = "XK_EuroSign"   ; *s = "\200" ; break;

        case 0x00a0 : *k = XK_nobreakspace   ; *n = "XK_nobreakspace"   ; *s = "\240"; break;
        case 0x00a1 : *k = XK_exclamdown     ; *n = "XK_exclamdown"     ; *s = "\241"; break;
        case 0x00a2 : *k = XK_cent           ; *n = "XK_cent"           ; *s = "\242"; break;
        case 0x00a3 : *k = XK_sterling       ; *n = "XK_sterling"       ; *s = "\243"; break;
        case 0x00a4 : *k = XK_currency       ; *n = "XK_currency"       ; *s = "\244"; break;
        case 0x00a5 : *k = XK_yen            ; *n = "XK_yen"            ; *s = "\245"; break;
        case 0x00a6 : *k = XK_brokenbar      ; *n = "XK_brokenbar"      ; *s = "\246"; break;
        case 0x00a7 : *k = XK_section        ; *n = "XK_section"        ; *s = "\247"; break;
        case 0x00a8 : *k = XK_diaeresis      ; *n = "XK_diaeresis"      ; *s = "\250"; break;
        case 0x00a9 : *k = XK_copyright      ; *n = "XK_copyright"      ; *s = "\251"; break;
        case 0x00aa : *k = XK_ordfeminine    ; *n = "XK_ordfeminine"    ; *s = "\252"; break;
        case 0x00ab : *k = XK_guillemotleft  ; *n = "XK_guillemotleft"  ; *s = "\253"; break;
        case 0x00ac : *k = XK_notsign        ; *n = "XK_notsign"        ; *s = "\254"; break;
        case 0x00ad : *k = XK_hyphen         ; *n = "XK_hyphen"         ; *s = "\255"; break;
        case 0x00ae : *k = XK_registered     ; *n = "XK_registered"     ; *s = "\256"; break;
        case 0x00af : *k = XK_macron         ; *n = "XK_macron"         ; *s = "\257"; break;
        case 0x00b0 : *k = XK_degree         ; *n = "XK_degree"         ; *s = "\260"; break;
        case 0x00b1 : *k = XK_plusminus      ; *n = "XK_plusminus"      ; *s = "\261"; break;
        case 0x00b2 : *k = XK_twosuperior    ; *n = "XK_twosuperior"    ; *s = "\262"; break;
        case 0x00b3 : *k = XK_threesuperior  ; *n = "XK_threesuperior"  ; *s = "\263"; break;
        case 0x00b4 : *k = XK_acute          ; *n = "XK_acute"          ; *s = "\264"; break;
        case 0x00b5 : *k = XK_mu             ; *n = "XK_mu"             ; *s = "\265"; break;
        case 0x00b6 : *k = XK_paragraph      ; *n = "XK_paragraph"      ; *s = "\266"; break;
        case 0x00b7 : *k = XK_periodcentered ; *n = "XK_periodcentered" ; *s = "\267"; break;
        case 0x00b8 : *k = XK_cedilla        ; *n = "XK_cedilla"        ; *s = "\270"; break;
        case 0x00b9 : *k = XK_onesuperior    ; *n = "XK_onesuperior"    ; *s = "\271"; break;
        case 0x00ba : *k = XK_masculine      ; *n = "XK_masculine"      ; *s = "\272"; break;
        case 0x00bb : *k = XK_guillemotright ; *n = "XK_guillemotright" ; *s = "\273"; break;
        case 0x00bc : *k = XK_onequarter     ; *n = "XK_onequarter"     ; *s = "\274"; break;
        case 0x00bd : *k = XK_onehalf        ; *n = "XK_onehalf"        ; *s = "\275"; break;
        case 0x00be : *k = XK_threequarters  ; *n = "XK_threequarters"  ; *s = "\276"; break;
        case 0x00bf : *k = XK_questiondown   ; *n = "XK_questiondown"   ; *s = "\277"; break;
        case 0x00c0 : *k = XK_Agrave         ; *n = "XK_Agrave"         ; *s = "\300"; break;
        case 0x00c1 : *k = XK_Aacute         ; *n = "XK_Aacute"         ; *s = "\301"; break;
        case 0x00c2 : *k = XK_Acircumflex    ; *n = "XK_Acircumflex"    ; *s = "\302"; break;
        case 0x00c3 : *k = XK_Atilde         ; *n = "XK_Atilde"         ; *s = "\303"; break;
        case 0x00c4 : *k = XK_Adiaeresis     ; *n = "XK_Adiaeresis"     ; *s = "\304"; break;
        case 0x00c5 : *k = XK_Aring          ; *n = "XK_Aring"          ; *s = "\305"; break;
        case 0x00c6 : *k = XK_AE             ; *n = "XK_AE"             ; *s = "\306"; break;
        case 0x00c7 : *k = XK_Ccedilla       ; *n = "XK_Ccedilla"       ; *s = "\307"; break;
        case 0x00c8 : *k = XK_Egrave         ; *n = "XK_Egrave"         ; *s = "\310"; break;
        case 0x00c9 : *k = XK_Eacute         ; *n = "XK_Eacute"         ; *s = "\311"; break;
        case 0x00ca : *k = XK_Ecircumflex    ; *n = "XK_Ecircumflex"    ; *s = "\312"; break;
        case 0x00cb : *k = XK_Ediaeresis     ; *n = "XK_Ediaeresis"     ; *s = "\313"; break;
        case 0x00cc : *k = XK_Igrave         ; *n = "XK_Igrave"         ; *s = "\314"; break;
        case 0x00cd : *k = XK_Iacute         ; *n = "XK_Iacute"         ; *s = "\315"; break;
        case 0x00ce : *k = XK_Icircumflex    ; *n = "XK_Icircumflex"    ; *s = "\316"; break;
        case 0x00cf : *k = XK_Idiaeresis     ; *n = "XK_Idiaeresis"     ; *s = "\317"; break;
        case 0x00d0 : *k = XK_ETH            ; *n = "XK_ETH"            ; *s = "\320"; break;
        case 0x00d1 : *k = XK_Ntilde         ; *n = "XK_Ntilde"         ; *s = "\321"; break;
        case 0x00d2 : *k = XK_Ograve         ; *n = "XK_Ograve"         ; *s = "\322"; break;
        case 0x00d3 : *k = XK_Oacute         ; *n = "XK_Oacute"         ; *s = "\323"; break;
        case 0x00d4 : *k = XK_Ocircumflex    ; *n = "XK_Ocircumflex"    ; *s = "\324"; break;
        case 0x00d5 : *k = XK_Otilde         ; *n = "XK_Otilde"         ; *s = "\325"; break;
        case 0x00d6 : *k = XK_Odiaeresis     ; *n = "XK_Odiaeresis"     ; *s = "\326"; break;
        case 0x00d7 : *k = XK_multiply       ; *n = "XK_multiply"       ; *s = "\327"; break;
        case 0x00d8 : *k = XK_Oslash         ; *n = "XK_Oslash"         ; *s = "\330"; break;
        case 0x00d9 : *k = XK_Ugrave         ; *n = "XK_Ugrave"         ; *s = "\331"; break;
        case 0x00da : *k = XK_Uacute         ; *n = "XK_Uacute"         ; *s = "\332"; break;
        case 0x00db : *k = XK_Ucircumflex    ; *n = "XK_Ucircumflex"    ; *s = "\333"; break;
        case 0x00dc : *k = XK_Udiaeresis     ; *n = "XK_Udiaeresis"     ; *s = "\334"; break;
        case 0x00dd : *k = XK_Yacute         ; *n = "XK_Yacute"         ; *s = "\335"; break;
        case 0x00de : *k = XK_THORN          ; *n = "XK_THORN"          ; *s = "\336"; break;
        case 0x00df : *k = XK_ssharp         ; *n = "XK_ssharp"         ; *s = "\337"; break;
        case 0x00e0 : *k = XK_agrave         ; *n = "XK_agrave"         ; *s = "\340"; break;
        case 0x00e1 : *k = XK_aacute         ; *n = "XK_aacute"         ; *s = "\341"; break;
        case 0x00e2 : *k = XK_acircumflex    ; *n = "XK_acircumflex"    ; *s = "\342"; break;
        case 0x00e3 : *k = XK_atilde         ; *n = "XK_atilde"         ; *s = "\343"; break;
        case 0x00e4 : *k = XK_adiaeresis     ; *n = "XK_adiaeresis"     ; *s = "\344"; break;
        case 0x00e5 : *k = XK_aring          ; *n = "XK_aring"          ; *s = "\345"; break;
        case 0x00e6 : *k = XK_ae             ; *n = "XK_ae"             ; *s = "\346"; break;
        case 0x00e7 : *k = XK_ccedilla       ; *n = "XK_ccedilla"       ; *s = "\347"; break;
        case 0x00e8 : *k = XK_egrave         ; *n = "XK_egrave"         ; *s = "\350"; break;
        case 0x00e9 : *k = XK_eacute         ; *n = "XK_eacute"         ; *s = "\351"; break;
        case 0x00ea : *k = XK_ecircumflex    ; *n = "XK_ecircumflex"    ; *s = "\352"; break;
        case 0x00eb : *k = XK_ediaeresis     ; *n = "XK_ediaeresis"     ; *s = "\353"; break;
        case 0x00ec : *k = XK_igrave         ; *n = "XK_igrave"         ; *s = "\354"; break;
        case 0x00ed : *k = XK_iacute         ; *n = "XK_iacute"         ; *s = "\355"; break;
        case 0x00ee : *k = XK_icircumflex    ; *n = "XK_icircumflex"    ; *s = "\356"; break;
        case 0x00ef : *k = XK_idiaeresis     ; *n = "XK_idiaeresis"     ; *s = "\357"; break;
        case 0x00f0 : *k = XK_eth            ; *n = "XK_eth"            ; *s = "\360"; break;
        case 0x00f1 : *k = XK_ntilde         ; *n = "XK_ntilde"         ; *s = "\361"; break;
        case 0x00f2 : *k = XK_ograve         ; *n = "XK_ograve"         ; *s = "\362"; break;
        case 0x00f3 : *k = XK_oacute         ; *n = "XK_oacute"         ; *s = "\363"; break;
        case 0x00f4 : *k = XK_ocircumflex    ; *n = "XK_ocircumflex"    ; *s = "\364"; break;
        case 0x00f5 : *k = XK_otilde         ; *n = "XK_otilde"         ; *s = "\365"; break;
        case 0x00f6 : *k = XK_odiaeresis     ; *n = "XK_odiaeresis"     ; *s = "\366"; break;
        case 0x00f7 : *k = XK_division       ; *n = "XK_division"       ; *s = "\367"; break;
        case 0x00f8 : *k = XK_oslash         ; *n = "XK_oslash"         ; *s = "\370"; break;
        case 0x00f9 : *k = XK_ugrave         ; *n = "XK_ugrave"         ; *s = "\371"; break;
        case 0x00fa : *k = XK_uacute         ; *n = "XK_uacute"         ; *s = "\372"; break;
        case 0x00fb : *k = XK_ucircumflex    ; *n = "XK_ucircumflex"    ; *s = "\373"; break;
        case 0x00fc : *k = XK_udiaeresis     ; *n = "XK_udiaeresis"     ; *s = "\374"; break;
        case 0x00fd : *k = XK_yacute         ; *n = "XK_yacute"         ; *s = "\375"; break;
        case 0x00fe : *k = XK_thorn          ; *n = "XK_thorn"          ; *s = "\376"; break;
        case 0x00ff : *k = XK_ydiaeresis     ; *n = "XK_ydiaeresis"     ; *s = "\377"; break;

        /* Symbols still to be processed, please help! */
     /* case ''     : *k = XK_Scroll_Lock    ; *n = "XK_Scroll_Lock"    ; *s = ""; break; */
     /* case ''     : *k = XK_Menu           ; *n = "XK_Menu"           ; *s = ""; break; */
     /* case ''     : *k = XK_Num_Lock       ; *n = "XK_Num_Lock"       ; *s = ""; break; */
     /* case ''     : *k = XK_KP_Enter       ; *n = "XK_KP_Enter"       ; *s = ""; break; */
     /* case ''     : *k = XK_KP_Home        ; *n = "XK_KP_Home"        ; *s = ""; break; */
     /* case ''     : *k = XK_KP_Left        ; *n = "XK_KP_Left"        ; *s = ""; break; */
     /* case ''     : *k = XK_KP_Up          ; *n = "XK_KP_Up"          ; *s = ""; break; */
     /* case ''     : *k = XK_KP_Right       ; *n = "XK_KP_Right"       ; *s = ""; break; */
     /* case ''     : *k = XK_KP_Down        ; *n = "XK_KP_Down"        ; *s = ""; break; */
     /* case ''     : *k = XK_KP_Prior       ; *n = "XK_KP_Prior"       ; *s = ""; break; */
     /* case ''     : *k = XK_KP_Next        ; *n = "XK_KP_Next"        ; *s = ""; break; */
     /* case ''     : *k = XK_KP_End         ; *n = "XK_KP_End"         ; *s = ""; break; */
     /* case ''     : *k = XK_KP_Begin       ; *n = "XK_KP_Begin"       ; *s = ""; break; */
     /* case ''     : *k = XK_KP_Equal       ; *n = "XK_KP_Equal"       ; *s = "="; break; */
     /* case ''     : *k = XK_KP_Multiply    ; *n = "XK_KP_Multiply"    ; *s = "*"; break; */
     /* case ''     : *k = XK_KP_Add         ; *n = "XK_KP_Add"         ; *s = "+"; break; */
     /* case ''     : *k = XK_KP_Separator   ; *n = "XK_KP_Separator"   ; *s = ""; break; */
     /* case ''     : *k = XK_KP_Subtract    ; *n = "XK_KP_Subtract"    ; *s = "-"; break; */
     /* case ''     : *k = XK_KP_Divide      ; *n = "XK_KP_Divide"      ; *s = "/"; break; */
     /* case ''     : *k = XK_KP_0           ; *n = "XK_KP_0"           ; *s = "0"; break; */
     /* case ''     : *k = XK_KP_1           ; *n = "XK_KP_1"           ; *s = "1"; break; */
     /* case ''     : *k = XK_KP_2           ; *n = "XK_KP_2"           ; *s = "2"; break; */
     /* case ''     : *k = XK_KP_3           ; *n = "XK_KP_3"           ; *s = "3"; break; */
     /* case ''     : *k = XK_KP_4           ; *n = "XK_KP_4"           ; *s = "4"; break; */
     /* case ''     : *k = XK_KP_5           ; *n = "XK_KP_5"           ; *s = "5"; break; */
     /* case ''     : *k = XK_KP_6           ; *n = "XK_KP_6"           ; *s = "6"; break; */
     /* case ''     : *k = XK_KP_7           ; *n = "XK_KP_7"           ; *s = "7"; break; */
     /* case ''     : *k = XK_KP_8           ; *n = "XK_KP_8"           ; *s = "8"; break; */
     /* case ''     : *k = XK_KP_9           ; *n = "XK_KP_9"           ; *s = "9"; break; */

     /* case ''     : *k = XK_Caps_Lock      ; *n = "XK_Caps_Lock"      ; *s = ""; break; */
     /* case ''     : *k = XK_Meta_L         ; *n = "XK_Meta_L"         ; *s = ""; break; */
     /* case ''     : *k = XK_Meta_R         ; *n = "XK_Meta_R"         ; *s = ""; break; */
     /* case ''     : *k = XK_Alt_L          ; *n = "XK_Alt_L"          ; *s = ""; break; */
     /* case ''     : *k = XK_Alt_R          ; *n = "XK_Alt_R"          ; *s = ""; break; */

        default :
            if (ez_draw_debug())
                printf ("KEYCODE 0x%x\n", (unsigned) wParam);
            *k = 0; *n = es; *s = es; return -1;
    }

    return 0;
}


char *ez_win_msg_name (unsigned int m)
{
    switch (m) {
        case 0x0000 : return "WM_NULL";
        case 0x0001 : return "WM_CREATE";
        case 0x0002 : return "WM_DESTROY";
        case 0x0003 : return "WM_MOVE";
        case 0x0005 : return "WM_SIZE";
        case 0x0006 : return "WM_ACTIVATE";
        case 0x0007 : return "WM_SETFOCUS";
        case 0x0008 : return "WM_KILLFOCUS";
        case 0x000A : return "WM_ENABLE";
        case 0x000B : return "WM_SETREDRAW";
        case 0x000C : return "WM_SETTEXT";
        case 0x000D : return "WM_GETTEXT";
        case 0x000E : return "WM_GETTEXTLENGTH";
        case 0x000F : return "WM_PAINT";
        case 0x0010 : return "WM_CLOSE";
        case 0x0011 : return "WM_QUERYENDSESSION";
        case 0x0013 : return "WM_QUERYOPEN";
        case 0x0016 : return "WM_ENDSESSION";
        case 0x0012 : return "WM_QUIT";
        case 0x0014 : return "WM_ERASEBKGND";
        case 0x0015 : return "WM_SYSCOLORCHANGE";
        case 0x0018 : return "WM_SHOWWINDOW";
        case 0x001A : return "WM_WININICHANGE";
        case 0x001B : return "WM_DEVMODECHANGE";
        case 0x001C : return "WM_ACTIVATEAPP";
        case 0x001D : return "WM_FONTCHANGE";
        case 0x001E : return "WM_TIMECHANGE";
        case 0x001F : return "WM_CANCELMODE";
        case 0x0020 : return "WM_SETCURSOR";
        case 0x0021 : return "WM_MOUSEACTIVATE";
        case 0x0022 : return "WM_CHILDACTIVATE";
        case 0x0023 : return "WM_QUEUESYNC";
        case 0x0024 : return "WM_GETMINMAXINFO";
        case 0x0026 : return "WM_PAINTICON";
        case 0x0027 : return "WM_ICONERASEBKGND";
        case 0x0028 : return "WM_NEXTDLGCTL";
        case 0x002A : return "WM_SPOOLERSTATUS";
        case 0x002B : return "WM_DRAWITEM";
        case 0x002C : return "WM_MEASUREITEM";
        case 0x002D : return "WM_DELETEITEM";
        case 0x002E : return "WM_VKEYTOITEM";
        case 0x002F : return "WM_CHARTOITEM";
        case 0x0030 : return "WM_SETFONT";
        case 0x0031 : return "WM_GETFONT";
        case 0x0032 : return "WM_SETHOTKEY";
        case 0x0033 : return "WM_GETHOTKEY";
        case 0x0037 : return "WM_QUERYDRAGICON";
        case 0x0039 : return "WM_COMPAREITEM";
        case 0x003D : return "WM_GETOBJECT";
        case 0x0041 : return "WM_COMPACTING";
        case 0x0044 : return "WM_COMMNOTIFY";
        case 0x0046 : return "WM_WINDOWPOSCHANGING";
        case 0x0047 : return "WM_WINDOWPOSCHANGED";
        case 0x0048 : return "WM_POWER";
        case 0x004A : return "WM_COPYDATA";
        case 0x004B : return "WM_CANCELJOURNAL";
        case 0x004E : return "WM_NOTIFY";
        case 0x0050 : return "WM_INPUTLANGCHANGEREQUEST";
        case 0x0051 : return "WM_INPUTLANGCHANGE";
        case 0x0052 : return "WM_TCARD";
        case 0x0053 : return "WM_HELP";
        case 0x0054 : return "WM_USERCHANGED";
        case 0x0055 : return "WM_NOTIFYFORMAT";
        case 0x007B : return "WM_CONTEXTMENU";
        case 0x007C : return "WM_STYLECHANGING";
        case 0x007D : return "WM_STYLECHANGED";
        case 0x007E : return "WM_DISPLAYCHANGE";
        case 0x007F : return "WM_GETICON";
        case 0x0080 : return "WM_SETICON";
        case 0x0081 : return "WM_NCCREATE";
        case 0x0082 : return "WM_NCDESTROY";
        case 0x0083 : return "WM_NCCALCSIZE";
        case 0x0084 : return "WM_NCHITTEST";
        case 0x0085 : return "WM_NCPAINT";
        case 0x0086 : return "WM_NCACTIVATE";
        case 0x0087 : return "WM_GETDLGCODE";
        case 0x0088 : return "WM_SYNCPAINT";
        case 0x00A0 : return "WM_NCMOUSEMOVE";
        case 0x00A1 : return "WM_NCLBUTTONDOWN";
        case 0x00A2 : return "WM_NCLBUTTONUP";
        case 0x00A3 : return "WM_NCLBUTTONDBLCLK";
        case 0x00A4 : return "WM_NCRBUTTONDOWN";
        case 0x00A5 : return "WM_NCRBUTTONUP";
        case 0x00A6 : return "WM_NCRBUTTONDBLCLK";
        case 0x00A7 : return "WM_NCMBUTTONDOWN";
        case 0x00A8 : return "WM_NCMBUTTONUP";
        case 0x00A9 : return "WM_NCMBUTTONDBLCLK";
        case 0x00AB : return "WM_NCXBUTTONDOWN";
        case 0x00AC : return "WM_NCXBUTTONUP";
        case 0x00AD : return "WM_NCXBUTTONDBLCLK";
        case 0x00FF : return "WM_INPUT";
        case 0x0100 : return "WM_KEYDOWN";
        case 0x0101 : return "WM_KEYUP";
        case 0x0102 : return "WM_CHAR";
        case 0x0103 : return "WM_DEADCHAR";
        case 0x0104 : return "WM_SYSKEYDOWN";
        case 0x0105 : return "WM_SYSKEYUP";
        case 0x0106 : return "WM_SYSCHAR";
        case 0x0107 : return "WM_SYSDEADCHAR";
        case 0x0109 : return "WM_UNICHAR";
        case 0x010D : return "WM_IME_STARTCOMPOSITION";
        case 0x010E : return "WM_IME_ENDCOMPOSITION";
        case 0x010F : return "WM_IME_COMPOSITION";
        case 0x0110 : return "WM_INITDIALOG";
        case 0x0111 : return "WM_COMMAND";
        case 0x0112 : return "WM_SYSCOMMAND";
        case 0x0113 : return "WM_TIMER";
        case 0x0114 : return "WM_HSCROLL";
        case 0x0115 : return "WM_VSCROLL";
        case 0x0116 : return "WM_INITMENU";
        case 0x0117 : return "WM_INITMENUPOPUP";
        case 0x011F : return "WM_MENUSELECT";
        case 0x0120 : return "WM_MENUCHAR";
        case 0x0121 : return "WM_ENTERIDLE";
        case 0x0122 : return "WM_MENURBUTTONUP";
        case 0x0123 : return "WM_MENUDRAG";
        case 0x0124 : return "WM_MENUGETOBJECT";
        case 0x0125 : return "WM_UNINITMENUPOPUP";
        case 0x0126 : return "WM_MENUCOMMAND";
        case 0x0127 : return "WM_CHANGEUISTATE";
        case 0x0128 : return "WM_UPDATEUISTATE";
        case 0x0129 : return "WM_QUERYUISTATE";
        case 0x0132 : return "WM_CTLCOLORMSGBOX";
        case 0x0133 : return "WM_CTLCOLOREDIT";
        case 0x0134 : return "WM_CTLCOLORLISTBOX";
        case 0x0135 : return "WM_CTLCOLORBTN";
        case 0x0136 : return "WM_CTLCOLORDLG";
        case 0x0137 : return "WM_CTLCOLORSCROLLBAR";
        case 0x0138 : return "WM_CTLCOLORSTATIC";
        case 0x0200 : return "WM_MOUSEMOVE";
        case 0x0201 : return "WM_LBUTTONDOWN";
        case 0x0202 : return "WM_LBUTTONUP";
        case 0x0203 : return "WM_LBUTTONDBLCLK";
        case 0x0204 : return "WM_RBUTTONDOWN";
        case 0x0205 : return "WM_RBUTTONUP";
        case 0x0206 : return "WM_RBUTTONDBLCLK";
        case 0x0207 : return "WM_MBUTTONDOWN";
        case 0x0208 : return "WM_MBUTTONUP";
        case 0x0209 : return "WM_MBUTTONDBLCLK";
        case 0x020A : return "WM_MOUSEWHEEL";
        case 0x020B : return "WM_XBUTTONDOWN";
        case 0x020C : return "WM_XBUTTONUP";
        case 0x020D : return "WM_XBUTTONDBLCLK";
        case 0x0210 : return "WM_PARENTNOTIFY";
        case 0x0211 : return "WM_ENTERMENULOOP";
        case 0x0212 : return "WM_EXITMENULOOP";
        case 0x0213 : return "WM_NEXTMENU";
        case 0x0214 : return "WM_SIZING";
        case 0x0215 : return "WM_CAPTURECHANGED";
        case 0x0216 : return "WM_MOVING";
        case 0x0218 : return "WM_POWERBROADCAST";
        case 0x0220 : return "WM_MDICREATE";
        case 0x0221 : return "WM_MDIDESTROY";
        case 0x0222 : return "WM_MDIACTIVATE";
        case 0x0223 : return "WM_MDIRESTORE";
        case 0x0224 : return "WM_MDINEXT";
        case 0x0225 : return "WM_MDIMAXIMIZE";
        case 0x0226 : return "WM_MDITILE";
        case 0x0227 : return "WM_MDICASCADE";
        case 0x0228 : return "WM_MDIICONARRANGE";
        case 0x0229 : return "WM_MDIGETACTIVE";
        case 0x0230 : return "WM_MDISETMENU";
        case 0x0231 : return "WM_ENTERSIZEMOVE";
        case 0x0232 : return "WM_EXITSIZEMOVE";
        case 0x0233 : return "WM_DROPFILES";
        case 0x0234 : return "WM_MDIREFRESHMENU";
        case 0x0281 : return "WM_IME_SETCONTEXT";
        case 0x0282 : return "WM_IME_NOTIFY";
        case 0x0283 : return "WM_IME_CONTROL";
        case 0x0284 : return "WM_IME_COMPOSITIONFULL";
        case 0x0285 : return "WM_IME_SELECT";
        case 0x0286 : return "WM_IME_CHAR";
        case 0x0288 : return "WM_IME_REQUEST";
        case 0x0290 : return "WM_IME_KEYDOWN";
        case 0x0291 : return "WM_IME_KEYUP";
        case 0x02A1 : return "WM_MOUSEHOVER";
        case 0x02A3 : return "WM_MOUSELEAVE";
        case 0x02A0 : return "WM_NCMOUSEHOVER";
        case 0x02A2 : return "WM_NCMOUSELEAVE";
        case 0x02B1 : return "WM_WTSSESSION_CHANGE";
        case 0x02c0 : return "WM_TABLET_FIRST";
        case 0x02df : return "WM_TABLET_LAST";
        case 0x0300 : return "WM_CUT";
        case 0x0301 : return "WM_COPY";
        case 0x0302 : return "WM_PASTE";
        case 0x0303 : return "WM_CLEAR";
        case 0x0304 : return "WM_UNDO";
        case 0x0305 : return "WM_RENDERFORMAT";
        case 0x0306 : return "WM_RENDERALLFORMATS";
        case 0x0307 : return "WM_DESTROYCLIPBOARD";
        case 0x0308 : return "WM_DRAWCLIPBOARD";
        case 0x0309 : return "WM_PAINTCLIPBOARD";
        case 0x030A : return "WM_VSCROLLCLIPBOARD";
        case 0x030B : return "WM_SIZECLIPBOARD";
        case 0x030C : return "WM_ASKCBFORMATNAME";
        case 0x030D : return "WM_CHANGECBCHAIN";
        case 0x030E : return "WM_HSCROLLCLIPBOARD";
        case 0x030F : return "WM_QUERYNEWPALETTE";
        case 0x0310 : return "WM_PALETTEISCHANGING";
        case 0x0311 : return "WM_PALETTECHANGED";
        case 0x0312 : return "WM_HOTKEY";
        case 0x0317 : return "WM_PRINT";
        case 0x0318 : return "WM_PRINTCLIENT";
        case 0x0319 : return "WM_APPCOMMAND";
        case 0x031A : return "WM_THEMECHANGED";
        case 0x0358 : return "WM_HANDHELDFIRST";
        case 0x035F : return "WM_HANDHELDLAST";
        case 0x0360 : return "WM_AFXFIRST";
        case 0x037F : return "WM_AFXLAST";
        case 0x0380 : return "WM_PENWINFIRST";
        case 0x038F : return "WM_PENWINLAST";
        case 0x8000 : return "WM_APP";
        case EZ_MSG_PAINT : return "EZ_MSG_PAINT";
    }
    return "*** UNKNOWN ***";
}

#endif /* EZ_BASE_ */

