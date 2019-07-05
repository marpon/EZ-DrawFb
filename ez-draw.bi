
#ifndef EZ_DRAW__H
    #define EZ_DRAW__H

    #define EZ_CALLBACK cdecl

    #include once "crt/stdio.bi"
    '#include once "crt/stdlib.bi"
    '#include once "crt/string.bi"
    '#include once "crt/stdarg.bi"
    '#include once "crt/time.bi"

    extern "C"

        #ifdef __FB_WIN32__
            #include once "crt/sys/win32/time.bi"
            #include once "windows.bi"
            #include once "win/wingdi.bi"
            #include once "win/windowsx.bi"
            #ifdef __FB_64BIT__
                #inclib "ez-draw_w64"
            #else
                #inclib "ez-draw_w32"
            #endif
            'const EZ_BASE_WIN32 = 1
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
                    #inclib "ez-draw_l64"
                #else
                    #inclib "ez-draw_l32"
                #endif
                'const EZ_BASE_XLIB = 1
            #else
                #error ==> wrong Os, works only on Windows or Linux
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
                    font(0 to 15)  as HFONT
                    key_sym        as KeySym
                    key_name       as zstring ptr
                    key_string     as zstring ptr
                    start_time     as timeval
                    start_count    as LARGE_INTEGER
                    perf_freq      as double
                #else
                    atom_protoc    as XAtom
                    atom_delwin    as XAtom
                    font(0 to 15)  as XFontStruct ptr
                    depth          as long
                    visual         as Visual ptr
                    pseudoColor    as Ez_PseudoColor
                    trueColor      as Ez_TrueColor
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
                win_l(0 to 1023)   as Ez_window
                win_nb             as long
        end type

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
        dim shared ez_get_RGB as function(byval r as Ez_uint8 , byval g as Ez_uint8 , byval b as Ez_uint8) as Ez_uint32
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

        #ifdef __FB_WIN32__
            const XK_BackSpace = &hff08
            const XK_Tab = &hff09
            const XK_Return = &hff0d
            const XK_Pause = &hff13
            const XK_Scroll_Lock = &hff14
            const XK_Escape = &hff1b
            const XK_Delete = &hffff
            const XK_Home = &hff50
            const XK_Left = &hff51
            const XK_Up = &hff52
            const XK_Right = &hff53
            const XK_Down = &hff54
            const XK_Prior = &hff55
            const XK_Next = &hff56
            const XK_End = &hff57
            const XK_Insert = &hff63
            const XK_Menu = &hff67
            const XK_Num_Lock = &hff7f
            const XK_KP_Enter = &hff8d
            const XK_KP_Home = &hff95
            const XK_KP_Left = &hff96
            const XK_KP_Up = &hff97
            const XK_KP_Right = &hff98
            const XK_KP_Down = &hff99
            const XK_KP_Prior = &hff9a
            const XK_KP_Next = &hff9b
            const XK_KP_End = &hff9c
            const XK_KP_Begin = &hff9d
            const XK_KP_Equal = &hffbd
            const XK_KP_Multiply = &hffaa
            const XK_KP_Add = &hffab
            const XK_KP_Separator = &hffac
            const XK_KP_Subtract = &hffad
            const XK_KP_Divide = &hffaf
            const XK_KP_0 = &hffb0
            const XK_KP_1 = &hffb1
            const XK_KP_2 = &hffb2
            const XK_KP_3 = &hffb3
            const XK_KP_4 = &hffb4
            const XK_KP_5 = &hffb5
            const XK_KP_6 = &hffb6
            const XK_KP_7 = &hffb7
            const XK_KP_8 = &hffb8
            const XK_KP_9 = &hffb9
            const XK_F1 = &hffbe
            const XK_F2 = &hffbf
            const XK_F3 = &hffc0
            const XK_F4 = &hffc1
            const XK_F5 = &hffc2
            const XK_F6 = &hffc3
            const XK_F7 = &hffc4
            const XK_F8 = &hffc5
            const XK_F9 = &hffc6
            const XK_F10 = &hffc7
            const XK_F11 = &hffc8
            const XK_F12 = &hffc9
            const XK_Shift_L = &hffe1
            const XK_Shift_R = &hffe2
            const XK_Control_L = &hffe3
            const XK_Control_R = &hffe4
            const XK_Caps_Lock = &hffe5
            const XK_Meta_L = &hffe7
            const XK_Meta_R = &hffe8
            const XK_Alt_L = &hffe9
            const XK_Alt_R = &hffea
            const XK_space = &h0020
            const XK_exclam = &h0021
            const XK_quotedbl = &h0022
            const XK_numbersign = &h0023
            const XK_dollar = &h0024
            const XK_percent = &h0025
            const XK_ampersand = &h0026
            const XK_apostrophe = &h0027
            const XK_parenleft = &h0028
            const XK_parenright = &h0029
            const XK_asterisk = &h002a
            const XK_plus = &h002b
            const XK_comma = &h002c
            const XK_minus = &h002d
            const XK_period = &h002e
            const XK_slash = &h002f
            const XK_0 = &h0030
            const XK_1 = &h0031
            const XK_2 = &h0032
            const XK_3 = &h0033
            const XK_4 = &h0034
            const XK_5 = &h0035
            const XK_6 = &h0036
            const XK_7 = &h0037
            const XK_8 = &h0038
            const XK_9 = &h0039
            const XK_colon = &h003a
            const XK_semicolon = &h003b
            const XK_less = &h003c
            const XK_equal = &h003d
            const XK_greater = &h003e
            const XK_question = &h003f
            const XK_at = &h0040
            const XK_A = &h0041
            const XK_B = &h0042
            const XK_C = &h0043
            const XK_D = &h0044
            const XK_E = &h0045
            const XK_F = &h0046
            const XK_G = &h0047
            const XK_H = &h0048
            const XK_I = &h0049
            const XK_J = &h004a
            const XK_K = &h004b
            const XK_L = &h004c
            const XK_M = &h004d
            const XK_N = &h004e
            const XK_O = &h004f
            const XK_P = &h0050
            const XK_Q = &h0051
            const XK_R = &h0052
            const XK_S = &h0053
            const XK_T = &h0054
            const XK_U = &h0055
            const XK_V = &h0056
            const XK_W = &h0057
            const XK_X = &h0058
            const XK_Y = &h0059
            const XK_Z = &h005a
            const XK_bracketleft = &h005b
            const XK_backslash = &h005c
            const XK_bracketright = &h005d
            const XK_asciicircum = &h005e
            const XK_underscore = &h005f
            const XK_grave = &h0060
             /' const XK_a = &h0061
            const XK_b = &h0062
            const XK_c = &h0063
            const XK_d = &h0064
            const XK_e = &h0065
            const XK_f = &h0066
            const XK_g = &h0067
            const XK_h = &h0068
            const XK_i = &h0069
            const XK_j = &h006a
            const XK_k = &h006b
            const XK_l = &h006c
            const XK_m = &h006d
            const XK_n = &h006e
            const XK_o = &h006f
            const XK_p = &h0070
            const XK_q = &h0071
            const XK_r = &h0072
            const XK_s = &h0073
            const XK_t = &h0074
            const XK_u = &h0075
            const XK_v = &h0076
            const XK_w = &h0077
            const XK_x = &h0078
            const XK_y = &h0079
            const XK_z = &h007a '/
            const XK_braceleft = &h007b
            const XK_bar = &h007c
            const XK_braceright = &h007d
            const XK_asciitilde = &h007e
            const XK_nobreakspace = &h00a0
            const XK_exclamdown = &h00a1
            const XK_cent = &h00a2
            const XK_sterling = &h00a3
            const XK_currency = &h00a4
            const XK_yen = &h00a5
            const XK_brokenbar = &h00a6
            const XK_section = &h00a7
            const XK_diaeresis = &h00a8
            const XK_copyright = &h00a9
            const XK_ordfeminine = &h00aa
            const XK_guillemotleft = &h00ab
            const XK_notsign = &h00ac
            const XK_hyphen = &h00ad
            const XK_registered = &h00ae
            const XK_macron = &h00af
            const XK_degree = &h00b0
            const XK_plusminus = &h00b1
            const XK_twosuperior = &h00b2
            const XK_threesuperior = &h00b3
            const XK_acute = &h00b4
            const XK_mu = &h00b5
            const XK_paragraph = &h00b6
            const XK_periodcentered = &h00b7
            const XK_cedilla = &h00b8
            const XK_onesuperior = &h00b9
            const XK_masculine = &h00ba
            const XK_guillemotright = &h00bb
            const XK_onequarter = &h00bc
            const XK_onehalf = &h00bd
            const XK_threequarters = &h00be
            const XK_questiondown = &h00bf
            const XK_Agrave = &h00c0
            const XK_Aacute = &h00c1
            const XK_Acircumflex = &h00c2
            const XK_Atilde = &h00c3
            const XK_Adiaeresis = &h00c4
            const XK_Aring = &h00c5
            const XK_AE = &h00c6
            const XK_Ccedilla = &h00c7
            const XK_Egrave = &h00c8
            const XK_Eacute = &h00c9
            const XK_Ecircumflex = &h00ca
            const XK_Ediaeresis = &h00cb
            const XK_Igrave = &h00cc
            const XK_Iacute = &h00cd
            const XK_Icircumflex = &h00ce
            const XK_Idiaeresis = &h00cf
            const XK_ETH = &h00d0
            const XK_Ntilde = &h00d1
            const XK_Ograve = &h00d2
            const XK_Oacute = &h00d3
            const XK_Ocircumflex = &h00d4
            const XK_Otilde = &h00d5
            const XK_Odiaeresis = &h00d6
            const XK_multiply = &h00d7
            const XK_Oslash = &h00d8
            const XK_Ugrave = &h00d9
            const XK_Uacute = &h00da
            const XK_Ucircumflex = &h00db
            const XK_Udiaeresis = &h00dc
            const XK_Yacute = &h00dd
            const XK_THORN = &h00de
            const XK_ssharp = &h00df
             /' const XK_agrave = &h00e0
            const XK_aacute = &h00e1
            const XK_acircumflex = &h00e2
            const XK_atilde = &h00e3
            const XK_adiaeresis = &h00e4
            const XK_aring = &h00e5
            const XK_ae = &h00e6
            const XK_ccedilla = &h00e7
            const XK_egrave = &h00e8
            const XK_eacute = &h00e9
            const XK_ecircumflex = &h00ea
            const XK_ediaeresis = &h00eb
            const XK_igrave = &h00ec
            const XK_iacute = &h00ed
            const XK_icircumflex = &h00ee
            const XK_idiaeresis = &h00ef
            const XK_eth = &h00f0
            const XK_ntilde = &h00f1
            const XK_ograve = &h00f2
            const XK_oacute = &h00f3
            const XK_ocircumflex = &h00f4
            const XK_otilde = &h00f5
            const XK_odiaeresis = &h00f6
            const XK_division = &h00f7
            const XK_oslash = &h00f8
            const XK_ugrave = &h00f9
            const XK_uacute = &h00fa
            const XK_ucircumflex = &h00fb
            const XK_udiaeresis = &h00fc
            const XK_yacute = &h00fd
            const XK_thorn = &h00fe'/
            const XK_ydiaeresis = &h00ff
            const XK_EuroSign = &h20ac
        #endif

    end extern
#ENDIF

