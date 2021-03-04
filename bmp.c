#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bmp2cgb.h"
#include "errors.h"



void split_4bpp(unsigned char *bmp_data, int px_count)
{
	unsigned char	c;
	int				i, j, k;
	
	i = (px_count / 2);
	j = (i - 1);
	k = (px_count - 1);

	for(; i > 0; i--)
	{
		c = bmp_data[j] & 0x0f;
		bmp_data[k] = c;
		k--;
		c = (bmp_data[j] >> 4) & 0x0f;
		bmp_data[k] = c;
		k--;
		j--;
	}
}



char merge_24bpp(RGBQUAD *bmp_pal, unsigned char *src, int px_count)
{
	unsigned char	r, g, b, next_color, found, *dst;
	unsigned short	color;
	int				pixel;

	dst = src;
	next_color = 0;

	for(pixel = 0; pixel < px_count; pixel++)							
	{
		b = *src++;
		g = *src++;
		r = *src++;

		found = 0;

		for(color = 0; color < 256; color++)							
		{		
			if(bmp_pal[color].blue == b && bmp_pal[color].green == g && bmp_pal[color].red == r)
			{
				found++;
				break;
			}
		}

		if(found)														
		{
			dst[pixel] = (unsigned char) color;
		}
		else if(next_color == 32)										
		{
			return(ERR_MAX_COLS1);
		}
		else															
		{
			bmp_pal[next_color].blue = b;
			bmp_pal[next_color].green = g;
			bmp_pal[next_color].red = r;
			dst[pixel] = next_color;
			next_color++;
		}
	}

	return(NO_ERROR);
}



char merge_32bpp(RGBQUAD *bmp_pal, unsigned char *src, int px_count, unsigned int comp)
{
	BITFIELDS		mask;
	unsigned char	r, g, b, next_color, found, *dst;
	unsigned short	color;
	int				pixel;


	if(comp == BI_BITFIELDS)
	{
		memcpy(&mask, src, sizeof(mask));

		if(mask.red != 0x00ff0000)
			return(ERR_BITFIELDS);

		if(mask.green != 0x0000ff00)
			return(ERR_BITFIELDS);

		if(mask.blue != 0x000000ff)
			return(ERR_BITFIELDS);

		memmove(src, src + sizeof(mask), px_count * 4);
	}

	dst = src;
	next_color = 0;

	for(pixel = 0; pixel < px_count; pixel++)							
	{
		b = *src++;
		g = *src++;
		r = *src++;
		src++;															

		found = 0;

		for(color = 0; color < 256; color++)							
		{
			if(bmp_pal[color].blue == b && bmp_pal[color].green == g && bmp_pal[color].red == r && bmp_pal[color].reserved == 0)
			{
				found = 1;
				break;
			}
		}

		if(found)														
		{
			dst[pixel] = (unsigned char) color;
		}
		else if(next_color == 32)										
		{
			return(ERR_MAX_COLS1);
		}
		else															
		{
			bmp_pal[next_color].blue = b;
			bmp_pal[next_color].green = g;
			bmp_pal[next_color].red = r;
			bmp_pal[next_color].reserved = 0;							
			dst[pixel] = next_color;
			next_color++;
		}
	}

	return(NO_ERROR);
}



void flip(unsigned char *bmp_data, int width, int height)
{
	unsigned char	tmp;
	int				x, y, src, dst;


	for(x = 0; x < width; x++) {
		for(y = 0; y < width * (height - 1) / 2; y += width) {
			dst = y + x;
			src = width * (height - 1) - y + x;
			tmp = bmp_data[dst];
 			bmp_data[dst] = bmp_data[src];
			bmp_data[src] = tmp;
		}
	}
}



char process_bmp(char *fname, BITMAPFILEHEADER **header, BITMAPINFOHEADER **info, RGBQUAD *bmp_pal, unsigned char **bmp_data, unsigned short *width, unsigned short *height)
{
	FILE				*fp;
	char				result;
	int					size, bmp_pal_len;

	fp = fopen(fname, "rb");
	if(fp == NULL)
		return(ERR_NOT_FOUND);

	*header = (BITMAPFILEHEADER *) malloc(sizeof(BITMAPFILEHEADER));
	if(*header == NULL)
		return(ERR_MALLOC_HDR);
	fread(*header, sizeof(BITMAPFILEHEADER), 1, fp);

	if ((*header)->bfType != 0x4D42)
		return(ERR_NOT_BMP);

	*info = (BITMAPINFOHEADER *) malloc(sizeof(BITMAPINFOHEADER));
	if(*info == NULL)
		return(ERR_MALLOC_NFO);
	fread(*info, sizeof(BITMAPINFOHEADER), 1, fp);

	if((*info)->biSize != 0x28)
		return(ERR_NO_NFOHDR);

	if (((*info)->biWidth & 7) || ((*info)->biHeight & 7))
		return(ERR_NOT_ROUNDED);

	if (((*info)->biWidth / TILE_WIDTH) * ((*info)->biHeight / TILE_HEIGHT) > MAX_MAP_SIZE)
		return(ERR_TOO_BIG);

	*width = (unsigned short) (*info)->biWidth;
	*height = (unsigned short) abs((*info)->biHeight);					

	if((*info)->biCompression != BI_RGB && (*info)->biCompression != BI_BITFIELDS)
		return(ERR_COMPRESSED);

	memset(bmp_pal, 0xff, sizeof(RGBQUAD) * 256);

	bmp_pal_len = (*header)->bfOffBits - ftell(fp);				// fixes trimmed palette issue in 4bpp/8bpp bitmaps

	switch((*info)->biBitCount)
	{
		case 4:
			fread(bmp_pal, bmp_pal_len, 1, fp);
			*bmp_data = malloc((*info)->biWidth * (*info)->biHeight);
			if (*bmp_data == NULL)
				return(ERR_MALLOC_BMPD);
			fread(*bmp_data, (*info)->biWidth * (*info)->biHeight / 2, 1, fp);
			split_4bpp(*bmp_data, (*info)->biWidth * (*info)->biHeight);
			break;

		case 8:
			fread(bmp_pal, bmp_pal_len, 1, fp);
			*bmp_data = malloc((*info)->biWidth * (*info)->biHeight);
			if (*bmp_data == NULL)
				return(ERR_MALLOC_BMPD);
			fread(*bmp_data, (*info)->biWidth * (*info)->biHeight, 1, fp);
			break;

		case 16:
			return(ERR_NOT_SUPRTD);
			break;

		case 24:
			*bmp_data = malloc((*info)->biWidth * (*info)->biHeight * 3);
			if (*bmp_data == NULL)
				return(ERR_MALLOC_BMPD);
			fread(*bmp_data, (*info)->biWidth * (*info)->biHeight * 3, 1, fp);
			result = merge_24bpp(bmp_pal, *bmp_data, (*info)->biWidth * (*info)->biHeight);
			if(result)
				return(result);
			break;

		case 32:
			if((*info)->biCompression == BI_RGB)
				size = (*info)->biWidth * (*info)->biHeight * 4;
			else
				size = ((*info)->biWidth * (*info)->biHeight * 4) + BITFIELDS_SIZE;
			*bmp_data = malloc(size);
			if (*bmp_data == NULL)
				return(ERR_MALLOC_BMPD);
			fread(*bmp_data, size, 1, fp);
			result = merge_32bpp(bmp_pal, *bmp_data, (*info)->biWidth * (*info)->biHeight, (*info)->biCompression);
			if(result)
				return(result);
			break;

		default:
			return(ERR_NOT_SUPRTD);
			break;
	}

	fclose(fp);

	if((*info)->biHeight > 0)
		flip(*bmp_data, (*info)->biWidth, (*info)->biHeight);

	printf("Bitmap size: %d * %d px\n", (*info)->biWidth, (*info)->biHeight);
	printf("Character/attribute map: %d * %d = %d bytes\n", (*info)->biWidth / 8, (*info)->biHeight / 8, ((*info)->biWidth / 8) * ((*info)->biHeight / 8));

	return(NO_ERROR);
}
