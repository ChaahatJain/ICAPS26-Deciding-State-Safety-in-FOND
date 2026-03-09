//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "fault_analysis_factory.h"
#include "../../../option_parser/option_parser_aux.h"
#include "../../../option_parser/plaja_options.h"
#ifdef BUILD_PA
#include "../predicate_abstraction/pa_factory.h"
#endif
#ifdef USE_CUDD
#include "../../fault_analysis/bdd_computation/reachable_bdd.h"
#include "../../fault_analysis/bdd_computation/unsafe_bdd.h"
#endif
#include "../../fault_analysis/search_based/oracle_uses/include_oracle_uses.h"

// #include "../../fault_analysis/search_based/oracle_uses/fault_finding.h"
// #include "../../fault_analysis/search_based/oracle_uses/shielding.h"
// #include "../../fault_analysis/search_based/oracle_uses/retraining_data_generation.h"

#include "../../fault_analysis/search_based/fault_engine.h"
#include "../../fault_analysis/search_based/oracle.h"
#include "../../information/property_information.h"
#include "../configuration.h"
#include "fault_analysis_options.h"

#include <boost/program_options/option.hpp>

namespace PLAJA_OPTION_DEFAULT {
    const std::string region("unsafe"); // NOLINT(cert-err58-cpp)
    const std::string oracle_use("finding"); // NOLINT(cert-err58-cpp)
}

FaultAnalysisFactory::FaultAnalysisFactory():
    SMTBaseFactory(SearchEngineFactory::FaultAnalysisType) {}

FaultAnalysisFactory::~FaultAnalysisFactory() = default;

void FaultAnalysisFactory::add_options(PLAJA::OptionParser& option_parser) const {
    SMTBaseFactory::add_options(option_parser);
    #if USE_CUDD
    //
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::unsafe_path);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::reachable_path);
    OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::region, PLAJA_OPTION_DEFAULT::region);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::reachable_prop, PLAJA_OPTION_DEFAULT::reachable_prop);
    #endif
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::oracle_use);
    OPTION_PARSER::add_int_option(option_parser, PLAJA_OPTION::num_starts, PLAJA_OPTION_DEFAULT::num_starts);
    OPTION_PARSER::add_int_option(option_parser, PLAJA_OPTION::num_paths_per_start, PLAJA_OPTION_DEFAULT::num_paths_per_start);
    

// RETRAINING EXPERIMENTS
    OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::retraining_train_test_split, PLAJA_OPTION_DEFAULT::retraining_train_test_split);
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::evaluation_set);

    // Use PA CEGAR
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::use_cegar);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::ic3_logs);

    add_pa_cegar_options(option_parser);
    PLAJA_OPTION::add_initial_state_enum(option_parser);
    PLAJA_OPTION::FA_POLICY_ACTION_IS_FAULT::add_options(option_parser);
    PLAJA_OPTION::FA_DATASET_GENERATION::add_options(option_parser);
    PLAJA_OPTION::SAFE_STATE_MANAGER::add_options(option_parser);
    PLAJA_OPTION::FA_ENGINE::add_options(option_parser);
    PLAJA_OPTION::FA_ORACLE::add_options(option_parser);

}
#if USE_CUDD
std::string FaultAnalysisFactory::print_supported_regions() const { return PLAJA::OptionParser::print_supported_options(stringToRegion); }
#endif
std::string FaultAnalysisFactory::print_supported_oracle_uses() const { return PLAJA::OptionParser::print_supported_options(stringToOracleUse); }

#if USE_CUDD
FAULT_ANALYSIS::Region FaultAnalysisFactory::get_region(const PLAJA::Configuration& config) const {
    if (not config.has_option(PLAJA_OPTION::region)) { return FAULT_ANALYSIS::Region::UNSAFE; }
    return config.get_option(stringToRegion, PLAJA_OPTION::region);
}
#endif
FAULT_ANALYSIS::OracleUse FaultAnalysisFactory::get_oracle_use(const PLAJA::Configuration& config) const {
    auto localization = config.get_value_option_string(PLAJA_OPTION::oracle_use);
    return stringToOracleUse.at(localization);
}

void FaultAnalysisFactory::print_options() const {
    OPTION_PARSER::print_options_headline("Fault Analysis");
#if USE_CUDD
    OPTION_PARSER::print_option(PLAJA_OPTION::unsafe_path, OPTION_PARSER::fileStr, "Path to the unsafe region for storing/loading");
    OPTION_PARSER::print_option(PLAJA_OPTION::reachable_path, OPTION_PARSER::fileStr, "Path to the safe reachable region for storing/loading");
    OPTION_PARSER::print_option(PLAJA_OPTION::region, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::region, "Specify computing the reachable or unreachable region", true);
    OPTION_PARSER::print_additional_specification(print_supported_regions(), true);
    OPTION_PARSER::print_bool_option(PLAJA_OPTION::reachable_prop, PLAJA_OPTION_DEFAULT::reachable_prop, "Objective in property file used as additional reachablility constraints.");
#endif
    OPTION_PARSER::print_option(PLAJA_OPTION::oracle_use, OPTION_PARSER::valueStr, "Compute approximate faults given a policy", true);
    OPTION_PARSER::print_additional_specification(print_supported_oracle_uses());
    OPTION_PARSER::print_int_option(PLAJA_OPTION::num_starts, PLAJA_OPTION_DEFAULT::num_starts, "Number of start states to search for faults.");
    OPTION_PARSER::print_int_option(PLAJA_OPTION::num_paths_per_start, PLAJA_OPTION_DEFAULT::num_paths_per_start, "Number of paths to sample per start");
    PLAJA_OPTION::print_initial_state_enum();

    // Verification
    OPTION_PARSER::print_flag(PLAJA_OPTION::use_cegar, "True if we want to generate unsafe path via verification rather than testing.");
    OPTION_PARSER::print_option(PLAJA_OPTION::ic3_logs, OPTION_PARSER::fileStr, "Path to json file with unsafe path");
    
    // RETRAINING EXPERIMENTS
    OPTION_PARSER::print_double_option(PLAJA_OPTION::retraining_train_test_split, PLAJA_OPTION_DEFAULT::retraining_train_test_split, "Fraction of states that should be used when not using evaluation set of start states in retraining work.");
    OPTION_PARSER::print_flag(PLAJA_OPTION::evaluation_set, "Specify whether metrics are being computed using the training set or evaluation set of start states.");
    PLAJA_OPTION::FA_POLICY_ACTION_IS_FAULT::print_options();
    PLAJA_OPTION::FA_DATASET_GENERATION::print_options();
    PLAJA_OPTION::SAFE_STATE_MANAGER::print_options();
    PLAJA_OPTION::FA_ENGINE::print_options();
    PLAJA_OPTION::FA_ORACLE::print_options();
    OPTION_PARSER::print_options_footline();
    print_pa_cegar_options();
}

std::unique_ptr<SearchEngine> FaultAnalysisFactory::construct(const PLAJA::Configuration& config) const { 
    if (config.has_value_option(PLAJA_OPTION::oracle_use)) {
        switch (get_oracle_use(config)) {
            case FAULT_ANALYSIS::OracleUse::FAULT_FINDING: { return std::make_unique<FaultFinding>(config);}
            case FAULT_ANALYSIS::OracleUse::SHIELDING: { return std::make_unique<FaultShielding>(config); }
            case FAULT_ANALYSIS::OracleUse::DATASET_GENERATION: { return std::make_unique<FaultDatasetGeneration>(config);}
            case FAULT_ANALYSIS::OracleUse::POLICY_ACTION_FAULT_ON_STATE: { return std::make_unique<PolicyActionFaultOnState>(config); }
            case FAULT_ANALYSIS::OracleUse::SAFE_STATE_MANAGER: { return std::make_unique<SafeStateManager>(config); }
        }
    } else {
        #if USE_CUDD
            switch (get_region(config)) {
            case FAULT_ANALYSIS::Region::UNSAFE: { return std::make_unique<UnsafeBDD>(config);}
            case FAULT_ANALYSIS::Region::REACHABLE: { return std::make_unique<ReachableBDD>(config); }
        }
        #endif
    }
    
    PLAJA_ABORT;
}

bool FaultAnalysisFactory::supports_property_sub(const PropertyInformation& prop_info) const { return prop_info.get_reach(); }

void FaultAnalysisFactory::add_pa_cegar_options(PLAJA::OptionParser& option_parser) const {
    auto pa_cegar_factory = std::make_unique<PACegarFactory>();
    pa_cegar_factory->add_options(option_parser);
    // pa_cegar_factory->print_options();
}

void FaultAnalysisFactory::print_pa_cegar_options() const {
    auto pa_cegar_factory = std::make_unique<PACegarFactory>();
    pa_cegar_factory->print_options();
}