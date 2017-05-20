#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bmp2cgb.h"

// globals

BITMAPHEADER	*hdr = NULL;		// bmp header
RGBQUAD			*bmiColors = NULL;	// bmp palette
u8				*bmiData = NULL;	// bmp image data
CGBQUAD 		*tmpColors = NULL;	// 4 color cgb style palette for each tile created from bmp
u8				*tmpIndex = NULL;	// 'middleman' between tmpColors and cgbPalettes, supports creation of cgbAtrb
u8				*cgbTiles = NULL;
u8				cgbMap[MAX_MAP_SIZE];
u8				cgbAtrb[MAX_MAP_SIZE];
CGBQUAD 		cgbPalettes[MAX_PALETTES];
u8				options = 0;


// haelp!
u8 usage(void) {
	printf("syntax: bmp2cgb.exe [options] picture[.bmp]\n\n");
	printf("options:\n");
	printf("\t-d  - do not optimize tileset\n");
	printf("\t-i  - dump information, no output data is created\n");
	printf("\t-p# - push tileset right by 1-255 characters\n");
	printf("\t-q# - push first palette to 1-7 slot\n");
	printf("\t-r# - pad maps to 32 characters, fill difference with tile 0-255\n");
	printf("\t-x  - disable horizontal flip optimization\n");
	printf("\t-y  - disable vertical flip optimization\n");
	printf("\t-z  - disable horizontal & vertical flip optimization\n");
	printf("\n");
	exit(EXIT_FAILURE);
}


// error handler
void error(u8 msg) {

	if(msg > 0)
		printf("\nError: ");

	switch(msg) {
		case ERR_SILENT		:	break;
		case ERR_NOT_FOUND	:	printf("Input file not found!\n"); break;
		case ERR_MALLOC_HDR	:	printf("Memory allocation for BMP header failed!\n"); break;
		case ERR_NOT_BMP	:	printf("BMP signature missing!\n"); break;
		case ERR_NOT_8BPP	:	printf("Only 256 color BMPs (8 bits per pixel) are supported!\n"); break;
		case ERR_BI_RLE		:	printf("No BMP compression is allowed!\n"); break;
		case ERR_NOT_ROUNDED:	printf("Height and width must be a multiple of 8!\n"); break;
		case ERR_TOO_BIG	:	printf("Image too big - character map wouldn't fit in one bank!\n"); break;
		case ERR_MALLOC_BMIC:	printf("Memory allocation for BMP palette failed!\n"); break;
		case ERR_MALLOC_BMID:	printf("Memory allocation for BMP data failed!"); break;
		case ERR_MALLOC_TMPC:	printf("Memory allocation for palette helper failed!\n"); break;
		case ERR_MALLOC_TMPI:	printf("Memory allocation for palette index failed!\n"); break;
		case ERR_MALLOC_BMIT:	printf("Memory allocation for BMP data helper failed!\n"); break;
		case ERR_MAX_TILES	:	printf("Maximum tiles limit exceeded!\n"); break;
		case ERR_MALLOC_CGBT:	printf("Memory allocation for CGB tiles failed!\n"); break;
		case ERR_PASS1		:	printf("Pass 1 failed! Maximum of 8 palettes reached!\n"); break;
		case ERR_PASS2		:	printf("Pass 2 failed! Maximum of 8 palettes reached!\n"); break;
		case ERR_PASS3		:	printf("Pass 3 failed! Maximum of 8 palettes reached!\n"); break;
		case ERR_PASS4		:	printf("Pass 4 failed! Maximum of 8 palettes reached!\n"); break;
		case ERR_TMPI_FAILED:	printf("Building palette index helper failed!\n"); break;
		case ERR_UNK_OPTION :	printf("Invalid option!\n"); break;
		case ERR_WRONG_PAL	:	printf("findPalette supports only 1 or 2 color palettes!\n"); break;
		case ERR_PADDING	:	printf("Padding allowed for maps below 32 * 32 chars only!\n"); break;
		default				:	printf("Undefined error code!\n"); break;
	}

	release();

	exit(EXIT_FAILURE);
}


// load bitmap header, check neccessary values, allocate memory and read data
void loadBMP(char *fname) {
	FILE	*ifp;

	ifp = fopen(fname, "rb");
	if(ifp == NULL)
		error(ERR_NOT_FOUND);

	hdr = (BITMAPHEADER *) malloc(sizeof(BITMAPHEADER));
	if(!hdr)
		error(ERR_MALLOC_HDR);
	fread(hdr, sizeof(BITMAPHEADER), 1, ifp);

	if (hdr->bfType != 0x4D42)
		error(ERR_NOT_BMP);

	if (hdr->biBitCount != 8)
		error(ERR_NOT_8BPP);

	if (hdr->biCompression != BI_RGB)
		error(ERR_BI_RLE);

	if ((hdr->biWidth & 7) || (hdr->biHeight & 7))
		error(ERR_NOT_ROUNDED);

	if ((hdr->biWidth / TILE_WIDTH) * (hdr->biHeight / TILE_HEIGHT) > MAX_MAP_SIZE)
		error(ERR_TOO_BIG);

	bmiColors = (RGBQUAD *) malloc(sizeof(RGBQUAD) * BI_COLORS);
	if (!bmiColors)
		error(ERR_MALLOC_BMIC);
	fread(bmiColors, sizeof(RGBQUAD), BI_COLORS, ifp);

	bmiData = (u8 *) malloc(hdr->biWidth * hdr->biHeight);
	if (!bmiData)
		error(ERR_MALLOC_BMID);
	fread(bmiData, hdr->biWidth, hdr->biHeight, ifp);

	fclose(ifp);

	printf("Bitmap size: %d * %d px\n", hdr->biWidth, hdr->biHeight);
	printf("Character/Attribute map: %d * %d chars\n", hdr->biWidth / 8, hdr->biHeight / 8);
}


// flip bitmap and convert it to tile mode for further processing
// looks a bit messy? maybe i'll fix it someday :)
void prepareBMP(void) {
	u8	tmp, *bmiDataTmp = NULL;
	u32 x, y, map_x, map_y, tmp_x, tmp_y;

	// "upside-down" to normal image raster scan order
	for(y = 0; y < (hdr->biHeight / 2); y++) {
		for(x = 0; x < hdr->biWidth; x++) {
			tmp = bmiData[(y * hdr->biWidth) + x];
 			bmiData[(y * hdr->biWidth) + x] = bmiData[((hdr->biHeight - y) * hdr->biWidth - hdr->biWidth) + x];
			bmiData[((hdr->biHeight - y) * hdr->biWidth - hdr->biWidth) + x] = tmp;
		}
	}

	bmiDataTmp = (u8 *) malloc(hdr->biWidth * hdr->biHeight);
	if (!bmiDataTmp)
		error(ERR_MALLOC_BMIT);

	// linear to tile conversion
	for(map_y = 0; map_y < (hdr->biHeight / TILE_HEIGHT); map_y++) {
		tmp_y = map_y * TILE_HEIGHT * hdr->biWidth;
		for(map_x = 0; map_x < (hdr->biWidth / TILE_WIDTH); map_x++) {
			tmp_x = map_x * TILE_WIDTH;
			for(y = 0; y < TILE_HEIGHT; y++) {
				for(x = 0; x < TILE_WIDTH; x++) {
					bmiDataTmp[tmp_y + tmp_x * TILE_HEIGHT + y * TILE_HEIGHT + x] = \
					bmiData[tmp_y + tmp_x + y * hdr->biWidth + x];
				}
			}
		}
	}

	memcpy(bmiData, bmiDataTmp, (hdr->biWidth * hdr->biHeight));
	free(bmiDataTmp);
}


// pre converts bitmap data: creates and sorts palette for each tile, then remaps tile and checks if it contains no more than 4 colors
void processBMP(void) {
	u8	used, tmp;
	u8	palette[256] = {0};
	u16 map_x, map_y, *ptr, row, rgb15, tile;
	u16 tiles = (hdr->biWidth / TILE_WIDTH) * (hdr->biHeight / TILE_HEIGHT);
	u32 index, i, j;

	tmpColors = (CGBQUAD *) malloc(sizeof(CGBQUAD) * (hdr->biWidth / TILE_WIDTH) * (hdr->biHeight / TILE_HEIGHT));
	if (!tmpColors)
		error(ERR_MALLOC_TMPC);

	memset(tmpColors, 0xff, sizeof(CGBQUAD) * (hdr->biWidth / TILE_WIDTH) * (hdr->biHeight / TILE_HEIGHT));

	for(tile = 0; tile < tiles; tile++){
		index = tile * BI_TILE_SIZE;
		// reset palette LUT to default
		memset(palette, 0xFF, sizeof(palette));

		// use palette LUT to mark all colors used in tile (color = index)
		for(i = 0; i < BI_TILE_SIZE; i++){
			tmp = bmiData[index + i];
			palette[tmp] = 0;
		}
		// tmpColors contains 4 color (15 bit) cgb palette for each tile
		ptr = (u16*) &tmpColors[tile];

		// count, enumerate and store first 4 colors used in tile
		used = 0;
		for(i = 0; i < sizeof(palette); i++) {
			if(palette[i] == 0) {
				if(used < 4) {
					rgb15 = (bmiColors[i].rgbBlue >> 3) << 10 | (bmiColors[i].rgbGreen >> 3) << 5 | (bmiColors[i].rgbRed >> 3);
					ptr[used] = rgb15;
				}
				used++;
			}
		}

		// debug
		if(used > 4) {
			row = (hdr->biWidth / TILE_WIDTH);
			map_x = (tile - (tile / row * row)) * TILE_WIDTH;
			map_y = (tile / row) * TILE_HEIGHT;
			printf("Error: tile at %d * %d uses %d colors, no more than 4 is allowed!\n", map_x, map_y, used);
			if((options & FLAG_INFO) == 0)
				error(ERR_SILENT);
		}

		// bubble sort palette entries to allow further removal of duplicates
		for(i = 1; i < 4; ++i) {
			for(j = 4 - 1; j >= i; --j) {
				// compare adjacent elements
				if(ptr[j - 1] > ptr[j]) {
					// exchange elements
					rgb15 = ptr[j - 1];
					ptr[j - 1] = ptr[j];
					ptr[j] = rgb15;
				}
			}
		}

		// reorder used colors according to sorted palette
		for(i = 0; i < sizeof(palette); i++) {
			if(palette[i] == 0) {
				rgb15 = (bmiColors[i].rgbBlue >> 3) << 10 | (bmiColors[i].rgbGreen >> 3) << 5 | (bmiColors[i].rgbRed >> 3);
				for(j = 0; j < 4; j++) {
					if(ptr[j] == rgb15)
						palette[i] = j;
				}
			}
		}

		// prepare tile for 8 -> 1 bit conversion applying new color map
		for(i = 0; i < BI_TILE_SIZE; i++){
			tmp = bmiData[index + i];
			bmiData[index + i] = palette[tmp];
		}
	}
}


// removes duplicates and shrinks palettes below 4 colors, generates sorted cgbPalettes and tmpIndex
u8 createPalettes(u8 slot) {
	u8 match = 0, colors_used, empty, palette, palettes_used, pidx, cidx;
	u16 i, *dst, *src, tile;
	u16 tiles = (hdr->biWidth / TILE_WIDTH) * (hdr->biHeight / TILE_HEIGHT);

	tmpIndex = malloc(sizeof(u8) * (hdr->biWidth / TILE_WIDTH) * (hdr->biHeight / TILE_HEIGHT));
	if (!tmpIndex)
		error(ERR_MALLOC_TMPI);

	memset(cgbPalettes, 0xff, sizeof(CGBQUAD) * MAX_PALETTES);

	palettes_used = slot;
	
	// 1st pass
	for(tile = 0; tile < tiles; tile++) {

		src = (u16*) &tmpColors[tile];
		colors_used = tileColors(src);

		// process 4 colors palettes only
		if(colors_used == 4) {
			dst = (u16*) &cgbPalettes[palettes_used];
			// first palette can be copied w/out checking
			if(palettes_used == 0) {
				memcpy(dst, src, colors_used * 2);
				palettes_used++;
			} else { // another palettes are checked before being copied
				for(i = 0; i < palettes_used; i++) {
					match = memcmp(&cgbPalettes[i], &tmpColors[tile], 8);
					if(match == 0)
						break;
				}

				if(match != 0) {
					// check if there's free space in palettes table
					if(palettes_used < MAX_PALETTES) {
						// copy palette
						memcpy(dst, src, colors_used * 2);
						palettes_used++;
					} else {
						error(ERR_PASS1);
					}
				}
			}
		}
	}

	// 2nd pass
	for(tile = 0; tile < tiles; tile++) {

		src = (u16*) &tmpColors[tile];
		// count colors in palette for processed tile
		colors_used = tileColors(src);

		// process 3 colors palettes only
		if(colors_used == 3) {
			// first palette can be copied w/out checking
			if(palettes_used == 0) {
				dst = (u16*) &cgbPalettes[palettes_used];
				memcpy(dst, src, colors_used * 2);
				palettes_used++;
			} else { 
				// another palettes are checked before being copied
				match = matchPalette(src, palettes_used, colors_used) & 0x0F;

				// add palette if there's no match
				if(match == 0) {
					// check if there's free space in palettes table
					if(palettes_used < MAX_PALETTES) {
						dst = (u16*) &cgbPalettes[palettes_used];
						memcpy(dst, src, colors_used * 2);
						palettes_used++;
					} else {
						error(ERR_PASS2);
					}
				}
			}
		}
	}

	// 3rd pass
	for(tile = 0; tile < tiles; tile++) {

		src = (u16*) &tmpColors[tile];
		// count colors in palette for processed tile
		colors_used = tileColors(src);

		// process 2 colors palettes only
		if(colors_used == 2) {
			// first palette can be copied w/out checking
			if(palettes_used == 0) {
				dst = (u16*) &cgbPalettes[palettes_used];
				memcpy(dst, src, colors_used * 2);
				palettes_used++;
			} else { 
				// another palettes are checked before being copied
				match = matchPalette(src, palettes_used, colors_used) & 0x0F;

				// add palette if there's no match
				if(match == 0) {
					// check if there's free space in palettes table
					if(palettes_used < MAX_PALETTES) {
						// check if there's palette w/ common color and free slot(s)
						empty = findPalette(src, palettes_used, colors_used);
						// no such palette, add new entry
						if(empty == 0) {
							dst = (u16*) &cgbPalettes[palettes_used];
							memcpy(dst, src, colors_used * 2);
							palettes_used++;
						// found free space in existing palette
						} else {
							palette = empty >> 4;
							// hi - palette that has dupe color and free slot
							dst = (u16*) &cgbPalettes[palette];
							// index
							pidx = (empty & 0x0C) >> 2;
							// how many colors 1-2
							cidx = (empty & 0x03);

							switch(cidx) {
								case 0:
									dst[pidx] = src[cidx];
									break;
								case 1:
									dst[pidx] = src[cidx];
									break;
								case 2:
									dst[pidx] = src[0];
									pidx++;
									dst[pidx] = src[1];
									break;
								default:
									break;
							}
						}
					} else {
						error(ERR_PASS3);
					}
				}
			}
		}
	}

	// 4th pass
	for(tile = 0; tile < tiles; tile++) {

		src = (u16*) &tmpColors[tile];
		// count colors in palette for processed tile
		colors_used = tileColors(src);

		// process 1 color palettes only
		if(colors_used == 1) {
			// first palette can be copied w/out checking
			if(palettes_used == 0) {
				dst = (u16*) &cgbPalettes[palettes_used];
				dst[0] = src[0];
				palettes_used++;
			} else { 
				// another palettes are checked before being copied
				match = matchPalette(src, palettes_used, colors_used) & 0x0F;

				// add palette if there's no match
				if(match == 0) {
					// check if there's free space in palettes table
					if(palettes_used < MAX_PALETTES) {
						// check if there's palette w/ common color and free slot(s)
						empty = findPalette(src, palettes_used, colors_used);
						// no such palette, add new entry
						if(empty == 0) {
							dst = (u16*) &cgbPalettes[palettes_used];
							dst[0] = src[0];
							palettes_used++;
						} else {
							palette = empty >> 4;
							// hi - palette that has dupe color and free slot
							dst = (u16*) &cgbPalettes[palette];
							pidx = (empty & 0x0C) >> 2;
							cidx = (empty & 0x03);
							dst[pidx] = src[cidx];
						}
					} else {
						error(ERR_PASS4);
					}
				}
			}
		}
	}
	// sort palettes
	bubbleSort(palettes_used);

	// index tmpColors according to newly created cgbPalettes, this LUT will simplify creating cgbAtrb later on
	for(tile = 0; tile < tiles; tile++) {
		src = (u16*) &tmpColors[tile];
		colors_used = tileColors(src);
		match = matchPalette(src, palettes_used, colors_used);
		palette = (match & 0xF0) >> 4;
		match = match & 0x0F;

		if(match == 1) {
			tmpIndex[tile] = palette;
		} else {
			error(ERR_TMPI_FAILED);
		}
	}

	printf("Palettes usage: %d/8 slots\n", palettes_used - slot);
	return palettes_used - slot;
}


// find duplicate entries and/or free space in cgbPalettes to fit new 2 and 1 color palettes
// return: 0 if there's no match; hi - matching palette, lo: hi - free entry index, lo - color(s)
u8 findPalette(u16 *src, u8 palettes_used, u8 colors_used) {
	u8 i, j, k, empty = 0, match, v1 = 0xFF, v2 = 0xFF;
	u16 *dst;

	for(i = 0; i < palettes_used; i++) {
		dst = (u16*) &cgbPalettes[i];
		for(j = 0; j < colors_used; j++) {
			empty = 0;
			match = 0;
			// color by color, they might be on different positions
			for(k = 0; k < 4; k++) {
				if(dst[k] == src[j])
					match++;
				if(dst[k] == 0xFFFF)
					empty++;
			}
			switch(colors_used) {
				case 1:
					if(match == 0 && empty > 0) // 1 color palette not found by matchPalette()
						return (i << 4 | (4 - empty) << 2); // hi - palette, lo : hi - index, lo = 0, doesn't matter
					break;
				case 2:
					if(match == 1 && empty > 0) // 2 colors palette - add 1 color
						v1 = j ^ 1; // not one that matches but the other
					if(match == 0 && empty == 2) // 2 colors palette - add 2 colors
						v2 = 2; // copy both
					break;
				default:
					error(ERR_WRONG_PAL);
					break;
			}
		}
		// this is to prevent returninig first solution found for 2 colors palette, we want best possible ie. v1
		if(v1 != 0xFF)
			return (i << 4 | (4 - empty) << 2 | v1);	// hi - palette, lo : hi - index, lo - color to store
		if(v2 != 0xFF)
			return (i << 4 | (4 - empty) << 2 | v2); // hi - palette, lo : hi - index, lo = 2 colors
	}
	return 0;
}


// find duplicate palette entries in cgbPalettes
// return: 0 if there's no match; hi - matching palette, lo - 1 if it's a dupe
u8 matchPalette(u16 *src, u8 palettes_used, u8 colors_used) {
	u8 i, j, k, match = 0;
	u16 *dst;

	for(i = 0; i < palettes_used; i++) {
		dst = (u16*) &cgbPalettes[i];
		match = 0;
		for(j = 0; j < colors_used; j++) {
			// color by color, they might be on different positions
			for(k = 0; k < 4; k++) {
				if(dst[k] == src[j])
					match++;
			}
		}
		// no need to continue, we have dupe
		if(match == colors_used) {
			match = (i << 4) | 1;
			return match;
		}
	}

	return 0;
}


// sort palette entries low to high, because i can..
void bubbleSort(u8 palettes_used) {
	u8 i, j, k;
	u16 color, *ptr;

	for(i = 0; i < palettes_used; i++) {
		ptr = (u16*) &cgbPalettes[i];
		for(j = 1; j < 4; ++j) {
			for(k = 4 - 1; k >= j; --k) {
				// compare adjacent elements
				if(ptr[k - 1] > ptr[k]) {
					// exchange elements
					color = ptr[k - 1];
					ptr[k - 1] = ptr[k];
					ptr[k] = color;
				}
			}
		}
	}
}


// remap color entries in every tile according to its entry in cgbPalettes
void remapTiles(void) {
	u8 color, colors_used, index, *ptr;
	u8 palette[4] = {0};
	u16 i, j, *dst, *src, tile;
	u16 tiles = (hdr->biWidth / TILE_WIDTH) * (hdr->biHeight / TILE_HEIGHT);
	
	for(tile = 0; tile < tiles; tile++) {
		src = (u16*) &tmpColors[tile];
		colors_used = tileColors(src);
		index = tmpIndex[tile];
		dst = (u16*) &cgbPalettes[index];

		for(i = 0; i < colors_used; i++) {
			for(j = 0; j < 4; j++) {
				if(dst[j] == src[i])
					palette[i] = j;
			}
		}

		ptr = (u8*) &bmiData[tile * BI_TILE_SIZE];

		for(i = 0; i < BI_TILE_SIZE; i++) {
			color = ptr[i];
			ptr[i] = palette[color];
		}
	}
}


// counts colors used in one tile
// return: number of colors
u8 tileColors(u16 *src) {
	u8 i, used = 0;

	for(i = 0; i < MAX_COLORS; i++) {
		if(src[i] == 0xFFFF)
			break;
		used++;
	}

	return used;
}


// ugly hack to solve memcmp issues in convertData(), can't nail the problem atm :/
u8 pmcmem(u8 *dst, u8 *src, u8 n) {
#ifdef __linux__
	u8 i;

	for(i = 0; i < n; i++) {
		if(dst[i] != src[i])
			return 1;
	}

	return 0;
#else
	return memcmp(dst, src, n);
#endif
}


// creates map, attr and finalizes tileset conversion
// return: tiles used after optimization
u16 convertData(u8 padding) {
	u8 bank = 0, flip, hi, lo, *src, *dst, match;
	u8 normal[64], x_flipped[64], y_flipped[64], xy_flipped[64];
	u16 i, j, k, dupes[4], tile, tiles_used;
	u16 tiles = (hdr->biWidth / TILE_WIDTH) * (hdr->biHeight / TILE_HEIGHT);
	u32 index;

	memset(cgbMap, 0, MAX_MAP_SIZE);
	memset(cgbAtrb, 0, MAX_MAP_SIZE);
	memset(dupes, 0, sizeof(dupes));
	tiles_used = 0;

	// process all tiles
	for(tile = 0; tile < tiles; tile++) {

		index = tile * BI_TILE_SIZE;

		// create copy of a tile
		memcpy(&normal, &bmiData[index], sizeof(normal));

		// create x flipped tile
		for(i = 0; i < TILE_HEIGHT; i++){
			src = &normal[i * TILE_HEIGHT];
			dst = &x_flipped[i * TILE_HEIGHT];
			for(j = TILE_WIDTH; j > 0; j--){
				dst[TILE_WIDTH - j] = src[j - 1];
			}
		}
		
		// create y flipped tile
		for(i = TILE_HEIGHT; i > 0; i--){
			src = &normal[i * TILE_HEIGHT - TILE_WIDTH];
			dst = &y_flipped[(TILE_HEIGHT - i) * TILE_HEIGHT];
			memcpy(dst, src, TILE_WIDTH);
		}

		// create x/y flipped tile mod
		for(i = 0; i < TILE_HEIGHT; i++){
			src = &y_flipped[i * TILE_HEIGHT];
			dst = &xy_flipped[i * TILE_HEIGHT];
			for(j = TILE_WIDTH; j > 0; j--){
				dst[TILE_WIDTH - j] = src[j - 1];
			}
		}

		flip = 1;
		// check if any variant of processed tile already exists in table
		for(i = 0; i < tiles_used; i++) {
			src = &bmiData[i * BI_TILE_SIZE];

			if((options & FLAG_DUPE) != 0) {
				break;
			}

			match = pmcmem(src, normal, BI_TILE_SIZE);
			if(match == 0) {
				++dupes[0];
				flip = 0;
				break;
			}

			if((options & FLAG_FLIPX) == 0) {
				match = pmcmem(src, x_flipped, BI_TILE_SIZE);
				if(match == 0) {
					++dupes[1];
					flip = 32;
					break;
				}
			}

			if((options & FLAG_FLIPY) == 0) {
				match = pmcmem(src, y_flipped, BI_TILE_SIZE);
				if(match == 0) {
					++dupes[2];
					flip = 64;
					break;
				}
			}

			if((options & FLAG_FLIPXY) == 0) {
				match = pmcmem(src, xy_flipped, BI_TILE_SIZE);
				if(match == 0) {
					++dupes[3];
					flip = 96;
					break;
				}
			}
		}

		if(flip == 1) {
			if(tiles_used + padding < MAX_TILES) {
				dst = (u8*) &bmiData[tiles_used * BI_TILE_SIZE];
				memcpy(dst, normal, BI_TILE_SIZE);
				cgbMap[tile] = (tiles_used + padding) & 0xFF;
				flip--;
				bank = ((tiles_used + padding) & 0xFF00) >> 5;
				tiles_used++;
			} else {
				error(ERR_MAX_TILES);
			}
		} else {
			cgbMap[tile] = (i + padding) & 0xFF;
			bank = ((i + padding) & 0xFF00) >> 5;
		}
		// refer to cgb manual
		cgbAtrb[tile] = tmpIndex[tile] + bank + flip;
	}

	cgbTiles = (u8 *) malloc(tiles_used * CGB_TILE_SIZE);
	if (!cgbTiles)
		error(ERR_MALLOC_CGBT);

	// convert 8bpp to interleaved 1bpp gb style
	for(i = 0; i < tiles_used; i++) {
		dst = cgbTiles + (i * 16);
		for(j = 0; j < TILE_HEIGHT; j++) {
			lo = 0;
			hi = 0;
			src = (u8*) &bmiData[(i * BI_TILE_SIZE) + (j * 8)];
			for(k = 0; k < TILE_WIDTH; k++) {
				lo = (lo << 1) | (src[k] & 0x01);
				hi = (hi << 1) | (src[k] & 0x02) >> 1;
			}
			dst[j*2] = lo;
			dst[(j*2)+1] = hi;
		}
	}
	printf("Tiles used: %d (%d duplicates removed: %d normal, %d horizontal, %d vertical, %d horizontal & vertical)\n", tiles_used, tiles - tiles_used, dupes[0], dupes[1], dupes[2], dupes[3]);
	return tiles_used;
}


// pad map below 32 tiles width
u8 padmaps(u8 chr) {

	u8 i, map_x, map_y, padding, *dst, *src;

	map_x = hdr->biWidth / TILE_WIDTH;
	map_y = hdr->biHeight / TILE_HEIGHT;
	if(map_x >= CGB_MAP_X || map_y >= CGB_MAP_Y)
		error(ERR_PADDING);
	padding = CGB_MAP_X - map_x;

	for(i = map_y; i > 0; i--) {
		src = &cgbMap[(i - 1) * map_x];
		dst = &cgbMap[(i - 1) * CGB_MAP_X];
		memmove(dst, src, map_x);
		memset((dst + map_x), chr, padding);

		src = &cgbAtrb[(i - 1) * map_x];
		dst = &cgbAtrb[(i - 1) * CGB_MAP_X];
		memmove(dst, src, map_x);
		memset((dst + map_x), 0, padding);
	}

	return padding;
}


// write data to input file name with proper extension
void save(char *fname, char *ext, void *src, u16 size) {

	char ofn[FILENAME_MAX];
	FILE *ofp = NULL;

	strcpy(ofn, fname);
	strcat(ofn, ext);

	ofp = fopen(ofn, "wb");
	fwrite(src, size, 1, ofp);
	fclose(ofp);
}


// release all allocated memory before exit
void release(void) {

	if(hdr != NULL)
		free(hdr);
	if(bmiColors != NULL)
		free(bmiColors);
	if(bmiData != NULL)
		free(bmiData);
	if(tmpColors != NULL)
		free(tmpColors);
	if(tmpIndex != NULL)
		free(tmpIndex);
	if(cgbTiles != NULL)
		free(cgbTiles);
}


int main (int argc, char *argv[]) {

	char *pos;	
	u8 arg, chr = 0, fname = 0, padding = 0, palettes_used, slot = 0;
	u16	tiles_used;

	printf("\nbmp2cgb v%.2f - 8bpp bitmap to Gameboy Color converter\n", VERSION);
	printf("programmed by: tmk, email: tmk@tuta.io\n");
	printf("bugs & updates: https://github.com/gitendo/bmp2cgb/\n\n");

	if(argc < 2)
		usage();

	for(arg = 1; arg < argc; arg++) {
		if((argv[arg][0] == '-') || (argv[arg][0] == '/')) {
			switch(tolower(argv[arg][1])) {
				case 'i': options |= FLAG_INFO; break;
				case 'd': options |= FLAG_DUPE; break;
				case 'p': options |= FLAG_PUSH; padding = atoi(&argv[arg][2]); break;
				case 'q': options |= FLAG_PAL; slot = atoi(&argv[arg][2]) & 7; break;
				case 'r': options |= FLAG_PAD; chr = atoi(&argv[arg][2]); break;
				case 'x': options |= FLAG_FLIPX; break;
				case 'y': options |= FLAG_FLIPY; break;
				case 'z': options |= FLAG_FLIPXY; break;
				default: error(ERR_UNK_OPTION); break;
			}
		} else {
			fname = arg;
		}
	}

	loadBMP(argv[fname]);
	prepareBMP();
	processBMP();
	palettes_used = createPalettes(slot);
	remapTiles();
	tiles_used = convertData(padding);

	if((options & FLAG_PAD) != 0)
		padding = padmaps(chr);
	else
		padding = 0;

	if((options & FLAG_INFO) == 0) {
		pos = strstr(argv[fname], ".bmp");
		if(pos != NULL)
			pos[0] = 0;

		save(argv[fname], EXT_ATRB, cgbAtrb, ((hdr->biWidth / TILE_WIDTH) + padding) * (hdr->biHeight / TILE_HEIGHT));
		save(argv[fname], EXT_MAP, cgbMap, ((hdr->biWidth / TILE_WIDTH) + padding) * (hdr->biHeight / TILE_HEIGHT));
		save(argv[fname], EXT_TILES, cgbTiles, tiles_used * CGB_TILE_SIZE);
		save(argv[fname], EXT_PALETTES, &cgbPalettes[slot], sizeof(CGBQUAD) * palettes_used);
	}

	release();

	exit(EXIT_SUCCESS);
}
