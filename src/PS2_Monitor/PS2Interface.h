#pragma once

#include <util/parity.h>
#include "FIFO.h"
#include "config.h"


// Class to read and buffer the bytes from a PS/2 interface
class PS2Interface {
public:
    PS2Interface() : m_prevMillis(0), m_rxCount(-1), m_rxData(0) {}

    // Setup the I/O pins
    void setup();

    // Returns the next data byte received from the keyboard or 0 if there is none.
    uint8_t getDataByte();

    // Handles the falling clock edge by reading the next data bit
    void onClockFallingEdge();

private:
    unsigned long m_prevMillis;
    int8_t m_rxCount;
    uint8_t m_rxData;
    FIFO<uint8_t, PS2_RX_BUFFER_SIZE> m_rxBuffer;
};


void PS2Interface::setup() {
    pinMode(PS2_CLOCK_PIN, INPUT_PULLUP);
    pinMode(PS2_DATA_PIN, INPUT_PULLUP);
}


uint8_t PS2Interface::getDataByte() {
    noInterrupts();
    uint8_t b =  m_rxBuffer.pop();
    interrupts();
    return b;
}


void PS2Interface::onClockFallingEdge() {
    unsigned long now = millis();
    if(now - m_prevMillis > 50) {
        // auto-reset if too long
        m_rxCount = -1;
    }
    m_prevMillis = now;

    int dataBit = digitalRead(PS2_DATA_PIN);
    if(m_rxCount < 0) {
        // check for a start bit
        if(dataBit == 0) {
            m_rxCount = 0;
            m_rxData = 0;
        }
    } else if(m_rxCount < 8) {
        // got a data bit, received LSB to MSB
        m_rxData |= (dataBit << m_rxCount);
        m_rxCount++;
    } else if(m_rxCount == 8) {
        // received the parity bit (odd)
        if(parity_even_bit(m_rxData) != dataBit) {
            m_rxCount++;
        } else {
            // bad parity
            m_rxCount = -1;
        }
    } else if(m_rxCount == 9) {
        // check for the stop bit
        if(dataBit == 1) {
            m_rxBuffer.push(m_rxData);
        }
        m_rxCount = -1;
    }
}
