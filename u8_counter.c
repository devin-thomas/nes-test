#include "neslib.h"
#include "vrambuf.h"

// 8bitworkshop's NES preset supplies chr_generic.s.
// vrambuf.c is included in this repository.
//#link "chr_generic.s"
//#link "vrambuf.c"

typedef unsigned char u8;

#define SKY_BLUE 0x31
#define BLACK    0x0f

/* chr_generic.s tile 0 is blank and tile 2 is a solid color-1 tile. */
#define BLANK_TILE 0
#define SOLID_TILE 2

#define NUMBER_X      10
#define NUMBER_Y      12
#define NUMBER_WIDTH  11
#define NUMBER_HEIGHT 5

#define SQUARE_X    3
#define SQUARE_Y    13
#define SQUARE_SIZE 3

#define TRIANGLE_X      13
#define TRIANGLE_Y      3
#define TRIANGLE_WIDTH  5
#define TRIANGLE_HEIGHT 3

static u8 counter;
static u8 number_tiles[NUMBER_WIDTH * NUMBER_HEIGHT];

/* Five 3-bit rows per digit. Bit 2 is the left column. */
static const u8 digit_rows[50] = {
    7,5,5,5,7,  /* 0 */
    2,6,2,2,7,  /* 1 */
    7,1,7,4,7,  /* 2 */
    7,1,7,1,7,  /* 3 */
    5,5,7,1,1,  /* 4 */
    7,4,7,1,7,  /* 5 */
    7,4,7,5,7,  /* 6 */
    7,1,1,1,1,  /* 7 */
    7,5,7,5,7,  /* 8 */
    7,5,7,1,7   /* 9 */
};

static const u8 square_row[SQUARE_SIZE] = {
    SOLID_TILE, SOLID_TILE, SOLID_TILE
};

static const u8 triangle_rows[TRIANGLE_WIDTH * TRIANGLE_HEIGHT] = {
    BLANK_TILE, BLANK_TILE, SOLID_TILE, BLANK_TILE, BLANK_TILE,
    BLANK_TILE, SOLID_TILE, SOLID_TILE, SOLID_TILE, BLANK_TILE,
    SOLID_TILE, SOLID_TILE, SOLID_TILE, SOLID_TILE, SOLID_TILE
};

static void clear_number_tiles(void) {
    u8 i;

    for (i = 0; i < NUMBER_WIDTH * NUMBER_HEIGHT; ++i) {
        number_tiles[i] = BLANK_TILE;
    }
}

static void draw_large_digit(u8 digit, u8 x_offset) {
    u8 row;
    u8 column;
    u8 mask;

    for (row = 0; row < NUMBER_HEIGHT; ++row) {
        mask = digit_rows[digit * NUMBER_HEIGHT + row];

        for (column = 0; column < 3; ++column) {
            if (mask & (4 >> column)) {
                number_tiles[row * NUMBER_WIDTH + x_offset + column] = SOLID_TILE;
            }
        }
    }
}

/* Rebuild an always-centered one-, two-, or three-digit display. */
static void build_number(void) {
    u8 hundreds;
    u8 tens;
    u8 ones;

    clear_number_tiles();

    hundreds = counter / 100;
    tens = (counter / 10) % 10;
    ones = counter % 10;

    if (hundreds) {
        draw_large_digit(hundreds, 0);
        draw_large_digit(tens, 4);
        draw_large_digit(ones, 8);
    } else if (counter >= 10) {
        draw_large_digit(tens, 2);
        draw_large_digit(ones, 6);
    } else {
        draw_large_digit(ones, 4);
    }
}

static void draw_initial_screen(void) {
    u8 row;

    /* Clear the nametable and attribute table while rendering is off. */
    vram_adr(NAMETABLE_A);
    vram_fill(BLANK_TILE, 1024);

    /* Three-by-three black square on the left. */
    for (row = 0; row < SQUARE_SIZE; ++row) {
        vram_adr(NTADR_A(SQUARE_X, SQUARE_Y + row));
        vram_write(square_row, SQUARE_SIZE);
    }

    /* Five-by-three block triangle at the top center. */
    for (row = 0; row < TRIANGLE_HEIGHT; ++row) {
        vram_adr(NTADR_A(TRIANGLE_X, TRIANGLE_Y + row));
        vram_write(&triangle_rows[row * TRIANGLE_WIDTH], TRIANGLE_WIDTH);
    }

    /* Large five-tile-high counter in the center. */
    for (row = 0; row < NUMBER_HEIGHT; ++row) {
        vram_adr(NTADR_A(NUMBER_X, NUMBER_Y + row));
        vram_write(&number_tiles[row * NUMBER_WIDTH], NUMBER_WIDTH);
    }
}

static void queue_number_update(void) {
    u8 row;

    for (row = 0; row < NUMBER_HEIGHT; ++row) {
        vrambuf_put(
            NTADR_A(NUMBER_X, NUMBER_Y + row),
            (const char*)&number_tiles[row * NUMBER_WIDTH],
            NUMBER_WIDTH
        );
    }
}

void main(void) {
    /* Sky-blue background with every foreground palette entry set to black. */
    pal_col(0, SKY_BLUE);
    pal_col(1, BLACK);
    pal_col(2, BLACK);
    pal_col(3, BLACK);

    counter = 0;
    build_number();
    draw_initial_screen();

    /* Future counter changes are copied safely during vertical blank. */
    vrambuf_clear();
    set_vram_update(updbuf);

    ppu_on_bg();

    while (1) {
        /* Trigger mode counts one increment per distinct A-button press. */
        if (pad_trigger(0) & PAD_A) {
            ++counter; /* As an unsigned byte, 255 wraps back to 0. */
            build_number();
            queue_number_update();
        }

        vrambuf_flush();
    }
}
