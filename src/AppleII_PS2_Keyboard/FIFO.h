#pragma once

#include <stdint.h>


template<typename T, size_t N>
class FIFO {
public:
    static_assert(N <= 256, "FIFO size cannot exceed 256 entries");

    FIFO();

    bool isFull() const;
    bool isEmpty() const;
    void push(T value);
    T pop();

private:
    uint8_t m_head;
    uint8_t m_tail;
    T m_buffer[N];
};


template<typename T, size_t N>
inline FIFO<T, N>::FIFO() : m_head(0), m_tail(0)
{
}


template<typename T, size_t N>
inline bool FIFO<T, N>::isFull() const {
    // if the tail is right behind the head then there's no more space
    return (((m_tail + 1) % N) == m_head);
}


template<typename T, size_t N>
inline bool FIFO<T, N>::isEmpty() const {
    // if the tail isn't ahead of the head then we don't have any data
    return (m_head == m_tail);
}


template<typename T, size_t N>
inline void FIFO<T, N>::push(T value) {
    uint8_t next_tail = (m_tail + 1) % N;
    if(next_tail != m_head) {
        m_buffer[m_tail] = value;
        m_tail = next_tail;
    }
}


template<typename T, size_t N>
inline T FIFO<T, N>::pop() {
    if(isEmpty()) {
        return 0;
    }

    T value = m_buffer[m_head];
    m_head = (m_head + 1) % N;
    return value;
}
