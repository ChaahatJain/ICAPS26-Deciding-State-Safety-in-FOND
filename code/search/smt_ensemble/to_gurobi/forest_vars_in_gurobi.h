//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//
// Representing tree ensembles as MILP encoding
// Encoding based on the paper: Evasion and Hardening of Tree Ensemble Classifiers by Kantchelian etal. ICML (2016)
//

#ifndef PLAJA_FOREST_VARS_IN_GUROBI_H
#define PLAJA_FOREST_VARS_IN_GUROBI_H

#include <unordered_map>
#include "../../../assertions.h"
#include "../../smt/using_smt.h"
#include "../using_veritas.h"
#include "../vars_in_veritas.h"
#include "addtree.hpp"
#include <map>
#include <tuple>
#include "../../smt/context_z3.h"
#include "gurobi_c++.h"
#include "../predicates/Equation.h"
// forward declaration:
namespace GUROBI_IN_PLAJA { class Context; }
namespace VERITAS_IN_PLAJA { class Context; class VeritasQuery; }


namespace GUROBI_IN_PLAJA {
class RFVeritasToGurobiVars final {

private:
    std::shared_ptr<GRBModel> model;
    std::unordered_map<VeritasVarIndex_type, GRBVar> feature_mapping;
    std::map<std::pair<int, int>, GRBVar> leavesToGurobi; // leaves variables - For every tree, we have tree, leaf index key
    std::map<std::pair<int, int>, GRBVar> nodesToGurobi; // predicate variables - For every tree, we have tree, node index pair
    std::map<std::pair<int, int>, GRBVar> treeAuxToGurobi; // aux variable for value of tree - For every tree, we have tree, class pair
    std::map<int, GRBVar> label_values; // one variable for each label

public:
    RFVeritasToGurobiVars(std::shared_ptr<GRBModel> gurobi_model, const veritas::FlatBox& inputs, veritas::AddTree trees = veritas::AddTree(0, veritas::AddTreeType::REGR), int action_label = 0);
    ~RFVeritasToGurobiVars();
    DELETE_CONSTRUCTOR(RFVeritasToGurobiVars)

    [[nodiscard]] inline GRBModel& _model() const { return *model; }

    void add(VeritasVarIndex_type veritas_var, double lb, double ub);
    void add_leaf(int tree_index, int leaf_index);
    void add_tree_action_aux(int tree_index, int action_label);
    void add_tree_action_aux_constraint(int tree_index, veritas::Tree tree);
    void add_label_value(int action_label, int num_trees);
    void add_predicate(int tree_index, int node_id, veritas::LtSplit split);
    void add_leaf_consistency(int tree_index, veritas::Tree tree);
    void add_leaf_sum_consistency(int tree_index, veritas::Tree tree);
    void add_leaf_root_consistency(int tree_index, veritas::Tree tree, veritas::NodeId node);
    void add_leaf_internal_consistency(int tree_index, veritas::Tree tree, veritas::NodeId node);

    void dump_model(bool abort = true) {
        try {
            model->write("forest_gurobi_initialize.lp");
        }
        catch (GRBException e)
        {
            std::cout << "Error code = " << e.getErrorCode() << std::endl;
            std::cout << e.getMessage() << std::endl;
        }
        PLAJA_ASSERT(not abort);
    }

    void add_equation(VERITAS_IN_PLAJA::Equation eq);
    void add_equations(std::vector<VERITAS_IN_PLAJA::Equation> equations) {
        for (auto e : equations) {
            add_equation(e);
        }
    }

    void set_objective(int label) {
        auto var = output_label_to_gurobi(label);
        GRBLinExpr expr = var;
        model->setObjective(expr, GRB_MAXIMIZE);
        for (auto label_not = 0; label_not < label_values.size(); ++label_not) {
            if (label == label_not) continue;
            auto var_not = output_label_to_gurobi(label_not);
            std::string name = "select_" + std::to_string(label) + "_" + std::to_string(label_not);
            model->addConstr(var, GRB_GREATER_EQUAL, var_not, name);
        }
    }
    [[nodiscard]] inline bool exists(VeritasVarIndex_type veritas_var) const { return feature_mapping.count(veritas_var); }
    [[nodiscard]] inline int num_features() const { return feature_mapping.size(); }
    [[nodiscard]] inline GRBVar feature_to_gurobi(VeritasVarIndex_type veritas_var) const { PLAJA_ASSERT(exists(veritas_var)) return feature_mapping.at(veritas_var); }
    [[nodiscard]] const GRBVar& leaf_to_gurobi(int tree_index, int leaf) const {
        std::pair<int, int> key = {tree_index, leaf};
        auto it = leavesToGurobi.find(key);
        return it->second; 
    }

    [[nodiscard]] const GRBVar& node_to_gurobi(int tree_index, int node_id) const {
        std::pair<int, int> key = {tree_index, node_id};
        auto it = nodesToGurobi.find(key);
        return it->second; 
    }

    [[nodiscard]] const GRBVar& tree_aux_to_gurobi(int tree_index, int label) const {
        std::pair<int, int> key = {tree_index, label};
        auto it = treeAuxToGurobi.find(key);
        return it->second;
    }

    [[nodiscard]] const GRBVar& output_label_to_gurobi(int label) const {
            auto it = label_values.find(label);
            return it->second; 
        }
};

}

#endif //PLAJA_FOREST_VARS_IN_GUROBI_H
