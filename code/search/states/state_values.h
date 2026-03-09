//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_STATE_VALUES_H
#define PLAJA_STATE_VALUES_H

#include <memory>
#include <vector>
#include "state_base.h"
#include "forward_states.h"
#ifdef USE_VERITAS
    #include <interval.hpp>
#endif


class StateValues final: public StateBase {

private:
    std::vector<PLAJA::integer> valuesInt;
    std::vector<PLAJA::floating> valuesFloating;

public:
    explicit StateValues(const State& state);
    explicit StateValues(std::vector<PLAJA::integer> values_int, std::vector<PLAJA::floating> values_float = {});
    explicit StateValues(std::size_t int_state_size, std::size_t floating_state_size = 0);
    ~StateValues() final = default;

    StateValues(const StateValues& other) = default;
    StateValues& operator=(const StateValues& other) = delete;
    StateValues(StateValues&& other) = default;
    StateValues& operator=(StateValues&& other) = default;

    /* Interface. */
    [[nodiscard]] std::size_t get_int_state_size() const override { return valuesInt.size(); }

    [[nodiscard]] std::size_t get_floating_state_size() const override { return valuesFloating.size(); }


    //
    [[nodiscard]] PLAJA::integer get_int(VariableIndex_type var) const override;
    [[nodiscard]] PLAJA::floating get_float(VariableIndex_type var) const override;
    //
    void set_int(VariableIndex_type var, PLAJA::integer value) override;
    void set_float(VariableIndex_type var, PLAJA::floating value) override;

    //

    [[nodiscard]] StateValues to_state_values() const override { return { *this }; }

    [[nodiscard]] inline std::unique_ptr<StateValues> to_ptr() const { return std::make_unique<StateValues>(*this); }

    // FCT_IF_DEBUG([[nodiscard]] std::unique_ptr<const State> to_state() const;)
    [[nodiscard]] std::unique_ptr<const State> to_state() const; // not sure why this was only for debugging.

#ifdef USE_VERITAS
    [[nodiscard]] std::vector<veritas::Interval> get_box() const {
        std::vector<veritas::Interval> box;
        box.reserve(get_int_state_size() + get_floating_state_size() - 1);
        for (auto it = std::begin(valuesInt) + 1; it != std::end(valuesInt); ++it) {
            auto val = *it;
            veritas::Interval ival = {static_cast<double>(val), static_cast<double>(val) + 1};
            box.push_back(ival);
        }
        for (auto val: valuesFloating) {
            veritas::Interval ival = {static_cast<double>(val), static_cast<double>(val) + PLAJA::floatingPrecision};
            box.push_back(ival);
        }
        return box;
    }
#endif



};

#endif //PLAJA_STATE_VALUES_H
