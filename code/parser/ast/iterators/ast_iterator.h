//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_AST_ITERATOR_H
#define PLAJA_AST_ITERATOR_H

#include <memory>
#include <vector>

template<typename Entry_t>
class AstIterator {
private:
    typename std::vector<std::unique_ptr<Entry_t>>::iterator it;
    const typename std::vector<std::unique_ptr<Entry_t>>::iterator itEnd;

public:
    explicit AstIterator(std::vector<std::unique_ptr<Entry_t>>& entries): it(entries.begin()), itEnd(entries.end()) {}

    inline void operator++() { ++it; }
    [[nodiscard]] inline bool end() const { return it == itEnd; }

    [[nodiscard]] inline Entry_t* operator()() const { return it->get(); }
    [[nodiscard]] inline Entry_t* operator->() const { return it->get(); }
    [[nodiscard]] inline Entry_t& operator*() const { return **it; }

    inline std::unique_ptr<Entry_t> set(std::unique_ptr<Entry_t>&& entry = nullptr) const { std::swap(*it, entry); return std::move(entry); }
};

template<typename Entry_t>
class AstConstIterator {
private:
    typename std::vector<std::unique_ptr<Entry_t>>::const_iterator it;
    const typename std::vector<std::unique_ptr<Entry_t>>::const_iterator itEnd;

public:
    explicit AstConstIterator(const std::vector<std::unique_ptr<Entry_t>>& entries): it(entries.cbegin()), itEnd(entries.cend()) {}

    inline void operator++() { ++it; }
    [[nodiscard]] inline bool end() const { return it == itEnd; }

    [[nodiscard]] inline const Entry_t* operator()() const { return it->get(); }
    [[nodiscard]] inline const Entry_t* operator->() const { return it->get(); }
    [[nodiscard]] inline const Entry_t& operator*() const { return **it; }
};

#endif //PLAJA_AST_ITERATOR_H
