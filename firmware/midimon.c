#include "midimon.h"
#include "blokas_logo.h"

#include <avr/pgmspace.h>
#include <avr/io.h>
#include <util/delay.h>

#define SPI_PORT B
#define LCD_PORT C
#define LCD_BACKLIGHT_PORT B

#define CONCAT(x, y) x ## y

#define DDR(p) CONCAT(DDR, p)
#define PORT(p) CONCAT(PORT, p)

enum { SPI_SS        = (1 << 2) };
enum { SPI_MOSI      = (1 << 3) };
enum { SPI_SCK       = (1 << 5) };
enum { LCD_RST       = (1 << 1) };
enum { LCD_CD        = (1 << 0) };
enum { LCD_BACKLIGHT = (1 << 1) };

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
	PORT(LCD_PORT) &= ~LCD_CD;
	spi_begin();
	g_scroll = line & 0x3f;
	spi_send(0x40 | g_scroll);
	spi_end();
}

static void draw_logo(void)
{
	spi_begin();

	uint8_t x = 29;
	uint8_t i;
	uint8_t j;

	for (j=0; j<BLOKAS_LOGO_HEIGHT/8; ++j)
	{
		PORT(LCD_PORT) &= ~LCD_CD;
		spi_send(0x00);
		spi_send(0x10);
		spi_send(0xb0 | (j & 0x07));
		PORT(LCD_PORT) |= LCD_CD;
		for (i=0; i<x; ++i)
		{
			spi_send(0x00);
		}
		for (i=0; i<BLOKAS_LOGO_WIDTH; ++i)
		{
			spi_send(~pgm_read_byte(&BLOKAS_LOGO[j*BLOKAS_LOGO_WIDTH+i]));
		}
		for (i=x+ BLOKAS_LOGO_WIDTH; i<132; ++i)
		{
			spi_send(0x00);
		}
	}
	for (; j<=8; ++j)
	{
		PORT(LCD_PORT) &= ~LCD_CD;
		spi_send(0x00);
		spi_send(0x10);
		spi_send(0xb0 | j);
		PORT(LCD_PORT) |= LCD_CD;
		for (i = 0; i<132; ++i)
		{
			spi_send(0x00);
		}
	}

	spi_end();
}

void midimon_init(void)
{
	DDR(SPI_PORT) |= SPI_SS | SPI_MOSI | SPI_SCK;
	DDR(LCD_PORT) |= LCD_RST | LCD_CD;
	DDR(LCD_BACKLIGHT_PORT) |= LCD_BACKLIGHT;
	PORT(LCD_BACKLIGHT_PORT) |= LCD_BACKLIGHT;
	SPCR = (1 << SPE) | (1 << MSTR);
	SPSR = (1 << SPI2X);
	PORT(LCD_PORT) |= LCD_RST;
	PORT(LCD_PORT) &= ~LCD_CD; // Command mode.
	spi_begin();

	spi_send(0x40);
	spi_send(0xb0);
	spi_send(0x00); spi_send(0x10);
	spi_send(0xa0);
	spi_send(0xc8);
	spi_send(0xa6);
	spi_send(0xa2);
	spi_send(0x2f);
	spi_send(0xf8); spi_send(0x00);
	spi_send(0x23);
	spi_send(0x81); spi_send(0x27);
	spi_send(0xac);
	spi_send(0xfa); spi_send(0x83);

	spi_send(0xa4);
	spi_send(0xaf);

	spi_end();

	draw_logo();
	set_scroll(64 - 4);
}

void midimon_uninit(void)
{
	DDR(SPI_PORT)  &= ~(SPI_SS | SPI_MOSI | SPI_SCK);
	PORT(SPI_PORT) &= ~(SPI_SS | SPI_MOSI | SPI_SCK);

	// LCD_RST left at HIGH state intentionally.
	DDR(LCD_PORT)  &= ~(/*LCD_RST |*/ LCD_CD);
	PORT(LCD_PORT) &= ~(/*LCD_RST |*/ LCD_CD | LCD_BACKLIGHT);
	DDR(LCD_BACKLIGHT_PORT)  &= ~(LCD_BACKLIGHT);
	PORT(LCD_BACKLIGHT_PORT) &= ~(LCD_BACKLIGHT);
}

void midimon_progress(void)
{
	set_scroll(g_scroll + 1);
}

void midimon_progress_reset(void)
{
	while (g_scroll != 60)
	{
		midimon_progress();
		_delay_ms(5);
	}
}
