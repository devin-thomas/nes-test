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

#define SQUARE_CENTER_X 40
#define SQUARE_CENTER_Y 132
#define SQUARE_BLOCKS   9
#define SQUARE_STEPS    9

#define TRIANGLE_CENTER_X 128
#define TRIANGLE_CENTER_Y 40
#define TRIANGLE_BLOCKS   9
#define TRIANGLE_ANGLES   12

static u8 counter;
static u8 number_tiles[NUMBER_WIDTH * NUMBER_HEIGHT];

static u8 square_rotating;
static u8 square_step;
static u8 triangle_angle;

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

/*
 * The square is nine overlapping 8x8 blocks arranged in a 3x3 grid.
 * Coordinates are abstract units around its center.
 */
static const sbyte square_x_units[SQUARE_BLOCKS] = {
    -1, 0, 1,
    -1, 0, 1,
    -1, 0, 1
};

static const sbyte square_y_units[SQUARE_BLOCKS] = {
    -1,-1,-1,
     0, 0, 0,
     1, 1, 1
};

/* Approximate cosine and sine multiplied by 16 for 0,10,...,90 degrees. */
static const sbyte square_cos16[10] = {16,16,15,14,12,10,8,5,3,0};
static const sbyte square_sin16[10] = { 0, 3, 5, 8,10,12,14,15,16,16};

/*
 * The triangle is nine overlapping 8x8 blocks: one on top, three in the
 * middle, and five on the bottom.
 */
static const sbyte triangle_x_units[TRIANGLE_BLOCKS] = {
     0,
    -1, 0, 1,
    -2,-1, 0, 1, 2
};

static const sbyte triangle_y_units[TRIANGLE_BLOCKS] = {
    -1,
     0, 0, 0,
     1, 1, 1, 1, 1
};

/* Approximate cosine and sine multiplied by 8 for 0,30,...,330 degrees. */
static const sbyte triangle_cos8[TRIANGLE_ANGLES] = {
     8, 7, 4, 0,-4,-7,-8,-7,-4, 0, 4, 7
};

static const sbyte triangle_sin8[TRIANGLE_ANGLES] = {
     0, 4, 7, 8, 7, 4, 0,-4,-7,-8,-7,-4
};

/* Multiply by one of the tiny coordinate units -2,-1,0,1,2. */
static sbyte times_unit(sbyte value, sbyte unit) {
    switch (unit) {
        case -2: return -(value + value);
        case -1: return -value;
        case  0: return 0;
        case  1: return value;
        default: return value + value;
    }
}

/*
 * Compress the square's rotated center spacing from eight pixels to six.
 * The 8x8 sprites therefore overlap by two pixels and cannot expose holes.
 */
static sbyte square_overlap_scale(sbyte value) {
    unsigned char magnitude;

    if (value < 0) {
        magnitude = (unsigned char)(-value);
        return -((magnitude * 3 + 4) >> 3);
    }

    return (value * 3 + 4) >> 3;
}

/* Apply the same six-pixel spacing to the triangle's lookup-table values. */
static sbyte triangle_overlap_scale(sbyte value) {
    unsigned char magnitude;

    if (value < 0) {
        magnitude = (unsigned char)(-value);
        return -((magnitude * 3 + 2) >> 2);
    }

    return (value * 3 + 2) >> 2;
}

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

/* Draw the square at 0,10,...,90 degrees using overlapping solid sprites. */
static u8 draw_square(u8 sprite_id) {
    u8 i;
    sbyte cosine;
    sbyte sine;
    sbyte x_term;
    sbyte y_term;
    sbyte x_offset;
    sbyte y_offset;

    cosine = square_cos16[square_step];
    sine = square_sin16[square_step];

    for (i = 0; i < SQUARE_BLOCKS; ++i) {
        x_term = times_unit(cosine, square_x_units[i])
               - times_unit(sine, square_y_units[i]);
        y_term = times_unit(sine, square_x_units[i])
               + times_unit(cosine, square_y_units[i]);

        x_offset = square_overlap_scale(x_term);
        y_offset = square_overlap_scale(y_term);

        sprite_id = oam_spr(
            SQUARE_CENTER_X + x_offset - 4,
            SQUARE_CENTER_Y + y_offset - 4,
            SOLID_TILE,
            0,
            sprite_id
        );
    }

    return sprite_id;
}

/* Draw the triangle at one of twelve solid, overlapping orientations. */
static u8 draw_triangle(u8 sprite_id) {
    u8 i;
    sbyte cosine;
    sbyte sine;
    sbyte x_term;
    sbyte y_term;
    sbyte x_offset;
    sbyte y_offset;

    cosine = triangle_cos8[triangle_angle];
    sine = triangle_sin8[triangle_angle];

    for (i = 0; i < TRIANGLE_BLOCKS; ++i) {
        x_term = times_unit(cosine, triangle_x_units[i])
               - times_unit(sine, triangle_y_units[i]);
        y_term = times_unit(sine, triangle_x_units[i])
               + times_unit(cosine, triangle_y_units[i]);

        x_offset = triangle_overlap_scale(x_term);
        y_offset = triangle_overlap_scale(y_term);

        sprite_id = oam_spr(
            TRIANGLE_CENTER_X + x_offset - 4,
            TRIANGLE_CENTER_Y + y_offset - 4,
            SOLID_TILE,
            0,
            sprite_id
        );
    }

    return sprite_id;
}

static void draw_shapes(void) {
    u8 sprite_id;

    sprite_id = 0;
    sprite_id = draw_square(sprite_id);
    sprite_id = draw_triangle(sprite_id);
    oam_hide_rest(sprite_id);
}

void main(void) {
    u8 pressed;
    u8 held;

    /* Sky-blue background and black foreground graphics. */
    pal_col(0, SKY_BLUE);
    pal_col(1, BLACK);
    pal_col(2, BLACK);
    pal_col(3, BLACK);

    /* Sprite palette zero also uses black for the solid shape blocks. */
    pal_col(17, BLACK);
    pal_col(18, BLACK);
    pal_col(19, BLACK);

    counter = 0;
    square_rotating = 0;
    square_step = 0;
    triangle_angle = 0;

    build_number();
    draw_initial_screen();

    oam_clear();
    draw_shapes();

    /* Future counter changes are copied safely during vertical blank. */
    vrambuf_clear();
    set_vram_update(updbuf);

    ppu_on_all();

    while (1) {
        /* Trigger mode is polled first; pad_state then exposes held buttons. */
        pressed = pad_trigger(0);
        held = pad_state(0);

        /* Preserve the original counter: one increment per new A press. */
        if (pressed & PAD_A) {
            ++counter; /* As an unsigned byte, 255 wraps back to 0. */
            build_number();
            queue_number_update();
        }

        /* While A is held, turn the triangle 30 degrees every frame. */
        if (held & PAD_A) {
            ++triangle_angle;
            if (triangle_angle >= TRIANGLE_ANGLES) {
                triangle_angle = 0;
            }
        }

        /* A new B press starts one nine-frame, 90-degree clockwise turn. */
        if ((pressed & PAD_B) && !square_rotating) {
            square_rotating = 1;
            square_step = 0;
        }

        if (square_rotating) {
            ++square_step; /* 10 degrees per frame for nine frames. */
        }

        draw_shapes();

        /* The 90-degree endpoint looks identical for a perfect square. */
        if (square_rotating && square_step >= SQUARE_STEPS) {
            square_rotating = 0;
            square_step = 0;
        }

        vrambuf_flush();
    }
}
