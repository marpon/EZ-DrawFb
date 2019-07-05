#include once "ez-draw.bi"
#include once "ez-image.bi"


#define DEF_BALL_MAX_               500
#define DEF_BALL_NB_                20
#define DEF_WIN1_W_                 900
#define DEF_WIN1_H_                 700
#define DEF_DELAY1_                 5


type t_ball
    as long x
    as long y
    as long dx
    as long dy
end type

type t_App_data
    as Ez_image ptr image1
    as Ez_pixmap ptr pixmap1
    as Ez_window win1
    as long win1_h
    as long win1_w
    as t_Ball ball(0 to DEF_BALL_MAX_ - 1)
    as long ball_nb
    as long expose_nb
    as long flag_pix
    as double time_ref
    as double fps
end type


sub SubExit(byval i as long)
    if i <> 0 then
        printf "Error , any key to close"
    else
        printf "Any key to close"
    end if
    'sleep

    end(abs(i))
end sub




private sub ball_init(byval ball as t_ball ptr , byval ball_w as long , byval ball_h as long , byval win_h as long , byval win_w as long)
    ball->x = ez_random(win_w - ball_w)
    ball->y = ez_random(win_h - ball_h)
    ball->dx = 2 + ez_random(6)
    ball->dy = 2 + ez_random(6)
end sub


private sub ball_next_step(byval ball as t_ball ptr , byval ball_w as long , byval ball_h as long , byval win_h as long , byval win_w as long)
    ball->x += ball->dx
    ball->y += ball->dy

    if (ball->x < 0) then
        ball->x = 0
        ball->dx = - ball->dx
    elseif (ball->x > win_w - ball_w) then
        ball->x = win_w - ball_w
        ball->dx = - ball->dx
    end if

    if (ball->y < 0) then
        ball->y = 0
        ball->dy = - ball->dy
    elseif (ball->y > win_h - ball_h) then
        ball->y = win_h - ball_h
        ball->dy = - ball->dy
    end if
end sub


private sub ball_draw_image(byval win as Ez_window , byval img as Ez_image ptr , byval ball as t_ball ptr)
    ez_image_paint(win , img , ball->x , ball->y)
end sub


private sub ball_draw_pixmap(byval win as Ez_window , byval pix as Ez_pixmap ptr , byval ball as t_ball ptr)
    ez_pixmap_paint(win , pix , ball->x , ball->y)
end sub





private sub App_data_init(byval a as t_App_data ptr , byref filename as string)
    dim         as long i

    a->image1 = ez_image_load(strptr(filename))
    if (a->image1 = NULL) then SubExit(1)

    a->pixmap1 = ez_pixmap_create_from_image(a->image1)
    if (a->pixmap1 = NULL) then SubExit(1)

    a->win1_h = DEF_WIN1_H_
    a->win1_w = DEF_WIN1_W_

    a->ball_nb = DEF_BALL_NB_
    for i = 0 to a->ball_nb - 1
        ball_init(@a->ball(i) , a->image1->width , a->image1->height , a->win1_h , a->win1_w)
    next
    a->time_ref = ez_get_time()
    a->fps = - 1
    a->expose_nb = 0

    a->flag_pix = 0
end sub


private sub App_data_destroy(byval a as t_App_data ptr)
    ez_image_destroy(a->image1)
    ez_pixmap_destroy(a->pixmap1)
end sub


private sub update_fps(byval a as t_App_data ptr)

    dim as double time_now = ez_get_time()
    a->expose_nb += 1
    if (time_now - a->time_ref < 0.5) then exit sub
    a->fps = a->expose_nb / (time_now - a->time_ref)
    a->expose_nb = 0
    a->time_ref = time_now
end sub


private sub win1_on_expose(byval ev as Ez_event ptr)

    dim as t_App_data ptr a = ez_get_data(ev->win)
    dim         as long i

    update_fps(a)

    for i = 0 to a->ball_nb - 1
        if (a->flag_pix) then
            ball_draw_pixmap(a->win1 , a->pixmap1 , @a->ball(i))
        else
            ball_draw_image(a->win1 , a->image1 , @a->ball(i))
        end if
    next
    ez_set_color(ez_black)
    ez_draw_text(a->win1 , EZ_BLF , 10 , a->win1_h - 10 , !"+-: balls %d   p: pixmap %s" , a->ball_nb , iif(a->flag_pix , "ON ", "OFF"))
    if (a->fps > 0) then
        ez_draw_text(a->win1 , EZ_BRF , a->win1_w - 10 , a->win1_h - 10 , !"fps %.1f" , a->fps)
    end if
end sub


private sub win1_on_key_press(byval ev as Ez_event ptr)

    dim as t_App_data ptr a = ez_get_data(ev->win)

    select case ev->key_sym
        case XK_q , XK_q + 32
            ez_quit()
        case XK_minus , XK_KP_Subtract
            a->ball_nb -= 1
            if (a->ball_nb < 1) then a->ball_nb = 1
        case XK_KP_Add , XK_plus
            a->ball_nb += 1
            if (a->ball_nb > DEF_BALL_MAX_) then
                a->ball_nb = DEF_BALL_MAX_
            else
                ball_init(@a->ball(a->ball_nb - 1) , a->image1->width , a->image1->height , a->win1_h , a->win1_w)
            end if
        case XK_p , XK_p + 32
            a->flag_pix = not a->flag_pix

    end select

    ez_send_expose(a->win1)
end sub


private sub win1_on_configure_notify(byval ev as Ez_event ptr)

    dim as t_App_data ptr a = ez_get_data(ev->win)

    a->win1_w = ev->width
    a->win1_h = ev->height
end sub


private sub win1_on_timer_notify(byval ev as Ez_event ptr)

    dim as t_App_data ptr a = ez_get_data(ev->win)
    dim         as long i

    for i = 0 to a->ball_nb - 1
        ball_next_step(@a->ball(i) , a->image1->width , a->image1->height , a->win1_h , a->win1_w)
    next
    ez_start_timer(a->win1 , DEF_DELAY1_)
    ez_send_expose(a->win1)
end sub


private sub win1_on_event  EZ_CALLBACK (byval ev as Ez_event ptr)

    select case(ev->type)
        case Expose
            win1_on_expose(ev)
        case KeyPress
            win1_on_key_press(ev)
        case ConfigureNotify
            win1_on_configure_notify(ev)
        case TimerNotify
            win1_on_timer_notify(ev)
    end select
end sub




private sub submain()

    dim as string filename = "ball2.gif"
    dim         as t_App_data a



    if (ez_init() < 0) then subexit(1)
    App_data_init(@a , filename)

    a.win1 = ez_window_create(DEF_WIN1_W_ , DEF_WIN1_H_ , "Demo 17: Pixmaps" , procptr( win1_on_event()  ))
    ez_set_data(a.win1 , @a)
    ez_window_dbuf(a.win1 , 1)

    ez_start_timer(a.win1 , DEF_DELAY1_)
    ez_main_loop()

    App_data_destroy(@a)
    subexit(0)
end sub

submain()
