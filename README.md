# bmp2cgb
Tiny, command line utility that performs conversion of 8bpp uncompressed bitmap to GameBoy Color data format. I couldn't find anything like that around so I've reproduced my favourite DOS tool - created back in the days by Ars of Fatality. I've also planned some additional features already but don't keep your hopes high. I tend to start many projects but barely finish any. ;)

### Input
- 8 bits per pixel, uncompressed bitmap
- width and height must be multiple of 8
- no more than 32 unique colors
- no more than 4 colors per tile (8\*8 pixel block)
- image size is restricted, so the output map doesn't exceed 16KB

### Output
- `.atr` - attributes map
- `.map` - character map
- `.chr` - optimized tileset - up to 384 unique tiles
- `.pal` - palettes

By default input image will be optimized as much as possible to provide best output. This includes removal of duplicate tiles, usage of 'flipping' feature, color sorting with duplicate removal (where it's possible) and finally remaping tileset to match generated palette data. Please notice that you'll probably have to adjust colors manually using hardware. 

### Options
For extra needs there's few you can use:

`bmp2cgb.exe [options] picture[.bmp]`
```
-d  - do not optimize tileset
-i  - dump information, no output data is created
-p# - push tileset right by 1-255 characters
-q# - push first palette to 1-7 slot
-r# - pad maps to 32 characters, fill difference with tile 0-255
-x  - disable horizontal flip optimization
-y  - disable vertical flip optimization
-z  - disable horizontal & vertical flip optimization
```
### Examples
Suppose we have 2 bitmaps ie. 160\*24 px - with 4 color fonts in ascii order and 160\*80 px - with game logo that uses 8 colors. Let's convert them.

`bmp2cgb -d font8x8.bmp` - this will disable duplicate removal and x/y flipping, so you'll get output as in image. If we place it at $8200 we'll get ascii mapping, so writing TEXT will be equal to copying it to character map memory without additional code. In the end we'll need only font8x8.chr and font8x8.pal.

`bmp2cgb -p92 -q1 -r32 game_logo.bmp` - this will have optimized output, with maps expecting your first tile at $85C0 (we leave $8000-$81FF empty, fonts will take $8200-$85A4). Map width will be expanded to 32 characters and will be padded with 32 value, which is $20 (ascii space). Also maps will be expecting your palette data in slot 1, since we need slot 0 to store font palette. `-r` option is for lazy coders who prefer to copy whole map using DMA or simple loop. 

### Bugs
There's some weird issue with `memcmp` in `convertData()` function when you compile on Linux. It doesn't crash but the output data/info is unusable. I couldn't nail it so I temporarily fixed it with some ugly hack. :/
