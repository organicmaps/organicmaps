/*

Copyright (c) 2015, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef PERCENT_HPP
#define PERCENT_HPP

#include <iostream>
#include <atomic>

class Percent
{
  public:
    explicit Percent(unsigned max_value, unsigned step = 5) { reinit(max_value, step); }

    // Reinitializes
    void reinit(unsigned max_value, unsigned step = 5)
    {
        m_max_value = max_value;
        m_current_value = 0;
        m_percent_interval = m_max_value / 100;
        m_next_threshold = m_percent_interval;
        m_last_percent = 0;
        m_step = step;
    }

    // If there has been significant progress, display it.
    void printStatus(unsigned current_value)
    {
        if (current_value >= m_next_threshold)
        {
            m_next_threshold += m_percent_interval;
            printPercent(current_value / static_cast<double>(m_max_value) * 100.);
        }
        if (current_value + 1 == m_max_value)
            std::cout << " 100%" << std::endl;
    }

    void printIncrement()
    {
        ++m_current_value;
        printStatus(m_current_value);
    }

    void printAddition(const unsigned addition)
    {
        m_current_value += addition;
        printStatus(m_current_value);
    }

  private:
    std::atomic_uint m_current_value;
    unsigned m_max_value;
    unsigned m_percent_interval;
    unsigned m_next_threshold;
    unsigned m_last_percent;
    unsigned m_step;

    // Displays progress.
    void printPercent(double percent)
    {
        while (percent >= m_last_percent + m_step)
        {
            m_last_percent += m_step;
            if (m_last_percent % 10 == 0)
            {
                std::cout << " " << m_last_percent << "% ";
            }
            else
            {
                std::cout << ".";
            }
            std::cout.flush();
        }
    }
};

#endif // PERCENT_HPP
