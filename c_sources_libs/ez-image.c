/*
 * ez-image.c: EZ-Draw module to load and draw images
 *
 * Edouard.Thiel@lif.univ-mrs.fr - 12/05/2013 - version 1.2
 * Benoit.Favre@lif.univ-mrs.fr
 *
 * This program is free software under the terms of the
 * GNU Lesser General Public License (LGPL) version 2.1.
*/

/* To debug, define environment variable EZ_IMAGE_DEBUG */

#define EZ_PRIVATE_DEFS 1
#include "ez-image.h"

/* Contains internal parameters of ez-draw.c */
extern Ez_X ezx;

/* Counters for debugging */
int ez_image_count = 0, ez_pixmap_count = 0;


/*---------------------- P U B L I C   I N T E R F A C E --------------------*/

/*
 * Allocate an image, initialized with default values.
 * Return the image, else NULL.
*/

Ez_image *ez_image_new (void)
{
    Ez_image *img = malloc (sizeof(Ez_image));
    if (img == NULL) {
        ez_error ("ez_image_new: out of memory\n");
        return NULL;
    }
    ez_image_count++;

    img->width = img->height = 0;
    img->pixels_rgba = NULL;
    img->has_alpha = 0;
    img->opacity = 128;

    return img;
}


/*
 * Destroy image and free memory.
*/

void ez_image_destroy (Ez_image *img)
{
    if (img == NULL) return;
    if (img->pixels_rgba != NULL) free (img->pixels_rgba);
    free (img);

    ez_image_count--;
    if (ez_image_debug ())
        printf ("ez_image_destroy  count = %d\n", ez_image_count);
}


/*
 * Create an image having width w and height h.
 * Return the image, else NULL.
*/

Ez_image *ez_image_create (int w, int h)
{
    Ez_image *img;

    if (w < 0 || h < 0) {
        ez_error ("ez_image_create: bad size\n");
        return NULL;
    }

    img = ez_image_new ();
    if (img == NULL) return NULL;

    img->pixels_rgba = calloc (w*h*4, 1);
    if (img->pixels_rgba == NULL) {
        ez_error ("ez_image_create: out of memory\n");
        return NULL;
    }
    img->width = w; img->height = h;

    return img;
}


/*
 * Create a deep copy of image img.
 * Return the new image, else NULL.
*/

Ez_image *ez_image_dup (Ez_image *img)
{
    Ez_image *res;

    if (img == NULL) return NULL;
    res = ez_image_create (img->width, img->height);
    if (res == NULL) return NULL;
    memcpy (res->pixels_rgba, img->pixels_rgba, img->width * img->height * 4);
    res->has_alpha = img->has_alpha;
    res->opacity   = img->opacity;
    return res;
}


/*
 * Load an image from a file filename.
 * Return the image, else NULL.
 */

Ez_image *ez_image_load (const char *filename)
{
    Ez_image *img;
    int nbytes;
    double time1 = 0, time2 = 0;

    img = ez_image_new ();
    if (img == NULL) return NULL;

    if (ez_image_debug()) time1 = ez_get_time ();

    /* Loading with stbi */
    img->pixels_rgba = ez_stbi_load (filename, &img->width, &img->height,
        &nbytes, EZ_STBI_RGB_ALPHA);
    if (img->pixels_rgba == NULL) {
        ez_error ("ez_load_image: can't load file \"%s\"\n", filename);
        ez_image_destroy (img);
        return NULL;
    }
    /* An alpha channel is present in the file? */
    img->has_alpha = nbytes == 4;

    if (ez_image_debug()) {
        time2 = ez_get_time ();
        printf ("ez_image_load  file \"%s\"  in %.3f ms  w = %d  h = %d  "
                "n = %d  has_alpha = %d\n",
                filename, (time2-time1)*1000, img->width, img->height,
                nbytes, img->has_alpha);
    }

    return img;
}


/*
 * Properties has_alpha and opacity
*/

void ez_image_set_alpha (Ez_image *img, int has_alpha)
{
    if (img == NULL) return;
    img->has_alpha = has_alpha ? 1 : 0;
}


int ez_image_has_alpha (Ez_image *img)
{
    if (img == NULL) return 0;
    return img->has_alpha ? 1 : 0;
}


void ez_image_set_opacity (Ez_image *img, int opacity)
{
    if (img == NULL) return;
    img->opacity = opacity;
}


int ez_image_get_opacity (Ez_image *img)
{
    if (img == NULL) return 0;
    return img->opacity;
}


/*
 * Display an image or a rectangular region of the image img in the
 * window win, with the upper left corner of the image at the x,y
 * coordinates in the window.
 *
 * If img->has_alpha is true, apply transparency.
*/

void ez_image_paint (Ez_window win, Ez_image *img, int x, int y)
{
    if (img == NULL) return;
    ez_image_paint_sub (win, img, x, y, 0, 0, img->width, img->height);
}


void ez_image_paint_sub (Ez_window win, Ez_image *img, int x, int y,
    int src_x, int src_y, int w, int h)
{
    int src_x_old = src_x, src_y_old = src_y;

    if (win == None) return;

    if (ez_image_confine_sub_coords (img, &src_x, &src_y, &w, &h) < 0) return;
    x += src_x - src_x_old;
    y += src_y - src_y_old;

#ifdef EZ_BASE_XLIB
    if (win == ezx.dbuf_win) win = ezx.dbuf_pix;
    ez_image_draw_xi (win, img, x, y, src_x, src_y, w, h);
#elif defined EZ_BASE_WIN32
    ez_cur_win (win);
    ez_image_draw_dib (ezx.hdc, img, x, y, src_x, src_y, w, h);
#endif /* EZ_BASE_ */
}


/*
 * Print a rectangular region of the image img in the terminal.
*/

void ez_image_print (Ez_image *img, int src_x, int src_y, int w, int h)
{
    if (ez_image_confine_sub_coords (img, &src_x, &src_y, &w, &h) < 0) return;

    printf ("Image: width %d  height %d  has_alpha %s  opacity %d\n",
        img->width, img->height, img->has_alpha?"true":"false", img->opacity);
    printf ("Region: src_x %d  src_y %d  width %d  height %d\n",
        src_x, src_y, w, h);

    ez_image_print_rgba (img, src_x, src_y, w, h);
}


/*
 * Fill an image with RGBA color
*/

void ez_image_fill_rgba (Ez_image *img, Ez_uint8 r, Ez_uint8 g, Ez_uint8 b, Ez_uint8 a)
{
    if (img == NULL) return;

    ez_image_comp_fill_rgba (img, r, g, b, a);
}


/*
 * Superimpose a region of image src into the image dst. If src image has an
 * alpha channel, apply transparency.
 * If the coordinates go beyond the images src or dst, just the common region
 * is superimposed.
*/

void ez_image_blend (Ez_image *dst, Ez_image *src, int dst_x, int dst_y)
{
    if (src == NULL) return;
    ez_image_blend_sub (dst, src, dst_x, dst_y, 0, 0, src->width, src->height);
}


void ez_image_blend_sub (Ez_image *dst, Ez_image *src, int dst_x, int dst_y,
    int src_x, int src_y, int w, int h)
{
    int src_x_old = src_x, src_y_old = src_y, dst_x_old, dst_y_old;

    if (ez_image_confine_sub_coords (src, &src_x, &src_y, &w, &h) < 0) return;
    dst_x += src_x - src_x_old; dst_x_old = dst_x;
    dst_y += src_y - src_y_old; dst_y_old = dst_y;
    if (ez_image_confine_sub_coords (dst, &dst_x, &dst_y, &w, &h) < 0) return;
    src_x += dst_x - dst_x_old;
    src_y += dst_y - dst_y_old;

    if (src->has_alpha)
         ez_image_comp_blend (dst, src, dst_x, dst_y, src_x, src_y, w, h);
    else ez_image_comp_over  (dst, src, dst_x, dst_y, src_x, src_y, w, h);
}


/*
 * Extract a rectangular region of an image.
 * Return new image, else NULL.
*/

Ez_image *ez_image_extract (Ez_image *img, int src_x, int src_y, int w, int h)
{
    Ez_image *res;

    if (ez_image_confine_sub_coords (img, &src_x, &src_y, &w, &h) < 0)
        return NULL;

    res = ez_image_create (w, h);
    if (res == NULL) return NULL;
    res->has_alpha = img->has_alpha;
    res->opacity   = img->opacity;

    ez_image_copy_sub (img, res, src_x, src_y);

    return res;
}


/*
 * Vertical or horizontal symmetry.
 * Return new image, else NULL.
*/

Ez_image *ez_image_sym_ver (Ez_image *img)
{
    Ez_image *res;

    if (img == NULL) return NULL;

    res = ez_image_create (img->width, img->height);
    if (res == NULL) return NULL;
    res->has_alpha = img->has_alpha;
    res->opacity   = img->opacity;

    ez_image_comp_symv (img, res);
    return res;
}


Ez_image *ez_image_sym_hor (Ez_image *img)
{
    Ez_image *res;

    if (img == NULL) return NULL;

    res = ez_image_create (img->width, img->height);
    if (res == NULL) return NULL;
    res->has_alpha = img->has_alpha;
    res->opacity   = img->opacity;

    ez_image_comp_symh (img, res);
    return res;
}


/*
 * Scale an image.
 * Return new image, else NULL.
*/

Ez_image *ez_image_scale (Ez_image *img, double factor)
{
    Ez_image *res;

    if (img == NULL) return NULL;

    if (factor <= 0) {
        ez_error ("ez_image_scale: bad scale factor %f\n", factor);
        return NULL;
    }
    if (factor == 1)
        return ez_image_dup (img);

    res = ez_image_create (img->width*factor, img->height*factor);
    if (res == NULL) return NULL;
    res->has_alpha = 1;
    res->opacity   = img->opacity;

    if (factor > 1)
         ez_image_expand (img, res, factor);
    else ez_image_shrink (img, res, factor);
    return res;
}


/*
 * Rotate image img for angle theta in degrees.
 * Return a new image whose size is adjusted to contain the result, or NULL
 * on error. The outer parts of the original image become transparent.
*/

Ez_image *ez_image_rotate (Ez_image *img, double theta, int quality)
{
    int w, h;
    Ez_image *res;

    if (img == NULL) return NULL;

    ez_rotate_get_size (theta, img->width, img->height, &w, &h);

    res = ez_image_create (w, h);
    if (res == NULL) return NULL;
    res->has_alpha = 1;
    res->opacity   = img->opacity;

    if (quality)
         ez_image_rotate_bilinear (img, res, theta);
    else ez_image_rotate_nearest  (img, res, theta);
    return res;
}


/*
 * Compute for a point having coordinates src_x,src_y in the source image, the
 * corresponding coordinates dst_x,dst_y of the point in the destination image.
*/

void ez_image_rotate_point (Ez_image *img, double theta, int src_x, int src_y,
    int *dst_x, int *dst_y)
{
    if (img == NULL || dst_x == NULL || dst_y == NULL) return;

    ez_rotate_get_coords (theta, img->width, img->height, src_x, src_y,
        dst_x, dst_y);
}


/*
 * Allocate a pixmap, initialized to default value.
 * Return the pixmap, else NULL.
*/

Ez_pixmap *ez_pixmap_new (void)
{
    Ez_pixmap *pix = malloc (sizeof(Ez_pixmap));
    if (pix == NULL) {
        ez_error ("ez_pixmap_new: out of memory\n");
        return NULL;
    }
    ez_pixmap_count++;

    pix->width = pix->height = 0;
#ifdef EZ_BASE_XLIB
    pix->map = None;
    pix->mask = None;
#elif defined EZ_BASE_WIN32
    pix->hmap = NULL;
    pix->has_alpha = 0;
#endif /* EZ_BASE_ */

    return pix;
}


/*
 * Destroy the pixmap and free memory.
*/

void ez_pixmap_destroy (Ez_pixmap *pix)
{
    if (pix == NULL) return;

#ifdef EZ_BASE_XLIB
    if (pix->map  != None) XFreePixmap (ezx.display, pix->map );
    if (pix->mask != None) XFreePixmap (ezx.display, pix->mask);
#elif defined EZ_BASE_WIN32
    if (pix->hmap != NULL) DeleteObject (pix->hmap);
#endif /* EZ_BASE_ */
    free (pix);

    ez_pixmap_count--;
    if (ez_image_debug ())
        printf ("ez_pixmap_destroy  count = %d\n", ez_pixmap_count);
}


/*
 * Create a pixmap from an image img.
 * Return the pixmap, else NULL.
*/

Ez_pixmap *ez_pixmap_create_from_image (Ez_image *img)
{
    Ez_pixmap *pix;

    if (img == NULL) return NULL;
    pix = ez_pixmap_new ();
    if (pix == NULL) return NULL;

    pix->width  = img->width;
    pix->height = img->height;

#ifdef EZ_BASE_XLIB
    if (ez_pixmap_build_map (pix, img) < 0) {
        ez_error ("ez_pixmap_create_from_image: can't create map\n");
        goto free_pix;
    }
    if (img->has_alpha) {
        pix->mask = ez_xmask_create (ezx.root_win, img, 0, 0,
            img->width, img->height);
        if (pix->mask == None) {
            ez_error ("ez_pixmap_create_from_image: can't create mask\n");
            goto free_pix;
        }
    }
#elif defined EZ_BASE_WIN32
    if (ez_pixmap_build_hmap (pix, img) < 0) {
        ez_error ("ez_pixmap_create_from_image: can't create hmap\n");
        goto free_pix;
    }
    pix->has_alpha = img->has_alpha;

#endif /* EZ_BASE_ */

    return pix;

  free_pix:
    ez_pixmap_destroy (pix);
    return NULL;
}


/*
 * Display the pixmap pix in the window win.
 * The top left corner of the pixmap is displayed at the x,y coordinates
 * in the window.
*/

void ez_pixmap_paint (Ez_window win, Ez_pixmap *pix, int x, int y)
{
    if (win == None) return;

#ifdef EZ_BASE_XLIB
    if (win == ezx.dbuf_win) win = ezx.dbuf_pix;
    ez_pixmap_draw_area (win, pix, x, y);
#elif defined EZ_BASE_WIN32
    ez_cur_win (win);
    ez_pixmap_draw_hmap (ezx.hdc, pix, x, y);
#endif /* EZ_BASE_ */
}


/*
 * Display the pixmap pix repeatedly in the window win.
 * The pixmap is displayed as a wallpaper in the window region bounded
 * by coordinates x,y (top left corner) and x+w-1,y+h-1 (bottom right corner).
*/

void ez_pixmap_tile (Ez_window win, Ez_pixmap *pix, int x, int y, int w, int h)
{
    if (win == None || w <= 0 || h <= 0) return;

#ifdef EZ_BASE_XLIB
    if (win == ezx.dbuf_win) win = ezx.dbuf_pix;
    ez_pixmap_tile_area (win, pix, x, y, w, h);
#elif defined EZ_BASE_WIN32
    ez_cur_win (win);
    ez_pixmap_tile_hmap (ezx.hdc, pix, x, y, w, h);
#endif /* EZ_BASE_ */
}


/*-------------------- P R I V A T E   F U N C T I O N S --------------------*/

/*
 * Unique test of the definition of the environment variable EZ_IMAGE_DEBUG
*/

int ez_image_debug (void)
{
    static int debug = -1;
    if (debug < 0) debug = getenv ("EZ_IMAGE_DEBUG") != NULL;
    return debug;
}


/*
 * Confine coordinates of sub-image inside img
*/

int ez_image_confine_sub_coords (Ez_image *img, int *src_x, int *src_y,
    int *w, int *h)
{
    if (img == NULL) return -1;

    if (ez_confine_coord (src_x, w, img->width) < 0 ||
        ez_confine_coord (src_y, h, img->height) < 0) {
        if (ez_image_debug ())
            printf ("ez_image_confine_sub_coords empty region "
                    "src_x = %d  src_y = %d  w = %d  h = %d\n",
                *src_x, *src_y, *w, *h);
        return -1;
    }
    return 0;
}

int ez_confine_coord (int *t, int *r, int tmax)
{
    if (*t < 0) { *r += *t; *t = 0; }
    if (*t+*r > tmax) *r = tmax -*t;
    return *r > 0 ? 0 : -1;
}


#ifdef EZ_BASE_XLIB

/*
 * Display sub-image with transparency.
*/

void ez_image_draw_xi (Ez_window win, Ez_image *img, int x, int y,
    int src_x, int src_y, int w, int h)
{
    XImage *xi = NULL;
    Pixmap mask = None;
    ez_xi_func xi_func;

    xi_func = ez_xi_get_func ();
    xi = ez_xi_create (img, src_x, src_y, w, h, xi_func);
    if (xi == NULL) return;

    if (img->has_alpha) {
        mask = ez_xmask_create (win, img, src_x, src_y, w, h);
        if (mask == None) goto free_xi;
        XSetClipOrigin (ezx.display, ezx.gc, x, y);
        XSetClipMask (ezx.display, ezx.gc, mask);
    }

    XPutImage (ezx.display, win, ezx.gc, xi, 0, 0, x, y, w, h);

    if (img->has_alpha) {
        XSetClipOrigin (ezx.display, ezx.gc, 0, 0);
        XSetClipMask (ezx.display, ezx.gc, None);
        XFreePixmap (ezx.display, mask);
    }

  free_xi:
    XDestroyImage (xi);
}


/*
 * Creation of ximage
*/

XImage *ez_xi_create (Ez_image *img, int src_x, int src_y, int w, int h,
    ez_xi_func xi_func)
{
    XImage *xi = NULL;

    if (xi_func == NULL) {
        ez_error ("ez_xi_create: NULL xi_func\n");
        return NULL;
    }

    /* We must first create xi to obtain informations to create data */
    xi = XCreateImage (ezx.display, ezx.visual, ezx.depth, ZPixmap, 0,
        NULL, w, h, 32, 0);
    if (xi == NULL) {
        ez_error ("ez_xi_create: can't create XImage\n");
        return NULL;
    }

    if (ez_image_debug())
        printf ("ez_xi_create  w = %d  h = %d  depth = %d  bpp = %d\n",
            w, h, ezx.depth, xi->bits_per_pixel);

    xi->data = calloc (xi->bytes_per_line * h, 1);
    if (xi->data == NULL)  {
        ez_error ("ez_xi_create: out of memory\n");
        XDestroyImage (xi);
        return NULL;
    }
    /* xi->data will be freed by XDestroyImage */

    /* Draw pixels in xi->data */
    xi_func (xi, img, src_x, src_y, w, h);

    return xi;
}


ez_xi_func ez_xi_get_func (void)
{
    static ez_xi_func xi_func = NULL;

    if (xi_func != NULL) return xi_func;
    xi_func = ez_xi_fill_default;

    if (ezx.depth == 24)
    {
        Ez_image *img = ez_xi_test_create ();
        XImage *xi1 = ez_xi_create (img, 0, 0, img->width, img->height,
            ez_xi_fill_default);
        XImage *xi2 = ez_xi_create (img, 0, 0, img->width, img->height,
            ez_xi_fill_24);

        if (ez_xi_diff (xi1, xi2) == 0)
            xi_func = ez_xi_fill_24;

        XDestroyImage (xi1);
        XDestroyImage (xi2);
        ez_image_destroy (img);
    }

    return xi_func;
}


void ez_xi_fill_default (XImage *xi, Ez_image *img, int src_x, int src_y,
    int w, int h)
{
    int x, y, tx, ty, img_w = img->width;
    Ez_uint8 cr, cg, cb;
    double time1 = 0;

    if (ez_image_debug()) time1 = ez_get_time ();

    for (y = 0, ty = src_y * img_w; y < h; y++, ty += img_w)
    for (x = 0, tx = (ty + src_x)*4; x < w; x++, tx += 4) {
        cr = img->pixels_rgba[tx];
        cg = img->pixels_rgba[tx+1];
        cb = img->pixels_rgba[tx+2];
        XPutPixel (xi, x, y, ez_get_RGB (cr, cg, cb));
    }

    if (ez_image_debug())
        printf ("ez_xi_fill_default %.3f ms\n", (ez_get_time() - time1)*1000);
}


/* Speed gain compared to fill_default: 50% */

void ez_xi_fill_24 (XImage *xi, Ez_image *img, int src_x, int src_y,
    int w, int h)
{
    int x, y, tx, ty, i, j, img_w = img->width,
        dj = xi->bytes_per_line,
        di = xi->bits_per_pixel / 8,
        ir = ezx.trueColor.red.shift   / 8,
        ig = ezx.trueColor.green.shift / 8,
        ib = ezx.trueColor.blue.shift  / 8;
    Ez_uint8 *data = (Ez_uint8 *) xi->data;
    double time1 = 0;

    if (ez_image_debug()) time1 = ez_get_time ();

    for (y = 0, ty = src_y * img_w, j = 0; y < h; y++, ty += img_w, j += dj)
    for (x = 0, tx = (ty + src_x)*4, i = j; x < w; x++, tx += 4, i += di) {
        data[i+ir] = img->pixels_rgba[tx];
        data[i+ig] = img->pixels_rgba[tx+1];
        data[i+ib] = img->pixels_rgba[tx+2];
    }

    if (ez_image_debug())
        printf ("ez_xi_fill_24 %.3f ms\n", (ez_get_time() - time1)*1000);
}


Ez_image *ez_xi_test_create (void)
{
    int w = 9, h = 13, t, tmax = w*h*4;
    Ez_image *img = ez_image_create (w, h);
    if (img == NULL) return NULL;

    for (t = 0; t < tmax; t += 4) {
        img->pixels_rgba[t  ] = t*11/256;
        img->pixels_rgba[t+1] = t*31/256;
        img->pixels_rgba[t+2] = t* 7/256;
    }

    return img;
}


int ez_xi_diff (XImage *xi1, XImage *xi2)
{
    int ntot, i;
    if (xi1 == NULL || xi2 == NULL) return -1;

    ntot = xi1->bytes_per_line * xi1->height;
    for (i = 0; i < ntot; i++)
        if (xi1->data[i] != xi2->data[i]) return -1;

    return 0;
}


/*
 * Create a cutting mask from alpha channel and opacity threshold
*/

Pixmap ez_xmask_create (Ez_window win, Ez_image *img, int src_x, int src_y,
    int w, int h)
{
    Pixmap mask = None;
    Ez_uint8 *data = NULL;
    int bytes_per_line = (w+7)/8;
    double time1 = 0, time2 = 0, time3 = 0;

    data = calloc (bytes_per_line*h, 1);
    if (data == NULL) {
        ez_error ("ez_xmask_create: out of memory\n");
        return None;
    }

    if (ez_image_debug()) time1 = ez_get_time ();

    ez_xmask_fill (data, img, src_x, src_y, w, h);

    if (ez_image_debug()) time2 = ez_get_time ();

    mask = XCreateBitmapFromData (ezx.display, win, (char*) data, w, h);
    if (mask == None)
        ez_error ("ez_xmask_create: can't create bitmap");

    if (ez_image_debug()) {
        time3 = ez_get_time ();
        printf ("ez_xmask_create   fill %.3f ms   bitmap %.3f ms\n",
            (time2-time1)*1000, (time3-time2)*1000);
    }

    free (data);
    return mask;
}


void ez_xmask_fill (Ez_uint8 *data, Ez_image *img,
    int src_x, int src_y, int w, int h)
{

    int tx, mx, y, ty, i, j, bpl = (w+7)/8, img_w = img->width;

    for (y = 0, ty = src_y * img_w ; y < h ; y++, ty += img_w)
    for (tx = (ty+src_x)*4+3, mx = tx+w*4, i = y*bpl, j = 0 ; tx < mx ; tx += 4, j++) {
        if (j == 8) { i++; j = 0; }
        if (img->pixels_rgba[tx] < img->opacity) continue;
        data[i] |= 1 << j;
    }
}

#elif defined EZ_BASE_WIN32

/*
 * Display a sub-image with transparency.
 * DIB = Device Independent Bitmap
*/

void ez_image_draw_dib (HDC hdc_dst, Ez_image *img, int x, int y,
    int src_x, int src_y, int w, int h)
{
    HDC hdc = NULL;
    HBITMAP hbitmap = NULL;
    BITMAPINFO bmi;
    Ez_uint8 *data;

    /* Create a DC for the bitmap */
    hdc = CreateCompatibleDC(hdc_dst);
    if (hdc == NULL) {
        ez_error ("ez_image_draw_dib: can't create compatible DC\n");
        return;
    }

    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;  /* negative height for Origin at top left */
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = w * h * 4;

    hbitmap = CreateDIBSection (hdc_dst, &bmi, DIB_RGB_COLORS, (void **) &data, NULL, 0);
    if (hbitmap == NULL) {
        ez_error ("ez_image_draw_dib: can't create DIB Section\n");
        goto final;
    }
    SelectObject (hdc, hbitmap);

    if (!img->has_alpha)
         ez_dib_fill_noalpha   (data, img, src_x, src_y, w, h);
    else if (img->opacity >= 0)
         ez_dib_fill_opacity   (data, img, src_x, src_y, w, h);
    else ez_dib_fill_truealpha (data, img, src_x, src_y, w, h);

    if (img->has_alpha) {
        BLENDFUNCTION bf;

        bf.BlendOp = AC_SRC_OVER;
        bf.BlendFlags = 0;
        bf.AlphaFormat = AC_SRC_ALPHA;  /* alpha channel premultiplied */
        bf.SourceConstantAlpha = 0xff;

        if (AlphaBlend (hdc_dst, x, y, w, h, hdc, 0, 0, w, h, bf) == FALSE)
            ez_error ("ez_image_draw_dib: AlphaBlend failed\n");
    } else {
        if (BitBlt (hdc_dst, x, y, w, h, hdc, 0, 0, SRCCOPY ) == 0)
            ez_error ("ez_image_draw_dib: bitblt failed\n");
    }

  final:
    if (hbitmap != NULL) DeleteObject (hbitmap);
    if (hdc != NULL) DeleteDC (hdc);
}


void ez_dib_fill_noalpha (Ez_uint8 *data, Ez_image *img, int src_x, int src_y,
    int w, int h)
{
    int x, y, tx, ty, i, j, img_w = img->width, dj = w*4;
    double time1 = 0;

    if (ez_image_debug()) time1 = ez_get_time ();

    for (y = 0, ty = src_y * img_w,  j = 0; y < h; y++, ty += img_w, j += dj)
    for (x = 0, tx = (ty + src_x)*4, i = j; x < w; x++, tx += 4, i += 4) {
        data[i+3] = 0xff;
        data[i+2] = img->pixels_rgba[tx];
        data[i+1] = img->pixels_rgba[tx+1];
        data[i  ] = img->pixels_rgba[tx+2];
    }

    if (ez_image_debug())
        printf ("ez_dib_fill_noalpha %.3f ms\n", (ez_get_time() - time1)*1000);
}


void ez_dib_fill_opacity (Ez_uint8 *data, Ez_image *img, int src_x, int src_y,
    int w, int h)
{
    int x, y, tx, ty, i, j, img_w = img->width, dj = w*4;
    double time1 = 0;

    if (ez_image_debug()) time1 = ez_get_time ();

    for (y = 0, ty = src_y * img_w,  j = 0; y < h; y++, ty += img_w, j += dj)
    for (x = 0, tx = (ty + src_x)*4, i = j; x < w; x++, tx += 4, i += 4) {
        if (img->pixels_rgba[tx+3] >= img->opacity) {
            data[i+3] = 0xff;
            data[i+2] = img->pixels_rgba[tx];
            data[i+1] = img->pixels_rgba[tx+1];
            data[i  ] = img->pixels_rgba[tx+2];
        } else {
            data[i+3] = 0;
            data[i+2] = 0;
            data[i+1] = 0;
            data[i  ] = 0;
        }
    }

    if (ez_image_debug())
        printf ("ez_dib_fill_opacity %.3f ms\n", (ez_get_time() - time1)*1000);
}


void ez_dib_fill_truealpha (Ez_uint8 *data, Ez_image *img, int src_x, int src_y,
    int w, int h)
{
    int x, y, tx, ty, i, j, img_w = img->width, dj = w*4, a;
    double time1 = 0;

    if (ez_image_debug()) time1 = ez_get_time ();

    for (y = 0, ty = src_y * img_w,  j = 0; y < h; y++, ty += img_w, j += dj)
    for (x = 0, tx = (ty + src_x)*4, i = j; x < w; x++, tx += 4, i += 4) {
        data[i+3] = a = img->pixels_rgba[tx+3];
        data[i+2] = img->pixels_rgba[tx  ]*a/255;
        data[i+1] = img->pixels_rgba[tx+1]*a/255;
        data[i  ] = img->pixels_rgba[tx+2]*a/255;
    }

    if (ez_image_debug())
        printf ("ez_dib_fill_truealpha %.3f ms\n", (ez_get_time() - time1)*1000);
}

#endif /* EZ_BASE_ */


/*
 * Print a rectangular region of the image img in the terminal.
*/

void ez_image_print_rgba (Ez_image *img, int src_x, int src_y, int w, int h)
{
    int x, y;
    Ez_uint8 *p;

    printf (" y,x ");
    for (x = src_x; x < src_x+w; x++)
        printf ("|      %4d      ", x);
    printf ("\n");

    for (y = src_y; y < src_y+h; y++) {
        printf ("%4d ", y);
        for (x = src_x; x < src_x+w; x++) {
            p = img->pixels_rgba + (y*img->width+x)*4;
            printf ("| %3d %3d %3d %3d", p[0], p[1], p[2], p[3]);
        }
        printf ("\n");
    }
}


/*
 * Fill image with a RGBA color
*/

void ez_image_comp_fill_rgba (Ez_image *img, Ez_uint8 r, Ez_uint8 g, Ez_uint8 b,
    Ez_uint8 a)
{
    int t, tmax4 = img->width * img->height * 4;

    for (t = 0; t < tmax4; t += 4) {
        img->pixels_rgba[t  ] = r;
        img->pixels_rgba[t+1] = g;
        img->pixels_rgba[t+2] = b;
        img->pixels_rgba[t+3] = a;
    }
}


/*
 * Superimpose src into dst, without transparency
*/

void ez_image_comp_over (Ez_image *dst, Ez_image *src, int dst_x, int dst_y,
    int src_x, int src_y, int w, int h)
{
    int x, y, t_src, t_dst;

    for (y = 0; y < h; y++)
    for (x = 0; x < w; x++) {
        t_src = ((y+src_y)*src->width+x+src_x)*4;
        t_dst = ((y+dst_y)*dst->width+x+dst_x)*4;

        dst->pixels_rgba[t_dst  ] = src->pixels_rgba[t_src  ];
        dst->pixels_rgba[t_dst+1] = src->pixels_rgba[t_src+1];
        dst->pixels_rgba[t_dst+2] = src->pixels_rgba[t_src+2];
        dst->pixels_rgba[t_dst+3] = src->pixels_rgba[t_src+3];
    }
}


/*
 * Superimpose src into dst, with transparency
*/

void ez_image_comp_blend (Ez_image *dst, Ez_image *src, int dst_x, int dst_y,
    int src_x, int src_y, int w, int h)
{
    int x, y, t_src, t_dst, a_src, a_dst, a_res;

    for (y = 0; y < h; y++)
    for (x = 0; x < w; x++) {
        t_src = ((y+src_y)*src->width+x+src_x)*4;
        t_dst = ((y+dst_y)*dst->width+x+dst_x)*4;

        a_src = src->pixels_rgba[t_src+3];
        a_dst = dst->pixels_rgba[t_dst+3];
        a_res = a_src + a_dst * (255-a_src) / 255;

        if (a_res == 0) {
            dst->pixels_rgba[t_dst  ] = 0;
            dst->pixels_rgba[t_dst+1] = 0;
            dst->pixels_rgba[t_dst+2] = 0;
            dst->pixels_rgba[t_dst+3] = 0;
            continue;
        }

        dst->pixels_rgba[t_dst  ] =
            ( src->pixels_rgba[t_src  ] * a_src +
              dst->pixels_rgba[t_dst  ] * a_dst * (255-a_src) / 255 ) / a_res;
        dst->pixels_rgba[t_dst+1] =
            ( src->pixels_rgba[t_src+1] * a_src +
              dst->pixels_rgba[t_dst+1] * a_dst * (255-a_src) / 255 ) / a_res;
        dst->pixels_rgba[t_dst+2] =
            ( src->pixels_rgba[t_src+2] * a_src +
              dst->pixels_rgba[t_dst+2] * a_dst * (255-a_src) / 255 ) / a_res;
        dst->pixels_rgba[t_dst+3] = a_res;
    }
}


/*
 * Extract a rectangular region from an image
*/

void ez_image_copy_sub (Ez_image *src, Ez_image *dest , int src_x, int src_y)
{
    int tw4  = dest->width*4,
        sw4  = src ->width*4,
        sxy4 = (src_y*src->width + src_x)*4,
        y, ty, sy;

    for (y = ty = 0, sy = sxy4; y < dest->height; y++, ty += tw4, sy += sw4)
        memcpy (dest->pixels_rgba + ty, src ->pixels_rgba + sy, tw4);
}


/*
 * Transformations
*/

void ez_image_comp_symv (Ez_image *src, Ez_image *dst)
{
    int dst_h = dst->height, dst_w = dst->width,
        x, y, ts, td, tw = dst_w;
    Ez_uint32 *src_p = (Ez_uint32 *)src->pixels_rgba,
              *dst_p = (Ez_uint32 *)dst->pixels_rgba;

    for (y = 0, td = 0      ; y < dst_h; y++)
    for (x = 0, ts = td+tw-1; x < dst_w; x++, td++, ts--)
        dst_p[td] = src_p[ts];
}


void ez_image_comp_symh (Ez_image *src, Ez_image *dst)
{
    int dst_h = dst->height, dst_w = dst->width,
        y, ts, td, tw4 = dst_w*4, tdm4 = (dst_h-1)*tw4;

    for (td = y = 0, ts = tdm4; y < dst_h; y++, td += tw4, ts -= tw4)
        memcpy (dst->pixels_rgba + td, src->pixels_rgba + ts, tw4);
}


void ez_image_expand (Ez_image *src, Ez_image *dst, double factor)
{
    int x, y, t;

    for (t = y = 0; y < dst->height; y++)
    for (    x = 0; x < dst->width ; x++, t+=4)
        ez_bilinear_4points (src->pixels_rgba, dst->pixels_rgba,
            src->width, src->height, x/factor, y/factor, t);
}


void ez_image_shrink (Ez_image *src, Ez_image *dst, double factor)
{
    int x, y, t;

    for (t = y = 0; y < dst->height; y++)
    for (    x = 0; x < dst->width ; x++, t+=4)
        ez_bilinear_pane (src->pixels_rgba, dst->pixels_rgba,
            src->width, src->height, x/factor, y/factor, t, factor);
}


void ez_rotate_get_size (double theta, int src_w, int src_h, int *dst_w, int *dst_h)
{
    double a = theta*M_PI/180, c = cos(a), s = sin(a),
           w1 = fabs (c*src_w - s*src_h), h1 = fabs (s*src_w + c*src_h),
           w2 = fabs (c*src_w + s*src_h), h2 = fabs (s*src_w - c*src_h);
    *dst_w = EZ_ROUND (w1 > w2 ? w1 : w2);
    *dst_h = EZ_ROUND (h1 > h2 ? h1 : h2);
}


void ez_rotate_get_coords (double theta, int src_w, int src_h, int src_x, int src_y,
    int *dst_x, int *dst_y)
{
    double a = theta*M_PI/180, c = cos(a), s = sin(a);
    double sx[4], sy[4], tx[4], ty[4], minx, miny;
    int i;

    /* Corners coordinates relative to the rotation center src */
    sx[0] = 0     - src_x; sy[0] = 0     - src_y;
    sx[1] = src_w - src_x; sy[1] = 0     - src_y;
    sx[2] = src_w - src_x; sy[2] = src_h - src_y;
    sx[3] = 0     - src_x; sy[3] = src_h - src_y;

    /* Corners rotation */
    for (i = 0; i < 4; i++) {
        tx[i] = c*sx[i] - s*sy[i];
        ty[i] = s*sx[i] + c*sy[i];
    }

    /* Research of min */
    minx = tx[0];
    miny = ty[0];
    for (i = 1; i < 4; i++) {
        if (tx[i] < minx) minx = tx[i];
        if (ty[i] < miny) miny = ty[i];
    }

    *dst_x = EZ_ROUND (-minx);
    *dst_y = EZ_ROUND (-miny);
}


void ez_image_rotate_nearest (Ez_image *src, Ez_image *dst, double theta)
{
    double a = theta*M_PI/180, c = cos(-a), s = sin(-a), sx, sy;
    int x, y, t, dx, dy, xn, yn, dst_x, dst_y,
        src_w = src->width, src_h = src->height,
        dst_w = dst->width, dst_h = dst->height;
    Ez_uint32 *src_p = (Ez_uint32 *)src->pixels_rgba,
              *dst_p = (Ez_uint32 *)dst->pixels_rgba;

    /* We set the rotation center to 0,0 and we retrieve the center
       coordinates dst_x,dst_y in dst */
    ez_rotate_get_coords (theta, src_w, src_h, 0, 0, &dst_x, &dst_y);

    for (t = y = 0, dy = -dst_y; y < dst_h; y++, dy++)
    for (    x = 0, dx = -dst_x; x < dst_w; x++, dx++, t++)
    {
        /* Coordinates of real antecedent and closest integer */
        sx = c*dx - s*dy; xn = EZ_ROUND(sx);
        sy = s*dx + c*dy; yn = EZ_ROUND(sy);

        /* Antecedent outside? */
        if (xn < 0 || xn >= src_w || yn < 0 || yn >= src_h)
             dst_p[t] = 0;
        else dst_p[t] = src_p[yn * src_w + xn];
    }
}


void ez_image_rotate_bilinear (Ez_image *src, Ez_image *dst, double theta)
{
    double a = theta*M_PI/180, c = cos(-a), s = sin(-a), sx, sy;
    int x, y, t, dx, dy, dst_x, dst_y,
        src_w = src->width, src_h = src->height,
        dst_w = dst->width, dst_h = dst->height;
    Ez_uint8 *src_p = src->pixels_rgba, *dst_p = dst->pixels_rgba;

    /* We set the rotation center to 0,0 and we retrieve the center
       coordinates dst_x,dst_y in dst */
    ez_rotate_get_coords (theta, src_w, src_h, 0, 0, &dst_x, &dst_y);

    for (t = y = 0, dy = -dst_y; y < dst_h; y++, dy++)
    for (    x = 0, dx = -dst_x; x < dst_w; x++, dx++, t+=4)
    {
        /* Coordinates of real antecedent */
        sx = c*dx - s*dy;
        sy = s*dx + c*dy;

        ez_bilinear_4points (src_p, dst_p, src_w, src_h, sx, sy, t);
    }
}


void ez_bilinear_4points (Ez_uint8 *src_p, Ez_uint8 *dst_p,
    int src_w, int src_h, double sx, double sy, int t)
{
    double rx0, rx1, ry0, ry1, r00, r01, r10, r11;
    int x0, y0, x1, y1, k00, k01, k10, k11;

    /* Antecedent outside? */
    if (sx < -0.5 || sx >= src_w-0.5 || sy < -0.5 || sy >= src_h-0.5) {
        dst_p[t] = dst_p[t+1] = dst_p[t+2] = dst_p[t+3] = 0;
        return;
    }

    /* Neighbors of real antecedent reel on the grid */
    x0 = sx < 0 ? sx-1 : sx; x1 = x0+1;
    y0 = sy < 0 ? sy-1 : sy; y1 = y0+1;

    /* Real distance */
    rx0 = sx-x0; rx1 = x1-sx;
    ry0 = sy-y0; ry1 = y1-sy;
    r00 = ry0*rx0; r01 = ry0*rx1;
    r10 = ry1*rx0; r11 = ry1*rx1;

    /* Neighbours offsets */
    k00 = (y0 * src_w + x0)*4; k01 = k00 + 4;
    k10 = k00 + src_w*4;       k11 = k10 + 4;

    /* Neighbour outside? We take the neighbour inside */
    if      (x0 < 0     ) k00 = k01, k10 = k11;
    else if (x1 >= src_w) k01 = k00, k11 = k10;
    if      (y0 < 0     ) k00 = k10, k01 = k11;
    else if (y1 >= src_h) k10 = k00, k11 = k01;

    /* Bilinear interpolation of colors and alpha channel */
    dst_p[t  ] = r11*src_p[k00  ] + r10*src_p[k01  ] + r01*src_p[k10  ] + r00*src_p[k11  ];
    dst_p[t+1] = r11*src_p[k00+1] + r10*src_p[k01+1] + r01*src_p[k10+1] + r00*src_p[k11+1];
    dst_p[t+2] = r11*src_p[k00+2] + r10*src_p[k01+2] + r01*src_p[k10+2] + r00*src_p[k11+2];
    dst_p[t+3] = r11*src_p[k00+3] + r10*src_p[k01+3] + r01*src_p[k10+3] + r00*src_p[k11+3];
}


void ez_bilinear_pane (Ez_uint8 *src_p, Ez_uint8 *dst_p,
    int src_w, int src_h, double sx, double sy, int t, double factor)
{
    double d = 1/factor, d2 = d*d, r = d/2, v, rx, ry,
           mx0 = sx-r, my0 = sy-r, mx1 = sx+r, my1 = sy+r;
    int x, y, i,
        x0 = EZ_ROUND (mx0), y0 = EZ_ROUND (my0),
        x1 = EZ_ROUND (mx1), y1 = EZ_ROUND (my1);

    if (x0 < 0) x0 = 0; else if (x1 > src_w-1) x1 = src_w-1;
    if (y0 < 0) y0 = 0; else if (y1 > src_h-1) y1 = src_h-1;

    if (x0 >= x1 || y0 >= y1) {
        dst_p[t] = dst_p[t+1] = dst_p[t+2] = dst_p[t+3] = 0;
        return;
    }

    for (i = 0; i < 4; i++) {
        v = 0;
        for (x = x0; x <= x1; x++)
        for (y = y0; y <= y1; y++) {
            rx = x == x0 ? x0+0.5-mx0 : x == x1 ? mx1-x1+0.5 : 1;
            ry = y == y0 ? y0+0.5-my0 : y == y1 ? my1-y1+0.5 : 1;
            v += src_p[(y*src_w+x)*4+i] * rx * ry;
        }
        dst_p[t+i] = v / d2;
    }
}


/*
 * Operations on Ez_pixmap
*/

#ifdef EZ_BASE_XLIB

int ez_pixmap_build_map (Ez_pixmap *pix, Ez_image *img)
{
    XImage *xi = NULL;
    ez_xi_func xi_func;

    pix->map = XCreatePixmap (ezx.display, ezx.root_win,
        img->width, img->height, ezx.depth);
    if (pix->map == None) return -1;

    xi_func = ez_xi_get_func ();
    xi = ez_xi_create (img, 0, 0, img->width, img->height, xi_func);
    if (xi == NULL) return -1;

    XPutImage (ezx.display, pix->map, ezx.gc, xi, 0, 0, 0, 0,
        img->width, img->height);
    XDestroyImage (xi);

    return 0;
}


void ez_pixmap_draw_area (Ez_window win, Ez_pixmap *pix, int x, int y)
{
    if (pix->mask != None) {
        XSetClipOrigin (ezx.display, ezx.gc, x, y);
        XSetClipMask (ezx.display, ezx.gc, pix->mask);
    }

    XCopyArea(ezx.display, pix->map, win, ezx.gc, 0, 0,
        pix->width, pix->height, x, y);

    if (pix->mask != None) {
        XSetClipOrigin (ezx.display, ezx.gc, 0, 0);
        XSetClipMask (ezx.display, ezx.gc, None);
    }
}


void ez_pixmap_tile_area (Ez_window win, Ez_pixmap *pix, int x, int y, int w, int h)
{
    int nx, ny;

    for (ny = 0; ny < h; ny += pix->height)
    for (nx = 0; nx < w; nx += pix->width)
    {
        if (pix->mask != None) {
            XSetClipOrigin (ezx.display, ezx.gc, x+nx, y+ny);
            XSetClipMask (ezx.display, ezx.gc, pix->mask);
        }

        XCopyArea(ezx.display, pix->map, win, ezx.gc, 0, 0,
            nx+pix->width  <= w ? pix->width  : w-nx,
            ny+pix->height <= h ? pix->height : h-ny,
            x+nx, y+ny);
    }

    if (pix->mask != None) {
        XSetClipOrigin (ezx.display, ezx.gc, 0, 0);
        XSetClipMask (ezx.display, ezx.gc, None);
    }
}

#elif defined EZ_BASE_WIN32

int ez_pixmap_build_hmap (Ez_pixmap *pix, Ez_image *img)
{
    HDC root_dc = NULL, pix_dc = NULL;
    int status = -1;

    root_dc = GetDC (NULL);
    if (root_dc == NULL) return -1;

    pix->hmap = CreateCompatibleBitmap (root_dc, img->width, img->height);
    if (pix->hmap == NULL) goto final;

    pix_dc = CreateCompatibleDC (root_dc);
    if (pix_dc == NULL) goto final;
    SelectObject (pix_dc, pix->hmap);

    ez_image_draw_dib (pix_dc, img, 0, 0, 0, 0, img->width, img->height);
    status = 0;

  final :
    if (root_dc != NULL) ReleaseDC (NULL, root_dc);
    if (pix_dc != NULL) DeleteDC (pix_dc);

    return status;
}


void ez_pixmap_draw_hmap (HDC hdc_dst, Ez_pixmap *pix, int x, int y)
{
    HDC hdc = NULL;

    /* Create a DC for the bitmap */
    hdc = CreateCompatibleDC(hdc_dst);
    if (hdc == NULL) {
        ez_error ("ez_pixmap_draw_hmap: can't create compatible DC\n");
        return;
    }
    SelectObject (hdc, pix->hmap);

    if (pix->has_alpha) {
        BLENDFUNCTION bf;

        bf.BlendOp = AC_SRC_OVER;
        bf.BlendFlags = 0;
        bf.AlphaFormat = AC_SRC_ALPHA;  /* Alpha channel premultiplied */
        bf.SourceConstantAlpha = 0xff;

        if (AlphaBlend (hdc_dst, x, y, pix->width, pix->height, hdc,
                0, 0, pix->width, pix->height, bf) == FALSE)
            ez_error ("ez_pixmap_draw_hmap: AlphaBlend failed\n");
    } else {
        if (BitBlt (hdc_dst, x, y, pix->width, pix->height, hdc,
                0, 0, SRCCOPY ) == 0)
            ez_error ("ez_pixmap_draw_hmap: bitblt failed\n");
    }

    if (hdc != NULL) DeleteDC (hdc);
}


void ez_pixmap_tile_hmap (HDC hdc_dst, Ez_pixmap *pix, int x, int y, int w, int h)
{
    HDC hdc = NULL;
    int nx, ny, nw, nh;
    BLENDFUNCTION bf;

    /* Create a DC for the bitmap */
    hdc = CreateCompatibleDC(hdc_dst);
    if (hdc == NULL) {
        ez_error ("ez_pixmap_tile_hmap: can't create compatible DC\n");
        return;
    }
    SelectObject (hdc, pix->hmap);

    if (pix->has_alpha) {
        bf.BlendOp = AC_SRC_OVER;
        bf.BlendFlags = 0;
        bf.AlphaFormat = AC_SRC_ALPHA;  /* Alpha channel premultiplied */
        bf.SourceConstantAlpha = 0xff;
    }

    for (ny = 0; ny < h; ny += pix->height)
    for (nx = 0; nx < w; nx += pix->width)
    {
        nw = nx+pix->width  <= w ? pix->width  : w-nx;
        nh = ny+pix->height <= h ? pix->height : h-ny;

        if (pix->has_alpha) {
            if (AlphaBlend (hdc_dst, x+nx, y+ny, nw, nh, hdc,
                    0, 0, nw, nh, bf) == FALSE) {
                ez_error ("ez_pixmap_tile_hmap: AlphaBlend failed\n");
                goto final;
            }
        } else {
            if (BitBlt (hdc_dst, x+nx, y+ny, nw, nh, hdc, 0, 0, SRCCOPY ) == 0) {
                ez_error ("ez_pixmap_tile_hmap: bitblt failed\n");
                goto final;
            }
        }
    }

  final:
    if (hdc != NULL) DeleteDC (hdc);
}

#endif /* EZ_BASE_ */


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

/*
 *  Basic usage:
 *     int x,y,n;
 *     Ez_uint8 *data = ez_stbi_load (filename, &x, &y, &comp, req_comp);
 *     if (data != NULL) ...
 *     stbi_image_free (data)
 *
 *  Standard parameters:
 *     int *x       -- outputs image width in pixels
 *     int *y       -- outputs image height in pixels
 *     int *comp    -- outputs # of image components in image file
 *     int req_comp -- if non-zero, # of image components requested in result
 *
 *  The return value from an image loader is an 'Ez_uint8 *' which points
 *  to the pixel data. The pixel data consists of *y scanlines of *x pixels,
 *  with each pixel consisting of N interleaved 8-bit components; the first
 *  pixel pointed to is top-left-most in the image. There is no padding between
 *  image scanlines or between pixels, regardless of format. The number of
 *  components N is 'req_comp' if req_comp is non-zero, or *comp otherwise.
 *  If req_comp is non-zero, *comp has the number of components that _would_
 *  have been output otherwise. E.g. if you set req_comp to 4, you will always
 *  get RGBA output, but you can check *comp to easily see if it's opaque.
 *
 *  An output image with N components has the following components interleaved
 *  in this order in each pixel:
 *
 *      N=#comp     components
 *        1           grey
 *        2           grey, alpha
 *        3           red, green, blue
 *        4           red, green, blue, alpha
 *
 *  If image loading fails for any reason, the return value will be NULL,
 *  and *x, *y, *comp will be unchanged.
 *
 *  Paletted PNG, BMP and GIF images are automatically depalettized.
*/


#if defined (_MSC_VER) && _MSC_VER >= 0x1400
#define _CRT_SECURE_NO_WARNINGS /* suppress bogus warnings about fopen () */
#endif

#ifndef _MSC_VER
  #ifdef __cplusplus
  #define EZ_INLINE inline
  #else
  #define EZ_INLINE
  #endif
#else
  #define EZ_INLINE __forceinline
#endif

#ifdef _MSC_VER
#define EZ_STBI_HAS_LROTL
#endif

#ifdef EZ_STBI_HAS_LROTL
  #define EZ_LROT(x,y)  _lrotl(x,y)
#else
  #define EZ_LROT(x,y)  (((x) << (y)) | ((x) >> (32 - (y))))
#endif


/*
 *  Ez_stbi struct and ez_stbi_start_xxx functions
 *
 *  Ez_stbi structure is our basic context used by all images, so it
 *  contains all the IO context, plus some basic image information
*/

typedef struct {
    Ez_uint32 img_x, img_y;
    int img_n, img_out_n;

    Ez_stbi_io_callbacks io;
    void *io_user_data;

    int read_from_callbacks;
    int buflen;
    Ez_uint8 buffer_start[128];

    Ez_uint8 *img_buffer, *img_buffer_end;
    Ez_uint8 *img_buffer_original;
} Ez_stbi;


void ez_refill_buffer (Ez_stbi *s);


/* Initialize a memory-decode context */

void ez_stbi_start_mem (Ez_stbi *s, Ez_uint8 const *buffer, int len)
{
    s->io.read = NULL;
    s->read_from_callbacks = 0;
    s->img_buffer = s->img_buffer_original = (Ez_uint8 *) buffer;
    s->img_buffer_end = (Ez_uint8 *) buffer+len;
}


/* Initialize a callback-based context */

void ez_stbi_start_callbacks (Ez_stbi *s, Ez_stbi_io_callbacks *c, void *user)
{
    s->io = *c;
    s->io_user_data = user;
    s->buflen = sizeof (s->buffer_start);
    s->read_from_callbacks = 1;
    s->img_buffer_original = s->buffer_start;
    ez_refill_buffer (s);
}


int ez_io_read (void *user, char *data, int size)
{
    return (int) fread (data, 1, size, (FILE*) user);
}

void ez_io_skip (void *user, unsigned n)
{
    fseek ((FILE*) user, n, SEEK_CUR);
}

int ez_io_eof (void *user)
{
    return feof ((FILE*) user);
}

Ez_stbi_io_callbacks ez_stbi_io_callbacks =
{
    ez_io_read,
    ez_io_skip,
    ez_io_eof,
};

void ez_start_file (Ez_stbi *s, FILE *f)
{
    ez_stbi_start_callbacks (s, &ez_stbi_io_callbacks, (void *) f);
}


void ez_stbi_rewind (Ez_stbi *s)
{
    /* Conceptually rewind SHOULD rewind to the beginning of the stream, but
       we just rewind to the beginning of the initial buffer, because we only
       use it after doing 'test', which only ever looks at at most 92 bytes */
    s->img_buffer = s->img_buffer_original;
}


int       ez_stbi_jpeg_test (Ez_stbi *s);
Ez_uint8 *ez_stbi_jpeg_load (Ez_stbi *s, int *x, int *y, int *comp, int req_comp);
int       ez_stbi_jpeg_info (Ez_stbi *s, int *x, int *y, int *comp);
int       ez_stbi_png_test  (Ez_stbi *s);
Ez_uint8 *ez_stbi_png_load  (Ez_stbi *s, int *x, int *y, int *comp, int req_comp);
int       ez_stbi_png_info  (Ez_stbi *s, int *x, int *y, int *comp);
int       ez_stbi_bmp_test  (Ez_stbi *s);
Ez_uint8 *ez_stbi_bmp_load  (Ez_stbi *s, int *x, int *y, int *comp, int req_comp);
int       ez_stbi_bmp_info  (Ez_stbi *s, int *x, int *y, int *comp);
int       ez_stbi_gif_test  (Ez_stbi *s);
Ez_uint8 *ez_stbi_gif_load  (Ez_stbi *s, int *x, int *y, int *comp, int req_comp);
int       ez_stbi_gif_info  (Ez_stbi *s, int *x, int *y, int *comp);


void ez_stbi_image_free (void *retval_from_stbi_load)
{
    free (retval_from_stbi_load);
}


Ez_uint8 *ez_stbi_load_main (Ez_stbi *s, int *x, int *y, int *comp,
    int req_comp)
{
    if (ez_stbi_jpeg_test (s)) return ez_stbi_jpeg_load (s, x, y, comp, req_comp);
    if (ez_stbi_png_test (s))  return ez_stbi_png_load  (s, x, y, comp, req_comp);
    if (ez_stbi_bmp_test (s))  return ez_stbi_bmp_load  (s, x, y, comp, req_comp);
    if (ez_stbi_gif_test (s))  return ez_stbi_gif_load  (s, x, y, comp, req_comp);

    ez_error ("ez_stbi_load_main: image not of any known type, or corrupt\n");
    return NULL;
}


Ez_uint8 *ez_stbi_load (char const *filename, int *x, int *y, int *comp,
    int req_comp)
{
    FILE *f = fopen (filename, "rb");
    Ez_uint8 *result;
    if (!f) {
        ez_error ("ez_stbi_load: unable to open file \"%s\"\n", filename);
        return NULL;
    }
    result = ez_stbi_load_from_file (f, x, y, comp, req_comp);
    fclose (f);
    return result;
}


Ez_uint8 *ez_stbi_load_from_file (FILE *f, int *x, int *y, int *comp,
    int req_comp)
{
    Ez_stbi s;
    ez_start_file (&s, f);
    return ez_stbi_load_main (&s, x, y, comp, req_comp);
}


Ez_uint8 *ez_stbi_load_from_memory (Ez_uint8 const *buffer, int len,
    int *x, int *y, int *comp, int req_comp)
{
    Ez_stbi s;
    ez_stbi_start_mem (&s, buffer, len);
    return ez_stbi_load_main (&s, x, y, comp, req_comp);
}


Ez_uint8 *ez_stbi_load_from_callbacks (Ez_stbi_io_callbacks const *clbk,
    void *user, int *x, int *y, int *comp, int req_comp)
{
    Ez_stbi s;
    ez_stbi_start_callbacks (&s, (Ez_stbi_io_callbacks *) clbk, user);
    return ez_stbi_load_main (&s, x, y, comp, req_comp);
}


/*
 * Common code used by all image loaders
*/

enum {
    EZ_SCAN_LOAD = 0,
    EZ_SCAN_TYPE,
    EZ_SCAN_HEADER
};


void ez_refill_buffer (Ez_stbi *s)
{
    int n = (s->io.read) (s->io_user_data, (char*)s->buffer_start, s->buflen);
    if (n == 0) {
        /* At end of file, treat same as if from memory */
        s->read_from_callbacks = 0;
        s->img_buffer = s->img_buffer_end-1;
        *s->img_buffer = 0;
    } else {
        s->img_buffer = s->buffer_start;
        s->img_buffer_end = s->buffer_start + n;
    }
}


EZ_INLINE int ez_buffer_get8 (Ez_stbi *s)
{
    if (s->img_buffer < s->img_buffer_end)
        return *s->img_buffer++;
    if (s->read_from_callbacks) {
        ez_refill_buffer (s);
        return *s->img_buffer++;
    }
    return 0;
}


EZ_INLINE int ez_buffer_at_eof (Ez_stbi *s)
{
    if (s->io.read) {
        if (! (s->io.eof) (s->io_user_data)) return 0;
        /* If feof () is true, check if buffer = end.
           Special case: we've only got the special 0 character at the end */
        if (s->read_from_callbacks == 0) return 1;
    }

    return s->img_buffer >= s->img_buffer_end;
}


EZ_INLINE Ez_uint8 ez_buffer_get8u (Ez_stbi *s)
{
    return (Ez_uint8) ez_buffer_get8 (s);
}


void ez_buffer_skip (Ez_stbi *s, int n)
{
    if (s->io.read) {
        int blen = s->img_buffer_end - s->img_buffer;
        if (blen < n) {
            s->img_buffer = s->img_buffer_end;
            (s->io.skip) (s->io_user_data, n - blen);
            return;
        }
    }
    s->img_buffer += n;
}


int ez_buffer_getn (Ez_stbi *s, Ez_uint8 *buffer, int n)
{
    if (s->io.read) {
        int blen = s->img_buffer_end - s->img_buffer;
        if (blen < n) {
            int res, count;

            memcpy (buffer, s->img_buffer, blen);
            count = (s->io.read) (s->io_user_data,
                        (char*) buffer + blen, n - blen);
            res = (count == (n-blen));
            s->img_buffer = s->img_buffer_end;
            return res;
        }
    }

    if (s->img_buffer+n <= s->img_buffer_end) {
        memcpy (buffer, s->img_buffer, n);
        s->img_buffer += n;
        return 1;
    } else
        return 0;
}


int ez_buffer_get16 (Ez_stbi *s)
{
    int z = ez_buffer_get8 (s);
    return (z << 8) + ez_buffer_get8 (s);
}


Ez_uint32 ez_buffer_get32 (Ez_stbi *s)
{
    Ez_uint32 z = ez_buffer_get16 (s);
    return (z << 16) + ez_buffer_get16 (s);
}


int ez_buffer_get16le (Ez_stbi *s)
{
    int z = ez_buffer_get8 (s);
    return z + (ez_buffer_get8 (s) << 8);
}


Ez_uint32 ez_buffer_get32le (Ez_stbi *s)
{
    Ez_uint32 z = ez_buffer_get16le (s);
    return z + (ez_buffer_get16le (s) << 16);
}


/*
 * Generic converter from built-in img_n to req_comp.
 *
 * Individual types do this automatically as much as possible (e.g. jpeg
 * does all cases internally since it needs to colorspace convert anyway,
 * and it never has alpha, so very few cases). png can automatically
 * interleave an alpha=255 channel, but falls back to this for other cases.
 *
 * Assume data buffer is malloced, so malloc a new one and free that one
 * only failure mode is malloc failing.
*/

Ez_uint8 ez_convert_comp_y (int r, int g, int b)
{
    return (Ez_uint8) (( (r*77) + (g*150) + (29*b)) >> 8);
}


Ez_uint8 *ez_convert_format (Ez_uint8 *data, int img_n, int req_comp,
    Ez_uint x, Ez_uint y)
{
    int i, j;
    Ez_uint8 *good;

    if (req_comp == img_n) return data;
    if (req_comp < 1 || req_comp > 4) {
        ez_error ("ez_convert_format: req_comp %d not in range 1..4\n", req_comp);
        return NULL;
    }

    good = malloc (req_comp * x * y);
    if (good == NULL) {
        free (data);
        ez_error ("ez_convert_format: out of memory\n");
        return NULL;
    }

    for (j=0; j < (int) y; ++j) {
        Ez_uint8 *src  = data + j * x * img_n   ;
        Ez_uint8 *dest = good + j * x * req_comp;

        #define COMBO(a,b)  ((a)*8+ (b))
        #define CASE(a,b)   case COMBO(a,b): \
            for (i=x-1; i >= 0; --i, src += a, dest += b)

        /* Convert source image with img_n components to one with req_comp
           components; avoid switch per pixel, so use switch per scanline
           and massive macros */
        switch (COMBO (img_n, req_comp)) {

            CASE (1,2) dest[0]=src[0], dest[1]=255; break;
            CASE (1,3) dest[0]=dest[1]=dest[2]=src[0]; break;
            CASE (1,4) dest[0]=dest[1]=dest[2]=src[0], dest[3]=255; break;
            CASE (2,1) dest[0]=src[0]; break;
            CASE (2,3) dest[0]=dest[1]=dest[2]=src[0]; break;
            CASE (2,4) dest[0]=dest[1]=dest[2]=src[0], dest[3]=src[1]; break;
            CASE (3,4) dest[0]=src[0], dest[1]=src[1], dest[2]=src[2],
                       dest[3]=255; break;
            CASE (3,1) dest[0]=ez_convert_comp_y (src[0], src[1], src[2]); break;
            CASE (3,2) dest[0]=ez_convert_comp_y (src[0], src[1], src[2]),
                       dest[1] = 255; break;
            CASE (4,1) dest[0]=ez_convert_comp_y (src[0], src[1], src[2]); break;
            CASE (4,2) dest[0]=ez_convert_comp_y (src[0], src[1], src[2]),
                       dest[1] = src[3]; break;
            CASE (4,3) dest[0]=src[0], dest[1]=src[1], dest[2]=src[2]; break;

            default:
                ez_error ("ez_convert_format: internal error for case (%d, %d)\n",
                    img_n, req_comp);
                return NULL;
        }
        #undef CASE
    }

    free (data);
    return good;
}


/*
 * "baseline" JPEG/JFIF decoder (not actually fully baseline implementation).
 *
 * Simple implementation:
 *     - channel subsampling of at most 2 in each dimension
 *     - doesn't support delayed output of y-dimension
 *     - simple interface (only one output format: 8-bit interleaved RGB)
 *     - doesn't try to recover corrupt jpegs
 *     - doesn't allow partial loading, loading multiple at once
 *     - still fast on x86 (copying globals into locals doesn't help x86)
 *     - allocates lots of intermediate memory (full size of all components)
 *     - non-interleaved case requires this anyway
 *     - allows good upsampling (see next)
 * High-quality:
 *     - upsampled channels are bilinearly interpolated, even across blocks
 *     - quality integer IDCT derived from IJG's 'slow'
 * Performance
 *     - fast huffman; reasonable integer IDCT
 *     - uses a lot of intermediate memory, could cache poorly
 *     - load http://nothings.org/remote/anemones.jpg 3 times on 2.8Ghz P4
 *         stb_jpeg:   1.34 seconds (MSVC6, default release build)
 *         stb_jpeg:   1.06 seconds (MSVC6, processor = Pentium Pro)
 *         IJL11.dll:  1.08 seconds (compiled by intel)
 *         IJG 1998:   0.98 seconds (MSVC6, makefile provided by IJG)
 *         IJG 1998:   0.95 seconds (MSVC6, makefile + proc=PPro)
*/

/* Huffman decoding acceleration */
#define EZ_FAST_BITS 9  /* larger handles more cases; smaller stomps less cache */

typedef struct {
    Ez_uint8  fast[1 << EZ_FAST_BITS];
    /* Weirdly, repacking this into AoS is a 10% speed loss, instead of a win */
    Ez_uint16 code[256];
    Ez_uint8  values[256];
    Ez_uint8  size[257];
    unsigned int maxcode[18];
    int       delta[17];   /* old 'firstsymbol' - old 'firstcode' */
} Ez_huffman;

typedef struct {
    Ez_stbi *s;
    Ez_huffman huff_dc[4];
    Ez_huffman huff_ac[4];
    Ez_uint8 dequant[4][64];

    /* Sizes for components, interleaved MCUs */
    int img_h_max, img_v_max;
    int img_mcu_x, img_mcu_y;
    int img_mcu_w, img_mcu_h;

    /* Definition of jpeg image component */
    struct {
        int id;
        int h, v;
        int tq;
        int hd, ha;
        int dc_pred;

        int x, y, w2, h2;
        Ez_uint8 *data;
        void *raw_data;
        Ez_uint8 *linebuf;
    } img_comp[4];

    Ez_uint32  code_buffer; /* jpeg entropy-coded buffer */
    int        code_bits;   /* number of valid bits */
    Ez_uint8   marker;      /* marker seen while filling entropy buffer */
    int        nomore;      /* flag if we saw a marker so must stop */

    int scan_n, order[4];
    int restart_interval, todo;
} Ez_jpeg;


int ez_build_huffman (Ez_huffman *h, int *count)
{
    int i, j, k=0, code;
    /* Build size list for each symbol (from JPEG spec) */
    for (i=0; i < 16; ++i)
        for (j=0; j < count[i]; ++j)
            h->size[k++] = (Ez_uint8) (i+1);
    h->size[k] = 0;

    /* Compute actual symbols (from jpeg spec) */
    code = 0;
    k = 0;
    for (j=1; j <= 16; ++j) {
        /* Compute delta to add to code to compute symbol id */
        h->delta[j] = k - code;
        if (h->size[k] == j) {
            while (h->size[k] == j)
                h->code[k++] = (Ez_uint16) (code++);
            if (code-1 >= (1 << j)) {
               ez_error ("ez_build_huffman: corrupt JPEG: bad code lengths\n");
               return 0;
            }
        }
        /* compute largest code + 1 for this size, preshifted as needed later */
        h->maxcode[j] = code << (16-j);
        code <<= 1;
    }
    h->maxcode[j] = 0xffffffff;

    /* Build non-spec acceleration table; 255 is flag for not-accelerated */
    memset (h->fast, 255, 1 << EZ_FAST_BITS);
    for (i=0; i < k; ++i) {
        int s = h->size[i];
        if (s <= EZ_FAST_BITS) {
            int c = h->code[i] << (EZ_FAST_BITS-s);
            int m = 1 << (EZ_FAST_BITS-s);
            for (j=0; j < m; ++j) {
                h->fast[c+j] = (Ez_uint8) i;
            }
        }
    }
    return 1;
}


void ez_grow_buffer_unsafe (Ez_jpeg *j)
{
    do {
        int b = j->nomore ? 0 : ez_buffer_get8 (j->s);
        if (b == 0xff) {
            int c = ez_buffer_get8 (j->s);
            if (c != 0) {
                j->marker = (Ez_uint8) c;
                j->nomore = 1;
                return;
            }
        }
        j->code_buffer |= b << (24 - j->code_bits);
        j->code_bits += 8;
    } while (j->code_bits <= 24);
}


/* (1 << n) - 1 */
Ez_uint32 bmask[17] = {0, 1, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095,
    8191, 16383, 32767, 65535};


/* Decode a jpeg huffman value from the bitstream */

EZ_INLINE int ez_jpeg_decode (Ez_jpeg *j, Ez_huffman *h)
{
    unsigned int temp;
    int c, k;

    if (j->code_bits < 16) ez_grow_buffer_unsafe (j);

    /* Look at the top EZ_FAST_BITS and determine what symbol ID it is,
       if the code is <= EZ_FAST_BITS */
    c = (j->code_buffer >> (32 - EZ_FAST_BITS)) & ((1 << EZ_FAST_BITS)-1);
    k = h->fast[c];
    if (k < 255) {
        int s = h->size[k];
        if (s > j->code_bits)
            return -1;
        j->code_buffer <<= s;
        j->code_bits -= s;
        return h->values[k];
    }

    /* Naive test is to shift the code_buffer down so k bits are
       valid, then test against maxcode. To speed this up, we've
       preshifted maxcode left so that it has (16-k) 0s at the
       end; in other words, regardless of the number of bits, it
       wants to be compared against something shifted to have 16;
       that way we don't need to shift inside the loop. */
    temp = j->code_buffer >> 16;
    for (k=EZ_FAST_BITS+1 ; ; ++k)
        if (temp < h->maxcode[k])
            break;
    if (k == 17) {
        /* error! code not found */
        j->code_bits -= 16;
        return -1;
    }

    if (k > j->code_bits)
        return -1;

    /* Convert the huffman code to the symbol id */
    c = ((j->code_buffer >> (32 - k)) & bmask[k]) + h->delta[k];
    if (( ((j->code_buffer) >> (32-h->size[c])) & bmask[h->size[c]]) != h->code[c]) {
        ez_error ("ez_jpeg_decode: internal error while converting huffman code\n");
        return 0;
    }

    /* Convert the id to a symbol */
    j->code_bits -= k;
    j->code_buffer <<= k;
    return h->values[c];
}


/* Combined JPEG 'receive' and JPEG 'extend', since baseline
   always extends everything it receives. */

EZ_INLINE int ez_jpeg_extend_receive (Ez_jpeg *j, int n)
{
    unsigned int m = 1 << (n-1);
    unsigned int k;
    if (j->code_bits < n) ez_grow_buffer_unsafe (j);

    k = EZ_LROT (j->code_buffer, n);
    j->code_buffer = k & ~bmask[n];
    k &= bmask[n];
    j->code_bits -= n;

    /* The following test is probably a random branch that won't
       predict well. I tried to table accelerate it but failed.
       maybe it's compiling as a conditional move? */
    if (k < m)
         return (-1 << n) + k + 1;
    else return k;
}


/* Given a value that's at position X in the zigzag stream,
   where does it appear in the 8x8 matrix coded as row-major? */

Ez_uint8 dezigzag[64+15] =
{
    0,  1,  8, 16,  9,  2,  3, 10,
   17, 24, 32, 25, 18, 11,  4,  5,
   12, 19, 26, 33, 40, 48, 41, 34,
   27, 20, 13,  6,  7, 14, 21, 28,
   35, 42, 49, 56, 57, 50, 43, 36,
   29, 22, 15, 23, 30, 37, 44, 51,
   58, 59, 52, 45, 38, 31, 39, 46,
   53, 60, 61, 54, 47, 55, 62, 63,
   /* let corrupt input sample past end */
   63, 63, 63, 63, 63, 63, 63, 63,
   63, 63, 63, 63, 63, 63, 63
};

/* Decode one 64-entry block */

int ez_jpeg_decode_block (Ez_jpeg *j, short data[64], Ez_huffman *hdc,
    Ez_huffman *hac, int b)
{
    int diff, dc, k;
    int t = ez_jpeg_decode (j, hdc);
    if (t < 0) {
        ez_error ("ez_jpeg_decode_block: corrupt JPEG: bad huffman code\n");
        return 0;
    }

    /* 0 all the ac values now so we can do it 32-bits at a time */
    memset (data, 0, 64*sizeof (data[0]));

    diff = t ? ez_jpeg_extend_receive (j, t) : 0;
    dc = j->img_comp[b].dc_pred + diff;
    j->img_comp[b].dc_pred = dc;
    data[0] = (short) dc;

    /* Decode AC components, see JPEG spec */
    k = 1;
    do {
        int r, s;
        int rs = ez_jpeg_decode (j, hac);
        if (rs < 0)  {
            ez_error ("ez_jpeg_decode_block: corrupt JPEG: bad huffman code\n");
            return 0;
        }
        s = rs & 15;
        r = rs >> 4;
        if (s == 0) {
            if (rs != 0xf0) break; /* end block */
            k += 16;
        } else {
            k += r;
            /* decode into unzigzag'd location */
            data[dezigzag[k++]] = (short) ez_jpeg_extend_receive (j, s);
        }
    } while (k < 64);
    return 1;
}


/* Take a -128..127 value and clamp it and convert to 0..255 */

EZ_INLINE Ez_uint8 ez_jpeg_clamp (int x)
{
    /* Trick to use a single test to catch both cases */
    if ((unsigned int) x > 255) {
        if (x < 0) return 0;
        if (x > 255) return 255;
    }
    return (Ez_uint8) x;
}


#define Ez_f2f(x)  (int)(((x) * 4096 + 0.5))
#define Ez_fsh(x)  ((x) << 12)

/* derived from jidctint -- DCT_ISLOW */
#define EZ_IDCT_1D(s0,s1,s2,s3,s4,s5,s6,s7)     \
    int t0, t1, t2, t3, p1, p2, p3, p4, p5, x0, x1, x2, x3; \
    p2 = s2;                                    \
    p3 = s6;                                    \
    p1 = (p2+p3) * Ez_f2f (0.5411961f);         \
    t2 = p1 + p3*Ez_f2f (-1.847759065f);        \
    t3 = p1 + p2*Ez_f2f ( 0.765366865f);        \
    p2 = s0;                                    \
    p3 = s4;                                    \
    t0 = Ez_fsh (p2+p3);                        \
    t1 = Ez_fsh (p2-p3);                        \
    x0 = t0+t3;                                 \
    x3 = t0-t3;                                 \
    x1 = t1+t2;                                 \
    x2 = t1-t2;                                 \
    t0 = s7;                                    \
    t1 = s5;                                    \
    t2 = s3;                                    \
    t3 = s1;                                    \
    p3 = t0+t2;                                 \
    p4 = t1+t3;                                 \
    p1 = t0+t3;                                 \
    p2 = t1+t2;                                 \
    p5 = (p3+p4)*Ez_f2f ( 1.175875602f);        \
    t0 = t0*Ez_f2f ( 0.298631336f);             \
    t1 = t1*Ez_f2f ( 2.053119869f);             \
    t2 = t2*Ez_f2f ( 3.072711026f);             \
    t3 = t3*Ez_f2f ( 1.501321110f);             \
    p1 = p5 + p1*Ez_f2f (-0.899976223f);        \
    p2 = p5 + p2*Ez_f2f (-2.562915447f);        \
    p3 = p3*Ez_f2f (-1.961570560f);             \
    p4 = p4*Ez_f2f (-0.390180644f);             \
    t3 += p1+p4;                                \
    t2 += p2+p3;                                \
    t1 += p2+p4;                                \
    t0 += p1+p3;

typedef Ez_uint8 Ez_stbi_dequantize_t;


/* .344 seconds on 3*anemones.jpg */

void ez_jpeg_idct_block (Ez_uint8 *out, int out_stride, short data[64],
    Ez_stbi_dequantize_t *dequantize)
{
    int i, val[64], *v=val;
    Ez_stbi_dequantize_t *dq = dequantize;
    Ez_uint8 *o;
    short *d = data;

    /* Columns */
    for (i=0; i < 8; ++i, ++d, ++dq, ++v) {
        /* If all zeroes, shortcut -- this avoids dequantizing 0s and IDCTing */
        if (d[ 8]==0 && d[16]==0 && d[24]==0 && d[32]==0 &&
            d[40]==0 && d[48]==0 && d[56]==0)
        {
            /* no shortcut                 0     seconds
               (1|2|3|4|5|6|7)==0          0     seconds
               all separate               -0.047 seconds
               1 && 2|3 && 4|5 && 6|7:    -0.047 seconds */
            int dcterm = d[0] * dq[0] << 2;
            v[0] = v[8] = v[16] = v[24] = v[32] = v[40] = v[48] = v[56] = dcterm;
        } else {
            EZ_IDCT_1D (d[ 0]*dq[ 0], d[ 8]*dq[ 8], d[16]*dq[16], d[24]*dq[24],
                     d[32]*dq[32], d[40]*dq[40], d[48]*dq[48], d[56]*dq[56])
            /* Constants scaled things up by 1<<12; let's bring them back
               down, but keep 2 extra bits of precision */
            x0 += 512; x1 += 512; x2 += 512; x3 += 512;
            v[ 0] = (x0+t3) >> 10;
            v[56] = (x0-t3) >> 10;
            v[ 8] = (x1+t2) >> 10;
            v[48] = (x1-t2) >> 10;
            v[16] = (x2+t1) >> 10;
            v[40] = (x2-t1) >> 10;
            v[24] = (x3+t0) >> 10;
            v[32] = (x3-t0) >> 10;
        }
    }

    for (i=0, v=val, o=out; i < 8; ++i, v+=8, o+=out_stride) {
        /* No fast case since the first 1D IDCT spread components out */
        EZ_IDCT_1D (v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7])
        /* Constants scaled things up by 1<<12, plus we had 1<<2 from first
           loop, plus horizontal and vertical each scale by sqrt (8) so together
           we've got an extra 1<<3, so 1<<17 total we need to remove.
           so we want to round that, which means adding 0.5 * 1<<17,
           aka 65536. Also, we'll end up with -128 to 127 that we want to
           encode as 0..255 by adding 128, so we'll add that before the shift */
        x0 += 65536 + (128<<17);
        x1 += 65536 + (128<<17);
        x2 += 65536 + (128<<17);
        x3 += 65536 + (128<<17);
        /* Tried computing the shifts into temps, or'ing the temps to see
           if any were out of range, but that was slower */
        o[0] = ez_jpeg_clamp ((x0+t3) >> 17);
        o[7] = ez_jpeg_clamp ((x0-t3) >> 17);
        o[1] = ez_jpeg_clamp ((x1+t2) >> 17);
        o[6] = ez_jpeg_clamp ((x1-t2) >> 17);
        o[2] = ez_jpeg_clamp ((x2+t1) >> 17);
        o[5] = ez_jpeg_clamp ((x2-t1) >> 17);
        o[3] = ez_jpeg_clamp ((x3+t0) >> 17);
        o[4] = ez_jpeg_clamp ((x3-t0) >> 17);
    }
}


#define EZ_MARKER_NONE  0xff

/* If there's a pending marker from the entropy stream, return that
   otherwise, fetch from the stream and get a marker. if there's no
   marker, return 0xff, which is never a valid marker value */

Ez_uint8 ez_jpeg_get_marker (Ez_jpeg *j)
{
    Ez_uint8 x;
    if (j->marker != EZ_MARKER_NONE) {
        x = j->marker; j->marker = EZ_MARKER_NONE; return x;
    }
    x = ez_buffer_get8u (j->s);
    if (x != 0xff) return EZ_MARKER_NONE;
    while (x == 0xff)
        x = ez_buffer_get8u (j->s);
    return x;
}


/* In each scan, we'll have scan_n components, and the order
   of the components is specified by order[] */
#define EZ_RESTART(x)     ((x) >= 0xd0 && (x) <= 0xd7)


/* After a restart interval, reset the entropy decoder and
   the dc prediction */

void ez_jpeg_reset (Ez_jpeg *j)
{
    j->code_bits = 0;
    j->code_buffer = 0;
    j->nomore = 0;
    j->img_comp[0].dc_pred = j->img_comp[1].dc_pred = j->img_comp[2].dc_pred = 0;
    j->marker = EZ_MARKER_NONE;
    j->todo = j->restart_interval ? j->restart_interval : 0x7fffffff;
    /* No more than 1<<31 MCUs if no restart_interal? that's plenty safe,
       since we don't even allow 1<<30 pixels */
}


int ez_jpeg_parse_entropy_coded_data (Ez_jpeg *z)
{
    ez_jpeg_reset (z);
    if (z->scan_n == 1) {
        int i, j;
        short data[64];
        int n = z->order[0];
        /* Non-interleaved data, we just need to process one block at a time,
           in trivial scanline order
           number of blocks to do just depends on how many actual "pixels" this
           component has, independent of interleaved MCU blocking and such */
        int w = (z->img_comp[n].x+7) >> 3;
        int h = (z->img_comp[n].y+7) >> 3;
        for (j=0; j < h; ++j) {
            for (i=0; i < w; ++i) {
                if (!ez_jpeg_decode_block (z, data, z->huff_dc+z->img_comp[n].hd,
                    z->huff_ac+z->img_comp[n].ha, n)) return 0;
                ez_jpeg_idct_block (z->img_comp[n].data+z->img_comp[n].w2*j*8+i*8,
                    z->img_comp[n].w2, data, z->dequant[z->img_comp[n].tq]);

                /* Every data block is an MCU, so countdown the restart interval */
                if (--z->todo <= 0) {
                    if (z->code_bits < 24) ez_grow_buffer_unsafe (z);
                    /* If it's NOT a restart, then just bail, so we get corrupt
                       data rather than no data */
                    if (!EZ_RESTART (z->marker)) return 1;
                    ez_jpeg_reset (z);
                }
            }
        }
    } else { /* interleaved! */
        int i, j, k, x, y;
        short data[64];
        for (j=0; j < z->img_mcu_y; ++j) {
            for (i=0; i < z->img_mcu_x; ++i) {
                /* Scan an interleaved mcu... process scan_n components in order */
                for (k=0; k < z->scan_n; ++k) {
                    int n = z->order[k];
                    /* Scan out an mcu's worth of this component; that's just
                       determined by the basic H and V specified for the component */
                    for (y=0; y < z->img_comp[n].v; ++y) {
                        for (x=0; x < z->img_comp[n].h; ++x) {
                            int x2 = (i*z->img_comp[n].h + x)*8;
                            int y2 = (j*z->img_comp[n].v + y)*8;
                            if (!ez_jpeg_decode_block (z, data,
                                z->huff_dc+z->img_comp[n].hd,
                                z->huff_ac+z->img_comp[n].ha, n)) return 0;

                            ez_jpeg_idct_block (
                                z->img_comp[n].data+z->img_comp[n].w2*y2+x2,
                                z->img_comp[n].w2, data,
                                z->dequant[z->img_comp[n].tq]);
                        }
                    }
                }
                /* After all interleaved components, that's an interleaved MCU,
                   so now count down the restart interval */
                if (--z->todo <= 0) {
                    if (z->code_bits < 24) ez_grow_buffer_unsafe (z);
                    /* If it's NOT a restart, then just bail, so we get corrupt
                       data rather than no data */
                    if (!EZ_RESTART (z->marker)) return 1;
                    ez_jpeg_reset (z);
                }
            }
        }
    }
    return 1;
}


int ez_jpeg_process_marker (Ez_jpeg *z, int m)
{
    int L;
    switch (m) {
        case EZ_MARKER_NONE: /* no marker found */
            ez_error ("ez_jpeg_process_marker: corrupt JPEG: expected marker\n");
            return 0;

        case 0xC2: /* SOF - progressive */
            ez_error ("ez_jpeg_process_marker: JPEG format not supported (progressive)\n");
            return 0;

        case 0xDD: /* DRI - specify restart interval */
            if (ez_buffer_get16 (z->s) != 4) {
                ez_error ("ez_jpeg_process_marker: corrupt JPEG: bad DRI len\n");
                return 0;
            }
            z->restart_interval = ez_buffer_get16 (z->s);
            return 1;

        case 0xDB: /* DQT - define quantization table */
            L = ez_buffer_get16 (z->s)-2;
            while (L > 0) {
                int q = ez_buffer_get8 (z->s);
                int p = q >> 4;
                int t = q & 15, i;
                if (p != 0) {
                    ez_error ("ez_jpeg_process_marker: corrupt JPEG: bad DQT type\n");
                    return 0;
                }
                if (t > 3) {
                    ez_error ("ez_jpeg_process_marker: corrupt JPEG: bad DQT table\n");
                    return 0;
                }
                for (i=0; i < 64; ++i)
                    z->dequant[t][dezigzag[i]] = ez_buffer_get8u (z->s);
                L -= 65;
            }
            return L==0;

        case 0xC4: /* DHT - define huffman table */
            L = ez_buffer_get16 (z->s)-2;
            while (L > 0) {
                Ez_uint8 *v;
                int sizes[16], i, m=0;
                int q = ez_buffer_get8 (z->s);
                int tc = q >> 4;
                int th = q & 15;
                if (tc > 1 || th > 3) {
                    ez_error ("ez_jpeg_process_marker: corrupt JPEG: bad DHT header\n");
                    return 0;
                }
                for (i=0; i < 16; ++i) {
                    sizes[i] = ez_buffer_get8 (z->s);
                    m += sizes[i];
                }
                L -= 17;
                if (tc == 0) {
                    if (!ez_build_huffman (z->huff_dc+th, sizes)) return 0;
                    v = z->huff_dc[th].values;
                } else {
                    if (!ez_build_huffman (z->huff_ac+th, sizes)) return 0;
                    v = z->huff_ac[th].values;
                }
                for (i=0; i < m; ++i)
                    v[i] = ez_buffer_get8u (z->s);
                L -= m;
            }
            return L==0;
    }

    /* check for comment block or APP blocks */
    if ((m >= 0xE0 && m <= 0xEF) || m == 0xFE) {
        ez_buffer_skip (z->s, ez_buffer_get16 (z->s)-2);
        return 1;
    }
   return 0;
}


/* after we see SOS */

int ez_jpeg_process_scan_header (Ez_jpeg *z)
{
    int i;
    int Ls = ez_buffer_get16 (z->s);
    z->scan_n = ez_buffer_get8 (z->s);
    if (z->scan_n < 1 || z->scan_n > 4 || z->scan_n > (int) z->s->img_n) {
        ez_error ("ez_jpeg_process_scan_header: corrupt JPEG: bad SOS component count\n");
        return 0;
    }
    if (Ls != 6+2*z->scan_n) {
        ez_error ("ez_jpeg_process_scan_header: corrupt JPEG: bad SOS len\n");
        return 0;
    }
    for (i=0; i < z->scan_n; ++i) {
        int id = ez_buffer_get8 (z->s), which;
        int q = ez_buffer_get8 (z->s);
        for (which = 0; which < z->s->img_n; ++which)
            if (z->img_comp[which].id == id)
                break;
        if (which == z->s->img_n) return 0;
        z->img_comp[which].hd = q >> 4;
        if (z->img_comp[which].hd > 3) {
            ez_error ("ez_jpeg_process_scan_header: corrupt JPEG: bad DC huff\n");
            return 0;
        }
        z->img_comp[which].ha = q & 15;
        if (z->img_comp[which].ha > 3) {
            ez_error ("ez_jpeg_process_scan_header: corrupt JPEG: bad AC huff\n");
            return 0;
        }
        z->order[i] = which;
    }
    if (ez_buffer_get8 (z->s) != 0) {
        ez_error ("ez_jpeg_process_scan_header: corrupt JPEG: bad SOS\n");
        return 0;
    }
    ez_buffer_get8 (z->s); /* should be 63, but might be 0 */
    if (ez_buffer_get8 (z->s) != 0) {
        ez_error ("ez_jpeg_process_scan_header: corrupt JPEG: bad SOS\n");
        return 0;
    }

    return 1;
}

int ez_jpeg_process_frame_header (Ez_jpeg *z, int scan)
{
    Ez_stbi *s = z->s;
    int Lf, p, i, q, h_max=1, v_max=1, c;
    Lf = ez_buffer_get16 (s); if (Lf < 11) {
        ez_error ("ez_jpeg_process_frame_header: corrupt JPEG: bad SOF len\n");
        return 0;
    }
    p = ez_buffer_get8 (s); if (p != 8) {
        ez_error ("ez_jpeg_process_frame_header: JPEG format not supported: 8-bit only\n");
        return 0;
    }
    s->img_y = ez_buffer_get16 (s); if (s->img_y == 0) {
        ez_error ("ez_jpeg_process_frame_header: JPEG format not supported: delayed height\n");
        return 0;
    }
    s->img_x = ez_buffer_get16 (s); if (s->img_x == 0) {
        ez_error ("ez_jpeg_process_frame_header: corrupt JPEG: 0 width\n");
        return 0;
    }
    c = ez_buffer_get8 (s);
    if (c != 3 && c != 1) {
        ez_error ("ez_jpeg_process_frame_header: corrupt JPEG: bad component count\n");
        return 0;
    }
    s->img_n = c;
    for (i=0; i < c; ++i) {
        z->img_comp[i].data = NULL;
        z->img_comp[i].linebuf = NULL;
    }

    if (Lf != 8+3*s->img_n) {
        ez_error ("ez_jpeg_process_frame_header: corrupt JPEG: bad SOF len\n");
        return 0;
    }

    for (i=0; i < s->img_n; ++i) {
        z->img_comp[i].id = ez_buffer_get8 (s);
        if (z->img_comp[i].id != i+1)   /* JFIF requires */
            if (z->img_comp[i].id != i) {
                /* Some version of jpegtran outputs non-JFIF-compliant files! */
                ez_error ("ez_jpeg_process_frame_header: corrupt JPEG: bad component ID\n");
                return 0;
            }
        q = ez_buffer_get8 (s);
        z->img_comp[i].h = (q >> 4);
        if (!z->img_comp[i].h || z->img_comp[i].h > 4) {
            ez_error ("ez_jpeg_process_frame_header: corrupt JPEG: bad H\n");
            return 0;
        }
        z->img_comp[i].v = q & 15;
        if (!z->img_comp[i].v || z->img_comp[i].v > 4)  {
            ez_error ("ez_jpeg_process_frame_header: corrupt JPEG: bad V\n");
            return 0;
        }
        z->img_comp[i].tq = ez_buffer_get8 (s);
        if (z->img_comp[i].tq > 3)  {
            ez_error ("ez_jpeg_process_frame_header: corrupt JPEG: bad TQ\n");
            return 0;
        }
    }

    if (scan != EZ_SCAN_LOAD) return 1;

    if ((1 << 30) / s->img_x / s->img_n < s->img_y)  {
        ez_error ("ez_jpeg_process_frame_header: JPEG image too large to decode\n");
        return 0;
    }

    for (i=0; i < s->img_n; ++i) {
        if (z->img_comp[i].h > h_max) h_max = z->img_comp[i].h;
        if (z->img_comp[i].v > v_max) v_max = z->img_comp[i].v;
    }

    /* Compute interleaved mcu info */
    z->img_h_max = h_max;
    z->img_v_max = v_max;
    z->img_mcu_w = h_max * 8;
    z->img_mcu_h = v_max * 8;
    z->img_mcu_x = (s->img_x + z->img_mcu_w-1) / z->img_mcu_w;
    z->img_mcu_y = (s->img_y + z->img_mcu_h-1) / z->img_mcu_h;

    for (i=0; i < s->img_n; ++i) {
        /* Number of effective pixels (e.g. for non-interleaved MCU) */
        z->img_comp[i].x = (s->img_x * z->img_comp[i].h + h_max-1) / h_max;
        z->img_comp[i].y = (s->img_y * z->img_comp[i].v + v_max-1) / v_max;
        /* To simplify generation, we'll allocate enough memory to decode
           the bogus oversized data from using interleaved MCUs and their
           big blocks (e.g. a 16x16 iMCU on an image of width 33); we won't
           discard the extra data until colorspace conversion */
        z->img_comp[i].w2 = z->img_mcu_x * z->img_comp[i].h * 8;
        z->img_comp[i].h2 = z->img_mcu_y * z->img_comp[i].v * 8;
        z->img_comp[i].raw_data = malloc (z->img_comp[i].w2 * z->img_comp[i].h2+15);
        if (z->img_comp[i].raw_data == NULL) {
            for (--i; i >= 0; --i) {
                free (z->img_comp[i].raw_data);
                z->img_comp[i].data = NULL;
            }
            ez_error ("ez_jpeg_process_frame_header: out of memory\n");
            return 0;
        }
        /* Align blocks for installable-idct using mmx/sse */
        z->img_comp[i].data =
            (Ez_uint8*) (( (size_t) z->img_comp[i].raw_data + 15) & ~15);
        z->img_comp[i].linebuf = NULL;
    }

    return 1;
}


/* Use comparisons since in some cases we handle more than one case (e.g. SOF) */
#define EZ_DNL(x)         ((x) == 0xdc)
#define EZ_SOI(x)         ((x) == 0xd8)
#define EZ_EOI(x)         ((x) == 0xd9)
#define EZ_SOF(x)         ((x) == 0xc0 || (x) == 0xc1)
#define EZ_SOS(x)         ((x) == 0xda)


int ez_jpeg_decode_header (Ez_jpeg *z, int scan, int verbose)
{
    int m;
    z->marker = EZ_MARKER_NONE; /* initialize cached marker to empty */
    m = ez_jpeg_get_marker (z);
    if (!EZ_SOI (m)) {
        if (verbose) ez_error ("ez_jpeg_decode_header: corrupt JPEG: no SOI\n");
        return 0;
    }
    if (scan == EZ_SCAN_TYPE) return 1;
    m = ez_jpeg_get_marker (z);
    while (!EZ_SOF (m)) {
        if (!ez_jpeg_process_marker (z, m)) return 0;
        m = ez_jpeg_get_marker (z);
        while (m == EZ_MARKER_NONE) {
            /* Some files have extra padding after their blocks, so ok, we'll scan */
            if (ez_buffer_at_eof (z->s)) {
                if (verbose) ez_error ("ez_jpeg_decode_header: corrupt JPEG: no SOF\n");
                return 0;
            }
            m = ez_jpeg_get_marker (z);
        }
    }
    if (!ez_jpeg_process_frame_header (z, scan)) return 0;
    return 1;
}


int ez_jpeg_decode_image (Ez_jpeg *j)
{
    int m;
    j->restart_interval = 0;
    if (!ez_jpeg_decode_header (j, EZ_SCAN_LOAD, 1)) return 0;
    m = ez_jpeg_get_marker (j);
    while (!EZ_EOI (m)) {
        if (EZ_SOS (m)) {
            if (!ez_jpeg_process_scan_header (j)) return 0;
            if (!ez_jpeg_parse_entropy_coded_data (j)) return 0;
            if (j->marker == EZ_MARKER_NONE ) {
                /* Handle 0s at the end of image data from IP Kamera 9060 */
                while (!ez_buffer_at_eof (j->s)) {
                    int x = ez_buffer_get8 (j->s);
                    if (x == 255) {
                        j->marker = ez_buffer_get8u (j->s);
                        break;
                    } else if (x != 0) {
                        return 0;
                    }
                }
                /* If we reach eof without hitting a marker, ez_jpeg_get_marker ()
                   below will fail and we'll eventually return 0 */
            }
        } else {
            if (!ez_jpeg_process_marker (j, m)) return 0;
        }
        m = ez_jpeg_get_marker (j);
    }
    return 1;
}


/* jfif-centered resampling (across block boundaries) */

typedef Ez_uint8* (*Ez_resample_row_func)
    (Ez_uint8 *out, Ez_uint8 *in0, Ez_uint8 *in1, int w, int hs);

#define div4(x) ((Ez_uint8) ((x) >> 2))


Ez_uint8 *ez_jpeg_resample_row_1 (Ez_uint8 *out, Ez_uint8 *in_near,
    Ez_uint8 *in_far, int w, int hs)
{
    (void) out;
    (void) in_far;
    (void) w;
    (void) hs;
    return in_near;
}

Ez_uint8* ez_jpeg_resample_row_v_2 (Ez_uint8 *out, Ez_uint8 *in_near,
    Ez_uint8 *in_far, int w, int hs)
{
    /* Need to generate two samples vertically for every one in input */
    int i;
    (void) hs;
    for (i=0; i < w; ++i)
        out[i] = div4 (3*in_near[i] + in_far[i] + 2);
    return out;
}


Ez_uint8*  ez_jpeg_resample_row_h_2 (Ez_uint8 *out, Ez_uint8 *in_near,
    Ez_uint8 *in_far, int w, int hs)
{
    /* Need to generate two samples horizontally for every one in input */
    int i;
    Ez_uint8 *input = in_near;

    if (w == 1) {
        /* If only one sample, can't do any interpolation */
        out[0] = out[1] = input[0];
        return out;
    }

    out[0] = input[0];
    out[1] = div4 (input[0]*3 + input[1] + 2);
    for (i=1; i < w-1; ++i) {
        int n = 3*input[i]+2;
        out[i*2+0] = div4 (n+input[i-1]);
        out[i*2+1] = div4 (n+input[i+1]);
    }
    out[i*2+0] = div4 (input[w-2]*3 + input[w-1] + 2);
    out[i*2+1] = input[w-1];

    (void) in_far;
    (void) hs;

    return out;
}


#define div16(x) ((Ez_uint8) ((x) >> 4))

Ez_uint8 *ez_jpeg_resample_row_hv_2 (Ez_uint8 *out, Ez_uint8 *in_near,
    Ez_uint8 *in_far, int w, int hs)
{
    /* Need to generate 2x2 samples for every one in input */
    int i, t0, t1;
    if (w == 1) {
        out[0] = out[1] = div4 (3*in_near[0] + in_far[0] + 2);
        return out;
    }

    t1 = 3*in_near[0] + in_far[0];
    out[0] = div4 (t1+2);
    for (i=1; i < w; ++i) {
        t0 = t1;
        t1 = 3*in_near[i]+in_far[i];
        out[i*2-1] = div16 (3*t0 + t1 + 8);
        out[i*2  ] = div16 (3*t1 + t0 + 8);
    }
    out[w*2-1] = div4 (t1+2);

    (void) hs;

    return out;
}


Ez_uint8 *ez_jpeg_resample_row_generic (Ez_uint8 *out, Ez_uint8 *in_near,
    Ez_uint8 *in_far, int w, int hs)
{
    /* Resample with nearest-neighbor */
    int i, j;
    (void) in_far;
    for (i=0; i < w; ++i)
        for (j=0; j < hs; ++j)
            out[i*hs+j] = in_near[i];
    return out;
}


#define Ez_float2fixed(x)  ((int) ((x) * 65536 + 0.5))

/* 0.38 seconds on 3*anemones.jpg   (0.25 with processor = Pro)
   VC6 without processor=Pro is generating multiple LEAs per multiply! */

void ez_jpeg_YCbCr_to_RGB_row (Ez_uint8 *out, const Ez_uint8 *y,
    const Ez_uint8 *pcb, const Ez_uint8 *pcr, int count, int step)
{
    int i;
    for (i=0; i < count; ++i) {
        int y_fixed = (y[i] << 16) + 32768; /* rounding */
        int r, g, b;
        int cr = pcr[i] - 128;
        int cb = pcb[i] - 128;
        r = y_fixed + cr*Ez_float2fixed (1.40200f);
        g = y_fixed - cr*Ez_float2fixed (0.71414f) - cb*Ez_float2fixed (0.34414f);
        b = y_fixed                                + cb*Ez_float2fixed (1.77200f);
        r >>= 16;
        g >>= 16;
        b >>= 16;
        if ((unsigned) r > 255) { if (r < 0) r = 0; else r = 255; }
        if ((unsigned) g > 255) { if (g < 0) g = 0; else g = 255; }
        if ((unsigned) b > 255) { if (b < 0) b = 0; else b = 255; }
        out[0] = (Ez_uint8)r;
        out[1] = (Ez_uint8)g;
        out[2] = (Ez_uint8)b;
        out[3] = 255;
        out += step;
    }
}


/* clean up the temporary component buffers */

void ez_jpeg_cleanup (Ez_jpeg *j)
{
    int i;
    for (i=0; i < j->s->img_n; ++i) {
        if (j->img_comp[i].data) {
            free (j->img_comp[i].raw_data);
            j->img_comp[i].data = NULL;
        }
        if (j->img_comp[i].linebuf) {
            free (j->img_comp[i].linebuf);
            j->img_comp[i].linebuf = NULL;
        }
    }
}


typedef struct {
    Ez_resample_row_func resample;
    Ez_uint8 *line0, *line1;
    int hs, vs;   /* expansion factor in each axis */
    int w_lores;  /* horizontal pixels pre-expansion  */
    int ystep;    /* how far through vertical expansion we are */
    int ypos;     /* which pre-expansion row we're on */
} Ez_stbi_resample;


Ez_uint8 *ez_jpeg_load_image (Ez_jpeg *z, int *out_x, int *out_y, int *comp,
    int req_comp)
{
    int n, decode_n;
    /* Validate req_comp */
    if (req_comp < 0 || req_comp > 4) {
        ez_error ("ez_jpeg_load_image: internal error: bad req_comp\n");
        return NULL;
    }
    z->s->img_n = 0;

    /* Load a jpeg image from whichever source */
    if (!ez_jpeg_decode_image (z)) { ez_jpeg_cleanup (z); return NULL; }

    /* Determine actual number of components to generate */
    n = req_comp ? req_comp : z->s->img_n;

    if (z->s->img_n == 3 && n < 3)
        decode_n = 1;
    else
        decode_n = z->s->img_n;

    /* Resample and color-convert */
    {
        int k;
        Ez_uint i, j;
        Ez_uint8 *output;
        Ez_uint8 *coutput[4];

        Ez_stbi_resample res_comp[4];

        for (k=0; k < decode_n; ++k) {
            Ez_stbi_resample *r = &res_comp[k];

            /* Allocate line buffer big enough for upsampling off the edges
               with upsample factor of 4 */
            z->img_comp[k].linebuf = malloc (z->s->img_x + 3);
            if (!z->img_comp[k].linebuf) {
                ez_jpeg_cleanup (z);
                ez_error ("ez_jpeg_load_image: out of memory\n");
                return NULL;
            }

            r->hs      = z->img_h_max / z->img_comp[k].h;
            r->vs      = z->img_v_max / z->img_comp[k].v;
            r->ystep   = r->vs >> 1;
            r->w_lores = (z->s->img_x + r->hs-1) / r->hs;
            r->ypos    = 0;
            r->line0   = r->line1 = z->img_comp[k].data;

            if      (r->hs == 1 && r->vs == 1) r->resample = ez_jpeg_resample_row_1;
            else if (r->hs == 1 && r->vs == 2) r->resample = ez_jpeg_resample_row_v_2;
            else if (r->hs == 2 && r->vs == 1) r->resample = ez_jpeg_resample_row_h_2;
            else if (r->hs == 2 && r->vs == 2) r->resample = ez_jpeg_resample_row_hv_2;
            else                               r->resample = ez_jpeg_resample_row_generic;
        }

        /* can't error after this so, this is safe */
        output = malloc (n * z->s->img_x * z->s->img_y + 1);
        if (!output) {
            ez_jpeg_cleanup (z);
            ez_error ("ez_jpeg_load_image: out of memory\n");
            return NULL;
        }

        /* Now go ahead and resample */
        for (j=0; j < z->s->img_y; ++j) {
            Ez_uint8 *out = output + n * z->s->img_x * j;
            for (k=0; k < decode_n; ++k) {
                Ez_stbi_resample *r = &res_comp[k];
                int y_bot = r->ystep >= (r->vs >> 1);
                coutput[k] = r->resample (z->img_comp[k].linebuf,
                                          y_bot ? r->line1 : r->line0,
                                          y_bot ? r->line0 : r->line1,
                                          r->w_lores, r->hs);
                if (++r->ystep >= r->vs) {
                    r->ystep = 0;
                    r->line0 = r->line1;
                    if (++r->ypos < z->img_comp[k].y)
                        r->line1 += z->img_comp[k].w2;
                }
            }
            if (n >= 3) {
                Ez_uint8 *y = coutput[0];
                if (z->s->img_n == 3) {
                    ez_jpeg_YCbCr_to_RGB_row (out, y, coutput[1], coutput[2],
                        z->s->img_x, n);
                } else
                    for (i=0; i < z->s->img_x; ++i) {
                        out[0] = out[1] = out[2] = y[i];
                        out[3] = 255; /* not used if n==3 */
                        out += n;
                    }
            } else {
                Ez_uint8 *y = coutput[0];
                if (n == 1)
                    for (i=0; i < z->s->img_x; ++i) out[i] = y[i];
                else
                    for (i=0; i < z->s->img_x; ++i) *out++ = y[i], *out++ = 255;
            }
        }
        ez_jpeg_cleanup (z);
        *out_x = z->s->img_x;
        *out_y = z->s->img_y;
        if (comp) *comp  = z->s->img_n; /* report original components, not output */
        return output;
    }
}


Ez_uint8 *ez_stbi_jpeg_load (Ez_stbi *s, int *x, int *y, int *comp, int req_comp)
{
    Ez_jpeg j;
    j.s = s;
    return ez_jpeg_load_image (&j, x, y, comp, req_comp);
}


int ez_stbi_jpeg_test (Ez_stbi *s)
{
    int r;
    Ez_jpeg j;
    j.s = s;
    r = ez_jpeg_decode_header (&j, EZ_SCAN_TYPE, 0);  /* 0 for silent */
    ez_stbi_rewind (s);
    return r;
}


int ez_stbi_jpeg_info_raw (Ez_jpeg *j, int *x, int *y, int *comp)
{
    if (!ez_jpeg_decode_header (j, EZ_SCAN_HEADER, 1)) {
        ez_stbi_rewind ( j->s );
        return 0;
    }
    if (x) *x = j->s->img_x;
    if (y) *y = j->s->img_y;
    if (comp) *comp = j->s->img_n;
    return 1;
}


int ez_stbi_jpeg_info (Ez_stbi *s, int *x, int *y, int *comp)
{
    Ez_jpeg j;
    j.s = s;
    return ez_stbi_jpeg_info_raw (&j, x, y, comp);
}


/*
 * Public domain zlib decode    v0.2  Sean Barrett 2006-11-18
 *   Simple implementation:
 *    - all input must be provided in an upfront buffer
 *    - all output is written to a single output buffer (can malloc/realloc)
 *   Performance:
 *    - fast huffman
*/

/* fast-way is faster to check than jpeg huffman, but slow way is slower */
#define EZ_ZFAST_BITS  9 /* accelerate all cases in default tables */
#define EZ_ZFAST_MASK  ((1 << EZ_ZFAST_BITS) - 1)


/* zlib-style huffman encoding */
/* (jpegs packs from left, zlib from right, so can't share code) */

typedef struct {
    Ez_uint16 fast[1 << EZ_ZFAST_BITS];
    Ez_uint16 firstcode[16];
    int maxcode[17];
    Ez_uint16 firstsymbol[16];
    Ez_uint8  size[288];
    Ez_uint16 value[288];
} Ez_zhuffman;


EZ_INLINE int ez_zlib_bitreverse16 (int n)
{
    n = ((n & 0xAAAA) >>  1) | ((n & 0x5555) << 1);
    n = ((n & 0xCCCC) >>  2) | ((n & 0x3333) << 2);
    n = ((n & 0xF0F0) >>  4) | ((n & 0x0F0F) << 4);
    n = ((n & 0xFF00) >>  8) | ((n & 0x00FF) << 8);
    return n;
}


EZ_INLINE int ez_zlib_bit_reverse (int v, int bits)
{
    if (bits > 16) {
        ez_error ("ez_zlib_bit_reverse: bits %d > 16\n", bits);
        return 0;
    }
    /* To bit reverse n bits, reverse 16 and shift
       e.g. 11 bits, bit reverse and shift away 5 */
    return ez_zlib_bitreverse16 (v) >> (16-bits);
}


int ez_zlib_build_huffman (Ez_zhuffman *z, Ez_uint8 *sizelist, int num)
{
    int i, k=0;
    int code, next_code[16], sizes[17];

    /* DEFLATE spec for generating codes */
    memset (sizes, 0, sizeof (sizes));
    memset (z->fast, 255, sizeof (z->fast));
    for (i=0; i < num; ++i)
        ++sizes[sizelist[i]];
    sizes[0] = 0;
    for (i=1; i < 16; ++i)
        if (sizes[i] > (1 << i)) {
            ez_error ("ez_zlib_build_huffman: internal error size[%d] = %d\n",
                i, sizes[i]);
            return 0;
        }
    code = 0;
    for (i=1; i < 16; ++i) {
        next_code[i] = code;
        z->firstcode[i] = (Ez_uint16) code;
        z->firstsymbol[i] = (Ez_uint16) k;
        code = (code + sizes[i]);
        if (sizes[i])
            if (code-1 >= (1 << i)) {
                ez_error ("ez_zlib_build_huffman: corrupt JPEG: bad codelengths\n");
            return 0;
            }
        z->maxcode[i] = code << (16-i); /* preshift for inner loop */
        code <<= 1;
        k += sizes[i];
    }
    z->maxcode[16] = 0x10000; /* sentinel */
    for (i=0; i < num; ++i) {
        int s = sizelist[i];
        if (s) {
            int c = next_code[s] - z->firstcode[s] + z->firstsymbol[s];
            z->size[c] = (Ez_uint8)s;
            z->value[c] = (Ez_uint16)i;
            if (s <= EZ_ZFAST_BITS) {
                int k = ez_zlib_bit_reverse (next_code[s], s);
                while (k < (1 << EZ_ZFAST_BITS)) {
                    z->fast[k] = (Ez_uint16) c;
                    k += (1 << s);
                }
            }
            ++next_code[s];
        }
    }
    return 1;
}


/*
 * Zlib-from-memory implementation for PNG reading
*
 * Because PNG allows splitting the zlib stream arbitrarily,
 * and it's annoying structurally to have PNG call ZLIB call PNG,
 * we require PNG read all the IDATs and combine them into a single
 * memory buffer.
*/

typedef struct {
    Ez_uint8 *zbuffer, *zbuffer_end;
    int num_bits;
    Ez_uint32 code_buffer;

    char *zout;
    char *zout_start;
    char *zout_end;
    int   z_expandable;

    Ez_zhuffman z_length, z_distance;
} Ez_zbuf;


EZ_INLINE int ez_zlib_get8 (Ez_zbuf *z)
{
    if (z->zbuffer >= z->zbuffer_end) return 0;
    return *z->zbuffer++;
}


void ez_zlib_fill_bits (Ez_zbuf *z)
{
    do {
        if (z->code_buffer >= (1U << z->num_bits)) {
            ez_error ("ez_zlib_fill_bits: internal error for numbits = %s\n",
                z->num_bits);
            return;
        }
        z->code_buffer |= ez_zlib_get8 (z) << z->num_bits;
        z->num_bits += 8;
    } while (z->num_bits <= 24);
}


EZ_INLINE unsigned int ez_zlib_receive (Ez_zbuf *z, int n)
{
    unsigned int k;
    if (z->num_bits < n) ez_zlib_fill_bits (z);
    k = z->code_buffer & ((1 << n) - 1);
    z->code_buffer >>= n;
    z->num_bits -= n;
    return k;
}


EZ_INLINE int ez_zlib_huffman_decode (Ez_zbuf *a, Ez_zhuffman *z)
{
    int b, s, k;
    if (a->num_bits < 16) ez_zlib_fill_bits (a);
    b = z->fast[a->code_buffer & EZ_ZFAST_MASK];
    if (b < 0xffff) {
        s = z->size[b];
        a->code_buffer >>= s;
        a->num_bits -= s;
        return z->value[b];
    }

    /* Not resolved by fast table, so compute it the slow way
       use jpeg approach, which requires MSbits at top */
    k = ez_zlib_bit_reverse (a->code_buffer, 16);
    for (s=EZ_ZFAST_BITS+1; ; ++s)
        if (k < z->maxcode[s])
            break;
    if (s == 16) return -1; /* invalid code! */
    /* code size is s, so: */
    b = (k >> (16-s)) - z->firstcode[s] + z->firstsymbol[s];
    if (z->size[b] != s) {
        ez_error ("ez_zlib_huffman_decode: internal error\n");
        return 0;
    }
    a->code_buffer >>= s;
    a->num_bits -= s;
    return z->value[b];
}


int ez_zlib_expand (Ez_zbuf *z, int n)  /* need to make room for n bytes */
{
    char *q;
    int cur, limit;
    if (!z->z_expandable) {
        ez_error ("ez_zlib_expand: corrupt PNG: output buffer limit\n");
        return 0;
    }
    cur   = (int) (z->zout     - z->zout_start);
    limit = (int) (z->zout_end - z->zout_start);
    while (cur + n > limit)
        limit *= 2;
    q = realloc (z->zout_start, limit);
    if (q == NULL)  {
        ez_error ("ez_zlib_expand: out of memory\n");
        return 0;
    }
    z->zout_start = q;
    z->zout       = q + cur;
    z->zout_end   = q + limit;
    return 1;
}


int length_base[31] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13,
    15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
    67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0 };

int length_extra[31] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 0, 0 };

int dist_base[32] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129,
    193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289,
    16385, 24577, 0, 0};

int dist_extra[32] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7,
    8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};


int ez_zlib_parse_huffman_block (Ez_zbuf *a)
{
    for (;;) {
        int z = ez_zlib_huffman_decode (a, &a->z_length);
        if (z < 256) {
            if (z < 0) {
                ez_error ("ez_zlib_parse_huffman_block: corrupt PNG: bad huffman code\n");
                return 0;
            }
            if (a->zout >= a->zout_end) if (!ez_zlib_expand (a, 1)) return 0;
            *a->zout++ = (char) z;
        } else {
            Ez_uint8 *p;
            int len, dist;
            if (z == 256) return 1;
            z -= 257;
            len = length_base[z];
            if (length_extra[z]) len += ez_zlib_receive (a, length_extra[z]);
            z = ez_zlib_huffman_decode (a, &a->z_distance);
            if (z < 0) {
                ez_error ("ez_zlib_parse_huffman_block: corrupt PNG: bad huffman code\n");
                return 0;
            }
            dist = dist_base[z];
            if (dist_extra[z]) dist += ez_zlib_receive (a, dist_extra[z]);
            if (a->zout - a->zout_start < dist) {
                ez_error ("ez_zlib_parse_huffman_block: corrupt PNG: bad dist\n");
                return 0;
            }
            if (a->zout + len > a->zout_end) if (!ez_zlib_expand (a, len)) return 0;
            p = (Ez_uint8 *) (a->zout - dist);
            while (len--)
                *a->zout++ = *p++;
        }
    }
}


int ez_zlib_compute_huffman_codes (Ez_zbuf *a)
{
    Ez_uint8 length_dezigzag[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12,
        3, 13, 2, 14, 1, 15 };
    Ez_zhuffman z_codelength;
    Ez_uint8 lencodes[286+32+137];/* padding for maximum single op */
    Ez_uint8 codelength_sizes[19];
    int i, n;

    int hlit  = ez_zlib_receive (a, 5) + 257;
    int hdist = ez_zlib_receive (a, 5) + 1;
    int hclen = ez_zlib_receive (a, 4) + 4;

    memset (codelength_sizes, 0, sizeof (codelength_sizes));
    for (i=0; i < hclen; ++i) {
        int s = ez_zlib_receive (a, 3);
        codelength_sizes[length_dezigzag[i]] = (Ez_uint8) s;
    }
    if (!ez_zlib_build_huffman (&z_codelength, codelength_sizes, 19)) return 0;

    n = 0;
    while (n < hlit + hdist) {
        int c = ez_zlib_huffman_decode (a, &z_codelength);
        if (c < 0 || c >= 19) {
            ez_error ("ez_zlib_compute_huffman_codes: internal error c = %d\n", c);
            return 0;
        }
        if (c < 16)
            lencodes[n++] = (Ez_uint8) c;
        else if (c == 16) {
            c = ez_zlib_receive (a, 2)+3;
            memset (lencodes+n, lencodes[n-1], c);
            n += c;
        } else if (c == 17) {
            c = ez_zlib_receive (a, 3)+3;
            memset (lencodes+n, 0, c);
            n += c;
        } else {
            if (c != 18) {
                ez_error ("ez_zlib_compute_huffman_codes: internal error c = %d\n", c);
                return 0;
            }
            c = ez_zlib_receive (a, 7)+11;
            memset (lencodes+n, 0, c);
            n += c;
        }
    }
    if (n != hlit+hdist) {
        ez_error ("ez_zlib_compute_huffman_codes: corrupt PNG: bad code lengths\n");
        return 0;
    }
    if (!ez_zlib_build_huffman (&a->z_length, lencodes, hlit)) return 0;
    if (!ez_zlib_build_huffman (&a->z_distance, lencodes+hlit, hdist)) return 0;
    return 1;
}


int ez_zlib_parse_uncompressed_block (Ez_zbuf *a)
{
    Ez_uint8 header[4];
    int len, nlen, k;
    if (a->num_bits & 7)
        ez_zlib_receive (a, a->num_bits & 7); /* discard */
    /* Drain the bit-packed data into header */
    k = 0;
    while (a->num_bits > 0) {
        header[k++] = (Ez_uint8) (a->code_buffer & 255); /* wtf this warns? */
        a->code_buffer >>= 8;
        a->num_bits -= 8;
    }
    if (a->num_bits != 0) {
        ez_error ("ez_zlib_parse_uncompressed_block: internal error num_bits = %d\n",
            a->num_bits);
        return 0;
    }
    /* Now fill header the normal way */
    while (k < 4)
        header[k++] = (Ez_uint8) ez_zlib_get8 (a);
    len  = header[1] * 256 + header[0];
    nlen = header[3] * 256 + header[2];
    if (nlen != (len ^ 0xffff)) {
        ez_error ("ez_zlib_parse_uncompressed_block: corrupt PNG: zlib corrupt\n");
        return 0;
    }
    if (a->zbuffer + len > a->zbuffer_end) {
        ez_error ("ez_zlib_parse_uncompressed_block: corrupt PNG: read past buffer\n");
        return 0;
    }
    if (a->zout + len > a->zout_end)
        if (!ez_zlib_expand (a, len)) return 0;
    memcpy (a->zout, a->zbuffer, len);
    a->zbuffer += len;
    a->zout += len;
    return 1;
}


int ez_zlib_parse_header (Ez_zbuf *a)
{
    int cmf   = ez_zlib_get8 (a);
    int cm    = cmf & 15;
    /* int cinfo = cmf >> 4; */
    int flg   = ez_zlib_get8 (a);
    if ((cmf*256+flg) % 31 != 0) { /* zlib spec */
        ez_error ("ez_zlib_parse_header: corrupt PNG: bad zlib header\n");
        return 0;
    }
    if (flg & 32) { /* preset dictionary not allowed in png */
        ez_error ("ez_zlib_parse_header: corrupt PNG: no preset dict\n");
        return 0;
    }
    if (cm != 8)  { /* DEFLATE required for png */
        ez_error ("ez_zlib_parse_header: corrupt PNG: bad compression\n");
        return 0;
    }

    /* window = 1 << (8 + cinfo)... but who cares, we fully buffer output */
    return 1;
}


/* @TODO: should statically initialize these for optimal thread safety */
Ez_uint8 default_length[288], default_distance[32];

void ez_zlib_init_defaults (void)
{
    int i;   /* use <= to match clearly with spec */
    for (i=0; i <= 143; ++i)     default_length[i]   = 8;
    for (   ; i <= 255; ++i)     default_length[i]   = 9;
    for (   ; i <= 279; ++i)     default_length[i]   = 7;
    for (   ; i <= 287; ++i)     default_length[i]   = 8;

    for (i=0; i <=  31; ++i)     default_distance[i] = 5;
}


/* A quick hack to only allow decoding some of a PNG... I should implement
   real streaming support instead */
int ez_stbi_png_partial;

int ez_zlib_parse (Ez_zbuf *a, int parse_header)
{
    int final, type;
    if (parse_header)
        if (!ez_zlib_parse_header (a)) return 0;
    a->num_bits = 0;
    a->code_buffer = 0;
    do {
        final = ez_zlib_receive (a, 1);
        type  = ez_zlib_receive (a, 2);
        if (type == 0) {
            if (!ez_zlib_parse_uncompressed_block (a)) return 0;
        } else if (type == 3) {
            return 0;
        } else {
            if (type == 1) {
                /* use fixed code lengths */
                if (!default_distance[31]) ez_zlib_init_defaults ();
                if (!ez_zlib_build_huffman (&a->z_length  , default_length  , 288))
                    return 0;
                if (!ez_zlib_build_huffman (&a->z_distance, default_distance,  32))
                    return 0;
            } else {
                if (!ez_zlib_compute_huffman_codes (a)) return 0;
            }
            if (!ez_zlib_parse_huffman_block (a)) return 0;
        }
        if (ez_stbi_png_partial && a->zout - a->zout_start > 65536)
            break;
    } while (!final);
    return 1;
}


int ez_zlib_do (Ez_zbuf *a, char *obuf, int olen, int exp, int parse_header)
{
    a->zout_start = obuf;
    a->zout       = obuf;
    a->zout_end   = obuf + olen;
    a->z_expandable = exp;

    return ez_zlib_parse (a, parse_header);
}

char *ez_stbi_zlib_decode_malloc_guesssize (const char *buffer, int len,
    int initial_size, int *outlen)
{
    Ez_zbuf a;
    char *p = malloc (initial_size);
    if (p == NULL) return NULL;
    a.zbuffer = (Ez_uint8 *) buffer;
    a.zbuffer_end = (Ez_uint8 *) buffer + len;
    if (ez_zlib_do (&a, p, initial_size, 1, 1)) {
        if (outlen) *outlen = (int) (a.zout - a.zout_start);
        return a.zout_start;
    } else {
        free (a.zout_start);
        return NULL;
    }
}

char *ez_stbi_zlib_decode_malloc (char const *buffer, int len, int *outlen)
{
    return ez_stbi_zlib_decode_malloc_guesssize (buffer, len, 16384, outlen);
}


char *ez_stbi_zlib_decode_malloc_guesssize_headerflag (const char *buffer,
    int len, int initial_size, int *outlen, int parse_header)
{
    Ez_zbuf a;
    char *p = malloc (initial_size);
    if (p == NULL) return NULL;
    a.zbuffer = (Ez_uint8 *) buffer;
    a.zbuffer_end = (Ez_uint8 *) buffer + len;
    if (ez_zlib_do (&a, p, initial_size, 1, parse_header)) {
        if (outlen) *outlen = (int) (a.zout - a.zout_start);
        return a.zout_start;
    } else {
        free (a.zout_start);
        return NULL;
    }
}


int ez_stbi_zlib_decode_buffer (char *obuffer, int olen, char const *ibuffer,
    int ilen)
{
    Ez_zbuf a;
    a.zbuffer = (Ez_uint8 *) ibuffer;
    a.zbuffer_end = (Ez_uint8 *) ibuffer + ilen;
    if (ez_zlib_do (&a, obuffer, olen, 0, 1))
        return (int) (a.zout - a.zout_start);
    else
        return -1;
}


char *ez_stbi_zlib_decode_noheader_malloc (char const *buffer, int len,
    int *outlen)
{
    Ez_zbuf a;
    char *p = malloc (16384);
    if (p == NULL) return NULL;
    a.zbuffer = (Ez_uint8 *) buffer;
    a.zbuffer_end = (Ez_uint8 *) buffer+len;
    if (ez_zlib_do (&a, p, 16384, 1, 0)) {
        if (outlen) *outlen = (int) (a.zout - a.zout_start);
        return a.zout_start;
    } else {
        free (a.zout_start);
        return NULL;
    }
}


int ez_stbi_zlib_decode_noheader_buffer (char *obuffer, int olen,
    const char *ibuffer, int ilen)
{
    Ez_zbuf a;
    a.zbuffer = (Ez_uint8 *) ibuffer;
    a.zbuffer_end = (Ez_uint8 *) ibuffer + ilen;
    if (ez_zlib_do (&a, obuffer, olen, 0, 0))
        return (int) (a.zout - a.zout_start);
    else
        return -1;
}


/*
 * Public domain "baseline" PNG decoder   v0.10  Sean Barrett 2006-11-18
 *
 * Simple implementation:
 *   - only 8-bit samples
 *   - no CRC checking
 *   - allocates lots of intermediate memory
 *   - avoids problem of streaming data between subsystems
 *   - avoids explicit window management
 * Performance:
 *   - uses stb_zlib, a PD zlib implementation with fast huffman decoding
*/

typedef struct {
    Ez_uint32 length;
    Ez_uint32 type;
} Ez_chunk;

#define EZ_PNG_TYPE(a,b,c,d)  (((a) << 24) + ((b) << 16) + ((c) << 8) + (d))


Ez_chunk ez_png_get_chunk_header (Ez_stbi *s)
{
    Ez_chunk c;
    c.length = ez_buffer_get32 (s);
    c.type   = ez_buffer_get32 (s);
    return c;
}


int ez_png_check_header (Ez_stbi *s, int verbose)
{
    static Ez_uint8 png_sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    int i;
    for (i=0; i < 8; ++i)
        if (ez_buffer_get8u (s) != png_sig[i]) {
            if (verbose) ez_error ("ez_png_check_header: not a PNG: bad signature\n");
            return 0;
        }
    return 1;
}


typedef struct {
    Ez_stbi *s;
    Ez_uint8 *idata, *expanded, *out;
} Ez_png;


enum {
    EZ_F_NONE = 0, EZ_F_SUB = 1, EZ_F_UP = 2, EZ_F_AVG = 3, EZ_F_PAETH = 4,
    EZ_F_AVG_FIRST, EZ_F_PAETH_FIRST
};

Ez_uint8 first_row_filter[5] = {
    EZ_F_NONE, EZ_F_SUB, EZ_F_NONE, EZ_F_AVG_FIRST, EZ_F_PAETH_FIRST
};


int ez_png_paeth (int a, int b, int c)
{
    int p = a + b - c;
    int pa = abs (p-a);
    int pb = abs (p-b);
    int pc = abs (p-c);
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}


/* Create the png data from post-deflated data */

int ez_png_create_image_raw (Ez_png *a, Ez_uint8 *raw, Ez_uint32 raw_len,
    int out_n, Ez_uint32 x, Ez_uint32 y)
{
    Ez_stbi *s = a->s;
    Ez_uint32 i, j, stride = x*out_n;
    int k;
    int img_n = s->img_n; /* copy it into a local for later */
    if (out_n != s->img_n && out_n != s->img_n+1) {
        ez_error ("ez_png_create_image_raw: internal error out_n = %d\n", out_n);
        return 0;
    }
    if (ez_stbi_png_partial) y = 1;
    a->out = malloc (x * y * out_n);
    if (!a->out) {
        ez_error ("ez_png_create_image_raw: out of memory\n");
        return 0;
    }
    if (!ez_stbi_png_partial) {
        if (s->img_x == x && s->img_y == y) {
            if (raw_len != (img_n * x + 1) * y) {
                ez_error ("ez_png_create_image_raw: corrupt PNG: not enough pixels\n");
            return 0;
            }
        } else { /* interlaced: */
            if (raw_len < (img_n * x + 1) * y) {
                ez_error ("ez_png_create_image_raw: corrupt PNG: not enough pixels\n");
                return 0;
            }
        }
    }
    for (j=0; j < y; ++j) {
        Ez_uint8 *cur = a->out + stride*j;
        Ez_uint8 *prior = cur - stride;
        int filter = *raw++;
        if (filter > 4) {
            ez_error ("ez_png_create_image_raw: corrupt PNG: invalid filter %d\n",
                filter);
            return 0;
        }
        /* If first row, use special filter that doesn't sample previous row */
        if (j == 0) filter = first_row_filter[filter];
        /* Handle first pixel explicitly */
        for (k=0; k < img_n; ++k) {
            switch (filter) {
                case EZ_F_NONE       : cur[k] = raw[k]; break;
                case EZ_F_SUB        : cur[k] = raw[k]; break;
                case EZ_F_UP         : cur[k] = raw[k] + prior[k]; break;
                case EZ_F_AVG        : cur[k] = raw[k] + (prior[k]>>1); break;
                case EZ_F_PAETH      : cur[k] = (Ez_uint8) (raw[k] + ez_png_paeth (
                                            0, prior[k], 0)); break;
                case EZ_F_AVG_FIRST  : cur[k] = raw[k]; break;
                case EZ_F_PAETH_FIRST: cur[k] = raw[k]; break;
            }
        }
        if (img_n != out_n) cur[img_n] = 255;
        raw += img_n;
        cur += out_n;
        prior += out_n;
        /* This is a little gross, so that we don't switch per-pixel or per-
           component */
        if (img_n == out_n) {
            #define CASE(f) \
                case f:     \
                    for (i=x-1; i >= 1; --i, raw+=img_n, cur+=img_n, prior+=img_n) \
                        for (k=0; k < img_n; ++k)
            switch (filter) {
                CASE(EZ_F_NONE)
                    cur[k] = raw[k]; break;
                CASE(EZ_F_SUB)
                    cur[k] = raw[k] + cur[k-img_n]; break;
                CASE(EZ_F_UP)
                    cur[k] = raw[k] + prior[k]; break;
                CASE(EZ_F_AVG)
                    cur[k] = raw[k] + ((prior[k] + cur[k-img_n])>>1); break;
                CASE(EZ_F_PAETH)
                    cur[k] = (Ez_uint8) (raw[k] + ez_png_paeth (
                        cur[k-img_n], prior[k], prior[k-img_n])); break;
                CASE(EZ_F_AVG_FIRST)
                    cur[k] = raw[k] + (cur[k-img_n] >> 1); break;
                CASE(EZ_F_PAETH_FIRST)
                    cur[k] = (Ez_uint8) (raw[k] + ez_png_paeth (
                        cur[k-img_n], 0, 0)); break;
            }
            #undef CASE
        } else {
            if (img_n+1 != out_n) {
                ez_error ("ez_png_create_image_raw: internal error out_n = %d\n",
                    out_n);
                return 0;
            }
            #define CASE(f) \
                case f:     \
                    for (i=x-1; i >= 1; --i, cur[img_n]=255, raw+=img_n, \
                        cur+=out_n, prior+=out_n) for (k=0; k < img_n; ++k)
            switch (filter) {
                CASE(EZ_F_NONE)
                    cur[k] = raw[k]; break;
                CASE(EZ_F_SUB)
                    cur[k] = raw[k] + cur[k-out_n]; break;
                CASE(EZ_F_UP)
                    cur[k] = raw[k] + prior[k]; break;
                CASE(EZ_F_AVG)
                    cur[k] = raw[k] + ((prior[k] + cur[k-out_n])>>1); break;
                CASE(EZ_F_PAETH)
                    cur[k] = (Ez_uint8) (raw[k] + ez_png_paeth (cur[k-out_n],
                        prior[k], prior[k-out_n])); break;
                CASE(EZ_F_AVG_FIRST)
                    cur[k] = raw[k] + (cur[k-out_n] >> 1); break;
                CASE(EZ_F_PAETH_FIRST)
                    cur[k] = (Ez_uint8) (raw[k] + ez_png_paeth (cur[k-out_n], 0, 0));
                    break;
            }
            #undef CASE
        }
    }
    return 1;
}


int ez_png_create_image (Ez_png *a, Ez_uint8 *raw, Ez_uint32 raw_len, int out_n,
    int interlaced)
{
    Ez_uint8 *final;
    int p;
    int save;
    if (!interlaced)
        return ez_png_create_image_raw (a, raw, raw_len, out_n, a->s->img_x,
            a->s->img_y);
    save = ez_stbi_png_partial;
    ez_stbi_png_partial = 0;

    /* De-interlacing */
    final = malloc (a->s->img_x * a->s->img_y * out_n);
    for (p=0; p < 7; ++p) {
        int xorig[] = { 0, 4, 0, 2, 0, 1, 0 };
        int yorig[] = { 0, 0, 4, 0, 2, 0, 1 };
        int xspc[]  = { 8, 8, 4, 4, 2, 2, 1 };
        int yspc[]  = { 8, 8, 8, 4, 4, 2, 2 };
        int i, j, x, y;
        /* pass1_x[4] = 0, pass1_x[5] = 1, pass1_x[12] = 1 */
        x = (a->s->img_x - xorig[p] + xspc[p]-1) / xspc[p];
        y = (a->s->img_y - yorig[p] + yspc[p]-1) / yspc[p];
        if (x && y) {
            if (!ez_png_create_image_raw (a, raw, raw_len, out_n, x, y)) {
                free (final);
                return 0;
            }
            for (j=0; j < y; ++j)
            for (i=0; i < x; ++i)
                memcpy (final + (j*yspc[p]+yorig[p])*a->s->img_x*out_n +
                    (i*xspc[p]+xorig[p])*out_n, a->out + (j*x+i)*out_n, out_n);
            free (a->out);
            raw += (x*out_n+1)*y;
            raw_len -= (x*out_n+1)*y;
        }
    }
    a->out = final;

    ez_stbi_png_partial = save;
    return 1;
}


int ez_png_compute_transparency (Ez_png *z, Ez_uint8 tc[3], int out_n)
{
    Ez_stbi *s = z->s;
    Ez_uint32 i, pixel_count = s->img_x * s->img_y;
    Ez_uint8 *p = z->out;

    /* Compute color-based transparency, assuming we've
       already got 255 as the alpha value in the output */
    if (out_n != 2 && out_n != 4) {
        ez_error ("ez_png_compute_transparency: internal error out_n = %d\n",
            out_n);
        return 0;
    }

    if (out_n == 2) {
        for (i=0; i < pixel_count; ++i) {
            p[1] = (p[0] == tc[0] ? 0 : 255);
            p += 2;
        }
    } else {
        for (i=0; i < pixel_count; ++i) {
            if (p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2])
                p[3] = 0;
            p += 4;
        }
    }
    return 1;
}


int ez_png_expand_palette (Ez_png *a, Ez_uint8 *palette, int len, int pal_img_n)
{
    Ez_uint32 i, pixel_count = a->s->img_x * a->s->img_y;
    Ez_uint8 *p, *temp_out, *orig = a->out;

    p = malloc (pixel_count * pal_img_n);
    if (p == NULL) {
        ez_error ("ez_png_expand_palette: out of memory\n");
        return 0;
    }

    /* Between here and free (out) below, exitting would leak */
    temp_out = p;

    if (pal_img_n == 3) {
        for (i=0; i < pixel_count; ++i) {
            int n = orig[i]*4;
            p[0] = palette[n  ];
            p[1] = palette[n+1];
            p[2] = palette[n+2];
            p += 3;
        }
    } else {
        for (i=0; i < pixel_count; ++i) {
            int n = orig[i]*4;
            p[0] = palette[n  ];
            p[1] = palette[n+1];
            p[2] = palette[n+2];
            p[3] = palette[n+3];
            p += 4;
        }
    }
    free (a->out);
    a->out = temp_out;

    (void) len;

    return 1;
}


int ez_png_parse_file (Ez_png *z, int scan, int req_comp)
{
    Ez_uint8 palette[1024], pal_img_n=0;
    Ez_uint8 has_trans=0, tc[3];
    Ez_uint32 ioff=0, idata_limit=0, i, pal_len=0;
    int first=1, k, interlace=0;
    Ez_stbi *s = z->s;

    z->expanded = NULL;
    z->idata = NULL;
    z->out = NULL;

    if (!ez_png_check_header (s, 1)) return 0;

    if (scan == EZ_SCAN_TYPE) return 1;

    for (;;) {
        Ez_chunk c = ez_png_get_chunk_header (s);
        switch (c.type) {
            case EZ_PNG_TYPE ('C', 'g', 'B', 'I'):
                ez_buffer_skip (s, c.length);
                break;
            case EZ_PNG_TYPE ('I', 'H', 'D', 'R'): {
                int depth, color, comp, filter;
                if (!first) {
                    ez_error ("ez_png_parse_file: corrupt PNG: multiple IHDR\n");
                    return 0;
                }
                first = 0;
                if (c.length != 13) {
                    ez_error ("ez_png_parse_file: corrupt PNG: bad IHDR len\n");
                    return 0;
                }
                s->img_x = ez_buffer_get32 (s); if (s->img_x > (1 << 24)) {
                    ez_error ("ez_png_parse_file: corrupt PNG: too large image\n");
                    return 0;
                }
                s->img_y = ez_buffer_get32 (s); if (s->img_y > (1 << 24)) {
                    ez_error ("ez_png_parse_file: corrupt PNG: too large image\n");
                    return 0;
                }
                depth = ez_buffer_get8 (s); if (depth != 8) {
                    ez_error ("ez_png_parse_file: PNG not supported: 8-bit only\n");
                    return 0;
                }
                color = ez_buffer_get8 (s); if (color > 6) {
                    ez_error ("ez_png_parse_file: corrupt PNG: bad ctype\n");
                    return 0;
                }
                if (color == 3) pal_img_n = 3; else if (color & 1) {
                    ez_error ("ez_png_parse_file: corrupt PNG: bad ctype\n");
                    return 0;
                }
                comp  = ez_buffer_get8 (s); if (comp) {
                    ez_error ("ez_png_parse_file: corrupt PNG: bad comp method\n");
                    return 0;
                }
                filter= ez_buffer_get8 (s);  if (filter) {
                    ez_error ("ez_png_parse_file: corrupt PNG: bad filter method\n");
                    return 0;
                }
                interlace = ez_buffer_get8 (s); if (interlace>1) {
                    ez_error ("ez_png_parse_file: corrupt PNG: bad interlace method\n");
                return 0;
                }
                if (!s->img_x || !s->img_y) {
                    ez_error ("ez_png_parse_file: corrupt PNG: 0 pixel image\n");
                    return 0;
                }
                if (!pal_img_n) {
                    s->img_n = (color & 2 ? 3 : 1) + (color & 4 ? 1 : 0);
                    if ((1 << 30) / s->img_x / s->img_n < s->img_y) {
                        ez_error ("ez_png_parse_file: PNG image too large to decode\n");
                        return 0;
                    }
                    if (scan == EZ_SCAN_HEADER) return 1;
                } else {
                    /* If paletted, then pal_n is our final components, and
                       img_n is # components to decompress/filter. */
                    s->img_n = 1;
                    if ((1 << 30) / s->img_x / 4 < s->img_y) {
                        ez_error ("ez_png_parse_file: corrupt PNG: too large\n");
                        return 0;
                    }
                    /* if EZ_SCAN_HEADER, have to scan to see if we have a tRNS */
                }
                break;
            }

            case EZ_PNG_TYPE ('P', 'L', 'T', 'E'):  {
                if (first) {
                    ez_error ("ez_png_parse_file: corrupt PNG: first not IHDR\n");
                    return 0;
                }
                if (c.length > 256*3) {
                    ez_error ("ez_png_parse_file: corrupt PNG: invalid PLTE\n");
                    return 0;
                }
                pal_len = c.length / 3;
                if (pal_len * 3 != c.length) {
                    ez_error ("ez_png_parse_file: corrupt PNG: invalid PLTE\n");
                    return 0;
                }
                for (i=0; i < pal_len; ++i) {
                    palette[i*4+0] = ez_buffer_get8u (s);
                    palette[i*4+1] = ez_buffer_get8u (s);
                    palette[i*4+2] = ez_buffer_get8u (s);
                    palette[i*4+3] = 255;
                }
                break;
            }

            case EZ_PNG_TYPE ('t', 'R', 'N', 'S'): {
                if (first) {
                    ez_error ("ez_png_parse_file: corrupt PNG: first not IHDR\n");
                    return 0;
                }
                if (z->idata) {
                    ez_error ("ez_png_parse_file: corrupt PNG: tRNS after IDAT\n");
                    return 0;
                }
                if (pal_img_n) {
                    if (scan == EZ_SCAN_HEADER) { s->img_n = 4; return 1; }
                    if (pal_len == 0) {
                        ez_error ("ez_png_parse_file: corrupt PNG: tRNS before PLTE\n");
                        return 0;
                    }
                    if (c.length > pal_len) {
                        ez_error ("ez_png_parse_file: corrupt PNG: bad tRNS len\n");
                        return 0;
                    }
                    pal_img_n = 4;
                    for (i=0; i < c.length; ++i)
                        palette[i*4+3] = ez_buffer_get8u (s);
                } else {
                    if (! (s->img_n & 1)) {
                        ez_error ("ez_png_parse_file: corrupt PNG: tRNS with alpha\n");
                        return 0;
                    }
                    if (c.length != (Ez_uint32) s->img_n*2) {
                        ez_error ("ez_png_parse_file: corrupt PNG: bad tRNS len\n");
                        return 0;
                    }
                    has_trans = 1;
                    for (k=0; k < s->img_n; ++k)
                        tc[k] = (Ez_uint8) ez_buffer_get16 (s);
                        /* non 8-bit images will be larger */
                }
                break;
            }

            case EZ_PNG_TYPE ('I', 'D', 'A', 'T'): {
                if (first) {
                    ez_error ("ez_png_parse_file: corrupt PNG: first not IHDR\n");
                    return 0;
                }
                if (pal_img_n && !pal_len) {
                    ez_error ("ez_png_parse_file: corrupt PNG: no PLTE\n");
                    return 0;
                }
                if (scan == EZ_SCAN_HEADER) { s->img_n = pal_img_n; return 1; }
                if (ioff + c.length > idata_limit) {
                    Ez_uint8 *p;
                    if (idata_limit == 0)
                        idata_limit = c.length > 4096 ? c.length : 4096;
                    while (ioff + c.length > idata_limit)
                        idata_limit *= 2;
                    p = realloc (z->idata, idata_limit); if (p == NULL) {
                        ez_error ("ez_png_parse_file: out of memory\n");
                        return 0;
                    }
                    z->idata = p;
                }
                if (!ez_buffer_getn (s, z->idata+ioff, c.length)) {
                    ez_error ("ez_png_parse_file: corrupt PNG: out of data\n");
                    return 0;
                }
                ioff += c.length;
                break;
            }

            case EZ_PNG_TYPE ('I', 'E', 'N', 'D'): {
                Ez_uint32 raw_len;
                if (first) {
                    ez_error ("ez_png_parse_file: corrupt PNG: first not IHDR\n");
                    return 0;
                }
                if (scan != EZ_SCAN_LOAD) return 1;
                if (z->idata == NULL) {
                    ez_error ("ez_png_parse_file: corrupt PNG: no IDAT\n");
                    return 0;
                }
                z->expanded = (Ez_uint8 *)
                    ez_stbi_zlib_decode_malloc_guesssize_headerflag (
                        (char *) z->idata, ioff, 16384, (int *) &raw_len, 1);
                if (z->expanded == NULL) return 0; /* zlib should set error */
                free (z->idata); z->idata = NULL;
                if ((req_comp == s->img_n+1 && req_comp != 3 && !pal_img_n) ||
                    has_trans) s->img_out_n = s->img_n+1;
                else
                    s->img_out_n = s->img_n;
                if (!ez_png_create_image (z, z->expanded, raw_len, s->img_out_n,
                    interlace)) return 0;
                if (has_trans)
                    if (!ez_png_compute_transparency (z, tc, s->img_out_n)) return 0;
                if (pal_img_n) {
                    /* pal_img_n == 3 or 4 */
                    s->img_n = pal_img_n; /* record the actual colors we had */
                    s->img_out_n = pal_img_n;
                    if (req_comp >= 3) s->img_out_n = req_comp;
                    if (!ez_png_expand_palette (z, palette, pal_len, s->img_out_n))
                        return 0;
                }
                free (z->expanded); z->expanded = NULL;
                return 1;
            }

            default:
                /* if critical, fail */
                if (first) {
                    ez_error ("ez_png_parse_file: corrupt PNG: first not IHDR\n");
                    return 0;
                }
                if ((c.type & (1 << 29)) == 0) {
                    ez_error ("ez_png_parse_file: PNG not supported: invalid chunk"
                        " 0x%x\n", c.type);
                    return 0;
                }
                ez_buffer_skip (s, c.length);
                break;
        }
        /* End of chunk, read and skip CRC */
        ez_buffer_get32 (s);
    }
}


Ez_uint8 *ez_png_do (Ez_png *p, int *x, int *y, int *n, int req_comp)
{
    Ez_uint8 *result=NULL;
    if (req_comp < 0 || req_comp > 4) {
        ez_error ("ez_png_do: internal error : bad req_comp\n");
        return NULL;
    }
    if (ez_png_parse_file (p, EZ_SCAN_LOAD, req_comp)) {
        result = p->out;
        p->out = NULL;
        if (req_comp && req_comp != p->s->img_out_n) {
            result = ez_convert_format (result, p->s->img_out_n, req_comp,
                p->s->img_x, p->s->img_y);
            p->s->img_out_n = req_comp;
            if (result == NULL) return result;
        }
        *x = p->s->img_x;
        *y = p->s->img_y;
        if (n) *n = p->s->img_n;
    }
    free (p->out);      p->out      = NULL;
    free (p->expanded); p->expanded = NULL;
    free (p->idata);    p->idata    = NULL;

    return result;
}


Ez_uint8 *ez_stbi_png_load (Ez_stbi *s, int *x, int *y, int *comp, int req_comp)
{
    Ez_png p;
    p.s = s;
    return ez_png_do (&p, x, y, comp, req_comp);
}


int ez_stbi_png_test (Ez_stbi *s)
{
    int r;
    r = ez_png_check_header (s, 0);  /* 0 for silent */
    ez_stbi_rewind (s);
    return r;
}


int ez_stbi_png_info_raw (Ez_png *p, int *x, int *y, int *comp)
{
    if (!ez_png_parse_file (p, EZ_SCAN_HEADER, 0)) {
        ez_stbi_rewind ( p->s );
        return 0;
    }
    if (x) *x = p->s->img_x;
    if (y) *y = p->s->img_y;
    if (comp) *comp = p->s->img_n;
    return 1;
}


int ez_stbi_png_info (Ez_stbi *s, int *x, int *y, int *comp)
{
    Ez_png p;
    p.s = s;
    return ez_stbi_png_info_raw (&p, x, y, comp);
}


/*
 * Microsoft/Windows BMP image
*/

int ez_bmp_test (Ez_stbi *s)
{
    int sz;
    if (ez_buffer_get8 (s) != 'B') return 0;
    if (ez_buffer_get8 (s) != 'M') return 0;
    ez_buffer_get32le (s); /* discard filesize */
    ez_buffer_get16le (s); /* discard reserved */
    ez_buffer_get16le (s); /* discard reserved */
    ez_buffer_get32le (s); /* discard data offset */
    sz = ez_buffer_get32le (s);
    if (sz == 12 || sz == 40 || sz == 56 || sz == 108) return 1;
    return 0;
}


int ez_stbi_bmp_test (Ez_stbi *s)
{
    int r = ez_bmp_test (s);
    ez_stbi_rewind (s);
    return r;
}



/* Returns 0..31 for the highest set bit */

int ez_bmp_highest_bit (unsigned int z)
{
    int n=0;
    if (z == 0) return -1;
    if (z >= 0x10000) n += 16, z >>= 16;
    if (z >= 0x00100) n +=  8, z >>=  8;
    if (z >= 0x00010) n +=  4, z >>=  4;
    if (z >= 0x00004) n +=  2, z >>=  2;
    if (z >= 0x00002) n +=  1, z >>=  1;
    return n;
}


int ez_bmp_bitcount (unsigned int a)
{
    a = (a & 0x55555555) + ((a >>  1) & 0x55555555); /* max 2 */
    a = (a & 0x33333333) + ((a >>  2) & 0x33333333); /* max 4 */
    a = (a + (a >> 4)) & 0x0f0f0f0f; /* max 8 per 4, now 8 bits */
    a = (a + (a >> 8));  /* max 16 per 8 bits */
    a = (a + (a >> 16)); /* max 32 per 8 bits */
    return a & 0xff;
}


int ez_bmp_shiftsigned (int v, int shift, int bits)
{
    int result;
    int z=0;

    if (shift < 0) v <<= -shift;
    else v >>= shift;
    result = v;

    z = bits;
    while (z < 8) {
        result += v >> z;
        z += bits;
    }
    return result;
}


Ez_uint8 *ez_bmp_load (Ez_stbi *s, int *x, int *y, int *comp, int req_comp)
{
    Ez_uint8 *out;
    unsigned int mr=0, mg=0, mb=0, ma=0;
    Ez_uint8 pal[256][4];
    int psize=0, i, j, compress=0, width;
    int bpp, flip_vertically, pad, target, offset, hsz;
    if (ez_buffer_get8 (s) != 'B' || ez_buffer_get8 (s) != 'M') {
        ez_error ("ez_bmp_load: corrupt BMP : not BMP\n");
        return NULL;
    }
    ez_buffer_get32le (s); /* discard filesize */
    ez_buffer_get16le (s); /* discard reserved */
    ez_buffer_get16le (s); /* discard reserved */
    offset = ez_buffer_get32le (s);
    hsz = ez_buffer_get32le (s);
    if (hsz != 12 && hsz != 40 && hsz != 56 && hsz != 108) {
        ez_error ("ez_bmp_load: BMP type not supported: unknown\n");
        return NULL;
    }
    if (hsz == 12) {
        s->img_x = ez_buffer_get16le (s);
        s->img_y = ez_buffer_get16le (s);
    } else {
        s->img_x = ez_buffer_get32le (s);
        s->img_y = ez_buffer_get32le (s);
    }
    if (ez_buffer_get16le (s) != 1) {
        ez_error ("ez_bmp_load: bad BMP\n");
        return NULL;
    }
    bpp = ez_buffer_get16le (s);
    if (bpp == 1) {
        ez_error ("ez_bmp_load: BMP type not supported: 1-bit monochrome\n");
        return NULL;
    }
    flip_vertically = ((int) s->img_y) > 0;
    s->img_y = abs ((int) s->img_y);
    if (hsz == 12) {
        if (bpp < 24)
            psize = (offset - 14 - 24) / 3;
    } else {
        compress = ez_buffer_get32le (s);
        if (compress == 1 || compress == 2) {
            ez_error ("ez_bmp_load: BMP type not supported: RLE\n");
            return NULL;
        }
        ez_buffer_get32le (s); /* discard sizeof */
        ez_buffer_get32le (s); /* discard hres */
        ez_buffer_get32le (s); /* discard vres */
        ez_buffer_get32le (s); /* discard colorsused */
        ez_buffer_get32le (s); /* discard max important */
        if (hsz == 40 || hsz == 56) {
            if (hsz == 56) {
                ez_buffer_get32le (s);
                ez_buffer_get32le (s);
                ez_buffer_get32le (s);
                ez_buffer_get32le (s);
            }
            if (bpp == 16 || bpp == 32) {
                mr = mg = mb = 0;
                if (compress == 0) {
                    if (bpp == 32) {
                        mr = 0xffu << 16;
                        mg = 0xffu <<  8;
                        mb = 0xffu <<  0;
                        ma = 0xffu << 24;
                        /* Fake_a = 1; @TODO: check for cases like alpha value
                           is all 0 and switch it to 255 */
                    } else {
                        mr = 31u << 10;
                        mg = 31u <<  5;
                        mb = 31u <<  0;
                    }
                } else if (compress == 3) {
                    mr = ez_buffer_get32le (s);
                    mg = ez_buffer_get32le (s);
                    mb = ez_buffer_get32le (s);
                    /* Not documented, but generated by photoshop and handled
                        by mspaint */
                    if (mr == mg && mg == mb) {
                        /* ?!?!? */
                        ez_error ("ez_bmp_load: bad BMP\n");
                        return NULL;
                    }
                } else {
                    ez_error ("ez_bmp_load: bad BMP\n");
                    return NULL;
                }
            }
        } else {
            if (hsz != 108) {
                ez_error ("ez_bmp_load: internal error hsz = %d\n", hsz);
                return NULL;
            }
            mr = ez_buffer_get32le (s);
            mg = ez_buffer_get32le (s);
            mb = ez_buffer_get32le (s);
            ma = ez_buffer_get32le (s);
            ez_buffer_get32le (s); /* discard color space */
            for (i=0; i < 12; ++i)
                ez_buffer_get32le (s); /* discard color space parameters */
        }
        if (bpp < 16)
            psize = (offset - 14 - hsz) >> 2;
    }
    s->img_n = ma ? 4 : 3;
    if (req_comp && req_comp >= 3) /* we can directly decode 3 or 4 */
        target = req_comp;
    else
        target = s->img_n; /* if they want monochrome, we'll post-convert */
    out = malloc (target * s->img_x * s->img_y);
    if (!out) {
        ez_error ("ez_bmp_load: out of memory\n");
        return NULL;
    }
    if (bpp < 16) {
        int z=0;
        if (psize == 0 || psize > 256) {
            free (out);
            ez_error ("ez_bmp_load: corrupt BMP: bad psize\n");
            return NULL;
        }
        for (i=0; i < psize; ++i) {
            pal[i][2] = ez_buffer_get8u (s);
            pal[i][1] = ez_buffer_get8u (s);
            pal[i][0] = ez_buffer_get8u (s);
            if (hsz != 12) ez_buffer_get8 (s);
            pal[i][3] = 255;
        }
        ez_buffer_skip (s, offset - 14 - hsz - psize * (hsz == 12 ? 3 : 4));
        if (bpp == 4) width = (s->img_x + 1) >> 1;
        else if (bpp == 8) width = s->img_x;
        else {
            free (out);
            ez_error ("ez_bmp_load: corrupt BMP: bad pbb\n");
            return NULL;
        }
        pad = (-width)&3;
        for (j=0; j < (int) s->img_y; ++j) {
            for (i=0; i < (int) s->img_x; i += 2) {
                int v=ez_buffer_get8 (s), v2=0;
                if (bpp == 4) {
                    v2 = v & 15;
                    v >>= 4;
                }
                out[z++] = pal[v][0];
                out[z++] = pal[v][1];
                out[z++] = pal[v][2];
                if (target == 4) out[z++] = 255;
                if (i+1 == (int) s->img_x) break;
                v = (bpp == 8) ? ez_buffer_get8 (s) : v2;
                out[z++] = pal[v][0];
                out[z++] = pal[v][1];
                out[z++] = pal[v][2];
                if (target == 4) out[z++] = 255;
            }
            ez_buffer_skip (s, pad);
        }
    } else {
        int rshift=0, gshift=0, bshift=0, ashift=0, rcount=0, gcount=0,
            bcount=0, acount=0;
        int z = 0;
        int easy=0;
        ez_buffer_skip (s, offset - 14 - hsz);
        if (bpp == 24) width = 3 * s->img_x;
        else if (bpp == 16) width = 2*s->img_x;
        else /* bpp = 32 and pad = 0 */ width=0;
        pad = (-width) & 3;
        if (bpp == 24) {
            easy = 1;
        } else if (bpp == 32) {
            if (mb == 0xff       && mg == 0xff00 &&
                mr == 0x00ff0000 && ma == 0xff000000)
                easy = 2;
        }
        if (!easy) {
            if (!mr || !mg || !mb) {
                free (out);
                ez_error ("ez_bmp_load: corrupt BMP: bad masks\n");
                return NULL;
            }
            /* Right shift amt to put high bit in position #7 */
            rshift = ez_bmp_highest_bit (mr)-7; rcount = ez_bmp_bitcount (mr);
            gshift = ez_bmp_highest_bit (mg)-7; gcount = ez_bmp_bitcount (mr);
            bshift = ez_bmp_highest_bit (mb)-7; bcount = ez_bmp_bitcount (mr);
            ashift = ez_bmp_highest_bit (ma)-7; acount = ez_bmp_bitcount (mr);
        }
        for (j=0; j < (int) s->img_y; ++j) {
            if (easy) {
                for (i=0; i < (int) s->img_x; ++i) {
                    int a;
                    out[z+2] = ez_buffer_get8u (s);
                    out[z+1] = ez_buffer_get8u (s);
                    out[z+0] = ez_buffer_get8u (s);
                    z += 3;
                    a = (easy == 2 ? ez_buffer_get8 (s) : 255);
                    if (target == 4) out[z++] = (Ez_uint8) a;
                }
            } else {
                for (i=0; i < (int) s->img_x; ++i) {
                    Ez_uint32 v = (bpp == 16 ? (Ez_uint32) ez_buffer_get16le (s)
                                             : ez_buffer_get32le (s));
                    int a;
                    out[z++] = (Ez_uint8) ez_bmp_shiftsigned (v & mr, rshift, rcount);
                    out[z++] = (Ez_uint8) ez_bmp_shiftsigned (v & mg, gshift, gcount);
                    out[z++] = (Ez_uint8) ez_bmp_shiftsigned (v & mb, bshift, bcount);
                    a = (ma ? ez_bmp_shiftsigned (v & ma, ashift, acount) : 255);
                    if (target == 4) out[z++] = (Ez_uint8) a;
                }
            }
            ez_buffer_skip (s, pad);
        }
    }
    if (flip_vertically) {
        Ez_uint8 t;
        for (j=0; j < (int) s->img_y>>1; ++j) {
            Ez_uint8 *p1 = out +      j     *s->img_x*target;
            Ez_uint8 *p2 = out + (s->img_y-1-j)*s->img_x*target;
            for (i=0; i < (int) s->img_x*target; ++i) {
                t = p1[i], p1[i] = p2[i], p2[i] = t;
            }
        }
    }

    if (req_comp && req_comp != target) {
        out = ez_convert_format (out, target, req_comp, s->img_x, s->img_y);
        if (out == NULL) return out; /* ez_convert_format frees input on failure */
    }

    *x = s->img_x;
    *y = s->img_y;
    if (comp) *comp = s->img_n;
    return out;
}


Ez_uint8 *ez_stbi_bmp_load (Ez_stbi *s, int *x, int *y, int *comp, int req_comp)
{
    return ez_bmp_load (s, x, y, comp, req_comp);
}


int ez_stbi_bmp_info (Ez_stbi *s, int *x, int *y, int *comp)
{
    int hsz;
    if (ez_buffer_get8 (s) != 'B' || ez_buffer_get8 (s) != 'M') {
        ez_stbi_rewind ( s );
        return 0;
    }
    ez_buffer_skip (s, 12);
    hsz = ez_buffer_get32le (s);
    if (hsz != 12 && hsz != 40 && hsz != 56 && hsz != 108) {
        ez_stbi_rewind ( s );
        return 0;
    }
    if (hsz == 12) {
        *x = ez_buffer_get16le (s);
        *y = ez_buffer_get16le (s);
    } else {
        *x = ez_buffer_get32le (s);
        *y = ez_buffer_get32le (s);
    }
    if (ez_buffer_get16le (s) != 1) {
        ez_stbi_rewind ( s );
        return 0;
    }
    *comp = ez_buffer_get16le (s) / 8;
    return 1;
}


/*
 * GIF loader -- public domain by Jean-Marc Lienher -- simplified/shrunk by stb
*/

typedef struct Ez_stbi_gif_lzw_struct {
    Ez_int16 prefix;
    Ez_uint8 first;
    Ez_uint8 suffix;
} Ez_stbi_gif_lzw;


typedef struct Ez_stbi_gif_struct {
    int w, h;
    Ez_uint8 *out;                 /* output buffer (always 4 components) */
    int flags, bgindex, ratio, transparent, eflags;
    Ez_uint8  pal[256][4];
    Ez_uint8 lpal[256][4];
    Ez_stbi_gif_lzw codes[4096];
    Ez_uint8 *color_table;
    int parse, step;
    int lflags;
    int start_x, start_y;
    int max_x, max_y;
    int cur_x, cur_y;
    int line_size;
} Ez_stbi_gif;


int ez_gif_test (Ez_stbi *s)
{
    int sz;
    if (ez_buffer_get8 (s) != 'G' || ez_buffer_get8 (s) != 'I' ||
        ez_buffer_get8 (s) != 'F' || ez_buffer_get8 (s) != '8') return 0;
    sz = ez_buffer_get8 (s);
    if (sz != '9' && sz != '7') return 0;
    if (ez_buffer_get8 (s) != 'a') return 0;
    return 1;
}


int ez_stbi_gif_test (Ez_stbi *s)
{
    int r = ez_gif_test (s);
    ez_stbi_rewind (s);
    return r;
}


void ez_stbi_gif_parse_colortable (Ez_stbi *s, Ez_uint8 pal[256][4],
    int num_entries, int transp)
{
    int i;
    for (i=0; i < num_entries; ++i) {
        pal[i][2] = ez_buffer_get8u (s);
        pal[i][1] = ez_buffer_get8u (s);
        pal[i][0] = ez_buffer_get8u (s);
        pal[i][3] = transp ? 0 : 255;
    }
}


int ez_stbi_gif_header (Ez_stbi *s, Ez_stbi_gif *g, int *comp, int is_info)
{
    Ez_uint8 version;
    if (ez_buffer_get8 (s) != 'G' || ez_buffer_get8 (s) != 'I' ||
        ez_buffer_get8 (s) != 'F' || ez_buffer_get8 (s) != '8') {
        ez_error ("ez_stbi_gif_header: corrupt GIF: not GIF\n");
        return 0;
    }

    version = ez_buffer_get8u (s);
    if (version != '7' && version != '9') {
        ez_error ("ez_stbi_gif_header: corrupt GIF: not GIF\n");
        return 0;
    }
    if (ez_buffer_get8 (s) != 'a') {
        ez_error ("ez_stbi_gif_header: corrupt GIF: not GIF\n");
        return 0;
    }

    g->w = ez_buffer_get16le (s);
    g->h = ez_buffer_get16le (s);
    g->flags = ez_buffer_get8 (s);
    g->bgindex = ez_buffer_get8 (s);
    g->ratio = ez_buffer_get8 (s);
    g->transparent = -1;

    /* can't actually tell whether it's 3 or 4 until we parse the comments */
    if (comp != 0) *comp = 4;

    if (is_info) return 1;

    if (g->flags & 0x80)
        ez_stbi_gif_parse_colortable (s, g->pal, 2 << (g->flags & 7), -1);

    return 1;
}


int ez_stbi_gif_info_raw (Ez_stbi *s, int *x, int *y, int *comp)
{
    Ez_stbi_gif g;
    if (!ez_stbi_gif_header (s, &g, comp, 1)) {
        ez_stbi_rewind ( s );
        return 0;
    }
    if (x) *x = g.w;
    if (y) *y = g.h;
    return 1;
}


void ez_stbi_out_gif_code (Ez_stbi_gif *g, Ez_uint16 code)
{
    Ez_uint8 *p, *c;

   /* Recurse to decode the prefixes, since the linked-list is backwards,
      and working backwards through an interleaved image would be nasty */
    if (g->codes[code].prefix >= 0)
        ez_stbi_out_gif_code (g, g->codes[code].prefix);

    if (g->cur_y >= g->max_y) return;

    p = &g->out[g->cur_x + g->cur_y];
    c = &g->color_table[g->codes[code].suffix * 4];

    if (c[3] >= 128) {
        p[0] = c[2];
        p[1] = c[1];
        p[2] = c[0];
        p[3] = c[3];
    }
    g->cur_x += 4;

    if (g->cur_x >= g->max_x) {
        g->cur_x = g->start_x;
        g->cur_y += g->step;

        while (g->cur_y >= g->max_y && g->parse > 0) {
            g->step = (1 << g->parse) * g->line_size;
            g->cur_y = g->start_y + (g->step >> 1);
            --g->parse;
        }
    }
}


Ez_uint8 *ez_stbi_process_gif_raster (Ez_stbi *s, Ez_stbi_gif *g)
{
    Ez_uint8 lzw_cs;
    Ez_int32 len, code;
    Ez_uint32 first;
    Ez_int32 codesize, codemask, avail, oldcode, bits, valid_bits, clear;
    Ez_stbi_gif_lzw *p;

    lzw_cs = ez_buffer_get8u (s);
    clear = 1 << lzw_cs;
    first = 1;
    codesize = lzw_cs + 1;
    codemask = (1 << codesize) - 1;
    bits = 0;
    valid_bits = 0;
    for (code = 0; code < clear; code++) {
        g->codes[code].prefix = -1;
        g->codes[code].first = (Ez_uint8) code;
        g->codes[code].suffix = (Ez_uint8) code;
    }

   /* Support no starting clear code */
    avail = clear+2;
    oldcode = -1;

    len = 0;
    for (;;) {
        if (valid_bits < codesize) {
            if (len == 0) {
                len = ez_buffer_get8 (s); /* start new block */
                if (len == 0)
                    return g->out;
            }
            --len;
            bits |= (Ez_int32) ez_buffer_get8 (s) << valid_bits;
            valid_bits += 8;
        } else {
            Ez_int32 code = bits & codemask;
            bits >>= codesize;
            valid_bits -= codesize;
            /* @OPTIMIZE: is there some way we can accelerate the non-clear path? */
            if (code == clear) {  /* clear code */
                codesize = lzw_cs + 1;
                codemask = (1 << codesize) - 1;
                avail = clear + 2;
                oldcode = -1;
                first = 0;
            } else if (code == clear + 1) { /* end of stream code */
                ez_buffer_skip (s, len);
                while ((len = ez_buffer_get8 (s)) > 0)
                    ez_buffer_skip (s, len);
                return g->out;
            } else if (code <= avail) {
                if (first) {
                    ez_error ("ez_stbi_process_gif_raster: corrupt GIF:"
                        " no clear code\n");
                    return NULL;
                }
                if (oldcode >= 0) {
                    p = &g->codes[avail++];
                    if (avail > 4096) {
                        ez_error ("ez_stbi_process_gif_raster: corrupt GIF:"
                            " too many codes\n");
                        return NULL;
                    }
                    p->prefix = (Ez_int16) oldcode;
                    p->first = g->codes[oldcode].first;
                    p->suffix = (code == avail) ? p->first : g->codes[code].first;
                } else if (code == avail) {
                    ez_error ("ez_stbi_process_gif_raster: corrupt GIF:"
                        " illegal code in raster\n");
                    return NULL;
                }

                ez_stbi_out_gif_code (g, (Ez_uint16) code);

                if ((avail & codemask) == 0 && avail <= 0x0FFF) {
                    codesize++;
                    codemask = (1 << codesize) - 1;
                }

                oldcode = code;
            } else {
                ez_error ("ez_stbi_process_gif_raster: corrupt GIF:"
                    " illegal code in raster\n");
                return NULL;
            }
        }
    }
}


void ez_stbi_fill_gif_background (Ez_stbi_gif *g)
{
    int i;
    Ez_uint8 *c = g->pal[g->bgindex];
    /* @OPTIMIZE: write a dword at a time */
    for (i = 0; i < g->w * g->h * 4; i += 4) {
        Ez_uint8 *p  = &g->out[i];
        p[0] = c[2];
        p[1] = c[1];
        p[2] = c[0];
        p[3] = c[3];
    }
}

/* This function is designed to support animated gifs,
   although stb_image doesn't support it */

Ez_uint8 *ez_stbi_gif_load_next (Ez_stbi *s, Ez_stbi_gif *g, int *comp,
    int req_comp)
{
    int i;
    Ez_uint8 *old_out = 0;

    if (g->out == 0) {
        if (!ez_stbi_gif_header (s, g, comp, 0)) return 0;
        g->out = malloc (4 * g->w * g->h);
        if (g->out == 0) {
            ez_error ("ez_stbi_gif_load_next: out of memory\n");
            return NULL;
        }
        ez_stbi_fill_gif_background (g);
    } else {
        /* Animated-gif-only path */
        if (( (g->eflags & 0x1C) >> 2) == 3) {
            old_out = g->out;
            g->out = malloc (4 * g->w * g->h);
            if (g->out == 0) {
                ez_error ("ez_stbi_gif_load_next: out of memory\n");
                return NULL;
            }
            memcpy (g->out, old_out, g->w*g->h*4);
        }
    }

    for (;;) {
        switch (ez_buffer_get8 (s)) {
            case 0x2C: /* Image Descriptor */
            {
                Ez_int32 x, y, w, h;
                Ez_uint8 *o;

                x = ez_buffer_get16le (s);
                y = ez_buffer_get16le (s);
                w = ez_buffer_get16le (s);
                h = ez_buffer_get16le (s);
                if (( (x + w) > (g->w)) || ((y + h) > (g->h))) {
                    ez_error ("ez_stbi_gif_load_next: corrupt GIF:"
                        " bad image descriptor\n");
                    return NULL;
                }

                g->line_size = g->w * 4;
                g->start_x = x * 4;
                g->start_y = y * g->line_size;
                g->max_x   = g->start_x + w * 4;
                g->max_y   = g->start_y + h * g->line_size;
                g->cur_x   = g->start_x;
                g->cur_y   = g->start_y;

                g->lflags = ez_buffer_get8 (s);

                if (g->lflags & 0x40) {
                    g->step = 8 * g->line_size; /* first interlaced spacing */
                    g->parse = 3;
                } else {
                    g->step = g->line_size;
                    g->parse = 0;
                }

                if (g->lflags & 0x80) {
                    ez_stbi_gif_parse_colortable (s, g->lpal,
                        2 << (g->lflags & 7),
                        g->eflags & 0x01 ? g->transparent : -1);
                    g->color_table = (Ez_uint8 *) g->lpal;
                } else if (g->flags & 0x80) {
                    /* @OPTIMIZE: reset only the previous transparent */
                    for (i=0; i < 256; ++i)
                        g->pal[i][3] = 255;
                    if (g->transparent >= 0 && (g->eflags & 0x01))
                        g->pal[g->transparent][3] = 0;
                    g->color_table = (Ez_uint8 *) g->pal;
                } else {
                    ez_error ("ez_stbi_gif_load_next: corrupt GIF:"
                        " missing color table\n");
                    return NULL;
                }

                o = ez_stbi_process_gif_raster (s, g);
                if (o == NULL) return NULL;

                if (req_comp && req_comp != 4)
                    o = ez_convert_format (o, 4, req_comp, g->w, g->h);
                return o;
            }

            case 0x21: /* Comment Extension. */
            {
                int len;
                if (ez_buffer_get8 (s) == 0xF9) { /* Graphic Control Extension. */
                    len = ez_buffer_get8 (s);
                    if (len == 4) {
                        g->eflags = ez_buffer_get8 (s);
                        ez_buffer_get16le (s); /* delay */
                        g->transparent = ez_buffer_get8 (s);
                    } else {
                        ez_buffer_skip (s, len);
                        break;
                    }
                }
                while ((len = ez_buffer_get8 (s)) != 0)
                    ez_buffer_skip (s, len);
                break;
            }

            case 0x3B: /* gif stream termination code */
                return (Ez_uint8 *) 1;

            default:
                ez_error ("ez_stbi_gif_load_next: corrupt GIF: unkbown code\n");
                return NULL;
        }
    }
}


Ez_uint8 *ez_stbi_gif_load (Ez_stbi *s, int *x, int *y, int *comp, int req_comp)
{
    Ez_uint8 *u = 0;
    Ez_stbi_gif g;  /* = {0}; gcc complains about it; we use memset instead */
    memset (&g, 0, sizeof (g));

    u = ez_stbi_gif_load_next (s, &g, comp, req_comp);
    if (u == (void *) 1) u = 0;  /* end of animated gif marker */
    if (u) {
        *x = g.w;
        *y = g.h;
    }

    return u;
}


int ez_stbi_gif_info (Ez_stbi *s, int *x, int *y, int *comp)
{
    return ez_stbi_gif_info_raw (s, x, y, comp);
}


/*
 * Get image dimensions and components without fully decoding
*/

int ez_stbi_info_main (Ez_stbi *s, int *x, int *y, int *comp)
{
    if (ez_stbi_jpeg_info (s, x, y, comp)) return 1;
    if (ez_stbi_png_info  (s, x, y, comp)) return 1;
    if (ez_stbi_gif_info  (s, x, y, comp)) return 1;
    if (ez_stbi_bmp_info  (s, x, y, comp)) return 1;

    ez_error ("ez_stbi_info_main: image not of any known type, or corrupt\n");
    return 0;
}


int ez_stbi_info (char const *filename, int *x, int *y, int *comp)
{
    FILE *f = fopen (filename, "rb");
    int result;
    if (!f) {
        ez_error ("ez_stbi_info: unable to open file \"%s\"\n", filename);
        return 0;
    }
    result = ez_stbi_info_from_file (f, x, y, comp);
    fclose (f);
    return result;
}


int ez_stbi_info_from_file (FILE *f, int *x, int *y, int *comp)
{
    int r;
    Ez_stbi s;
    long pos = ftell (f);
    ez_start_file (&s, f);
    r = ez_stbi_info_main (&s, x, y, comp);
    fseek (f, pos, SEEK_SET);
    return r;
}


int ez_stbi_info_from_memory (Ez_uint8 const *buffer, int len, int *x, int *y,
    int *comp)
{
    Ez_stbi s;
    ez_stbi_start_mem (&s, buffer, len);
    return ez_stbi_info_main (&s, x, y, comp);
}


int ez_stbi_info_from_callbacks (Ez_stbi_io_callbacks const *c, void *user,
    int *x, int *y, int *comp)
{
    Ez_stbi s;
    ez_stbi_start_callbacks (&s, (Ez_stbi_io_callbacks *) c, user);
    return ez_stbi_info_main (&s, x, y, comp);
}

#endif /* !USE_ORIGINAL_STBI */


