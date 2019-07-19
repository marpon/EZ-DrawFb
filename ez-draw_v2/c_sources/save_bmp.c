/*********************************************************
* save_bmp.c                                             *
* By Malcolm McLean                                      *
* This is a file from the book Basic Algorithms by       *
*   Malcolm McLean                                       *
*********************************************************/
#include <stdio.h>
#include <stdlib.h>

static void saveheader_bmp(FILE *fp, int width, int height, int bits);
static void fput32le(long x, FILE *fp);
static void fput16le(int x, FILE *fp);

/***********************************************************
* save a24-bit bmp file.                                   *
* Params: fname - name of file to save.                    *
*         rgb - raster data in rgb format                  *
*         width - image width                              *
*         height - image height                            *
* Returns: 0 on success, -1 on fail                        *
***********************************************************/             
int savebmp(char *fname, unsigned char *rgb, int width, int height)
{
  FILE *fp;
  int i;
  int ii;

  fp = fopen(fname, "wb");
  if(!fp)
	return -1;

  saveheader_bmp(fp, width, height, 24);
  for(i=0;i<height;i++)
  {
	for(ii=0;ii<width;ii++)
	{
      fputc(rgb[2], fp);
	  fputc(rgb[1], fp);
	  fputc(rgb[0], fp);
	  rgb += 3;
	}
	if(( width * 3) % 4)
	{
	  for(ii=0;ii< 4 - ( (width * 3) % 4); ii++)
	  {
	    fputc(0, fp);
	  }
	}
  }

  if(ferror(fp))
  {
    fclose(fp);
	return -1;
  }

  return fclose(fp);
}


/****************************************************************
* write a bitmap header.                                        *
* Params: fp - pointer to an open file.                         *
*         width - bitmap width                                  *
*         height - bitmap height                                *
*         bit - bit depth (24)                 *
****************************************************************/
static void saveheader_bmp(FILE *fp, int width, int height, int bits)
{
  long sz;
  long offset;

  /* the file header */
  /* "BM" */
  fputc(0x42, fp);
  fputc(0x4D, fp);

  /* file size */
//  sz = getfilesize(width, height, bits) + 40 + 14;
  sz = ((width * 3 + 3)/4 * 4 * height) + 40 + 14;

  fput32le(sz, fp);

  /* reserved */
  fput16le(0, fp);
  fput16le(0, fp);
  /* offset of raster data from header */
  if(bits < 16)
    offset = 40 + 14 + 4 * (1 << bits);
  else
	offset = 40 + 14;
  fput32le(offset, fp);

  /* the infoheader */

  /* size of structure */
  fput32le(40, fp);
  fput32le(width, fp);
  /* height negative because top-down */
  fput32le(-height, fp);
  /* bit planes */
  fput16le(1, fp);
  fput16le(bits, fp);
  /* compression */
  fput32le(0, fp);
  /* size of image (can be zero) */
  fput32le(0, fp);
  /* pels per metre */
  fput32le(600000, fp);
  fput32le(600000, fp);
  /* colours used */
  fput32le(0, fp);
  /* colours important */
  fput32le(0, fp);
}


/***************************************************************
* write a 32-bit little-endian number to a file.               *
* Params: x - the number to write                              *
*         fp - pointer to an open file.                        *
***************************************************************/
static void fput32le(long x, FILE *fp)
{
  fputc(x & 0xFF, fp);
  fputc( (x >> 8) & 0xFF, fp);
  fputc( (x >> 16) & 0xFF, fp);
  fputc( (x >> 24) & 0xFF, fp);
}

/***************************************************************
* write a 16-bit little-endian number to a file.               *
* Params: x - the nmuber to write                              *
*         fp - pointer to an open file                         *
***************************************************************/                
static void fput16le(int x, FILE *fp)
{
  fputc(x & 0xFF, fp);
  fputc( (x >> 8) & 0xFF, fp);
}



