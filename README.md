### Notice
Starting October 12th, 2023 GitHub is enforcing mandatory [two-factor authentication](https://github.blog/2023-03-09-raising-the-bar-for-software-security-github-2fa-begins-march-13/) on my account.  
I'm not going to comply and move all my activity to GitLab instead.  
Any future updates / releases will be available at: [https://gitlab.com/gitendo/bmp2cgb](https://gitlab.com/gitendo/bmp2cgb)  
Thanks and see you there!
___

# bmp2cgb v1.21 ![standard](https://img.shields.io/badge/standard-C11-blue.svg?longCache=true&style=flat) ![dependencies](https://img.shields.io/badge/dependencies-none-green.svg?longCache=true&style=flat) ![status](https://img.shields.io/badge/status-working-green.svg?longCache=true&style=flat)

Complete solution for converting graphics and real time palette adjustments for Game Boy Color. Heavily inspired by original utility created by Ars of [Fatality](http://speccy.info/Fatality) in 1999. It's tiny, fast, command line driven and doesn't require any dependencies, so you can compile it without any problems on Windows or *nix systems.


### Options :
```
-c    disable character optimization
-x    disable horizontal flip optimization
-y    disable vertical flip optimization
-z    disable horizontal & vertical flip optimization
-o    disable palette optimization

-e#   expand map width to 32 blocks using character (0-255)
-m#   map padding - starting character (1-511)
-p#   palette padding - starting slot (1-7)
-r    rebase character map to $8800-$97FF ($8000-$8FFF is default)

-d    extended debug information without data output
-s#   sprites output (transparent color RGB hex value ie. 4682b4)
-t    RGBTuner ROM image output
```

You can read [the manual](MANUAL.md) if you need more detailed explanation or don't know how to use it.

### Limitations :
Currently only BITMAPINFOHEADER is supported. This one is most common and widely used so it shouldn't really matter. However if you end up with `Unsupported bitmap type, BITMAPINFOHEADER not found!` you might want to check settings of your graphics software ie. for Gimp you need to enable `Do not write color space information` in `Compatibility Options` while exporting image to BMP.

### Recent changes :
- fixed bug related to trimmed palettes in 4bpp/8bpp bitmaps

### To do :
- add support for BITMAPV4HEADER and BITMAPV5HEADER
- get rid of all level 2 warnings

### Bugs :
Hopefully none. Let me know if you find any.
