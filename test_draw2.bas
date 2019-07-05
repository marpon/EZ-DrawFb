#include once "ez-draw.bi"



 /' / *Global variables * / '/
dim shared  as Ez_window win1
dim shared  as Ez_window win2
dim shared  as Ez_window win3
dim shared  as long show2


sub win3_on_expose(byval ev as Ez_event ptr)
    ez_draw_text(ev->win , EZ_TL , 10 , 10 , "If you close this window, it will be destroyed.")
end sub


'/* The user has clicked on the Close button of the window */

sub win3_on_window_close(byval ev as Ez_event ptr)
    ez_window_destroy(win3)
    win3 = None
end sub


sub win3_on_event EZ_CALLBACK(byval ev as Ez_event ptr)

    select case(ev->type)
        case Expose
            win3_on_expose(ev)

        case WindowClose
            win3_on_window_close(ev)
    end select
end sub


sub win2_on_expose(byval ev as Ez_event ptr)
    ez_draw_text(ev->win , EZ_TL , 10 , 10 , "If you close this window, it will be hidden.")
end sub


sub win2_on_window_close(byval ev as Ez_event ptr)

    '(void) ev;
    ez_window_show(win2 , 0)
    show2 = 0
end sub


sub win2_on_event EZ_CALLBACK(byval ev as Ez_event ptr)
    select case(ev->type)
        case Expose
            win2_on_expose(ev)
        case WindowClose
            win2_on_window_close(ev)
    end select
end sub


sub win1_on_expose(byval ev as Ez_event ptr)

    ez_draw_text(ev->win , EZ_TL , 10 , 10 , _
            !"Click in this window (to get the keyboard focus),\n" _
            & !"then type :\n" _
            & !"    - on 'm' to show or hide window 2;\n" _
            & !"    - on 'c' to create or destroy window 3;\n" _
            & !"    - on 'q' to quit.\n" _
            & !"\n" _
            & !"If you close this window, the program will end.")
end sub


sub win1_on_key_press(byval ev as Ez_event ptr)

    select case(ev->key_sym)
        case XK_q , XK_q + 32
            ez_quit()

        case XK_m , XK_m + 32
            show2 = not show2
            ez_window_show(win2 , show2)

        case XK_c , XK_c + 32
            if (win3 = 0) then                   '/* if the window doesn't exist, create it */
                win3 = ez_window_create(380 , 220 , "Window 3" , procptr(win3_on_event()))
            else
                ez_window_destroy(win3)
                win3 = 0
            end if
    end select
end sub


sub win1_on_window_close(byval ev as Ez_event ptr)

    '(void) ev;
    ez_quit()
end sub


sub win1_on_event EZ_CALLBACK(byval ev as Ez_event ptr)

    select case(ev->type)
        case Expose
            win1_on_expose(ev)
        case KeyPress
            win1_on_key_press(ev)
        case WindowClose
            win1_on_window_close(ev)
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

win1 = ez_window_create(400 , 300 , "Demo 07: Several windows" , procptr(win1_on_event()))
win2 = ez_window_create(400 , 200 , "Window 2" , procptr(win2_on_event()))
ez_window_show(win2 , show2)

     /' /* By default, closing any window will cause the end of the program.
       We change this behaviour: for now on, closing any window will send
       a WindowClose event for this window. */ '/
ez_auto_quit(0)

ez_main_loop()
SubExit(0)


