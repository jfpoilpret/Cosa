/**
 * @file CosaBlinkThread.ino
 * @version 1.0
 *
 * @section License
 * Copyright (C) 2014, Mikael Patel
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
 * Demonstration of Cosa Nucleo Threads; Blink LED with thread
 * delay and use an additional thread to control the LED blink
 * frequency. The main thread will power down while waiting for
 * Watchdog ticks (ATtiny, LED on 2 mA, LED off >10 uA @ 3.7V).
 *
 * This file is part of the Arduino Che Cosa project.
 */

#include "Cosa/OutputPin.hh"
#include "Cosa/Watchdog.hh"
#include "Cosa/Nucleo/Thread.hh"

// This class is a thread that turns LED on and off given a
// delay period. The delay may be changed dynamically.
class LED : public Nucleo::Thread {
public:
  LED(Board::DigitalPin pin) : Thread(), m_pin(pin, 1), m_delay(200) {}
  void set_delay(uint16_t ms) { m_delay = ms; }
  virtual void run();
private:
  OutputPin m_pin;
  uint16_t m_delay;
};

void 
LED::run() 
{ 
  m_pin.toggle(); 
  delay(m_delay);
}

// This class is a thread that periodically changes a LED delay
// from low to high with a given increment
template <uint16_t LOW, uint16_t HIGH, uint16_t INC, uint16_t PERIOD>
class Controller : public Nucleo::Thread {
public:
  Controller(LED* led) : Thread(), m_led(led) {}
  virtual void run();
private:
  LED* m_led;
};

template <uint16_t LOW, uint16_t HIGH, uint16_t INC, uint16_t PERIOD>
void
Controller<LOW, HIGH, INC, PERIOD>::run()
{
  for (uint16_t ms = LOW; ms < HIGH; ms += INC) {
    m_led->set_delay(ms);
    delay(PERIOD);
  }
  for (uint16_t ms = HIGH; ms > LOW; ms -= INC) {
    m_led->set_delay(ms);
    delay(PERIOD);
  }
}

// The instances of the LED and the Controller
LED buildin(Board::LED);
Controller<25,500,5,200> controller(&buildin);

void setup()
{
  // Use power down idle mode for low power
  Power::set(SLEEP_MODE_PWR_DOWN);
  Watchdog::begin();

  // Allocate the threads with given stack size
  Nucleo::Thread::begin(&buildin, 64);
  Nucleo::Thread::begin(&controller, 64);
}

void loop()
{
  // Start the threads
  Nucleo::Thread::begin();
}