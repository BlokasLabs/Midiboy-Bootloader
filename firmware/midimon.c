#include "midimon.h"
#include "blokas_logo.h"

#include <stdbool.h>

#include <avr/pgmspace.h>
#include <avr/io.h>
#include <util/delay.h>

#define SPI_PORT B
#define LCD_PORT D

#define CONCAT(x, y) x ## y

#define DDR(p) CONCAT(DDR, p)
#define PORT(p) CONCAT(PORT, p)

enum { SPI_SS   = (1 << 2) };
enum { SPI_MOSI = (1 << 3) };
enum { SPI_SCK  = (1 << 5) };
enum { LCD_RST  = (1 << 6) };
enum { LCD_DC   = (1 << 7) };

static void spi_send(uint8_t b)
{
	SPDR = b;
	while (!(SPSR & (1 << SPIF)));
}

static void spi_begin(void)
{
	PORT(SPI_PORT) &= ~SPI_SS;
}

static void spi_end(void)
{
	PORT(SPI_PORT) |= SPI_SS;
}

static uint8_t g_scroll = 0;

static void set_scroll(uint8_t line)
{
	PORT(LCD_PORT) &= ~LCD_DC;
	spi_begin();
	g_scroll = line & 0x3f;
	spi_send(0x40 | g_scroll);
	spi_end();
}

static void draw_logo(void)
{
	spi_begin();

	uint8_t x = 48;
	uint8_t i;
	uint8_t j;
	uint8_t y;

	for (j = 0; j < 1; ++j)
	{
		PORT(LCD_PORT) &= ~LCD_DC;
		spi_send(0x02);
		spi_send(0x10);
		spi_send(0xb0 | j);
		PORT(LCD_PORT) |= LCD_DC;
		for (i = 0; i < 128; ++i)
		{
			spi_send(0x00);
		}
	}

	for (y = 0; y < BLOKAS_LOGO_HEIGHT / 8; ++j, ++y)
	{
		PORT(LCD_PORT) &= ~LCD_DC;
		spi_send(0x02);
		spi_send(0x10);
		spi_send(0xb0 | (j & 0x07));
		PORT(LCD_PORT) |= LCD_DC;
		for (i = 0; i < x; ++i)
		{
			spi_send(0x00);
		}
		for (i = 0; i < BLOKAS_LOGO_WIDTH; ++i)
		{
			spi_send(~pgm_read_byte(&BLOKAS_LOGO[y*BLOKAS_LOGO_WIDTH + i]));
		}
		for (i = x + BLOKAS_LOGO_WIDTH; i < 128; ++i)
		{
			spi_send(0x00);
		}
	}
	for (; j < 8; ++j)
	{
		PORT(LCD_PORT) &= ~LCD_DC;
		spi_send(0x02);
		spi_send(0x10);
		spi_send(0xb0 | j);
		PORT(LCD_PORT) |= LCD_DC;
		for (i = 0; i < 128; ++i)
		{
			spi_send(0x00);
		}
	}

	spi_end();
}

void midimon_init(void)
{
	DDR(SPI_PORT) |= SPI_SS | SPI_MOSI | SPI_SCK;
	DDR(LCD_PORT) |= LCD_RST | LCD_DC;
	PORT(LCD_PORT) &= ~LCD_RST;
	SPCR = (1 << SPE) | (1 << MSTR);
	SPSR = (1 << SPI2X);
	_delay_us(15);
	PORT(LCD_PORT) |= LCD_RST;

	PORT(LCD_PORT) &= ~LCD_DC;

	spi_begin();
	spi_send(0xae); // Disable display.
	spi_send(0xc8); // Mirror vertically.
	spi_send(0xa1); // Mirror horizontally.

	// Contrast
	spi_send(0x81);
	spi_send(0xff);
	spi_end();

	set_scroll(64 - 8);

#if 0
	spi_begin();
	int i, j;
	for (j = 0; j < 8; ++j)
	{
		PORT(LCD_PORT) &= ~LCD_DC;

		spi_send(0xb0 | j);
		spi_send(0x10);
		spi_send(0x02);

		PORT(LCD_PORT) |= LCD_DC;
		for (i = 0; i < 132 * 64; ++i)
		{
			spi_send(0x00);
		}
	}
	spi_end();
#endif

	draw_logo();

	PORT(LCD_PORT) &= ~LCD_DC; // Command mode.
	spi_begin();
	spi_send(0xaf); // Enable display.
	spi_end();
}

void midimon_uninit(void)
{
	DDR(SPI_PORT) &= ~(SPI_SS | SPI_MOSI | SPI_SCK);
	PORT(SPI_PORT) &= ~(SPI_SS | SPI_MOSI | SPI_SCK);

	// LCD_RST left at HIGH state intentionally.
	DDR(LCD_PORT) &= ~(/*LCD_RST |*/ LCD_DC);
	PORT(LCD_PORT) &= ~(/*LCD_RST |*/ LCD_DC);
}

void midimon_progress(void)
{
	set_scroll(g_scroll + 1);
}

void midimon_progress_reset(void)
{
	while (g_scroll != 56)
	{
		midimon_progress();
		_delay_ms(5);
	}
}
