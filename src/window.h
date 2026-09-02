#ifndef ASPIRATION_WINDOW_H
#define ASPIRATION_WINDOW_H

struct search_window
{
    int depth;
    int alpha;
    int beta;
};

class AspirationWindow
{
public:
    auto next_window() -> search_window;
    auto report_result(int eval) -> bool;

    static constexpr int beta_init = 1'000'000;
    static constexpr int alpha_init = -beta_init;

private:
    static constexpr int max_depth_with_full_window = 4;
    static constexpr int margin = 50;
    static constexpr int failure_growth = 100;
    static constexpr int max_fails = 4;

    int m_depth = 1;
    int m_alpha = alpha_init;
    int m_beta = beta_init;
    int m_fail_high_counter = 0;
    int m_fail_low_counter = 0;
    int m_last_successful_eval = 0;
};

#endif
