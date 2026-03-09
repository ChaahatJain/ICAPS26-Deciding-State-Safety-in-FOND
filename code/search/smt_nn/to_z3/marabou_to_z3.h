//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_MARABOU_TO_Z3_H
#define PLAJA_MARABOU_TO_Z3_H

#include "../../../include/marabou_include/forward_marabou.h"
#include "../../information/jani2nnet/forward_jani_2_nnet.h"
#include "../../information/jani2nnet/using_jani2nnet.h"
#include "../../smt/context_z3.h"
#include "../utils/preprocessed_bounds.h"
#include "../forward_smt_nn.h"
#include "../marabou_query.h"
#include "encoding_information.h"
#include "nn_vars_in_z3.h"

namespace MARABOU_TO_Z3 {

    // bounds:

    inline z3::expr generate_lower_bound(const z3::expr& var, MarabouFloating_type lower_bound) {
        PLAJA_ASSERT(FloatUtils::isFinite(lower_bound))
        if (var.is_int()) { return Z3_IN_PLAJA::z3_to_int(var.ctx(), PLAJA_UTILS::cast_numeric<PLAJA::integer>(std::floor(lower_bound))) <= var; }
        else {
            PLAJA_ASSERT(var.is_real())
            return Z3_IN_PLAJA::z3_to_real(var.ctx(), std::floor(lower_bound), Z3_IN_PLAJA::floatingPrecision) <= var;
        }
    }

    inline z3::expr generate_upper_bound(const z3::expr& var, MarabouFloating_type upper_bound) {
        PLAJA_ASSERT(FloatUtils::isFinite(upper_bound))
        if (var.is_int()) { return var <= Z3_IN_PLAJA::z3_to_int(var.ctx(), PLAJA_UTILS::cast_numeric<PLAJA::integer>(std::ceil(upper_bound))) <= var; }
        else {
            PLAJA_ASSERT(var.is_real())
            return var <= Z3_IN_PLAJA::z3_to_real(var.ctx(), std::ceil(upper_bound), Z3_IN_PLAJA::floatingPrecision);
        }
    }

    inline z3::expr generate_bounds(const z3::expr& var, MarabouFloating_type lower_bound, MarabouFloating_type upper_bound) { return generate_lower_bound(var, lower_bound) && generate_upper_bound(var, upper_bound); } // NOLINT(bugprone-easily-swappable-parameters)

    template<typename Solver_t>
    inline void add_bounds(Solver_t& solver, const z3::expr& var, MarabouFloating_type lower_bound, MarabouFloating_type upper_bound) {
        if (FloatUtils::isFinite(lower_bound)) { solver.add(generate_lower_bound(var, lower_bound)); }
        if (FloatUtils::isFinite(upper_bound)) { solver.add(generate_upper_bound(var, upper_bound)); }
    }

    template<typename Solver_t>
    inline void add_bounds(Solver_t& solver, const Z3_IN_PLAJA::MarabouToZ3Vars& variables, const MARABOU_IN_PLAJA::PreprocessedBounds& preprocessed_bounds) {
        for (const auto& [var_index, var_z3]: variables) {
            MARABOU_TO_Z3::add_bounds(solver, var_z3, preprocessed_bounds.get_lower_bound(var_index), preprocessed_bounds.get_upper_bound(var_index));
        }
    }

    // linear constraints:

    extern z3::expr generate_equation(const Equation& equation, const Z3_IN_PLAJA::MarabouToZ3Vars& variables);

    inline z3::expr generate_bound(const Tightening& bound, const Z3_IN_PLAJA::MarabouToZ3Vars& variables) {
        switch (bound._type) {
            case Tightening::LB: return variables.to_z3_expr(bound._variable) <= Z3_IN_PLAJA::z3_to_real(variables.get_context_z3(), bound._value, Z3_IN_PLAJA::floatingPrecision);
            case Tightening::UB: return variables.to_z3_expr(bound._variable) >= Z3_IN_PLAJA::z3_to_real(variables.get_context_z3(), bound._value, Z3_IN_PLAJA::floatingPrecision);
        }
        PLAJA_ABORT
    }

    extern z3::expr generate_case_split(const PiecewiseLinearCaseSplit& case_split, const Z3_IN_PLAJA::MarabouToZ3Vars& variables);

    extern z3::expr generate_disjunction(const DisjunctionConstraint& disjunction, const Z3_IN_PLAJA::MarabouToZ3Vars& variables);

    // activation constraints:

    template<bool is_fixed, bool is_mip>
    inline void add_relu_output_bounds(z3::expr& relu_constraint, const z3::expr& output_var, MarabouFloating_type lower_bound, MarabouFloating_type upper_bound) {
        PLAJA_ASSERT(not FloatUtils::lte(upper_bound, 0) or is_fixed)
        if (is_fixed and FloatUtils::lte(upper_bound, 0)) {
            // Fixed inactive case: we already have "output = 0",
            // important: we always try to fix the inactive case first, i.e., in close-to-0 cases the preference goes to the "bound-explicit" inactive case.
            return;
        }
        // lower bound
        if (FloatUtils::gte(lower_bound, 0)) { relu_constraint = relu_constraint && generate_lower_bound(output_var, lower_bound); }
        else if constexpr (not is_mip) { relu_constraint = relu_constraint && generate_lower_bound(output_var, 0); } // lower bound 0 is explicitly contained in MIP encoding
        // upper bound
        PLAJA_ASSERT(FloatUtils::gt(upper_bound, 0)) // above we break if upper_bound <= 0
        relu_constraint = relu_constraint && generate_upper_bound(output_var, upper_bound);
    }

    template<bool is_fixed, bool is_mip>
    inline void add_bounds_to_relu(z3::expr& relu_constraint, const z3::expr& output_var, const z3::expr& input, MarabouFloating_type lower_bound, MarabouFloating_type upper_bound, const EncodingInformation& encoding_info) {
        switch (encoding_info._bound_encoding()) {
            case EncodingInformation::BoundEncoding::ALL: {
                relu_constraint = relu_constraint && generate_bounds(input, lower_bound, upper_bound);
                add_relu_output_bounds<is_fixed, is_mip>(relu_constraint, output_var, lower_bound, upper_bound);
                break;
            }
            case EncodingInformation::BoundEncoding::SKIP_FIXED_RELU_INPUTS: {
                if (not is_fixed) { relu_constraint = relu_constraint && generate_bounds(input, lower_bound, upper_bound); }
                add_relu_output_bounds<is_fixed, is_mip>(relu_constraint, output_var, lower_bound, upper_bound);
                break;
            }
            case EncodingInformation::BoundEncoding::ONLY_RELU_OUTPUTS: {
                add_relu_output_bounds<is_fixed, is_mip>(relu_constraint, output_var, lower_bound, upper_bound);
                break;
            }
            case EncodingInformation::BoundEncoding::NONE: { break; }
        }
    }

    inline z3::expr try_to_fix_relu(const z3::expr& output_var, const z3::expr& input, MarabouFloating_type lower_bound, MarabouFloating_type upper_bound) {
        PLAJA_ASSERT(FloatUtils::isFinite(lower_bound) && FloatUtils::isFinite(upper_bound))
        // inactive case (checked first on purpose):
        if (FloatUtils::lte(upper_bound, 0)) { return output_var == 0; }
        // active case:
        if (FloatUtils::gte(lower_bound, 0)) { return output_var == input; }
        // unsuccessful:
        return { output_var.ctx() };
    }

    inline z3::expr generate_relu_as_ite(const z3::expr& output_var, const z3::expr& input_var) {
        // straight forward SMT-encoding via if-then-else (here: syntactic sugar z3::max)
#if 0
        return output_var == z3::max(output_var.ctx().int_val(0), input_var);
#else
        return output_var == z3::max(input_var, output_var.ctx().int_val(0)); // seems to yield better performance
#endif
    }

    inline z3::expr generate_relu_as_ite(const ReluConstraint& relu_constraint, const Z3_IN_PLAJA::MarabouToZ3Vars& variables) {
        return generate_relu_as_ite(variables.to_z3_expr(relu_constraint.getF()), variables.to_z3_expr(relu_constraint.getB()));
    }

    inline z3::expr fix_or_generate_relu_as_ite(const ReluConstraint& relu_constraint, const Z3_IN_PLAJA::MarabouToZ3Vars& variables, const EncodingInformation& encoding_info) {
        const auto preprocessed_bounds = encoding_info._preprocessed_bounds();
        PLAJA_ASSERT(preprocessed_bounds)
        const auto& output_var = variables.to_z3_expr(relu_constraint.getF());
        const auto& input = variables.to_z3_expr(relu_constraint.getB());
        const auto lower_bound = preprocessed_bounds->get_lower_bound(relu_constraint.getB());
        const auto upper_bound = preprocessed_bounds->get_upper_bound(relu_constraint.getB());
        // try to fix
        auto fixed_constraint = try_to_fix_relu(output_var, input, lower_bound, upper_bound);
        if (Z3_IN_PLAJA::expr_is(fixed_constraint)) {
            add_bounds_to_relu<true, false>(fixed_constraint, output_var, input, lower_bound, upper_bound, encoding_info);
            return fixed_constraint;
        }
        // standard ite
        auto ite_constraint = generate_relu_as_ite(output_var, input);
        add_bounds_to_relu<false, false>(ite_constraint, output_var, input, lower_bound, upper_bound, encoding_info);
        return ite_constraint;
    }

    inline z3::expr generate_relu_as_mip_encoding(const z3::expr& output_var, const z3::expr& input_var, MarabouFloating_type lower_bound, MarabouFloating_type upper_bound, const Z3_IN_PLAJA::MarabouToZ3Vars& variables) {
        PLAJA_ASSERT(FloatUtils::isFinite(lower_bound) && FloatUtils::isFinite(upper_bound))
        // bounded MIP encoding (Valera et al., Scaling Guarantees for Nearest Counterfactual Explanations, 2020):
        const auto& binary_var = variables.to_z3_expr(variables.add_int_aux_var());
        auto& c = variables.get_context_z3();
        return (0 <= output_var) // redundant *if* encoding bounds: already enforced by variable bounds
               && (input_var <= output_var) // redundant *if* encoding bounds: implicitly enforced by derived bounds of both variables
               && (output_var <= Z3_IN_PLAJA::z3_to_real(c, upper_bound, Z3_IN_PLAJA::floatingPrecision) * binary_var)
               && (output_var <= (input_var - Z3_IN_PLAJA::z3_to_real(c, lower_bound, Z3_IN_PLAJA::floatingPrecision) * (1 - binary_var)))
               && c.int_val(0) <= binary_var and binary_var <= c.int_val(1);
    }

    inline z3::expr generate_relu_as_mip_encoding(const ReluConstraint& relu_constraint, const Z3_IN_PLAJA::MarabouToZ3Vars& variables, const MARABOU_IN_PLAJA::MarabouQuery& query) {
        const auto& output_var = variables.to_z3_expr(relu_constraint.getF());
        const auto& input_var = variables.to_z3_expr(relu_constraint.getB());
        const auto lower_bound = query.get_lower_bound(relu_constraint.getB());
        const auto upper_bound = query.get_upper_bound(relu_constraint.getB());
        //
        return generate_relu_as_mip_encoding(output_var, input_var, lower_bound, upper_bound, variables);
    }

    inline z3::expr fix_or_generate_relu_as_mip_encoding(const ReluConstraint& relu_constraint, const Z3_IN_PLAJA::MarabouToZ3Vars& variables, const EncodingInformation& encoding_info) {
        const auto preprocessed_bounds = encoding_info._preprocessed_bounds();
        PLAJA_ASSERT(preprocessed_bounds)
        const auto& output_var = variables.to_z3_expr(relu_constraint.getF());
        const auto& input_var = variables.to_z3_expr(relu_constraint.getB());
        const auto lower_bound = preprocessed_bounds->get_lower_bound(relu_constraint.getB());
        const auto upper_bound = preprocessed_bounds->get_upper_bound(relu_constraint.getB());
        // try to fix
        auto fixed_constraint = try_to_fix_relu(output_var, input_var, lower_bound, upper_bound);
        if (Z3_IN_PLAJA::expr_is(fixed_constraint)) {
            add_bounds_to_relu<true, false>(fixed_constraint, output_var, input_var, lower_bound, upper_bound, encoding_info);
            return fixed_constraint;
        }
        //
        auto mip_constraint = generate_relu_as_mip_encoding(output_var, input_var, lower_bound, upper_bound, variables);
        add_bounds_to_relu<false, true>(mip_constraint, output_var, input_var, lower_bound, upper_bound, encoding_info);
        return mip_constraint;
    }

    // NN (sub-) structures:
    extern z3::expr generate_output_interface(OutputIndex_type output_label, const NNInMarabou& nn, const Z3_IN_PLAJA::MarabouToZ3Vars& vars);
    extern std::vector<z3::expr> generate_output_interfaces(const NNInMarabou& nn, const Z3_IN_PLAJA::MarabouToZ3Vars& vars);

    extern z3::expr_vector generate_conjunction_of_equations(const MARABOU_IN_PLAJA::MarabouQuery& query, const Z3_IN_PLAJA::MarabouToZ3Vars& vars);
    extern z3::expr_vector generate_conjunction_of_relu_constraints(const MARABOU_IN_PLAJA::MarabouQuery& query, const Z3_IN_PLAJA::MarabouToZ3Vars& vars, const EncodingInformation& encoding_info);
    extern z3::expr_vector generate_nn_forwarding_structures(const MARABOU_IN_PLAJA::MarabouQuery& query, const Z3_IN_PLAJA::MarabouToZ3Vars& vars, const EncodingInformation& encoding_info);

}

#endif //PLAJA_MARABOU_TO_Z3_H
