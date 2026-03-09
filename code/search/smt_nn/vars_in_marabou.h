//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_VARS_IN_MARABOU_H
#define PLAJA_VARS_IN_MARABOU_H

#include <memory>
#include <unordered_map>
#include "../../parser/using_parser.h"
#include "../../assertions.h"
#include "../information/model_information.h"
#include "../states/forward_states.h"
#include "forward_smt_nn.h"
#include "using_marabou.h"

class StateIndexesInMarabou final {

private:
    const ModelInformation* modelInfo;
    MARABOU_IN_PLAJA::Context* context;

    /* Mapping information. */
    std::unordered_map<VariableIndex_type, MarabouVarIndex_type> stateIndexToMarabou;
    std::unordered_map<MarabouVarIndex_type, VariableIndex_type> marabouToStateIndex;

    /* To reduce code duplication. */
    void extract_solution(const MARABOU_IN_PLAJA::Solution& solution, StateValues& solution_state, VariableIndex_type state_index, MarabouVarIndex_type marabou_var) const;

public:
    StateIndexesInMarabou(const ModelInformation& model_info, MARABOU_IN_PLAJA::Context& c);
    FCT_IF_DEBUG(explicit StateIndexesInMarabou(MARABOU_IN_PLAJA::Context&c);)
    ~StateIndexesInMarabou();

    StateIndexesInMarabou(const StateIndexesInMarabou& other) = default;
    StateIndexesInMarabou(StateIndexesInMarabou&& other) = delete;
    DELETE_ASSIGNMENT(StateIndexesInMarabou)

    [[nodiscard]] inline MARABOU_IN_PLAJA::Context& get_context() const { return *context; }

    /* base information */

    [[nodiscard]] inline const std::unordered_map<VariableIndex_type, MarabouVarIndex_type>& _state_index_to_marabou() const { return stateIndexToMarabou; }

    [[nodiscard]] inline const std::unordered_map<MarabouVarIndex_type, VariableIndex_type>& _marabou_to_state_index() const { return marabouToStateIndex; }

    [[nodiscard]] inline bool empty() const { return stateIndexToMarabou.empty(); }

    /* retrieve information */

    [[nodiscard]] inline bool exists(VariableIndex_type state_index) const { return stateIndexToMarabou.count(state_index); }

    [[nodiscard]] inline MarabouVarIndex_type to_marabou(VariableIndex_type state_index) const {
        PLAJA_ASSERT(exists(state_index))
        return stateIndexToMarabou.at(state_index);
    }

    [[nodiscard]] inline bool exists_reverse(MarabouVarIndex_type marabou_index) const { return marabouToStateIndex.count(marabou_index); }

    [[nodiscard]] inline VariableIndex_type to_state_index(MarabouVarIndex_type marabou_index) const {
        PLAJA_ASSERT(exists_reverse(marabou_index))
        return marabouToStateIndex.at(marabou_index);
    }

    /* add information */

    MarabouVarIndex_type add(VariableIndex_type state_index, bool mark_integer, bool is_input);

    void set(VariableIndex_type state_index, MarabouVarIndex_type marabou_var);

    FCT_IF_DEBUG(MarabouVarIndex_type add(VariableIndex_type state_index, MarabouFloating_type lb, MarabouFloating_type ub, bool is_integer, bool is_input, bool is_output);)

    /* */

    void extract_solution(const MARABOU_IN_PLAJA::Solution& solution, StateValues& solution_state, VariableIndex_type state_index) const;
    void extract_solution(const MARABOU_IN_PLAJA::Solution& solution, StateValues& solution_state, bool include_locs) const;
    [[nodiscard]] std::unique_ptr<MarabouConstraint> encode_solution(const MARABOU_IN_PLAJA::Solution& solution) const;
    [[nodiscard]] std::unique_ptr<MarabouConstraint> encode_state(const StateValues& state) const;

    /* */

    void compute_substitution(std::unordered_map<MarabouVarIndex_type, MarabouVarIndex_type>& substitution, const StateIndexesInMarabou& other) const;
    [[nodiscard]] std::unordered_map<MarabouVarIndex_type, MarabouVarIndex_type> compute_substitution(const StateIndexesInMarabou& other) const;

    /* */

    FCT_IF_DEBUG(void init(bool do_locs, bool mark_input);)

    /******************************************************************************************************************/

    class Iterator {
        friend StateIndexesInMarabou;
    protected:
        const StateIndexesInMarabou* stateIndexes;
        std::unordered_map<VariableIndex_type, MarabouVarIndex_type>::const_iterator it;
        const std::unordered_map<VariableIndex_type, MarabouVarIndex_type>::const_iterator itEnd;

        explicit Iterator(const StateIndexesInMarabou& state_indexes):
            stateIndexes(&state_indexes)
            , it(state_indexes._state_index_to_marabou().cbegin())
            , itEnd(state_indexes._state_index_to_marabou().cend()) {}

    public:
        ~Iterator() = default;
        DELETE_CONSTRUCTOR(Iterator)

        inline void operator++() { ++it; }

        [[nodiscard]] inline bool end() const { return it == itEnd; } // stop *after* max step
        [[nodiscard]] inline VariableIndex_type state_index() const { return it->first; }

        [[nodiscard]] inline MarabouVarIndex_type marabou_var() const { return it->second; }
    };

    [[nodiscard]] inline Iterator iterator() const { return Iterator(*this); }

};

#endif //PLAJA_VARS_IN_MARABOU_H
