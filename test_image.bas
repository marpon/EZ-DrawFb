#include once "ez-image.bi"


type  App_data
    image1 as Ez_image ptr
    win1   as Ez_window
end type

private sub SubExit(byval i as long)
    if i <> 0 THEN
        print "Error , any key to close"
    else
        print "Any key to close"
    END IF
    'sleep

    end(abs(i))
END SUB

private sub app_data_init (byval a as App_data ptr, byref filename as string)

    a->image1 = ez_image_load(strptr(filename))                   '/* Load an image */
    if (a->image1 = NULL) then SubExit(1)
end sub


private sub app_data_destroy (byval a as App_data ptr)
    ez_image_destroy (a->image1)                           '/* Destroy image */
end sub


private sub win1_on_expose (byval ev as Ez_event ptr)

    dim as App_data ptr a = ez_get_data (ev->win)

    ez_image_paint (a->win1, a->image1, 0, 0)              '/* Display image */
end sub


private sub win1_on_key_press (byval ev as Ez_event ptr)
    if (ev->key_sym = XK_q ) then ez_quit ()
end sub


private sub win1_on_event EZ_CALLBACK(byval ev as Ez_event ptr)

    if(ev->type = Expose)   then
        win1_on_expose    (ev)

    elseif(ev->type = KeyPress ) then
        win1_on_key_press (ev)
    end if

end sub


private sub submain ()

    dim as string filename = "tux2.gif"
    dim as App_data a


    if (ez_init() < 0) then SubExit(1)

    app_data_init (@a, filename)
    '/* Resize window for image */
    a.win1 = ez_window_create (  a.image1->width, a.image1->height, strptr(filename), procptr(win1_on_event()))
    ez_set_data (a.win1, @a)
    ez_window_dbuf(a.win1, 1)

    ez_main_loop ()

    app_data_destroy (@a)
    SubExit(0)
end sub
        submain ()