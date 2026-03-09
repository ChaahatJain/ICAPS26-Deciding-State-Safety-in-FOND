//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "successor_generator_abstract.h"
#include "../../../parser/ast/expression/expression.h"
#include "../../../parser/ast/edge.h"
#include "../../../parser/ast/automaton.h"
#include "../../../parser/ast/model.h"
#include "../../../stats/stats_base.h"
#include "../../factories/configuration.h"
#include "../../information/model_information.h"
#include "../../successor_generation/base/enabled_sync_computation.h"
#include "../../successor_generation/successor_generator_c.h"
#include "../../smt/model/model_z3.h"
#include "../../smt/solver/smt_solver_z3.h"
#include "../../smt/successor_generation/op_applicability.h"
#include "../../smt/visitor/extern/to_z3_visitor.h"
#include "../pa_states/abstract_state.h"
#include "action_op/action_op_abstract.h"

// auxiliary:

namespace SUCCESSOR_GENERATOR_ABSTRACT {

    struct EdgeZ3 final {
    private:
        const Edge* edge;
        const ModelZ3* modelZ3;
        std::unique_ptr<z3::expr> guard;
        std::vector<VariableID_type> guardVars; // alt: z3::expr_vector
    public:
        explicit EdgeZ3(const Edge& edge_):
            edge(&edge_)
            , modelZ3(nullptr)
            , guard(nullptr) {}

        EdgeZ3(const Edge& edge_, const ModelZ3& model_z3):
            edge(&edge_)
            , modelZ3(&model_z3)
            , guard(nullptr) {
            const auto* guard_jani = edge->get_guardExpression();
            if (guard_jani) {
                if (guard_jani->is_constant()) { if (not guard_jani->evaluate_integer_const()) { guard = std::make_unique<z3::expr>(model_z3.get_context_z3().bool_val(false)); } } // "nullptr" corresponds to const-true
                else { guard = std::make_unique<z3::expr>(model_z3.to_z3_condition(*guard_jani, 0)); }
            }
        }

        ~EdgeZ3() = default;

        DELETE_CONSTRUCTOR(EdgeZ3)

        [[nodiscard]] inline const Edge* get_edge() const { return edge; }

        [[nodiscard]] inline EdgeID_type get_id() const { return edge->get_id(); }

        [[nodiscard]] inline ActionID_type get_action_id_labeled() const { return edge->get_action_id_labeled(); }

        [[nodiscard]] inline const z3::expr* get_guard() const { return guard.get(); }

        [[nodiscard]] inline bool guard_enabled(Z3_IN_PLAJA::SMTSolver& solver) const {
            PLAJA_ASSERT(modelZ3)
            if (not guard) { return true; }
            solver.push();
            for (const auto var: guardVars) { modelZ3->register_src_bound(solver, var); }
            solver.add(*guard);
            return solver.check_pop();
        }

    };

    struct EdgesPerLabel final {
    private:
        std::list<EdgeZ3> silentEdges;
        std::list<EdgeZ3> labeledEdges;
        std::list<EdgeZ3> singleOpEdges; // (special case) *labeled* edges contributing to a single operator
    public:
        [[nodiscard]] inline const std::list<EdgeZ3>& _silentEdges() const { return silentEdges; }

        [[nodiscard]] inline const std::list<EdgeZ3>& _labeledEdges() const { return labeledEdges; }

        [[nodiscard]] inline const std::list<EdgeZ3>& _singleOpEdges() const { return singleOpEdges; }

        inline void add_to_silent(const Edge& edge) { silentEdges.emplace_back(edge); }

        inline void add_to_labeled(const Edge& edge, const ModelZ3& model_z3) { labeledEdges.emplace_back(edge, model_z3); }

        inline void add_to_single_op(const Edge& edge) { singleOpEdges.emplace_back(edge); }
    };

    inline std::vector<std::vector<unsigned int>> compute_operators_per_edge(const SynchronisationInformation& sync_info, const std::vector<std::vector<std::list<EdgeID_type>>>& enabled_edges) {

        std::vector<std::vector<unsigned int>> operators_per_edge(sync_info.num_automata_instances, std::vector<unsigned>(sync_info.num_actions, 0));
        //
        for (AutomatonIndex_type automaton_index = 0; automaton_index < sync_info.num_automata_instances; ++automaton_index) {
            //
            for (ActionID_type action_id = 0; action_id < sync_info.num_actions; ++action_id) {
                unsigned int num_of_operators = 0;
                //
                for (const SyncIndex_type sync_index: sync_info.relevantSyncs[automaton_index][action_id]) {
                    unsigned int num_of_operators_per_sync = 1;
                    for (const auto& condition: sync_info.synchronisations[sync_index]) {
                        if (condition.automatonIndex == automaton_index) { continue; } // do not count same automaton edges
                        num_of_operators_per_sync *= enabled_edges[condition.automatonIndex][condition.actionID].size();
                    }
                    num_of_operators += num_of_operators_per_sync;
                }
                //
                operators_per_edge[automaton_index][action_id] = num_of_operators;
            }
        }

        return operators_per_edge;
    }

}

//

SuccessorGeneratorAbstract::SuccessorGeneratorAbstract(const ModelZ3& model_z3, const PLAJA::Configuration& config):
    parent(&model_z3.get_successor_generator())
    , modelZ3(&model_z3)
    , modelInformation(&modelZ3->get_model_info())
    , syncInformation(&modelInformation->get_synchronisation_information())
    , sharedStatistics(config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS))
    , cachedSolver(nullptr) {
    PLAJA_ASSERT(&parent->get_model() == &modelZ3->get_model())
    PLAJA_ASSERT(syncInformation->num_automata_instances == modelZ3->get_number_automata_instances())

    PLAJA_LOG_DEBUG_IF(not sharedStatistics, PLAJA_UTILS::to_red_string("PA-successor-generator without stats."))

    // storage maintenance
    edgesPerAutoPerLoc.resize(syncInformation->num_automata_instances);
    for (AutomatonIndex_type automaton_index = 0; automaton_index < syncInformation->num_automata_instances; ++automaton_index) {
        edgesPerAutoPerLoc[automaton_index].resize(modelInformation->get_range(automaton_index));
    }

    // initialise exploration structures
    SUCCESSOR_GENERATION::initialize_enablement_structures(*syncInformation, enabledEdges, enabledSynchronisations);
    labeledOps.resize(syncInformation->num_actions);
}

SuccessorGeneratorAbstract::~SuccessorGeneratorAbstract() = default;

std::unique_ptr<SuccessorGeneratorAbstract> SuccessorGeneratorAbstract::construct(const ModelZ3& model_z3, const PLAJA::Configuration& config) {
    std::unique_ptr<SuccessorGeneratorAbstract> successor_generator(new SuccessorGeneratorAbstract(model_z3, config));

    successor_generator->initialize_edges();
    successor_generator->initialize_action_ops();

    return successor_generator;
}

/**helper**************************************************************************************************************/

void SuccessorGeneratorAbstract::compute_enabled_synchronisations() {
    SUCCESSOR_GENERATION::compute_enabled_synchronisations(*syncInformation, enabledEdges, enabledSynchronisations);
}

void SuccessorGeneratorAbstract::reset_exploration_structures() {

    // Synchronisation data:
    for (SyncIndex_type sync_index = 0; sync_index < syncInformation->num_syncs; ++sync_index) { enabledSynchronisations[sync_index] = 0; }

    // Action ops:

    while (not silentOps.empty()) { silentOps.pop_back(); }

    for (auto& ops_per_action: labeledOps) { while (not ops_per_action.empty()) { ops_per_action.pop_back(); } }

    // Edges:
    for (auto& enabled_per_automaton: enabledEdges) { for (auto& enabled_per_action: enabled_per_automaton) { enabled_per_action.clear(); } }
}

/**********************************************************************************************************************/

void SuccessorGeneratorAbstract::initialize_edges() {
    // clear structures in case of re-usage
    reset_exploration_structures();

    const auto& model_jani = modelZ3->get_model();
    // iterate over edges and assign to proper structure
    std::vector<std::list<const Edge*>> sync_edges_per_auto_tmp(syncInformation->num_automata_instances); // tmp copy
    for (AutomatonIndex_type automaton_index = 0; automaton_index < syncInformation->num_automata_instances; ++automaton_index) { // for each automaton
        const auto* automaton = model_jani.get_automatonInstance(automaton_index);
        for (auto it_edge = automaton->edgeIterator(); !it_edge.end(); ++it_edge) { // for each edge...
            const auto* edge = it_edge(); // TODO MAYBE optimization: unsat-check of the guard, however, not implemented as modelling a unsat guard is pointless in the first place
            const auto* action_ptr = edge->get_action();
            if (action_ptr) { // synchronisation
                sync_edges_per_auto_tmp[automaton_index].push_back(edge);
                enabledEdges[automaton_index][action_ptr->get_id()].push_back(edge->get_id()); // Have to push value to make *compute_enabled_synchronisations* work properly.
            } else { // Immediately add to edges per loc.
                edgesPerAutoPerLoc[automaton_index][edge->get_location_value()].add_to_silent(*edge);
            }
        }
    }

    // sync edges
    std::vector<std::vector<unsigned int>> operators_per_edge = SUCCESSOR_GENERATOR_ABSTRACT::compute_operators_per_edge(*syncInformation, enabledEdges);
    for (AutomatonIndex_type automaton_index = 0; automaton_index < syncInformation->num_automata_instances; ++automaton_index) {
        for (const Edge* edge: sync_edges_per_auto_tmp[automaton_index]) {
            //
            const unsigned int num_of_operators = operators_per_edge[automaton_index][edge->get_action_id_labeled()];
            // PLAJA_ASSERT(num_of_operators >= 1) // if num_of_operators == 0 edge can be dropped, however not expected so far
            if (num_of_operators == 1) {
                edgesPerAutoPerLoc[automaton_index][edge->get_location_value()].add_to_single_op(*edge);
            } else if (num_of_operators > 1) {
                edgesPerAutoPerLoc[automaton_index][edge->get_location_value()].add_to_labeled(*edge, *modelZ3);
            }
        }
    }
}

void SuccessorGeneratorAbstract::initialize_action_ops() {
    // non sync ops
    nonSyncOps.clear();
    nonSyncOps.reserve(parent->nonSyncOps.size());
    for (const auto& non_sync_op: parent->nonSyncOps) { nonSyncOps.push_back(std::make_unique<ActionOpAbstract>(*non_sync_op, *modelZ3)); }

    // sync operators
    syncOps.clear();
    syncOps.reserve(parent->syncOps.size());
    for (const auto& sync_ops_per_label: parent->syncOps) {
        syncOps.emplace_back();
        auto& sync_ops_per_label_abstract = syncOps.back();
        sync_ops_per_label_abstract.reserve(sync_ops_per_label.size());
        for (const auto& sync_op: sync_ops_per_label) { sync_ops_per_label_abstract.push_back(std::make_unique<ActionOpAbstract>(*sync_op, *modelZ3)); }
    }
}

/**********************************************************************************************************************/

struct SuccessorGeneratorAbstractExplorer {

    template<bool check_guard_enabled>
    inline static void push_action_op(SuccessorGeneratorAbstract* successor_generator, ActionOpAbstract* action_op) {

        if constexpr (check_guard_enabled) {
            PLAJA_ASSERT(successor_generator->cachedSolver)
            PLAJA_ASSERT(successor_generator->sharedStatistics)

            successor_generator->sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::APP_TESTS);
            PUSH_LAP(successor_generator->sharedStatistics, PLAJA::StatsDouble::TIME_APP_TEST)
            const bool rlt = action_op->guard_enabled(*successor_generator->cachedSolver);
            POP_LAP(successor_generator->sharedStatistics, PLAJA::StatsDouble::TIME_APP_TEST)

            if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) {
                auto* op_app = successor_generator->modelZ3->get_op_applicability();
                if (op_app) {
                    if (not rlt) { op_app->set_applicability(action_op->_op_id(), OpApplicability::Infeasible); }
                    else if (op_app->check_entailed() and action_op->guard_entailed(*successor_generator->cachedSolver)) { op_app->set_applicability(action_op->_op_id(), OpApplicability::Entailed); }
                    else { op_app->set_applicability(action_op->_op_id(), OpApplicability::Feasible); }
                }
            }

            if (not rlt) {
                successor_generator->sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::APP_TESTS_UNSAT);
                return;
            }

        }

        // push action op structure:
        const ActionLabel_type action_label = action_op->_action_label();
        if (action_label == ACTION::silentAction) {
            successor_generator->silentOps.push_back(action_op);
        } else {
            successor_generator->labeledOps[action_label].push_back(action_op);
        }

    }

    template<bool check_guard_enabled>
    inline static void compute_enabled_edges(
        SuccessorGeneratorAbstract* successor_generator, // i.e., the "this" pointer
        const AbstractState& abstract_state) {

        // explore (create non-sync ops and compute enabled edges for sync ops):
        for (AutomatonIndex_type automaton_index = 0; automaton_index < successor_generator->syncInformation->num_automata_instances; ++automaton_index) { // for each automaton
            PLAJA_ASSERT(abstract_state.location_value(automaton_index) >= 0)
            const auto& edges_per_auto_per_loc = successor_generator->edgesPerAutoPerLoc[automaton_index][abstract_state.location_value(automaton_index)];
            // silent edges
            for (const auto& edge_z3: edges_per_auto_per_loc._silentEdges()) { // note: automaton_index is also the location id
                PLAJA_ASSERT(not edge_z3.get_edge()->get_action())
                ActionOpAbstract* action_op = successor_generator->nonSyncOps[successor_generator->modelInformation->compute_action_op_index_non_sync(automaton_index, edge_z3.get_id())].get();
                push_action_op<check_guard_enabled>(successor_generator, action_op);
            }
            // special case: single operator edges (do not check guard individually)
            for (const auto& edge_z3: edges_per_auto_per_loc._singleOpEdges()) {
                successor_generator->enabledEdges[automaton_index][edge_z3.get_action_id_labeled()].push_back(edge_z3.get_id());
            }
            // labeled edges (check guard individually if present)
            for (const auto& edge_z3: edges_per_auto_per_loc._labeledEdges()) { // for each edge starting in the current location
                const ActionID_type action = edge_z3.get_action_id_labeled();
                const EdgeID_type edge_id = edge_z3.get_id();
                if (edge_z3.guard_enabled(*successor_generator->cachedSolver)) { successor_generator->enabledEdges[automaton_index][action].push_back(edge_id); }
            }
        }

    }

    template<bool check_guard_enabled>
    static void compute_enabled_sync_ops( // NOLINT(misc-no-recursion)
        SuccessorGeneratorAbstract* successor_generator, // i.e., the "this" pointer
        SyncIndex_type sync_index, const std::vector<AutomatonSyncAction>& condition, ActionOpIndex_type action_op_index, std::size_t condition_index) {

        if (condition_index == condition.size()) {
            push_action_op<check_guard_enabled>(successor_generator, successor_generator->syncOps[sync_index][action_op_index].get());
        } else {
            const std::list<EdgeID_type>& enabled_edges_loc = successor_generator->enabledEdges[condition[condition_index].automatonIndex][condition[condition_index].actionID];
            for (const EdgeID_type edge_id: enabled_edges_loc) {
                compute_enabled_sync_ops<check_guard_enabled>(successor_generator, sync_index, condition, action_op_index + successor_generator->modelInformation->compute_action_op_index_sync_additive(sync_index, condition[condition_index].automatonIndex, edge_id), condition_index + 1);
            }
        }

    }

    template<bool check_guard_enabled>
    inline static void explore(
        SuccessorGeneratorAbstract* successor_generator, // i.e., the "this" pointer
        const AbstractState& abstract_state) {

        // reset exploration structures
        successor_generator->reset_exploration_structures();

        // compute enabled edges
        compute_enabled_edges<check_guard_enabled>(successor_generator, abstract_state);

        // compute whether a synchronisation is enabled or not
        successor_generator->compute_enabled_synchronisations();

        // compute sync ops:
        for (SyncIndex_type sync_index = 0; sync_index < successor_generator->syncInformation->num_syncs; ++sync_index) {
            if (!successor_generator->enabledSynchronisations[sync_index]) { continue; }
            compute_enabled_sync_ops<check_guard_enabled>(successor_generator, sync_index, successor_generator->syncInformation->synchronisations[sync_index], 0, 0);
        }

    }

    template<bool check_guard_enabled>
    inline static void explore(SuccessorGeneratorAbstract* successor_generator) { // i.e., the "this" pointer

        // reset exploration structures
        successor_generator->reset_exploration_structures();

        // silent ops
        for (const auto& non_sync_op: successor_generator->nonSyncOps) {
            PLAJA_ASSERT(non_sync_op)
            push_action_op<check_guard_enabled>(successor_generator, non_sync_op.get());
        }
        // sync ops
        for (const auto& ops_per_sync: successor_generator->syncOps) {
            for (const auto& sync_op: ops_per_sync) {
                PLAJA_ASSERT(sync_op)
                push_action_op<check_guard_enabled>(successor_generator, sync_op.get());
            }
        }

    }

};

void SuccessorGeneratorAbstract::explore(const AbstractState& abstract_state, Z3_IN_PLAJA::SMTSolver& solver) {
    cachedSolver = &solver;
    //
    SuccessorGeneratorAbstractExplorer::explore<true>(this, abstract_state);
    //
    cachedSolver = nullptr; // de-cache
}

void SuccessorGeneratorAbstract::explore_for_incremental(const AbstractState& abstract_state, Z3_IN_PLAJA::SMTSolver& solver) {
    cachedSolver = &solver;
    //
    SuccessorGeneratorAbstractExplorer::explore<false>(this, abstract_state);
    //
    cachedSolver = nullptr; // de-cache
}

void SuccessorGeneratorAbstract::explore(Z3_IN_PLAJA::SMTSolver& solver) {
    cachedSolver = &solver;
    //
    SuccessorGeneratorAbstractExplorer::explore<true>(this);
    //
    cachedSolver = nullptr; // de-cache
}

void SuccessorGeneratorAbstract::explore() {
    SuccessorGeneratorAbstractExplorer::explore<false>(this);
}

/**********************************************************************************************************************/

const ActionOpAbstract& SuccessorGeneratorAbstract::get_action_op(ActionOpID_type action_op_id) const {
    const ActionOpIDStructure action_op_id_structure = modelInformation->compute_action_op_id_structure(action_op_id);
    if (action_op_id_structure.syncID == SYNCHRONISATION::nonSyncID) {
        return *nonSyncOps[action_op_id_structure.actionOpIndex];
    } else {
        return *syncOps[ModelInformation::sync_id_to_index(action_op_id_structure.syncID)][action_op_id_structure.actionOpIndex];
    }
}

const ActionOp& SuccessorGeneratorAbstract::get_action_op_concrete(ActionOpID_type action_op_id) const {
    return get_action_op(action_op_id)._concrete();
}

ActionLabel_type SuccessorGeneratorAbstract::get_action_label(ActionOpID_type action_op_id) const {
    return get_action_op(action_op_id)._action_label();
}

std::list<const ActionOpAbstract*> SuccessorGeneratorAbstract::extract_ops_per_label(ActionLabel_type action_label) const {
    PLAJA_ASSERT(action_label <= syncInformation->num_actions)

    std::list<const ActionOpAbstract*> ops_per_label;

    if (action_label == ACTION::silentAction) { // SILENT
        // non sync
        for (auto& non_sync_op: nonSyncOps) {
            ops_per_label.push_back(non_sync_op.get());
        }
        // silent synchs
        for (const SyncIndex_type sync_index: syncInformation->synchronisationsPerSilentAction) {
            for (auto& sync_op: syncOps[sync_index]) {
                ops_per_label.push_back(sync_op.get());
            }
        }
    } else { // NON-SILENT
        for (const SyncIndex_type sync_index: syncInformation->synchronisationsPerAction[action_label]) {
            for (auto& sync_op: syncOps[sync_index]) {
                ops_per_label.push_back(sync_op.get());
            }
        }
    }

    return ops_per_label;
}

#if 0
z3::expr SuccessorGeneratorAbstract::extract_action_in_z3(ActionLabel_type action_label) const {
    const auto& enabled_ops = action_label == ACTION::silentLabel ? silentOps : labeledOps[action_label];
    PLAJA_ASSERT(not enabled_ops.empty())
    std::vector<z3::expr> disjunction_of_ops;
    disjunction_of_ops.reserve(enabled_ops.size());
    for (const auto& op: enabled_ops) { disjunction_of_ops.push_back(op->_op_in_z3()); }
    return Z3_IN_PLAJA::to_disjunction(disjunction_of_ops);
}
#endif

std::list<const ActionOp*> SuccessorGeneratorAbstract::get_action_concrete(const class StateBase& state, ActionLabel_type action_label) const { return parent->get_action(state, action_label); }

//

namespace SUCCESSOR_GENERATOR_ABSTRACT {
    [[nodiscard]] std::list<const ActionOpAbstract*> extract_ops_per_label(const SuccessorGeneratorAbstract& successor_generator, ActionLabel_type action_label) { return successor_generator.extract_ops_per_label(action_label); }
}
