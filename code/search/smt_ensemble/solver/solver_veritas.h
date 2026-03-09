//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//
#ifndef PLAJA_SMT_SOLVER_VERITAS
#define PLAJA_SMT_SOLVER_VERITAS

#include "../../../../veritas/src/cpp/box.hpp"
#include "../../../../veritas/src/cpp/addtree.hpp"
#include "../../../../veritas/src/cpp/fp_search.hpp"
#include "../../states/state_values.h"
#include <memory>
#include "solution_check_wrapper_veritas.h"
#include <stack>
#include "../../smt/base/smt_solver.h"
#include "../../smt/base/forward_solution_check.h"
#include "../veritas_query.h"
#include "solver.h"
#include "../../factories/configuration.h"
#include <fstream>
namespace VERITAS_IN_PLAJA {

class SolverVeritas : public VERITAS_IN_PLAJA::Solver {

private:
    bool check_internal(veritas::AddTree veritas_tree, int action_label, veritas::FlatBox prune_box);
    std::shared_ptr<veritas::Search> engine = nullptr;
    int count = 0;

public:
    explicit SolverVeritas(const PLAJA::Configuration& config, std::shared_ptr<VERITAS_IN_PLAJA::Context>&& c);
    explicit SolverVeritas(const PLAJA::Configuration& config, const VERITAS_IN_PLAJA::VeritasQuery& query);
    ~SolverVeritas() override;
    bool check(int action_label) override;

    void reset() override { engine = nullptr; }

    VERITAS_IN_PLAJA::Solution extract_solution();
    std::unique_ptr<SolutionCheckerInstance> extract_solution_via_checker() override;
    std::vector<veritas::Interval> extract_box() override {
        auto sol = engine->get_solution(0);
        veritas::Box b = sol.box;
        std::vector<veritas::Interval> box;
        box.reserve(b.size());
        for (auto bound: b) {
            box.push_back(bound.interval);
        }
        return box;
    };

    std::vector<std::vector<veritas::Interval>> extract_boxes() override {
        std::vector<std::vector<veritas::Interval>> boxes;
        boxes.reserve(engine->num_solutions());
        for (int i = 0; i < engine->num_solutions(); ++i) {
            auto sol = engine->get_solution(i);
            veritas::Box b = sol.box;
            std::vector<veritas::Interval> box;
            box.reserve(b.size());
            for (auto bound: b) {
                box.push_back(bound.interval);
            }
            boxes.push_back(box);
        }
        return boxes;
    };

    void dump(veritas::AddTree trees, int count = 0);

};

}

#endif //PLAJA_SMT_SOLVER_VERITAS
