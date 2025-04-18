#include "tsp_base.h"

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

// https://en.wikipedia.org/wiki/Xorshift
// Portable RNG Seed
constexpr uint64_t SPLIT64_S1 = 30;
constexpr uint64_t SPLIT64_S2 = 27;
constexpr uint64_t SPLIT64_S3 = 31;
constexpr uint64_t SPLIT64_C1 = 0x9E3779B97f4A7C15;
constexpr uint64_t SPLIT64_C2 = 0xBF58476D1CE4E5B9;
constexpr uint64_t SPLIT64_C3 = 0x94D049BB133111EB;
auto to_local_hash(int flat_size, Element el, int offset) noexcept -> uint64_t {
    uint64_t seed = (flat_size * static_cast<int>(el)) + offset;
    uint64_t result = seed + SPLIT64_C1;
    result = (result ^ (result >> SPLIT64_S1)) * SPLIT64_C2;
    result = (result ^ (result >> SPLIT64_S2)) * SPLIT64_C3;
    return result ^ (result >> SPLIT64_S3);
}

}    // namespace

TSPGameState::TSPGameState(const std::string& board_str) {
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

    // Parse
    for (int i = 2; i < static_cast<int>(seglist.size()); ++i) {
        int el_idx = std::stoi(seglist[i]);
        if (el_idx < 0 || el_idx > 3) {
            std::cerr << board_str << std::endl;
            std::cerr << el_idx << std::endl;
            throw std::invalid_argument("Unknown element type.");
        }
        const auto el = static_cast<Element>(el_idx);
        visited_flags.push_back(el == Element::kCityUnvisited ? false : true);
        bool is_city = el == Element::kCityUnvisited;
        hash ^= is_city ? to_local_hash(rows * cols, Element::kCityUnvisited, agent_idx) : 0;
        board_is_city.push_back(is_city);
        remaining_cities += is_city;
        board_is_wall.push_back(el == Element::kWall);
        if (el == Element::kAgent) {
            if (agent_idx != -1) {
                throw std::invalid_argument("More than one agent.");
            }
            agent_idx = i - 2;
            hash ^= to_local_hash(rows * cols, Element::kAgent, agent_idx);
        }
    }
    if (agent_idx == -1) {
        throw std::invalid_argument("Missing agent.");
    }
}

auto TSPGameState::operator==(const TSPGameState& other) const noexcept -> bool {
    return rows == other.rows && cols == other.cols && agent_idx == other.agent_idx &&
           start_city_idx == other.start_city_idx && remaining_cities == other.remaining_cities &&
           board_is_city == other.board_is_city && visited_flags == other.visited_flags &&
           board_is_wall == other.board_is_wall;
}

auto TSPGameState::operator!=(const TSPGameState& other) const noexcept -> bool {
    return !(*this == other);
}

// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------

void TSPGameState::apply_action(Action action) {
    reward_signal = 0;

    // Do nothing if move puts agent out of bounds or into wall
    const auto& [new_idx, in_bounds] = IndexAndBoundsCheck(action);
    if (!in_bounds || board_is_wall[new_idx]) {
        return;
    }

    auto get_agent_type = [&]() -> Element {
        bool on_city = board_is_city[static_cast<std::size_t>(agent_idx)];
        bool on_start_city = agent_idx == start_city_idx;
        return on_city ? (on_start_city ? Element::kAgentAtStartCity : Element::kAgentAtCity) : Element::kAgent;
    };

    // Undo agent hash
    hash ^= to_local_hash(rows * cols, get_agent_type(), agent_idx);

    // Move agent
    agent_idx = new_idx;
    bool on_city = board_is_city[static_cast<std::size_t>(agent_idx)];
    bool set_visited_city = on_city && !visited_flags[static_cast<std::size_t>(agent_idx)];
    bool set_start_city = on_city && start_city_idx == -1;
    reward_signal = set_visited_city;
    remaining_cities -= set_visited_city;
    visited_flags[static_cast<std::size_t>(agent_idx)] = true;
    // Set start city if on city and start not set yet, else keep same
    hash ^= set_visited_city ? to_local_hash(rows * cols, Element::kCityUnvisited, agent_idx) : 0;
    hash ^= (set_visited_city && !set_start_city) ? to_local_hash(rows * cols, Element::kCityVisited, agent_idx) : 0;
    hash ^= set_start_city ? to_local_hash(rows * cols, Element::kStartCity, agent_idx) : 0;
    start_city_idx = set_start_city ? agent_idx : start_city_idx;

    // Update agent hash
    hash ^= to_local_hash(rows * cols, get_agent_type(), agent_idx);
}

auto TSPGameState::is_solution() const noexcept -> bool {
    return remaining_cities == 0 && agent_idx == start_city_idx;
}

auto TSPGameState::observation_shape() const noexcept -> std::array<int, 3> {
    return {kNumChannels, cols, rows};
}

auto TSPGameState::get_observation() const noexcept -> std::vector<float> {
    const auto channel_length = rows * cols;
    std::vector<float> obs(kNumChannels * channel_length, 0);

    bool on_city = board_is_city[static_cast<std::size_t>(agent_idx)];
    bool on_start_city = agent_idx == start_city_idx;

    // Fill board (elements which are not empty)
    for (int i = 0; i < channel_length; ++i) {
        auto el = Element::kEmpty;
        el = board_is_wall[static_cast<std::size_t>(i)] ? Element::kWall : el;
        el = board_is_city[static_cast<std::size_t>(i)]
                 ? (visited_flags[static_cast<std::size_t>(i)] ? Element::kCityVisited : Element::kCityUnvisited)
                 : el;
        el = (i == start_city_idx) ? Element::kStartCity : el;
        el = (i == agent_idx) ? Element::kAgent : el;
        el = (i == agent_idx && on_city) ? Element::kAgentAtCity : el;
        el = (i == agent_idx && on_start_city) ? Element::kAgentAtStartCity : el;
        obs[static_cast<std::size_t>(el) * channel_length + i] = 1;
    }
    return obs;
}

auto TSPGameState::image_shape() const noexcept -> std::array<int, 3> {
    return {rows * SPRITE_HEIGHT, cols * SPRITE_WIDTH, SPRITE_CHANNELS};
}

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

auto TSPGameState::to_image() const noexcept -> std::vector<uint8_t> {
    const auto channel_length = rows * cols;
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

auto TSPGameState::get_reward_signal() const noexcept -> uint64_t {
    return reward_signal;
}

auto TSPGameState::get_hash() const noexcept -> uint64_t {
    return hash;
}

auto TSPGameState::get_agent_index() const noexcept -> int {
    return agent_idx;
}

auto TSPGameState::get_start_city_index() const noexcept -> int {
    return start_city_idx;
}

auto TSPGameState::get_unvisited_city_indices() const noexcept -> std::vector<int> {
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

auto TSPGameState::get_visited_city_indices() const noexcept -> std::vector<int> {
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

auto TSPGameState::IndexAndBoundsCheck(Action action) const noexcept -> std::pair<int, bool> {
    auto col = agent_idx % cols;
    auto row = (agent_idx - col) / cols;
    const auto& offsets = kActionOffsets[static_cast<std::size_t>(action)];    // NOLINT(*-bounds-constant-array-index)
    col = col + offsets.first;
    row = row + offsets.second;
    bool in_bounds = col >= 0 && col < cols && row >= 0 && row < rows;
    return {(cols * row) + col, in_bounds};
}

auto operator<<(std::ostream& os, const TSPGameState& state) -> std::ostream& {
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
    return os;
}

}    // namespace tsp
