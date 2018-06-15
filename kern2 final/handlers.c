#include "decls.h"
#include <stdbool.h>

/**
 * Handler para el timer (IRQ0). Escribe un carácter cada segundo.
 */
static const uint8_t hz_ratio = 18;  // Default IRQ0 freq (18.222 Hz).

#define CURSOR '-'
#define LEFT_ARROW '='
#define RIGHT_ARROW '#'
#define MAX_WRITE 79
#define SPACE ' '
#define BACKSPACE '\b'
#define ENTER '\n'

void timer() {
    static char chars[81];
    static unsigned ticks;
    static int8_t line = 21;
    static uint8_t idx = 0;

    if (++ticks % hz_ratio == 0) {
        chars[idx] = '.';
        chars[++idx] = '\0';
        vga_write(chars, line, 0x07);
    }

    if (idx >= sizeof(chars) - 1) {
        line++;
        idx = 0;
    }
}

/**
 * Mapa de "scancodes" a caracteres ASCII en un teclado QWERTY.
 */
static char klayout[128] = {
    0,   0,   '1', '2', '3', '4', '5', '6', '7', '8',             // 0-9
    '9', '0', 0,   0, BACKSPACE, 0, 'q', 'w', 'e', 'r',                // 10-19
    't', 'y', 'u', 'i', 'o', 'p', 0,   0,   ENTER,   0,               // 20-29
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 0,   0,          // 30-40
    0,   0,   0,   'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.',
    0, 0, 0, 0, SPACE, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,LEFT_ARROW,
    0, RIGHT_ARROW}; 

static const uint8_t KBD_PORT = 0x60;

static bool use_upper(uint8_t code) {
    // return false;
    static bool shift_pressed;

    bool released = code & 0x80;
    code = code & ~0x80;

    if (code == 42 || code == 54) {
        shift_pressed = !released;
    }

    return shift_pressed;
}

/**
 * Handler para el teclado (IRQ1).
 *
 * Imprime la letra correspondiente por pantalla.
 */
void keyboard() {
    uint8_t code;
    static char chars[MAX_WRITE];
    static char chars_selected[MAX_WRITE];
    static uint8_t idx = 0;
    static int init = 0;
    if (init == 0) {
		for (int i = 0; i < MAX_WRITE; i++) {
			chars[i] = SPACE;
			chars_selected[i] = SPACE;
		}
		init = 1;
	}
    asm volatile("inb %1,%0" : "=a"(code) : "n"(KBD_PORT));

    int8_t upper_shift = use_upper(code) ? -32 : 0;

    if (code >= sizeof(klayout) || !klayout[code])
        return;

	if (klayout[code] == BACKSPACE) {
		if (idx == 0) {
	    	idx++;
	    }
	    chars_selected[idx] = SPACE;
		chars[--idx] = SPACE;
		chars_selected[idx] = CURSOR;
	} else if (klayout[code] == LEFT_ARROW) {
		chars_selected[idx] = ' ';
		if (idx == 0) {
	    	idx++;
	    }
		idx--;
		chars_selected[idx] = CURSOR;
	} else if (klayout[code] == RIGHT_ARROW) {
		chars_selected[idx] = SPACE;
		if (idx == MAX_WRITE) {
	    	idx--;
	    }
		idx++;
		chars_selected[idx] = CURSOR;
	} else if (klayout[code] == ENTER) {
		for (int i = 0; i < MAX_WRITE; i++) {
			chars[i] = SPACE;
			chars_selected[i] = SPACE;
		}
		idx = 0;
		chars_selected[idx] = CURSOR;
	} else {
		if (idx >= MAX_WRITE) {
			chars_selected[idx] = SPACE;
		    while (idx--)
		        chars[idx] = SPACE;
	        idx = 0;
			chars_selected[idx] = CURSOR;
		}
		chars_selected[idx] = SPACE;
		chars[idx++] = klayout[code] + upper_shift;
		chars_selected[idx] = CURSOR;
    }
    vga_write(chars, 19, 0x0A);
	vga_write(chars_selected, 20, 0x0A);
}
