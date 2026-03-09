//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "smt_solver_z3.h"
#include "../../../exception/smt_exception.h"
#include "../../../include/factory_config_const.h"
#include "../../../option_parser/option_parser_aux.h"
#include "../../../option_parser/option_parser.h"
#include "../../../parser/ast/expression/expression_utils.h"
#include "../../../stats/stats_base.h"
#include "../base/solution_checker_instance.h"
#include "../context_z3.h"
#include "solution_check_wrapper_z3.h"
#include "solution_z3.h"

namespace MARABOU_TO_Z3 { extern std::string print_supported_bound_encodings(); }

namespace PLAJA_OPTION_DEFAULTS { const unsigned int z3_reset_rate(10000); }

namespace PLAJA_OPTION {

    const std::string z3_reset_rate("z3-reset-rate"); // NOLINT(cert-err58-cpp)
    const std::string z3_seed("z3-seed"); // NOLINT(cert-err58-cpp)
    // const std::string fix_constants_bounds("fix_constants_bounds"); // NOLINT(cert-err58-cpp) // Deprecated in favor of "const-vars-to-consts" instead.

    const std::string preprocess_query("preprocess_query"); // NOLINT(cert-err58-cpp)
    const std::string bound_encoding("bound-encoding"); // NOLINT(cert-err58-cpp)
    const std::string mip_encoding("mip-encoding"); // NOLINT(cert-err58-cpp)

    const std::string nn_support_z3("nn-support-z3"); // NOLINT(cert-err58-cpp) // only used internally
    const std::string nn_multi_step_support_z3("nn-multi-step-support-z3"); // NOLINT(cert-err58-cpp)

    namespace SMT_SOLVER_Z3 {

        void add_options(PLAJA::OptionParser& option_parser) {
            OPTION_PARSER::add_int_option(option_parser, PLAJA_OPTION::z3_reset_rate, PLAJA_OPTION_DEFAULTS::z3_reset_rate);
            OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::z3_seed);
            // OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::fix_constants_bounds);

#ifdef USE_MARABOU
            OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::preprocess_query);
            OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::bound_encoding);
            OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::mip_encoding);
#endif

            OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::nn_support_z3);
            OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::nn_multi_step_support_z3);

        }

        extern void print_options() {

            OPTION_PARSER::print_int_option(PLAJA_OPTION::z3_reset_rate, PLAJA_OPTION_DEFAULTS::z3_reset_rate, "Query-rate to reset a z3 solver instance.");
            OPTION_PARSER::print_option(PLAJA_OPTION::z3_seed, OPTION_PARSER::valueStr, "Random seed for z3.");
            // OPTION_PARSER::print_flag(PLAJA_OPTION::fix_constants_bounds, "Flag to indicate that the SMT encoding should fix constant variable's bounds to their initial value.");

#ifdef USE_MARABOU
            OPTION_PARSER::print_flag(PLAJA_OPTION::preprocess_query, "Activate preprocessing towards NN-SAT queries in Z3; by default for " + PLAJA_OPTION::mip_encoding + PLAJA_UTILS::dotString);
            OPTION_PARSER::print_option(PLAJA_OPTION::bound_encoding, OPTION_PARSER::valueStr, "How to encode (preprocessed) bounds in Z3 queries (if present):", true);
            OPTION_PARSER::print_additional_specification(MARABOU_TO_Z3::print_supported_bound_encodings());
            OPTION_PARSER::print_flag(PLAJA_OPTION::mip_encoding, "Activate MIP-encoding (of ReLU constraints) in Z3.");
#endif

        }

    }

}

/**********************************************************************************************************************/

Z3_IN_PLAJA::Z3Assertions::Z3Assertions(z3::context& c):
    assertions(c) {
}

Z3_IN_PLAJA::Z3Assertions::~Z3Assertions() = default;

#ifndef NDEBUG

const z3::expr* Z3_IN_PLAJA::Z3Assertions::retrieve_bound(Z3_IN_PLAJA::VarId_type var) const {
    const auto it = registeredBounds.find(var);
    return it == registeredBounds.cend() ? nullptr : &it->second;
}

#endif

inline void Z3_IN_PLAJA::Z3Assertions::add_to_solver(z3::solver& solver) const {
    solver.add(assertions);
    for (const auto& v: vecAssertions) { solver.add(v); }
    for (const auto& var_bound: registeredBounds) { solver.add(var_bound.second); }
}

/**********************************************************************************************************************/

unsigned int Z3_IN_PLAJA::SMTSolver::resetRate = PLAJA_OPTION_DEFAULTS::z3_reset_rate; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

Z3_IN_PLAJA::SMTSolver::SMTSolver(std::shared_ptr<Z3_IN_PLAJA::Context>&& c, PLAJA::StatsBase* shared_statistics):
    PLAJA::SmtSolver(shared_statistics)
    , context(std::move(c))
    , solver(_context()()) // Candidates: QF_LIA, QF_ALIA; not a candidate "QF_FD": our state variables may be finite-domain, but integer sort is non-finite-domain.
    , pushedAssertions()
    , resetIn(PLAJA_UTILS::cast_numeric<int>(resetRate))
    , blockAutoReset(false) {

    set_statistics(shared_statistics);

    pushedAssertions.emplace_back(get_context_z3());

#if 0
    z3::set_param("auto_config", false);
    z3::set_param("smt.auto_config", false);
    c.set("auto_config", false);
    c.set("smt.auto_config", false);
    z3::params p(c);
    solver.set(p);
    solver.set("auto_config", false);
    solver.set("smt.auto_config", false);
    // solver.set("smt.arith.solver", (unsigned) 6);
    // solver.set("smt.arith.auto_config_simplex", true);
    // solver.set("threads", ((unsigned) 1)); as well as -DZ3_SINGLE_THREADED=True do not prohibit tactic2solver issue
#endif

    static bool parse_options = true;
    if (parse_options) {

        resetRate = PLAJA_GLOBAL::get_int_option(PLAJA_OPTION::z3_reset_rate);
        resetIn = PLAJA_UTILS::cast_numeric<int>(resetRate);

        if (PLAJA_GLOBAL::has_value_option(PLAJA_OPTION::z3_seed)) {
            PLAJA_LOG("Parsing options for Z3 ...")
            const int seed = PLAJA_GLOBAL::get_option_value(PLAJA_OPTION::z3_seed, 1);
            z3::set_param("sat.random_seed", seed);
            z3::set_param("nlsat.seed", seed);
            z3::set_param("fp.spacer.random_seed", seed);
            z3::set_param("smt.random_seed", seed);
            z3::set_param("sls.random_seed", seed);
        }

        parse_options = false;
    }

    PLAJA_LOG_DEBUG_IF(not sharedStatistics, PLAJA_UTILS::to_red_string("Warning: Z3-solver without stats."))
}

Z3_IN_PLAJA::SMTSolver::~SMTSolver() = default;

/* */

std::unique_ptr<Z3_IN_PLAJA::Solution> Z3_IN_PLAJA::SMTSolver::get_solution() const { return std::make_unique<Z3_IN_PLAJA::Solution>(*this); }

std::unique_ptr<SolutionCheckerInstance> Z3_IN_PLAJA::SMTSolver::extract_solution_via_checker(const ModelZ3& model_z3, std::unique_ptr<SolutionCheckerInstance>&& solution_checker_instance) const {
    auto wrapper = std::make_unique<Z3_IN_PLAJA::SolutionCheckWrapper>(std::move(solution_checker_instance), model_z3);
    auto solution = get_solution();
    wrapper->set(*solution);
    return wrapper->release_instance();
}

/* */

bool Z3_IN_PLAJA::SMTSolver::handle_unknown() {

    unknownRlt = true;

    if (sharedStatistics) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::Z3_QUERIES_UNDECIDED); }
    dump_solver_to_file("z3_undecided_z3_query.z3");
    PLAJA_UTILS::write_to_file("z3.log", "Z3 query result: UNKNOWN " + solver.reason_unknown() + PLAJA_UTILS::lineBreakString, false);

    switch (unknownHandling) {
        case UnknownHandling::True: { return true; }
        case UnknownHandling::False: { return false; }
        case UnknownHandling::Error: { throw SMTException(SMTException::Z3, SMTException::Unknown, "z3_undecided_z3_query.z3"); }
        default: { PLAJA_ABORT }
    }

    PLAJA_ABORT
}

/* */

void Z3_IN_PLAJA::SMTSolver::reset() {
    PUSH_LAP_IF(sharedStatistics, PLAJA::StatsDouble::TIME_Z3_RESET)

    solver.reset(); // = z3::solver(solver.ctx());
    PLAJA_ASSERT(not pushedAssertions.empty())
    auto it = pushedAssertions.begin();
    const auto end = pushedAssertions.end();
    it->add_to_solver(solver); // DO NOT PUSH PRIOR TO base assertions
    for (++it; it != end; ++it) {
        solver.push();
        it->add_to_solver(solver);
    }

    resetIn = PLAJA_UTILS::cast_numeric<int>(resetRate);

    POP_LAP_IF(sharedStatistics, PLAJA::StatsDouble::TIME_Z3_RESET)
}

void Z3_IN_PLAJA::SMTSolver::push() {
    PUSH_LAP_IF(sharedStatistics, PLAJA::StatsDouble::TIME_Z3_PUSH)
    solver.push();
    pushedAssertions.emplace_back(get_context_z3());
    POP_LAP_IF(sharedStatistics, PLAJA::StatsDouble::TIME_Z3_PUSH)
}

void Z3_IN_PLAJA::SMTSolver::pop_repeated(unsigned int n) {
    PUSH_LAP_IF(sharedStatistics, PLAJA::StatsDouble::TIME_Z3_POP)
    solver.pop(n);
    for (std::size_t i = 0; i < n; ++i) { pushedAssertions.pop_back(); }
    POP_LAP_IF(sharedStatistics, PLAJA::StatsDouble::TIME_Z3_POP)
}

void Z3_IN_PLAJA::SMTSolver::pop() { pop_repeated(1); }

/* */

#ifndef NDEBUG
const z3::expr* Z3_IN_PLAJA::SMTSolver::retrieve_bound(Z3_IN_PLAJA::VarId_type var) {

    for (const auto& assertion: pushedAssertions) {
        const auto* bound = assertion.retrieve_bound(var);
        if (bound) { return bound; }
    }

    return nullptr;
}
#endif

void Z3_IN_PLAJA::SMTSolver::clear() {
    PUSH_LAP_IF(sharedStatistics, PLAJA::StatsDouble::TIME_Z3_POP)
    pushedAssertions.clear();
    pushedAssertions.emplace_back(get_context_z3());
    POP_LAP_IF(sharedStatistics, PLAJA::StatsDouble::TIME_Z3_POP)
    PLAJA_ASSERT(pushedAssertions.size() == 1)
    reset();
}

/* */

bool Z3_IN_PLAJA::SMTSolver::check() {

    if (--resetIn <= 0 and not blockAutoReset) { reset(); }

    if (sharedStatistics) {
        sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::Z3_QUERIES);
        PLAJA_GLOBAL::timer->push_lap(sharedStatistics, PLAJA::StatsDouble::TIME_Z3);
    }

    auto rlt = solver.check();

    if (sharedStatistics) {
        Timer::Time_type time_for_query; // NOLINT(*-init-variables)
        POP_LAP_AND_CACHE(sharedStatistics, PLAJA::StatsDouble::TIME_Z3, time_for_query)
        if (rlt == z3::unsat) {
            sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::Z3_QUERIES_UNSAT);
            sharedStatistics->inc_attr_double(PLAJA::StatsDouble::TIME_Z3_UNSAT, time_for_query);
        }
    }

    switch (rlt) {
        case z3::sat: { return true; }
        case z3::unsat: { return false; }
        case z3::unknown: { return handle_unknown(); }
    }

    PLAJA_ABORT

}

/**/

[[maybe_unused]] void Z3_IN_PLAJA::SMTSolver::dump_solver() const { std::cout << solver << std::endl; }

[[maybe_unused]] void Z3_IN_PLAJA::SMTSolver::dump_solver_to_file(const std::string& file_name) const {
    std::stringstream solver_stream;
    solver_stream << solver << std::endl << "(check-sat)" << std::endl;
    PLAJA_UTILS::write_to_file(file_name, solver_stream.str(), true);
}
