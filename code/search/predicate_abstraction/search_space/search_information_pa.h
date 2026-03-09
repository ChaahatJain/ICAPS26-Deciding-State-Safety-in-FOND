//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SEARCH_INFORMATION_PA_H
#define PLAJA_SEARCH_INFORMATION_PA_H

#include <memory>
#include "../inc_state_space/forward_inc_state_space_pa.h"

namespace SEARCH_SPACE_PA {

    struct SharedSearchSpace final {

    private:
        std::shared_ptr<IncStateSpace> sharedStateSpace;
        std::shared_ptr<StateRegistryPa> sharedStateRegistryPa;

    public:
        SharedSearchSpace() = default;
        ~SharedSearchSpace() = default;
        DELETE_CONSTRUCTOR(SharedSearchSpace)

        inline void set_state_space(std::shared_ptr<IncStateSpace> state_space) { sharedStateSpace = std::move(state_space); }

        [[nodiscard]] inline bool has_state_space() { return sharedStateSpace.get(); };

        [[nodiscard]] inline std::shared_ptr<IncStateSpace> share_state_space() { return sharedStateSpace; };

        inline void set_state_registry_pa(std::shared_ptr<StateRegistryPa> state_registry_pa) { sharedStateRegistryPa = std::move(state_registry_pa); }

        [[nodiscard]] inline bool has_state_registry() { return sharedStateRegistryPa.get(); };

        [[nodiscard]] inline std::shared_ptr<StateRegistryPa> share_state_registry() { return sharedStateRegistryPa; };

    };

}

#endif //PLAJA_SEARCH_INFORMATION_PA_H
