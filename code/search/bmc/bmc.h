//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_BMC_H
#define PLAJA_BMC_H

#include "../../option_parser/forward_option_parser.h"
#include "../fd_adaptions/search_engine.h"
#include "forward_bmc.h"

class BoundedMC: public SearchEngine {

private:
    std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet> bmcMode;
    std::unique_ptr<SolutionCheckerBmc> solutionChecker;

    std::unique_ptr<std::string> savePath;

    /* Options. */
    const unsigned int maxSteps;
protected:
    const bool findShortestPathOnly;
    const bool constrainNoStart;
    const bool constrainLoopFree;
    const bool constrainNoReach;

    [[nodiscard]] inline const SolutionCheckerBmc& get_checker() const { return *solutionChecker; }

    [[nodiscard]] inline const std::string* get_save_path_file() const { return savePath.get(); }

private:
    int currentStep;
    int shortestUnsafePath;
    int longestLoopFreePath;

    SearchStatus initialize() override;
    SearchStatus step() override;

    SearchStatus check_loop_free();
    SearchStatus check_unsafe();

    [[nodiscard]] virtual bool check_start_smt() const = 0;
    [[nodiscard]] virtual bool check_loop_free_smt(unsigned int steps) const = 0;
    [[nodiscard]] virtual bool check_unsafe_smt(unsigned int steps) const = 0;

public:
    explicit BoundedMC(const PLAJA::Configuration& config);
    ~BoundedMC() override;
    DELETE_CONSTRUCTOR(BoundedMC)

};

namespace PLAJA_OPTION { extern std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet> construct_bmc_mode_enum(); }

#endif //PLAJA_BMC_H
