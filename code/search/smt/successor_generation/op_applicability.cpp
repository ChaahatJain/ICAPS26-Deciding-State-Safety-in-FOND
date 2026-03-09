//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "op_applicability.h"
#include "../../../option_parser/option_parser_aux.h"
#include "../../../option_parser/option_parser.h"
#include "../../../include/ct_config_const.h"
#include "../../../assertions.h"
#include "../../factories/configuration.h"

namespace PLAJA_OPTION_DEFAULT {

    const std::string cacheOpApp("cache-feasible"); // NOLINT(cert-err58-cpp) // Note this only applies for ENABLE_APPLICABILITY_FILTERING but then also for no-filtering problem instances.

}

namespace PLAJA_OPTION {

    const std::string cacheOpApp("cache-op-app"); // NOLINT(cert-err58-cpp)

    namespace OP_APPLICABILITY {

        enum Mode {
            None,
            CacheFeasible,
            CheckEntailed,
        };

        extern const std::unordered_map<std::string, Mode> stringToMode; // extern usage
        extern void add_options(PLAJA::OptionParser& option_parser);
        extern void print_options();

        const std::unordered_map<std::string, Mode> stringToMode { // NOLINT(cert-err58-cpp)
            { "none",           Mode::None },
            { "cache-feasible", Mode::CacheFeasible },
            { "check-entailed", Mode::CheckEntailed },
        };

        void add_options(PLAJA::OptionParser& option_parser) {
            if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) { OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::cacheOpApp, PLAJA_OPTION_DEFAULT::cacheOpApp); }
        }

        void print_options() {
            if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) {
                OPTION_PARSER::print_option(PLAJA_OPTION::cacheOpApp, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULT::cacheOpApp, "Cache (and utilize) per-operator applicability during abstract state expansion.", true);
                OPTION_PARSER::print_additional_specification(PLAJA::OptionParser::print_supported_options(stringToMode));
            }
        }

    }

}

/**********************************************************************************************************************/

std::unique_ptr<OpApplicability> OpApplicability::construct(const PLAJA::Configuration& config) {
    PLAJA_ASSERT(PLAJA_GLOBAL::enableApplicabilityCache)

    switch (config.get_option(PLAJA_OPTION::OP_APPLICABILITY::stringToMode, PLAJA_OPTION::cacheOpApp)) {
        case PLAJA_OPTION::OP_APPLICABILITY::None: { return nullptr; }
        case PLAJA_OPTION::OP_APPLICABILITY::CacheFeasible: { return std::make_unique<OpApplicability>(); }
        case PLAJA_OPTION::OP_APPLICABILITY::CheckEntailed: {
            auto rlt = std::make_unique<OpApplicability>();
            rlt->checkEntailed = true;
            return rlt;
        }
    }

    PLAJA_ABORT
}

OpApplicability::OpApplicability():
    checkEntailed(false)
    , selfApplicability(nullptr) {
}

OpApplicability::~OpApplicability() = default;

void OpApplicability::set_applicability(ActionOpID_type op, OpApplicability::Applicability value) {
    PLAJA_ASSERT(value != Applicability::Unknown)
    PLAJA_ASSERT(check_entailed() or value != Applicability::Entailed)
    PLAJA_ASSERT(not opApp.count(op))
    opApp.emplace(op, value);
}

OpApplicability::Applicability OpApplicability::get_applicability(ActionOpID_type op) const {
    auto it = opApp.find(op);
    PLAJA_ASSERT(it == opApp.end() or check_entailed() or it->second != Applicability::Entailed)
    return it == opApp.end() ? Applicability::Unknown : it->second;
}
