#include <tsp/tsp.h>

#include <iostream>
#include <unordered_map>
#include <unordered_set>

using namespace tsp;

namespace {

auto count_states(TSPDeadlockGameState start_state) -> std::tuple<long long int, long long int> {
    using Hash256T = decltype(start_state.get_hash256());

    std::unordered_set<Hash256T> visited;
    std::unordered_set<Hash256T> solution_hashes;
    std::vector<TSPDeadlockGameState> frontier;

    auto root_hash = start_state.get_hash256();
    visited.insert(root_hash);
    if (start_state.is_solution()) {
        solution_hashes.insert(root_hash);
    }
    frontier.push_back(std::move(start_state));

    while (!frontier.empty()) {
        auto state = std::move(frontier.back());
        frontier.pop_back();

        auto state_hash = state.get_hash256();
        if (state.is_solution()) {
            solution_hashes.insert(state_hash);
        }

        if (state.is_solution()) {
            continue;
        }

        for (int action = 0; action < kNumActions; ++action) {
            auto child = state;
            child.apply_action(Action{action});

            auto child_hash = child.get_hash256();
            if (child.is_solution()) {
                solution_hashes.insert(child_hash);
            }

            auto [_, inserted] = visited.insert(child_hash);
            if (inserted) {
                frontier.push_back(std::move(child));
            }
        }
    }

    return {static_cast<long long int>(visited.size()), static_cast<long long int>(solution_hashes.size())};
}

struct DeadlockStateHash {
    auto operator()(const TSPDeadlockGameState& s) const noexcept -> std::size_t {
        return std::hash<decltype(s.get_hash256())>{}(s.get_hash256());
    }
};

struct DeadlockStateEq {
    auto operator()(const TSPDeadlockGameState& a, const TSPDeadlockGameState& b) const noexcept -> bool {
        return a == b;
    }
};

auto count_states2(TSPDeadlockGameState start_state) -> std::tuple<long long int, long long int> {
    std::unordered_set<TSPDeadlockGameState, DeadlockStateHash> visited;
    std::unordered_set<TSPDeadlockGameState, DeadlockStateHash> solution_states;
    std::vector<TSPDeadlockGameState> frontier;

    visited.insert(start_state);
    if (start_state.is_solution()) {
        solution_states.insert(start_state);
    }
    frontier.push_back(std::move(start_state));

    while (!frontier.empty()) {
        auto state = std::move(frontier.back());
        frontier.pop_back();

        if (state.is_solution()) {
            solution_states.insert(state);
        }

        if (state.is_solution()) {
            continue;
        }

        for (int action = 0; action < kNumActions; ++action) {
            auto child = state;
            child.apply_action(Action{action});

            if (child.is_solution()) {
                solution_states.insert(child);
            }

            auto [_, inserted] = visited.insert(child);
            if (inserted) {
                frontier.push_back(std::move(child));
            }
        }
    }

    return {static_cast<long long int>(visited.size()), static_cast<long long int>(solution_states.size())};
}
}    // namespace

int main() {
    const std::string board_str =
        "10|10|02|00|00|00|00|00|03|00|00|02|00|02|00|00|00|03|00|03|02|00|00|00|00|00|00|00|03|00|00|00|00|00|00|02|"
        "00|00|02|00|00|00|00|03|00|00|00|00|00|00|00|03|00|00|00|00|03|00|00|03|00|03|00|00|00|02|00|00|02|00|00|00|"
        "00|00|00|00|00|00|00|00|03|00|00|02|00|00|03|00|00|00|02|00|02|00|00|00|00|00|07|00|00|02";
    TSPDeadlockGameState init_state(board_str);
    auto [num_states, num_solutions] = count_states(init_state);
    std::cout << num_solutions << std::endl;
    auto [num_states2, num_solutions2] = count_states2(init_state);
    std::cout << num_solutions2 << std::endl;
    return num_solutions > 0;
}
