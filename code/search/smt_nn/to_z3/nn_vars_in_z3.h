//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_NN_VARS_IN_Z3_H
#define PLAJA_NN_VARS_IN_Z3_H

#include <unordered_map>
#include "../../../assertions.h"
#include "../../smt/using_smt.h"
#include "../using_marabou.h"

// forward declaration:
class StateIndexesInMarabou;
namespace Z3_IN_PLAJA { class Context; class StateVarsInZ3; }
namespace MARABOU_IN_PLAJA { class Context; class MarabouQuery; }


namespace Z3_IN_PLAJA {

extern const std::string marabouPrefix;

class MarabouToZ3Vars final {

private:
    Z3_IN_PLAJA::Context* context;
    const MARABOU_IN_PLAJA::Context* contextMarabou;
    std::unordered_map<MarabouVarIndex_type, Z3_IN_PLAJA::VarId_type> marabouToZ3;

public:
    MarabouToZ3Vars(Z3_IN_PLAJA::Context& c, const MARABOU_IN_PLAJA::Context& context_m, std::unordered_map<MarabouVarIndex_type, Z3_IN_PLAJA::VarId_type>&& marabou_to_z3 = {});
    ~MarabouToZ3Vars();
    DELETE_CONSTRUCTOR(MarabouToZ3Vars)

    [[nodiscard]] inline Z3_IN_PLAJA::Context& _context() const { return *context; }
    [[nodiscard]] inline const MARABOU_IN_PLAJA::Context& _context_marabou() const { return *contextMarabou; }
    [[nodiscard]] z3::context& get_context_z3() const;

    Z3_IN_PLAJA::VarId_type add(MarabouVarIndex_type marabou_var);
    [[nodiscard]] Z3_IN_PLAJA::VarId_type add_int_aux_var() const;

    [[nodiscard]] inline bool exists(MarabouVarIndex_type marabou_var) const { return marabouToZ3.count(marabou_var); }
    [[nodiscard]] inline Z3_IN_PLAJA::VarId_type to_z3(MarabouVarIndex_type marabou_var) const { PLAJA_ASSERT(exists(marabou_var)) return marabouToZ3.at(marabou_var); }
    [[nodiscard]] const z3::expr& to_z3_expr(MarabouVarIndex_type marabou_var) const;

    // mapping interface:
    [[nodiscard]] inline bool empty() const { return marabouToZ3.empty(); }
    [[nodiscard]] inline std::size_t size() const { return marabouToZ3.size(); }
    [[nodiscard]] inline std::unordered_map<MarabouVarIndex_type, Z3_IN_PLAJA::VarId_type>::const_iterator begin() const { return marabouToZ3.cbegin(); }
    [[nodiscard]] inline std::unordered_map<MarabouVarIndex_type, Z3_IN_PLAJA::VarId_type>::const_iterator end() const { return marabouToZ3.cend(); }

    static std::unordered_map<MarabouVarIndex_type, Z3_IN_PLAJA::VarId_type> reuse(const Z3_IN_PLAJA::StateVarsInZ3& state_vars_z3, const StateIndexesInMarabou& state_vars_marabou);

};

}

#endif //PLAJA_NN_VARS_IN_Z3_H
