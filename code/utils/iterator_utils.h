//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ITERATOR_UTILS_H
#define PLAJA_ITERATOR_UTILS_H

#include "utils.h"

template<typename EntryView_t, typename Entry_t, template<typename, typename> typename Container_t>
class ContainerIteratorConst {

private:
    typename Container_t<Entry_t, std::allocator<Entry_t>>::const_iterator it;
    typename Container_t<Entry_t, std::allocator<Entry_t>>::const_iterator itEnd;

public:
    explicit ContainerIteratorConst(const Container_t<Entry_t, std::allocator<Entry_t>>& container):
        it(container.cbegin())
        , itEnd(container.cend()) {
    }

    virtual ~ContainerIteratorConst() = default;
    DEFAULT_CONSTRUCTOR(ContainerIteratorConst)

    inline void operator++() { ++it; }

    [[nodiscard]] inline bool end() const { return it == itEnd; }

    [[nodiscard]] inline const EntryView_t* operator()() const {
        if constexpr (PLAJA_UTILS::is_unique_ptr<Entry_t>()) { return static_cast<const EntryView_t*>(it->get()); }
        else if constexpr (std::is_pointer_v<Entry_t>) { return static_cast<const EntryView_t*>(*it); }
        else { return &static_cast<const EntryView_t&>(*it); }
    }

    [[nodiscard]] inline const EntryView_t* operator->() const {
        if constexpr (PLAJA_UTILS::is_unique_ptr<Entry_t>()) { return static_cast<const EntryView_t*>(it->get()); }
        else if constexpr (std::is_pointer_v<Entry_t>) { return static_cast<const EntryView_t*>(*it); }
        else { return &static_cast<const EntryView_t&>(*it); }
    }

    [[nodiscard]] inline const EntryView_t& operator*() const {
        if constexpr (PLAJA_UTILS::is_ptr_type<Entry_t>()) { return static_cast<const EntryView_t&>(**it); }
        else { return static_cast<const EntryView_t&>(*it); }
    }

};

template<typename EntryView_t, typename Entry_t, template<typename, typename> typename Container_t>
class ContainerIterator {

private:
    typename Container_t<Entry_t, std::allocator<Entry_t>>::iterator it;
    typename Container_t<Entry_t, std::allocator<Entry_t>>::iterator itEnd;

public:
    explicit ContainerIterator(Container_t<Entry_t, std::allocator<Entry_t>>& container):
        it(container.begin())
        , itEnd(container.end()) {
    }

    virtual ~ContainerIterator() = default;
    DEFAULT_CONSTRUCTOR(ContainerIterator)

    inline void operator++() { ++it; }

    [[nodiscard]] inline bool end() const { return it == itEnd; }

    [[nodiscard]] inline EntryView_t* operator()() const {
        if constexpr (PLAJA_UTILS::is_unique_ptr<Entry_t>()) { return static_cast<EntryView_t*>(it->get()); }
        else if constexpr (std::is_pointer_v<Entry_t>) { return static_cast<EntryView_t*>(*it); }
        else { return &static_cast<EntryView_t&>(*it); }
    }

    [[nodiscard]] inline EntryView_t* operator->() const {
        if constexpr (PLAJA_UTILS::is_unique_ptr<Entry_t>()) { return static_cast<EntryView_t*>(it->get()); }
        else if constexpr (std::is_pointer_v<Entry_t>) { return static_cast<EntryView_t*>(*it); }
        else { return &static_cast<EntryView_t&>(*it); }
    }

    [[nodiscard]] inline EntryView_t& operator*() const {
        if constexpr (PLAJA_UTILS::is_ptr_type<Entry_t>()) { return static_cast<EntryView_t&>(**it); }
        else { return static_cast<EntryView_t&>(*it); }
    }

};

#endif //PLAJA_ITERATOR_UTILS_H
