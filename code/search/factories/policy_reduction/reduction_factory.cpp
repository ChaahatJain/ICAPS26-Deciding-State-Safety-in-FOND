#include "reduction_factory.h"
#include "../../../include/factory_config_const.h"
#include "../../../option_parser/option_parser_aux.h"
#include "../../../option_parser/plaja_options.h"
#include "../../fd_adaptions/search_engine.h"
#include "../../information/property_information.h"
#include "../../non_prob_search/restrictions_checker_explicit.h"
#include "../configuration.h"
#include "../policy_learning/ql_options.h"
#include "../policy_reduction/reduction_options.h"

#ifdef USE_Z3
namespace PLAJA_OPTION::SMT_SOLVER_Z3 {
    extern void add_options(PLAJA::OptionParser& option_parser);
    extern void print_options();
}
#endif

namespace at {
    extern void set_num_threads(int);
    extern void set_num_interop_threads(int);
}

namespace torch {
    using at::set_num_threads;
    using at::set_num_interop_threads;
}

namespace REDUCTION_AGENT { extern std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config); }

/**********************************************************************************************************************/

ReductionFactory::ReductionFactory():
    SearchEngineFactory(SearchEngineType::ReductionAgentType) {}

ReductionFactory::~ReductionFactory() = default;

void ReductionFactory::add_options(PLAJA::OptionParser& option_parser) const {
    SearchEngineFactory::add_options(option_parser);
    //
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::load_nn);
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::save_nnet);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::save_backup);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::save_policy_path);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::postfix);
    OPTION_PARSER::add_int_option(option_parser, PLAJA_OPTION::num_episodes, PLAJA_OPTION_DEFAULT::numEpisodes);
    OPTION_PARSER::add_int_option(option_parser, PLAJA_OPTION::max_length_episode, PLAJA_OPTION_DEFAULT::maxLengthEpisode);
    OPTION_PARSER::add_int_option(option_parser, PLAJA_OPTION::num_start_states, PLAJA_OPTION_DEFAULT::num_start_states);
    OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::frequency, PLAJA_OPTION_DEFAULT::frequency);
    OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::reduction_threshold, PLAJA_OPTION_DEFAULT::reduction_threshold);
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::compact);
    PLAJA_OPTION::add_initial_state_enum(option_parser);

#ifdef USE_Z3
    PLAJA_OPTION::SMT_SOLVER_Z3::add_options(option_parser);
#endif
}

void ReductionFactory::print_options() const {
    OPTION_PARSER::print_options_headline("Policy Learning");
    OPTION_PARSER::print_option(PLAJA_OPTION::load_nn, OPTION_PARSER::fileStr, "Load existing PyTorch model.", true);
    OPTION_PARSER::print_option(PLAJA_OPTION::save_policy_path, OPTION_PARSER::dirStr, "Save trained policy to a specific path.");
    OPTION_PARSER::print_option(PLAJA_OPTION::postfix, OPTION_PARSER::fileStr, "Add postfix to output policy file.");
    OPTION_PARSER::print_additional_specification("If that fails, try to load as NNet model.", true);
    OPTION_PARSER::print_additional_specification("If " + OPTION_PARSER::fileStr + " is invalid, load models specified by the interface (if possible).");
    OPTION_PARSER::print_flag(PLAJA_OPTION::save_nnet, "Save NN also in nnet format.");
    OPTION_PARSER::print_option(PLAJA_OPTION::save_backup, OPTION_PARSER::dirStr, "Save additional backup of learned network to external/customized directory.");
    OPTION_PARSER::print_int_option(PLAJA_OPTION::num_episodes, PLAJA_OPTION_DEFAULT::numEpisodes, "Number of episodes to train.");
    OPTION_PARSER::print_int_option(PLAJA_OPTION::max_length_episode, PLAJA_OPTION_DEFAULT::maxLengthEpisode, "Maximal length of a training episode.");
    OPTION_PARSER::print_int_option(PLAJA_OPTION::num_start_states, PLAJA_OPTION_DEFAULT::num_start_states, "Number of start states, when using start states enumerator.");
    OPTION_PARSER::print_double_option(PLAJA_OPTION::frequency, PLAJA_OPTION_DEFAULT::frequency, "Percentage of how often the neuron must be under the threshold to be reduced.");
    OPTION_PARSER::print_double_option(PLAJA_OPTION::reduction_threshold, PLAJA_OPTION_DEFAULT::reduction_threshold, "Set a threshold for neuron to be considered for reduction.");
    OPTION_PARSER::print_flag(PLAJA_OPTION::compact, "Remove the reduced neuron completely, instead of zeroing it out.");

#ifdef USE_Z3
    PLAJA_OPTION::SMT_SOLVER_Z3::print_options();
#endif

    OPTION_PARSER::print_options_footline();
}

std::unique_ptr<SearchEngine> ReductionFactory::construct(const PLAJA::Configuration& config) const {
    torch::set_num_threads(1);
    torch::set_num_interop_threads(1);
    return REDUCTION_AGENT::construct(config);
}

bool ReductionFactory::supports_property_sub(const PropertyInformation& prop_info) const { return prop_info.get_reach() and prop_info.get_learning_objective(); }

/**********************************************************************************************************************/

namespace SEARCH_ENGINE_FACTORY {

    std::unique_ptr<SearchEngineFactory> construct_reduction_agent_factory() { return std::make_unique<ReductionFactory>(); }

}