//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_TREE_VARS_IN_Z3_H
#define PLAJA_TREE_VARS_IN_Z3_H

#include <unordered_map>
#include "../../../assertions.h"
#include "../../smt/using_smt.h"
#include "../using_veritas.h"
#include "../vars_in_veritas.h"
#include "addtree.hpp"
#include <map>
#include "../../smt/context_z3.h"
// forward declaration:
namespace Z3_IN_PLAJA { class Context; class StateVarsInZ3; }
namespace VERITAS_IN_PLAJA { class Context; class VeritasQuery; }


namespace Z3_IN_PLAJA {

extern const std::string veritasPrefix;
extern const std::string treePrefix;
extern const std::string outputPrefix;

class VeritasToZ3Vars final {

private:
    Z3_IN_PLAJA::Context& context;
    std::unordered_map<VeritasVarIndex_type, Z3_IN_PLAJA::VarId_type> veritasToZ3;
    std::map<std::pair<int, int>, Z3_IN_PLAJA::VarId_type> treesToZ3;
    std::map<int, Z3_IN_PLAJA::VarId_type> label_values;

public:
    VeritasToZ3Vars(Z3_IN_PLAJA::Context& c, const VERITAS_IN_PLAJA::Context& context_m, std::unordered_map<VeritasVarIndex_type, Z3_IN_PLAJA::VarId_type>&& veritas_to_z3 = {}, veritas::AddTree trees = veritas::AddTree(0, veritas::AddTreeType::REGR));
    ~VeritasToZ3Vars();
    DELETE_CONSTRUCTOR(VeritasToZ3Vars)

    [[nodiscard]] inline Z3_IN_PLAJA::Context& _context() const { return context; }
    [[nodiscard]] z3::context& get_context_z3() const;

    void add(const VERITAS_IN_PLAJA::Context* verContext);
    [[nodiscard]] Z3_IN_PLAJA::VarId_type add_int_aux_var() const;
    [[nodiscard]] Z3_IN_PLAJA::VarId_type add_real_aux_var() const;
    void add_operator_aux(const VERITAS_IN_PLAJA::Context* verContext);
    void add_tree_vars(veritas::AddTree trees);
    z3::expr_vector get_bounds(const std::shared_ptr<VERITAS_IN_PLAJA::Context> verContext);

    [[nodiscard]] inline bool exists(VeritasVarIndex_type veritas_var) const { return veritasToZ3.count(veritas_var); }
    [[nodiscard]] inline Z3_IN_PLAJA::VarId_type to_z3(VeritasVarIndex_type veritas_var) const { PLAJA_ASSERT(exists(veritas_var)) return veritasToZ3.at(veritas_var); }
    [[nodiscard]] const z3::expr& to_z3_expr(VeritasVarIndex_type veritas_var) const;
    [[nodiscard]] const z3::expr& to_z3_expr(int label, int tree) const {
        std::pair<int, int> key = {label, tree};
        auto it = treesToZ3.find(key);
         return context.get_var(it->second); 
        }
    [[nodiscard]] const z3::expr& output_label_to_z3_expr(int label) const {
            auto it = label_values.find(label);
            return context.get_var(it->second); 
        }
    // mapping interface:
    [[nodiscard]] inline bool empty() const { return veritasToZ3.empty(); }
    [[nodiscard]] inline std::size_t size() const { return veritasToZ3.size(); }
    [[nodiscard]] inline std::unordered_map<VeritasVarIndex_type, Z3_IN_PLAJA::VarId_type>::const_iterator begin() const { return veritasToZ3.cbegin(); }
    [[nodiscard]] inline std::unordered_map<VeritasVarIndex_type, Z3_IN_PLAJA::VarId_type>::const_iterator end() const { return veritasToZ3.cend(); }

    static std::unordered_map<VeritasVarIndex_type, Z3_IN_PLAJA::VarId_type> reuse(const Z3_IN_PLAJA::StateVarsInZ3& state_vars_z3, const StateIndexesInVeritas& state_vars_veritas);

};

}

#endif //PLAJA_TREE_VARS_IN_Z3_H
