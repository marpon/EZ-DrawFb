#include "ez-draw2.h"
#include "ez-image2.h"


extern Ez_X ezx;


#ifndef _WIN32
Ez_uint8 *x11_capscreen( Ez_window my_win, int *pwidth, int *pheight)
{
	int lx, ly;
	int width, height;
	XWindowAttributes xwa;
	XGetWindowAttributes(ezx.display, my_win, &xwa);


	width = xwa.width;
	height = xwa.height;
	*pwidth = xwa.width;
	*pheight = xwa.height;
	XImage *m = XGetImage( ezx.display, my_win, 0, 0, width, height, AllPlanes, XYPixmap);

	if( m->bitmap_pad != 32 )
	{
		fprintf( stderr, "Bad bitmap pad\n" );
		return NULL;
	}

	Ez_uint8 *bp = malloc( width * height * 3 );
	if(bp == NULL)
	{
		fprintf( stderr, "Not allocated memory\n" );
		return NULL;
	}	
	Ez_uint8 *bp0 = bp;
	for( ly = 0; ly < height; ly++ )
	{
		for( lx = 0; lx < width; lx++ )
		{
			Ez_uint32 px = XGetPixel( m, lx, ly );
			//(*(pb++)) = (px>>16) | (px&0xff00) | ((px&0xff)<<16);
			(*bp++) = px>>16 & 0xFF;
			(*bp++) = px>>8 & 0xFF;
			(*bp++) = px & 0xFF;
		}
	}
	XDestroyImage( m );
	return bp0;
}

#else


Ez_uint8 *win_capscreen(Ez_window hwnd, int *GWidth, int *GHeight)
{
    HDC DevC = GetDC(hwnd);
    if(DevC == NULL)
	{
		fprintf( stderr, "GetDC error\n" );
		return NULL;
	}	
    HDC CaptureDC = CreateCompatibleDC(DevC);
    if(CaptureDC == NULL)
	{
		fprintf( stderr, "CreateCompatibleDC error\n" );
		ReleaseDC(hwnd, DevC);
		return NULL;
	}	
    WINDOWINFO WinInfos;
    WinInfos.cbSize= sizeof(WINDOWINFO);
    GetWindowInfo(hwnd, &WinInfos);
	
    int Width = WinInfos.rcClient.right - WinInfos.rcClient.left;
    int Height = WinInfos.rcClient.bottom - WinInfos.rcClient.top;

    HBITMAP CaptureBitmap = CreateCompatibleBitmap(DevC, Width, Height);
    if(CaptureBitmap == NULL)
	{
		fprintf( stderr, "CreateCompatibleBitmap error\n" );
		ReleaseDC(hwnd, DevC);
    	DeleteObject(CaptureDC);
		return NULL;
	}	
	SelectObject(CaptureDC, CaptureBitmap);
    BitBlt(CaptureDC, 0, 0, Width, Height, DevC, 0, 0, SRCCOPY|CAPTUREBLT);

    BITMAPINFOHEADER   bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = Width ;
    bi.biHeight = -Height ;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;


    *GWidth = Width ;
    *GHeight = Height ;
    DWORD dwBmpSize = Width  * Height  * 4;

    Ez_uint8 *pPixels = malloc(dwBmpSize);
    if(pPixels == NULL)
	{
		fprintf( stderr, "Not allocated memory\n" );
		ReleaseDC(hwnd, DevC);
    	DeleteObject(CaptureDC);
    	DeleteObject(CaptureBitmap);
		return NULL;
	}	
    GetDIBits(CaptureDC, CaptureBitmap, 0, Height, pPixels, (BITMAPINFO *)&bi, DIB_RGB_COLORS);
	ReleaseDC(hwnd, DevC);
    DeleteObject(CaptureDC);
    DeleteObject(CaptureBitmap);
    DWORD dx = 0;
    DWORD kx = 0;
    Ez_uint8 temp;
    while(dx < dwBmpSize)
    {
		temp = pPixels[dx ];
		pPixels[kx] = pPixels[dx + 2];
		pPixels[kx + 1] = pPixels[dx + 1];
		pPixels[kx + 2] = temp;
		kx +=3;
		dx +=4;
    }
    pPixels = realloc(pPixels, dwBmpSize * 3 / 4);
    return pPixels;
}
#endif


Ez_rgb *ez_win_to_rgb(Ez_window my_win)
{
	Ez_uint8 *arr;
	int width;
	int height;	
#ifndef _WIN32
	arr = x11_capscreen( my_win, &width, &height);
#else
	arr = win_capscreen( my_win, &width, &height);
#endif
	if(arr == NULL || height < 1 || width < 1)
		return NULL;
	Ez_rgb *rgb	= malloc(sizeof(Ez_rgb));
	if(rgb == NULL )
	{
		free(arr);
		return NULL;		
	}			
	rgb->pixels_rgb = arr;
	rgb->width = width;
	rgb->height = height;
return rgb;		
}


Ez_image *ez_win_to_image(Ez_window my_win)
{
	int i = 0;
	int k = 0;
	Ez_rgb *rgb = ez_win_to_rgb(my_win);
	if(rgb == NULL || rgb->pixels_rgb == NULL || rgb->height < 1 || rgb->width < 1)
		return NULL;
	Ez_image *img	= malloc(sizeof(Ez_image));
	if(img == NULL )
	{
		free(rgb->pixels_rgb);
		free(rgb);
		return NULL;
	}		
	Ez_uint8 *rgba = malloc(rgb->height * rgb->width * 4);
	if(rgba == NULL )
	{
		free(img);
		free(rgb->pixels_rgb);
		free(rgb);
		return NULL;
	}
	while (i < rgb->height * rgb->width * 3)
	{
		rgba[k]= rgb->pixels_rgb[i];
		rgba[k + 1] = rgb->pixels_rgb[i + 1];
		rgba[k + 2] = rgb->pixels_rgb[i + 2];
		rgba[k + 3] = 0;
		i +=3;
		k +=4;
	}
	free(rgb);
	img->pixels_rgba = rgba;
	img->width = rgb->width;
	img->height = rgb->height;
	img->has_alpha = 0;
	img->opacity = 1;
	return img;		
}

Ez_rgb *ez_image_to_rgb(Ez_image *my_img)
{
	if(my_img == NULL || my_img->pixels_rgba == NULL || my_img->height < 1 || my_img->width < 1)
		return NULL;
	Ez_rgb *rgb	= malloc(sizeof(Ez_rgb));
	if(rgb == NULL )
		return NULL;	
	Ez_uint8 *arr = malloc(my_img->height * my_img->width * 3);
	if(arr == NULL )
	{
		free(rgb);
		return NULL;
	}	
	int i = 0;
	int k = 0;
	while (i < my_img->height * my_img->width * 4)
	{
		arr[k]= my_img->pixels_rgba[i];
		arr[k + 1] = my_img->pixels_rgba[i + 1];
		arr[k + 2] = my_img->pixels_rgba[i + 2];
		i +=4;
		k +=3;
	}
		
	rgb->pixels_rgb = arr;
	rgb->width = my_img->width;
	rgb->height = my_img->height;
	return rgb;			
}


Ez_uint8 *ez_get_rgb_data_free(Ez_rgb *rgb, int *w, int *h)
{
	if(rgb == NULL)
		return NULL;
	*w = rgb->width;
	*h = rgb->height;
	Ez_uint8 *arr = rgb->pixels_rgb;
	free(rgb);
	return arr;	
}

Ez_uint8 *ez_get_rgb_data(Ez_rgb *rgb, int *w, int *h)
{
	if(rgb == NULL)
		return NULL;
	*w = rgb->width;
	*h = rgb->height;
	Ez_uint8 *arr = rgb->pixels_rgb;
	return arr;	
}


int ez_set_pixel_image(Ez_image *my_img, int x, int y, Ez_uint32 color1)
{
	if(my_img == NULL || my_img->pixels_rgba == NULL || x > my_img->width || y > my_img->height)
		return 0;
	int ipix = (y * my_img->width * 4) + (x * 4);
	my_img->pixels_rgba[ipix] = color1 & 0xFF;
	my_img->pixels_rgba[ipix + 1] = (color1 >> 8) & 0xFF;
	my_img->pixels_rgba[ipix + 2] = (color1 >> 16) & 0xFF;
 	my_img->pixels_rgba[ipix + 3] = (color1 >> 24) & 0xFF;
 	return ipix;
}

int ez_set_pixel_rgb(Ez_rgb *my_rgb, int x, int y, Ez_uint32 color1)
{
	if(my_rgb == NULL || my_rgb->pixels_rgb == NULL || x > my_rgb->width || y > my_rgb->height)
		return 0;
	int ipix = (y * my_rgb->width * 3) + (x * 3);
	my_rgb->pixels_rgb[ipix] = color1 & 0xFF;
	my_rgb->pixels_rgb[ipix + 1] = (color1 >> 8) & 0xFF;
	my_rgb->pixels_rgb[ipix + 2] = (color1 >> 16) & 0xFF;
 	return ipix;
}

Ez_uint32 ez_compose_rgba_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	Ez_uint32 color1 = (Ez_uint32)r +((Ez_uint32)g * 256)+ ((Ez_uint32)b *256 *256) + ((Ez_uint32)a *256 *256 * 256);
	return color1;
}

