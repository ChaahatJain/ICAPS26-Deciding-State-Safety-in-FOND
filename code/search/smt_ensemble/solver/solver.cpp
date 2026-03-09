//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include <unordered_set>
#include "solver.h"
#include "../../../exception/smt_exception.h"
#include "../../../option_parser/option_parser.h"
#include "../../../option_parser/option_parser_aux.h"
#include "../../../stats/stats_base.h"
#include "../../factories/configuration.h"
#include "../../fd_adaptions/timer.h"
#include "solution_check_wrapper_veritas.h"
#include "solution_veritas.h"

namespace PLAJA_OPTION_DEFAULT {
    const std::string ensemble_mode("veritas"); // NOLINT(cert-err58-cpp)
}

namespace PLAJA_OPTION {

    const std::string ensemble_mode("ensemble-mode"); // NOLINT(cert-err58-cpp)

    namespace SOLVER_ENSEMBLE {

        enum EnsembleMode {
            VERITAS,
            MILP, 
            SMT
       };

        extern const std::unordered_map<std::string, EnsembleMode> stringToEnsembleMode; // extern usage
        const std::unordered_map<std::string, EnsembleMode> stringToEnsembleMode { // NOLINT(cert-err58-cpp)
            { "veritas", EnsembleMode::VERITAS },
            { "milp",    EnsembleMode::MILP },
            { "smt", EnsembleMode::SMT },
        };

        void add_options(PLAJA::OptionParser& option_parser) {
            OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::ensemble_mode, PLAJA_OPTION_DEFAULT::ensemble_mode);
        }

        extern void print_options() {
            OPTION_PARSER::print_option(PLAJA_OPTION::ensemble_mode, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::ensemble_mode, "Mode to check Veritas queries.", true);
            OPTION_PARSER::print_additional_specification(PLAJA::OptionParser::print_supported_options(stringToEnsembleMode));
        }
    }
}

VERITAS_IN_PLAJA::Solver::Solver(const PLAJA::Configuration& config, std::shared_ptr<VERITAS_IN_PLAJA::Context>&& c):
        PLAJA::SmtSolver(config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS))
        , VERITAS_IN_PLAJA::QueryConstructable(std::move(c)),
        query(std::make_unique<VERITAS_IN_PLAJA::VeritasQuery>(this->share_context())){}

VERITAS_IN_PLAJA::Solver::Solver(const PLAJA::Configuration& config, const VERITAS_IN_PLAJA::VeritasQuery& veritas_query):
    PLAJA::SmtSolver(config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS)), VERITAS_IN_PLAJA::QueryConstructable(veritas_query.share_context()),
    query(std::make_unique<VERITAS_IN_PLAJA::VeritasQuery>(veritas_query)) {}

VERITAS_IN_PLAJA::Solver::~Solver() = default;

void VERITAS_IN_PLAJA::Solver::set_solution_checker(const ModelVeritas& model_veritas, std::unique_ptr<SolutionCheckerInstance> solution_checker_instance) { 
    solutionCheckerInstance = std::move(solution_checker_instance);
}


/** factory ***********************************************************************************************************/

#include "solver_veritas.h"
#ifdef SUPPORT_GUROBI
#include "solver_gurobi.h"
#endif
#include "solver_z3.h"

namespace VERITAS_IN_PLAJA::ENSEMBLE_SOLVER {

    std::unique_ptr<VERITAS_IN_PLAJA::Solver> construct(const PLAJA::Configuration& config, std::shared_ptr<VERITAS_IN_PLAJA::Context>&& c) {
        switch (config.get_option(PLAJA_OPTION::SOLVER_ENSEMBLE::stringToEnsembleMode, PLAJA_OPTION::ensemble_mode)) {
            case PLAJA_OPTION::SOLVER_ENSEMBLE::EnsembleMode::VERITAS: { return std::make_unique<VERITAS_IN_PLAJA::SolverVeritas>(config, std::move(c)); }
          #ifdef SUPPORT_GUROBI
            case PLAJA_OPTION::SOLVER_ENSEMBLE::EnsembleMode::MILP: { return std::make_unique<VERITAS_IN_PLAJA::SolverGurobi>(config, std::move(c)); }
	 #endif
            case PLAJA_OPTION::SOLVER_ENSEMBLE::EnsembleMode::SMT: { return std::make_unique<VERITAS_IN_PLAJA::SolverZ3>(config, std::move(c)); }
            default: { PLAJA_ABORT }
        }
        PLAJA_ABORT
    }

    std::unique_ptr<VERITAS_IN_PLAJA::Solver> construct(const PLAJA::Configuration& config, const VERITAS_IN_PLAJA::VeritasQuery& query) {
        switch (config.get_option(PLAJA_OPTION::SOLVER_ENSEMBLE::stringToEnsembleMode, PLAJA_OPTION::ensemble_mode)) {
            case PLAJA_OPTION::SOLVER_ENSEMBLE::EnsembleMode::VERITAS: { return std::make_unique<VERITAS_IN_PLAJA::SolverVeritas>(config, query); }
         #ifdef SUPPORT_GUROBI
            case PLAJA_OPTION::SOLVER_ENSEMBLE::EnsembleMode::MILP: { return std::make_unique<VERITAS_IN_PLAJA::SolverGurobi>(config, query); }
	#endif
            case PLAJA_OPTION::SOLVER_ENSEMBLE::EnsembleMode::SMT: { return std::make_unique<VERITAS_IN_PLAJA::SolverZ3>(config, query); }
            default: { PLAJA_ABORT }
        }
        PLAJA_ABORT
    }

}

