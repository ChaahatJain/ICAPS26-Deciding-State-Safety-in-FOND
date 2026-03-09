//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SEARCH_PA_MACROS_H
#define PLAJA_SEARCH_PA_MACROS_H

#include "../predicate_abstraction.h"
#include "../search_pa_path.h"
#include "../heuristic/pa_heuristic.h"

namespace PA_COMPUTATION_AUX {

    template<typename SearchPA_t>
    constexpr bool is_path_search() { return std::is_same_v<SearchPA_t, SearchPAPath>; }

    template<typename SearchPA_t>
    constexpr bool is_pa() { return std::is_same_v<SearchPA_t, PredicateAbstraction>; }

    template<typename SearchPA_t>
    constexpr bool is_pa_heuristic() { return std::is_same_v<SearchPA_t, PAHeuristic>; }

};

/**********************************************************************************************************************/

#endif //PLAJA_SEARCH_PA_MACROS_H
