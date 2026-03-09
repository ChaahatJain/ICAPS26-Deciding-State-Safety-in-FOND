//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "globals.h"
#include "utils/rng.h"
#include "search/fd_adaptions/timer.h"

namespace PLAJA_GLOBAL {

    std::shared_mutex signalLock; // NOLINT(cert-err58-cpp)

    const std::unique_ptr<Timer> timer(new Timer()); // NOLINT(cert-err58-cpp)

    std::unique_ptr<RandomNumberGenerator> rng(new RandomNumberGenerator()); // NOLINT(cert-err58-cpp)

    const PLAJA::OptionParser* optionParser = nullptr;

    SearchEngine* currentEngine = nullptr;

    const Model* currentModel = nullptr;

    bool randomizeNonDetEval = false;

    FIELD_IF_DEBUG(extern std::list<bool> additionalOutputsFrame;)
    FIELD_IF_DEBUG(std::list<bool> additionalOutputsFrame { true };)

    FCT_IF_DEBUG(void push_additional_outputs_frame(bool value) { additionalOutputsFrame.push_back(value); })

    FCT_IF_DEBUG(
        void pop_additional_outputs_frame() {
            PLAJA_ASSERT(additionalOutputsFrame.size() > 1)
            additionalOutputsFrame.pop_back();
        }
    )

    FCT_IF_DEBUG(bool do_additional_outputs() { return additionalOutputsFrame.back(); })

}
