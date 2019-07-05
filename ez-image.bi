#pragma once

#ifndef EZ_IMAGE__H

    #ifndef EZ_DRAW__H
        #include once "ez-draw.bi"
    #endif
    #ifdef EZ_DRAW__H
        #define EZ_IMAGE__H

        #ifdef __FB_WIN32__
            #ifdef __FB_64BIT__
                #inclib "ez-image_w64"
            #else
                #inclib "ez-image_w32"
            #endif
        #else
            #ifdef __FB_LINUX__
                #ifdef __FB_64BIT__
                    #inclib "ez-image_l64"
                #else
                    #inclib "ez-image_l32"
                #endif
            #else
                #error ==> wrong Os, works only on Windows or Linux
            #endif
        #endif

        #include once "crt/math.bi"
        'const M_PI = 3.14159265358979323846

        type Ez_image
                width              as long
                height             as long
                pixels_rgba        as Ez_uint8 ptr
                has_alpha          as long
                opacity            as long
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

            enum
                EZ_STBI_DEFAULT = 0
                EZ_STBI_GREY = 1
                EZ_STBI_GREY_ALPHA = 2
                EZ_STBI_RGB = 3
                EZ_STBI_RGB_ALPHA = 4
            end enum

            declare function ez_stbi_load_from_memory(byval buffer as const Ez_uint8 ptr , byval len as long , byval x as long ptr , byval y as long ptr , byval comp as long ptr , byval req_comp as long) as Ez_uint8 ptr
            declare function ez_stbi_load(byval filename as const zstring ptr , byval x as long ptr , byval y as long ptr , byval comp as long ptr , byval req_comp as long) as Ez_uint8 ptr
            declare function ez_stbi_load_from_file(byval f as FILE ptr , byval x as long ptr , byval y as long ptr , byval comp as long ptr , byval req_comp as long) as Ez_uint8 ptr

            type Ez_stbi_io_callbacks
                    read as function(byval user as any ptr , byval data as zstring ptr , byval size as long) as long
                    skip as sub(byval user as any ptr , byval n as ulong)
                    eof as function(byval user as any ptr) as long
            end type

            declare function ez_stbi_load_from_callbacks(byval clbk as const Ez_stbi_io_callbacks ptr , byval user as any ptr , byval x as long ptr , byval y as long ptr , byval comp as long ptr , byval req_comp as long) as Ez_uint8 ptr
            declare sub ez_stbi_image_free(byval retval_from_Ez_stbi_load as any ptr)
            declare function ez_stbi_info_from_memory(byval buffer as const Ez_uint8 ptr , byval len as long , byval x as long ptr , byval y as long ptr , byval comp as long ptr) as long
            declare function ez_stbi_info_from_callbacks(byval clbk as const Ez_stbi_io_callbacks ptr , byval user as any ptr , byval x as long ptr , byval y as long ptr , byval comp as long ptr) as long
            declare function ez_stbi_info(byval filename as const zstring ptr , byval x as long ptr , byval y as long ptr , byval comp as long ptr) as long
            declare function ez_stbi_info_from_file(byval f as FILE ptr , byval x as long ptr , byval y as long ptr , byval comp as long ptr) as long
            declare function ez_stbi_zlib_decode_malloc_guesssize(byval buffer as const zstring ptr , byval len as long , byval initial_size as long , byval outlen as long ptr) as zstring ptr
            declare function ez_stbi_zlib_decode_malloc(byval buffer as const zstring ptr , byval len as long , byval outlen as long ptr) as zstring ptr
            declare function ez_stbi_zlib_decode_buffer(byval obuffer as zstring ptr , byval olen as long , byval ibuffer as const zstring ptr , byval ilen as long) as long
            declare function ez_stbi_zlib_decode_noheader_malloc(byval buffer as const zstring ptr , byval len as long , byval outlen as long ptr) as zstring ptr
            declare function ez_stbi_zlib_decode_noheader_buffer(byval obuffer as zstring ptr , byval olen as long , byval ibuffer as const zstring ptr , byval ilen as long) as long

        end extern

    #ENDIF
#ENDIF


