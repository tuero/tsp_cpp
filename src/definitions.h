#ifndef TSP_DEFS_H_
#define TSP_DEFS_H_

namespace tsp {

enum class Element : int {
    kEmpty = 0,
    kAgent = 1,
    kWall = 2,
    kCityUnvisited = 3,
    kCityVisited = 4,
    kStartCity = 5,
    kAgentAtCity = 6,
    kAgentAtStartCity = 7,
};

constexpr int kNumElements = 8;
constexpr int kNumChannels = kNumElements;

// Possible actions for the agent to take
enum class Action : int {
    kUp = 0,
    kRight = 1,
    kDown = 2,
    kLeft = 3,
};
constexpr int kNumActions = 4;

}    // namespace tsp

#endif    // TSP_DEFS_H_
