#include "window.h"

#include <algorithm>
#include <cstdint>
#include <iostream>

#include "engine.h"

using namespace std;

auto AspirationWindow::next_window() -> search_window
{
    return search_window{.depth = m_depth, .alpha = m_alpha, .beta = m_beta};
}

auto AspirationWindow::report_result(int eval) -> bool
{
    if (m_depth <= max_depth_with_full_window)
    {
        m_depth++;
        DEBUG_LOG("Depth: {}, Full window", m_depth);
        return true;
    }
    // fail high
    if (eval >= m_beta)
    {
        DEBUG_LOG("FH: a {} b {} e {} d {}", m_alpha, m_beta, eval, m_depth);
        if (m_fail_high_counter >= max_fails)
        {
            m_beta = beta_init;
        }
        else
        {
            m_beta = static_cast<int16_t>(
                max(m_last_successful_eval + margin + (failure_growth << m_fail_high_counter),
                    eval + margin));
        }
        m_fail_high_counter++;
        return false;
    }
    // fail low
    if (eval <= m_alpha)
    {
        DEBUG_LOG("FL: a {} b {} e {} d {}", m_alpha, m_beta, eval, m_depth);
        if (m_fail_low_counter >= max_fails)
        {
            m_alpha = alpha_init;
        }
        else
        {
            m_alpha = static_cast<int16_t>(
                min(m_last_successful_eval - margin - (failure_growth << m_fail_low_counter),
                    eval - margin));
        }
        m_fail_low_counter++;
        return false;
    }

    // passed
    DEBUG_LOG("P: a {} b {} e {} d {}", m_alpha, m_beta, eval, m_depth);
    m_depth++;
    m_alpha = static_cast<int16_t>(eval - margin);
    m_beta = static_cast<int16_t>(eval + margin);
    m_last_successful_eval = static_cast<int16_t>(eval);
    m_fail_high_counter = 0;
    m_fail_low_counter = 0;
    return true;
}
