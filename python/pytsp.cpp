// pytsp.cpp
// Python bindings

#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "tsp/tsp.h"

namespace py = pybind11;

PYBIND11_MODULE(pytsp, m) {
    m.doc() = "TSP environment module docs.";
    using T = tsp::TSPGameState;

    py::class_<T>(m, "TSPGameState")
        .def(py::init<const std::string &>())
        .def_readonly_static("name", &T::name)
        .def_readonly_static("num_actions", &tsp::kNumActions)
        .def(py::self == py::self)    // NOLINT (misc-redundant-expression)
        .def(py::self != py::self)    // NOLINT (misc-redundant-expression)
        .def("__hash__", [](const T &self) { return self.get_hash(); })
        .def("__copy__", [](const T &self) { return T(self); })
        .def("__deepcopy__", [](const T &self, py::dict) { return T(self); })
        .def("__repr__",
             [](const T &self) {
                 std::stringstream stream;
                 stream << self;
                 return stream.str();
             })
        .def(py::pickle(
            [](const T &self) {    // __getstate__
                auto s = self.pack();
                return py::make_tuple(s.rows, s.cols, s.agent_idx, s.start_city_idx, s.remaining_cities, s.hash,
                                      s.reward_signal, s.board_is_city, s.visited_flags, s.board_is_wall);
            },
            [](py::tuple t) -> T {    // __setstate__
                if (t.size() != 10) {
                    throw std::runtime_error("Invalid state");
                }
                T::InternalState s;
                s.rows = t[0].cast<int>();                           // NOLINT(*-magic-numbers)
                s.cols = t[1].cast<int>();                           // NOLINT(*-magic-numbers)
                s.agent_idx = t[2].cast<int>();                      // NOLINT(*-magic-numbers)
                s.start_city_idx = t[3].cast<int>();                 // NOLINT(*-magic-numbers)
                s.remaining_cities = t[4].cast<int>();               // NOLINT(*-magic-numbers)
                s.hash = t[5].cast<uint64_t>();                      // NOLINT(*-magic-numbers)
                s.reward_signal = t[6].cast<uint64_t>();             // NOLINT(*-magic-numbers)
                s.board_is_city = t[7].cast<std::vector<bool>>();    // NOLINT(*-magic-numbers)
                s.visited_flags = t[8].cast<std::vector<bool>>();    // NOLINT(*-magic-numbers)
                s.board_is_wall = t[9].cast<std::vector<bool>>();    // NOLINT(*-magic-numbers)
                return {std::move(s)};
            }))
        .def("apply_action",
             [](T &self, int action) {
                 if (action < 0 || action >= T::action_space_size()) {
                     throw std::invalid_argument("Invalid action.");
                 }
                 self.apply_action(static_cast<tsp::Action>(action));
             })
        .def("is_solution", &T::is_solution)
        .def("observation_shape", &T::observation_shape)
        .def("get_observation",
             [](const T &self) {
                 py::array_t<float> out = py::cast(self.get_observation());
                 return out.reshape(self.observation_shape());
             })
        .def("image_shape", &T::image_shape)
        .def("to_image",
             [](T &self) {
                 py::array_t<uint8_t> out = py::cast(self.to_image());
                 const auto obs_shape = self.observation_shape();
                 return out.reshape({static_cast<py::ssize_t>(obs_shape[1] * tsp::SPRITE_HEIGHT),
                                     static_cast<py::ssize_t>(obs_shape[2] * tsp::SPRITE_WIDTH),
                                     static_cast<py::ssize_t>(tsp::SPRITE_CHANNELS)});
             })
        .def("get_reward_signal", &T::get_reward_signal)
        .def("get_agent_index", &T::get_agent_index)
        .def("get_start_city_index", &T::get_start_city_index)
        .def("get_unvisited_city_indices", &T::get_unvisited_city_indices)
        .def("get_visited_city_indices", &T::get_visited_city_indices);
}
