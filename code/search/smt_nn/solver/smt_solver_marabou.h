//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SMT_SOLVER_MARABOU_H
#define PLAJA_SMT_SOLVER_MARABOU_H

#include <forward_list>
#include <memory>
#include <unordered_map>
#include "../../../include/marabou_include/forward_marabou.h"
#include "../../../include/ct_config_const.h"
#include "../../../assertions.h"
#include "../../factories/forward_factories.h"
#include "../../smt/base/forward_solution_check.h"
#include "../../smt/base/smt_solver.h"
#include "../forward_smt_nn.h"
#include "../marabou_query.h"
#include "../using_marabou.h"

namespace MARABOU_IN_PLAJA {

    class SMTSolver: public PLAJA::SmtSolver, public QueryConstructable {

    private:
        struct StackEntry final {
            friend SMTSolver;
        private:
            unsigned int originalNumVars; // To properly handle auxiliary variables.
            unsigned int originalNumEquations;
            unsigned int originalNumPlConstraints;
#ifdef MARABOU_DIS_BASELINE_SUPPORT
            unsigned int originalNumDisjunctions;
#endif

            std::unordered_map<MarabouVarIndex_type, MarabouFloating_type> tightenedLowerBounds; // value before tightening
            std::unordered_map<MarabouVarIndex_type, MarabouFloating_type> tightenedUpperBounds;
            bool unsat;

            std::list<MarabouVarIndex_type> additionalInputs;
            std::list<MarabouVarIndex_type> additionalIntegers;
            std::list<MarabouVarIndex_type> additionalOutputs;

            std::unique_ptr<LearnedConflicts> learnedConflicts;
        public:
            explicit StackEntry(const SMTSolver& parent);
            ~StackEntry();
            DELETE_CONSTRUCTOR(StackEntry)

            void add_lb(MarabouVarIndex_type var, MarabouFloating_type val_prev);

            void add_ub(MarabouVarIndex_type var, MarabouFloating_type val_prev);

            void add_input(MarabouVarIndex_type var);

            void add_integer(MarabouVarIndex_type var);

            void add_output(MarabouVarIndex_type var);

            [[nodiscard]] inline LearnedConflicts* _learned_conflicts() { return learnedConflicts.get(); }
        };

    protected:
        std::unique_ptr<MARABOU_IN_PLAJA::SolutionCheckWrapper> solutionCheckerInstanceWrapper;

    private:
        std::unique_ptr<MARABOU_IN_PLAJA::MarabouQuery> currentQuery;
        std::forward_list<StackEntry> stack;
#ifdef MARABOU_DIS_BASELINE_SUPPORT
        std::vector<std::unique_ptr<DisjunctionConstraint>> disjunctions; // When handling disjunctions externally
#endif

        std::unique_ptr<struct ExtendedEngine> lastEngine;

        /* Option flags. */
#ifdef MARABOU_DIS_BASELINE_SUPPORT
        bool enumDisjunctions;
#endif
        bool learnConflicts;

#ifdef MARABOU_DIS_BASELINE_SUPPORT
        [[nodiscard]] bool enumerate_disjunctions(unsigned int disjunction_index, bool preprocess = true);
#endif

        [[nodiscard]] inline StackEntry& top() {
            PLAJA_ASSERT(not stack.empty())
            return stack.front();
        }

        bool solve(bool preprocess = true);

        /* DNC mode. Currently not supported. Just for documentation. */
        bool solve_dnc_mode();

        [[nodiscard]] bool handle_unknown() const;

    protected:
        void mark_invalid_solution();

    public:
        explicit SMTSolver(const PLAJA::Configuration& config, std::shared_ptr<MARABOU_IN_PLAJA::Context> c);
        explicit SMTSolver(const PLAJA::Configuration& config, std::unique_ptr<MARABOU_IN_PLAJA::MarabouQuery>&& query);
        ~SMTSolver() override;
        DELETE_CONSTRUCTOR(SMTSolver)

        /** Is the solver exact for integer variables? */
        [[nodiscard]] virtual bool is_exact() const;

        [[nodiscard]] inline const MARABOU_IN_PLAJA::SolutionCheckWrapper* get_solution_checker() const { return solutionCheckerInstanceWrapper.get(); }

        void set_solution_checker(const ModelMarabou& model_marabou, std::unique_ptr<SolutionCheckerInstance> solution_checker_instance);

        void unset_solution_checker();

        [[nodiscard]] inline const MARABOU_IN_PLAJA::MarabouQuery& _query() const { return *currentQuery; }

        [[nodiscard]] inline MARABOU_IN_PLAJA::MarabouQuery& _query() { return *currentQuery; }

        void set_query(const MARABOU_IN_PLAJA::MarabouQuery& query);
        void set_query(std::unique_ptr<MARABOU_IN_PLAJA::MarabouQuery>&& query);
        void push() final;
        void pop() final;
        void clear() final;

        /* Incremental construction. */

        [[nodiscard]] inline std::size_t get_num_query_vars() const { return _query()._query().getNumberOfVariables(); }

        [[nodiscard]] const InputQuery& get_input_query() const final;

        [[nodiscard]] MarabouVarIndex_type add_aux_var() final;

        [[nodiscard]] MarabouFloating_type get_lower_bound(MarabouVarIndex_type var) const final;

        [[nodiscard]] MarabouFloating_type get_upper_bound(MarabouVarIndex_type var) const final;

        void tighten_lower_bound(MarabouVarIndex_type var, MarabouFloating_type lb) final;

        void tighten_upper_bound(MarabouVarIndex_type var, MarabouFloating_type ub) final;

        void mark_input(MarabouVarIndex_type var);

        void mark_integer(MarabouVarIndex_type var);

        void mark_output(MarabouVarIndex_type var);

        [[nodiscard]] std::size_t get_num_equations() const final;

        void add_equation(const Equation& eq) final;

        void add_equation(Equation&& eq) final;

        void add_equations(std::list<Equation>&& equations) final;

        [[nodiscard]] std::size_t get_num_pl_constraints() const final;

        void add_pl_constraint(PiecewiseLinearConstraint* pl_constraint) final;

        void add_pl_constraint(const PiecewiseLinearConstraint* pl_constraint) final;

        void add_disjunction(DisjunctionConstraint* disjunction) final;

        [[nodiscard]] NLR::NetworkLevelReasoner* provide_nlr() final;

        void update_nlr() final;

        [[nodiscard]] std::unique_ptr<MARABOU_IN_PLAJA::MarabouQuery> preprocess(MARABOU_IN_PLAJA::MarabouQuery* query = nullptr);
        [[nodiscard]] std::unique_ptr<PreprocessedBounds> preprocess_bounds();

        [[nodiscard]] SmtResult pre_check() override;
        bool check_relaxed();
        bool check() override;

        /** Derived sat or unsat? False if unknown. */
        [[nodiscard]] bool is_solved() const;

        /** Truth value of last query. */
        [[nodiscard]] bool get_sat_value() const;

        /** Has solution? Queries declared sat may not provide solution, e.g., due to unknown handling. */
        [[nodiscard]] bool has_solution() const;

        [[nodiscard]] MARABOU_IN_PLAJA::Solution extract_solution();
        [[nodiscard]] std::unique_ptr<SolutionCheckerInstance> extract_solution_via_checker();
        [[nodiscard]] std::unique_ptr<PreprocessedBounds> extract_bounds();
        void reset();

        FCT_IF_DEBUG(void dump_solution(bool include_output);)
        FCT_IF_DEBUG([[nodiscard]] const InputQuery* get_last_solution();)

    };

    /* Factory. */
    namespace SMT_SOLVER {

        extern std::unique_ptr<MARABOU_IN_PLAJA::SMTSolver> construct(const PLAJA::Configuration& config, std::shared_ptr<MARABOU_IN_PLAJA::Context>&& c);
        extern std::unique_ptr<MARABOU_IN_PLAJA::SMTSolver> construct(const PLAJA::Configuration& config, std::unique_ptr<MARABOU_IN_PLAJA::MarabouQuery>&& query);

    }

}

/**********************************************************************************************************************/

namespace PLAJA_OPTION {

    extern const std::string marabou_mode;
    extern const std::string marabou_solver;
    extern const std::string marabou_options;
    extern const std::string nn_tightening;
    extern const std::string nn_tightening_per_label;

    namespace SMT_SOLVER_MARABOU {
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();
    }

}

#endif //PLAJA_SMT_SOLVER_MARABOU_H
