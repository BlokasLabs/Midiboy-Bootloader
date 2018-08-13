#ifndef BLOKAS_LOGO_H
#define BLOKAS_LOGO_H

#include <avr/pgmspace.h>

enum { BLOKAS_LOGO_WIDTH = 70 };
enum { BLOKAS_LOGO_HEIGHT = 56 };

extern const PROGMEM uint8_t BLOKAS_LOGO[BLOKAS_LOGO_WIDTH*BLOKAS_LOGO_HEIGHT/8];

#endif // BLOKAS_LOGO_H
