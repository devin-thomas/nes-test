# NES U8 Counter

[Open this project in 8bitworkshop](https://8bitworkshop.com/redir.html?platform=nes&githubURL=https://github.com/devin-thomas/nes-test&file=u8_counter.c)

A deliberately tiny NES program that now also experiments with sprite rotation:

- Constant sky-blue background
- Large black block-digit counter centered on screen
- Black square on the left
- Black triangle at the top center
- Starts at `0`
- A new NES **A** press increments the unsigned 8-bit counter once
- Holding **A** also rotates the triangle clockwise by 30 degrees every frame
- A new NES **B** press rotates the square clockwise by 90 degrees over nine frames
- `255 + 1` wraps to `0`

In 8bitworkshop, click the emulator screen before using the controls:

- Keyboard **X** = NES A
- Keyboard **Z** = NES B

## Implementation note

The large number remains in the NES background nametable. The square and
triangle are now composed of nine solid 8x8 sprites each. Their block centers
are rotated around fixed pivots using small integer sine/cosine lookup tables.
This avoids floating-point math and allows the shapes to move at pixel-level
positions rather than jumping only between background-tile cells.

The square uses 10-degree animation steps, producing a 90-degree turn over
exactly nine frames. The triangle has twelve discrete orientations spaced 30
degrees apart and advances once per frame while A remains held.

## Files kept in this repository

```text
README.md
u8_counter.c
vrambuf.h
vrambuf.c
```

The NES platform in 8bitworkshop supplies `neslib.h` and `chr_generic.s`.
The program reuses the blank and solid tiles already present in
`chr_generic.s`, so no additional graphics file is required. This repository
includes the VRAM-buffer header and implementation that the counter source
explicitly depends on.

The `platform=nes` and `file=u8_counter.c` values in the link above tell
8bitworkshop which platform and main file to load instead of falling back
to its built-in `DEFAULT` assembly project.
