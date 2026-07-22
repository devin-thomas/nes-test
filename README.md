# NES U8 Counter

[Open this project in 8bitworkshop](https://8bitworkshop.com/redir.html?platform=nes&githubURL=https://github.com/devin-thomas/nes-test&file=u8_counter.c)

A deliberately tiny NES program:

- Constant sky-blue background
- White unsigned 8-bit decimal counter
- Starts at `0`
- Each new press of NES **A** increments once
- `255 + 1` wraps to `0`

In 8bitworkshop, click the emulator screen and press **X**, which maps to
the NES A button.

## Files kept in this repository

```text
README.md
u8_counter.c
vrambuf.h
vrambuf.c
```

The NES platform in 8bitworkshop supplies `neslib.h` and `chr_generic.s`.
This repository includes the VRAM-buffer header and implementation that
the counter source explicitly depends on.

The `platform=nes` and `file=u8_counter.c` values in the link above tell
8bitworkshop which platform and main file to load instead of falling back
to its built-in `DEFAULT` assembly project.
