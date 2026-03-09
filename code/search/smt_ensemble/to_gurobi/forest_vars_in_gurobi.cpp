//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "forest_vars_in_gurobi.h"
#include "../../information/model_information.h"
#include "../../smt/context_z3.h"
#include "../../smt/vars_in_z3.h"
#include "../veritas_context.h"
#include "veritas_to_gurobi.h"
#include <string>

    GUROBI_IN_PLAJA::RFVeritasToGurobiVars::RFVeritasToGurobiVars(std::shared_ptr<GRBModel> gurobi_model, const veritas::FlatBox& inputs, veritas::AddTree trees, int action_label) : 
    model(gurobi_model) {

    for (auto i = 0; i < inputs.size(); ++i) {
        const auto bounds = inputs[i];
        const auto lb = bounds.lo;
        const auto ub = bounds.hi - 1;
        add(i, lb, ub);
    }
    
    auto num_outputs = trees.num_leaf_values();
    int num_trees = trees.size();
    for (auto tree = 0; tree < num_trees; ++tree) {
        auto t = trees[tree];
        // Add leaf variables for each tree
        auto leaf_ids = t.get_leaf_ids();
        for (auto id : leaf_ids) {
            add_leaf(tree, id);
        }

        // Add predicate variables for each node of each tree
        for (auto node = 0; node < t.num_nodes(); ++node) {
            if (t.is_internal(node)) {
                add_predicate(tree, node, t.get_split(node));
            }
        }
    }
    
    for (auto label = 0; label < num_outputs; ++label) {
        for (auto tree = 0; tree < num_trees; ++tree) {
            add_tree_action_aux(tree, label);
        }
    }

    for (auto tree = 0; tree < num_trees; ++tree) {   
        veritas::Tree t = trees[tree];
        add_tree_action_aux_constraint(tree, t);
    }

    for (auto label = 0; label < num_outputs; ++label) {
            add_label_value(label, num_trees);
    }

    // Add leaf consistency:
    for (auto tree = 0; tree < num_trees; ++tree) {
            veritas::Tree t = trees[tree];
            add_leaf_consistency(tree, t);
    }

    set_objective(action_label);
    
}

GUROBI_IN_PLAJA::RFVeritasToGurobiVars::~RFVeritasToGurobiVars() = default;

//

void GUROBI_IN_PLAJA::RFVeritasToGurobiVars::add(VeritasVarIndex_type veritas_var, double lb, double ub) {
    auto var = model->addVar(lb, ub, veritas_var, GRB_INTEGER, GUROBI_IN_PLAJA::featurePrefix + std::to_string(veritas_var));
    feature_mapping.emplace(veritas_var, var);
}

void GUROBI_IN_PLAJA::RFVeritasToGurobiVars::add_leaf(int tree_index, int leaf_index) {
    std::string name = leafPrefix + std::to_string(tree_index) + "_" + std::to_string(leaf_index);
    auto var = model->addVar(0, 1, 0, GRB_BINARY, name);
    std::pair tup = {tree_index, leaf_index};
    leavesToGurobi[tup] = var;
}

void GUROBI_IN_PLAJA::RFVeritasToGurobiVars::add_tree_action_aux(int tree, int label) {
    std::string name = treePrefix + std::to_string(tree) + "_" + std::to_string(label);
    const auto var = model->addVar(VERITAS_IN_PLAJA::negative_infinity, VERITAS_IN_PLAJA::infinity, 0, GRB_CONTINUOUS, name);
    treeAuxToGurobi[{tree, label}] = var;
}

void GUROBI_IN_PLAJA::RFVeritasToGurobiVars::add_tree_action_aux_constraint(int tree_index, veritas::Tree tree) {
    auto num_labels = tree.num_leaf_values();
    auto leaf_ids = tree.get_leaf_ids();

    for (auto label = 0; label < num_labels; ++label) {
        auto tree_var = tree_aux_to_gurobi(tree_index, label);
        GRBLinExpr expr = 0;
        for (auto leaf : leaf_ids) {
            auto leaf_var = leaf_to_gurobi(tree_index, leaf);
            veritas::FloatT leaf_value = 0;
            leaf_value = tree.leaf_value(leaf, label);
            expr += leaf_value*leaf_var;
        }
        std::string name = "calculate_tree_" + std::to_string(tree_index) + "_" + std::to_string(label);
        try {
            model->addConstr(expr, GRB_EQUAL, tree_var, name);
            } 
            catch (GRBException e)
            {
                std::cout << "Error code = " << e.getErrorCode() << std::endl;
                std::cout << e.getMessage() << std::endl;
            }
    }
}


void GUROBI_IN_PLAJA::RFVeritasToGurobiVars::add_label_value(int label, int num_trees) {
    std::string name = outputPrefix + std::to_string(label);
    const auto var = model->addVar(-200000, 100000, 0, GRB_CONTINUOUS, name);
    label_values[label] = var;
    GRBLinExpr expr = 0;
    for (auto t = 0; t < num_trees; ++t) {
        auto var = tree_aux_to_gurobi(t, label);
        expr += var;
    }
    std::string con_name = "label_value_" + std::to_string(label);
    model->addConstr(expr, GRB_EQUAL, var, con_name);
}


void GUROBI_IN_PLAJA::RFVeritasToGurobiVars::add_predicate(int tree_index, int node_id, veritas::LtSplit split) {
    std::string name = predicatePrefix + std::to_string(tree_index) + "_" + std::to_string(node_id);
    auto var = model->addVar(0, 1, 0, GRB_BINARY, name);
    std::pair tup = {tree_index, node_id};
    nodesToGurobi[tup] = var;
    auto feat_var = feature_to_gurobi(split.feat_id);
    auto constant = split.split_value;
    model->addGenConstrIndicator(var, true, feat_var, GRB_LESS_EQUAL, constant - 1);
    model->addGenConstrIndicator(var, false, feat_var, GRB_GREATER_EQUAL, constant);
}

void GUROBI_IN_PLAJA::RFVeritasToGurobiVars::add_leaf_consistency(int tree_index, veritas::Tree tree) {
    add_leaf_sum_consistency(tree_index, tree);
        // All leafs of tree add upto 1.
    for (auto node = 0; node < tree.num_nodes(); ++node) {
        if (tree.is_root(node)) {
            add_leaf_root_consistency(tree_index, tree, node);
        } else {
            if (tree.is_internal(node)) {
                add_leaf_internal_consistency(tree_index, tree, node);
            }
        }
    }
}


void GUROBI_IN_PLAJA::RFVeritasToGurobiVars::add_leaf_sum_consistency(int tree_index, veritas::Tree tree) {
    auto leaf_ids = tree.get_leaf_ids();
    GRBLinExpr expr = 0;
    for (auto leaf : leaf_ids) {
        auto var = leaf_to_gurobi(tree_index, leaf);
        expr += var;
    }
    std::string name = "leaf_sum_" + std::to_string(tree_index);
    model->addConstr(expr, GRB_EQUAL, 1, name);
}

void GUROBI_IN_PLAJA::RFVeritasToGurobiVars::add_leaf_root_consistency(int tree_index, veritas::Tree tree, veritas::NodeId node) {
    std::vector<veritas::NodeId> left_ids;
    if (tree.is_leaf(node)) {
        return;
    }
    tree.get_leaf_ids(tree.left(node), left_ids);
    auto pred_var = node_to_gurobi(tree_index, node);
    GRBLinExpr expr = 0;
    std::string left_name = "left_root_con_" + std::to_string(tree_index);
    std::string right_name = "right_root_con_" + std::to_string(tree_index);

    for (auto id: left_ids) {
        auto leaf_var = leaf_to_gurobi(tree_index, id);
        expr += leaf_var;
    }
    model->addConstr(expr, GRB_EQUAL, pred_var, left_name);
    
    std::vector<veritas::NodeId> right_ids;
    tree.get_leaf_ids(tree.right(node), right_ids);
    GRBLinExpr right_expr = 1;
    for (auto id: right_ids) {
        auto leaf_var = leaf_to_gurobi(tree_index, id);
        right_expr -= leaf_var;
    }
    model->addConstr(right_expr, GRB_EQUAL, pred_var, right_name);
}

void GUROBI_IN_PLAJA::RFVeritasToGurobiVars::add_leaf_internal_consistency(int tree_index, veritas::Tree tree, veritas::NodeId node) {
    std::vector<veritas::NodeId> left_ids;
    if (tree.is_leaf(node)) {
        return;
    }
    tree.get_leaf_ids(tree.left(node), left_ids);
    auto pred_var = node_to_gurobi(tree_index, node);
    GRBLinExpr expr = 0;
    std::string left_name = "left_node_con_" + std::to_string(tree_index) + "_" + std::to_string(node) ;
    std::string right_name = "right_node_con_" + std::to_string(tree_index) + "_" + std::to_string(node);

    for (auto id: left_ids) {
        auto leaf_var = leaf_to_gurobi(tree_index, id);
        expr += leaf_var;
    }
    model->addConstr(expr, GRB_LESS_EQUAL, pred_var, left_name);
    
    std::vector<veritas::NodeId> right_ids;
    tree.get_leaf_ids(tree.right(node), right_ids);
    GRBLinExpr right_expr = 1;
    for (auto id: right_ids) {
        auto leaf_var = leaf_to_gurobi(tree_index, id);
        right_expr -= leaf_var;
    }
    model->addConstr(right_expr, GRB_GREATER_EQUAL, pred_var, right_name);
}

void GUROBI_IN_PLAJA::RFVeritasToGurobiVars::add_equation(VERITAS_IN_PLAJA::Equation eq) {
    GRBLinExpr expr = -eq._scalar;
    for (auto addend : eq._addends) {
        auto var = addend._variable;
        auto coeff = addend._coefficient;
        auto feat_var = feature_to_gurobi(var);
        expr += coeff*feat_var;
    }
    model->addConstr(expr, GRB_EQUAL, 0);
}
