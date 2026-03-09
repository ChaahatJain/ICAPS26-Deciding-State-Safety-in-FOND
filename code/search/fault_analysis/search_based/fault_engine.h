//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

// Note: This is an engine construction file.

#ifndef PLAJA_FAULT_ENGINE_H
#define PLAJA_FAULT_ENGINE_H

#include <vector>
#include <unordered_set>
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../fd_adaptions/search_engine.h"
#include "../../information/jani_2_interface.h"
#include "../../states/forward_states.h"
#include "../../successor_generation/forward_successor_generation.h"
#include "../../using_search.h"
#include "../../non_prob_search/policy/policy_restriction.h"
#include "../../non_prob_search/initial_states_enumerator.h"
#include <stack>
#include "oracle.h"
#include "search_statistics_fault.h"
#include "../../../search/smt/model/model_z3.h"
#include "../../../search/smt/bias_functions/distance_function.h"
#include "../../information/property_information.h"

#include "policy_run_sampler.h"

class FaultEngine: public SearchEngine {
public: 
    enum PolicyEvaluationMetric {
        PI_ENVELOPE,
        ALL_PI_PATHS,
        SAMPLE_PI_PATHS,
    };
protected:
    /* Variable Hyperparameters. */
    unsigned int numEpisodes; 
    unsigned int episodesCounter;
    bool evaluation_mode = false;
    double train_test_split = 1;

    /** **/
    /* fault search task specification. */
    std::unique_ptr<class InitialStatesEnumerator> startEnumerator;
    std::vector<StateID_type> startStates;
    const Jani2Interface* jani2Interface;
    const ObjectiveExpression* objective;
    const Expression* start;
    const Expression* avoid;

    /* policy sampling */
    std::shared_ptr<const ModelZ3> z3model;
    std::shared_ptr<Bias::DistanceFunction> distance_to_avoid;

    /* Heart of the fault search agent. */
    std::shared_ptr<SimulationEnvironment> simulatorEnv; //
    std::shared_ptr<PolicyRestriction> policy;
    std::unique_ptr<PolicyRunSampler> policy_run_sampler;
    std::unique_ptr<Oracle> oracle;

    /* Support for running experiments on a fixed start state */
    int specified_start_state_index = -1;
    /* Verification engine info*/
    PolicyEvaluationMetric evaluation_metric; 


    // To reach first choice point starting from start state.
    std::unique_ptr<State> simulate_until_choice_point(const State& state) const;
    
    SearchStatus initialize() override;
    SearchStatus step() override = 0;

    // Statistics:
    void print_statistics() const override;
    void stats_to_csv(std::ofstream& file) const override;
    void stat_names_to_csv(std::ofstream& file) const override;

    std::shared_ptr<const ModelZ3> get_or_create_z3model(const PLAJA::Configuration& config); // For distance function specification
public:
    explicit FaultEngine(const PLAJA::Configuration& config);
    ~FaultEngine() override;
    DELETE_CONSTRUCTOR(FaultEngine)

};

namespace PLAJA_OPTION {

    extern const std::string evaluation_method;
    extern const std::string fixed_start_state_index;

    namespace FA_ENGINE {
        const std::unordered_map<std::string, FaultEngine::PolicyEvaluationMetric> stringToFailMetric {
            {"policy_envelope", FaultEngine::PolicyEvaluationMetric::PI_ENVELOPE},
            {"all_policy_paths", FaultEngine::PolicyEvaluationMetric::ALL_PI_PATHS},
            {"sample_policy_paths", FaultEngine::PolicyEvaluationMetric::SAMPLE_PI_PATHS},
        };
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();
    } // namespace FA_ENGINE

}// namespace PLAJA_OPTION

#endif //PLAJA_FAULT_ENGINE_H
