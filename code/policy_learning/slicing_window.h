//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SLICING_WINDOW_H
#define PLAJA_SLICING_WINDOW_H

struct SlicingWindow {

private:
    std::list<double> window;
    std::size_t maxSize;

public:
    explicit SlicingWindow(std::size_t max_size):
        maxSize(max_size) {
    }

    ~SlicingWindow() = default;
    DELETE_CONSTRUCTOR(SlicingWindow)

    inline void add(double value) {
        if (window.size() >= maxSize) { window.pop_front(); }
        window.push_back(value);
    }

    [[nodiscard]] inline double average() const {
        double sum = 0;
        for (auto value: window) { sum += value; }
        PLAJA_ASSERT(not window.empty())
        return sum / PLAJA_UTILS::cast_numeric<double>(window.size());
    }

    [[nodiscard]] inline std::size_t slicing_size() const { return maxSize; }

};

#endif //PLAJA_SLICING_WINDOW_H
