/*
 * ez-image.h: EZ-Draw module to load and draw images
 *
 * Edouard.Thiel@lif.univ-mrs.fr - 12/05/2013 - version 1.2
 * Benoit.Favre@lif.univ-mrs.fr
 *
 * This program is free software under the terms of the
 * GNU Lesser General Public License (LGPL) version 2.1.
*/

#ifndef EZ_IMAGE__H
#define EZ_IMAGE__H

#include "ez-draw.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


typedef struct {
    int width, height;
    Ez_uint8 *pixels_rgba;
    int has_alpha;
    int opacity;
} Ez_image;

typedef struct {
    int width, height;
#ifdef EZ_BASE_XLIB
    Pixmap map, mask;
#elif defined EZ_BASE_WIN32
    HBITMAP hmap;
    int has_alpha;
#endif /* EZ_BASE_ */
} Ez_pixmap;


/* Public functions */

Ez_image *ez_image_new (void);
void ez_image_destroy (Ez_image *img);
Ez_image *ez_image_create (int w, int h);
Ez_image *ez_image_dup (Ez_image *img);
Ez_image *ez_image_load (const char *filename);

void ez_image_set_alpha (Ez_image *img, int has_alpha);
int  ez_image_has_alpha (Ez_image *img);
void ez_image_set_opacity (Ez_image *img, int opacity);
int  ez_image_get_opacity (Ez_image *img);

void ez_image_paint (Ez_window win, Ez_image *img, int x, int y);
void ez_image_paint_sub (Ez_window win, Ez_image *img, int x, int y,
    int src_x, int src_y, int w, int h);
void ez_image_print (Ez_image *img, int src_x, int src_y, int w, int h);

void ez_image_fill_rgba (Ez_image *img, Ez_uint8 r, Ez_uint8 g, Ez_uint8 b, Ez_uint8 a);
void ez_image_blend (Ez_image *dst, Ez_image *src, int dst_x, int dst_y);
void ez_image_blend_sub (Ez_image *dst, Ez_image *src, int dst_x, int dst_y,
    int src_x, int src_y, int w, int h);

Ez_image *ez_image_extract (Ez_image *img, int src_x, int src_y, int w, int h);
Ez_image *ez_image_sym_ver (Ez_image *img);
Ez_image *ez_image_sym_hor (Ez_image *img);
Ez_image *ez_image_scale (Ez_image *img, double factor);
Ez_image *ez_image_rotate (Ez_image *img, double theta, int quality);
void ez_image_rotate_point (Ez_image *img, double theta, int src_x, int src_y,
    int *dst_x, int *dst_y);

Ez_pixmap *ez_pixmap_new (void);
void ez_pixmap_destroy (Ez_pixmap *pix);
Ez_pixmap *ez_pixmap_create_from_image (Ez_image *img);
void ez_pixmap_paint (Ez_window win, Ez_pixmap *pix, int x, int y);
void ez_pixmap_tile (Ez_window win, Ez_pixmap *pix, int x, int y, int w, int h);


/* Private functions */
#ifdef EZ_PRIVATE_DEFS

int ez_image_debug (void);

int ez_image_confine_sub_coords (Ez_image *img, int *src_x, int *src_y,
    int *w, int *h);
int ez_confine_coord (int *t, int *r, int tmax);

#ifdef EZ_BASE_XLIB

void ez_image_draw_pict (Ez_window win, Ez_image *img, int x, int y,
    int src_x, int src_y, int w, int h);

typedef void (*ez_xi_func)(XImage *, Ez_image *, int, int, int, int);

void ez_image_draw_xi (Ez_window win, Ez_image *img, int x, int y,
    int src_x, int src_y, int w, int h);
XImage *ez_xi_create (Ez_image *img, int src_x, int src_y, int w, int h,
    ez_xi_func xi_func);
ez_xi_func ez_xi_get_func (void);
void ez_xi_fill_default (XImage *xi, Ez_image *img, int src_x, int src_y,
    int w, int h);
void ez_xi_fill_24 (XImage *xi, Ez_image *img, int src_x, int src_y,
    int w, int h);
Ez_image *ez_xi_test_create (void);
int ez_xi_diff (XImage *xi1, XImage *xi2);

Pixmap ez_xmask_create (Ez_window win, Ez_image *img, int src_x, int src_y,
    int w, int h);
void ez_xmask_fill (Ez_uint8 *data, Ez_image *img,
    int src_x, int src_y, int w, int h);

#elif defined EZ_BASE_WIN32

void ez_image_draw_dib (HDC hdc_dst, Ez_image *img, int x, int y,
    int src_x, int src_y, int w, int h);
void ez_dib_fill_noalpha (Ez_uint8 *data, Ez_image *img, int src_x, int src_y,
    int w, int h);
void ez_dib_fill_opacity (Ez_uint8 *data, Ez_image *img, int src_x, int src_y,
    int w, int h);
void ez_dib_fill_truealpha (Ez_uint8 *data, Ez_image *img, int src_x, int src_y,
    int w, int h);

#endif /* EZ_BASE_ */

void ez_image_print_rgba (Ez_image *img, int src_x, int src_y, int w, int h);
void ez_image_comp_fill_rgba (Ez_image *img, Ez_uint8 r, Ez_uint8 g, Ez_uint8 b,
    Ez_uint8 a);
void ez_image_comp_over (Ez_image *dst, Ez_image *src, int dst_x, int dst_y,
    int src_x, int src_y, int w, int h);
void ez_image_comp_blend (Ez_image *dst, Ez_image *src, int dst_x, int dst_y,
    int src_x, int src_y, int w, int h);

void ez_image_copy_sub (Ez_image *src, Ez_image *dest , int src_x, int src_y);
void ez_image_comp_symv (Ez_image *src, Ez_image *dst);
void ez_image_comp_symh (Ez_image *src, Ez_image *dst);
void ez_image_expand (Ez_image *src, Ez_image *dst, double factor);
void ez_image_shrink (Ez_image *src, Ez_image *dst, double factor);
void ez_rotate_get_size (double theta, int src_w, int src_h, int *dst_w, int *dst_h);
void ez_rotate_get_coords (double theta, int src_w, int src_h, int src_x, int src_y,
    int *dst_x, int *dst_y);
void ez_image_rotate_nearest (Ez_image *src, Ez_image *dst, double theta);
void ez_image_rotate_bilinear (Ez_image *src, Ez_image *dst, double theta);
void ez_bilinear_4points (Ez_uint8 *src_p, Ez_uint8 *dst_p,
    int src_w, int src_h, double sx, double sy, int t);
void ez_bilinear_pane (Ez_uint8 *src_p, Ez_uint8 *dst_p,
    int src_w, int src_h, double sx, double sy, int t, double factor);

#ifdef EZ_BASE_XLIB
int ez_pixmap_build_map (Ez_pixmap *pix, Ez_image *img);
void ez_pixmap_draw_area (Ez_window win, Ez_pixmap *pix, int x, int y);
void ez_pixmap_tile_area (Ez_window win, Ez_pixmap *pix, int x, int y, int w, int h);
#elif defined EZ_BASE_WIN32
int ez_pixmap_build_hmap (Ez_pixmap *pix, Ez_image *img);
void ez_pixmap_draw_hmap (HDC hdc_dst, Ez_pixmap *pix, int x, int y);
void ez_pixmap_tile_hmap (HDC hdc_dst, Ez_pixmap *pix, int x, int y, int w, int h);
#endif /* EZ_BASE_ */

#endif /* EZ_PRIVATE_DEFS */


/*---------------------------------------------------------------------------
 *
 * Image files loading.
 *
 * Code extracted from stb_image.c version 1.33 - public domain JPEG/PNG reader
 * Original source and authors:  http://nothings.org/stb_image.c
 *
 * Supported formats:
 *    JPEG baseline (no JPEG progressive)
 *    PNG 8-bit-per-channel only
 *    BMP non-1bpp, non-RLE
 *    GIF (*comp always reports as 4-channel)
 *
 * Main contributors (see original sources):
 *    Sean Barrett (jpeg, png, bmp)
 *    Jean-Marc Lienher (gif)
*/

/* To test with the original file stb_image.c , define USE_ORIGINAL_STBI */
#ifndef USE_ORIGINAL_STBI

enum {
    EZ_STBI_DEFAULT    = 0, /* only used for req_comp */
    EZ_STBI_GREY       = 1,
    EZ_STBI_GREY_ALPHA = 2,
    EZ_STBI_RGB        = 3,
    EZ_STBI_RGB_ALPHA  = 4
};

/* Load image by filename, open file, or memory buffer */
Ez_uint8 *ez_stbi_load_from_memory (Ez_uint8 const *buffer, int len, int *x, int *y, int *comp, int req_comp);
Ez_uint8 *ez_stbi_load (char const *filename, int *x, int *y, int *comp, int req_comp);
Ez_uint8 *ez_stbi_load_from_file (FILE *f, int *x, int *y, int *comp, int req_comp);
/* for ez_stbi_load_from_file, file pointer is left pointing immediately after image */

typedef struct {
    /* Fill 'data' with 'size' bytes. Return number of bytes actually read */
    int (*read) (void *user, char *data, int size);
    /* Skip the next 'n' bytes */
    void (*skip) (void *user, unsigned n);
    /* Return nonzero if we are at end of file/data */
    int (*eof) (void *user);
} Ez_stbi_io_callbacks;

Ez_uint8 *ez_stbi_load_from_callbacks (Ez_stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int req_comp);

/* Free the loaded image -- this is just free () */
void ez_stbi_image_free (void *retval_from_Ez_stbi_load);

/* Get image dimensions and components without fully decoding */
int ez_stbi_info_from_memory (Ez_uint8 const *buffer, int len, int *x, int *y, int *comp);
int ez_stbi_info_from_callbacks (Ez_stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp);
int ez_stbi_info (char const *filename, int *x, int *y, int *comp);
int ez_stbi_info_from_file (FILE *f, int *x, int *y, int *comp);

/* Zlib client - used by PNG, available for other purposes */
char *ez_stbi_zlib_decode_malloc_guesssize (const char *buffer, int len, int initial_size, int *outlen);
char *ez_stbi_zlib_decode_malloc (const char *buffer, int len, int *outlen);
int   ez_stbi_zlib_decode_buffer (char *obuffer, int olen, const char *ibuffer, int ilen);
char *ez_stbi_zlib_decode_noheader_malloc (const char *buffer, int len, int *outlen);
int   ez_stbi_zlib_decode_noheader_buffer (char *obuffer, int olen, const char *ibuffer, int ilen);

#else /* USE_ORIGINAL_STBI */

#define STBI_HEADER_FILE_ONLY
#include "stb_image.c"
#undef STBI_HEADER_FILE_ONLY

#define EZ_STBI_DEFAULT     STBI_default
#define EZ_STBI_GREY        STBI_grey
#define EZ_STBI_GREY_ALPHA  STBI_grey_alpha
#define EZ_STBI_RGB         STBI_rgb
#define EZ_STBI_RGB_ALPHA   STBI_rgb_alpha

#define ez_stbi_load_from_memory    stbi_load_from_memory
#define ez_stbi_load                stbi_load
#define ez_stbi_load_from_file      stbi_load_from_file

#define Ez_stbi_io_callbacks        stbi_io_callbacks
#define ez_stbi_load_from_callbacks stbi_load_from_callbacks

#define ez_stbi_image_free          stbi_image_free

#define ez_stbi_info_from_memory    stbi_info_from_memory
#define ez_stbi_info_from_callbacks stbi_info_from_callbacks
#define ez_stbi_info                stbi_info
#define ez_stbi_info_from_file      stbi_info_from_file

#define ez_stbi_zlib_decode_malloc_guesssize    stbi_zlib_decode_malloc_guesssize
#define ez_stbi_zlib_decode_malloc              stbi_zlib_decode_malloc
#define ez_stbi_zlib_decode_buffer              stbi_zlib_decode_buffer
#define ez_stbi_zlib_decode_noheader_malloc     stbi_zlib_decode_noheader_malloc
#define ez_stbi_zlib_decode_noheader_buffer     stbi_zlib_decode_noheader_buffer

#endif /* USE_ORIGINAL_STBI */

#endif /* EZ_IMAGE__H */

