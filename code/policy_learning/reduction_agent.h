#ifndef PLAJA_REDUCTION_AGENT_H
#define PLAJA_REDUCTION_AGENT_H

#include "../parser/ast/expression/forward_expression.h"
#include "../search/fd_adaptions/search_engine.h"
#include "../search/information/jani2nnet/forward_jani_2_nnet.h"
#include "../search/information/jani2nnet/using_jani2nnet.h"
#include "../search/states/forward_states.h"
#include "../search/successor_generation/forward_successor_generation.h"
#include "../search/using_search.h"
#include "forward_policy_learning.h"
#include "transition_set.h"
#include "using_policy_learning.h"
#include <unordered_set>
#include <vector>

class ReductionAgent: public SearchEngine {

private:
    unsigned int numEpisodes;
    const unsigned int maxLengthEpisode;
    const unsigned int numStartStates;
    const double reductionThreshold;
    const double frequency;
    const bool compact;
    const std::string* saveNNet;
    const std::string* savePolicyPath;
    const std::string* postfix;
    std::unique_ptr<std::string> saveBackup;
    const bool saveIntermediateStats;
    unsigned int episodesCounter;
    unsigned int subEpisodesCounter;
    int goalCounter;
    std::unique_ptr<ReductionStatistics> reductionStats;
    std::unique_ptr<class InitialStatesEnumerator> startEnumerator;
    std::vector<StateID_type> startStates;
    const Jani2NNet* jani2NNet;
    const ObjectiveExpression* objective;
    const Expression* avoid;
    std::unique_ptr<SimulationEnvironment> simulatorEnv;
    std::unique_ptr<QlAgentTorch> agent;
    std::unique_ptr<State> currentState;
    std::unique_ptr<State> nextState;
    std::vector<ActionLabel_type> cachedApplicableActions;
    QL_AGENT::TransitionSet cachedTransitionsPerSubEpisode;
    bool cycleDetected;

    [[nodiscard]] bool check_goal(const State& state) const;
    [[nodiscard]] bool check_avoid(const State& state) const;
    void set_current_state(const State& state);
    void set_next_state(const State& state);
    void set_next_state_applicability();
    void set_not_done(const State& state);
    void set_goal(const State& state);
    void set_avoid(const State& state);
    void set_stalling(bool terminal);
    void set_next_to_current_state();
    void load_nn(const std::string& torch_file, const std::string& nnet_file);
    void save_copy(const std::string& prefix);
    ActionLabel_type act();
    std::unique_ptr<State> simulate(const State& state, ActionLabel_type action_label);
    std::unique_ptr<State> simulate_until_choice_point(const State& state);
    State sample_start_state();
    SearchStatus initialize() override;
    SearchStatus step() override;
    void print_statistics() const override;
    void stats_to_csv(std::ofstream& file) const override;
    void stat_names_to_csv(std::ofstream& file) const override;

public:
    explicit ReductionAgent(const PLAJA::Configuration& config);
    ~ReductionAgent() override;
    DELETE_CONSTRUCTOR(ReductionAgent)
};


#endif//PLAJA_REDUCTION_AGENT_H
