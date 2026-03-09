//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SUCCESSOR_GENERATOR_ABSTRACT_H
#define PLAJA_SUCCESSOR_GENERATOR_ABSTRACT_H

#include <list>
#include <memory>
#include <vector>
#include "../../../parser/ast/forward_ast.h"
#include "../../../stats/forward_stats.h"
#include "../../../assertions.h"
#include "../../factories/forward_factories.h"
#include "../../information/forward_information.h"
#include "../../information/synchronisation_information.h"
#include "../../smt/forward_smt_z3.h"
#include "../../smt/using_smt.h"
#include "../../successor_generation/base/iterators.h"
#include "../../successor_generation/forward_successor_generation.h"
#include "../pa_states/forward_pa_states.h"
#include "forward_succ_gen_pa.h"

namespace SUCCESSOR_GENERATOR_ABSTRACT { struct EdgesPerLabel; }

class SuccessorGeneratorAbstract {
    friend struct SuccessorGeneratorAbstractExplorer;

protected:
    const SuccessorGeneratorC* parent;

private:
    // model data:
    const ModelZ3* modelZ3;
    const ModelInformation* modelInformation;
    const SynchronisationInformation* syncInformation;
    PLAJA::StatsBase* sharedStatistics;

    // stored successor information
    std::vector<std::vector<SUCCESSOR_GENERATOR_ABSTRACT::EdgesPerLabel>> edgesPerAutoPerLoc; // per automaton per location the edges

    // per state exploration *auxiliary* structures
    std::vector<unsigned int> enabledSynchronisations; // number of (enabled) action ops per sync
    std::vector<std::vector<std::list<EdgeID_type>>> enabledEdges; // enabled edges per automaton and per non-silent action
    //
    Z3_IN_PLAJA::SMTSolver* cachedSolver;

    // action op construction/exploration time sub-routines:
    void compute_enabled_synchronisations(); // compute number of enabled action ops per sync
    void reset_exploration_structures();

protected:

    // action op construction structures
    std::vector<std::unique_ptr<ActionOpAbstract>> nonSyncOps;
    std::vector<std::vector<std::unique_ptr<ActionOpAbstract>>> syncOps;

    // actual operator list at exploration time
    std::list<ActionOpAbstract*> silentOps; // action ops labeled with silent action (guard satisfiability may/must be checked at iteration)
    std::vector<std::list<ActionOpAbstract*>> labeledOps; // action ops per action label (guard satisfiability may/must be checked at iteration)

    void initialize_edges(); // solver must contain src bounds already
    void initialize_action_ops();

    explicit SuccessorGeneratorAbstract(const ModelZ3& model_z3, const PLAJA::Configuration& config);
public:
    virtual ~SuccessorGeneratorAbstract();
    static std::unique_ptr<SuccessorGeneratorAbstract> construct(const ModelZ3& model_z3, const PLAJA::Configuration& config); // hack to allow splitting construction in derived classes
    DELETE_CONSTRUCTOR(SuccessorGeneratorAbstract)

    [[nodiscard]] const ModelZ3& get_model_z3() const { return *modelZ3; }

    [[nodiscard]] PLAJA::StatsBase* share_stats() const { return sharedStatistics; }

    /**
     * Determine applicable action ops.
     * @param state for checking current location state explicitly.
     * @param solver asserts source state.
     */
    void explore(const AbstractState& abstract_state, Z3_IN_PLAJA::SMTSolver& solver);
    void explore_for_incremental(const AbstractState& abstract_state, Z3_IN_PLAJA::SMTSolver& solver); // determine applicable action ops modulo guard enabled.
    void explore(Z3_IN_PLAJA::SMTSolver& solver); // locations not checked explicitly
    void explore(); // basically to iterate all action ops

    [[nodiscard]] const SuccessorGeneratorC& get_concrete() const { return *parent; }

    [[nodiscard]] const ActionOpAbstract& get_action_op(ActionOpID_type action_op_id) const;
    [[nodiscard]] const ActionOp& get_action_op_concrete(ActionOpID_type action_op_id) const;
    [[nodiscard]] ActionLabel_type get_action_label(ActionOpID_type action_op_id) const;

    [[nodiscard]] std::list<const ActionOpAbstract*> extract_ops_per_label(ActionLabel_type action_label) const;

    // [[nodiscard]] z3::expr extract_action_in_z3(ActionLabel_type action_label) const; // With respect to explore.

    [[nodiscard]] std::list<const ActionOp*> get_action_concrete(const class StateBase& state, ActionLabel_type action_label) const;

    /** Iterators *****************************************************************************************************/

    [[nodiscard]] inline auto init_silent_action_it() const { return ActionOpIteratorBase<ActionOpAbstract, ActionOpAbstract*, std::list>(silentOps); }

    [[nodiscard]] inline auto init_labeled_action_it(ActionLabel_type action_label) const {
        PLAJA_ASSERT(0 <= action_label && action_label < labeledOps.size())
        return ActionOpIteratorBase<ActionOpAbstract, ActionOpAbstract*, std::list>(labeledOps[action_label]);
    }

    [[nodiscard]] inline auto init_action_it(ActionLabel_type action_label) const { return action_label == ACTION::silentLabel ? init_silent_action_it() : init_labeled_action_it(action_label); }

    [[nodiscard]] inline auto init_action_it() const { return ActionLabelIteratorBase<ActionOpAbstract, ActionOpAbstract*, std::list>(silentOps, labeledOps); }

};

namespace SUCCESSOR_GENERATOR_ABSTRACT { [[nodiscard]] extern std::list<const ActionOpAbstract*> extract_ops_per_label(const SuccessorGeneratorAbstract& successor_generator, ActionLabel_type action_label); }

#endif //PLAJA_SUCCESSOR_GENERATOR_ABSTRACT_H
