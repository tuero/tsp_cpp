#include <tsp/tsp.h>

#include <chrono>
#include <iostream>

using namespace tsp;

using std::chrono::duration;
using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;

constexpr int NUM_STEPS = 10000000;
constexpr int MILLISECONDS_PER_SECOND = 1000;

namespace {
void test_speed() {
    const std::string board_str =
        "12|12|02|00|00|00|00|00|00|00|00|00|00|02|00|02|00|00|00|00|00|00|00|00|02|00|00|00|02|00|00|00|00|00|00|02|"
        "00|00|00|00|00|02|00|00|00|00|02|00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|"
        "00|00|00|00|00|00|00|00|00|00|00|00|03|00|00|03|00|00|00|00|00|00|00|00|00|00|00|00|00|02|00|03|00|00|02|00|"
        "00|00|00|00|02|00|00|00|00|00|00|02|00|00|00|02|00|00|00|03|00|00|01|00|02|00|02|00|00|00|00|00|00|00|00|00|"
        "00|02";
    TSPGameState init_state(board_str);

    std::cout << "starting ..." << std::endl;

    const auto t1 = high_resolution_clock::now();
    auto state = init_state;
    float sum = 0;
    uint64_t h = 0;
    for (int i = 0; i < NUM_STEPS; ++i) {
        auto child = state;
        child.apply_action(static_cast<Action>(i % TSPGameState::action_space_size()));
        const std::vector<float> obs = child.get_observation();
        (void)obs;
        sum += obs[0];
        const uint64_t hash = child.get_hash();
        (void)hash;
        h ^= hash;
        state = child;
    }
    const auto t2 = high_resolution_clock::now();
    const duration<double, std::milli> ms_double = t2 - t1;
    std::cout << sum << " " << h << std::endl;

    std::cout << "Total time for " << NUM_STEPS << " steps: " << ms_double.count() / MILLISECONDS_PER_SECOND
              << std::endl;
    std::cout << "Time per step :  " << ms_double.count() / MILLISECONDS_PER_SECOND / NUM_STEPS << std::endl;
}
}    // namespace

int main() {
    test_speed();
}
