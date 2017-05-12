#ifndef BMP2CGB_H
#define BMP2CGB_H

typedef signed char		s8;
typedef unsigned char	u8;
typedef signed short	s16;
typedef unsigned short	u16;
typedef signed int		s32;
typedef unsigned int	u32;

#define VERSION 		1.0

#define BI_RGB 			0x0000
#define BI_COLORS 		256
#define BI_TILE_SIZE	64
#define CGB_TILE_SIZE 	16
#define EXT_ATRB 		".atr"
#define EXT_MAP 		".map"
#define EXT_TILES 		".chr"
#define EXT_PALETTES 	".pal"
#define FLAG_INFO 		1
#define FLAG_DUPE 		2
#define FLAG_FLIPX 		4
#define FLAG_FLIPY 		8
#define FLAG_FLIPXY 	16
#define FLAG_PUSH		32
#define FLAG_PAD 		64
#define FLAG_PAL		128
#define MAX_MAP_SIZE 	0x4000
#define MAX_COLORS 		4
#define MAX_TILES 		384
#define MAX_PALETTES 	8
#define TILE_WIDTH 		8
#define TILE_HEIGHT 	8

#pragma pack(push, 1)

typedef struct _BITMAPHEADER {
	u16 bfType;
	u32 bfSize;
	u16 bfReserved1;
	u16 bfReserved2;
	u32 bfOffBits;
	u32 biSize;
	u32 biWidth;
	u32 biHeight;
	u16 biPlanes;
	u16 biBitCount;
	u32 biCompression;
	u32 biSizeImage;
	u32 biXPelsPerMeter;
	u32 biYPelsPerMeter;
	u32 biClrUsed;
	u32 biClrImportant;
} BITMAPHEADER;

typedef struct _RGBQUAD {
	u8 rgbBlue;
	u8 rgbGreen;
	u8 rgbRed;
	u8 rgbReserved;
} RGBQUAD;

typedef struct _RGB15 {
	u32 rgbBlue : 5;
	u32 rgbGreen : 5;
	u32 rgbRed : 5;
} RGB15;

typedef struct _CGBQUAD {
	u16 col0;
	u16 col1;
	u16 col2;
	u16 col3;
} CGBQUAD;

#pragma pack(pop)

u8 createPalettes(u8);
u8 findPalette(u16 *, u8, u8);
u8 matchPalette(u16 *, u8, u8);
u8 padmaps(u8);
u8 tileColors(u16 *);
u8 usage(void);
u16 convertData(u8);
void bubbleSort(u8);
void error(u8);
void loadBMP(char *);
void prepareBMP(void);
void processBMP(void);
void release(void);
void remapTiles(void);
void save(char *, char *, void *, u16);

#endif /* BMP2CGB_H */
