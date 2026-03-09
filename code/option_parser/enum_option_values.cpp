//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "enum_option_values.h"
#include "../utils/string_set.h"
#include "../assertions.h"

namespace PLAJA_ENUM_STR {

    const std::string none("none"); // NOLINT(*-err58-cpp)

#ifdef USE_Z3
    const std::string z3("z3"); // NOLINT(*-err58-cpp)
    const std::string naive("naive"); // NOLINT(*-err58-cpp)
    const std::string inc("inc"); // NOLINT(*-err58-cpp)
    const std::string bs("bs"); // NOLINT(*-err58-cpp)
    const std::string td("td"); // NOLINT(*-err58-cpp)
    const std::string sample("sample"); // NOLINT(*-err58-cpp)
    const std::string biased("biased"); // NOLINT(*-err58-cpp)
    const std::string wp("wp"); // NOLINT(*-err58-cpp)
    const std::string count("count"); // NOLINT(*-err58-cpp)
#endif

#ifdef USE_MARABOU
    const std::string marabou("marabou"); // NOLINT(*-err58-cpp)
    const std::string relaxed("relaxed"); // NOLINT(*-err58-cpp)
    const std::string bb("bb"); // NOLINT(*-err58-cpp)
    const std::string enumerate("enumerate"); // NOLINT(*-err58-cpp)
#endif

#ifdef BUILD_BMC
    const std::string unsafe("unsafe"); // NOLINT(*-err58-cpp)
    const std::string loopFree("loop-free"); // NOLINT(*-err58-cpp)
    const std::string unsafeLoopFree("unsafe-loop-free"); // NOLINT(*-err58-cpp)
    const std::string loopFreeUnsafe("loop-free-unsafe"); // NOLINT(*-err58-cpp)
#endif

#ifdef BUILD_PA

#ifndef PA_ONLY_REACHABILITY
    const std::string global("global"); // NOLINT(*-err58-cpp)
    const std::string perSrc("per-src"); // NOLINT(*-err58-cpp)
    const std::string perLabel("per-label"); // NOLINT(*-err58-cpp)
    const std::string perOp("per-op"); // NOLINT(*-err58-cpp)
#endif

#endif

#ifdef BUILD_TO_NUXMV
    const std::string bmc("bmc"); // NOLINT(*-err58-cpp)
    const std::string sbmc("sbmc"); // NOLINT(*-err58-cpp)
    const std::string ipaBmc("ipa-bmc"); // NOLINT(*-err58-cpp)
    const std::string ipaCegar("ipa-cegar"); // NOLINT(*-err58-cpp)
    const std::string cegar("cegar"); // NOLINT(*-err58-cpp)
    const std::string ic3("ic3"); // NOLINT(*-err58-cpp)
    const std::string coi("coi"); // NOLINT(*-err58-cpp)
#endif

}

namespace PLAJA_OPTION {

    std::unique_ptr<EnumOptionValue> str_to_enum_option_value(const std::string& str) {
        static const PLAJA::StringMap<EnumOptionValue> str_to_config {
            { &PLAJA_ENUM_STR::none, EnumOptionValue::None },
#ifdef USE_Z3
            { &PLAJA_ENUM_STR::z3, EnumOptionValue::Z3 },
            { &PLAJA_ENUM_STR::naive, EnumOptionValue::Naive },
            { &PLAJA_ENUM_STR::inc, EnumOptionValue::Inc },
            { &PLAJA_ENUM_STR::bs, EnumOptionValue::Bs },
            { &PLAJA_ENUM_STR::td, EnumOptionValue::Td },
            { &PLAJA_ENUM_STR::sample, EnumOptionValue::Sample },
            { &PLAJA_ENUM_STR::biased, EnumOptionValue::Biased },
            { &PLAJA_ENUM_STR::wp, EnumOptionValue::WP },
            { &PLAJA_ENUM_STR::count, EnumOptionValue::AvoidCount },

#endif
#ifdef USE_MARABOU
            { &PLAJA_ENUM_STR::marabou, EnumOptionValue::Marabou },
            { &PLAJA_ENUM_STR::relaxed, EnumOptionValue::Relaxed },
            { &PLAJA_ENUM_STR::bb, EnumOptionValue::BranchAndBound },
            { &PLAJA_ENUM_STR::relaxed, EnumOptionValue::Enumerate },
#endif
#ifdef BUILD_BMC
            { &PLAJA_ENUM_STR::unsafe, EnumOptionValue::BmcUnsafe },
            { &PLAJA_ENUM_STR::loopFree, EnumOptionValue::BmcLoopFree },
            { &PLAJA_ENUM_STR::unsafeLoopFree, EnumOptionValue::BmcUnsafeLoopFree },
            { &PLAJA_ENUM_STR::loopFreeUnsafe, EnumOptionValue::BmcLoopFreeUnsafe },
#endif
#ifdef BUILD_PA

#ifndef PA_ONLY_REACHABILITY
            { &PLAJA_ENUM_STR::global, EnumOptionValue::PaReachabilityGlobal },
            { &PLAJA_ENUM_STR::perSrc, EnumOptionValue::PaOnlyReachabilityPerSrc },
            { &PLAJA_ENUM_STR::perLabel, EnumOptionValue::PaOnlyReachabilityPerLabel },
            { &PLAJA_ENUM_STR::perOp, EnumOptionValue::PaOnlyReachabilityPerOp },
#endif

#endif
#ifdef BUILD_TO_NUXMV
            { &PLAJA_ENUM_STR::bmc, EnumOptionValue::Bmc },
            { &PLAJA_ENUM_STR::sbmc, EnumOptionValue::SBmc },
            { &PLAJA_ENUM_STR::ipaBmc, EnumOptionValue::IpaBmc },
            { &PLAJA_ENUM_STR::ipaCegar, EnumOptionValue::IpaCegar },
            { &PLAJA_ENUM_STR::cegar, EnumOptionValue::Cegar },
            { &PLAJA_ENUM_STR::ic3, EnumOptionValue::Ic3 },
            { &PLAJA_ENUM_STR::coi, EnumOptionValue::Coi },
#endif
        };

        const auto it = str_to_config.find(&str);
        return it == str_to_config.cend() ? nullptr : std::make_unique<EnumOptionValue>(it->second);
    }

    const std::string& enum_option_value_to_str(EnumOptionValue value) {
        switch (value) {
            case EnumOptionValue::None: { return PLAJA_ENUM_STR::none; }
#ifdef USE_Z3
            case EnumOptionValue::Z3: { return PLAJA_ENUM_STR::z3; }
            case EnumOptionValue::Naive: { return PLAJA_ENUM_STR::naive; }
            case EnumOptionValue::Inc: { return PLAJA_ENUM_STR::inc; }
            case EnumOptionValue::Bs: { return PLAJA_ENUM_STR::bs; }
            case EnumOptionValue::Td: { return PLAJA_ENUM_STR::td; }
            case EnumOptionValue::Sample: { return PLAJA_ENUM_STR::sample; }
            case EnumOptionValue::Biased: { return PLAJA_ENUM_STR::biased; }
            case EnumOptionValue::WP: { return PLAJA_ENUM_STR::wp; }
            case EnumOptionValue::AvoidCount: { return PLAJA_ENUM_STR::count; }
#endif
#ifdef USE_MARABOU
            case EnumOptionValue::Marabou: { return PLAJA_ENUM_STR::marabou; }
            case EnumOptionValue::Relaxed: { return PLAJA_ENUM_STR::relaxed; }
            case EnumOptionValue::BranchAndBound: { return PLAJA_ENUM_STR::bb; }
            case EnumOptionValue::Enumerate: { return PLAJA_ENUM_STR::enumerate; }
#endif
#ifdef BUILD_BMC
            case EnumOptionValue::BmcUnsafe: { return PLAJA_ENUM_STR::unsafe; }
            case EnumOptionValue::BmcLoopFree: { return PLAJA_ENUM_STR::loopFree; }
            case EnumOptionValue::BmcUnsafeLoopFree: { return PLAJA_ENUM_STR::unsafeLoopFree; }
            case EnumOptionValue::BmcLoopFreeUnsafe: { return PLAJA_ENUM_STR::loopFreeUnsafe; }
#endif
#ifdef BUILD_PA

#ifndef PA_ONLY_REACHABILITY
            case EnumOptionValue::PaReachabilityGlobal: { return PLAJA_ENUM_STR::global; }
            case EnumOptionValue::PaOnlyReachabilityPerSrc: { return PLAJA_ENUM_STR::perSrc; }
            case EnumOptionValue::PaOnlyReachabilityPerLabel: { return PLAJA_ENUM_STR::perLabel; }
            case EnumOptionValue::PaOnlyReachabilityPerOp: { return PLAJA_ENUM_STR::perOp; }
#endif

#endif
#ifdef BUILD_TO_NUXMV
            case EnumOptionValue::Bmc: { return PLAJA_ENUM_STR::bmc; }
            case EnumOptionValue::SBmc: { return PLAJA_ENUM_STR::sbmc; }
            case EnumOptionValue::IpaBmc: { return PLAJA_ENUM_STR::ipaBmc; }
            case EnumOptionValue::IpaCegar: { return PLAJA_ENUM_STR::ipaCegar; }
            case EnumOptionValue::Cegar: { return PLAJA_ENUM_STR::cegar; }
            case EnumOptionValue::Ic3: { return PLAJA_ENUM_STR::ic3; }
            case EnumOptionValue::Coi: { return PLAJA_ENUM_STR::coi; }
#endif
            default: PLAJA_ABORT
        }
    }

}