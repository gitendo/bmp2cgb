# bmp2cgb
Tiny, command line utility that performs conversion of 8bpp uncompressed bitmap to Game Boy Color data format. I couldn't find anything similar, so I've reproduced my favourite DOS tool - created back in the days by Ars of [Fatality](http://speccy.info/Fatality).

### Input
- 8 bits per pixel, uncompressed bitmap
- width and height must be multiple of 8
- no more than 32 unique colors
- no more than 4 colors per tile (8\*8 pixel block)
- image size is restricted, so the output map doesn't exceed 16KB

### Output
- `.atr` - attributes map
- `.map` - character map
- `.chr` - optimized tileset
- `.pal` - palettes
- `.gbc` - RGB tuner ROM image

By default input image will be optimized as much as possible to provide the best output. This includes removal of duplicate tiles, usage of 'flipping' feature, color sorting with duplicate removal and finally remaping tileset to match generated palette data. Please notice that you'll probably have to adjust colors manually using hardware since Game Boy Color RGBs are in 0-31 range while bitmap RGBs are 0-255. Use RGB tuner option to achieve best result. Also, if you convert sprites, make sure that transparent color has lowest RGB value in such tile. Otherwise it will end up on wrong position in palette and the output will be unusable. This will be sorted out someday.

### Options
For extra needs there's few you can use:

`bmp2cgb.exe [options] picture[.bmp]`
```
-d  - do not optimize tileset
-i  - information output, no data is created
-p# - push map by 1-255 characters
-q# - push first palette to slot 1-7
-r# - pad map width to 32 characters with tile number 0-255
-t  - output RGB tuner image
-x  - disable horizontal flip optimization
-y  - disable vertical flip optimization
-z  - disable horizontal & vertical flip optimization
```
### RGB tuner
This handy feature allows you to adjust palette data in real time so it's not being output by `bmp2cgb`. You need Game Boy Color or Advance and flash cart to make any use of it. By default it starts with first color in first palette. Controls are as following:
- Left/Right - moves between R, G and B values of selected color
- Up/Down - increases/decreases selected value
- B+Left/Right - moves between colors in selected palette
- B+Up/Down - moves between palettes
- A - provides focus on edited color (all other colors are blacked out)
- Select+Left/Right/Up/Down - moves screen window over bg area
- Start - saves palette to SRAM

After you save any changes they will be loaded next time you start. Erase SRAM to repeat from beginning or just use it properly. When finished extract SRAM file from cartridge and include it in your project - you need only first 64 bytes. Just hex view/edit it to see what I mean.

### Examples
I recommend using `-i` option to make sure input image is valid. It will also help you to "debug" any cases where's more than 4 colors per tile used. Now, suppose we have 2 bitmaps ie. 160\*24 px - with 4 color fonts in ascii order and 160\*80 px - with game logo that uses 8 colors. Let's convert them.

`bmp2cgb -d font8x8.bmp` - this will disable duplicate removal and x/y flipping, so you'll get output as in image. If we place it at $8200 we'll get ascii mapping, so writing TEXT will be equal to copying it to character map memory without additional code. In the end we'll need only font8x8.chr and font8x8.pal.

`bmp2cgb -p92 -q1 -r32 game_logo.bmp` - this will have optimized output, with maps expecting your first tile at $85C0 (we leave $8000-$81FF empty, fonts will take $8200-$85A4). Map width will be expanded to 32 characters and will be padded with 32 value, which is $20 (ascii space). Also maps will be expecting your palette data in slot 1, since we need slot 0 to store font palette. `-r` option is just for lazy coders who prefer to copy whole map using DMA or simple loop.

If for some reason you'd like to get DMG output, use:
`bmp2cgb -x -y -z dmg_image.bmp` - delete .atr and .pal and use dmg_image.chr and dmg_image.map. Then manually set BGP register.

### Bugs
There's some weird issue with `memcmp` in `convertData()` function when you compile on Linux. It doesn't crash but the output data/info is unusable. I couldn't nail it so I temporarily fixed it with some ugly hack. :/
