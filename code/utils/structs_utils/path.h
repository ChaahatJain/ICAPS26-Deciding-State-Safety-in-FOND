//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PATH_H
#define PLAJA_PATH_H

#include <list>

template<typename PathElement_t>
struct Path {

private:
    std::list<PathElement_t> path;

public:
    inline void push_front(const PathElement_t& element) { path.push_front(element); }
    inline void push_back(const PathElement_t& element) { path.push_back(element); }
    inline void push_front(PathElement_t&& element) { path.push_front(std::move(element)); }
    inline void push_back(PathElement_t&& element) { path.push_back(std::move(element)); }
    template <class... Args>
    inline void emplace_front(Args&&... args) { path.emplace_front(args...); }
    template <class... Args>
    inline void emplace_back(Args&&... args) { path.emplace_back(args...); }

    [[nodiscard]] inline std::size_t size() const { return path.size(); }
    [[nodiscard]] inline bool empty() const { return path.empty(); }
    const PathElement_t& front() const { return path.front(); }
    const PathElement_t& back() const { return path.back(); }
    const std::list<PathElement_t>& _path() const { return path; }
    [[nodiscard]] inline typename std::list<PathElement_t>::const_iterator begin() const { return path.cbegin(); }
    [[nodiscard]] inline typename std::list<PathElement_t>::const_iterator end() const { return path.cend(); }

};

#endif //PLAJA_PATH_H
