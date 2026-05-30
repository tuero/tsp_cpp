#include <tsp/tsp.h>

#include <array>
#include <cassert>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace tsp {

namespace {
// Data
const std::array<std::string, kNumElements> kElementToStrMap{
    " ",    // kEmpty
    "@",    // kAgent
    "#",    // kWall
    ".",    // kCityUnvisited
    "!",    // kCityVisited
    "S",    // kStartCity
    "&",    // kAgentAtCity
    "$",    // kAgentAtStartCity
};

// Direction to offsets (col, row)
using Offset = std::pair<int, int>;
constexpr std::array<Offset, kNumActions> kActionOffsets{{
    {0, -1},    // Action::kUp
    {1, 0},     // Action::kRight
    {0, 1},     // Action::kDown
    {-1, 0},    // Action::kLeft
}};
static_assert(kActionOffsets.size() == kNumActions);

// Colour maps for state to image
struct Pixel {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};
const Pixel WHITE = {.r = 0xff, .g = 0xff, .b = 0xff};
const Pixel BLACK = {.r = 0x00, .g = 0x00, .b = 0x00};
const Pixel RED = {.r = 0xff, .g = 0x00, .b = 0x00};
const Pixel GREEN = {.r = 0x00, .g = 0xFF, .b = 0x00};
const Pixel BLUE = {.r = 0x00, .g = 0xFF, .b = 0xFF};
const Pixel YELLOW = {.r = 0xFF, .g = 0xFF, .b = 0x00};
const Pixel MAGENTA = {.r = 0xFF, .g = 0x00, .b = 0xFF};
const Pixel GREY = {.r = 0xA9, .g = 0xA9, .b = 0xA9};
const std::unordered_map<Element, Pixel> kElementToPixelMap{
    {Element::kAgent, BLACK},        {Element::kEmpty, WHITE},
    {Element::kWall, GREY},          {Element::kCityUnvisited, RED},
    {Element::kCityVisited, GREEN},  {Element::kStartCity, BLUE},
    {Element::kAgentAtCity, YELLOW}, {Element::kAgentAtStartCity, MAGENTA},
};

auto deadlock_hash_token(int flat_size) -> Zobrist256 {
    Zobrist256 out{};
    const uint64_t base = static_cast<uint64_t>(flat_size) * static_cast<uint64_t>(kNumElements + 1);
    for (std::size_t lane = 0; lane < out.word.size(); ++lane) {
        out.word[lane] = splitmix64(base + static_cast<uint64_t>(lane));
    }
    return out;
}

}    // namespace

namespace detail {

template <bool IsDeadlock, StaticString name_str>
TSPGameStateImpl<IsDeadlock, name_str>::TSPGameStateImpl(const std::string& board_str) {
    std::stringstream board_ss(board_str);
    std::string segment;
    std::vector<std::string> seglist;
    // string split on |
    while (std::getline(board_ss, segment, '|')) {
        seglist.push_back(segment);
    }

    // Check input
    if (seglist.size() < 2) {
        throw std::invalid_argument("Board string should have at minimum 3 values separated by '|'.");
    }
    rows = std::stoi(seglist[0]);
    cols = std::stoi(seglist[1]);
    if (seglist.size() != static_cast<std::size_t>(rows * cols) + 2) {
        throw std::invalid_argument("Supplied rows/cols does not match input board length.");
    }
    is_deadlocked = false;

    // Parse
    hash = {};
    for (int i = 2; i < static_cast<int>(seglist.size()); ++i) {
        int el_idx = std::stoi(seglist[static_cast<std::size_t>(i)]);
        if (el_idx < 0 || el_idx >= kNumElements) {
            std::cerr << board_str << std::endl;
            std::cerr << el_idx << std::endl;
            throw std::invalid_argument("Unknown element type.");
        }
        const auto el = static_cast<Element>(el_idx);
        visited_flags.push_back(el == Element::kCityUnvisited ? false : true);
        const int idx = i - 2;
        bool is_city = (el == Element::kCityUnvisited || el == Element::kCityVisited || el == Element::kStartCity ||
                        el == Element::kAgentAtCity || el == Element::kAgentAtStartCity);
        hash ^= is_city ? to_local_hash<4>(rows * cols, Element::kCityUnvisited, idx) : Zobrist256{};
        board_is_city.push_back(is_city);
        remaining_cities += is_city;
        board_is_wall.push_back(el == Element::kWall);
        // If starting at a city, undo count for city remaining and set the starting city index
        if (el == Element::kAgentAtStartCity) {
            start_city_idx = idx;
            --remaining_cities;
        }
        if (el == Element::kAgent || el == Element::kAgentAtStartCity) {
            if (agent_idx != -1) {
                throw std::invalid_argument("More than one agent.");
            }
            agent_idx = idx;
            hash ^= to_local_hash<4>(rows * cols, el, agent_idx);
        }
    }
    if (agent_idx == -1) {
        throw std::invalid_argument("Missing agent.");
    }
}

template <bool IsDeadlock, StaticString name_str>
TSPGameStateImpl<IsDeadlock, name_str>::TSPGameStateImpl(InternalState&& internal_state)
    : rows(internal_state.rows),
      cols(internal_state.cols),
      agent_idx(internal_state.agent_idx),
      start_city_idx(internal_state.start_city_idx),
      remaining_cities(internal_state.remaining_cities),
      hash(internal_state.hash),
      reward_signal(internal_state.reward_signal),
      board_is_city(std::move(internal_state).board_is_city),
      visited_flags(std::move(internal_state).visited_flags),
      board_is_wall(std::move(internal_state).board_is_wall),
      is_deadlocked(internal_state.is_deadlocked) {}

template <bool IsDeadlock, StaticString name_str>
auto TSPGameStateImpl<IsDeadlock, name_str>::operator==(const TSPGameStateImpl& other) const noexcept -> bool {
    return rows == other.rows && cols == other.cols && agent_idx == other.agent_idx &&
           start_city_idx == other.start_city_idx && remaining_cities == other.remaining_cities &&
           board_is_city == other.board_is_city && visited_flags == other.visited_flags &&
           board_is_wall == other.board_is_wall && is_deadlocked == other.is_deadlocked;
}

template <bool IsDeadlock, StaticString name_str>
auto TSPGameStateImpl<IsDeadlock, name_str>::operator!=(const TSPGameStateImpl& other) const noexcept -> bool {
    return !(*this == other);
}

// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------

template <bool IsDeadlock, StaticString name_str>
void TSPGameStateImpl<IsDeadlock, name_str>::apply_action(Action action) {
    reward_signal = 0;

    // Do nothing if move puts agent out of bounds or into wall
    const auto& [new_idx, in_bounds] = IndexAndBoundsCheck(action);
    if (!in_bounds || board_is_wall[static_cast<std::size_t>(new_idx)]) {
        return;
    }

    auto get_agent_type = [&]() -> Element {
        bool on_city = board_is_city[static_cast<std::size_t>(agent_idx)];
        bool on_start_city = agent_idx == start_city_idx;
        return on_city ? (on_start_city ? Element::kAgentAtStartCity : Element::kAgentAtCity) : Element::kAgent;
    };

    // Undo agent hash
    hash ^= to_local_hash<4>(rows * cols, get_agent_type(), agent_idx);

    // Move agent
    agent_idx = new_idx;
    bool on_city = board_is_city[static_cast<std::size_t>(agent_idx)];
    bool set_visited_city = on_city && !visited_flags[static_cast<std::size_t>(agent_idx)];
    bool set_start_city = on_city && start_city_idx == -1;
    // Deadlocked if we revisit a city that is not starting city
    if constexpr (IsDeadlock) {
        const bool was_deadlocked = is_deadlocked;
        is_deadlocked = is_deadlocked ||
                        (on_city && visited_flags[static_cast<std::size_t>(agent_idx)] && agent_idx != start_city_idx);
        if (!was_deadlocked && is_deadlocked) {
            hash ^= deadlock_hash_token(rows * cols);
        }
    }
    reward_signal = set_visited_city;
    remaining_cities -= set_visited_city;
    visited_flags[static_cast<std::size_t>(agent_idx)] = true;
    // Set start city if on city and start not set yet, else keep same
    hash ^= set_visited_city ? to_local_hash<4>(rows * cols, Element::kCityUnvisited, agent_idx) : Zobrist256{};
    hash ^= (set_visited_city && !set_start_city) ? to_local_hash<4>(rows * cols, Element::kCityVisited, agent_idx)
                                                  : Zobrist256{};
    hash ^= set_start_city ? to_local_hash<4>(rows * cols, Element::kStartCity, agent_idx) : Zobrist256{};
    start_city_idx = set_start_city ? agent_idx : start_city_idx;

    // Update agent hash
    hash ^= to_local_hash<4>(rows * cols, get_agent_type(), agent_idx);
}

template <bool IsDeadlock, StaticString name_str>
auto TSPGameStateImpl<IsDeadlock, name_str>::is_solution() const noexcept -> bool {
    if constexpr (IsDeadlock) {
        return remaining_cities == 0 && agent_idx == start_city_idx && !is_deadlocked;
    } else {
        return remaining_cities == 0 && agent_idx == start_city_idx;
    }
}

template <bool IsDeadlock, StaticString name_str>
auto TSPGameStateImpl<IsDeadlock, name_str>::observation_shape() const noexcept -> std::array<int, 3> {
    return {kNumChannels, cols, rows};
}

template <bool IsDeadlock, StaticString name_str>
auto TSPGameStateImpl<IsDeadlock, name_str>::get_observation() const noexcept -> std::vector<float> {
    const auto channel_length = static_cast<std::size_t>(rows * cols);
    std::vector<float> obs(kNumChannels * channel_length, 0);

    bool on_city = board_is_city[static_cast<std::size_t>(agent_idx)];
    bool on_start_city = agent_idx == start_city_idx;

    // Fill board (elements which are not empty)
    for (std::size_t i = 0; i < channel_length; ++i) {
        auto el = Element::kEmpty;
        el = board_is_wall[i] ? Element::kWall : el;
        el = board_is_city[i] ? (visited_flags[i] ? Element::kCityVisited : Element::kCityUnvisited) : el;
        el = (i == static_cast<std::size_t>(start_city_idx)) ? Element::kStartCity : el;
        el = (i == static_cast<std::size_t>(agent_idx)) ? Element::kAgent : el;
        el = (i == static_cast<std::size_t>(agent_idx) && on_city) ? Element::kAgentAtCity : el;
        el = (i == static_cast<std::size_t>(agent_idx) && on_start_city) ? Element::kAgentAtStartCity : el;
        obs[static_cast<std::size_t>(el) * channel_length + i] = 1;
    }
    return obs;
}

template <bool IsDeadlock, StaticString name_str>
auto TSPGameStateImpl<IsDeadlock, name_str>::image_shape() const noexcept -> std::array<int, 3> {
    return {rows * SPRITE_HEIGHT, cols * SPRITE_WIDTH, SPRITE_CHANNELS};
}

namespace {
void fill_sprite(std::vector<uint8_t>& img, int h, int w, int cols, const Pixel& pixel) {
    const auto img_idx_top_left = h * (SPRITE_DATA_LEN * cols) + (w * SPRITE_DATA_LEN_PER_ROW);
    for (int r = 0; r < SPRITE_HEIGHT; ++r) {
        for (int c = 0; c < SPRITE_WIDTH; ++c) {
            const auto img_idx = static_cast<std::size_t>((r * SPRITE_DATA_LEN_PER_ROW * cols) + (SPRITE_CHANNELS * c) +
                                                          img_idx_top_left);
            img[img_idx + 0] = pixel.r;
            img[img_idx + 1] = pixel.g;
            img[img_idx + 2] = pixel.b;
        }
    }
}
}    // namespace

template <bool IsDeadlock, StaticString name_str>
auto TSPGameStateImpl<IsDeadlock, name_str>::to_image() const noexcept -> std::vector<uint8_t> {
    const auto channel_length = static_cast<std::size_t>(rows * cols);
    std::vector<uint8_t> img(channel_length * SPRITE_DATA_LEN, 0);

    bool on_city = board_is_city[static_cast<std::size_t>(agent_idx)];
    bool on_start_city = agent_idx == start_city_idx;

    int i = 0;
    for (int h = 0; h < rows; ++h) {
        for (int w = 0; w < cols; ++w) {
            auto el = Element::kEmpty;
            el = board_is_wall[static_cast<std::size_t>(i)] ? Element::kWall : el;
            el = board_is_city[static_cast<std::size_t>(i)]
                     ? (visited_flags[static_cast<std::size_t>(i)] ? Element::kCityVisited : Element::kCityUnvisited)
                     : el;
            el = (i == start_city_idx) ? Element::kStartCity : el;
            el = (i == agent_idx) ? Element::kAgent : el;
            el = (i == agent_idx && on_city) ? Element::kAgentAtCity : el;
            el = (i == agent_idx && on_start_city) ? Element::kAgentAtStartCity : el;
            fill_sprite(img, h, w, cols, kElementToPixelMap.at(el));
            ++i;
        }
    }
    return img;
}

template <bool IsDeadlock, StaticString name_str>
auto TSPGameStateImpl<IsDeadlock, name_str>::get_reward_signal() const noexcept -> uint64_t {
    return reward_signal;
}

template <bool IsDeadlock, StaticString name_str>
auto TSPGameStateImpl<IsDeadlock, name_str>::get_hash() const noexcept -> uint64_t {
    return hash.low64();
}

template <bool IsDeadlock, StaticString name_str>
auto TSPGameStateImpl<IsDeadlock, name_str>::get_hash256() const noexcept -> Zobrist256 {
    return hash;
}

template <bool IsDeadlock, StaticString name_str>
auto TSPGameStateImpl<IsDeadlock, name_str>::get_agent_index() const noexcept -> int {
    return agent_idx;
}

template <bool IsDeadlock, StaticString name_str>
auto TSPGameStateImpl<IsDeadlock, name_str>::get_start_city_index() const noexcept -> int {
    return start_city_idx;
}

template <bool IsDeadlock, StaticString name_str>
auto TSPGameStateImpl<IsDeadlock, name_str>::get_unvisited_city_indices() const noexcept -> std::vector<int> {
    std::vector<int> indices;
    for (int i = 0; i < rows * cols; ++i) {
        bool is_city = board_is_city[static_cast<std::size_t>(i)];
        bool is_visited = visited_flags[static_cast<std::size_t>(i)];
        if (is_city && !is_visited) {
            indices.push_back(i);
        }
    }
    return indices;
}

template <bool IsDeadlock, StaticString name_str>
auto TSPGameStateImpl<IsDeadlock, name_str>::get_visited_city_indices() const noexcept -> std::vector<int> {
    std::vector<int> indices;
    for (int i = 0; i < rows * cols; ++i) {
        bool is_city = board_is_city[static_cast<std::size_t>(i)];
        bool is_visited = visited_flags[static_cast<std::size_t>(i)];
        if (is_city && is_visited) {
            indices.push_back(i);
        }
    }
    return indices;
}

// ---------------------------------------------------------------------------

template <bool IsDeadlock, StaticString name_str>
auto TSPGameStateImpl<IsDeadlock, name_str>::IndexAndBoundsCheck(Action action) const noexcept -> std::pair<int, bool> {
    auto col = agent_idx % cols;
    auto row = (agent_idx - col) / cols;
    const auto& offsets = kActionOffsets[static_cast<std::size_t>(action)];    // NOLINT(*-bounds-constant-array-index)
    col = col + offsets.first;
    row = row + offsets.second;
    bool in_bounds = col >= 0 && col < cols && row >= 0 && row < rows;
    return {(cols * row) + col, in_bounds};
}

template <bool IsDeadlock, StaticString name_str>
auto operator<<(std::ostream& os, const detail::TSPGameStateImpl<IsDeadlock, name_str>& state) -> std::ostream& {
    const auto print_horz_boarder = [&]() {
        for (int w = 0; w < state.cols + 2; ++w) {
            os << "-";
        }
        os << std::endl;
    };

    bool on_city = state.board_is_city[static_cast<std::size_t>(state.agent_idx)];
    bool on_start_city = state.agent_idx == state.start_city_idx;

    // Board
    print_horz_boarder();
    int i = 0;
    for (int row = 0; row < state.rows; ++row) {
        os << "|";
        for (int col = 0; col < state.cols; ++col) {
            auto el = Element::kEmpty;
            el = state.board_is_wall[static_cast<std::size_t>(i)] ? Element::kWall : el;
            el = state.board_is_city[static_cast<std::size_t>(i)]
                     ? (state.visited_flags[static_cast<std::size_t>(i)] ? Element::kCityVisited
                                                                         : Element::kCityUnvisited)
                     : el;
            el = (i == state.start_city_idx) ? Element::kStartCity : el;
            el = (i == state.agent_idx) ? Element::kAgent : el;
            el = (i == state.agent_idx && on_city) ? Element::kAgentAtCity : el;
            el = (i == state.agent_idx && on_start_city) ? Element::kAgentAtStartCity : el;
            os << kElementToStrMap.at(static_cast<std::size_t>(el));
            ++i;
        }
        os << "|" << std::endl;
    }
    print_horz_boarder();
    std::cout << "is deadlocked " << state.is_deadlocked << std::endl;
    return os;
}
template std::ostream& operator<<(std::ostream&, const detail::TSPGameStateImpl<false, "tsp">&);
template std::ostream& operator<<(std::ostream&, const detail::TSPGameStateImpl<true, "tsp_deadlock">&);

}    // namespace detail

template class detail::TSPGameStateImpl<false, "tsp">;
template class detail::TSPGameStateImpl<true, "tsp_deadlock">;

}    // namespace tsp
