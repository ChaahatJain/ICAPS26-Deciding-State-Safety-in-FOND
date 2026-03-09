//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_SMT_SOLVER_ENSEMBLE_H
#define PLAJA_SMT_SOLVER_ENSEMBLE_H

#include "../../../../veritas/src/cpp/box.hpp"
#include "../../../../veritas/src/cpp/addtree.hpp"
#include "../../../../veritas/src/cpp/fp_search.hpp"
#include "../../states/state_values.h"
#include <memory>
#include "../../smt/base/forward_solution_check.h"
#include "solution_check_wrapper_veritas.h"
#include <stack>
#include "../../smt/base/smt_solver.h"
#include "../veritas_query.h"
#include "../../smt/base/forward_solution_check.h"
#include "../../factories/forward_factories.h"
#include "../../predicate_abstraction/ensemble/smt/model_veritas_pa.h"
#include "../../information/jani2ensemble/forward_jani_2_ensemble.h"

namespace VERITAS_IN_PLAJA {

    class Solver: public PLAJA::SmtSolver, public QueryConstructable {
        friend class SolverVeritas;
        friend class SolverGurobi;
        friend class SolverZ3;


    private:
        std::unique_ptr<VERITAS_IN_PLAJA::VeritasQuery> query = nullptr;
        std::shared_ptr<ModelVeritasPA> modelVeritas = nullptr;
        const Jani2Ensemble* jani2Ensemble = nullptr;
        std::vector<veritas::FloatT> concreteState;
        std::unique_ptr<SolutionCheckerInstance> solutionCheckerInstance = nullptr;
        std::shared_ptr<veritas::Search> engine = nullptr;
        veritas::AddTree appFilterTrees = veritas::AddTree(0, veritas::AddTreeType::REGR);
        bool use_filter = false;

        bool invariantStrengthening = false;
        veritas::AddTree boxTrees = veritas::AddTree(0, veritas::AddTreeType::REGR);
        bool prune_boxes = false;
    public:
        explicit Solver(const PLAJA::Configuration& config, std::shared_ptr<VERITAS_IN_PLAJA::Context>&& c);
        explicit Solver(const PLAJA::Configuration& config, const VERITAS_IN_PLAJA::VeritasQuery& query);
        ~Solver() override;
        inline std::vector<veritas::FloatT> get_state() { return concreteState; }
        virtual bool check(int action_label) = 0;
        virtual bool check() override { PLAJA_ABORT; return true; }

        void push() override { QueryConstructable::push(); }
        void pop() override { QueryConstructable::pop(); }
        void clear() override { QueryConstructable::clear(); }

        void unset_solution_checker() { solutionCheckerInstance = nullptr; }
        void set_model_veritas(std::shared_ptr<ModelVeritasPA> model) { modelVeritas = model; }
        void set_jani_ensemble(const Jani2Ensemble* jani_2_ensemble) { jani2Ensemble = jani_2_ensemble; }

        [[nodiscard]] inline const SolutionCheckerInstance* get_solution_checker() const { return solutionCheckerInstance.get(); }

        void set_solution_checker(const ModelVeritas& model_veritas, std::unique_ptr<SolutionCheckerInstance> solution_checker_instance);
        void set_app_filter_trees(veritas::AddTree trees) { appFilterTrees = trees; }
        void set_use_filter(bool filter) { use_filter = filter; }
        bool is_filtering_enabled() { return use_filter; }

        void set_invariant_strengthening() { invariantStrengthening = true; }
        void add_box_trees(veritas::AddTree trees) { 
            if (boxTrees.num_leaf_values() == 0) {
                boxTrees = trees;
            } else {
                boxTrees.add_trees(trees);
            }
         }
        void set_prune_boxes(bool prune) { prune_boxes = prune; }
        bool is_pruning_enabled() { return prune_boxes; }

        virtual std::vector<veritas::Interval> extract_box() = 0;
        virtual std::unique_ptr<SolutionCheckerInstance> extract_solution_via_checker() = 0;
        virtual void reset() = 0;

        int get_num_solutions() const { return engine->num_solutions(); }
        virtual std::vector<std::vector<veritas::Interval>> extract_boxes() = 0;

    };
            /* Factory. */
    namespace ENSEMBLE_SOLVER {

        extern std::unique_ptr<VERITAS_IN_PLAJA::Solver> construct(const PLAJA::Configuration& config, std::shared_ptr<VERITAS_IN_PLAJA::Context>&& c);
        extern std::unique_ptr<VERITAS_IN_PLAJA::Solver> construct(const PLAJA::Configuration& config, const VERITAS_IN_PLAJA::VeritasQuery& query);

    }

}

/**********************************************************************************************************************/

namespace PLAJA_OPTION {

    extern const std::string ensemble_mode;

    namespace SMT_SOLVER_VERITAS {
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();
    }

}

#endif //PLAJA_SMT_SOLVER_ENSEMBLE_H
