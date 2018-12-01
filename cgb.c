#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bmp2cgb.h"
#include "errors.h"



char create_tiles(unsigned char *bmp_data, unsigned short width, unsigned short height, char status)
{
	unsigned char	*tmp_data = NULL;
	int				x, y, z, idx;


	if(status)															
		return status;

	tmp_data = malloc(width * height);
	if(!tmp_data)
		return(ERR_MALLOC_TMPD);

	for(y = 0; y < width * height; y += (width * TILE_HEIGHT))			
	{
		for(x = 0; x < width; x += TILE_WIDTH)							
		{
			idx = y + x;												
			for(z = 0; z < TILE_HEIGHT; z++)							
			{
				memcpy(tmp_data, &bmp_data[idx], TILE_WIDTH);
				tmp_data += TILE_WIDTH;
				idx += width;				
			}
		}
	}

	tmp_data -= (width * height);										
	memcpy(bmp_data, tmp_data, (width * height));
	free(tmp_data);

	return(NO_ERROR);
}



char optimize(unsigned char *bmp_data, RGBQUAD *bmp_pal, CGBQUAD **tmp_pal, unsigned short rows, unsigned short columns, unsigned int rgbhex, unsigned short options, char status)
{
	char			rgb[15], rgb_dump[BMP_TILE_SIZE * sizeof(rgb)];
	unsigned char 	color, color_index, k, lut[256];
	unsigned short	i, j, *palette, rgb15, tiles, transparent, x, y;
	unsigned int	tile;


	if(status)															
		return status;

	tiles = rows * columns;

	*tmp_pal = malloc(sizeof(CGBQUAD) * tiles);
	if(*tmp_pal == NULL)
		return(ERR_MALLOC_TMPC);

	memset(*tmp_pal, 0xff, sizeof(CGBQUAD) * tiles);

	if(rgbhex >> 24)													
		transparent = (unsigned short) (((rgbhex & 0xff0000) >> 19) << 10 | ((rgbhex & 0xff00) >> 11) << 5 | ((rgbhex & 0xff) >> 3));
	else
		transparent = 0xffff;											

	for(i = 0; i < tiles; i++)											
	{
		tile = i * BMP_TILE_SIZE;
		memset(lut, 0xff, sizeof(lut));									

		for(j = 0; j < BMP_TILE_SIZE; j++)								
		{
			color = bmp_data[tile + j];
			lut[color] = 0;
		}

		if(options & FLAG_OBJ)
			color_index = 1;
		else
			color_index = 0;

		palette = (unsigned short *) (*tmp_pal + i);					
		memset(rgb_dump, 0, sizeof(rgb_dump));

		for(j = 0; j < sizeof(lut); j++)
		{
			if(lut[j] == 0)
			{
				if(color_index < MAX_COLORS)							
				{
					rgb15 = (bmp_pal[j].blue >> 3) << 10 | (bmp_pal[j].green >> 3) << 5 | (bmp_pal[j].red >> 3);

					if(rgb15 == transparent)							
					{
						palette[0] = transparent;						
						color_index--;									
					}
					else												
						palette[color_index] = rgb15;
				}
				snprintf(rgb, sizeof(rgb), "(%02x) #%02x%02x%02x, ", j, bmp_pal[j].green, bmp_pal[j].red, bmp_pal[j].blue);
				strcat(rgb_dump, rgb);									
				color_index++;
			}
		}

		if(color_index > MAX_COLORS)									
		{
			x = i + 1;
			y = 0;
 
			while(x > columns)
			{
				x -= columns;
				y++;
			}

			x--;
			rgb_dump[strlen(rgb_dump) - 2] = 0;							
			printf("\nError: tile at (%d, %d) uses %d colors: %s\n", (x * TILE_WIDTH), (y * TILE_HEIGHT), color_index, rgb_dump);

			if(options & FLAG_DEBUG)
			{
				printf("\n");
				for(y = 0; y < TILE_HEIGHT; y++)						
				{
					printf("\t");
					for(x = 0; x < TILE_WIDTH; x++)
					{
						printf("%02x ", bmp_data[tile + y * TILE_HEIGHT + x]);
					}
					printf("\n");
				}
				printf("\n");
			}
			else
				return(ERR_MAX_COLS2);
		}

		if((options & FLAG_UNOPT) == 0)
		{
			if(options & FLAG_OBJ)
				bubble_sort(palette, CHR_OBJ);							
			else
				bubble_sort(palette, CHR_BG);							
		}

		for(j = 0; j < sizeof(lut); j++)								
		{
			if(lut[j] == 0)
			{
				rgb15 = (bmp_pal[j].blue >> 3) << 10 | (bmp_pal[j].green >> 3) << 5 | (bmp_pal[j].red >> 3);
				for(k = 0; k < MAX_COLORS; k++)
				{
					if(palette[k] == rgb15)
						lut[j] = k;
				}
			}
		}

		for(j = 0; j < BMP_TILE_SIZE; j++)								
		{
			color = bmp_data[tile + j];
			bmp_data[tile + j] = lut[color];
		}
	}

	return(NO_ERROR);
}



void bubble_sort(unsigned short *palette, unsigned char color_index)
{
	unsigned char	i, j;
	unsigned short	color;


	for(i = color_index; i < MAX_COLORS; ++i)							
	{
		for(j = 3; j >= i; --j)
		{
			if(palette[j - 1] == palette[j])							
				palette[j] = 0xffff;

			if(palette[j - 1] > palette[j])								
			{
				color = palette[j - 1];									
				palette[j - 1] = palette[j];
				palette[j] = color;
			}
		}
	}
}



char create_palettes(CGBQUAD *cgb_pal, unsigned char **tmp_idx, CGBQUAD *tmp_pal, unsigned short tiles, unsigned char *slot, unsigned short options, char status)
{
	unsigned char	dci, dps, sci, used_colors;
	unsigned short	*dst, i, *src, tile;
	int 			match = 0;											


	if(status)															
		return status;

	*tmp_idx = malloc(tiles);
	if (*tmp_idx == NULL)
		return(ERR_MALLOC_TMPI);

	memset(cgb_pal, 0xff, sizeof(CGBQUAD) * MAX_SLOTS);

	if(options & FLAG_UNOPT)											
	{
		for(tile = 0; tile < tiles; tile++)
		{
			src = (unsigned short *) &tmp_pal[tile];
			dst = (unsigned short *) &cgb_pal[*slot];

			if(*slot == 0)												
			{
				memcpy(dst, src, MAX_COLORS * sizeof(short));
				(*slot)++;
			} 
			else
			{
				for(i = 0; i < *slot; i++)								
				{
					match = memcmp(src, &cgb_pal[i], sizeof(CGBQUAD));
					if(match == 0)
						break;											
				}

				if(match != 0)
				{
					if(*slot < MAX_SLOTS)								
					{
						memcpy(dst, src, MAX_COLORS * sizeof(short));	
						(*slot)++;
					}
					else
						return(ERR_PASS);								
				}
			}
		}
	}
	else																
	{
		for(tile = 0; tile < tiles; tile++)								
		{
			src = (unsigned short *) &tmp_pal[tile];
			used_colors = count_colors(src);

			if(used_colors == 4)										
			{
				dst = (unsigned short *) &cgb_pal[*slot];

				if(*slot == 0)											
				{
					memcpy(dst, src, used_colors * sizeof(short));
					(*slot)++;
				} 
				else
				{
					for(i = 0; i < *slot; i++)							
					{
						match = memcmp(src, &cgb_pal[i], sizeof(CGBQUAD));
						if(match == 0)
							break;										
					}

					if(match != 0)
					{
						if(*slot < MAX_SLOTS)							
						{
							memcpy(dst, src, used_colors * sizeof(short));	
							(*slot)++;
						}
						else
						{
							return(ERR_PASS1);							
						}
					}
				}
			}
		}

		for(tile = 0; tile < tiles; tile++)								
		{
			src = (unsigned short *) &tmp_pal[tile];
			used_colors = count_colors(src);

			if(used_colors == 3)										
			{	
				dst = (unsigned short *) &cgb_pal[*slot];

				if(*slot == 0)											
				{
					memcpy(dst, src, used_colors * sizeof(short));
					(*slot)++;
				}
				else
				{														
					match = find_palette(src, cgb_pal, *slot, used_colors);

					if(match == 0)										
					{
						if(*slot < MAX_SLOTS)							
						{
							memcpy(dst, src, used_colors * sizeof(short));
							(*slot)++;
						}
						else
						{
							return(ERR_PASS2);							
						}
					}

					if(match > 0)										
					{
						dps = (unsigned char) (match >> 4);				
						dst = (unsigned short *) &cgb_pal[dps];
						dci = (unsigned char) ((match & 0x0C) >> 2);	
						sci = (unsigned char) (match & 0x03);			
						dst[dci] = src[sci];
					}
				}
			}
		}

		for(tile = 0; tile < tiles; tile++)								
		{
			src = (unsigned short *) &tmp_pal[tile];
			used_colors = count_colors(src);

			if(used_colors == 2)										
			{
				dst = (unsigned short *) &cgb_pal[*slot];

				if(*slot == 0)											
				{
					memcpy(dst, src, used_colors * sizeof(short));
					(*slot)++;
				}
				else
				{														
					match = find_palette(src, cgb_pal, *slot, used_colors);

					if(match == 0)										
					{
						if(*slot < MAX_SLOTS)							
						{
							memcpy(dst, src, used_colors * sizeof(short));
							(*slot)++;
						}
						else
						{
							return(ERR_PASS3);							
						}
					}

					if(match > 0)										
					{
						dps = (unsigned char) (match >> 4);				
						dst = (unsigned short *) &cgb_pal[dps];
						dci = (unsigned char) ((match & 0x0C) >> 2);	
						sci = (unsigned char) (match & 0x03);			

						switch(sci)
						{
							case 0:
								dst[dci] = src[sci];
								break;

							case 1:
								dst[dci] = src[sci];
								break;

							case 2:
								dst[dci] = src[0];
								dci++;
								dst[dci] = src[1];
								break;

							default:
								break;
						}
					}
				}
			}
		}

		for(tile = 0; tile < tiles; tile++)								
		{	

			src = (unsigned short *) &tmp_pal[tile];
			used_colors = count_colors(src);

			if(used_colors == 1)										
			{
				dst = (unsigned short *) &cgb_pal[*slot];

				if(*slot == 0)											
				{
					*dst = *src;
					(*slot)++;
				}
				else
				{														
					match = find_palette(src, cgb_pal, *slot, used_colors);

					if(match == 0)										
					{
						if(*slot < MAX_SLOTS)							
						{
							*dst = *src;
							(*slot)++;
						}
						else
						{
							return(ERR_PASS4);							
						}

					}

					if(match > 0)										
					{
						dps = (unsigned char) (match >> 4);				
						dst = (unsigned short *) &cgb_pal[dps];
						dci = (unsigned char) ((match & 0x0C) >> 2);	
						sci = (unsigned char) (match & 0x03);			
						dst[dci] = src[sci];
					}
				}
			}
		}
	}

	if(options & FLAG_UNOPT)
	{
		for(tile = 0; tile < tiles; tile++)								
		{
			src = (unsigned short *) &tmp_pal[tile];

			for(i = 0; i < *slot; i++)									
			{
				match = memcmp(src, &cgb_pal[i], sizeof(CGBQUAD));
				if(match == 0)
					break;
			}

			if(match == 0)
				*(*tmp_idx + tile) = (unsigned char) i;					
			else
				return(ERR_TMPI_FAILED);
		}
	}
	else
	{
		if(options & FLAG_OBJ)
			sort_palettes(cgb_pal, *slot, CHR_OBJ);
		else
			sort_palettes(cgb_pal, *slot, CHR_BG);

		for(tile = 0; tile < tiles; tile++)								
		{
			src = (unsigned short *) &tmp_pal[tile];
			used_colors = count_colors(src);
			match = match_palette(src, cgb_pal, *slot, used_colors);

			if(match)
				*(*tmp_idx + tile) = (unsigned char) --match;
			else
				return(ERR_TMPI_FAILED);
		}
	}

	printf("Palette usage: %d/8 slots\n", *slot);
	return 0;
}




char count_colors(unsigned short *slot)
{
	unsigned char color_index, used_colors = 0;


	for(color_index = 0; color_index < MAX_COLORS; color_index++)
	{
		if(slot[color_index] == 0xffff)
			break;
		used_colors++;
	}

	return used_colors;
}




char match_palette(unsigned short *src, CGBQUAD *cgb_pal, unsigned char used_slots, unsigned char used_colors)
{
	unsigned char	i, j, k, match;
	unsigned short	*dst;


	for(i = 0; i < used_slots; i++)										
	{
		match = 0;
		dst = (unsigned short *) &cgb_pal[i];

		for(j = 0; j < used_colors; j++)								
		{
			for(k = 0; k < MAX_COLORS; k++)								
			{
				if(src[j] == dst[k])
					match++;
			}
		}
		
		if(used_colors == match)										
			return ++i;													
	}

	return 0;															
}




char find_palette(unsigned short *src, CGBQUAD *cgb_pal, unsigned char used_slots, unsigned char used_colors)
{
	unsigned char	empty = 0, i, j, k, match, v1 = 0xff, v2 = 0xff, v3c = 0, v3i = 0;
	unsigned short	*dst;


	match = match_palette(src, cgb_pal, used_slots, used_colors);

	if(match)
		return -1;														

	for(i = 0; i < used_slots; i++)										
	{
		dst = (unsigned short *) &cgb_pal[i];
		for(j = 0; j < used_colors; j++)								
		{
			empty = 0;
			match = 0;
			
			for(k = 0; k < 4; k++)										
			{
				if(dst[k] == src[j])
					match++;											
				if(dst[k] == 0xffff)
					empty++;											
			}

			switch(used_colors)
			{
				case 1:													
					if(empty > 0) 										
						return (i << 4 | (4 - empty) << 2);				
					break;

				case 2:													
					if(match == 1 && empty > 0)							
						v1 = j ^ 1;										
					if(match == 0 && empty == 2)						
						v2 = 2;											
					break;

				case 3:													
					if(match == 0 && empty == 1)						
					{
						v3c++;											
						v3i = j;										
					}
					break;

				default:
					return(ERR_WRONG_PAL);								
					break;
			}
		}

		if(v1 != 0xff)													
			return (i << 4 | (4 - empty) << 2 | v1);					
		if(v2 != 0xff)													
			return (i << 4 | (4 - empty) << 2 | v2);					

		if(v3c == 1)													
			return (i << 4 | 3 << 2 | v3i);								
	}

	return 0;															
}



void sort_palettes(CGBQUAD *cgb_pal, unsigned char used_slots, unsigned char mode)
{
	unsigned char	i;
	unsigned short	*slot;


	for(i = 0; i < used_slots; i++)
	{
		slot = (unsigned short *) &cgb_pal[i];
		bubble_sort(slot, mode);
	}
}



void remap_colors(unsigned char *bmp_data, CGBQUAD *cgb_pal, unsigned char *tmp_idx, CGBQUAD *tmp_pal, unsigned short tiles, char status)
{
	unsigned char	color, i, j, palette[MAX_COLORS] = {0}, *ptr, slot, used_colors;
	unsigned short	*dst, *src, tile;
	

	if(status)															
		return;

	for(tile = 0; tile < tiles; tile++)
	{
		src = (unsigned short *) &tmp_pal[tile];
		slot = tmp_idx[tile];
		dst = (unsigned short *) &cgb_pal[slot];
		used_colors = count_colors(src);

		for(i = 0; i < used_colors; i++)
		{
			for(j = 0; j < MAX_COLORS; j++)
			{
				if(src[i] == dst[j])
					palette[i] = j;
			}
		}

		ptr = (unsigned char *) &bmp_data[tile * BMP_TILE_SIZE];

		for(i = 0; i < BMP_TILE_SIZE; i++)
		{
			color = ptr[i];
			ptr[i] = palette[color];
		}
	}
}




char convert(unsigned char *bmp_data, unsigned char **cgb_chr, unsigned char *cgb_map, unsigned char *cgb_atr, unsigned char *tmp_idx, unsigned short tiles, unsigned short padding, unsigned short *used_tiles, unsigned short options, char status)
{
	unsigned char	bank = 0, *dst, flip, hi, k, lo, *src;
	unsigned char	empty[BMP_TILE_SIZE], normal[BMP_TILE_SIZE], x_flipped[BMP_TILE_SIZE], y_flipped[BMP_TILE_SIZE], xy_flipped[BMP_TILE_SIZE];
	unsigned short	dupes[5], i, j;
	int				match, tile; 										
																		

	if(status)															
		return status;

	memset(cgb_map, 0, MAX_MAP_SIZE);
	memset(cgb_atr, 0, MAX_MAP_SIZE);
	memset(dupes, 0, sizeof(dupes));
	memset(empty, 0, BMP_TILE_SIZE);

	for(i = 0; i < tiles; i++)											
	{
		tile = i * BMP_TILE_SIZE;
		memcpy(&normal, &bmp_data[tile], sizeof(normal));				

		match = 0;
		if(options & FLAG_OBJ)											
			match = memcmp(normal, empty, BMP_TILE_SIZE);				

		if((options & FLAG_OBJ) == 0 || match != 0)						
		{
			for(j = 0; j < TILE_HEIGHT; j++)							
			{
				src = &normal[j * TILE_HEIGHT];
				dst = &x_flipped[j * TILE_HEIGHT];
				for(k = TILE_WIDTH; k > 0; k--)
					dst[TILE_WIDTH - k] = src[k - 1];
			}
		
			for(j = TILE_HEIGHT; j > 0; j--)							
			{
				src = &normal[j * TILE_HEIGHT - TILE_WIDTH];
				dst = &y_flipped[(TILE_HEIGHT - j) * TILE_HEIGHT];
				memcpy(dst, src, TILE_WIDTH);
			}

			for(j = 0; j < TILE_HEIGHT; j++)							
			{
				src = &y_flipped[j * TILE_HEIGHT];
				dst = &xy_flipped[j * TILE_HEIGHT];
				for(k = TILE_WIDTH; k > 0; k--)
					dst[TILE_WIDTH - k] = src[k - 1];
			}

			flip = 0xff;												

			if((options & FLAG_DUMP) == 0)								
			{
				for(j = 0; j < *used_tiles; j++)						
				{
					src = &bmp_data[j * BMP_TILE_SIZE];

					match = memcmp(src, normal, BMP_TILE_SIZE);
					if(match == 0)
					{
						++dupes[0];
						flip = 0;
						break;
					}

					if((options & FLAG_FLIPX) == 0)
					{
						match = memcmp(src, x_flipped, BMP_TILE_SIZE);
						if(match == 0)
						{
							++dupes[1];
							flip = 0x20;
							break;
						}
					}

					if((options & FLAG_FLIPY) == 0)
					{
						match = memcmp(src, y_flipped, BMP_TILE_SIZE);
						if(match == 0)
						{
							++dupes[2];
							flip = 0x40;
							break;
						}
					}

					if((options & FLAG_FLIPXY) == 0)
					{
						match = memcmp(src, xy_flipped, BMP_TILE_SIZE);
						if(match == 0)
						{
							++dupes[3];
							flip = 0x60;
							break;
						}
					}
				}
			}
		

			if(flip == 0xff)											
			{
				if(*used_tiles + padding < MAX_TILES)
				{
					dst = (unsigned char *) &bmp_data[*used_tiles * BMP_TILE_SIZE];
					memcpy(dst, normal, BMP_TILE_SIZE);
					cgb_map[i] = (unsigned char) ((*used_tiles + padding) & 0xff);
					flip++;												
					bank = (unsigned char) (((*used_tiles + padding) & 0x100) >> 5);
					(*used_tiles)++;
				}
				else
					return(ERR_MAX_TILES);
			}
			else														
			{
				cgb_map[i] = (unsigned char) ((j + padding) & 0xff);
				bank = (unsigned char) (((j + padding) & 0x100) >> 5);
			}

			cgb_atr[i] = tmp_idx[i] + bank + flip;						

			if(options & FLAG_OBJ)
				cgb_atr[i] |= 0x10;										
		}
		else
			++dupes[4];
	}

	*cgb_chr = (unsigned char *) malloc(*used_tiles * CGB_TILE_SIZE);
	if (*cgb_chr == NULL)
		return(ERR_MALLOC_CGBT);

	for(i = 0; i < *used_tiles; i++)									
	{
		dst = (*cgb_chr + (i * 16));
		for(j = 0; j < TILE_HEIGHT; j++)
		{
			lo = 0;
			hi = 0;
			src = (unsigned char *) &bmp_data[(i * BMP_TILE_SIZE) + (j * 8)];
			for(k = 0; k < TILE_WIDTH; k++)
			{
				lo = (lo << 1) | (src[k] & 0x01);
				hi = (hi << 1) | (src[k] & 0x02) >> 1;
			}
			dst[j*2] = lo;
			dst[(j*2)+1] = hi;
		}
	}

	printf("Chars used: %d (%d duplicates removed: %d normal, %d horizontal, %d vertical, %d horizontal & vertical", *used_tiles, tiles - *used_tiles, dupes[0], dupes[1], dupes[2], dupes[3]);
	if(options & FLAG_OBJ)
		printf(", %d transparent)\n", dupes[4]);
	else
		printf(")\n");

	return 0;
}



void expand_maps(unsigned char *cgb_map, unsigned char *cgb_atr, unsigned short rows, unsigned short columns, unsigned char chr, char status)
{
	unsigned char	*dst, *src;


	if(status)
		return;

	do
	{
		columns--;
		src = &cgb_map[columns * rows];
		dst = &cgb_map[columns * 32];
		memmove(dst, src, rows);
		memset((dst + rows), chr, 32 - rows);

		src = &cgb_atr[columns * rows];
		dst = &cgb_atr[columns * 32];
		memmove(dst, src, rows);
		memset((dst + rows), 0, 32 - rows);
	}
	while (columns > 0);
}



void save_oam(char *fname, char *ext, unsigned char *cgb_atr, unsigned char *cgb_map, unsigned short map_size)
{
	char			fn[FILENAME_MAX];
	unsigned char	buffer[] = "%00000000", j;
	unsigned short	i;
	FILE 			*fp = NULL;


	strcpy(fn, fname);													
	strcat(fn, ext);													

	fp = fopen(fn, "w");												

	fprintf(fp, ";                     7        6       5       4      3       2        1        0    \n");
	fprintf(fp, "; y, x, chr, atr (priority, v-flip, h-flip, dmg pal, bank, palette, palette, palette)\n\n");
	fprintf(fp, "%s_oam:\n", fname);
	for(i = 0; i < map_size; i++)
	{
		if(cgb_atr[i] & 0x10)
		{
			cgb_atr[i] &= 0xef;

			for(j = 8; j > 0; j--)
			{
				if (cgb_atr[i] & 1)
					buffer[j] = 0x31;
				else
					buffer[j] = 0x30;
				cgb_atr[i] >>= 1;
			}

			fprintf(fp, "\tdb\t$00, $00, $%02x, %s\n", cgb_map[i], buffer);
		}
	}

	fclose(fp);
}


void rebase(unsigned char *cgb_map, unsigned short map_size)
{
	unsigned short	i;


	for(i = 0; i < map_size; i++)
		cgb_map[i] = (cgb_map[i] + 0x80) & 0xff;
}
