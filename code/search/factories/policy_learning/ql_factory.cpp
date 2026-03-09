//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "ql_factory.h"
#include "../../../include/factory_config_const.h"
#include "../../../option_parser/option_parser_aux.h"
#include "../../../option_parser/plaja_options.h"
#include "../../fd_adaptions/search_engine.h"
#include "../../information/property_information.h"
#include "../../non_prob_search/restrictions_checker_explicit.h"
#include "../configuration.h"
#include "ql_options.h"

#ifdef BUILD_FAULT_ANALYSIS
#include "../fault_analysis/fault_analysis_factory.h"
#endif

#ifdef USE_Z3
namespace PLAJA_OPTION::SMT_SOLVER_Z3 {
    extern void add_options(PLAJA::OptionParser& option_parser);
    extern void print_options();
}
#endif

// #include "torch/utils.h"
namespace at {
    extern void set_num_threads(int);
    extern void set_num_interop_threads(int);
}

namespace torch {
    using at::set_num_threads;
    using at::set_num_interop_threads;
}

namespace QL_AGENT { extern std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config); }

/**********************************************************************************************************************/

QlFactory::QlFactory():
    SearchEngineFactory(SearchEngineType::QlAgentType) {}

QlFactory::~QlFactory() = default;

void QlFactory::add_options(PLAJA::OptionParser& option_parser) const {
    SearchEngineFactory::add_options(option_parser);
    //
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::evaluation_mode);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::load_nn);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::load_ensemble);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::save_policy_path);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::postfix);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::save_backup);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::save_agent_actions);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::reward_nn);
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::reward_nn_layers);
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::save_nnet);
    OPTION_PARSER::add_int_option(option_parser, PLAJA_OPTION::num_episodes, PLAJA_OPTION_DEFAULT::numEpisodes);
    OPTION_PARSER::add_int_option(option_parser, PLAJA_OPTION::max_start_states, PLAJA_OPTION_DEFAULT::maxStartStates);
    OPTION_PARSER::add_int_option(option_parser, PLAJA_OPTION::max_length_episode, PLAJA_OPTION_DEFAULT::maxLengthEpisode);
    OPTION_PARSER::add_flag(option_parser, PLAJA_OPTION::terminate_cycles);
    OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::epsilon_start, PLAJA_OPTION_DEFAULT::epsilonStart);
    OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::epsilon_decay, PLAJA_OPTION_DEFAULT::epsilonDecay);
    OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::epsilon_end, PLAJA_OPTION_DEFAULT::epsilonEnd);
    OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::l1_lambda, PLAJA_OPTION_DEFAULT::l1_lambda);
    OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::l2_lambda, PLAJA_OPTION_DEFAULT::l2_lambda);
    OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::group_lambda, PLAJA_OPTION_DEFAULT::group_lambda);
    OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::kl_lambda, PLAJA_OPTION_DEFAULT::kl_lambda);
    OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::reward_lambda, PLAJA_OPTION_DEFAULT::reward_lambda);
    OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::goal_reward, PLAJA_OPTION_DEFAULT::goalReward);
    OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::avoid_reward, PLAJA_OPTION_DEFAULT::avoidReward);
    OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::stalling_reward, PLAJA_OPTION_DEFAULT::stallingReward);
    OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::terminal_reward, PLAJA_OPTION_DEFAULT::terminalReward);
    #ifdef BUILD_FAULT_ANALYSIS
    OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::unsafe_reward, PLAJA_OPTION_DEFAULT::unsafeReward);
    #endif
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::groundStartPotential, PLAJA_OPTION_DEFAULT::groundStartPotential);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::groundDonePotential, PLAJA_OPTION_DEFAULT::groundDonePotential);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::grad_clip, PLAJA_OPTION_DEFAULT::grad_clip);
    OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::minimal_score, PLAJA_OPTION_DEFAULT::minimalScore);
    OPTION_PARSER::add_int_option(option_parser, PLAJA_OPTION::slicingSize, PLAJA_OPTION_DEFAULT::slicingSize);

    /* Policy teacher. */
    OPTION_PARSER::add_value_option(option_parser, PLAJA_OPTION::teacherModel);
    OPTION_PARSER::add_double_option(option_parser, PLAJA_OPTION::teacherGreediness, PLAJA_OPTION_DEFAULT::teacherGreediness);
    OPTION_PARSER::add_bool_option(option_parser, PLAJA_OPTION::teacherExplore, PLAJA_OPTION_DEFAULT::teacherExplore);
    PLAJA_OPTION::add_initial_state_enum(option_parser);


#ifdef USE_Z3
    PLAJA_OPTION::SMT_SOLVER_Z3::add_options(option_parser);
#endif

#ifdef BUILD_FAULT_ANALYSIS
    add_fa_options(option_parser);
#endif
}

void QlFactory::print_options() const {
    OPTION_PARSER::print_options_headline("Policy Learning");

    OPTION_PARSER::print_flag(PLAJA_OPTION::evaluation_mode, "Perform evaluation for existing NN (without updating/learning).");

    OPTION_PARSER::print_option(PLAJA_OPTION::load_nn, OPTION_PARSER::fileStr, "Load existing PyTorch model.", true);
    OPTION_PARSER::print_additional_specification("If that fails, try to load as NNet model.", true);
    OPTION_PARSER::print_additional_specification("If " + OPTION_PARSER::fileStr + " is invalid, load models specified by the interface (if possible).");

    OPTION_PARSER::print_option(PLAJA_OPTION::load_ensemble, OPTION_PARSER::fileStr, "Load existing Veritas Ensemble in JSON.");
    OPTION_PARSER::print_option(PLAJA_OPTION::save_backup, OPTION_PARSER::dirStr, "Save additional backup of learned network to external/customized directory.");
    OPTION_PARSER::print_option(PLAJA_OPTION::save_agent_actions, OPTION_PARSER::fileStr, "Save each visited state with action label selected by the agent.");

    OPTION_PARSER::print_flag(PLAJA_OPTION::save_nnet, "Save NN also in nnet format.");
    OPTION_PARSER::print_option(PLAJA_OPTION::reward_nn, OPTION_PARSER::fileStr, "Use a separate neural network to produce reward values for states during training.");
    OPTION_PARSER::print_option(PLAJA_OPTION::reward_nn_layers, OPTION_PARSER::fileStr, "Layer info for reward nn in string form.");
    OPTION_PARSER::print_option(PLAJA_OPTION::save_policy_path, OPTION_PARSER::dirStr, "Save trained policy to a specific path.");
    OPTION_PARSER::print_option(PLAJA_OPTION::postfix, OPTION_PARSER::fileStr, "Add postfix to output policy file.");

    OPTION_PARSER::print_int_option(PLAJA_OPTION::num_episodes, PLAJA_OPTION_DEFAULT::numEpisodes, "Number of episodes to train.");
    OPTION_PARSER::print_int_option(PLAJA_OPTION::max_start_states, PLAJA_OPTION_DEFAULT::maxStartStates, "Maximal amount of starts states, when using start state enumerator");
    OPTION_PARSER::print_int_option(PLAJA_OPTION::max_length_episode, PLAJA_OPTION_DEFAULT::maxLengthEpisode, "Maximal length of a training episode.");

    OPTION_PARSER::print_flag(PLAJA_OPTION::terminate_cycles, "Terminate cycle episodes.");

    const std::string hyper_parameters = PLAJA_OPTION::epsilon_start + "|" + PLAJA_OPTION::epsilon_decay + "|" + PLAJA_OPTION::epsilon_end;
    OPTION_PARSER::print_option(hyper_parameters, OPTION_PARSER::valueStr, "Hyperparameters of epsilon-greed.");
    OPTION_PARSER::print_double_option(PLAJA_OPTION::l1_lambda, PLAJA_OPTION_DEFAULT::l1_lambda, "Lambda for L1 regularization, default is 0 meaning no regularization.");
    OPTION_PARSER::print_double_option(PLAJA_OPTION::l2_lambda, PLAJA_OPTION_DEFAULT::l2_lambda, "Lambda for L2 regularization, default is 0 meaning no regularization.");
    OPTION_PARSER::print_double_option(PLAJA_OPTION::group_lambda, PLAJA_OPTION_DEFAULT::group_lambda, "Lambda for group lasso regularization, default is 0 meaning no regularization.");
    OPTION_PARSER::print_double_option(PLAJA_OPTION::kl_lambda, PLAJA_OPTION_DEFAULT::kl_lambda, "Lambda for KL divergence regularization, default is 0 meaning no regularization.");
    OPTION_PARSER::print_double_option(PLAJA_OPTION::reward_lambda, PLAJA_OPTION_DEFAULT::reward_lambda, "Lambda for reward assigned by neural network.");

    std::string reward_values = PLAJA_OPTION::goal_reward + "|" + PLAJA_OPTION::avoid_reward + "|" + PLAJA_OPTION::stalling_reward + "|" + PLAJA_OPTION::terminal_reward;
    #ifdef BUILD_FAULT_ANALYSIS
     reward_values = reward_values + "|" + PLAJA_OPTION::unsafe_reward;
    #endif
    OPTION_PARSER::print_option(reward_values, OPTION_PARSER::valueStr, "Reward values.");

    const std::string ground_potential_values = PLAJA_OPTION::groundStartPotential + "|" + PLAJA_OPTION::groundDonePotential;
    OPTION_PARSER::print_option(ground_potential_values, OPTION_PARSER::valueStr, "Ground reward-shaping potential of respective states");

    OPTION_PARSER::print_double_option(PLAJA_OPTION::minimal_score, PLAJA_OPTION_DEFAULT::minimalScore, "Minimal learning score for saving.");

    OPTION_PARSER::print_int_option(PLAJA_OPTION::slicingSize, PLAJA_OPTION_DEFAULT::slicingSize, "Size of the sliding window to compute average.");

    OPTION_PARSER::print_option(PLAJA_OPTION::teacherModel, OPTION_PARSER::fileStr, "Load model used by a teacher guiding the trained policy.");

    OPTION_PARSER::print_double_option(PLAJA_OPTION::teacherGreediness, PLAJA_OPTION_DEFAULT::teacherGreediness, "Fraction of *non*-epsilon-greediness used for policy.");
    OPTION_PARSER::print_bool_option(PLAJA_OPTION::grad_clip, PLAJA_OPTION_DEFAULT::grad_clip, "Enable or disable gradient clipping, is disabled by default.");
    OPTION_PARSER::print_bool_option(PLAJA_OPTION::teacherExplore, PLAJA_OPTION_DEFAULT::teacherExplore, "Teacher explores state space of teacher model to compute goal path.");

#ifdef USE_Z3
    PLAJA_OPTION::SMT_SOLVER_Z3::print_options();
#endif

    OPTION_PARSER::print_options_footline();
#ifdef BUILD_FAULT_ANALYSIS
    print_fa_options();
#endif
}

std::unique_ptr<SearchEngine> QlFactory::construct(const PLAJA::Configuration& config) const {
    torch::set_num_threads(1);
    torch::set_num_interop_threads(1);
    return QL_AGENT::construct(config);
}

bool QlFactory::supports_property_sub(const PropertyInformation& prop_info) const { return prop_info.get_reach() and prop_info.get_learning_objective(); }

#ifdef BUILD_FAULT_ANALYSIS
void QlFactory::add_fa_options(PLAJA::OptionParser& option_parser) const {
    auto fa_factory = std::make_unique<FaultAnalysisFactory>();
    fa_factory->add_options(option_parser);
}

void QlFactory::print_fa_options() const {
    auto fa_factory = std::make_unique<FaultAnalysisFactory>();
    fa_factory->print_options();
}
#endif

/**********************************************************************************************************************/

namespace SEARCH_ENGINE_FACTORY {

    std::unique_ptr<SearchEngineFactory> construct_ql_agent_factory() { return std::make_unique<QlFactory>(); }

}