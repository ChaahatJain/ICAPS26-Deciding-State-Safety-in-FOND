//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "bmc_marabou.h"
#include "../factories/configuration.h"
#include "../smt_nn/model/model_marabou.h"
#include "../smt_nn/solver/smt_solver_marabou.h"
#include "../smt_nn/solver/statistics_smt_solver_marabou.h"
#include "solution_checker_instance_bmc.h"

// TODO: order of add_to_query || incrementality (learned conflicts)?
// Maybe external per-action (operator) enumeration baseline.

namespace BMC_MARABOU {

    const std::string debugInfoLoopFree("Failed to retrieve solution for loop free path.");
    const std::string debugInfoUnsafe("Failed to retrieve solution for unsafe path.");

    /** Auxiliary to reduce code duplication. */
    template<bool loopCheck>
    inline void extract_solution(const std::string* save_path_file, MARABOU_IN_PLAJA::SMTSolver& solver) {

        if (save_path_file or PLAJA_GLOBAL::debug) {
            auto instance = solver.extract_solution_via_checker();
            if (not instance or not instance->check()) { PLAJA_LOG(loopCheck ? debugInfoLoopFree : debugInfoUnsafe) }
            if (not PLAJA_GLOBAL::debug or save_path_file) { if (instance) { instance->dump_readable(*save_path_file); } }
        }

    }

}

/* */

BMCMarabou::BMCMarabou(const PLAJA::Configuration& config):
    BoundedMC(config)
    , solverMarabou(nullptr) {

    // prepare solver:
    MARABOU_IN_PLAJA::add_solver_stats(*searchStatistics);

    PLAJA::Configuration config_loc(config);
    config_loc.set_sharable(PLAJA::SharableKey::STATS, searchStatistics);
    config_loc.set_sharable_ptr(PLAJA::SharableKey::STATS, searchStatistics.get());

    modelMarabou = std::make_unique<ModelMarabou>(config_loc);
    solverMarabou = MARABOU_IN_PLAJA::SMT_SOLVER::construct(config_loc, modelMarabou->make_query(false, false, 0));

}

BMCMarabou::~BMCMarabou() = default;

bool BMCMarabou::check_start_smt() const {

    /* Hack: For Marabou we check path for model's initial state individually, because rewriting the start disjunction may blow up. */
    /* Model init. */
    if (not modelMarabou->initial_state_is_subsumed_by_start()) {
        solverMarabou->push();
        modelMarabou->add_init(*solverMarabou);
        if constexpr (PLAJA_GLOBAL::enableTerminalStateSupport and not PLAJA_GLOBAL::reachMayBeTerminal) { modelMarabou->exclude_terminal(*solverMarabou, 0); }
        modelMarabou->add_reach(*solverMarabou, 0);
        solverMarabou->set_solution_checker(*modelMarabou, std::make_unique<SolutionCheckerInstanceBmc>(get_checker(), 0, 0, nullptr, true, modelMarabou->get_reach(), false));
        const bool rlt_init = solverMarabou->check();
        PLAJA_ASSERT(not rlt_init or solverMarabou->extract_solution_via_checker()->check())
        solverMarabou->pop();
        if (rlt_init) { return true; }
    }

    /* Start. */
    solverMarabou->push();
    modelMarabou->add_start(*solverMarabou);
    if constexpr (PLAJA_GLOBAL::enableTerminalStateSupport and not PLAJA_GLOBAL::reachMayBeTerminal) { modelMarabou->exclude_terminal(*solverMarabou, 0); }
    modelMarabou->add_reach(*solverMarabou, 0);
    solverMarabou->set_solution_checker(*modelMarabou, std::make_unique<SolutionCheckerInstanceBmc>(get_checker(), 0, 0, modelMarabou->get_start(), false, modelMarabou->get_reach(), false));
    const bool rlt = solverMarabou->check();
    PLAJA_ASSERT(not rlt or solverMarabou->extract_solution_via_checker()->check())
    solverMarabou->pop();

    return rlt;

}

void BMCMarabou::prepare_solver(MARABOU_IN_PLAJA::SMTSolver& solver, unsigned int steps) const {

    modelMarabou->generate_steps(steps); // Prepare model.

    solver.set_query(modelMarabou->make_query(false, true, steps - 1));

    /* NN. */
    if (modelMarabou->has_nn()) {
        for (auto it = modelMarabou->iterate_path(); !it.end(); ++it) {
            modelMarabou->add_nn_to_query(solver._query(), it.step());
        }
    }

    /* Transition structure. */
    for (auto it = modelMarabou->iterate_path(); !it.end(); ++it) {
        modelMarabou->add_choice_point(solver, modelMarabou->has_nn(), it.step());
    }

    /* Non-terminal. */
    if (PLAJA_GLOBAL::enableTerminalStateSupport and modelMarabou->get_terminal()) {
        for (auto it = modelMarabou->iterate_steps(); !it.end(); ++it) {
            if (PLAJA_GLOBAL::reachMayBeTerminal and it.step() == steps) { continue; }
            modelMarabou->exclude_terminal(solver, it.step());
        }
    }

    /* Optimizations possible due to incremental checks. */

    if (constrainNoStart) {
        for (auto it = modelMarabou->iterate_steps_from_to(1, steps); !it.end(); ++it) { modelMarabou->exclude_start(solver, steps); }
    }

    if (constrainNoReach) {
        for (auto it = modelMarabou->iterate_steps_from_to(0, steps - 1); !it.end(); ++it) { modelMarabou->exclude_reach(solver, it.step()); }
    }
}

bool BMCMarabou::check_loop_free_smt(unsigned int steps) const {
    prepare_solver(*solverMarabou, steps);

    if (not modelMarabou->initial_state_is_subsumed_by_start()) {
        solverMarabou->push();
        modelMarabou->add_init(*solverMarabou);
        modelMarabou->add_loop_exclusion(*solverMarabou, steps);

        solverMarabou->set_solution_checker(*modelMarabou, std::make_unique<SolutionCheckerInstanceBmc>(get_checker(), steps, modelMarabou->has_nn() ? steps : 0, nullptr, true, nullptr, true));
        const bool rlt_init = solverMarabou->check();
        if (rlt_init) { BMC_MARABOU::extract_solution<true>(get_save_path_file(), *solverMarabou); }

        solverMarabou->pop();
        if (rlt_init) { return true; }
    }

    /* Start. */
    solverMarabou->push();
    modelMarabou->add_start(*solverMarabou);
    modelMarabou->add_loop_exclusion(*solverMarabou, steps);
    solverMarabou->set_solution_checker(*modelMarabou, std::make_unique<SolutionCheckerInstanceBmc>(get_checker(), steps, modelMarabou->has_nn() ? steps : 0, modelMarabou->get_start(), false, nullptr, true));

    const bool rlt = solverMarabou->check();
    if (rlt) { BMC_MARABOU::extract_solution<true>(get_save_path_file(), *solverMarabou); }

    solverMarabou->pop();
    return rlt;
}

bool BMCMarabou::check_unsafe_smt(unsigned int steps) const {
    prepare_solver(*solverMarabou, steps);

    if (not modelMarabou->initial_state_is_subsumed_by_start()) {
        solverMarabou->push();
        modelMarabou->add_init(*solverMarabou);
        modelMarabou->add_reach(*solverMarabou, steps);
        if (constrainLoopFree) { modelMarabou->add_loop_exclusion(*solverMarabou, steps); } // Optimizations possible due to incremental checks.

        solverMarabou->set_solution_checker(*modelMarabou, std::make_unique<SolutionCheckerInstanceBmc>(get_checker(), steps, modelMarabou->has_nn() ? steps : 0, nullptr, true, modelMarabou->get_reach(), false));
        const bool rlt_init = solverMarabou->check();
        if (rlt_init) { BMC_MARABOU::extract_solution<false>(get_save_path_file(), *solverMarabou); }

        solverMarabou->pop();
        if (rlt_init) { return true; }
    }

    /* Start. */
    solverMarabou->push();
    modelMarabou->add_start(*solverMarabou);
    modelMarabou->add_reach(*solverMarabou, steps);
    if (constrainLoopFree) { modelMarabou->add_loop_exclusion(*solverMarabou, steps); } // Optimizations possible due to incremental checks.

    solverMarabou->set_solution_checker(*modelMarabou, std::make_unique<SolutionCheckerInstanceBmc>(get_checker(), steps, modelMarabou->has_nn() ? steps : 0, modelMarabou->get_start(), false, modelMarabou->get_reach(), false));
    // String s("step_" + std::to_string(steps_) + "_new.lp");
    //  GlobalConfiguration::PRINT_BASE_QUERY_IN_GUROBI = &s;
    const bool rlt = solverMarabou->check();
    //  GlobalConfiguration::PRINT_BASE_QUERY_IN_GUROBI = nullptr;
    if (rlt) { BMC_MARABOU::extract_solution<false>(get_save_path_file(), *solverMarabou); }

    solverMarabou->pop();

    return rlt;
}
