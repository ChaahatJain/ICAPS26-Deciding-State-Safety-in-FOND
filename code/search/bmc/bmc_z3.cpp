//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "bmc_z3.h"
#include "../factories/configuration.h"
#include "../smt/model/model_z3.h"
#include "../smt/solver/smt_solver_z3.h"
#include "../smt/solver/statistics_smt_solver_z3.h"
#include "solution_checker_instance_bmc.h"

BMC_Z3::BMC_Z3(const PLAJA::Configuration& config):
    BoundedMC(config)
    , modelZ3(nullptr) {

    PLAJA::Configuration config_loc(config);
    config_loc.set_flag(PLAJA_OPTION::nn_multi_step_support_z3, true);

    modelZ3 = std::make_unique<ModelZ3>(config_loc);

    Z3_IN_PLAJA::add_solver_stats(*searchStatistics);
}

BMC_Z3::~BMC_Z3() = default;

/* */

bool BMC_Z3::check_start_smt() const {
    Z3_IN_PLAJA::SMTSolver solver(modelZ3->share_context(), searchStatistics.get());
    solver.push(); // to suppress scoped_timer thread in z3, which is started when using tactic2solver

    modelZ3->add_bounds(solver, 0, false);
    modelZ3->add_start(solver, true);
    if constexpr (PLAJA_GLOBAL::enableTerminalStateSupport and not PLAJA_GLOBAL::reachMayBeTerminal) { modelZ3->exclude_terminal(solver, 0); }
    modelZ3->add_reach(solver, 0);

    const bool rlt = solver.check();
    PLAJA_ASSERT(not rlt or solver.extract_solution_via_checker(*modelZ3, std::make_unique<SolutionCheckerInstanceBmc>(get_checker(), 0, 0, modelZ3->get_start(), true, modelZ3->get_reach(), false))->check())
    return rlt;
}

void BMC_Z3::prepare_solver(Z3_IN_PLAJA::SMTSolver& solver, unsigned int steps) const {

    modelZ3->generate_steps(steps);

    solver.push(); // to suppress scoped_timer thread in z3, which is started when using tactic2solver

    /* State variable bounds. */
    for (auto it = modelZ3->iterate_steps_from_to(0, steps); !it.end(); ++it) { modelZ3->add_bounds(solver, it.step(), false); }

    /* Start. */
    modelZ3->add_start(solver, true);

    /* NN. */
    if (modelZ3->has_nn()) { for (auto it = modelZ3->iterate_steps_from_to(0, steps - 1); !it.end(); ++it) { modelZ3->add_nn(solver, it.step()); } }

    /* Transition structure. */
    for (auto it = modelZ3->iterate_steps_from_to(0, steps - 1); !it.end(); ++it) { modelZ3->add_choice_point(solver, it.step(), modelZ3->has_nn()); }

    /* Non-terminal. */
    if (PLAJA_GLOBAL::enableTerminalStateSupport and modelZ3->get_terminal()) {
        for (auto it = modelZ3->iterate_steps(); !it.end(); ++it) {
            if (PLAJA_GLOBAL::reachMayBeTerminal and it.step() == steps)  { continue; }
            modelZ3->exclude_terminal(solver, it.step());
        }
    }

    /* Optimizations possible due to incremental checks. */

    if (constrainNoStart) {
        for (auto it = modelZ3->iterate_steps_from_to(1, steps); !it.end(); ++it) { modelZ3->exclude_start(solver, it.step()); }
    }

    if (constrainNoReach) {
        for (auto it = modelZ3->iterate_steps_from_to(0, steps - 1); !it.end(); ++it) { modelZ3->exclude_reach(solver, it.step()); }
    }

}

bool BMC_Z3::check_loop_free_smt(unsigned int steps) const {
    Z3_IN_PLAJA::SMTSolver solver(modelZ3->share_context(), searchStatistics.get());
    prepare_solver(solver, steps);
    modelZ3->add_loop_exclusion(solver, steps);

    const bool rlt = solver.check();
    if (get_save_path_file()) {
        auto instance = solver.extract_solution_via_checker(*modelZ3, std::make_unique<SolutionCheckerInstanceBmc>(get_checker(), steps, modelZ3->has_nn() ? steps : 0, modelZ3->get_start(), true, nullptr, true));
        PLAJA_ASSERT(instance)
        if (not instance->check()) { PLAJA_LOG("Failed to retrieve solution for loop free path.") } // Need to call check to set op sequence.
        instance->dump_readable(*get_save_path_file());
    } else {
        PLAJA_ASSERT(solver.extract_solution_via_checker(*modelZ3, std::make_unique<SolutionCheckerInstanceBmc>(get_checker(), steps, modelZ3->has_nn() ? steps : 0, modelZ3->get_start(), true, nullptr, true))->check())
    };
    return rlt;
}

bool BMC_Z3::check_unsafe_smt(unsigned int steps) const {
    Z3_IN_PLAJA::SMTSolver solver(modelZ3->share_context(), searchStatistics.get());
    prepare_solver(solver, steps);
    if (constrainLoopFree) { modelZ3->add_loop_exclusion(solver, steps); } // Optimizations possible due to incremental checks.
    modelZ3->add_reach(solver, steps);

    const bool rlt = solver.check();
    if (rlt) {
        if (get_save_path_file()) {
            auto instance = solver.extract_solution_via_checker(*modelZ3, std::make_unique<SolutionCheckerInstanceBmc>(get_checker(), steps, modelZ3->has_nn() ? steps : 0, modelZ3->get_start(), true, modelZ3->get_reach(), false));
            PLAJA_ASSERT(instance)
            if (not instance->check()) { PLAJA_LOG("Failed to retrieve solution for unsafe path.") }
            instance->dump_readable(*get_save_path_file());
        } else {
            PLAJA_ASSERT(solver.extract_solution_via_checker(*modelZ3, std::make_unique<SolutionCheckerInstanceBmc>(get_checker(), steps, modelZ3->has_nn() ? steps : 0, modelZ3->get_start(), true, modelZ3->get_reach(), false))->check())
        };
    }
    return rlt;
}