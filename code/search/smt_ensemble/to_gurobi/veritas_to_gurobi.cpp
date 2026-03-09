//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include <z3++.h>
#include "../../../assertions.h"
#include "../../smt/visitor/extern/to_z3_visitor.h"
#include "../veritas_query.h"
#include "veritas_to_gurobi.h"

namespace VERITAS_TO_GUROBI {

    z3::expr generate_equation(const VERITAS_IN_PLAJA::Equation& equation, const GUROBI_IN_PLAJA::VeritasToGurobiVars& variables) {
        const auto& addends = equation._addends;
        PLAJA_ASSERT(not addends.empty())
        PLAJA_ABORT
    }

    //


    std::vector<z3::expr> generate_output_interfaces(const veritas::AddTree& ensemble, const GUROBI_IN_PLAJA::VeritasToGurobiVars& vars) {
        std::vector<z3::expr> output_interface_per_label;
        const auto num_outputs = ensemble.num_leaf_values();
        output_interface_per_label.reserve(num_outputs);
        for (OutputIndex_type output_label = 0; output_label < num_outputs; ++output_label) {
            output_interface_per_label.push_back(generate_output_interface(output_label, ensemble, vars));
        }
        return output_interface_per_label;
    }

    void get_splits(std::vector<std::pair<std::vector<std::pair<veritas::LtSplit, bool>>, veritas::FloatT>>& splits, std::pair<std::vector<std::pair<veritas::LtSplit, bool>>, veritas::FloatT> split, veritas::Tree tree, veritas::NodeId id, int depth, int label) {
        int i = 1;
        for (; i < depth; ++i)
            split.first.shrink_to_fit();
        if (tree.is_leaf(id)) {
            split.second = tree.leaf_value(id, label);
            splits.push_back(split);
        } else {
            auto split_left = split;
            auto split_right = split;
            split_left.first.push_back({tree.get_split(id), true});
            split_right.first.push_back({tree.get_split(id), false});
            get_splits(splits, split_left, tree, tree.left(id), depth+1, label);
            get_splits(splits, split_right, tree, tree.right(id), depth+1, label);
        }
    }

    std::vector<std::pair<std::vector<std::pair<veritas::LtSplit, bool>>, veritas::FloatT>> get_splits_for_tree(veritas::Tree tree, int label) {
        std::vector<std::pair<std::vector<std::pair<veritas::LtSplit, bool>>, veritas::FloatT>> splits;
        splits.reserve(tree.num_leaves());
        std::vector<std::pair<veritas::LtSplit, bool>> aux(0);
        std::pair<std::vector<std::pair<veritas::LtSplit, bool>>, veritas::FloatT> split = {aux, 0};
        get_splits(splits, split, tree, tree.root(), tree.depth(tree.root()), label);
        return splits;
    }

    void print_tree_path(std::vector<std::pair<std::vector<std::pair<veritas::LtSplit, bool>>, veritas::FloatT>> splits) {
        for (auto split : splits) {
            std::cout << "Found path: [" << std::endl;
            for (auto s : split.first) {
                std::cout << (s.second ? "(" : " (not ") << s.first << "), ";
            }
            std::cout << "and leaf value: " << split.second<< std::endl;
        } 
    }

    z3::expr tree_paths_to_z3(std::vector<std::pair<std::vector<std::pair<veritas::LtSplit, bool>>, veritas::FloatT>> splits, const GUROBI_IN_PLAJA::VeritasToGurobiVars& vars, int label, int tree_index) {
        z3::expr disjuncts = tree_path_to_z3(splits[0], vars, label, tree_index);
        for (auto i = 1; i < splits.size(); ++i) {
            disjuncts = disjuncts || tree_path_to_z3(splits[i], vars, label, tree_index);
        }        
        return disjuncts;
    }


    z3::expr_vector generate_ensemble_forwarding_structures(const VERITAS_IN_PLAJA::VeritasQuery& query, const GUROBI_IN_PLAJA::VeritasToGurobiVars& vars) {
        z3::expr_vector equations = generate_tree_paths(query.get_input_query(), vars);
        z3::expr_vector constraints = generate_conjunction_of_equations(query, vars);
        for (auto e : constraints) {
            equations.push_back(e);
        }
        z3::expr_vector outputs = calculate_action_label_values_in_z3(query.get_input_query(), vars);
        for (auto e : outputs) {
            equations.push_back(e);
        }
        return equations;
    }

}
