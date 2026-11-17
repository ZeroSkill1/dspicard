#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>

#include <pico/multicore.h>
#include <pico/bootrom.h>
#include <pico/stdio.h>
#include <pico/stdlib.h>

#include <pico/util/queue.h>

static queue_t dataqueue = { 0 };

#include <tusb.h>

#include "spi.pio.h"

#define PIN_SCLK    11 // SCLK
#define PIN_D4      16 // SIO3
#define PIN_D5      17 // SIO2
#define PIN_D6      18 // SIO1
#define PIN_D7      19 // SIO0
#define PIN_IRQ     20 // IRQ
#define PIN_CS2     21 // CS

#define CARDSPI_MASK ((1 << PIN_CS2) | (1 << PIN_SCLK) | (1 << PIN_D4) | (1 << PIN_D5) | (1 << PIN_D6) | (1 << PIN_D7))

typedef union {
	char chars[4];
	uint32_t raw;
} fifo_elem_t;

uint8_t n = 0;
fifo_elem_t cur_elem = { .raw = 0 };

uint8_t reverse(uint8_t b) {
	return ((b & 1) << 3) | ((b & 2) << 1) | ((b & 4) >> 1) | ((b & 8) >> 3);
}

#define NC(x) (reverse((x) & 0xF) | reverse((x) >> 4) << 4)

void core1_entry(void) {
	fifo_elem_t data = { .raw = 0 };
	while (1) {
		if (queue_try_remove(&dataqueue, &data)) {
			printf("%c%c%c%c", NC(data.chars[3]), NC(data.chars[2]), NC(data.chars[1]), NC(data.chars[0]));
		}
	}
}

int main() {
	queue_init(&dataqueue, 4, 256); // 1 KiB of buffer

	gpio_put(PIN_IRQ, false);
	gpio_set_dir(PIN_IRQ, GPIO_OUT);
	gpio_disable_pulls(PIN_IRQ);

	set_sys_clock_pll(1200000000, 6, 1);
	stdio_init_all();

	multicore_launch_core1(core1_entry);

	gpio_set_drive_strength(PIN_D4, GPIO_DRIVE_STRENGTH_2MA);
	gpio_set_drive_strength(PIN_D5, GPIO_DRIVE_STRENGTH_2MA);
	gpio_set_drive_strength(PIN_D6, GPIO_DRIVE_STRENGTH_2MA);
	gpio_set_drive_strength(PIN_D7, GPIO_DRIVE_STRENGTH_2MA);
	gpio_disable_pulls(PIN_SCLK);
	gpio_disable_pulls(PIN_D4);
	gpio_disable_pulls(PIN_D5);
	gpio_disable_pulls(PIN_D6);
	gpio_disable_pulls(PIN_D7);
	gpio_disable_pulls(PIN_CS2);

	uint32_t spi_progoffs = pio_add_program(pio0, &ntr_card_qspi_program);
	pio_sm_config c = ntr_card_qspi_program_get_default_config(spi_progoffs);
	sm_config_set_in_pins(&c, PIN_D4);
	sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
	sm_config_set_jmp_pin(&c, PIN_CS2);
	sm_config_set_in_shift(&c, false, true, 32);
	sm_config_set_clkdiv(&c, 1);
	pio_sm_set_pindirs_with_mask(pio0, 1, 0, CARDSPI_MASK);
	pio_sm_set_pins_with_mask(pio0, 1, 0, CARDSPI_MASK);
	pio_gpio_init(pio0, PIN_CS2);
	pio_gpio_init(pio0, PIN_SCLK);
	pio_gpio_init(pio0, PIN_D4); // SIO3
	pio_gpio_init(pio0, PIN_D5); // SIO2
	pio_gpio_init(pio0, PIN_D6); // SIO1
	pio_gpio_init(pio0, PIN_D7); // SIO0
	pio_sm_init(pio0, 1, spi_progoffs, &c);
	pio_sm_clear_fifos(pio0, 1);
	pio_sm_set_enabled(pio0, 1, true);

	while (1) {
		if (!pio_sm_is_rx_fifo_empty(pio0, 1)) {
			uint32_t data = pio0->rxf[1];
			queue_try_add(&dataqueue, &data);
		}
	}
}
