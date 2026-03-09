//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//
#ifndef PLAJA_VARS_IN_VERITAS_H
#define PLAJA_VARS_IN_VERITAS_H

#include <memory>
#include <unordered_map>
#include "../../parser/using_parser.h"
#include "../../assertions.h"
#include "../information/model_information.h"
#include "../states/forward_states.h"
#include "forward_smt_veritas.h"
#include "using_veritas.h"

class StateIndexesInVeritas final {

private:
    const ModelInformation* modelInfo;
    VERITAS_IN_PLAJA::Context* context;

    /* Mapping information. */
    std::unordered_map<VariableIndex_type, VeritasVarIndex_type> stateIndexToVeritas;
    std::unordered_map<VeritasVarIndex_type, VariableIndex_type> veritasToStateIndex;

    /* To reduce code duplication. */
    void extract_solution(const VERITAS_IN_PLAJA::Solution& solution, StateValues& solution_state, VariableIndex_type state_index, VeritasVarIndex_type veritas_var) const;

public:
    StateIndexesInVeritas(const ModelInformation& model_info, VERITAS_IN_PLAJA::Context& c);
    FCT_IF_DEBUG(explicit StateIndexesInVeritas(VERITAS_IN_PLAJA::Context&c);)
    ~StateIndexesInVeritas();

    StateIndexesInVeritas(const StateIndexesInVeritas& other) = default;
    StateIndexesInVeritas(StateIndexesInVeritas&& other) = delete;
    DELETE_ASSIGNMENT(StateIndexesInVeritas)

    [[nodiscard]] inline VERITAS_IN_PLAJA::Context& _context() const { return *context; }

    /* base information */

    [[nodiscard]] inline const std::unordered_map<VariableIndex_type, VeritasVarIndex_type>& _state_index_to_veritas() const { return stateIndexToVeritas; }

    [[nodiscard]] inline const std::unordered_map<VeritasVarIndex_type, VariableIndex_type>& _veritas_to_state_index() const { return veritasToStateIndex; }

    [[nodiscard]] inline bool empty() const { return stateIndexToVeritas.empty(); }

    /* retrieve information */

    [[nodiscard]] inline bool exists(VariableIndex_type state_index) const { return stateIndexToVeritas.count(state_index); }

    [[nodiscard]] inline VeritasVarIndex_type to_veritas(VariableIndex_type state_index) const {
        PLAJA_ASSERT(exists(state_index))
        return stateIndexToVeritas.at(state_index);
    }

    [[nodiscard]] inline bool exists_reverse(VeritasVarIndex_type veritas_index) const { return veritasToStateIndex.count(veritas_index); }

    [[nodiscard]] inline VariableIndex_type to_state_index(VeritasVarIndex_type veritas_index) const {
        PLAJA_ASSERT(exists_reverse(veritas_index))
        return veritasToStateIndex.at(veritas_index);
    }

    /* add information */

    VeritasVarIndex_type add(VariableIndex_type state_index, bool mark_integer);

    void set(VariableIndex_type state_index, VeritasVarIndex_type veritas_var);

    FCT_IF_DEBUG(VeritasVarIndex_type add(VariableIndex_type state_index, VeritasFloating_type lb, VeritasFloating_type ub, bool is_integer);)

    /* */

    void extract_solution(const VERITAS_IN_PLAJA::Solution& solution, StateValues& solution_state, VariableIndex_type state_index) const;
    void extract_solution(const VERITAS_IN_PLAJA::Solution& solution, StateValues& solution_state, bool include_locs) const;
    [[nodiscard]] std::unique_ptr<VeritasConstraint> encode_solution(const VERITAS_IN_PLAJA::Solution& solution) const;
    [[nodiscard]] std::unique_ptr<VeritasConstraint> encode_state(const StateValues& state) const;

    /* */

    void compute_substitution(std::unordered_map<VeritasVarIndex_type, VeritasVarIndex_type>& substitution, const StateIndexesInVeritas& other) const;
    [[nodiscard]] std::unordered_map<VeritasVarIndex_type, VeritasVarIndex_type> compute_substitution(const StateIndexesInVeritas& other) const;

    /******************************************************************************************************************/

    class Iterator {
        friend StateIndexesInVeritas;
    protected:
        const StateIndexesInVeritas* stateIndexes;
        std::unordered_map<VariableIndex_type, VeritasVarIndex_type>::const_iterator it;
        const std::unordered_map<VariableIndex_type, VeritasVarIndex_type>::const_iterator itEnd;

        explicit Iterator(const StateIndexesInVeritas& state_indexes):
            stateIndexes(&state_indexes)
            , it(state_indexes._state_index_to_veritas().cbegin())
            , itEnd(state_indexes._state_index_to_veritas().cend()) {}

    public:
        ~Iterator() = default;
        DELETE_CONSTRUCTOR(Iterator)

        inline void operator++() { ++it; }

        [[nodiscard]] inline bool end() const { return it == itEnd; } // stop *after* max step
        [[nodiscard]] inline VariableIndex_type state_index() const { return it->first; }

        [[nodiscard]] inline VeritasVarIndex_type veritas_var() const { return it->second; }
    };

    [[nodiscard]] inline Iterator iterator() const { return Iterator(*this); }

};

#endif //PLAJA_VARS_IN_VERITAS_H
