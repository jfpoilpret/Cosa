/**
 * @file Cosa/TWI.hh
 * @version 1.0
 *
 * @section License
 * Copyright (C) 2012, Mikael Patel
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 *
 * @section Description
 * Two wire library. Support for the I2C/TWI bus.
 *
 * This file is part of the Arduino Che Cosa project.
 */

#ifndef __COSA_TWI_HH__
#define __COSA_TWI_HH__

#include "Cosa/Types.h"
#include "Cosa/Bits.h"
#include "Cosa/Event.hh"
#include "Cosa/Thing.hh"
#include <avr/sleep.h>

class TWI {
private:
  /**
   * Two wire state and status codes
   */
  enum State {
    ERROR_STATE = 0,
    IDLE_STATE = 1,
    MT_STATE = 2,
    MR_STATE = 3,
    ST_STATE = 4,
    SR_STATE = 5
  };
# define TWI_STATUS(x) ((x) >> 3)
  enum {
    /** General Status Codes */
    START = TWI_STATUS(0x08),
    REP_START = TWI_STATUS(0x10),
    ARB_LOST = TWI_STATUS(0x38),
    /** Master Tranmitter Mode*/
    MT_SLA_ACK = TWI_STATUS(0x18),
    MT_SLA_NACK = TWI_STATUS(0x20),
    MT_DATA_ACK = TWI_STATUS(0x28),
    MT_DATA_NACK = TWI_STATUS(0x30),
    /** Master Receiver Mode */
    MR_SLA_ACK = TWI_STATUS(0x40),
    MR_SLA_NACK = TWI_STATUS(0x48),
    MR_DATA_ACK = TWI_STATUS(0x50),
    MR_DATA_NACK = TWI_STATUS(0x58),
    /** Slave Transmitter Mode */
    ST_SLA_ACK = TWI_STATUS(0xA8),
    ST_ARB_LOST_SLA_ACK = TWI_STATUS(0xB0),
    ST_DATA_ACK = TWI_STATUS(0xB8),
    ST_DATA_NACK = TWI_STATUS(0xC0),
    ST_LAST_DATA = TWI_STATUS(0xC8),
    /** Slave Receiver Mode */
    SR_SLA_ACK = TWI_STATUS(0x60),
    SR_ARB_LOST_SLA_ACK = TWI_STATUS(0x68),
    SR_GCALL_ACK = TWI_STATUS(0x70),
    SR_ARB_LOST_GCALL_ACK = TWI_STATUS(0x78),
    SR_DATA_ACK = TWI_STATUS(0x80),
    SR_DATA_NACK = TWI_STATUS(0x88),
    SR_GCALL_DATA_ACK = TWI_STATUS(0x90),
    SR_GCALL_DATA_NACK = TWI_STATUS(0x98),
    SR_STOP = TWI_STATUS(0xA0),
    /** Misc */
    NO_INFO = TWI_STATUS(0xF8),
    BUS_ERROR = TWI_STATUS(0x00)
  };

  /**
   * Addressing and read/write bit 
   */
  enum {
    WRITE_OP = 0x00,
    READ_OP = 0x01,
    ADDR_MASK = 0xfe
  };

  /**
   * Commands for TWI hardware
   */
  enum {
    IDLE_CMD =               _BV(TWEA) |              _BV(TWEN) | _BV(TWIE),
    START_CMD = _BV(TWINT) | _BV(TWEA) | _BV(TWSTA) | _BV(TWEN) | _BV(TWIE),
    DATA_CMD =  _BV(TWINT) |                          _BV(TWEN) | _BV(TWIE),
    ACK_CMD =   _BV(TWINT) | _BV(TWEA) |              _BV(TWEN) | _BV(TWIE),
    NACK_CMD =  _BV(TWINT) |                          _BV(TWEN) | _BV(TWIE),
    STOP_CMD =  _BV(TWINT) | _BV(TWEA) | _BV(TWSTO) | _BV(TWEN) | _BV(TWIE)
  };

  /**
   * Pins used for TWI interface (in port C, analog pins).
   */
  enum {
    SDA = 4,
    SCL = 5
  };

  /**
   * Device state, data buffers and target.
   */
  static const uint8_t BUF_MAX = 4;
  static const uint8_t VEC_MAX = 4;
  volatile State _state;
  volatile uint8_t _status;
  Thing* _target;
  uint8_t _addr;
  uint8_t _buf[BUF_MAX];
  iovec_t _vec[VEC_MAX];
  volatile uint8_t _ix;
  volatile uint8_t _next;
  volatile int _count;

  /**
   * Initiate a request to the slave. The address field is
   * the TWI address and operation (read/write bit). 
   * Return true(1) if successful otherwise false(0).
   * @param[in] addr slave address and operation.
   * @return bool
   */
  bool request(uint8_t addr);

public:
  /**
   * Device drivers are friends and may have callback/
   * event handler for completion events.
   */
  class Driver : public Thing {
    friend class TWI;
  };

  /**
   * Slave devices are friends and may have callback/
   * event handler for request events.
   */
  class Device : public Thing {
    friend class TWI;
  };

  /** 
   * Construct two-wire instance. This is actually a single-ton.
   */
  TWI() :
    _state(IDLE_STATE),
    _status(NO_INFO),
    _target(0),
    _addr(0),
    _ix(0),
    _next(0),
    _count(0)
  {
  }

  /**
   * Start TWI bus logic. Default mode is master if address is non zero.
   * @param[in] target receiver of events on requests.
   * @param[in] addr slave address.
   * @return true(1) if successful otherwise false(0)
   */
  bool begin(Thing* target = 0, uint8_t addr = 0);
  
  /**
   * Disconnect usage of the TWI bus logic.
   * @return true(1) if successful otherwise false(0)
   */
  bool end();

  /**
   * Set the input buffer.
   * @param[in] buf buffer pointer.
   * @param[in] size of buffer.
   */
  void set_buf(void* buf, uint8_t size)
  {
    _vec[0].buf = (uint8_t*) buf;
    _vec[0].size = size;
  }

  /**
   * Issue a write data request to the given slave unit. Return
   * true(1) if successful otherwise(0).
   * @param[in] addr slave address.
   * @param[in] buf data to write.
   * @param[in] size number of bytes to write.
   * @return bool
   */
  bool write_request(uint8_t addr, void* buf, uint8_t size);

  /**
   * Issue a write data request to the given slave unit. Return
   * true(1) if successful otherwise(0).
   * @param[in] addr slave address.
   * @param[in] header to write before buffer.
   * @param[in] buf data to write.
   * @param[in] size number of bytes to write.
   * @return bool
   */
  bool write_request(uint8_t addr, uint8_t header, void* buf, uint8_t size);

  /**
   * Issue a write data request to the given slave unit. Return
   * true(1) if successful otherwise(0).
   * @param[in] addr slave address.
   * @param[in] header to write before buffer.
   * @param[in] buf data to write.
   * @param[in] size number of bytes to write.
   * @return bool
   */
  bool write_request(uint8_t addr, uint16_t header, void* buf, uint8_t size);

  /**
   * Issue a read data request to the given slave unit. Return
   * true(1) if successful otherwise(0).
   * @param[in] addr slave address.
   * @param[in] buf data to write.
   * @param[in] size number of bytes to read.
   * @return number of bytes
   */
  bool read_request(uint8_t addr, void* buf, uint8_t size);

  /**
   * Write data to the given slave unit. Returns number of bytes
   * written or negative error code.
   * @param[in] addr slave address.
   * @param[in] buf data to write.
   * @param[in] size number of bytes to write.
   * @return number of bytes
   */
  int write(uint8_t addr, void* buf, uint8_t size)
  {
    if (!write_request(addr, buf, size)) return (-1);
    return (await_completed());
  }

  /**
   * Write data to the given slave unit. Returns number of bytes
   * written or negative error code.
   * @param[in] addr slave address.
   * @param[in] header to write before buffer.
   * @param[in] buf data to write.
   * @param[in] size number of bytes to write.
   * @return number of bytes
   */
  int write(uint8_t addr, uint8_t header, void* buf = 0, uint8_t size = 0)
  {
    if (!write_request(addr, header, buf, size)) return (-1);
    return (await_completed());
  }

  /**
   * Write data to the given slave unit. Returns number of bytes
   * written or negative error code.
   * @param[in] addr slave address.
   * @param[in] header to write before buffer.
   * @param[in] buf data to write.
   * @param[in] size number of bytes to write.
   * @return number of bytes
   */
  int write(uint8_t addr, uint16_t header, void* buf = 0, uint8_t size = 0)
  {
    if (!write_request(addr, header, buf, size)) return (-1);
    return (await_completed());
  }

  /**
   * Read data to the given slave unit. Returns number of bytes read
   * or negative error code.
   * @param[in] addr slave address.
   * @param[in] buf data to write.
   * @param[in] size number of bytes to read.
   * @return number of bytes
   */
  int read(uint8_t addr, void* buf, uint8_t size)
  {
    if (!read_request(addr, buf, size)) return (-1);
    return (await_completed());
  }

  /**
   * Await issued request to complete. Returns number of bytes 
   * or negative error code.
   */
  int await_completed(uint8_t mode = SLEEP_MODE_IDLE);

  /**
   * TWI state machine. Run by interrupt handler.
   */
  void on_bus_event();
};

/**
 * Two-wire hardware interface module.
 */
extern TWI twi;

#endif