//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SEARCH_STATISTICS_H
#define PLAJA_SEARCH_STATISTICS_H

/*
 * PlaJA: Developed out of a FastDownward class (SearchProgress, search_progress.{h,cc}).
 * Placed here for legacy reasons.
 */

#if 0

#include "../../stats/stats_base.h"


/**
 * This class helps to keep track of the search statistics.
 * (Formally contained in SearchProgress, res. search_progress.{h,cc}.)
 */
class SearchStatistics: public PLAJA::StatsBase {

private:
    static const std::list<PLAJA::StatsDouble> timeAttrs;
    static const std::list<std::string> timeAttrsStr;

protected:
    void reset_specific() override;

    // output
    void print_statistics_specific() const override;
    void stats_to_csv_specific(std::ofstream& file) const override;
    void stat_names_to_csv_specific(std::ofstream& file) const override;

public:
    SearchStatistics();
    ~SearchStatistics() override;
    DELETE_CONSTRUCTOR(SearchStatistics)

    /**
     * This add basic stats attributes, e.g., for expanded/generated states, ...
     */
    static void add_basic_stats(PLAJA::StatsBase& stats);

};

#endif

namespace PLAJA { class StatsBase; }

namespace SEARCH_STATISTICS {

    /**
     * Add minimal statistics used by the parent search engine.
     * @param stats
     */
    extern void add_stats(PLAJA::StatsBase& stats);

    /**
     * This add basic stats attributes, e.g., for expanded/generated states, ...
     * (Formally contained in SearchProgress, res. search_progress.{h,cc}.)
     */
    extern void add_basic_stats(PLAJA::StatsBase& stats);

}

#endif //PLAJA_SEARCH_STATISTICS_H
