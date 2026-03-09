//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SUCCESSOR_GENERATOR_BASE_H
#define PLAJA_SUCCESSOR_GENERATOR_BASE_H

#include <memory>
#include "../../../include/ct_config_const.h"
#include "../../../parser/ast_forward_declarations.h"
#include "../../../utils/default_constructors.h"
#include "../../../assertions.h"
#include "../../factories/forward_factories.h"
#include "../../information/forward_information.h"
#include "../../information/synchronisation_information.h"
#include "../../states/forward_states.h"
#include "../../successor_generation/forward_successor_generation.h"
#include "../../using_search.h"

class SuccessorGeneratorBase {
    friend struct SuccessorGeneratorEvaluator;

private:
    // model data:
    const Model* model;
    const ModelInformation* modelInformation;
    const SynchronisationInformation* syncInformation;
    std::vector<std::vector<std::vector<const Edge*>>> edgesPerAutoPerLoc; // per automaton per location the edges

    // cached information
    std::size_t numberOfEnabledOps;

private:

#ifdef ENABLE_TERMINAL_STATE_SUPPORT
    const Expression* terminalStateCondition;
#endif

    /**
    * Set transient variables to their initial state.
    * For Runtime checks this will not reset the duplicate detection of state.
    * @param state
    */
    void initialise_transient(StateBase& state) const;

    /**
     * Execute the transient values of the locations in the state, i.p.
     * this includes setting non-assigned transient variables to their initial state.
     * @param state
     */
    void execute_transient_values(StateBase& state) const;

protected:
    explicit SuccessorGeneratorBase(const PLAJA::Configuration& config, const Model& model);

    [[nodiscard]] inline const SynchronisationInformation& get_sync_information() const { return *syncInformation; }

    [[nodiscard]] inline const std::vector<std::vector<std::vector<const Edge*>>>& get_edges_per_auto_per_loc() const { return edgesPerAutoPerLoc; }

    [[nodiscard]] inline std::size_t& access_num_of_enabled_ops() { return numberOfEnabledOps; }

    inline void reset_base() { numberOfEnabledOps = 0; }

public:
    virtual ~SuccessorGeneratorBase() = 0;
    DELETE_CONSTRUCTOR(SuccessorGeneratorBase)

    /* getter */

    [[nodiscard]] inline const Model& get_model() const { return *model; }

    [[nodiscard]] inline const ModelInformation& get_model_information() const { return *modelInformation; }

    [[nodiscard]] inline std::size_t _number_of_enabled_ops() const { return numberOfEnabledOps; }

    /* evaluate */

#ifdef ENABLE_TERMINAL_STATE_SUPPORT

    [[nodiscard]] bool is_terminal(const StateBase& state) const;

    [[nodiscard]] const Expression* get_terminal_state_condition() const { return terminalStateCondition; }

#else

    [[nodiscard]] inline constexpr bool is_terminal(const StateBase& /*state*/) const { return false; }

    [[nodiscard]] const Expression* get_terminal_state_condition() const { PLAJA_ABORT; }

#endif

    [[nodiscard]] std::unique_ptr<State> compute_successor(const Update& update, const State& source) const;

    [[nodiscard]] StateValues compute_successor(const Update& update, const StateValues& source) const;

    FCT_IF_DEBUG([[nodiscard]] bool has_transient_variables() const;)

    /******************************************************************************************************************/

    class ActionIdIterator final {
        friend SuccessorGeneratorBase;

    private:
        ActionID_type currentAction;
        const int numActions;

        explicit ActionIdIterator(const SynchronisationInformation* sync_info, bool include_silent);
    public:
        ~ActionIdIterator() = default;
        DELETE_CONSTRUCTOR(ActionIdIterator)

        inline void operator++() { ++currentAction; }

        [[nodiscard]] inline bool end() const { return currentAction >= numActions; }

        [[nodiscard]] ActionID_type get_id() { return currentAction; }

        [[nodiscard]] ActionLabel_type get_label() { return currentAction; }

    };

    [[nodiscard]] inline auto init_action_id_it(bool include_silent) const { return ActionIdIterator(syncInformation, include_silent); }

};

#endif //PLAJA_SUCCESSOR_GENERATOR_BASE_H
