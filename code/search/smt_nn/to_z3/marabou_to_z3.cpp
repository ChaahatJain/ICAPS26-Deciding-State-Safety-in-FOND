//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <z3++.h>
#include "../../../include/marabou_include/disjunction_constraint.h"
#include "../../../include/marabou_include/input_query.h"
#include "../../../assertions.h"
#include "../../smt/visitor/extern/to_z3_visitor.h"
#include "../marabou_query.h"
#include "../nn_in_marabou.h"
#include "marabou_to_z3.h"

namespace MARABOU_TO_Z3 {

    z3::expr generate_equation(const Equation& equation, const Z3_IN_PLAJA::MarabouToZ3Vars& variables) {
        auto& context = variables.get_context_z3();
        const auto& addends = equation._addends;
        PLAJA_ASSERT(not addends.empty())

        /* Linear sum. */
        auto addends_it = addends.begin();
        const auto addends_end = addends.end();
        z3::expr eq_expr(Z3_IN_PLAJA::z3_to_real(context, addends_it->_coefficient, Z3_IN_PLAJA::floatingPrecision) * variables.to_z3_expr(addends_it->_variable));
        for (++addends_it; addends_it != addends_end; ++addends_it) {
            eq_expr = eq_expr + Z3_IN_PLAJA::z3_to_real(context, addends_it->_coefficient, Z3_IN_PLAJA::floatingPrecision) * variables.to_z3_expr(addends_it->_variable);
        }

        /* Comparison. */
        switch (equation._type) {
            case Equation::EQ: { return eq_expr == Z3_IN_PLAJA::z3_to_real(context, equation._scalar, Z3_IN_PLAJA::floatingPrecision); }
            case Equation::LE: { return eq_expr <= Z3_IN_PLAJA::z3_to_real(context, equation._scalar, Z3_IN_PLAJA::floatingPrecision); }
            case Equation::GE: { return eq_expr >= Z3_IN_PLAJA::z3_to_real(context, equation._scalar, Z3_IN_PLAJA::floatingPrecision); }
        }
        // in case of preprocessing:
        // PLAJA_ASSERT(equation._type == Equation::EQ)
        // return eq_expr == Z3_TO_REAL(context, equation._scalar);

        PLAJA_ABORT
    }

    z3::expr generate_case_split(const PiecewiseLinearCaseSplit& case_split, const Z3_IN_PLAJA::MarabouToZ3Vars& variables) {
        auto& context = variables.get_context_z3();
        const auto& equations = case_split.getEquations();
        const auto& bounds = case_split.getBoundTightenings();
        PLAJA_ASSERT(not equations.empty() or not bounds.empty())
        z3::expr conjunction(context);

        /* Equations. */
        if (not equations.empty()) {
            auto equations_it = equations.begin();
            const auto equations_end = equations.end();
            conjunction = generate_equation(*equations_it, variables);
            for (++equations_it; equations_it != equations_end; ++equations_it) { conjunction = conjunction && generate_equation(*equations_it, variables); }
        }

        /* Bounds. */
        if (not bounds.empty()) {
            auto bounds_it = bounds.begin();
            const auto bounds_end = bounds.end();
            if (conjunction) { conjunction = conjunction && generate_bound(*bounds_it, variables); }
            else { conjunction = generate_bound(*bounds_it, variables); }
            for (++bounds_it; bounds_it != bounds_end; ++bounds_it) { conjunction = conjunction && generate_bound(*bounds_it, variables); }
        }

        return conjunction;
    }

    extern z3::expr generate_disjunction(const DisjunctionConstraint& disjunction, const Z3_IN_PLAJA::MarabouToZ3Vars& variables) {
        const auto& case_splits = disjunction.getCaseSplits();
        PLAJA_ASSERT(not case_splits.empty())

        // Iterate over disjunctions. */
        auto case_splits_it = case_splits.begin();
        const auto case_splits_end = case_splits.end();
        z3::expr disjunction_z3 = MARABOU_TO_Z3::generate_case_split(*case_splits_it, variables);
        for (; case_splits_it != case_splits_end; case_splits_it++) { disjunction_z3 = disjunction_z3 || MARABOU_TO_Z3::generate_case_split(*case_splits_it, variables); }

        return disjunction_z3;
    }

    /* */

#if 0
    std::unique_ptr<NNVarsInZ3> generate_nn_variables(const MARABOU_IN_PLAJA::Context& context_m, z3::context& context_z3, const std::string& var_name_prefix) {
        std::unique_ptr<NNVarsInZ3> nn_vars(new NNVarsInZ3());
        for (auto it = context_m.variableIterator(); !it.end(); ++it) {
            if (it.is_integer()) { nn_vars->add(it.var(), context_z3.int_const((var_name_prefix + std::to_string(it.var())).c_str())); }
            else { nn_vars->add(it.var(), context_z3.real_const((var_name_prefix + std::to_string(it.var())).c_str())); }
            // is output?
            if (it.is_output()) { nn_vars->add_output(it.var()); }
        }
        return nn_vars;
    }

    std::unique_ptr<NNVarsInZ3> generate_nn_variables(const MARABOU_IN_PLAJA::MarabouQuery& query, z3::context& context, const std::string& var_name_prefix) {
        std::unique_ptr<NNVarsInZ3> nn_vars(new NNVarsInZ3());
        for (auto it = query.variableIterator(); !it.end(); ++it) {
            if (it.is_integer()) { nn_vars->add(it.var(), context.int_const((var_name_prefix + std::to_string(it.var())).c_str())); }
            else { nn_vars->add(it.var(), context.real_const((var_name_prefix + std::to_string(it.var())).c_str())); }
            // is output?
            if (it.is_output()) { nn_vars->add_output(it.var()); }
        }

        // binary vars
        auto l = query._query().getPiecewiseLinearConstraints().size();
        for (BinaryVarIndex binary_var = 0; binary_var < l; ++binary_var) {
            nn_vars->add_binary_var(context.int_const((Z3_IN_PLAJA::z3NNAuxVarPrefix + std::to_string(binary_var)).c_str()));
        }

        return nn_vars;
    }


    void match_state_vars_to_nn(NNVarsInZ3& nn_vars, const NNInMarabou& nn, const StateVarsInZ3& state_vars, const Jani2NNet& nn_interface) {
        PLAJA_ASSERT(not state_vars.empty())
        auto& context = nn_vars.get_context();

        // adapt input variables:
        auto num_inputs = nn_interface.get_num_input_features();
        PLAJA_ASSERT(num_inputs == nn.get_input_layer_size())
        for (InputIndex_type input_index = 0; input_index < num_inputs; ++input_index) {
            const auto* input_feature = nn_interface.get_input_feature(input_index);
            auto marabou_var = nn.get_input_var(input_index);
            if (input_feature->is_value_variable()) {
                nn_vars[marabou_var] = MARABOU_TO_Z3::bool_to_int_if(state_vars[input_feature->get_variable_id()]);
            } else if (input_feature->is_array_variable()) {
                z3::expr state_index = z3::select(state_vars[input_feature->get_variable_id()], context.int_val(input_feature->get_array_index()));
                nn_vars[marabou_var] = MARABOU_TO_Z3::bool_to_int_if(state_index);
            } else if (input_feature->is_location()) {
                PLAJA_ABORT // currentlty not expected
            }
            nn_vars.add_input_feature(input_feature->stateIndex, marabou_var, input_feature->is_location());
        }
    }
#endif

    z3::expr generate_output_interface(OutputIndex_type output_label, const NNInMarabou& nn, const Z3_IN_PLAJA::MarabouToZ3Vars& vars) {

        std::vector<z3::expr> constraints;
        const std::size_t num_outputs = nn.get_output_layer_size();
        constraints.reserve(num_outputs);

        const z3::expr& output_var = vars.to_z3_expr(nn.get_output_var(output_label));
        OutputIndex_type output_index = 0;
        for (; output_index < output_label; ++output_index) { constraints.push_back(output_var > vars.to_z3_expr(nn.get_output_var(output_index))); }
        for (++output_index; output_index < num_outputs; ++output_index) { constraints.push_back(output_var >= vars.to_z3_expr(nn.get_output_var(output_index))); }

        return Z3_IN_PLAJA::to_conjunction(constraints);

    }

    std::vector<z3::expr> generate_output_interfaces(const NNInMarabou& nn, const Z3_IN_PLAJA::MarabouToZ3Vars& vars) {
        std::vector<z3::expr> output_interface_per_label;
        const auto num_outputs = nn.get_output_layer_size();
        output_interface_per_label.reserve(num_outputs);
        for (OutputIndex_type output_label = 0; output_label < num_outputs; ++output_label) { output_interface_per_label.push_back(generate_output_interface(output_label, nn, vars)); }
        return output_interface_per_label;
    }

    z3::expr_vector generate_conjunction_of_equations(const MARABOU_IN_PLAJA::MarabouQuery& query, const Z3_IN_PLAJA::MarabouToZ3Vars& vars) {
        z3::expr_vector constraints(vars.get_context_z3());
        const auto& equations = query._query().getEquations();
        for (const auto& equation: equations) { constraints.push_back(MARABOU_TO_Z3::generate_equation(equation, vars)); }
        return constraints;
    }

    z3::expr_vector generate_conjunction_of_relu_constraints(const MARABOU_IN_PLAJA::MarabouQuery& query, const Z3_IN_PLAJA::MarabouToZ3Vars& vars, const EncodingInformation& encoding_info) {
        z3::expr_vector constraints(vars.get_context_z3());

        const auto& pl_constraints = query._query().getPiecewiseLinearConstraints();

        if (encoding_info._preprocessed_bounds()) {

            if (encoding_info._mip_encoding()) {
                for (const auto* pl_constraint: pl_constraints) { constraints.push_back(MARABOU_TO_Z3::fix_or_generate_relu_as_mip_encoding(*PLAJA_UTILS::cast_ptr<ReluConstraint>(pl_constraint), vars, encoding_info)); }
            } else {
                for (const auto* pl_constraint: pl_constraints) { constraints.push_back(MARABOU_TO_Z3::fix_or_generate_relu_as_ite(*PLAJA_UTILS::cast_ptr<ReluConstraint>(pl_constraint), vars, encoding_info)); }
            }

        } else {

            if (encoding_info._mip_encoding()) {
                for (const auto* pl_constraint: pl_constraints) { constraints.push_back(MARABOU_TO_Z3::generate_relu_as_mip_encoding(*PLAJA_UTILS::cast_ptr<ReluConstraint>(pl_constraint), vars, query)); }
            } else {
                for (const auto* pl_constraint: pl_constraints) { constraints.push_back(MARABOU_TO_Z3::generate_relu_as_ite(*PLAJA_UTILS::cast_ptr<ReluConstraint>(pl_constraint), vars)); }
            }

        }

        return constraints;
    }

    z3::expr_vector generate_nn_forwarding_structures(const MARABOU_IN_PLAJA::MarabouQuery& query, const Z3_IN_PLAJA::MarabouToZ3Vars& vars, const EncodingInformation& encoding_info) {
        z3::expr_vector constraints = generate_conjunction_of_equations(query, vars);
        for (const auto& constraint: generate_conjunction_of_relu_constraints(query, vars, encoding_info)) { constraints.push_back(constraint); }
        return constraints;
    }

}
