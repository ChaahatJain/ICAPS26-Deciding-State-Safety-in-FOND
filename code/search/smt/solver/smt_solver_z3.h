//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SMT_SOLVER_Z3_H
#define PLAJA_SMT_SOLVER_Z3_H

#include <list>
#include <memory>
#include <z3++.h>
#include <algorithm>
#include "../../factories/forward_factories.h"
#include "../../fd_adaptions/timer.h"
#include "../base/forward_solution_check.h"
#include "../base/smt_solver.h"
#include "../context_z3.h"
#include "../forward_smt_z3.h"

namespace Z3_IN_PLAJA {

    struct Z3Assertions {

    private:
        /* Added assertions. */
        z3::expr_vector assertions;
        std::list<z3::expr_vector> vecAssertions;

        /* RegisteredBounds */
        std::unordered_map<Z3_IN_PLAJA::VarId_type, z3::expr> registeredBounds;

    public:
        explicit Z3Assertions(z3::context& c);
        ~Z3Assertions();
        DELETE_CONSTRUCTOR(Z3Assertions)

        inline void add(const z3::expr& e) { assertions.push_back(e); }

        inline void add(const z3::expr_vector& v) { vecAssertions.emplace_back(v); }

        [[nodiscard]] inline bool exists_bound(Z3_IN_PLAJA::VarId_type var) const { return registeredBounds.count(var); }

        FCT_IF_DEBUG([[nodiscard]] const z3::expr* retrieve_bound(Z3_IN_PLAJA::VarId_type var) const;)

        inline void register_bound(Z3_IN_PLAJA::VarId_type var, const z3::expr& bound) {
            PLAJA_ASSERT(not exists_bound(var))
            registeredBounds.emplace(var, bound);
        }

        void add_to_solver(z3::solver& solver) const;

    };

    class SMTSolver : public PLAJA::SmtSolver {

    private:
        std::shared_ptr<Z3_IN_PLAJA::Context> context;

        // solver:
        z3::solver solver;
        std::list<Z3Assertions> pushedAssertions; // cache assertions added to solver per push; including 0-push base assertions

        // reset:
        static unsigned int resetRate;
        int resetIn;
        bool blockAutoReset;

        bool handle_unknown();

    public:
        explicit SMTSolver(std::shared_ptr<Z3_IN_PLAJA::Context>&& c, PLAJA::StatsBase* shared_statistics = nullptr);
        ~SMTSolver() override;
        DELETE_CONSTRUCTOR(SMTSolver)

        inline void set_statistics(PLAJA::StatsBase* shared_statistics) { sharedStatistics = shared_statistics; }

        [[nodiscard]] inline PLAJA::StatsBase* share_statistics() { return sharedStatistics; }

        [[nodiscard]] inline Z3_IN_PLAJA::Context& _context() const { return *context; }

        [[nodiscard]] inline z3::context& get_context_z3() const { return solver.ctx(); } // if in doubt use _context()

        [[nodiscard]] virtual inline z3::model get_model() const { return solver.get_model(); } // should only be accessed via solution instance
        [[nodiscard]] virtual std::unique_ptr<Z3_IN_PLAJA::Solution> get_solution() const;
        [[nodiscard]] std::unique_ptr<SolutionCheckerInstance> extract_solution_via_checker(const ModelZ3& model_z3, std::unique_ptr<SolutionCheckerInstance>&& solution_checker_instance) const;

        /* z3::solver interface */

        void reset(); // note: reset on large scale is necessary due to memory allocation within in z3
        inline void block_reset() { blockAutoReset = true; }

        inline void unblock_reset() {
            blockAutoReset = true;
            if (resetIn <= 0) { reset(); }
        };

        [[nodiscard]] inline std::size_t get_stack_size() const { return pushedAssertions.size(); }

        void push();

        void pop();

        void pop_repeated(unsigned int n) final;

        void clear() final;

        inline virtual void add(const z3::expr& e) {
            solver.add(e);
            pushedAssertions.back().add(e);
        }

        inline void add_if(const z3::expr& e) { if (Z3_IN_PLAJA::expr_is(e)) { add(e); } }

        inline virtual void add(const z3::expr_vector& v) {
            solver.add(v);
            pushedAssertions.back().add(v);
        }

        /**
         * Check whether a variable has a registered (and thus added) bound.
         * For each z3 variable only a single bound can be registered, no tightening is possible.
         * Register bounds via @register_bound_if.
         */
        [[nodiscard]] virtual inline bool exists_bound(Z3_IN_PLAJA::VarId_type var) { return std::any_of(pushedAssertions.begin(), pushedAssertions.end(), [var](const Z3Assertions& assertions) { return assertions.exists_bound(var); }); }

        FCT_IF_DEBUG([[nodiscard]] const z3::expr* retrieve_bound(Z3_IN_PLAJA::VarId_type var);)

        /**
         * Register a bound for a variable (if not already present) and add to the solver.
         * For each z3 variable only a single bound can be registered, no tightening is possible.
         */
        virtual inline void register_bound_if(Z3_IN_PLAJA::VarId_type var, const z3::expr& e) {
            if (exists_bound(var)) {
                PLAJA_ASSERT(e.id() == retrieve_bound(var)->id())
                return;
            }
            solver.add(e);
            pushedAssertions.back().register_bound(var, e);
        }

        /* "check" interface */

        // enum SolverType { SMT, NN_SAT }; // so far was only for statistics

        [[nodiscard]] bool check() override;

        inline bool check_push_pop(const z3::expr& e) {
            push();
            add(e);
            const bool rlt = check();
            pop();
            return rlt;
        }

        inline bool check_push_pop(const z3::expr_vector& v) {
            push();
            add(v);
            const bool rlt = check();
            pop();
            return rlt;
        }

        inline bool check_pop() {
            const bool rlt = check();
            pop();
            return rlt;
        }

        [[nodiscard]] inline bool is_unknown() const { return unknownRlt; }

        [[nodiscard]] inline bool retrieve_unknown() {
            const bool rlt = unknownRlt;
            unknownRlt = false;
            return rlt;
        }

        /* */

        [[maybe_unused]] virtual void dump_solver() const;
        [[maybe_unused]] virtual void dump_solver_to_file(const std::string& file_name) const;

    };

}

/**********************************************************************************************************************/

namespace PLAJA_OPTION {
    // extern const std::string fix_constants_bounds;

    extern const std::string preprocess_query;
    extern const std::string bound_encoding;
    extern const std::string mip_encoding;

    // used internally:
    extern const std::string nn_support_z3;
    extern const std::string nn_multi_step_support_z3;

    namespace SMT_SOLVER_Z3 {
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();
    }
}

#endif //PLAJA_SMT_SOLVER_Z3_H
