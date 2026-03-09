//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "solver_veritas.h"
#include "../../fd_adaptions/timer.h"
#include "solution_veritas.h"
#include "statistics_smt_solver_veritas.h"
#include "../../predicate_abstraction/pa_states/abstract_state.h"
#include "../veritas_query.h"
#include "../../../stats/stats_base.h"
#include "../../factories/configuration.h"

VERITAS_IN_PLAJA::SolverVeritas::SolverVeritas(const PLAJA::Configuration& config, std::shared_ptr<VERITAS_IN_PLAJA::Context>&& c):
        VERITAS_IN_PLAJA::Solver(config, std::move(c)), engine(nullptr) {}

VERITAS_IN_PLAJA::SolverVeritas::SolverVeritas(const PLAJA::Configuration& config, const VERITAS_IN_PLAJA::VeritasQuery& veritas_query):
        VERITAS_IN_PLAJA::Solver(config, veritas_query), engine(nullptr) {}

VERITAS_IN_PLAJA::SolverVeritas::~SolverVeritas() = default;

void eval_example(veritas::AddTree tree, veritas::Box b) {
    /**
     * We get the closest example and evaluate the tree on this example.
    */
    std::vector<veritas::FloatT> example;
    example.reserve(b.size());
    for (auto bound : b) {
        example.push_back(bound.interval.lo);
    }
     std::cout << "Input example is: [";
     for (auto e : example) {
         std::cout << e << ", ";
     }
     std::cout << "]" << std::endl;
    auto num_leaves = tree.num_leaf_values();
    veritas::data d {&example[0], 1, example.size(), example.size(), 1};
    std::vector<double> out(num_leaves, 0.0); // [0,0,0,0,0]
    veritas::data<double> outdata(out);
    tree.eval(d.row(0), outdata);
     std::cout << "evaluate example gives: [";
     for (int i = 0; i < out.size(); ++i) {
         std::cout << outdata[i] << ", ";
     } 
     std::cout << "]" << std::endl;
}


bool VERITAS_IN_PLAJA::SolverVeritas::check_internal(veritas::AddTree veritas_tree, int action_label, veritas::FlatBox prune_box) {
    veritas::Config config = veritas::Config(veritas::HeuristicType::MULTI_MAX_MAX_OUTPUT_DIFF);
    config.ignore_state_when_worse_than = 0.0;
    config.multi_ignore_state_when_class0_worse_than = -15000; // Prune if T0[x] < -50
    config.stop_when_optimal = false;
    config.stop_when_num_solutions_exceeds = invariantStrengthening? 1000000: 1;
    config.stop_when_num_new_solutions_exceeds = invariantStrengthening? 1000000: 1;
    config.max_memory = 7ull * 1024ull * 1024ull * 1024ull; // set mem to 3GB
    veritas_tree.swap_class(action_label);
    engine = config.get_search(veritas_tree, prune_box);
    veritas::StopReason r = veritas::StopReason::NONE;
    PLAJA_GLOBAL::timer->push_lap(sharedStatistics, PLAJA::StatsDouble::TIME_VERITAS);


    for (; r != veritas::StopReason::NUM_SOLUTIONS_EXCEEDED && r!= veritas::StopReason::NO_MORE_OPEN && r!= veritas::StopReason::OUT_OF_MEMORY; r = engine->steps(100)) {
    }
    if (r == veritas::StopReason::OUT_OF_MEMORY) {
        PLAJA_LOG("Out of memory, veritas could not determine if transition SAT/UNSAT");
        PLAJA_ASSERT(false);
    }
    PLAJA_GLOBAL::timer->pop_lap();
    auto timer_for_query = engine->time_since_start();
    // std::cout << "( Sols:" << engine->num_solutions() << ", Time:" << engine->time_since_start() << ")" << veritas_tree << std::endl;
    if (engine->num_solutions() == 0) {
        if (sharedStatistics) {
            sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_QUERIES_UNSAT);
            sharedStatistics->inc_attr_double(PLAJA::StatsDouble::TIME_VERITAS_UNSAT, timer_for_query);
            auto max_unsat = sharedStatistics->get_attr_double(PLAJA::StatsDouble::TIME_VERITAS_UNSAT_MAX);
            if (timer_for_query > max_unsat) {
                sharedStatistics->set_attr_double(PLAJA::StatsDouble::TIME_VERITAS_UNSAT_MAX, timer_for_query);
            }
        }
        return false;
    } 
    if (sharedStatistics) {
        auto max_sat = sharedStatistics->get_attr_double(PLAJA::StatsDouble::TIME_VERITAS_SAT_MAX);
        if (timer_for_query > max_sat) {
            sharedStatistics->set_attr_double(PLAJA::StatsDouble::TIME_VERITAS_SAT_MAX, timer_for_query);
        }
    }

    return (engine->num_solutions() > 0);
}

bool VERITAS_IN_PLAJA::SolverVeritas::check(int action_label) {
    if (sharedStatistics) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::VERITAS_QUERIES); }
    auto veritas_tree = query->get_input_query();

    if constexpr (PLAJA_GLOBAL::enableApplicabilityFiltering) {
        if (VERITAS_IN_PLAJA::Solver::is_filtering_enabled()) {
            auto appTrees = VERITAS_IN_PLAJA::Solver::appFilterTrees;
            veritas_tree.add_trees(appTrees);
        }
    }

    if (VERITAS_IN_PLAJA::Solver::is_pruning_enabled()) {
        auto boxTrees = VERITAS_IN_PLAJA::Solver::boxTrees;
        veritas_tree.add_trees(boxTrees);
    }

    auto eq_trees = this->generateEqTrees(query->get_input_query());
    
    if (eq_trees.num_leaf_values() != 0) {
        // We have trees to falsify constraints. Add them to what we are verifying. 
        // std::cout << "Adding equation trees because they exist" << std::endl;
        // for (auto t: eq_trees) {
        //     std::cout << t << std::endl;
        // }
        // PLAJA_ASSERT(false);
        auto policy_trees = eq_trees.make_multiclass(action_label, veritas_tree.num_leaf_values());
        veritas_tree.add_trees(policy_trees);
    } 
    
    auto prune_box = query->generate_input_list();
    auto sat = check_internal(veritas_tree, action_label, prune_box);
    // count = count + 1;
    return sat;
}


VERITAS_IN_PLAJA::Solution VERITAS_IN_PLAJA::SolverVeritas::extract_solution() {
    PLAJA_ASSERT(engine->num_solutions() > 0)
    auto sol = engine->get_solution(0);
    VERITAS_IN_PLAJA::Solution solution(*VERITAS_IN_PLAJA::Solver::query.get());
    veritas::Box b = sol.box;
    auto updates = this->get_small_updates();
    for (auto bound : b) {
        if (updates.find(bound.feat_id) != updates.end()) {
            UpdatePropagation upd = updates[bound.feat_id];
            veritas::FloatT val = 0;
            for (auto i = 0; i < upd.source_vars.size(); ++i) {
                auto coeff = upd.source_vars[i].first;
                auto src_var = upd.source_vars[i].second;
                auto src_val = b.get_or_insert(src_var).lo;
                val += coeff*src_val;
            }
            auto scalar = upd.scalar;
            val += scalar;
            solution.set_value(bound.feat_id, val);           
        } else {
            solution.set_value(bound.feat_id, bound.interval.lo);
        }
    }
    return solution;
}

std::unique_ptr<SolutionCheckerInstance> VERITAS_IN_PLAJA::SolverVeritas::extract_solution_via_checker() {
    auto solutionCheckerInstanceWrapper = std::make_unique<VERITAS_IN_PLAJA::SolutionCheckWrapper>(std::move(solutionCheckerInstance), *modelVeritas);

    PLAJA_ASSERT(solutionCheckerInstanceWrapper)

    if (solutionCheckerInstanceWrapper->is_invalidated()) {
        solutionCheckerInstanceWrapper = nullptr; // Unset wrapper.
        return nullptr;
    }

    if (not solutionCheckerInstanceWrapper->has_solution()) { // May have been set already.

        PLAJA_ASSERT(engine->num_solutions() > 0)

        const auto solution = extract_solution();

        if (solution.clipped_out_of_bounds()) {
            solutionCheckerInstanceWrapper = nullptr; // Unset wrapper.
            return nullptr;
        }

        solutionCheckerInstanceWrapper->set(solution);
    }

    PLAJA_ASSERT(not solutionCheckerInstanceWrapper->is_invalidated())
    auto solution_checker_instance = solutionCheckerInstanceWrapper->release_instance();
    solutionCheckerInstanceWrapper = nullptr; // Unset wrapper.
    return solution_checker_instance;
}

void VERITAS_IN_PLAJA::SolverVeritas::dump(veritas::AddTree query, int count) {
    std::stringstream s;
    addtree_to_json(s, query);
    std::string path = "../../temp_logs/query_" + std::to_string(count) + ".json";
    std::ofstream file(path);
    if (file.is_open()) {
        // Write the contents of the stringstream to the file
        file << s.str();
        file.close();
        std::cout << "Data has been written to output.json\n";
    } else {
        std::cerr << "Failed to open file for writing\n";
    }
}
