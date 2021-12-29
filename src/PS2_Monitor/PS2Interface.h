#pragma once

#include <util/parity.h>


#define PS2_CLOCK_PIN 7
#define PS2_DATA_PIN 0
#define PS2_RX_BUFFER_SIZE 64


class PS2Interface {
public:
  PS2Interface() : m_prevMillis(0), m_rxCount(0), m_rxData(0), m_rxBufferHead(0), m_rxBufferTail(0) {}

  // Returns the next data byte received from the keyboard
  uint8_t getDataByte();

  // Handles the falling clock edge by reading the next data bit
  void onClockFallingEdge();

private:
  unsigned long m_prevMillis;
  uint8_t m_rxCount;
  uint8_t m_rxData;
  volatile uint8_t m_rxBufferHead;
  volatile uint8_t m_rxBufferTail;
  volatile uint8_t m_rxBuffer[PS2_RX_BUFFER_SIZE];
};

uint8_t PS2Interface::getDataByte() {
  // if the head isn't ahead of the tail, we don't have any data
  if(m_rxBufferHead == m_rxBufferTail) {
    return 0;
  }

  uint8_t val = m_rxBuffer[m_rxBufferTail];
  m_rxBufferTail = (uint8_t)(m_rxBufferTail + 1) % PS2_RX_BUFFER_SIZE;
  return val;
}

void PS2Interface::onClockFallingEdge() {
  unsigned long now = millis();
  if(now - m_prevMillis > 50) {
    // auto-reset if too long
    m_rxCount = 0;
  }
  m_prevMillis = now;

  int dataBit = digitalRead(PS2_DATA_PIN);
  if(m_rxCount == 0) {
    if(dataBit == 0) {
      // got start bit
      m_rxCount++;
      m_rxData = 0;
    }
  } else if(m_rxCount < 9) {
    // got data bit
    m_rxData |= (dataBit << (m_rxCount-1));
    m_rxCount++;
  } else if(m_rxCount == 9) {
    // parity bit (odd)
    if(parity_even_bit(m_rxData) != dataBit) {
      m_rxCount++;
    } else {
      // bad parity
      m_rxCount = 0;
    }
  } else if(m_rxCount == 10) {
    if(dataBit == 1) {
      // got the stop bit, save the byte if there's room
      uint8_t newRecvBufferHead = (m_rxBufferHead + 1) % PS2_RX_BUFFER_SIZE;

      if(newRecvBufferHead != m_rxBufferTail) {
        m_rxBuffer[m_rxBufferHead] = m_rxData;
        m_rxBufferHead = newRecvBufferHead;
      }
    }
    m_rxCount = 0;
  }
}
