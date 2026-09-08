#ifndef ASPIRATION_WINDOW_H
#define ASPIRATION_WINDOW_H

#include <cstdint>
struct search_window
{
    int8_t depth;
    int16_t alpha;
    int16_t beta;
};

class AspirationWindow
{
public:
    auto next_window() -> search_window;
    auto report_result(int eval) -> bool;

    static constexpr int16_t beta_init = 10'000;
    static constexpr int16_t alpha_init = -beta_init;

private:
    static constexpr int max_depth_with_full_window = 4;
    static constexpr int margin = 50;
    static constexpr int16_t failure_growth = 100;
    static constexpr int max_fails = 4;

    int8_t m_depth = 1;
    int16_t m_alpha = alpha_init;
    int16_t m_beta = beta_init;
    int m_fail_high_counter = 0;
    int m_fail_low_counter = 0;
    int16_t m_last_successful_eval = 0;
};

#endif
