
#include once "ez-draw.bi"
#include once "ez-image.bi"


sub win1_on_event EZ_CALLBACK(byval ev as Ez_event ptr)                '/* Called by ez_main_loop() */                                                '/* for each event on win1   */
    select case (ev->type)

        case Expose                                                    '/* We must redraw everything */
            ez_set_color(ez_red)
            ez_draw_text(ev->win, EZ_MC, 200, 150, _
                !"To quit, press the key 'q', or escape key \nor click on the Close button of the window")
            exit sub

        case KeyPress
            'print hex(ev->key_sym)                                     '/* A key was pressed */
            select case (ev->key_sym)
                case XK_Q, XK_Q + 32
                    ez_quit()
                    exit sub
                case XK_E, XK_E + 32
                    ez_quit()
                    exit sub
                case XK_Escape
                    ez_quit()
                    exit sub

            end select
        case ButtonPress
            'print ev->mx , ev->my
            ez_quit()
    end select
end sub



sub SubExit(byval i as long)
    if i <> 0 THEN
        printf "Error , any key to close"
    else
        printf "Any key to close"
    END IF
    'sleep

    end(abs(i))
END SUB

    if (ez_init() < 0) then SubExit(1)

    ez_window_create (400, 300, "Demo 02: Window and events", procptr(win1_on_event()))

    ez_main_loop ()
SubExit(0)
