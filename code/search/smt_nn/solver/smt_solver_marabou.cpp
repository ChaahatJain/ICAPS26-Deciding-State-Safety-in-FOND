//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <unordered_set>
#include "smt_solver_marabou.h"
#include "../../../exception/smt_exception.h"
#include "../../../include/marabou_include/engine.h"
#include "../../../include/marabou_include/dnc_manager.h"
#include "../../../include/marabou_include/exceptions.h"
#include "../../../include/marabou_include/equation_to_aux_var.h"
#include "../../../include/marabou_include/learned_conflicts.h"
#include "../../../include/marabou_include/milp_solver_bound_tightening_type.h"
#include "../../../option_parser/option_parser.h"
#include "../../../option_parser/option_parser_aux.h"
#include "../../../option_parser/enum_option_values_set.h"
#include "../../../stats/stats_base.h"
#include "../../factories/configuration.h"
#include "../../fd_adaptions/timer.h"
#include "../utils/options_marabou.h"
#include "../utils/preprocessed_bounds.h"
#include "solution_check_wrapper_marabou.h"
#include "solution_marabou.h"

namespace PLAJA_OPTION_DEFAULT {

    const std::string marabou_solver("soi"); // NOLINT(cert-err58-cpp)
    const std::string nn_tightening("none"); // NOLINT(cert-err58-cpp)
    const std::string nn_tightening_per_label("none"); // NOLINT(cert-err58-cpp)

    constexpr bool marabouEnumDis(false);

}

namespace PLAJA_OPTION {

    const std::string marabou_mode("marabou-mode"); // NOLINT(cert-err58-cpp)
    const std::string marabou_solver("marabou-solver"); // NOLINT(cert-err58-cpp)
    const std::string marabou_options("marabou-options"); // NOLINT(cert-err58-cpp)
    const std::string learn_conflicts("learn-conflicts"); // NOLINT(cert-err58-cpp)
    const std::string nn_tightening("nn-tightening"); // NOLINT(cert-err58-cpp)
    const std::string nn_tightening_per_label("nn-tightening-per-label"); // NOLINT(cert-err58-cpp)

    extern const std::string marabouEnumDis;
    extern const std::string perOpAppDis;
    extern const std::string marabouDoSlackForLabelSel;
    extern const std::string marabouOptSlackForLabelSel;
    extern const std::string marabouPreOptDis;

#ifdef MARABOU_DIS_BASELINE_SUPPORT
    const std::string marabouEnumDis("marabou-enum-dis"); // NOLINT(cert-err58-cpp)
    const std::string perOpAppDis("per-op-app-dis"); // NOLINT(cert-err58-cpp)
    const std::string marabouDoSlackForLabelSel("marabou-do-slack-for-label-sel"); // NOLINT(cert-err58-cpp)
    const std::string marabouOptSlackForLabelSel("marabou-opt-slack-for-label-sel"); // NOLINT(cert-err58-cpp)
    const std::string marabouPreOptDis("marabou-pre-opt-dis"); // NOLINT(cert-err58-cpp)
#endif

    namespace SMT_SOLVER_MARABOU {

        FCT_IF_DEBUG(extern std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet> construct_default_configs_enum();)

        std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet> construct_default_configs_enum() {
            return std::make_unique<PLAJA_OPTION::EnumOptionValuesSet>(
                std::list<PLAJA_OPTION::EnumOptionValue> {
#ifdef SUPPORT_RELAXED_NN_SAT
                    PLAJA_OPTION::EnumOptionValue::Relaxed,
#endif
                    PLAJA_OPTION::EnumOptionValue::BranchAndBound,
                    PLAJA_OPTION::EnumOptionValue::Enumerate },
                PLAJA_OPTION::EnumOptionValue::BranchAndBound
            );
        }

        extern const std::unordered_map<std::string, MILPSolverBoundTighteningType> stringToBoundTightening; // extern usage
        const std::unordered_map<std::string, MILPSolverBoundTighteningType> stringToBoundTightening { // NOLINT(cert-err58-cpp)
            { "none", MILPSolverBoundTighteningType::NONE },
#ifdef SUPPORT_GUROBI
            { "lp", MILPSolverBoundTighteningType::LP_RELAXATION },
            { "lp-inc", MILPSolverBoundTighteningType::LP_RELAXATION_INCREMENTAL },
            { "mip", MILPSolverBoundTighteningType::MILP_ENCODING },
            { "mip-inc", MILPSolverBoundTighteningType::MILP_ENCODING_INCREMENTAL },
#endif
        };

        std::string print_supported_bound_tightening() { return PLAJA::OptionParser::print_supported_options(stringToBoundTightening); }

        void add_options(PLAJA::OptionParser& option_parser) {
            OPTION_PARSER::add_enum_option(option_parser, PLAJA_OPTION::marabou_mode, PLAJA_OPTION::SMT_SOLVER_MARABOU::construct_default_configs_enum());
            OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::marabou_solver, PLAJA_OPTION_DEFAULT::marabou_solver);
            OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::marabou_options);
            OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::learn_conflicts);
            OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::nn_tightening, PLAJA_OPTION_DEFAULT::nn_tightening);
            OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::nn_tightening_per_label, PLAJA_OPTION_DEFAULT::nn_tightening_per_label);

            if constexpr (PLAJA_GLOBAL::marabouDisBaselineSupport) {
                OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::marabouEnumDis, PLAJA_OPTION_DEFAULT::marabouEnumDis);
                OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::perOpAppDis, PLAJA_OPTION_DEFAULT::perOpAppDis);
                OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::marabouDoSlackForLabelSel, PLAJA_OPTION_DEFAULT::marabouDoSlackForLabelSel);
                OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::marabouOptSlackForLabelSel, PLAJA_OPTION_DEFAULT::marabouOptSlackForLabelSel);
                OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::marabouPreOptDis, PLAJA_OPTION_DEFAULT::marabouPreOptDis);
            }

        }

        extern void print_options() {
            OPTION_PARSER::print_enum_option(PLAJA_OPTION::marabou_mode, *PLAJA_OPTION::SMT_SOLVER_MARABOU::construct_default_configs_enum(), "Mode to check Marabou queries.");
            OPTION_PARSER::print_option(PLAJA_OPTION::marabou_solver, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::marabou_solver, "Internal solver used by Marabou.", true);
            OPTION_PARSER::print_additional_specification(PLAJA::OptionParser::print_supported_options(OPTIONS_MARABOU::stringToSolverType));
            OPTION_PARSER::print_option(PLAJA_OPTION::marabou_options, OPTION_PARSER::valueStr, "Marabou options (default: \"--verbosity 0\").");
            OPTION_PARSER::print_flag(PLAJA_OPTION::learn_conflicts, "Learn and cache conflicts for piecewise-linear splits in incremental Marabou queries.");

            if constexpr (PLAJA_GLOBAL::supportGurobi) {
                OPTION_PARSER::print_option(PLAJA_OPTION::nn_tightening, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::nn_tightening, "Initial NN tightening:", true);
                OPTION_PARSER::print_additional_specification(PLAJA_OPTION::SMT_SOLVER_MARABOU::print_supported_bound_tightening());
                OPTION_PARSER::print_option(PLAJA_OPTION::nn_tightening_per_label, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::nn_tightening_per_label, "Initial NN tightening per label:", true);
                OPTION_PARSER::print_additional_specification(PLAJA_OPTION::SMT_SOLVER_MARABOU::print_supported_bound_tightening());
            }
        }

    }

}

/**********************************************************************************************************************/

namespace MARABOU_IN_PLAJA {

    struct ExtendedEngine {
        Engine engine;
        FIELD_IF_DEBUG(std::unique_ptr<InputQuery> lastSolution;)
        bool validProof; // So far this only reflects solutions for sat queries.

        explicit ExtendedEngine(const MARABOU_IN_PLAJA::Context& c):
            engine()
            CONSTRUCT_IF_DEBUG(lastSolution(nullptr))
            , validProof(true) {
            if constexpr (PLAJA_GLOBAL::supportGurobi) { engine.set_grb_environment(c.share_grb_env()); }
        }

        ~ExtendedEngine() = default;
        DELETE_CONSTRUCTOR(ExtendedEngine)
    };

    /******************************************************************************************************************/

    /* Placed here to enabled inline. */

    const InputQuery& SMTSolver::get_input_query() const { return _query()._query(); }

    MarabouVarIndex_type SMTSolver::add_aux_var() { return _query().add_aux_var(); }

    MarabouFloating_type SMTSolver::get_lower_bound(MarabouVarIndex_type var) const { return _query().get_lower_bound(var); }

    MarabouFloating_type SMTSolver::get_upper_bound(MarabouVarIndex_type var) const { return _query().get_upper_bound(var); }

    void SMTSolver::tighten_lower_bound(MarabouVarIndex_type var, MarabouFloating_type lb) {
        const auto lb_old = _query().get_lower_bound(var);
        if (FloatUtils::lt(lb_old, lb)) {

            top().add_lb(var, lb_old);
            _query().set_lower_bound(var, lb);

            if (not top().unsat and FloatUtils::lt(_query().get_upper_bound(var), lb)) { top().unsat = true; }

        }
    }

    void SMTSolver::tighten_upper_bound(MarabouVarIndex_type var, MarabouFloating_type ub) {
        const auto ub_old = _query().get_upper_bound(var);
        if (FloatUtils::gt(ub_old, ub)) {

            top().add_ub(var, ub_old);
            _query().set_upper_bound(var, ub);

            if (not top().unsat and FloatUtils::gt(_query().get_lower_bound(var), ub)) { top().unsat = true; }

        }
    }

    std::size_t SMTSolver::get_num_equations() const { return _query().get_num_equations(); }

    void SMTSolver::add_equation(const Equation& eq) { _query().add_equation(eq); }

    void SMTSolver::add_equation(Equation&& eq) { _query().add_equation(std::move(eq)); }

    void SMTSolver::add_equations(std::list<Equation>&& equations) { _query().add_equations(std::move(equations)); }

    std::size_t SMTSolver::get_num_pl_constraints() const { return _query().get_num_pl_constraints(); }

    void SMTSolver::add_pl_constraint(PiecewiseLinearConstraint* pl_constraint) { _query().add_pl_constraint(pl_constraint); }

    void SMTSolver::add_pl_constraint(const PiecewiseLinearConstraint* pl_constraint) { _query().add_pl_constraint(pl_constraint); }

    void SMTSolver::add_disjunction(DisjunctionConstraint* disjunction) {
#ifdef MARABOU_DIS_BASELINE_SUPPORT
        if (enumDisjunctions) {
            disjunction->updateConstraintId(nonePlConstraint);
            disjunctions.emplace_back(disjunction);
            return;
        }
#endif
        _query().add_pl_constraint(disjunction);
    }

    NLR::NetworkLevelReasoner* SMTSolver::provide_nlr() { return _query().provide_nlr(); }

    void SMTSolver::update_nlr() { _query().update_nlr(); }

/***********************************************************************************************************************/

    SMTSolver::StackEntry::StackEntry(const SMTSolver& parent):
        originalNumVars(parent.get_num_query_vars())
        , originalNumEquations(parent.get_num_equations())
        , originalNumPlConstraints(parent.get_num_pl_constraints())
#ifdef MARABOU_DIS_BASELINE_SUPPORT
        , originalNumDisjunctions(parent.disjunctions.size())
#endif
        , unsat(false)
        , learnedConflicts(parent.learnConflicts ? std::make_unique<LearnedConflicts>() : nullptr) {}

    SMTSolver::StackEntry::~StackEntry() = default;

    inline void SMTSolver::StackEntry::add_lb(MarabouVarIndex_type var, MarabouFloating_type val_prev) {
        if (var >= originalNumVars) { return; } // Not interested in the original bound of temporary auxiliary variables.
        auto it = tightenedLowerBounds.find(var);
        if (it == tightenedLowerBounds.end()) { tightenedLowerBounds.emplace(var, val_prev); } else { PLAJA_ASSERT(it->second < val_prev) }
    }

    inline void SMTSolver::StackEntry::add_ub(MarabouVarIndex_type var, MarabouFloating_type val_prev) {
        if (var >= originalNumVars) { return; } // Not interested in the original bound of temporary auxiliary variables.
        auto it = tightenedUpperBounds.find(var);
        if (it == tightenedUpperBounds.end()) { tightenedUpperBounds.emplace(var, val_prev); } else { PLAJA_ASSERT(it->second > val_prev) }
    }

    inline void SMTSolver::StackEntry::add_input(MarabouVarIndex_type var) {
        PLAJA_ASSERT(not std::any_of(additionalInputs.cbegin(), additionalInputs.cend(), [var](MarabouVarIndex_type other) { return var == other; }))
        additionalInputs.push_back(var);
    }

    inline void SMTSolver::StackEntry::add_integer(MarabouVarIndex_type var) {
        PLAJA_ASSERT(not std::any_of(additionalIntegers.cbegin(), additionalIntegers.cend(), [var](MarabouVarIndex_type other) { return var == other; }))
        additionalIntegers.push_back(var);
    }

    inline void SMTSolver::StackEntry::add_output(MarabouVarIndex_type var) {
        PLAJA_ASSERT(not std::any_of(additionalOutputs.cbegin(), additionalOutputs.cend(), [var](MarabouVarIndex_type var_) { return var == var_; }))
        additionalOutputs.push_back(var);
    }

/***********************************************************************************************************************/

    SMTSolver::SMTSolver(const PLAJA::Configuration& config, std::shared_ptr<MARABOU_IN_PLAJA::Context> c):
        PLAJA::SmtSolver(config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS))
        , QueryConstructable(std::move(c))
        , solutionCheckerInstanceWrapper(nullptr)
        , currentQuery(nullptr)
        , lastEngine(nullptr)
#ifdef MARABOU_DIS_BASELINE_SUPPORT
        , enumDisjunctions(config.get_bool_option(PLAJA_OPTION::marabouEnumDis))
#endif
        , learnConflicts(config.is_flag_set(PLAJA_OPTION::learn_conflicts)) {
        set_query(std::make_unique<MARABOU_IN_PLAJA::MarabouQuery>(context));
        OPTIONS_MARABOU::parse_marabou_options();
        PLAJA_LOG_DEBUG_IF(not sharedStatistics, PLAJA_UTILS::to_red_string("Warning: Marabou-solver without stats."))
    }

    SMTSolver::SMTSolver(const PLAJA::Configuration& config, std::unique_ptr<MARABOU_IN_PLAJA::MarabouQuery>&& query):
        PLAJA::SmtSolver(config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS))
        , QueryConstructable(query->share_context())
        , solutionCheckerInstanceWrapper(nullptr)
        , currentQuery(nullptr)
        , lastEngine(nullptr)
#ifdef MARABOU_DIS_BASELINE_SUPPORT
        , enumDisjunctions(config.get_bool_option(PLAJA_OPTION::marabouEnumDis))
#endif
        , learnConflicts(config.is_flag_set(PLAJA_OPTION::learn_conflicts)) {
        set_query(std::move(query));
        OPTIONS_MARABOU::parse_marabou_options();
        PLAJA_LOG_DEBUG_IF(not sharedStatistics, PLAJA_UTILS::to_red_string("Warning: Marabou-solver without stats."))
    }

    SMTSolver::~SMTSolver() = default;

    bool SMTSolver::is_exact() const { return false; }

    /* setter */

    void SMTSolver::set_solution_checker(const ModelMarabou& model_marabou, std::unique_ptr<SolutionCheckerInstance> solution_checker_instance) { solutionCheckerInstanceWrapper = std::make_unique<MARABOU_IN_PLAJA::SolutionCheckWrapper>(std::move(solution_checker_instance), model_marabou); }

    void SMTSolver::unset_solution_checker() { solutionCheckerInstanceWrapper = nullptr; }

    void SMTSolver::set_query(const MARABOU_IN_PLAJA::MarabouQuery& query) { set_query(std::make_unique<MARABOU_IN_PLAJA::MarabouQuery>(query)); }

    void SMTSolver::set_query(std::unique_ptr<MARABOU_IN_PLAJA::MarabouQuery>&& query) {
        PLAJA_ASSERT(_context() == query->_context())

        currentQuery = std::move(query);

        /*
         * Query should not be trivially unsat.
         * This is exploited by pre_check() implementation.
         */
        if constexpr (PLAJA_GLOBAL::debug) {
            for (auto it = currentQuery->variableIterator(); !it.end(); ++it) {
                PLAJA_ASSERT(FloatUtils::lte(it.lower_bound(), it.upper_bound()))
            }
        }

        PUSH_LAP_IF(sharedStatistics, PLAJA::StatsDouble::TIME_MARABOU_POP)
        reset();
        stack.clear();
        POP_LAP_IF(sharedStatistics, PLAJA::StatsDouble::TIME_MARABOU_POP)
        stack.emplace_front(*this);
    }

    void SMTSolver::push() {
        PLAJA_ASSERT(currentQuery)
        PLAJA_ASSERT(not stack.empty())
        PLAJA_ASSERT(not learnConflicts or stack.front()._learned_conflicts())
        PUSH_LAP_IF(sharedStatistics, PLAJA::StatsDouble::TIME_MARABOU_PUSH)
        stack.emplace_front(*this);
        POP_LAP_IF(sharedStatistics, PLAJA::StatsDouble::TIME_MARABOU_PUSH)
    }

    void SMTSolver::pop() {
        PLAJA_ASSERT(not stack.empty())
        PUSH_LAP_IF(sharedStatistics, PLAJA::StatsDouble::TIME_MARABOU_POP)

        // Restore query state.

        auto& stack_frame = top();

        _query().remove_aux_var_to(stack_frame.originalNumVars);
        _query().remove_equation(_query().get_num_equations() - stack_frame.originalNumEquations);
        _query().remove_pl_constraint(_query().get_num_pl_constraints() - stack_frame.originalNumPlConstraints);

#ifdef MARABOU_DIS_BASELINE_SUPPORT
        disjunctions.resize(stack_frame.originalNumDisjunctions);
#endif

        for (auto [var, lb]: stack_frame.tightenedLowerBounds) { _query().set_lower_bound(var, lb); }
        for (auto [var, ub]: stack_frame.tightenedUpperBounds) { _query().set_upper_bound(var, ub); }

        // Unmark.

        for (auto var: stack_frame.additionalInputs) {
            PLAJA_ASSERT(_query().is_marked_input(var))
            _query().unmark_input<false>(var);
        }

        for (auto var: stack_frame.additionalIntegers) {
            PLAJA_ASSERT(_query().is_marked_integer(var))
            _query().unmark_integer(var);
        }

        for (auto var: stack_frame.additionalOutputs) {
            PLAJA_ASSERT(_query().is_marked_output(var))
            _query().unmark_output<false>(var);
        }

        //

        stack.pop_front();
        POP_LAP_IF(sharedStatistics, PLAJA::StatsDouble::TIME_MARABOU_POP)
        PLAJA_ASSERT(not stack.empty()) // base query shall remain
    }

    void SMTSolver::clear() { set_query(std::make_unique<MARABOU_IN_PLAJA::MarabouQuery>(context)); }

    /**/

    void SMTSolver::mark_input(MarabouVarIndex_type var) {
        if (not _query().is_marked_input(var)) {
            top().add_input(var);
            _query().mark_input<false>(var);
        }
    }

    void SMTSolver::mark_integer(MarabouVarIndex_type var) {
        if (not _query().is_marked_integer(var)) {
            top().add_input(var);
            _query().mark_integer(var);
        }
    }

    void SMTSolver::mark_output(MarabouVarIndex_type var) {
        if (not _query().is_marked_output(var)) {
            top().add_output(var);
            _query().mark_output<false>(var);
        }
    }

/**********************************************************************************************************************/

#ifdef MARABOU_DIS_BASELINE_SUPPORT

    bool SMTSolver::enumerate_disjunctions(unsigned int disjunction_index, bool preprocess) { // NOLINT(*-no-recursion)
        PLAJA_ASSERT(disjunction_index <= disjunctions.size())

        if (disjunction_index == disjunctions.size()) {
            return solve(preprocess);
        }

        std::unique_ptr<DisjunctionConstraint> disjunction_optimized(nullptr);
        const DisjunctionConstraint* disjunction_ptr;

        if (GlobalConfiguration::PRE_OPTIMIZE_DISJUNCTIONS) {

            std::vector<PiecewiseLinearCaseSplit> disjuncts_optimized;
            const auto entailment_value = DisjunctionConstraintBase::optimizeForQuery(*static_cast<const DisjunctionConstraint*>(disjunctions[disjunction_index].get()), disjuncts_optimized, _query()._query());

            if (entailment_value == DisjunctionConstraintBase::Entailed) { return enumerate_disjunctions(disjunction_index + 1, preprocess); }
            else if (entailment_value == DisjunctionConstraintBase::Infeasible) { return false; }

            disjunction_optimized = std::make_unique<DisjunctionConstraint>(std::move(disjuncts_optimized), nonePlConstraint);
            disjunction_ptr = disjunction_optimized.get();

        } else {
            disjunction_ptr = disjunctions[disjunction_index].get();
        }

        for (unsigned i = 0; i < disjunction_ptr->getNumDisjuncts(); ++i) {
            const auto& disjunct = disjunction_ptr->getDisjunct(PLAJA_UTILS::cast_numeric<int>(i));

            // if (not DisjunctionConstraintBase::checkBounds(disjunct)) { continue; }

            push();
            /* Add case split. */
            for (const auto& eq: disjunct.getEquations()) { add_equation(eq); }
            for (const auto& tightening: disjunct.getBoundTightenings()) { set_tightening<false>(tightening); }
            /**/
            const auto rlt = enumerate_disjunctions(disjunction_index + 1, preprocess);
            pop();
            if (rlt) { return true; }

        }

        return false;

    }

#endif

#pragma GCC diagnostic push
#ifdef IS_APPLE_CC
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#endif
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

    bool SMTSolver::solve(bool preprocess) {
        lastEngine = std::make_unique<ExtendedEngine>(*context);
        auto& engine = lastEngine->engine;

        Timer::Time_type timer_for_preprocessing; // NOLINT(*-init-variables)
        Timer::Time_type timer_for_query; // NOLINT(*-init-variables)

        // preprocess
        PUSH_LAP_IF(sharedStatistics, PLAJA::StatsDouble::TIME_MARABOU_PREPROCESS)
        const bool rlt = not top().unsat and engine.processInputQuery(std::make_unique<InputQuery>(_query()._query()), preprocess);
        if (sharedStatistics) {
            POP_LAP_AND_CACHE(sharedStatistics, PLAJA::StatsDouble::TIME_MARABOU_PREPROCESS, timer_for_preprocessing)
            sharedStatistics->set_attr_double(PLAJA::StatsDouble::TIME_MARABOU_PREPROCESS_MAX, std::max(timer_for_preprocessing, sharedStatistics->get_attr_double(PLAJA::StatsDouble::TIME_MARABOU_PREPROCESS_MAX)));
        }

        if (rlt) {
            // solve
            if (sharedStatistics) {
                sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_QUERIES);
                PLAJA_GLOBAL::timer->push_lap(sharedStatistics, PLAJA::StatsDouble::TIME_MARABOU);
            }

            auto* learned_conflicts = learnConflicts ? top()._learned_conflicts() : nullptr;
            if (learned_conflicts) { engine.setLearnedConflicts(learned_conflicts); }

#ifndef NDEBUG
            try {
#endif
                engine.solve(OPTIONS_MARABOU::get_timeout());
#ifndef NDEBUG
            } catch (...) {
                _query().save_query("failed.txt");
                PLAJA_ABORT
            }
#endif

            if (sharedStatistics) {

                POP_LAP_AND_CACHE(sharedStatistics, PLAJA::StatsDouble::TIME_MARABOU, timer_for_query)
                sharedStatistics->set_attr_double(PLAJA::StatsDouble::TIME_MARABOU_MAX, std::max(timer_for_query, sharedStatistics->get_attr_double(PLAJA::StatsDouble::TIME_MARABOU_MAX)));

#if 1
                const auto* stats = engine.getStatistics();
                sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_NUM_SPLITS, stats->getUnsignedAttribute(Statistics::StatisticsUnsignedAttribute::NUM_SPLITS));
                sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_NUM_POPS, stats->getUnsignedAttribute(Statistics::StatisticsUnsignedAttribute::NUM_POPS));
                sharedStatistics->set_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_MAX_SPLIT_DEPTH, std::max(sharedStatistics->get_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_MAX_SPLIT_DEPTH), stats->getUnsignedAttribute(Statistics::MAX_DECISION_LEVEL)));
#endif
            }

            if (learned_conflicts) {

                auto old_num_conflicts = learned_conflicts->size();
                learned_conflicts->updateConflicts();

                if (sharedStatistics) {

                    if (not learned_conflicts->learnedUnsat()) {
                        PLAJA_ASSERT(old_num_conflicts < learned_conflicts->size())
                        sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_LEARNED_CONFLICTS, learned_conflicts->size() - old_num_conflicts);
                        PLAJA_ASSERT(engine.getExitCode() != Engine::SAT or (engine.getStatistics()->getUnsignedAttribute(Statistics::StatisticsUnsignedAttribute::NUM_POPS) > 0) == (learned_conflicts->size() - old_num_conflicts > 0))
                    }

                    sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_CASE_SPLITS_SKIPPED, learned_conflicts->getConflictingSplitsSkipped());
                    sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_CASE_SPLITS_FIXED, learned_conflicts->getActiveConstraintsFixed());
                }

                learned_conflicts->resetStats();
            }

        } else {

            if (sharedStatistics) {
                sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_PREPROCESSED_UNSAT);
                sharedStatistics->inc_attr_double(PLAJA::StatsDouble::TIME_MARABOU_FOR_PREPROCESSED_UNSAT, timer_for_preprocessing);
            }

            return false;
        }

        // return result
        switch (engine.getExitCode()) {

            case Engine::UNSAT: {
                if (sharedStatistics) {
                    sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_QUERIES_UNSAT);
                    sharedStatistics->inc_attr_double(PLAJA::StatsDouble::TIME_MARABOU_UNSAT, timer_for_query);
                    sharedStatistics->inc_attr_double(PLAJA::StatsDouble::TIME_MARABOU_PREPROCESS_UNSAT, timer_for_preprocessing);
                }
                return false;
            }

            case Engine::SAT: {
                STMT_IF_DEBUG(
                    auto& last_solution = lastEngine->lastSolution;
                    PLAJA_ASSERT(not last_solution)
                    last_solution = std::make_unique<InputQuery>(_query()._query());
                    engine.extractSolution(*last_solution);
                    if (not last_solution->evaluate(MARABOU_IN_PLAJA::floatingPrecision, false)) {
                        if (sharedStatistics) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_INVALID_SOLUTIONS); }
                        PLAJA_LOG_DEBUG(PLAJA_UTILS::to_red_string("Query evaluates to false in Marabou!"))
//                        last_solution->saveAssignment("false_solution_in_marabou_assignment.txt");
                        _query().save_query("false_solution_in_marabou_query.txt");
                    }
                )
                return true;
            }

            default: {
                if (sharedStatistics) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_QUERIES_UNDECIDED); }
                _query().save_query("undecided_marabou_query.txt");
                return handle_unknown();
            }

        }

        PLAJA_ABORT
    }

#pragma ide diagnostic ignored "UnreachableCode"

    bool SMTSolver::solve_dnc_mode() {
        PLAJA_ASSERT(not OPTIONS_MARABOU::get_dnc_mode()) // Not supported for now.

        if constexpr (not PLAJA_GLOBAL::marabouDncSupport) { PLAJA_ABORT; }

        if (sharedStatistics) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_QUERIES); }

        DnCManager dnc_manager(&_query()._query());
        dnc_manager.solve();

        switch (dnc_manager.getExitCode()) {

            case DnCManager::UNSAT: {
                if (sharedStatistics) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_QUERIES_UNSAT); }
                return false;
            }

            case DnCManager::SAT: {
                // Extract solution, note however, that the bounds are with respect to case splits, i.e., may be "over-tight" with respect to the general problem.
                // std::map<int, double> dummy;
                // dnc_manager.getSolution(dummy, input_query);
                return true;
            }

            default: {
                if (sharedStatistics) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_QUERIES_UNDECIDED); }
                _query().save_query("undecided_marabou_query.txt");
                return handle_unknown();
            }

        }
    }

    bool SMTSolver::handle_unknown() const {

        lastEngine->validProof = false;

        switch (unknownHandling) {
            case UnknownHandling::True: { return true; }
            case UnknownHandling::False: { return false; }
            case UnknownHandling::Error: { throw SMTException(SMTException::MARABOU, SMTException::Unknown, "undecided_marabou_query.txt"); }
            default: { PLAJA_ABORT }
        }

    }

    void SMTSolver::mark_invalid_solution() {
        PLAJA_ASSERT(lastEngine)
        lastEngine->validProof = false;
    }

#pragma GCC diagnostic pop

/**********************************************************************************************************************/

    std::unique_ptr<MARABOU_IN_PLAJA::MarabouQuery> SMTSolver::preprocess(MARABOU_IN_PLAJA::MarabouQuery* query) {
        try {
            Preprocessor preprocessor;
            MARABOU_IN_PLAJA::inform_constraints_of_initial_bounds(query->_query());
            auto preprocessed_query = preprocessor.preprocess(query->_query(), true);
            auto& preprocessed_query_ref = *preprocessed_query;
#ifndef NDEBUG
            for (const auto& pl_constraint: preprocessed_query->getPiecewiseLinearConstraints()) {
                const auto* relu_constraint = PLAJA_UTILS::cast_ptr_if<ReluConstraint>(pl_constraint);
                if (not relu_constraint) { continue; }
                PLAJA_ASSERT(FloatUtils::lt(preprocessed_query->getLowerBound(relu_constraint->getB()), 0) && FloatUtils::gt(preprocessed_query->getUpperBound(relu_constraint->getB()), 0))
            }
#endif

#ifndef NDEBUG
            // compute new integer index
            std::unordered_set<MarabouVarIndex_type> new_integer_vars; // TODO update
            for (auto marabou_var: query->_integer_vars()) {
                PLAJA_ASSERT(query->is_marked_input(marabou_var))
                PLAJA_ASSERT(not preprocessor.variableIsMerged(marabou_var))
                new_integer_vars.insert(preprocessor.getNewIndex(marabou_var));
            }

            PLAJA_ASSERT(new_integer_vars.size() == preprocessed_query_ref.getIntegerVars().size())
            PLAJA_ASSERT(std::all_of(new_integer_vars.cbegin(), new_integer_vars.cend(), [preprocessed_query_ref](MarabouVarIndex_type var) { return preprocessed_query_ref.isInteger(var); }))
#endif

            return std::make_unique<MARABOU_IN_PLAJA::MarabouQuery>(preprocessed_query_ref);

        } catch (const InfeasibleQueryException&) {
            if (sharedStatistics) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_PREPROCESSED_UNSAT); }
            return nullptr;
        }

        PLAJA_ABORT
    }

    std::unique_ptr<PreprocessedBounds> SMTSolver::preprocess_bounds() {
        auto preprocessed_bounds = MARABOU_IN_PLAJA::PreprocessedBounds::preprocess_bounds(_query()._query());
        if (not preprocessed_bounds) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_PREPROCESSED_UNSAT); }
        return preprocessed_bounds;
    }

/**********************************************************************************************************************/

    SMTSolver::SmtResult SMTSolver::pre_check() {
        if (top().unsat) { return SmtResult::UnSat; }
        else if (_query().get_num_equations() + _query().get_num_pl_constraints() == 0) { return SmtResult::Sat; }
        else { return SmtResult::Unknown; }
    }

    bool SMTSolver::check_relaxed() {
        PLAJA_ASSERT(not OPTIONS_MARABOU::get_dnc_mode()) // Not supported for now.
#ifdef MARABOU_DIS_BASELINE_SUPPORT
        return enumDisjunctions ? enumerate_disjunctions(0) : solve();
#else
        return solve();
#endif
    }

    bool SMTSolver::check() { return check_relaxed(); }

/**********************************************************************************************************************/

    bool SMTSolver::is_solved() const {
        PLAJA_ASSERT(not OPTIONS_MARABOU::get_dnc_mode()) // Not supported for now.
        PLAJA_ASSERT(lastEngine)
        switch (lastEngine->engine.getExitCode()) {
            case Engine::SAT:
            case Engine::UNSAT: { return true; }
            default: { return false; }
        }
    }

    bool SMTSolver::get_sat_value() const {
        PLAJA_ASSERT(not OPTIONS_MARABOU::get_dnc_mode()) // Not supported for now.
        PLAJA_ASSERT(lastEngine)
        switch (lastEngine->engine.getExitCode()) {
            case Engine::SAT: { return true; }
            case Engine::UNSAT: { return false; }
            default: { return handle_unknown(); }
        }
    }

    bool SMTSolver::has_solution() const {
        PLAJA_ASSERT(get_sat_value())
        return lastEngine->validProof;
    }

    MARABOU_IN_PLAJA::Solution SMTSolver::extract_solution() {
        PLAJA_ASSERT(has_solution())
        auto& engine = lastEngine->engine;
        MARABOU_IN_PLAJA::Solution solution(_query());
        for (auto var: _query()._query()._variableToInputIndex) { solution.set_value(var.first, engine.extractSolution(var.first)); }
        if (not PLAJA_GLOBAL::debug and sharedStatistics and solution.clipped_out_of_bounds()) { sharedStatistics->inc_attr_unsigned(PLAJA::StatsUnsigned::MARABOU_INVALID_SOLUTIONS); }
        return solution;
    }

    std::unique_ptr<SolutionCheckerInstance> SMTSolver::extract_solution_via_checker() {
        PLAJA_ASSERT(solutionCheckerInstanceWrapper)

        if (solutionCheckerInstanceWrapper->is_invalidated()) {
            solutionCheckerInstanceWrapper = nullptr; // Unset wrapper.
            return nullptr;
        }

        if (not solutionCheckerInstanceWrapper->has_solution()) { // May have been set already.

            PLAJA_ASSERT(has_solution())

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

    std::unique_ptr<MARABOU_IN_PLAJA::PreprocessedBounds> SMTSolver::extract_bounds() {
        PLAJA_ASSERT(not OPTIONS_MARABOU::get_dnc_mode()) // Not supported for now.
        PLAJA_ASSERT(lastEngine)
        return std::make_unique<MARABOU_IN_PLAJA::PreprocessedBounds>(lastEngine->engine.extractBounds(_query()._query()));
    }

    void SMTSolver::reset() { lastEngine = nullptr; }

#ifndef NDEBUG

    void SMTSolver::dump_solution(bool include_output) {

        extract_solution().dump();
        auto& engine = lastEngine->engine;

        if (include_output) {
            std::stringstream ss;
            for (auto var: _query()._query()._variableToOutputIndex) { ss << engine.extractSolution(var.first) << PLAJA_UTILS::spaceString; }
            PLAJA_LOG_DEBUG(ss.str())
        }

    }

    const InputQuery* SMTSolver::get_last_solution() { return lastEngine->lastSolution.get(); }

#endif

}

/** factory ***********************************************************************************************************/

#include "smt_solver_marabou_bb.h"
#include "smt_solver_marabou_enum.h"

namespace MARABOU_IN_PLAJA::SMT_SOLVER {

    std::unique_ptr<MARABOU_IN_PLAJA::SMTSolver> construct(const PLAJA::Configuration& config, std::shared_ptr<MARABOU_IN_PLAJA::Context>&& c) {
        switch (config.get_enum_option(PLAJA_OPTION::marabou_mode).get_value()) {
            case PLAJA_OPTION::EnumOptionValue::Relaxed: { return std::make_unique<MARABOU_IN_PLAJA::SMTSolver>(config, std::move(c)); }
            case PLAJA_OPTION::EnumOptionValue::BranchAndBound: { return std::make_unique<MARABOU_IN_PLAJA::SmtSolverBB>(config, std::move(c)); }
            case PLAJA_OPTION::EnumOptionValue::Enumerate: { return std::make_unique<MARABOU_IN_PLAJA::SmtSolverEnum>(config, std::move(c)); }
            default: { PLAJA_ABORT }
        }
        PLAJA_ABORT
    }

    std::unique_ptr<MARABOU_IN_PLAJA::SMTSolver> construct(const PLAJA::Configuration& config, std::unique_ptr<MARABOU_IN_PLAJA::MarabouQuery>&& query) {
        switch (config.get_enum_option(PLAJA_OPTION::marabou_mode).get_value()) {
            case PLAJA_OPTION::EnumOptionValue::Relaxed: { return std::make_unique<MARABOU_IN_PLAJA::SMTSolver>(config, std::move(query)); }
            case PLAJA_OPTION::EnumOptionValue::BranchAndBound: { return std::make_unique<MARABOU_IN_PLAJA::SmtSolverBB>(config, std::move(query)); }
            case PLAJA_OPTION::EnumOptionValue::Enumerate: { return std::make_unique<MARABOU_IN_PLAJA::SmtSolverEnum>(config, std::move(query)); }
            default: { return std::make_unique<MARABOU_IN_PLAJA::SMTSolver>(config, std::move(query)); } // PLAJA_ABORT
        }
        PLAJA_ABORT
    }

}

