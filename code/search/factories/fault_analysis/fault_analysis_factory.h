//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_FAULT_ANALYSIS_FACTORY_H
#define PLAJA_FAULT_ANALYSIS_FACTORY_H

#include "../smt_base/smt_base_factory.h"
#include "../../../option_parser/option_parser_aux.h"
#include "../../../option_parser/option_parser.h"
namespace FAULT_ANALYSIS {
#if USE_CUDD
    enum class Region {
        UNSAFE,
        REACHABLE,
    };
#endif
    enum class OracleUse {
        FAULT_FINDING, 
        SHIELDING,
        DATASET_GENERATION,
        POLICY_ACTION_FAULT_ON_STATE,
        SAFE_STATE_MANAGER,
    };


}


class FaultAnalysisFactory: public SMTBaseFactory {
private:
    [[nodiscard]] bool supports_property_sub(const PropertyInformation& prop_info) const override;
#if USE_CUDD
    const std::unordered_map<std::string, FAULT_ANALYSIS::Region> stringToRegion { // NOLINT(cert-err58-cpp)
            { "unsafe",    FAULT_ANALYSIS::Region::UNSAFE },
            { "reachable", FAULT_ANALYSIS::Region::REACHABLE }
    };
#endif

    const std::unordered_map<std::string, FAULT_ANALYSIS::OracleUse> stringToOracleUse { // NOLINT(cert-err58-cpp)
            { "finding", FAULT_ANALYSIS::OracleUse::FAULT_FINDING },
            { "shielding", FAULT_ANALYSIS::OracleUse::SHIELDING },
            { "dataset_generation", FAULT_ANALYSIS::OracleUse::DATASET_GENERATION },
            { "policy_action_fault_on_state", FAULT_ANALYSIS::OracleUse::POLICY_ACTION_FAULT_ON_STATE},
            { "safe_state_manager", FAULT_ANALYSIS::OracleUse::SAFE_STATE_MANAGER}, 
    };


public:
    FaultAnalysisFactory();
    ~FaultAnalysisFactory() override;
    DELETE_CONSTRUCTOR(FaultAnalysisFactory)

    std::string print_supported_regions() const;
    std::string print_supported_oracle_uses() const;

#if USE_CUDD
    FAULT_ANALYSIS::Region get_region(const PLAJA::Configuration& config) const;
#endif

    FAULT_ANALYSIS::OracleUse get_oracle_use(const PLAJA::Configuration& config) const;

    void add_options(PLAJA::OptionParser& option_parser) const override;
    void print_options() const override;
    void add_pa_cegar_options(PLAJA::OptionParser& option_parser) const;
    void print_pa_cegar_options() const;

    [[nodiscard]] std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config) const override;

};

#endif //PLAJA_FAULT_ANALYSIS_FACTORY_H
