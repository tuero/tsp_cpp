#include <tsp/tsp.h>

#include <iostream>
#include <unordered_map>
#include <unordered_set>

using namespace tsp;

namespace {
const std::unordered_map<std::string, int> ActionMap{
    {"w", 0}, {"d", 1}, {"s", 2}, {"a", 3}, {"e", 4},
};

void print_state(const TSPGameState &state) {
    std::cout << state << std::endl;
    std::cout << state.get_hash() << std::endl;
    std::cout << "Reward signal: " << state.get_reward_signal() << std::endl;
    std::cout << "Start city: " << state.get_start_city_index() << std::endl;
    std::cout << "Unvisited: ";
    for (const auto &v : state.get_unvisited_city_indices()) {
        std::cout << v << " ";
    }
    std::cout << std::endl;
    std::cout << "Visited: ";
    for (const auto &v : state.get_visited_city_indices()) {
        std::cout << v << " ";
    }
    std::cout << std::endl;
    const auto obs = state.get_observation();
    const auto [c, h, w] = state.observation_shape();

    for (int _h = 0; _h < h; ++_h) {
        for (int _w = 0; _w < w; ++_w) {
            int idx = -1;
            int count = 0;
            for (int _c = 0; _c < c; ++_c) {
                if (obs[_c * (h * w) + _h * w + _w] == 1) {
                    idx = _c;
                    ++count;
                }
            }
            if (count != 1) {
                std::cout << "err" << std::endl;
            }
            std::cout << idx << " ";
        }
        std::cout << std::endl;
    }
}

struct KeyHasher {
    std::size_t operator()(const TSPGameState &s) const {
        return s.get_hash();
    }
};

void test_play() {
    std::string board_str;
    std::cout << "Enter board str: ";
    std::cin >> board_str;

    std::unordered_set<uint64_t> seen_hashes;
    std::unordered_set<TSPGameState, KeyHasher> seen_states;

    TSPGameState state(board_str);
    print_state(state);
    seen_hashes.insert(state.get_hash());
    seen_states.insert(state);

    std::string action_str;
    while (!state.is_solution()) {
        std::cin >> action_str;
        if (ActionMap.find(action_str) != ActionMap.end()) {
            state.apply_action(static_cast<Action>(ActionMap.at(action_str)));
        }
        print_state(state);
        bool seen_hash = seen_hashes.find(state.get_hash()) != seen_hashes.end();
        bool seen_state = seen_states.find(state) != seen_states.end();
        std::cout << "Seen hash: " << seen_hash << ", seen state: " << seen_state << std::endl;
        seen_hashes.insert(state.get_hash());
        seen_states.insert(state);
    }
}
}    // namespace

int main() {
    test_play();
}
