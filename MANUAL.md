# bmp2cgb's manual
You're expected to be familiar with command line environment since `bmp2cgb` doesn't have any GUI. Execute it to see the list of available options. Some of them changed between v1.11 and v1.20 so be careful if you've been using it.

In this document term `character` is preffered to `tile` although they mean the same ie. 8\*8 px block of graphics. I also use term `slot` instead of `palette` when referring to single palette, consisting of 4 colors or less. So in `bmp2cgb` world `palette` has maximum of 8 `slots` and `slot` holds up to 4 colors, ok?

By default, input image will be optimized as much as possible to provide best result. This includes removal of duplicate characters, usage of 'flipping' feature, color sorting with duplicate removal and finally remaping charset to match generated palette data. In most basic scenario the only thing you need to provide is valid bitmap image. Currently 4/8/24/32bpp formats are accepted although I don't recommend true color ones. Indexed modes are simply easier to debug and correct in case of any problems.

On successful conversion you'll get following files (depending on used options):

- `.chr` - character set (background or sprites)
- `.map` - character map
- `.atr` - attribute map
- `.pal` - palette
- `.txt` - sprites OAM table mock-up
- `.gbc` - RGB Tuner ROM image

Except `.txt` one, all of them are binary.

## Optimization
Input image is processed in a couple of different ways, so all possible duplicates are removed. You can however control the process according to your needs:

option | desc
--- | ---
`-c`| Any character duplicates are allowed.
`-x`| Horizontally flipped duplicates are allowed, others are removed.
`-y`| Vertically flipped duplicates are allowed, others are removed.
`-z`| Duplicates that are flipped horizontally and vertically at the same time are allowed, others are removed.
`-o`| Palette data is created without any optimization. Each slot is filled with colors in the same order (index based) as in processed character. Therefore it is possible to have slots using 1,2,3 or 4 colors. This option was meant for experienced graphicians and coders who want to achieve custom layout of colors from carefully prepared image. It is usable for both sprite and background, so don't mix it with `-s#` option. I do not reccomend using it unless you know what you're doing.

*Example:* `bmp2cgb -x -y -z picture.bmp`

Such converted data (picture.chr, picture.map) can be used on DMG providing that `picture.bmp` uses no more than 4 unique colors.

## Tweaks
While converting couple of images there're two possible ways:
- Create one large image consisting of smaller ones, usually vertically stacked.
- Convert all images individually.

If first scenario is viable and there're no errors you're set. If not you might find those neccessary:

option | desc
--- | ---
`-e#`| Usable for images to be displayed which are less than 256 px wide. Character and attribute map width is expanded to 32 blocks and filled with character code from 0 to 255. Such map can be copied to VRAM in a simple loop then. Filling provides easy way to clean 'off screen' area in case of SCX usage. This is for lazy coders if you haven't noticed.
`-m#`| Allows proper indexing of character and attribute maps when there already are some characters occupying VRAM. Use value between 1 and 511 to specify starting character.
`-p#`| Same as above but related to palette data - allows you to choose starting slot. Values from 1 to 7 are to choose from.
`-r` | Use it if you intend to place character data in VRAM at $8800-$97FF. This changes format of character map from unsigned to signed values.

*Example:* `bmp2cgb -e0 -m32 -p3 picture.bmp`

Suppose `picture.bmp` is typical screen of 160\*144 px. After conversion character and attribute maps will be 32\*18 instead of 20\*18 blocks. Expanded parts will have value of 0. Character data should be stored at $8200 - this is where 32nd character is. And palette should be loaded to slot 3.

## Output
option | desc
--- | ---
`-d`| No data is created but you're presented with extended debug information. It contains coordinates, color values and color matrix of characters that exceed 4 colors limit - if there're any. Useful for graphicians.
`-s#`| Treats output as sprite data. You must specify transparency color in RGB hex value without hash. Output will also provide text file containing mockup of OAM table for your convinience.
`-t`| Outputs RGBTuner ROM with converted image for further color adjustments on Game Boy Color or Advance. Palette data is not being output. More informations about RGBT usage is next.

*Debug example:*
```
Error: tile at (8, 112) uses 5 colors: (02) #008000, (04) #000000, (05) #f0f0ea, (06) #b47e4b, (08) #c0dcc0

02 04 04 08 08 08 08 08
06 04 05 05 05 05 05 05
05 04 05 05 05 05 05 05
06 04 04 04 05 05 05 05
06 06 04 06 04 05 05 05
06 05 04 04 06 06 05 05
06 06 04 04 06 04 05 05
06 04 04 06 06 04 05 05
```
From above example you can learn that pixel in top left corner of such character has RGB color #008000 and is stored as 2nd entry in bitmap palette. Single pixel looks more like leftover so it should be replaced with colors 4,5,6 or 8 to pass conversion process.

*Example:* `bmp2cgb -s4682b4 sprites.bmp`

This will treat output data as sprite instead of background. Color #4682b4 will be converted to $2216, which is CGB 15bit RGB value and placed as first color in slot. File `sprites.txt` will include details to be used in your OAM table.

## RGB Tuner
Due to the Game Boy Color / Advance LCD's nature there's no way to properly convert all colors in software. This is where RGBT comes into play allowing you to adjust palette data in real time! You need Game Boy Color or Advance and flash cart to make any use of it. By default it starts with first color from first slot. Controls are as following:

- `Left/Right` - switches between Red, Green and Blue values of selected color.
- `Up/Down` - increases / decreases selected value.
- `B+Left/Right` - switches between colors in selected slot.
- `B+Up/Down` - switches between palette slots.
- `A` - provides focus on edited color (all other colors are blacked out).
- `Select+Left/Right/Up/Down` - moves screen window over background area.
- `Start` - saves palette to SRAM.

After you save any changes they will be loaded next time you start. Erase SRAM to restart or just use it properly. When finished extract SRAM file from cartridge and include it in your project - you need only first 64 bytes. Just hex view or edit it to see what I mean.

EOF