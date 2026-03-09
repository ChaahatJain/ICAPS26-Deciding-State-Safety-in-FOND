#ifndef PLAJA_REDUCTION_STATISTICS_H
#define PLAJA_REDUCTION_STATISTICS_H

#include <fstream>
#include <sstream>
#include <unordered_map>
#include "../utils/default_constructors.h"
#include "../assertions.h"

namespace PLAJA { class StatsBase; }

class ReductionStatistics final {

private:
    std::unordered_map<std::string, unsigned int> reducedNeuronsPerLayer;
    unsigned int totalReducedNeurons;

public:
    ReductionStatistics();
    ~ReductionStatistics();
    DELETE_CONSTRUCTOR(ReductionStatistics)

    inline void set_reduced_neurons_map(const std::string& layer) { reducedNeuronsPerLayer.emplace(layer, 0); };
    inline void inc_layer_reduced_neurons(const std::string& layer, unsigned int inc = 1) {
        totalReducedNeurons += inc;
        reducedNeuronsPerLayer[layer] += inc;
    };

    static void add_basic_stats(PLAJA::StatsBase& stats);
    void reset();
    void print_statistics() const;
    void stats_to_csv(std::ofstream& file) const;
    void stat_names_to_csv(std::ofstream& file) const;
};

#endif//PLAJA_REDUCTION_STATISTICS_H
