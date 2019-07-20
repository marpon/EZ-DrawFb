#include once "ez-image2.bi"


type  App_data
    image1 as Ez_image ptr
    win1   as Ez_window
end type

private sub SubExit(byval i as long)
    if i <> 0 THEN
        printf ("Error , press any key to close")
		sleep 2000
    else
        printf ("Closing now")
    END IF
    'sleep

    end(abs(i))
END SUB

private sub win_to_bmp(byval win as Ez_window, byref str1 as string)
	'print "win to bmp " & win
	dim as Ez_rgb ptr prgb = ez_win_to_rgb(win)
	'print "prgb " & prgb
	'print "prgb->pixels_rgb " & prgb->pixels_rgb
	if prgb = NULL THEN return
	dim as long w
	dim as long h
	dim as Ez_uint8 ptr rgb1= ez_get_rgb_data_free(prgb, @w , @h)
	'print "rgb1 " & rgb1
	'print w , h
	if rgb1 = NULL THEN return
	savebmp(strptr(str1), rgb1 , w, h)
	deallocate(rgb1)
end sub

private sub image_to_bmp(byval img as Ez_image ptr, byref str1 as string)
	dim as Ez_rgb ptr prgb = ez_image_to_rgb(img)
	if prgb = NULL THEN return
	dim as long w
	dim as long h
	dim as Ez_uint8 ptr rgb1= ez_get_rgb_data_free(prgb, @w , @h)
	if rgb1 = NULL THEN return
	savebmp(strptr(str1), rgb1 , w, h)
	deallocate(rgb1)
end sub

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
    if (ev->key_sym = XK_q or ev->key_sym = XK_q_) then ez_quit ()
	if (ev->key_sym = XK_s or ev->key_sym = XK_s_) then
		dim as App_data ptr a = ez_get_data (ev->win)
		'print "ev->win " & ev->win
		'print "a->win1 " & a->win1
		'print "ezx.mv_win " & ezx.mv_win
		win_to_bmp(a->win1, "win.bmp")
		image_to_bmp(a->image1, "image.bmp")


	end if
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
	 /' dim as Ez_image ptr pgimg1
	pgimg1 = ez_image_create(200 , 200)

	for x as long = 0 to 199
		for y as long = 0 to 199
			ez_set_pixel_image(pgimg1,  x , y, ez_compose_rgba_color(x + 55, y + 55, x + y ,  x * y /4 + 57) )
		next
    NEXT
	ez_image_set_alpha(pgimg1, 1)
	ez_image_set_opacity(pgimg1, 0)
'/
    app_data_init (@a, filename)
    '/* Resize window for image */
	 /' a.image1 = pgimg1 '/
    a.win1 = ez_window_createex (  200, 400, a.image1->width, a.image1->height, strptr(filename), procptr(win1_on_event()))


   'ez_image_paint (a.win1, a.image1, 0, 0)
    ez_set_data (a.win1, @a)
    ez_window_dbuf(a.win1, 1)

	'print "a.win1 " & a.win1
	'print "ezx.mv_win " & ezx.mv_win
    ez_main_loop ()

    app_data_destroy (@a)
    SubExit(0)
end sub
        submain ()