#ifndef _LIGDBG_SPI__
#define _LIGDBG_SPI__




int spi_api_do_set(int bus_id,
                   u8 mode,
                   u8 bits_per_word,
                   u8 max_speed_hz);

int spi_api_do_write(int bus_id, const u8 *buf, size_t len);
int spi_api_do_read(int bus_id, u8 *buf, size_t len);
int spi_api_do_write_then_read(int bus_id,
                               const u8 *txbuf, unsigned n_tx,
                               u8 *rxbuf, unsigned n_rx);



void mod_spi_main(int argc, char **argv);

#endif

