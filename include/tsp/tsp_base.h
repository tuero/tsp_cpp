#ifndef TSP_BASE_H_
#define TSP_BASE_H_

#include <tsp/definitions.h>

#include <array>
#include <cstdint>
#include <format>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace tsp {

// Image properties
constexpr int SPRITE_WIDTH = 32;
constexpr int SPRITE_HEIGHT = 32;
constexpr int SPRITE_CHANNELS = 3;
constexpr int SPRITE_DATA_LEN_PER_ROW = SPRITE_WIDTH * SPRITE_CHANNELS;
constexpr int SPRITE_DATA_LEN = SPRITE_WIDTH * SPRITE_HEIGHT * SPRITE_CHANNELS;

namespace detail {

// Static string
template <std::size_t N>
struct StaticString {
    std::array<char, N + 1> data = {};

    constexpr StaticString(const char (&input)[N + 1]) {    // NOLINT(*-avoid-c-arrays)
        std::ranges::copy_n(input, N + 1, data.begin());
    }
};

// Deduction guide
template <std::size_t N>
StaticString(const char (&)[N]) -> StaticString<N - 1>;    // NOLINT(*-avoid-c-arrays)

template <bool IsDeadlock, StaticString name_str>
class TSPGameStateImpl {
public:
    // Internal use for packing/unpacking with pybind11 pickle
    struct InternalState {
        int rows;
        int cols;
        int agent_idx;
        int start_city_idx;
        int remaining_cities;
        uint64_t hash;
        uint64_t reward_signal;
        std::vector<bool> board_is_city;
        std::vector<bool> visited_flags;
        std::vector<bool> board_is_wall;
        bool is_deadlocked;
    };

    TSPGameStateImpl() = delete;
    TSPGameStateImpl(const std::string& board_str);
    TSPGameStateImpl(InternalState&& internal_state);

    bool operator==(const TSPGameStateImpl& other) const noexcept;
    bool operator!=(const TSPGameStateImpl& other) const noexcept;

    static inline std::string name{name_str.data.data(), name_str.data.size() - 1};

    /**
     * Apply the action to the current state, and set the reward and signals.
     * @param action The action to apply, should be one of the legal actions
     */
    void apply_action(Action action);

    /**
     * Check if the state is in the solution state (agent visited all cities and returned back).
     * @return True if terminal, false otherwise
     */
    [[nodiscard]] auto is_solution() const noexcept -> bool;

    /**
     * Get the number of possible actions
     * @return Count of possible actions
     */
    [[nodiscard]] constexpr static auto action_space_size() noexcept -> int {
        return kNumActions;
    }

    /**
     * Get the shape the observations should be viewed as.
     * @return array indicating observation CHW
     */
    [[nodiscard]] auto observation_shape() const noexcept -> std::array<int, 3>;

    /**
     * Get a flat representation of the current state observation.
     * The observation should be viewed as the shape given by observation_shape().
     * @return vector where 1 represents object at position
     */
    [[nodiscard]] auto get_observation() const noexcept -> std::vector<float>;

    /**
     * Get the shape the image should be viewed as.
     * @return array indicating observation HWC
     */
    [[nodiscard]] auto image_shape() const noexcept -> std::array<int, 3>;

    /**
     * Get the flat (HWC) image representation of the current state
     * @return flattened byte vector represending RGB values (HWC)
     */
    [[nodiscard]] auto to_image() const noexcept -> std::vector<uint8_t>;

    /**
     * Get the current reward signal as a result of the previous action taken.
     * @return 0 if no reward, otherwise 1 if new city visited
     */
    [[nodiscard]] auto get_reward_signal() const noexcept -> uint64_t;

    /**
     * Get the hash representation for the current state.
     * @return hash value
     */
    [[nodiscard]] auto get_hash() const noexcept -> uint64_t;

    /**
     * Get the agent index position, even if in exit
     * @return Agent index
     */
    [[nodiscard]] auto get_agent_index() const noexcept -> int;

    /**
     * Get the index of the start city, or -1 if no city has been visited yet
     * @return the index
     */
    [[nodiscard]] auto get_start_city_index() const noexcept -> int;

    /**
     * Get the indices of the unvisited cities
     * @return Vector of unvisited cities
     */
    [[nodiscard]] auto get_unvisited_city_indices() const noexcept -> std::vector<int>;

    /**
     * Get the indices of the visited cities
     * @return Vector of visited cities
     */
    [[nodiscard]] auto get_visited_city_indices() const noexcept -> std::vector<int>;

    template <bool U, StaticString S>
    friend auto operator<<(std::ostream& os, const TSPGameStateImpl<U, S>& state) -> std::ostream&;

    [[nodiscard]] auto pack() const -> InternalState {
        return {rows,          cols,          agent_idx,     start_city_idx, remaining_cities, hash,
                reward_signal, board_is_city, visited_flags, board_is_wall,  is_deadlocked};
    }

private:
    void ParseBoard();
    [[nodiscard]] auto IndexFromAction(std::size_t index, Action action) const noexcept -> std::size_t;
    [[nodiscard]] auto IndexAndBoundsCheck(Action action) const noexcept -> std::pair<int, bool>;

    int rows = -1;
    int cols = -1;
    int agent_idx = -1;
    int start_city_idx = -1;
    int remaining_cities = 0;
    uint64_t hash = 0;
    uint64_t reward_signal = 0;
    std::vector<bool> board_is_city;
    std::vector<bool> visited_flags;
    std::vector<bool> board_is_wall;
    bool is_deadlocked = false;
};

}    // namespace detail

using TSPGameState = detail::TSPGameStateImpl<false, "tsp">;
using TSPDeadlockGameState = detail::TSPGameStateImpl<true, "tsp_deadlock">;

}    // namespace tsp

template <>
struct std::formatter<tsp::TSPDeadlockGameState> : std::formatter<std::string> {
    auto format(const tsp::TSPDeadlockGameState& s, format_context& ctx) const {
        std::ostringstream oss;
        oss << s;
        return formatter<string>::format(std::format("{}", oss.str()), ctx);
    }
};

template <>
struct std::formatter<tsp::TSPGameState> : std::formatter<std::string> {
    auto format(const tsp::TSPGameState& s, format_context& ctx) const {
        std::ostringstream oss;
        oss << s;
        return formatter<string>::format(std::format("{}", oss.str()), ctx);
    }
};

#endif    // TSP_BASE_H_
