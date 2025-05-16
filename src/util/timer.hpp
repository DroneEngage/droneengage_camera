#ifndef CTIMER_H
#define CTIMER_H

#include <iostream>
#include <chrono>

namespace de
{
namespace util
{
// https://stackoverflow.com/questions/728068/how-to-calculate-a-time-difference-in-c
class CTimer
{
    public:
        CTimer() : m_begin(m_clock::now()) {}
        inline void reset() { m_begin = m_clock::now(); }
        inline  double elapsed_milli() const { 
            return std::chrono::duration_cast<m_milli_second>
                (m_clock::now() - m_begin).count(); }

        

    private:
        typedef std::chrono::high_resolution_clock m_clock;
        typedef std::chrono::duration<double, std::milli>  m_milli_second;
        std::chrono::time_point<m_clock> m_begin;
    };

};
};

#endif 