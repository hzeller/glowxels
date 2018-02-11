To compile: Needs wiringpi and libpng
```
sudo aptitude install wiringpi libpng-dev
```

```
Usage essentially:

```
usage: ./glow [options] <png-file>
Options:
        -d    : switch direction
        -i    : inverse image
```

Example:
```
  ./glow ~/someimage.png
```

Image needs to be 320 pixels wide.