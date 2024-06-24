
#include "interface.h"
#include "UARTDriver.h"
#include <AP_Common/ExpandingString.h>

#if HAL_GCS_ENABLED
#include <AP_HAL/utility/packetise.h>
#endif

extern const AP_HAL::HAL& hal;

QURT::UARTDriver::UARTDriver(const char *name)
{
	HAP_PRINTF("Creating UART driver for port %s", name);

	if (strcmp(name, "/dev/console") == 0) {
		_is_console = true;
		HAP_PRINTF("UART console created");
	}

	if (strcmp(name, "/dev/gcs") == 0) {
		_packetise = true;
		HAP_PRINTF("Virtual GCS UART created");
	}

	_port = name;
}

void QURT::UARTDriver::printf(const char *fmt, ...)
{
	if (_is_console) {
		va_list ap;
		char buf[300];
		va_start(ap, fmt);
		vsnprintf(buf, sizeof(buf), fmt, ap);
		va_end(ap);
        HAP_PRINTF(buf);
        qurt_timer_sleep(5000);
	}
}

/* QURT implementations of virtual methods */
void QURT::UARTDriver::_begin(uint32_t b, uint16_t rxS, uint16_t txS)
{
	HAP_PRINTF(">>>>>>> _begin called for port %s", _port);

	if (_initialised) return;

    /* we have enough memory to have a larger transmit buffer for
     * all ports. This means we don't get delays while waiting to
     * write GPS config packets
     */
    if (rxS < 8192) {
        rxS = 8192;
    }
    if (txS < 32000) {
        txS = 32000;
    }

    while (_in_timer) hal.scheduler->delay(1);

    if (_writebuf.set_size(txS) && _readbuf.set_size(rxS)) {
        _initialised = true;
    }
}

void QURT::UARTDriver::_end()
{
	HAP_PRINTF(">>>>>>> _end called for port %s", _port);
}

void QURT::UARTDriver::_flush()
{
    // we are not doing any buffering, so flush is a no-op
	HAP_PRINTF(">>>>>>> _flush called for port %s", _port);
}

bool QURT::UARTDriver::is_initialized()
{
	return _initialised;
}

bool QURT::UARTDriver::tx_pending()
{
    return (_writebuf.available() > 0);
}

uint32_t QURT::UARTDriver::_available()
{
    if (!_initialised) {
        return 0;
    }
    return _readbuf.available();
}

uint32_t QURT::UARTDriver::txspace()
{
    if (!_initialised) {
        return 0;
    }
    return _writebuf.space();
}

bool QURT::UARTDriver::_discard_input()
{
    if (!_initialised) {
        return false;
    }
    _readbuf.clear();
    return true;
}

size_t QURT::UARTDriver::_write(const uint8_t *buffer, size_t size)
{
    if (!_initialised) {
        return 0;
    }
    if (!_write_mutex.take_nonblocking()) {
        return 0;
    }

	HAP_PRINTF("Writing %u bytes to serial port %s", size, _port);
    size_t ret = _writebuf.write(buffer, size);
    _write_mutex.give();
    return ret;
}

ssize_t QURT::UARTDriver::_read(uint8_t *buffer, uint16_t size)
{
    if (!_initialised) {
        return 0;
    }

	HAP_PRINTF("read from serial port %s", _port);
    return _readbuf.read(buffer, size);
}

/*
  try to push out one lump of pending bytes
  return true if progress is made
 */
bool QURT::UARTDriver::_write_pending_bytes(void)
{
    // write any pending bytes
    uint32_t available_bytes = _writebuf.available();
    uint16_t n = available_bytes;

#if HAL_GCS_ENABLED
    if (_packetise && n > 0) {
        // send on MAVLink packet boundaries if possible
        n = mavlink_packetise(_writebuf, n);
    }
#endif

    if (n > 0) {
        if (_packetise) {
            // keep as a single UDP packet
            _writebuf.peekbytes(_mavlink_msg.mav_msg, n);
			_writebuf.advance(n);
			if (n > 12) {
				HAP_PRINTF("Writing %u byte mavlink packet to GCS", n);
				(void) sl_client_send_data((const uint8_t*) &_mavlink_msg, n);
			} else {
				HAP_PRINTF("Skipping %u byte mavlink packet to GCS. Too short", n);
			}
        }
    }

    return _writebuf.available() != available_bytes;
}

/*
  push any pending bytes to/from the serial port. This is called at
  1kHz in the timer thread. Doing it this way reduces the system call
  overhead in the main task enormously.
 */
void QURT::UARTDriver::_timer_tick(void)
{
    if (!_initialised) return;

    _in_timer = true;

    uint8_t num_send = 10;
    while (num_send != 0 && _write_pending_bytes()) {
        num_send--;
    }

    // try to fill the read buffer
	// TODO

    _in_timer = false;
}