/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "AP_HAL_QURT.h"
#include "Semaphores.h"
#include <AP_HAL/utility/RingBuffer.h>
#include "protocol.h"

#define CONSOLE_BUFFER_SIZE 64

class QURT::UARTDriver : public AP_HAL::UARTDriver {
public:
    UARTDriver(const char *name);

    bool is_initialized() override;
    bool tx_pending() override;

    /* Empty implementations of Stream virtual methods */
    uint32_t txspace() override;

	void printf(const char *fmt, ...) override;

    bool _write_pending_bytes(void);
    virtual void _timer_tick(void) override;

    uint32_t bw_in_bytes_per_second() const override;
    enum AP_HAL::UARTDriver::flow_control get_flow_control(void) override;

protected:
    void _begin(uint32_t b, uint16_t rxS, uint16_t txS) override;
    size_t _write(const uint8_t *buffer, size_t size) override;
    ssize_t _read(uint8_t *buffer, uint16_t size) override WARN_IF_UNUSED;
    void _end() override;
    void _flush() override;
    uint32_t _available() override;
    bool _discard_input() override;
	bool _is_console{ false };
	char _console_buffer[CONSOLE_BUFFER_SIZE];
	const char *_port;
    volatile bool _initialised;
    volatile bool _in_timer;
    bool _packetise; // true if writes should try to be on mavlink boundaries
	struct qurt_mavlink_msg _mavlink_msg;

    // we use in-task ring buffers to reduce the system call cost
    // of ::read() and ::write() in the main loop
    ByteBuffer _readbuf{0};
    ByteBuffer _writebuf{0};

    QURT::Semaphore _write_mutex;

private:
	static void _mavlink_data_cb(const uint8_t *data, int len, void *p);
	void _fill_read_buffer(const uint8_t *data, int len);

};
