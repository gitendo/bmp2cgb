#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "bmp2cgb.h"
#include "errors.h"
#include "rgbt.h"


void banner(void)
{
	printf("\nbmp2cgb v%.2f - bitmap converter for Game Boy Color\n", VERSION);
	printf("Programmed by: tmk, email: tmk@tuta.io\n");
	printf("Project page: https://github.com/gitendo/bmp2cgb/\n\n");
}



void usage(void)
{
	banner();

	printf("Syntax: bmp2cgb.exe [options] picture[.bmp]\n\n");
	printf("Options:\n");
	
	printf("\t-c    disable character optimization\n");
	printf("\t-x    disable horizontal flip optimization\n");
	printf("\t-y    disable vertical flip optimization\n");
	printf("\t-z    disable horizontal & vertical flip optimization\n");
	printf("\t-o    disable palette optimization\n");
	printf("\n");
	printf("\t-e#   expand map width to 32 blocks using character (0-255)\n");
	printf("\t-m#   map padding - starting character (1-511)\n");
	printf("\t-p#   palette padding - starting slot (1-7)\n");
	printf("\t-r    rebase character map to $8800-$97FF ($8000-$8FFF is default)\n");
	printf("\n");
	printf("\t-d    extended debug information without data output\n");
	printf("\t-s#   sprites output (transparent color RGB hex value ie. 4682b4)\n");
	printf("\t-t    RGBTuner ROM image output\n");

	exit(EXIT_FAILURE);
}



void error_handler(char status)
{
	char	*msg;

	switch(status)
	{
		case ERR_UNK_OPTION :	msg = "Invalid option!\n"; break;
		case ERR_NOT_FOUND	:	msg = "Input file not found or access denied!\n"; break;
		case ERR_MALLOC_HDR	:	msg = "Memory allocation for BITMAPFILEHEADER failed!\n"; break;
		case ERR_NOT_BMP	:	msg = "BMP signature missing!\n"; break;
		case ERR_MALLOC_NFO	:	msg = "Memory allocation for BITMAPINFOHEADER failed!\n"; break;
		case ERR_NO_NFOHDR	:	msg = "Unsupported bitmap type, BITMAPINFOHEADER not found!\n"; break;
		case ERR_NOT_ROUNDED:	msg = "Height and width must be a multiple of 8!\n"; break;
		case ERR_TOO_BIG	:	msg = "Image too big - character map exceeds bank size!\n"; break;
		case ERR_COMPRESSED	:	msg = "Compression method is not supported!\n"; break;
		case ERR_MALLOC_BMPD:	msg = "Memory allocation for BMP data failed!\n"; break;
		case ERR_NOT_SUPRTD	:	msg = "Unsupported / unknown bitmap type!\n"; break;
		case ERR_MAX_COLS1	:	msg = "No more than 32 unique colors are allowed!\n"; break;
		case ERR_BITFIELDS	:	msg = "Unsupported BITFIELD size, only 8 bit masks are valid!\n"; break;
		case ERR_TRNSP		:	msg = "Transparent color not found in bitmap palette!\n"; break;
		case ERR_PADDING	:	msg = "Padding allowed for maps below 32 * 32 blocks only!\n"; break;
		case ERR_MALLOC_TMPD:	msg = "Memory allocation for BMP data helper failed!\n"; break;
		case ERR_MALLOC_TMPC:	msg = "Memory allocation for palette helper failed!\n"; break;
		case ERR_MAX_COLS2	:	return; break;							
		case ERR_MALLOC_TMPI:	msg = "Memory allocation for palette index failed!\n"; break;
		case ERR_PASS		:	msg = "Maximum of 8 palettes reached!\n"; break;
		case ERR_PASS1		:	msg = "Pass 1 failed! Maximum of 8 palettes reached!\n"; break;
		case ERR_PASS2		:	msg = "Pass 2 failed! Maximum of 8 palettes reached!\n"; break;
		case ERR_PASS3		:	msg = "Pass 3 failed! Maximum of 8 palettes reached!\n"; break;
		case ERR_PASS4		:	msg = "Pass 4 failed! Maximum of 8 palettes reached!\n"; break;
		case ERR_TMPI_FAILED:	msg = "Building palette index helper failed!\n"; break;
		case ERR_WRONG_PAL	:	msg = "find_palette() supports only 1 or 2 color palettes!\n"; break;
		case ERR_MAX_TILES	:	msg = "Maximum characters limit exceeded!\n"; break;
		case ERR_MALLOC_CGBT:	msg = "Memory allocation for CGB tiles failed!\n"; break;
		default				:	msg = "Undefined error code!\n"; break;
	}

	printf("\nError: %s", msg);
	return;
}



void save(char *fname, char *ext, void *src, int size)
{
	char 	fn[FILENAME_MAX];
	FILE 	*fp = NULL;

	strcpy(fn, fname);													
	strcat(fn, ext);													

	fp = fopen(fn, "wb");												
	fwrite(src, size, 1, fp);
	fclose(fp);
}



void make_rgbt(unsigned char *rgbtuner, unsigned char *cgb_chr, unsigned char *cgb_map, unsigned char *cgb_atr, CGBQUAD *cgb_pal, unsigned short map_height, unsigned short map_width, unsigned short used_tiles)
{
	unsigned char	*dst, *src, size;
	unsigned short	crc = 0;
	size_t			i;

	memcpy(&rgbtuner[PAL_DATA], cgb_pal, sizeof(CGBQUAD) * MAX_SLOTS);	

	if(map_width > 32)													
		size = 32;
	else
		size = (unsigned char) map_width;

	if(map_height > 32)
		map_height = 32;
	
	for(map_height--; map_height != 0xffff; map_height--)								
	{
		src = &cgb_map[map_height * map_width];
		dst = &rgbtuner[MAP_DATA + (map_height * 32)];
		memcpy(dst, src, size);

		src = &cgb_atr[map_height * map_width];
		dst = &rgbtuner[ATR_DATA + (map_height * 32)];
		memcpy(dst, src, size);
	}

	memcpy(&rgbtuner[CHR_DATA], cgb_chr, used_tiles * CGB_TILE_SIZE);	

	if(used_tiles > 240)												
	{
		*(unsigned short *) &rgbtuner[CHR_SIZE_VBK0] = 240 * CGB_TILE_SIZE;
		used_tiles -= 240;
		*(unsigned short *) &rgbtuner[CHR_SIZE_VBK1] = used_tiles * CGB_TILE_SIZE;
	}
	else																
	{
		*(unsigned short *) &rgbtuner[CHR_SIZE_VBK0] = used_tiles * CGB_TILE_SIZE;
		*(unsigned short *) &rgbtuner[CHR_SIZE_VBK1] = 0;
	}

	for(i = 0; i < RGBT_ROM_SIZE; i++)									
		crc += rgbtuner[i];

	*(unsigned short *) &rgbtuner[CHECKSUM] = ((crc << 8) & 0xff00) | ((crc >> 8) & 0xff);
}


void release(unsigned char **bmp_data, CGBQUAD **tmp_pal, unsigned char	**tmp_idx, unsigned char **cgb_chr)
{
	if(*bmp_data != NULL)
		free(*bmp_data);
	if(*tmp_pal != NULL)
		free(*tmp_pal);
	if(*tmp_idx != NULL)
		free(*tmp_idx);
	if(*cgb_chr != NULL)
		free(*cgb_chr);
}


int main (int argc, char *argv[])
{
	BITMAPFILEHEADER	*header = NULL;
	BITMAPINFOHEADER	*info = NULL;
	RGBQUAD				bmp_pal[256];
	unsigned char		*bmp_data = NULL;

	CGBQUAD				*tmp_pal = NULL;
	unsigned char		*tmp_idx = NULL;

	unsigned char		*cgb_chr = NULL;
	unsigned char		cgb_atr[MAX_MAP_SIZE];
	unsigned char		cgb_map[MAX_MAP_SIZE];
	CGBQUAD 			cgb_pal[MAX_SLOTS] = {{0, 0, 0, 0}};

	char				 match, status;
	unsigned char		arg, chr = 0, fname = 0, slot = 0, used_slots;
	unsigned short		i, width, height, columns, rows, options = 0, padding = 0, used_tiles = 0;
	unsigned int		rgbhex = 0;


	if(argc < 2)
		usage();

	for(arg = 1; arg < argc; arg++)
	{
		if((argv[arg][0] == '-') || (argv[arg][0] == '/'))
		{
			switch(tolower(argv[arg][1]))
			{
				case 'c': options |= FLAG_DUMP; break;
				case 'd': options |= FLAG_DEBUG; break;
				case 'e': options |= FLAG_EXPAND; chr = (unsigned char) atoi(&argv[arg][2]); break;
				case 'm': options |= FLAG_MPAD; padding = (unsigned short) ((atoi(&argv[arg][2])) & 0x1ff); break;
				case 'o': options |= FLAG_UNOPT; break;
				case 'p': options |= FLAG_PPAD; slot = (unsigned char) (atoi(&argv[arg][2]) & 7); break;
				case 'r': options |= FLAG_REBASE; break;
				case 's': options |= FLAG_OBJ; rgbhex = (strtol(&argv[arg][2], NULL, 16) & 0xffffff) | 1UL << 24; break;
				case 't': options |= FLAG_TUNER | FLAG_MPAD; padding = 0x10; break;	
				case 'x': options |= FLAG_FLIPX; break;
				case 'y': options |= FLAG_FLIPY; break;
				case 'z': options |= FLAG_FLIPXY; break;

				default: error_handler(ERR_UNK_OPTION); break;
			}
		}
		else
			fname = arg;
	}




	status = process_bmp(argv[fname], &header, &info, bmp_pal, &bmp_data, &width, &height);

	if(header != NULL)													
		free(header);
	if(info != NULL)
		free(info);

	rows = (unsigned short) (height / TILE_WIDTH);						
	columns = (unsigned short) (width / TILE_HEIGHT);

	if(options & FLAG_OBJ)												
	{
		match = 0;
		unsigned char *rgb = (unsigned char *) &rgbhex;
		for(i = 0; i < 256; i++)
		{
			if(bmp_pal[i].red == rgb[0] && bmp_pal[i].green == rgb[1] && bmp_pal[i].blue == rgb[2])
			{
				match++;
				break;
			}
		}

		if(match == 0)
			status = ERR_TRNSP;
	}

	if(options & FLAG_EXPAND)
	{
		if(columns >= 32)
			status = ERR_PADDING;
	}

	status = create_tiles(bmp_data, width, height, status);
	status = optimize(bmp_data, bmp_pal, &tmp_pal, rows, columns, rgbhex, options, status);

	used_slots = slot;

	status = create_palettes(cgb_pal, &tmp_idx, tmp_pal, rows * columns, &used_slots, options, status);
	remap_colors(bmp_data, cgb_pal, tmp_idx, tmp_pal, rows * columns, status);
	status = convert(bmp_data, &cgb_chr, cgb_map, cgb_atr, tmp_idx, rows * columns, padding, &used_tiles, options, status);

	if(options & FLAG_EXPAND)
	{
		expand_maps(cgb_map, cgb_atr, columns, rows, chr, status);		
		columns = 32;
	}

	if(status)
	{
		error_handler(status);
		release(&bmp_data, &tmp_pal, &tmp_idx, &cgb_chr);
		exit(EXIT_FAILURE);
	}

	if(!(options & FLAG_DEBUG))
	{
		char *ext = strstr(argv[fname], ".bmp");
		if(ext)
			ext[0] = 0;													

		save(argv[fname], EXT_CHR, cgb_chr, used_tiles * CGB_TILE_SIZE);

		if(options & FLAG_OBJ)
		{
			save_oam(argv[fname], EXT_TXT, cgb_atr, cgb_map, rows * columns);
		}
		else
		{
			save(argv[fname], EXT_ATR, cgb_atr, rows * columns);

			if(options & FLAG_REBASE)
				rebase(cgb_map, rows * columns);

			save(argv[fname], EXT_MAP, cgb_map, rows * columns);
		}

		if(options & FLAG_TUNER)
		{
			make_rgbt(rgbt, cgb_chr, cgb_map, cgb_atr, cgb_pal, rows, columns, used_tiles);
			save(argv[fname], EXT_GBC, rgbt, sizeof(rgbt));
		}
		else
			save(argv[fname], EXT_PAL, &cgb_pal[slot], sizeof(CGBQUAD) * (used_slots - slot));
	}

	release(&bmp_data, &tmp_pal, &tmp_idx, &cgb_chr);

	exit(EXIT_SUCCESS);
}
