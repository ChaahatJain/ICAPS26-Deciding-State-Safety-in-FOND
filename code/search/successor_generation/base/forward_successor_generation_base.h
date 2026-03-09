//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_FORWARD_SUCCESSOR_GENERATION_BASE_H
#define PLAJA_FORWARD_SUCCESSOR_GENERATION_BASE_H

namespace SUCCESSOR_GENERATION {

    template<typename Edge_t, typename ActionOp_t>
    struct ActionOpConstruction;

}

template<typename ActionOpView_t, typename ActionOpEntry_t, template<typename, typename> typename Container_t>
class ActionOpIteratorBase;

template<typename ActionOpView_t, typename ActionOpEntry_t, template<typename, typename> typename Container_t>
class ActionLabelIteratorBase;

template<typename ActionOpView_t, typename ActionOpEntry_t, template<typename, typename> typename Container_t>
class AllActionOpIteratorBase;

#endif //PLAJA_FORWARD_SUCCESSOR_GENERATION_BASE_H
