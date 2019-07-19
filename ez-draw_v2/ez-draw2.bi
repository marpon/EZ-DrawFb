#ifndef EZ_DRAW__H
    #define EZ_DRAW__H

    #define EZ_CALLBACK cdecl

    #include once "crt/stdio.bi"

    extern "C"

        #ifdef __FB_WIN32__
            #include once "crt/sys/win32/time.bi"
            #include once "windows.bi"
            #include once "win/wingdi.bi"
            #include once "win/windowsx.bi"
            #include once "X11/keysym.bi"
            #ifdef __FB_64BIT__
                #inclib "ez-draw2_w64"
            #else
                #inclib "ez-draw2_w32"
            #endif
        #else
            #ifdef __FB_LINUX__
                #include once "crt/sys/linux/time.bi"
                #include once "X11/Xlib.bi"
                #include once "X11/Xutil.bi"
                #include once "X11/Xresource.bi"
                #include once "X11/keysym.bi"
                #include once "X11/extensions/Xdbe.bi"
                #inclib "X11"
                #inclib "Xext"
                #inclib "m"
                #ifdef __FB_64BIT__
                    #inclib "ez-draw2_l64"
                #else
                    #inclib "ez-draw2_l32"
                #endif
            #else
                #error ==> Wrong Os, works only on Windows or Linux
            #endif
        #endif


        const EZ_FONT_MAX = 16
        const EZ_WIN_MAX = 1024
        type Ez_int8 as byte
        type Ez_uint8 as ubyte
        type Ez_uint16 as ushort
        type Ez_int16 as short
        type Ez_uint32 as ulong
        type Ez_int32 as long
        type Ez_uint as ulong
        #define EZ_ROUND(x) iif((x) < 0, clng((x) - 0.5), clng((x) + 0.5))

        #ifdef __FB_WIN32__
            type Ez_window as HWND
        #else
            type Ez_window as Window
        #endif

        extern ez_black as Ez_uint32
        extern ez_white as Ez_uint32
        extern ez_grey as Ez_uint32
        extern ez_red as Ez_uint32
        extern ez_green as Ez_uint32
        extern ez_blue as Ez_uint32
        extern ez_yellow as Ez_uint32
        extern ez_cyan as Ez_uint32
        extern ez_magenta as Ez_uint32


        #if defined(__FB_DOS__) or defined(__FB_UNIX__)
            type Ez_channel
                    mask           as Ez_uint32
                    shift          as Ez_uint32
                    length         as Ez_uint32
                    max            as Ez_uint32
            end type

            type Ez_TrueColor
                    green          as Ez_channel
                    red            as Ez_channel
                    blue           as Ez_channel
            end type

            type Ez_PseudoColor
                    colormap       as Colormap
                    palette(0 to 5 , 0 to 5 , 0 to 5) as Ez_uint32
                    samples(0 to 255) as XColor
            end type
        #endif

        const EZ_TIMER_MAX = 100

        type Ez_timer
                win                as Ez_window
                expiration         as timeval
        end type

        type Ez_Align as long

        enum
            EZ_AA = 183200
            EZ_TL
            EZ_TC
            EZ_TR
            EZ_ML
            EZ_MC
            EZ_MR
            EZ_BL
            EZ_BC
            EZ_BR
            EZ_BB
            EZ_TLF
            EZ_TCF
            EZ_TRF
            EZ_MLF
            EZ_MCF
            EZ_MRF
            EZ_BLF
            EZ_BCF
            EZ_BRF
            EZ_CC
        end enum

        #ifdef __FB_WIN32__
            type XdbeBackBuffer as HDC
            type XEvent as MSG
            type KeySym as long
            type XContext as LPCTSTR
            type XPoint as POINT

            #define None NULL
        #endif

        type Ez_X
                #ifdef __FB_WIN32__
                    hand_prog      as HINSTANCE
                    wnd_class      as WNDCLASSEX
                    hdc            as HDC
                    dc_win         as Ez_window
                    dbuf_dc        as HDC
                #else
                    display        as Display ptr
                    screen_num     as long
                    gc             as GC
                    dbuf_pix       as XdbeBackBuffer
                #endif

                dbuf_win           as Ez_window

                #ifdef __FB_WIN32__
                    dbuf_w         as long
                    dbuf_h         as long
                    hOldBmp        as HBITMAP
                    hMemBmp        as HBITMAP
                    hpen           as HPEN
                    hbrush         as HBRUSH
                    font(0 to EZ_FONT_MAX -1)  as HFONT
                    key_sym        as KeySym
                    key_name       as zstring ptr
                    key_string     as zstring ptr
                    start_time     as timeval
                    start_count    as LARGE_INTEGER
                    perf_freq      as double
                #else
                    atom_protoc    as XAtom
                    atom_delwin    as XAtom
                    font(0 to EZ_FONT_MAX-1)  as XFontStruct ptr
                    depth          as long
                    visual         as Visual ptr
                    pseudoColor    as Ez_PseudoColor
                    trueColor      as Ez_TrueColor
                    pixcolor       as Ez_uint32
                #endif

                display_width      as long
                display_height     as long
                root_win           as Ez_window
                info_prop          as XContext
                mv_x               as long
                mv_y               as long
                mv_win             as Ez_window
                color              as Ez_uint32
                thick              as long
                nfont              as long
                timer_l(0 to 99)   as Ez_timer
                timer_nb           as long
                main_loop          as long
                last_expose        as long
                auto_quit          as long
                mouse_b            as long
                win_l(0 to EZ_WIN_MAX -1)   as Ez_window
                win_nb             as long
                ipen               as byte
                icolor             as byte
                ithick             as byte
                ifont              as byte
        end type

        extern ezx as Ez_X

        #ifdef __FB_WIN32__
            const KeyPress = 2
            const KeyRelease = 3
            const ButtonPress = 4
            const ButtonRelease = 5
            const MotionNotify = 6
            const Expose = 12
            const ConfigureNotify = 22
            const ClientMessage = 33
            const LASTEvent = 35
            const EZ_TIMER1 = 208

            enum
                EZ_MSG_PAINT = WM_APP + 1
                EZ_MSG_LAST
            end enum
        #endif

        enum
            #ifdef __FB_WIN32__
                WindowClose = 35 + 1
            #else
                WindowClose = LASTEvent + 1
            #endif

            TimerNotify
            EzLastEvent
        end enum

        type Ez_event
                type       as long
                win        as Ez_window
                mx         as long
                my         as long
                mb         as long
                width      as long
                height     as long
                key_sym    as KeySym
                key_name   as zstring * 80
                key_string as zstring * 80
                key_count  as long
                xev        as XEvent
        end type

        type Ez_func as sub(byval ev as Ez_event ptr)

        type Ez_win_info
                func       as Ez_func
                data       as any ptr
                dbuf       as XdbeBackBuffer
                show       as long
        end type

        declare function ez_init() as long
        declare function ez_window_create(byval w as long , byval h as long , byval name as const zstring ptr , byval func as any ptr) as Ez_window
        declare function ez_window_createex(byval x as long , byval y as long , byval w as long , byval h as long , _
                            byval name as const zstring ptr , byval func as any ptr) as Ez_window

        declare function ez_window_get_id(byval win as Ez_window) as long
        declare sub ez_window_destroy(byval win as Ez_window)
        declare sub ez_window_show(byval win as Ez_window , byval val as long)
        declare sub ez_window_set_size(byval win as Ez_window , byval w as long , byval h as long)
        declare sub ez_window_get_size(byval win as Ez_window , byval w as long ptr , byval h as long ptr)
        declare sub ez_window_clear(byval win as Ez_window)
        declare sub ez_window_dbuf(byval win as Ez_window , byval val as long)
        declare sub ez_set_data(byval win as Ez_window , byval data as any ptr)
        declare function ez_get_data(byval win as Ez_window) as any ptr
        declare sub ez_quit()
        declare sub ez_auto_quit(byval val as long)
        declare sub ez_send_expose(byval win as Ez_window)
        declare sub ez_start_timer(byval win as Ez_window , byval delay as long)
        declare sub ez_main_loop()
        declare function ez_random(byval n as long) as long
        declare function ez_get_time() as double
        extern ez_get_RGB as function(byval r as Ez_uint8 , byval g as Ez_uint8 , byval b as Ez_uint8) as Ez_uint32
        declare function ez_get_grey(byval g as Ez_uint8) as Ez_uint32
        declare sub ez_HSV_to_RGB(byval h as double , byval s as double , byval v as double , byval r as Ez_uint8 ptr , byval g as Ez_uint8 ptr , byval b as Ez_uint8 ptr)
        declare function ez_get_HSV(byval h as double , byval s as double , byval v as double) as Ez_uint32
        declare sub ez_set_color(byval color as Ez_uint32)
        declare sub ez_set_thick(byval thick as long)
        declare sub ez_draw_point(byval win as Ez_window , byval x1 as long , byval y1 as long)
        declare sub ez_draw_line(byval win as Ez_window , byval x1 as long , byval y1 as long , byval x2 as long , byval y2 as long)
        declare sub ez_draw_rectangle(byval win as Ez_window , byval x1 as long , byval y1 as long , byval x2 as long , byval y2 as long)
        declare sub ez_fill_rectangle(byval win as Ez_window , byval x1 as long , byval y1 as long , byval x2 as long , byval y2 as long)
        declare sub ez_draw_triangle(byval win as Ez_window , byval x1 as long , byval y1 as long , byval x2 as long , byval y2 as long , byval x3 as long , byval y3 as long)
        declare sub ez_fill_triangle(byval win as Ez_window , byval x1 as long , byval y1 as long , byval x2 as long , byval y2 as long , byval x3 as long , byval y3 as long)
        declare sub ez_draw_circle(byval win as Ez_window , byval x1 as long , byval y1 as long , byval x2 as long , byval y2 as long)
        declare sub ez_fill_circle(byval win as Ez_window , byval x1 as long , byval y1 as long , byval x2 as long , byval y2 as long)
        declare function ez_font_load(byval num as long , byval name as const zstring ptr) as long
        declare sub ez_set_nfont(byval num as long)
        declare sub ez_draw_text(byval win as Ez_window , byval align as Ez_Align , byval x1 as long , byval y1 as long , byval format as const zstring ptr , ...)
        declare sub ez_set_pixel(byval win as Ez_window , byval x1 as long , byval y1 as long, byval color as Ez_uint32)

    end extern
#ENDIF

