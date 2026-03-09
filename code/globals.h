//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_GLOBALS_H
#define PLAJA_GLOBALS_H

#include <memory>
#include <shared_mutex>
#include <string>
#include "assertions.h"

class Model;

class RandomNumberGenerator;

class Timer;

class SearchEngine;

namespace PLAJA { class OptionParser; }

namespace PLAJA_GLOBAL {

    /**
     * For functions and data structures used globally.
     */

    /* Mutex for outputs. */
    extern std::shared_mutex signalLock;

    extern const std::unique_ptr<Timer> timer;

    extern std::unique_ptr<RandomNumberGenerator> rng;

    extern const PLAJA::OptionParser* optionParser;

    /* To output statistics in case of external timeout, see search/utilities.cc. */
    extern SearchEngine* currentEngine;

    /* For some runtime checks (or debugging outputs),
     * e.g. array assignment within array range,
     * we need model information not locally stored in the AST, e.g., variable expression.
    */
    extern const Model* currentModel; // May be updated over time when considering different models, e.g. abstractions.

    /* Randomize evaluation of non-deterministic assignments. */
    extern bool randomizeNonDetEval;

    /* Hack to trigger some additional outputs in debug mode. */
    FCT_IF_DEBUG(extern void push_additional_outputs_frame(bool value);)
    FCT_IF_DEBUG(extern void pop_additional_outputs_frame();)
    FCT_IF_DEBUG(extern bool do_additional_outputs();)

}

#endif //PLAJA_GLOBALS_H
