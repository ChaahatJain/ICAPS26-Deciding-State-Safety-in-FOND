//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include <z3++.h>
#include "../../../assertions.h"
#include "../../smt/visitor/extern/to_z3_visitor.h"
#include "../veritas_query.h"
#include "veritas_to_z3.h"

namespace VERITAS_TO_Z3 {

    z3::expr generate_equation(const VERITAS_IN_PLAJA::Equation& equation, const Z3_IN_PLAJA::VeritasToZ3Vars& variables) {
        auto& context = variables.get_context_z3();
        const auto& addends = equation._addends;
        PLAJA_ASSERT(not addends.empty())

        // linear sum
        auto addends_it = addends.begin();
        const auto addends_end = addends.end();
        z3::expr eq_expr(Z3_IN_PLAJA::z3_to_real(context, addends_it->_coefficient, Z3_IN_PLAJA::floatingPrecision) * variables.to_z3_expr(addends_it->_variable));
        for (++addends_it; addends_it != addends_end; ++addends_it) {
            eq_expr = eq_expr + Z3_IN_PLAJA::z3_to_real(context, addends_it->_coefficient, Z3_IN_PLAJA::floatingPrecision) * variables.to_z3_expr(addends_it->_variable);
        }

        // comparison
        switch (equation._type) {
            case VERITAS_IN_PLAJA::Equation::EQ: { return eq_expr == Z3_IN_PLAJA::z3_to_real(context, equation._scalar, Z3_IN_PLAJA::floatingPrecision); }
            case VERITAS_IN_PLAJA::Equation::LE: { return eq_expr <= Z3_IN_PLAJA::z3_to_real(context, equation._scalar, Z3_IN_PLAJA::floatingPrecision); }
            case VERITAS_IN_PLAJA::Equation::GE: { return eq_expr >= Z3_IN_PLAJA::z3_to_real(context, equation._scalar, Z3_IN_PLAJA::floatingPrecision); }
        }
        // in case of preprocessing:
        // PLAJA_ASSERT(equation._type == Equation::EQ)
        // return eq_expr == Z3_TO_REAL(context, equation._scalar);

        PLAJA_ABORT
    }

    //

    z3::expr generate_output_interface(OutputIndex_type output_label, const veritas::AddTree& ensemble, const Z3_IN_PLAJA::VeritasToZ3Vars& vars) {
#ifdef ENABLE_APPLICABILITY_FILTERING
        PLAJA_ABORT
#else
        std::vector<z3::expr> constraints;
        const std::size_t num_outputs = ensemble.num_leaf_values();
        constraints.reserve(num_outputs);
        //
        const z3::expr& output_var = vars.output_label_to_z3_expr(output_label);
        OutputIndex_type output_index = 0;
        for (; output_index < output_label; ++output_index) {
            constraints.push_back(output_var > vars.output_label_to_z3_expr(output_index));
        }
        for (++output_index; output_index < num_outputs; ++output_index) {
            constraints.push_back(output_var >= vars.output_label_to_z3_expr(output_index));
        }
        //
        return Z3_IN_PLAJA::to_conjunction(constraints);
#endif
    }

    std::vector<z3::expr> generate_output_interfaces(const veritas::AddTree& ensemble, const Z3_IN_PLAJA::VeritasToZ3Vars& vars) {
        // Generate output comparison condition for each action being selected
        std::vector<z3::expr> output_interface_per_label;
        const auto num_outputs = ensemble.num_leaf_values();
        output_interface_per_label.reserve(num_outputs);
        for (OutputIndex_type output_label = 0; output_label < num_outputs; ++output_label) {
            output_interface_per_label.push_back(generate_output_interface(output_label, ensemble, vars));
        }
        return output_interface_per_label;
    }

    z3::expr_vector generate_conjunction_of_equations(const VERITAS_IN_PLAJA::VeritasQuery& query, const Z3_IN_PLAJA::VeritasToZ3Vars& vars) {
        z3::expr_vector constraints(vars.get_context_z3());
        const auto& equations = query.getEquations();
        for (const auto& equation: equations) { constraints.push_back(VERITAS_TO_Z3::generate_equation(equation, vars)); }
        //
        return constraints;
    }

    z3::expr split_to_z3(const veritas::LtSplit split, const Z3_IN_PLAJA::VeritasToZ3Vars& vars, bool left, veritas::FloatT adjust = 0) {
        z3::expr scalar = Z3_IN_PLAJA::z3_to_real(vars.get_context_z3(), split.split_value + adjust, Z3_IN_PLAJA::floatingPrecision);
        z3::expr feature = vars.to_z3_expr(split.feat_id);
        z3::expr true_split = feature < scalar;
        z3::expr false_split = !(true_split);
        z3::expr ret = left ? true_split : false_split;
        // std::cout << split << (left ? " is proper: " : " after negation: ") << ret << std::endl;
        return left ? true_split : false_split;
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

    z3::expr tree_path_to_z3(std::pair<std::vector<std::pair<veritas::LtSplit, bool>>, veritas::FloatT> split, const Z3_IN_PLAJA::VeritasToZ3Vars& vars, int label, int tree_index, veritas::FloatT adjust = 0) {
        z3::expr tree_var = vars.to_z3_expr(label, tree_index);
        z3::expr leaf_value = Z3_IN_PLAJA::z3_to_real(vars.get_context_z3(), split.second, Z3_IN_PLAJA::floatingPrecision);
        z3::expr constraint = tree_var == leaf_value;
        for (auto s : split.first) {
            constraint = constraint && (split_to_z3(s.first, vars, s.second, adjust));
        }
        return constraint;
    }

    z3::expr tree_paths_to_z3(std::vector<std::pair<std::vector<std::pair<veritas::LtSplit, bool>>, veritas::FloatT>> splits, const Z3_IN_PLAJA::VeritasToZ3Vars& vars, int label, int tree_index, veritas::FloatT adjust = 0) {
        z3::expr disjuncts = tree_path_to_z3(splits[0], vars, label, tree_index, adjust);
        for (auto i = 1; i < splits.size(); ++i) {
            disjuncts = disjuncts || tree_path_to_z3(splits[i], vars, label, tree_index, adjust);
        }        
        return disjuncts;
    }

    z3::expr_vector generate_tree_paths(const veritas::AddTree trees, const Z3_IN_PLAJA::VeritasToZ3Vars& vars) {
        z3::expr_vector overall_expressions(vars.get_context_z3());
        if (trees.get_type() == veritas::AddTreeType::CLF_SOFTMAX) {
            auto num_trees = trees.size()/trees.num_leaf_values();
            for (int i = 0; i < trees.size(); ++i) {
                auto label = i/num_trees;
                auto tree_index = i - label*num_trees;
                auto tree = trees[i];
                auto tree_paths = get_splits_for_tree(tree, label);
                auto expressions = tree_paths_to_z3(tree_paths, vars, label, tree_index);
                overall_expressions.push_back(expressions);
            }
            return overall_expressions;
        }
        
        auto num_trees = trees.size();
        for (int label = 0; label < trees.num_leaf_values(); ++label) {
            for (int t = 0; t < num_trees; ++t) {
                auto tree = trees[t];
                auto tree_paths = get_splits_for_tree(tree, label);
                auto expressions = tree_paths_to_z3(tree_paths, vars, label, t);
                overall_expressions.push_back(expressions);
            }
        }
        return overall_expressions;
    }

    z3::expr_vector calculate_action_label_values_in_z3(const veritas::AddTree trees, const Z3_IN_PLAJA::VeritasToZ3Vars& vars) {
        z3::expr_vector overall_expressions(vars.get_context_z3());
        auto num_labels = trees.num_leaf_values();
        auto num_trees = (trees.get_type() == veritas::AddTreeType::CLF_SOFTMAX) ? trees.size()/num_labels : trees.size();

        for (auto label = 0; label < num_labels; ++label) {
            z3::expr sum = Z3_IN_PLAJA::z3_to_real(vars.get_context_z3(), 0, Z3_IN_PLAJA::floatingPrecision);
            for (auto tree_index = 0; tree_index < num_trees; ++tree_index) {
                sum = sum + vars.to_z3_expr(label, tree_index);
            }
            z3::expr constraint = (sum == vars.output_label_to_z3_expr(label));
            // std::cout << constraint << std::endl;
            overall_expressions.push_back(constraint);
        }
        return overall_expressions;
    }

    z3::expr_vector generate_ensemble_forwarding_structures(const veritas::AddTree trees, const Z3_IN_PLAJA::VeritasToZ3Vars& vars) {
        z3::expr_vector equations = generate_tree_paths(trees, vars); // Get tree ensemble propagation equations
        
        z3::expr_vector outputs = calculate_action_label_values_in_z3(trees, vars); // Action label calculation
        for (auto e : outputs) {
            equations.push_back(e);
        }
        return equations;
    }

}
