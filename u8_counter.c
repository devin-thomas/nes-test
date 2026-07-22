#include "neslib.h"
#include "vrambuf.h"

// 8bitworkshop's NES preset supplies chr_generic.s.
// vrambuf.c is included in this repository.
//#link "chr_generic.s"
//#link "vrambuf.c"

typedef unsigned char u8;

#define SKY_BLUE    0x31
#define WHITE       0x30
#define NUMBER_X    14
#define NUMBER_Y    14
#define NUMBER_ADDR NTADR_A(NUMBER_X, NUMBER_Y)

static u8 counter;
static char digits[3];

/* Convert the u8 counter to a right-aligned decimal value: "  0"..."255". */
static void format_counter(void) {
    u8 hundreds;
    u8 tens;
    u8 ones;

    hundreds = counter / 100;
    tens = (counter / 10) % 10;
    ones = counter % 10;

    digits[0] = hundreds ? ('0' + hundreds) : ' ';
    digits[1] = (hundreds || tens) ? ('0' + tens) : ' ';
    digits[2] = '0' + ones;
}

void main(void) {
    /* Sky-blue universal background and white text. */
    pal_col(0, SKY_BLUE);
    pal_col(1, WHITE);
    pal_col(2, WHITE);
    pal_col(3, WHITE);

    counter = 0;
    format_counter();

    /* Rendering starts off, so the initial number can be written directly. */
    vram_adr(NUMBER_ADDR);
    vram_write((const unsigned char*)digits, 3);

    /* Later changes are safely copied to VRAM during vertical blank. */
    vrambuf_clear();
    set_vram_update(updbuf);

    ppu_on_bg();

    while (1) {
        /* Trigger mode counts one increment per distinct A-button press. */
        if (pad_trigger(0) & PAD_A) {
            ++counter; /* As an unsigned byte, 255 wraps back to 0. */
            format_counter();
            vrambuf_put(NUMBER_ADDR, digits, 3);
        }

        vrambuf_flush();
    }
}
