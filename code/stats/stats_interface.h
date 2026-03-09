//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_STATS_INTERFACE_H
#define PLAJA_STATS_INTERFACE_H

#include <fstream>

namespace PLAJA {

class StatsInterface {

public:
    virtual void print_statistics() const;
    virtual void stats_to_csv(std::ofstream& file) const;
    virtual void stat_names_to_csv(std::ofstream& file) const;

};

}

#endif //PLAJA_STATS_INTERFACE_H
