#ifndef BMP2CGB_H
#define BMP2CGB_H

#define VERSION 		1.21

#define	BI_RGB			0
#define BI_RLE8			1
#define BI_RLE4			2
#define BI_BITFIELDS	3
#define BITFIELDS_SIZE	12
#define BMP_TILE_SIZE	64
#define CGB_TILE_SIZE 	16
#define	CHR_BG			1
#define	CHR_OBJ			2
#define MAX_COLORS 		4
#define MAX_MAP_SIZE 	16384
#define MAX_SLOTS 		8
#define MAX_TILES 		512
#define TILE_WIDTH 		8
#define TILE_HEIGHT 	8

#define EXT_ATR			".atr"
#define EXT_MAP			".map"
#define EXT_CHR			".chr"
#define EXT_PAL			".pal"
#define	EXT_GBC			".gbc"
#define EXT_TXT			".txt"

#define FLAG_DEBUG 		1
#define FLAG_DUMP		2
#define FLAG_FLIPX 		4
#define FLAG_FLIPY 		8
#define FLAG_FLIPXY 	16
#define FLAG_UNOPT		32
#define FLAG_REBASE		64
#define FLAG_EXPAND		128
#define FLAG_MPAD		256
#define FLAG_PPAD		512
#define FLAG_TUNER		1024
#define FLAG_OBJ		2048

#pragma pack(push, 1)

typedef struct _BITMAPFILEHEADER {
	unsigned short bfType;
	unsigned int   bfSize;
	unsigned short bfReserved1;
	unsigned short bfReserved2;
	unsigned int   bfOffBits;
} BITMAPFILEHEADER;

typedef struct _BITMAPINFOHEADER {
	unsigned int   biSize;
	int            biWidth;
	int            biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned int   biCompression;
	unsigned int   biSizeImage;
	int            biXPelsPerMeter;
	int            biYPelsPerMeter;
	unsigned int   biClrUsed;
	unsigned int   biClrImportant;
} BITMAPINFOHEADER;

typedef struct _BITFIELDS {
	unsigned int red;
	unsigned int green;
	unsigned int blue;
} BITFIELDS;

typedef struct _CGBQUAD {
	unsigned short col0;
	unsigned short col1;
	unsigned short col2;
	unsigned short col3;
} CGBQUAD;

typedef struct _RGB15 {
	unsigned int blue : 5;
	unsigned int green : 5;
	unsigned int red : 5;
} RGB15;

typedef struct _RGBQUAD {
	unsigned char blue;
	unsigned char green;
	unsigned char red;
	unsigned char reserved;
} RGBQUAD;

#pragma pack(pop)


void banner(void);
void error_handler(char status);
void make_rgbt(unsigned char *rgbtuner, unsigned char *cgb_chr, unsigned char *cgb_map, unsigned char *cgb_atr, CGBQUAD *cgb_pal, unsigned short map_height, unsigned short map_width, unsigned short used_tiles);
void release(unsigned char **bmp_data, CGBQUAD **tmp_pal, unsigned char	**tmp_idx, unsigned char **cgb_chr);
void save(char *fname, char *ext, void *src, int size);
void usage(void);

void flip(unsigned char *, int, int);
char merge_24bpp(RGBQUAD *, unsigned char *, int);
char merge_32bpp(RGBQUAD *, unsigned char *, int, unsigned int);
char process_bmp(char *fname, BITMAPFILEHEADER **header, BITMAPINFOHEADER **info, RGBQUAD *bmp_pal, unsigned char **bmp_data, unsigned short *width, unsigned short *height);
void split_4bpp(unsigned char *, int);

void bubble_sort(unsigned short *palette, unsigned char color_index);
char convert(unsigned char *bmp_data, unsigned char **cgb_chr, unsigned char *cgb_map, unsigned char *cgb_atr, unsigned char *tmp_idx, unsigned short tiles, unsigned short padding, unsigned short *used_tiles, unsigned short options, char status);
char count_colors(unsigned short *slot);
char create_tiles(unsigned char *bmp_data, unsigned short width, unsigned short height, char status);
char create_palettes(CGBQUAD *cgb_pal, unsigned char **tmp_idx, CGBQUAD *tmp_pal, unsigned short tiles, unsigned char *slot, unsigned short options, char status);
void expand_maps(unsigned char *cgb_map, unsigned char *cgb_atr, unsigned short rows, unsigned short columns, unsigned char chr, char status);
char find_palette(unsigned short *src, CGBQUAD *cgb_pal, unsigned char used_slots, unsigned char used_colors);
char match_palette(unsigned short *src, CGBQUAD *cgb_pal, unsigned char used_slots, unsigned char used_colors);
char optimize(unsigned char *bmp_data, RGBQUAD *bmp_pal, CGBQUAD **tmp_pal, unsigned short rows, unsigned short columns, unsigned int rgbhex, unsigned short options, char status);
void rebase(unsigned char *cgb_map, unsigned short map_size);
void remap_colors(unsigned char *bmp_data, CGBQUAD *cgb_pal, unsigned char *tmp_idx, CGBQUAD *tmp_pal, unsigned short tiles, char status);
void save_oam(char *fname, char *ext, unsigned char *cgb_atr, unsigned char *cgb_map, unsigned short map_size);
void sort_palettes(CGBQUAD *cgb_pal, unsigned char used_slots, unsigned char mode);


#endif /* BMP2CGB_H */
