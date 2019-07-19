#pragma once

#ifndef EZ_IMAGE__H

    #ifndef EZ_DRAW__H
        #include once "ez-draw2.bi"
    #endif
    #ifdef EZ_DRAW__H
        #define EZ_IMAGE__H

        #ifdef __FB_WIN32__
            #ifdef __FB_64BIT__
                #inclib "ez-image2_w64"
				#inclib "ez-plus2_w64"
            #else
                #inclib "ez-image2_w32"
				#inclib "ez-plus2_w32"
            #endif
        #else
            #ifdef __FB_LINUX__
                #ifdef __FB_64BIT__
                    #inclib "ez-image2_l64"
					#inclib "ez-plus2_l64"
                #else
                    #inclib "ez-image2_l32"
					#inclib "ez-plus2_l32"
                #endif
            #else
                #error ==> Wrong Os, works only on Windows or Linux
            #endif
        #endif

        #include once "crt/math.bi"

        type Ez_image
                width              as long
                height             as long
                pixels_rgba        as Ez_uint8 ptr
                has_alpha          as long
                opacity            as long
        end type

		type Ez_rgb
                width              as long
                height             as long
                pixels_rgb         as Ez_uint8 ptr
        end type

        type Ez_pixmap
                width              as long
                height             as long

                #ifdef __FB_WIN32__
                    hmap           as HBITMAP
                    has_alpha      as long
                #else
                    map            as Pixmap
                    mask           as Pixmap
                #endif
        end type

        extern "C"

            declare function ez_image_new() as Ez_image ptr
            declare sub ez_image_destroy(byval img as Ez_image ptr)
            declare function ez_image_create(byval w as long , byval h as long) as Ez_image ptr
            declare function ez_image_dup(byval img as Ez_image ptr) as Ez_image ptr
            declare function ez_image_load(byval filename as const zstring ptr) as Ez_image ptr
            declare sub ez_image_set_alpha(byval img as Ez_image ptr , byval has_alpha as long)
            declare function ez_image_has_alpha(byval img as Ez_image ptr) as long
            declare sub ez_image_set_opacity(byval img as Ez_image ptr , byval opacity as long)
            declare function ez_image_get_opacity(byval img as Ez_image ptr) as long
            declare sub ez_image_paint(byval win as Ez_window , byval img as Ez_image ptr , byval x as long , byval y as long)
            declare sub ez_image_paint_sub(byval win as Ez_window , byval img as Ez_image ptr , byval x as long , byval y as long , byval src_x as long , byval src_y as long , byval w as long , byval h as long)
            declare sub ez_image_print(byval img as Ez_image ptr , byval src_x as long , byval src_y as long , byval w as long , byval h as long)
            declare sub ez_image_fill_rgba(byval img as Ez_image ptr , byval r as Ez_uint8 , byval g as Ez_uint8 , byval b as Ez_uint8 , byval a as Ez_uint8)
            declare sub ez_image_blend(byval dst as Ez_image ptr , byval src as Ez_image ptr , byval dst_x as long , byval dst_y as long)
            declare sub ez_image_blend_sub(byval dst as Ez_image ptr , byval src as Ez_image ptr , byval dst_x as long , byval dst_y as long , byval src_x as long , byval src_y as long , byval w as long , byval h as long)
            declare function ez_image_extract(byval img as Ez_image ptr , byval src_x as long , byval src_y as long , byval w as long , byval h as long) as Ez_image ptr
            declare function ez_image_sym_ver(byval img as Ez_image ptr) as Ez_image ptr
            declare function ez_image_sym_hor(byval img as Ez_image ptr) as Ez_image ptr
            declare function ez_image_scale(byval img as Ez_image ptr , byval factor as double) as Ez_image ptr
            declare function ez_image_rotate(byval img as Ez_image ptr , byval theta as double , byval quality as long) as Ez_image ptr
            declare sub ez_image_rotate_point(byval img as Ez_image ptr , byval theta as double , byval src_x as long , byval src_y as long , byval dst_x as long ptr , byval dst_y as long ptr)
            declare function ez_pixmap_new() as Ez_pixmap ptr
            declare sub ez_pixmap_destroy(byval pix as Ez_pixmap ptr)
            declare function ez_pixmap_create_from_image(byval img as Ez_image ptr) as Ez_pixmap ptr
            declare sub ez_pixmap_paint(byval win as Ez_window , byval pix as Ez_pixmap ptr , byval x as long , byval y as long)
            declare sub ez_pixmap_tile(byval win as Ez_window , byval pix as Ez_pixmap ptr , byval x as long , byval y as long , byval w as long , byval h as long)


			declare function savebmp(byval fname as zstring ptr, byval rgb1 as Ez_uint8 ptr , byval width1 as long, byval height1 as long)as long
			declare function savejpeg(byval fname as zstring ptr, byval rgb1 as Ez_uint8 ptr , byval width1 as long, byval height1 as long)as long

			declare function ez_win_to_rgb(byval my_win as Ez_window )as Ez_rgb ptr
			declare function ez_win_to_image(byval my_win as Ez_window )as Ez_image ptr
			declare function ez_image_to_rgb(byval my_img as Ez_image ptr)as Ez_rgb ptr
			declare function ez_get_rgb_data_free(byval rgb1 as Ez_rgb ptr, byval w as long ptr, byval h as long ptr)as Ez_uint8 ptr
			declare function ez_get_rgb_data(byval rgb1 as Ez_rgb ptr, byval w as long ptr, byval h as long ptr)as Ez_uint8 ptr
			declare function ez_set_pixel_image(byval my_img as Ez_image ptr, byval x as long , byval y as long, byval color1 as Ez_uint32)as long
			declare function ez_set_pixel_rgb(byval my_rgb as Ez_rgb ptr, byval x as long , byval y as long, byval color1 as Ez_uint32)as long
			declare function ez_compose_rgba_color(byval r as ubyte, byval g as ubyte, byval b as ubyte, byval a as ubyte)as Ez_uint32
        end extern

    #ENDIF
#ENDIF


