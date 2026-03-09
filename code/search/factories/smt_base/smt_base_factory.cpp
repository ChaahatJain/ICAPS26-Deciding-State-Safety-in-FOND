//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "smt_base_factory.h"
#include "../../../include/factory_config_const.h"
#include "../../../option_parser/option_parser_aux.h"
#include "../../../parser/ast/model.h"
#include "../../smt/visitor/restrictions_smt_checker.h"
// #include "smt_options.h"


// extern:
namespace PLAJA_UTILS { extern const std::string dotString; }
namespace PLAJA_OPTION {
    namespace SMT_SOLVER_MARABOU { extern void add_options(PLAJA::OptionParser& option_parser); extern void print_options(); }
    namespace SMT_SOLVER_Z3 { extern void add_options(PLAJA::OptionParser& option_parser); extern void print_options(); }
    namespace SOLVER_ENSEMBLE { extern void add_options(PLAJA::OptionParser& option_parser); extern void print_options(); }
}

//

SMTBaseFactory::SMTBaseFactory(SearchEngineFactory::SearchEngineType engine_type): SearchEngineFactory(engine_type) {}

SMTBaseFactory::~SMTBaseFactory() = default;

void SMTBaseFactory::add_options(PLAJA::OptionParser& option_parser) const {
    SearchEngineFactory::add_options(option_parser);
#ifdef USE_Z3
    PLAJA_OPTION::SMT_SOLVER_Z3::add_options(option_parser);
#endif
#ifdef USE_MARABOU
    PLAJA_OPTION::SMT_SOLVER_MARABOU::add_options(option_parser);
#endif
#ifdef USE_VERITAS
    PLAJA_OPTION::SOLVER_ENSEMBLE::add_options(option_parser);
#endif
}

void SMTBaseFactory::print_options_base() {
#ifdef USE_Z3
    PLAJA_OPTION::SMT_SOLVER_Z3::print_options();
#endif
#ifdef USE_MARABOU
    PLAJA_OPTION::SMT_SOLVER_MARABOU::print_options();
#endif
#ifdef USE_VERITAS
    PLAJA_OPTION::SOLVER_ENSEMBLE::print_options();
#endif
}

void SMTBaseFactory::supports_model(const Model& model) const {
    RestrictionsSMTChecker::check_restrictions(&model, false);
}

bool SMTBaseFactory::supports_property_sub(const PropertyInformation& /*prop_info*/) const { return true; }
