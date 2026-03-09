//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_DEFAULT_MAP_H
#define PLAJA_DEFAULT_MAP_H

#include <unordered_map>
#include "../../assertions.h"
#include "../default_constructors.h"

namespace PLAJA {

    template<typename Key_t, typename Value_t>
    struct DefaultMap {
    private:
        std::unordered_map<Key_t, Value_t> mapping;
        Value_t defaultValue;
        FIELD_IF_DEBUG(bool isSet;)

    public:
        explicit DefaultMap(Value_t default_value):
            defaultValue(default_value)
            CONSTRUCT_IF_DEBUG(isSet(true)) {
        }

        ~DefaultMap() = default;
        DELETE_CONSTRUCTOR(DefaultMap)

        inline void set_default(Value_t value) {
            STMT_IF_DEBUG(isSet = true;)
            defaultValue = std::move(value);
        }

        inline void set(const Key_t& key, Value_t value) { mapping[key] = std::move(value); }

        inline void set(Key_t&& key, Value_t value) { mapping[std::move(key)] = std::move(value); }

        inline void unset(const Key_t& key) { mapping.erase(key); }

        inline void clear() {
            STMT_IF_DEBUG(isSet = false;)
            mapping.clear();
        }

        [[nodiscard]] inline const Value_t& get_default() const {
            PLAJA_ASSERT(isSet)
            return defaultValue;
        }

        [[nodiscard]] inline const Value_t& at(const Key_t& key) const {
            const auto it = mapping.find(key);
            return it == mapping.end() ? get_default() : it->second;
        }

        [[nodiscard]] inline bool exists(const Key_t& key) const { return mapping.count(key); }

        [[nodiscard]] inline Value_t& access(const Key_t& key) {
            PLAJA_ASSERT(exists(key))
            return mapping.at(key);
        }

    };

}

#endif //PLAJA_DEFAULT_MAP_H
