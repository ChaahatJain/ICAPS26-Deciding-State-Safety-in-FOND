//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PA_HEURISTIC_H
#define PLAJA_PA_HEURISTIC_H

#include "../heuristic/abstract_heuristic.h"

// forward declaration:
class ModelZ3PA;
class SearchPABase;
class PredicatesExpression;
struct PropertyInformation;
class SearchEngineConfigPA;

class PAHeuristic: public AbstractHeuristic {

/**********************************************************************************************************************/

public:

private:
    const ModelZ3PA& parentModel;
    std::unique_ptr<SearchEngineConfigPA> configEngine;
    std::unique_ptr<PropertyInformation> propInfo;
    std::unique_ptr<PredicatesExpression> predicates;
    std::size_t maxNumPreds;
    double maxTime;
    double maxTimeRel;
    double timeForPA;
    std::unique_ptr<SearchPABase> predAb;

public:
    explicit PAHeuristic(const PLAJA::Configuration& config);
    ~PAHeuristic() override;

    // heuristic
    [[nodiscard]] HeuristicValue_type _evaluate(const AbstractState& state) override;

    void increment() override; // incremental usage

    // stats:
    void print_statistics() const override;
    void stats_to_csv(std::ofstream& file) const override;
    void stat_names_to_csv(std::ofstream& file) const override;
};


#endif //PLAJA_PA_HEURISTIC_H
