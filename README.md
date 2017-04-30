# bmp2cgb
Tiny, command line utility that performs conversion of 8bpp uncompressed bitmap to GameBoy Color data format. This is basically going to be open source remake of DOS tool created back in the days by Ars of Fatality. I have planned some additional features already but don't keep your hopes high. I tend to start many projects and barely finish any. ;)

Input:
- 8 bit per pixel, uncompressed bitmap
- width and height must be multiple of 8
- no more than 32 unique colors
- no more than 4 colors per tile (8*8 pixel block)
- image size is restricted, so the output map doesn't exceed 16KB

Output:
- cgb_tiles.bin - optimized (duplicate removed), up to 384 unique tiles
- cgb_map.bin - character map
- cgb_attr.bin - attributes map
- cgb_palettes.bin - shrinked, combined and sorted

Source code will be added later? Needs usual cleanup, etc. Report bugs if you find any.
