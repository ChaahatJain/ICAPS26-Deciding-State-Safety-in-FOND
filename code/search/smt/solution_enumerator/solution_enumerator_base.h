//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SOLUTION_ENUMERATOR_BASE_H
#define PLAJA_SOLUTION_ENUMERATOR_BASE_H

#include <list>
#include <memory>
#include "../../../utils/default_constructors.h"
#include "../../../assertions.h"
#include "../../information/forward_information.h"
#include "../../states/forward_states.h"

class SolutionEnumeratorBase  {

private:
    class RandomNumberGenerator* rng;
    const ModelInformation* modelInfo;
    std::unique_ptr<StateValues> defaultValues;
    bool initialized;

protected:
    std::list<StateValues> solutions; // Used in derived classes

    [[nodiscard]] inline const ModelInformation& get_model_info() { return *modelInfo; }

    [[nodiscard]] inline RandomNumberGenerator& get_rng() { return *rng; }

    [[nodiscard]] inline const StateValues& get_constructor() { return *defaultValues; }

    inline void set_initialized() { initialized = true; }

    [[nodiscard]] inline bool is_initialized() { return initialized; }

public:
    explicit SolutionEnumeratorBase(const ModelInformation& model_info);
    virtual ~SolutionEnumeratorBase() = 0;
    DELETE_CONSTRUCTOR(SolutionEnumeratorBase)

    void set_constructor(std::unique_ptr<StateValues>&& default_values);

    virtual void initialize();

    /** Retrieve next solution. */
    [[nodiscard]] virtual std::unique_ptr<StateValues> retrieve_solution();

    [[nodiscard]] virtual std::list<StateValues> enumerate_solutions();

    [[nodiscard]] virtual std::unique_ptr<StateValues> sample();

};

#endif //PLAJA_SOLUTION_ENUMERATOR_BASE_H
