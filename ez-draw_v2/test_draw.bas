
#include once "ez-draw2.bi"
#include once "ez-image2.bi"
dim shared as Ez_image ptr pgimg1
dim shared as string str2

sub create_bmp(byval win as Ez_window, byref str1 as string)
	dim as Ez_rgb ptr prgb = ez_win_to_rgb(win)
	if prgb = NULL THEN return
	dim as long w
	dim as long h
	dim as Ez_uint8 ptr rgb1= ez_get_rgb_data_free(prgb, @w , @h)
	if rgb1 = NULL THEN return
	savebmp(strptr(str1), rgb1 , w, h)
	deallocate(rgb1)
end sub

sub create_jpg(byval win as Ez_window, byref str1 as string)
	dim as Ez_rgb ptr prgb = ez_win_to_rgb(win)
	if prgb = NULL THEN return
	dim as long w
	dim as long h
	dim as Ez_uint8 ptr rgb1= ez_get_rgb_data_free(prgb, @w , @h)
	if rgb1 = NULL THEN return
	savejpeg(strptr(str1), rgb1 , w, h)
	deallocate(rgb1)
end sub

sub win1_on_event EZ_CALLBACK(byval ev as Ez_event ptr)                '/* Called by ez_main_loop() */ '/* for each event on win1   */
    select case (ev->type)

        case Expose                                                    '/* We must redraw everything */
            ez_set_color(ez_red)
            ez_draw_text(ev->win, EZ_MC, 200, 150, _
                !"To quit, press the key 'q', or escape key \nor click on the Close button of the window\n or click on window")
			dim x as long
			dim y as long
			dim as double t1, t2
			t1= timer()
			for x = 0 to 99
				for y = 0 to 99
					ez_set_pixel(ev->win, x + 60, y + 20 , ez_get_RGB(x + 100, y + 100, x + y + 50))
                NEXT

            NEXT
			t1 = timer() - t1
			t2= timer()
			for x = 0 to 99
				for y = 0 to 99
					if (y < 50 and x < 50) or (y > 49 and x > 49) THEN
						'ez_set_color(ez_blue)
						ez_set_pixel(ev->win, x + 240, y + 20 , ez_red)
					else
						'ez_set_color(ez_red)
						ez_set_pixel(ev->win, x + 240, y + 20 , ez_blue)
                    END IF


					'ez_draw_point(ev->win, x + 240, y + 20 )
                NEXT

            NEXT
			t2 = timer() - t2
			ez_set_color(ez_blue)
			dim as string str1 = "t1 = " & str(t1) & "    t2 = " & str(t2)
			ez_draw_text(ev->win, EZ_MC, 240, 180, strptr( str1))
			t1= timer()
			ez_image_paint(ev->win , pgimg1 , 100 , 200)
			t1 =timer() - t1
			str1 = "image show t3 = " & str(t1)
			ez_draw_text(ev->win, EZ_MC, 200, 340, strptr( str1))
			ez_draw_text(ev->win, EZ_MC, 200, 380, strptr( str2))
			ez_image_set_opacity(pgimg1, 100)
			ez_image_paint(ev->win , pgimg1 , 300 , 200)
            exit sub

        case KeyPress
            'print hex(ev->key_sym)                                     '/* A key was pressed */
            select case (ev->key_sym)
                case XK_Q, XK_Q_
                    ez_quit()
                    exit sub
                case XK_E, XK_E_
                    ez_quit()
                    exit sub
                case XK_Escape
                    ez_quit()
                    exit sub
				case XK_S, XK_s_
					create_bmp(ev->win, "verif.bmp")
					create_jpg(ev->win, "verif.jpg")
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
	dim t1 as double = timer()
	pgimg1 = ez_image_create(100 , 100)

	for x as long = 0 to 99
		for y as long = 0 to 99
			ez_set_pixel_image(pgimg1,  x , y, ez_compose_rgba_color(x + 105, y + 105, x + y + 55,  x * y /4 + 57) )
		next
    NEXT
	ez_image_set_alpha(pgimg1, 1)
	ez_image_set_opacity(pgimg1, 180)
	t1 = timer() - t1

	str2 = "img create t = " & str(t1)

    ez_window_createex (200, 150, 480, 400, "Demo 02: Window, draw, image and events", procptr(win1_on_event()))

    ez_main_loop ()
SubExit(0)
