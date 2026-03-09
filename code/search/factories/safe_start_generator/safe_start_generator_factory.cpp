//
// Created by Daniel Sherbakov in 2025.
//

#include "safe_start_generator_factory.h"

#include "../../../option_parser/option_parser_aux.h"
#include "../../../option_parser/plaja_options.h"
#include "../../predicate_abstraction/nn/nn_sat_checker/nn_sat_checker_factory.h"
#include "../../safe_start_generator/safe_start_generator.h"
#include "../predicate_abstraction/pa_factory.h"
#include "safe_start_generator_options.h"

SafeStartGeneratorFactory::SafeStartGeneratorFactory():
    SMTBaseFactory(SafeStartGeneratorType) {}

SafeStartGeneratorFactory::~SafeStartGeneratorFactory() = default;

bool SafeStartGeneratorFactory::supports_property_sub(const PropertyInformation& property_information) const {
    return property_information.get_start() and property_information.get_reach();
}

void SafeStartGeneratorFactory::add_options(PLAJA::OptionParser& option_parser) const {
    // SMTBaseFactory::add_options(option_parser); // subsumed by cegar options
    // SSG options
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::verification_method);
    OPTION_PARSER::add_value_option(option_parser,PLAJA_OPTION::approximate);
    OPTION_PARSER::add_value_option(option_parser,PLAJA_OPTION::approximation_type);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::iteration_stats);

    // testing
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::use_testing);
    OPTION_PARSER::add_int_option(option_parser, PLAJA_OPTION::testing_time, PLAJA_OPTION_DEFAULT::testing_time);
    PLAJA_OPTION::add_initial_state_enum(option_parser); // adds state sampler options.
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::terminate_on_cycles);
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::log_path);

    // policy run sampling
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::policy_run_sampling);
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::use_probabilistic_sampling);
    OPTION_PARSER::add_double_option(
        option_parser,
        PLAJA_OPTION::sampling_probability,
        PLAJA_OPTION_DEFAULT::sampling_probability);
    OPTION_PARSER::add_int_option(option_parser, PLAJA_OPTION::max_run_length, PLAJA_OPTION_DEFAULT::max_run_length);

    OPTION_PARSER::add_flag(option_parser,PLAJA_OPTION::alternate);

    //nn checker
    // PLAJA_OPTION::NN_SAT_CHECKER_PA::add_options(option_parser); // subsumed by cegar options

    // used by SCS
    add_pa_cegar_options(option_parser);
}

void SafeStartGeneratorFactory::print_options() const {
    OPTION_PARSER::print_options_headline("Safe Start Generator");
    OPTION_PARSER::print_option(PLAJA_OPTION::verification_method, "<value>", PLAJA_OPTION_INFO::verification_method);
    OPTION_PARSER::print_option(PLAJA_OPTION::approximate, "<value>",PLAJA_OPTION_INFO::approximate);
    OPTION_PARSER::print_option(PLAJA_OPTION::approximation_type, "<value>", PLAJA_OPTION_INFO::approximation_type);
    OPTION_PARSER::print_option(PLAJA_OPTION::iteration_stats, "<path/to/file.csv>",PLAJA_OPTION_INFO::iteration_stats);
    OPTION_PARSER::print_flag(PLAJA_OPTION::use_testing, PLAJA_OPTION_INFO::use_testing);

    OPTION_PARSER::print_options_headline("Testing");
    OPTION_PARSER::print_int_option(PLAJA_OPTION::testing_time, PLAJA_OPTION_DEFAULT::testing_time, PLAJA_OPTION_INFO::testing_time);
    PLAJA_OPTION::print_initial_state_enum();
    OPTION_PARSER::print_flag(PLAJA_OPTION::terminate_on_cycles, PLAJA_OPTION_INFO::terminate_on_cycles);
    OPTION_PARSER::print_flag(PLAJA_OPTION::log_path, PLAJA_OPTION_INFO::log_path);

    OPTION_PARSER::print_options_headline("Policy-run Sampling");
    OPTION_PARSER::print_flag(PLAJA_OPTION::policy_run_sampling, PLAJA_OPTION_INFO::policy_run_sampling);
    OPTION_PARSER::print_flag(PLAJA_OPTION::use_probabilistic_sampling, PLAJA_OPTION_INFO::use_probabilistic_sampling);
    OPTION_PARSER::print_double_option(PLAJA_OPTION::sampling_probability, PLAJA_OPTION_DEFAULT::sampling_probability, PLAJA_OPTION_INFO::sampling_probability);
    OPTION_PARSER::print_int_option(PLAJA_OPTION::max_run_length, -1, PLAJA_OPTION_INFO::max_run_length);

    OPTION_PARSER::print_flag(PLAJA_OPTION::alternate, PLAJA_OPTION_INFO::alternate);

    OPTION_PARSER::print_options_headline("SMT");
    SMTBaseFactory::print_options_base(); // SMT options
    OPTION_PARSER::print_options_footline();
}

void SafeStartGeneratorFactory::add_pa_cegar_options(PLAJA::OptionParser& option_parser) const {
    auto pa_cegar_factory = std::make_unique<PACegarFactory>();
    pa_cegar_factory->add_options(option_parser);
    // pa_cegar_factory->print_options();
}


std::unique_ptr<SearchEngine> SafeStartGeneratorFactory::construct(const PLAJA::Configuration& config) const {
    return std::make_unique<SafeStartGenerator>(config);
}

namespace SEARCH_ENGINE_FACTORY {

    std::unique_ptr<SearchEngineFactory> construct_safe_start_generator_factory() {
        return std::make_unique<SafeStartGeneratorFactory>();
    }

} // namespace SEARCH_ENGINE_FACTORY