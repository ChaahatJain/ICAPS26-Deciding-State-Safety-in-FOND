//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SEARCH_STATISTICS_CEGAR_H
#define PLAJA_SEARCH_STATISTICS_CEGAR_H

#if 0

#include "../../fd_adaptions/search_statistics.h"

class SearchStatisticsCegar: public SearchStatistics {

private:
    static const std::list<PLAJA::StatsUnsigned> unsignedAttrs;
    static const std::list<std::string> unsignedAttrsStr;
    static const std::list<PLAJA::StatsDouble> doubleAttrs;
    static const std::list<std::string> doubleAttrsStr;

    void reset_specific() override;

    // output
    void print_statistics_specific() const override;
    void stats_to_csv_specific(std::ofstream& file) const override;
    void stat_names_to_csv_specific(std::ofstream& file) const override;

public:
    SearchStatisticsCegar();
    ~SearchStatisticsCegar() override;
};

#endif

namespace PLAJA { class StatsBase; }
namespace PA_CEGAR { extern void add_stats(PLAJA::StatsBase& stats); }

#endif //PLAJA_SEARCH_STATISTICS_CEGAR_H
