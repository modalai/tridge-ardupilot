#define __EXPORT __attribute__ ((visibility ("default")))

// Should be in termios.h
typedef unsigned int speed_t;

// TODO: This has to be defined in the slpi_proc build and in the PX4 build.
// Make it accessible from one file to both builds.
typedef struct {
    int (*send_data_func_ptr)(const uint8_t *data, int data_len_in_bytes);

    /*
      get a fd for the SPI bus
     */
    int (*_config_spi_bus_func_t)();

    /*
      perform a SPI bus transfer
     */
    int (*_spi_transfer_func_t)(int fd, const uint8_t *send, uint8_t *recv, const unsigned length);

    /*
      configure an I2C device, returns a file descriptor
     */
    int (*_config_i2c_bus_func_t)(uint8_t busnum, uint8_t i2c_address, uint32_t frequency);

    /*
      change I2C address
     */
    int (*_set_i2c_address_func_t)(int fd, uint8_t i2c_address);

    /*
      I2C transfer, returns 0 on success
     */
    int (*_i2c_transfer_func_t)(int fd, const uint8_t *send_data, const unsigned send_len, uint8_t *recv_data, const unsigned recv_len);

    int (*open_uart_func_t)(uint8_t, speed_t);
    int (*write_uart_func_t)(int, const void *, size_t);
    int (*read_uart_func_t)(int,  void *, size_t);
    int (*register_interrupt_callback)(int (*)(int, void *, void *), void *arg);
} qurt_func_ptrs_t;

#ifndef __cplusplus
#error "C++ should be defined!!!"
#endif

extern "C" {
    int slpi_link_client_init(qurt_func_ptrs_t *func_ptrs) __EXPORT;
    int slpi_link_client_receive(const uint8_t *data, int data_len_in_bytes) __EXPORT;
}
