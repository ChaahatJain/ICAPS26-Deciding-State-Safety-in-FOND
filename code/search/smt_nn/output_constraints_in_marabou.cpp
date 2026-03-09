//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "output_constraints_in_marabou.h"
#include "model/model_marabou.h"
#include "successor_generation/successor_generator_marabou.h"
#include "constraints_in_marabou.h"
#include "marabou_query.h"
#include "nn_in_marabou.h"

#ifdef ENABLE_APPLICABILITY_FILTERING

#include "../../utils/utils.h"
#include "../information/jani2nnet/jani_2_nnet.h"
#include "../predicate_abstraction/nn/smt/action_op_marabou_pa.h"
#include "../smt/successor_generation/op_applicability.h"

#include "../../option_parser/option_parser.h"
#include "../../globals.h"

#endif

MARABOU_IN_PLAJA::OutputConstraints::OutputConstraints(const ModelMarabou& model_marabou):
    modelMarabou(&model_marabou) {
    PLAJA_ASSERT(model_marabou.get_interface())
}

MARABOU_IN_PLAJA::OutputConstraints::~OutputConstraints() = default;

#ifdef ENABLE_APPLICABILITY_FILTERING

std::unique_ptr<MARABOU_IN_PLAJA::OutputConstraints> MARABOU_IN_PLAJA::OutputConstraints::construct(const ModelMarabou& model_marabou) {
    PLAJA_ASSERT(model_marabou.get_interface())

    if (model_marabou.get_interface()->_do_applicability_filtering()) {
        return std::make_unique<MARABOU_IN_PLAJA::OutputConstraintsAppFilter>(model_marabou);
    } else {
        return std::make_unique<MARABOU_IN_PLAJA::OutputConstraintsNoFilter>(model_marabou);
    }
}

#else

std::unique_ptr<MARABOU_IN_PLAJA::OutputConstraintsNoFilter> MARABOU_IN_PLAJA::OutputConstraints::construct(const ModelMarabou& model_marabou) {
    return std::make_unique<MARABOU_IN_PLAJA::OutputConstraintsNoFilter>(model_marabou);
}

#endif

/* output_var > output_var' for var' < var and output_var >= output_var' for var' > var
 * relaxed to
 * output_var + epsilon >= output_var'
 * with epsilon tolerance due to floating arithmetic.
 */
inline Equation MARABOU_IN_PLAJA::OutputConstraints::compute_eq(MarabouVarIndex_type output_var, MarabouVarIndex_type other_output_var) {
    PLAJA_ASSERT(output_var != other_output_var)
    Equation eq(Equation::GE);
    eq.setScalar(-MARABOU_IN_PLAJA::argMaxPrecision);
    eq.addAddend(1, output_var);
    eq.addAddend(-1, other_output_var);
    return eq;
}

/* Inline default negation computation here. */
inline Equation MARABOU_IN_PLAJA::OutputConstraints::compute_negation_eq(MarabouVarIndex_type output_var, MarabouVarIndex_type other_output_var) {
    PLAJA_ASSERT(output_var != other_output_var)
    Equation eq(Equation::LE);
    eq.setScalar(-MARABOU_IN_PLAJA::argMaxPrecision - MARABOU_IN_PLAJA::strictnessPrecision);
    eq.addAddend(1, output_var);
    eq.addAddend(-1, other_output_var);
    return eq;
}

inline Equation MARABOU_IN_PLAJA::OutputConstraints::compute_negation_other_label_eq(MarabouVarIndex_type output_var, MarabouVarIndex_type other_output_var) {
    return compute_eq(other_output_var, output_var);
}

/* o - o' >= -eps can be encoded as
 * o - o' + aux = -eps with aux <= 0
 * respectively
 * o - o' + aux = 0 with aux <= eps.
 * In a similar fashion we can encode the negation
 * o - o' <= -eps - delta
 * as
 * o - o' + aux = 0 with aux >= eps + delta.
 * Furthermore, observe that "other label selection" corresponds to
 * o' - o >= -eps
 * which is equivalent to
 * - o' + o <= eps
 * rewritten
 * o - o' <= eps
 * which can be encoded as
 * o - o' + aux = 0 with aux >= eps.
 * For disjunctive encodings over action selection,
 * this enables us to use a single auxiliary variable (and equation) for o >= o' and o' >= o respectively
 * only moving the auxiliary variable tightening into the disjunction.
 */
Equation MARABOU_IN_PLAJA::OutputConstraints::compute_aux_eq(MarabouVarIndex_type output_var, MarabouVarIndex_type other_output_var, MarabouVarIndex_type aux_var) {
    PLAJA_ASSERT(output_var != other_output_var)
    PLAJA_ASSERT(output_var != aux_var)
    PLAJA_ASSERT(other_output_var != aux_var)
    Equation eq(Equation::EQ);
    eq.setScalar(0);
    eq.addAddend(1, output_var);
    eq.addAddend(-1, other_output_var);
    eq.addAddend(1, aux_var);
    return eq;
}

Tightening MARABOU_IN_PLAJA::OutputConstraints::compute_aux_var_tightening(MarabouVarIndex_type aux_var) {
    return { aux_var, argMaxPrecision, Tightening::UB };
}

Tightening MARABOU_IN_PLAJA::OutputConstraints::compute_aux_var_tightening_negation(MarabouVarIndex_type aux_var) {
    return { aux_var, argMaxPrecision + MARABOU_IN_PLAJA::strictnessPrecision, Tightening::LB };
}

Tightening MARABOU_IN_PLAJA::OutputConstraints::compute_aux_var_tightening_other(MarabouVarIndex_type aux_var) {
    return { aux_var, argMaxPrecision, Tightening::LB };
}

/**********************************************************************************************************************/


MARABOU_IN_PLAJA::OutputConstraintsNoFilter::OutputConstraintsNoFilter(const ModelMarabou& model_marabou):
    OutputConstraints(model_marabou) {

    // Cache constraints for step=0.

    PLAJA_ASSERT(modelMarabou->get_interface())

    auto output_layer_size = model_marabou.get_nn_in_marabou(0).get_output_layer_size();
    constraintsPerLabel.reserve(output_layer_size);
    exclusionConstraintPerLabel.reserve(output_layer_size);
    otherLabelConstraintPerLabel.reserve(output_layer_size);

    for (OutputIndex_type output_index = 0; output_index < output_layer_size; ++output_index) {
        constraintsPerLabel.push_back(compute_internal(output_index, 0));
        exclusionConstraintPerLabel.push_back(compute_negation_internal(output_index, 0));
        otherLabelConstraintPerLabel.push_back(compute_negation_other_label_internal(output_index, 0));
    }

}

MARABOU_IN_PLAJA::OutputConstraintsNoFilter::~OutputConstraintsNoFilter() = default;

std::unique_ptr<ConjunctionInMarabou> MARABOU_IN_PLAJA::OutputConstraintsNoFilter::compute_internal(OutputIndex_type output_index, StepIndex_type step) const { // NOLINT(*-easily-swappable-parameters)
    const auto& nn_in_marabou = modelMarabou->get_nn_in_marabou(step);
    const MarabouVarIndex_type output_var = nn_in_marabou.get_output_var(output_index);

    auto selection_constraint = std::make_unique<ConjunctionInMarabou>(nn_in_marabou._context());

    auto output_layer_size = nn_in_marabou.get_output_layer_size();
    for (OutputIndex_type neuron_index = 0; neuron_index < output_layer_size; ++neuron_index) {

        if (neuron_index == output_index) { continue; }

        selection_constraint->add_equation(compute_eq(output_var, nn_in_marabou.get_output_var(neuron_index)));

    }

    return selection_constraint;

}

std::unique_ptr<MarabouConstraint> MARABOU_IN_PLAJA::OutputConstraintsNoFilter::compute_negation_internal(OutputIndex_type output_index, StepIndex_type step) const { // NOLINT(*-easily-swappable-parameters)
    const auto& nn_in_marabou = modelMarabou->get_nn_in_marabou(step);
    const MarabouVarIndex_type output_var = nn_in_marabou.get_output_var(output_index);

    auto exclusion_constraint = std::make_unique<DisjunctionInMarabou>(nn_in_marabou._context());
    auto output_layer_size = nn_in_marabou.get_output_layer_size();
    exclusion_constraint->reserve_for_additional(output_layer_size - 1);

    for (OutputIndex_type neuron_index = 0; neuron_index < output_layer_size; ++neuron_index) {

        if (neuron_index == output_index) { continue; }

        PiecewiseLinearCaseSplit case_split;
        case_split.addEquation(compute_negation_eq(output_var, nn_in_marabou.get_output_var(neuron_index)));
        exclusion_constraint->add_case_split(std::move(case_split));

    }

    return exclusion_constraint;
}

std::unique_ptr<MarabouConstraint> MARABOU_IN_PLAJA::OutputConstraintsNoFilter::compute_negation_other_label_internal(OutputIndex_type output_index, StepIndex_type step) const { // NOLINT(*-easily-swappable-parameters)
    const auto& nn_in_marabou = modelMarabou->get_nn_in_marabou(step);
    const MarabouVarIndex_type output_var = nn_in_marabou.get_output_var(output_index);

    auto other_label_constraint = std::make_unique<DisjunctionInMarabou>(nn_in_marabou._context());
    auto output_layer_size = nn_in_marabou.get_output_layer_size();
    other_label_constraint->reserve_for_additional(output_layer_size - 1);

    for (OutputIndex_type neuron_index = 0; neuron_index < output_layer_size; ++neuron_index) {

        if (neuron_index == output_index) { continue; }

        PiecewiseLinearCaseSplit case_split;
        case_split.addEquation(compute_negation_other_label_eq(output_var, nn_in_marabou.get_output_var(neuron_index)));
        other_label_constraint->add_case_split(std::move(case_split));

    }

    return other_label_constraint;

}

std::unique_ptr<ConjunctionInMarabou> MARABOU_IN_PLAJA::OutputConstraintsNoFilter::compute(OutputIndex_type output_index, StepIndex_type step) const {
    return step > 0 ? compute_internal(output_index, step) : std::make_unique<ConjunctionInMarabou>(*constraintsPerLabel[output_index]);
}

std::unique_ptr<MarabouConstraint> MARABOU_IN_PLAJA::OutputConstraintsNoFilter::compute_negation(OutputIndex_type output_index, StepIndex_type step) const {
    return step > 0 ? compute_negation_internal(output_index, step) : exclusionConstraintPerLabel[output_index]->copy();
}

std::unique_ptr<MarabouConstraint> MARABOU_IN_PLAJA::OutputConstraintsNoFilter::compute_negation_other_label(OutputIndex_type output_index, StepIndex_type step) const {
    return step > 0 ? compute_negation_other_label_internal(output_index, step) : otherLabelConstraintPerLabel[output_index]->copy();
}

void MARABOU_IN_PLAJA::OutputConstraintsNoFilter::add(MARABOU_IN_PLAJA::QueryConstructable& query, OutputIndex_type output_index, StepIndex_type step) const {
    if (step > 0) {
        compute_internal(output_index, step)->move_to_query(query);
    } else {
        PLAJA_ASSERT(output_index < constraintsPerLabel.size())
        constraintsPerLabel[output_index]->add_to_query(query);
    }
}

void MARABOU_IN_PLAJA::OutputConstraintsNoFilter::add_negation(MARABOU_IN_PLAJA::QueryConstructable& query, OutputIndex_type output_index, StepIndex_type step) const {
    if (step > 0) {
        compute_negation_internal(output_index, step)->move_to_query(query);
    } else {
        PLAJA_ASSERT(output_index < exclusionConstraintPerLabel.size())
        exclusionConstraintPerLabel[output_index]->add_to_query(query);
    }
}

void MARABOU_IN_PLAJA::OutputConstraintsNoFilter::add_negation_other_label(MARABOU_IN_PLAJA::QueryConstructable& query, OutputIndex_type output_index, StepIndex_type step) const {
    if (step > 0) {
        compute_negation_other_label_internal(output_index, step)->move_to_query(query);
    } else {
        PLAJA_ASSERT(output_index < otherLabelConstraintPerLabel.size())
        otherLabelConstraintPerLabel[output_index]->add_to_query(query);
    }
}

/**********************************************************************************************************************/


#ifdef ENABLE_APPLICABILITY_FILTERING

// Applicability filtering: output_var >= output_var' or ( not g_1(var') and ... not g_n(var') )
// where g_i(var') is the guard of a respectively labeled operator.

MARABOU_IN_PLAJA::OutputConstraintsAppFilter::OutputConstraintsAppFilter(const ModelMarabou& model_marabou):
    OutputConstraints(model_marabou)
    , queryConstruct(nullptr)
    , opApp(nullptr) {
    PLAJA_ASSERT(&model_marabou.get_successor_generator())

#if 0
    // Cache constraints for step=0.
    const auto& nn_in_marabou = model_marabou.get_nn_in_marabou(0);
    const auto output_layer_size = nn_in_marabou.get_output_layer_size();

    // init constraint per output
    maxConstraintsPerLabelPair.resize(output_layer_size);
    maxConstraintsNegPerLabelPair.resize(output_layer_size);
    maxConstraintsOtherPerLabelPair.resize(output_layer_size);

    for (OutputIndex_type output_index = 0; output_index < output_layer_size; ++output_index) {
        const MarabouVarIndex_type output_var = nn_in_marabou.get_output_var(output_index);

        auto& max_constraints_per_label = maxConstraintsPerLabelPair[output_index];
        auto& max_constraints_neg_per_label = maxConstraintsNegPerLabelPair[output_index];
        auto& max_constraints_other_per_label = maxConstraintsOtherPerLabelPair[output_index];
        max_constraints_per_label.resize(output_layer_size);
        max_constraints_neg_per_label.resize(output_layer_size);
        max_constraints_other_per_label.resize(output_layer_size);

        for (OutputIndex_type neuron_index = 0; neuron_index < output_layer_size; ++neuron_index) {

            if (output_index == neuron_index) { continue; }

            const auto other_output_var = nn_in_marabou.get_output_var(neuron_index);

            max_constraints_per_label[neuron_index] = std::make_unique<PiecewiseLinearCaseSplit>();
            max_constraints_per_label[neuron_index]->addEquation(compute_eq(output_var, other_output_var));

            max_constraints_neg_per_label[neuron_index] = std::make_unique<Equation>(compute_negation_eq(output_var, other_output_var));

            max_constraints_other_per_label[neuron_index] = std::make_unique<Equation>(compute_negation_other_label_eq(output_var, other_output_var));

        }
    }
#endif

}

MARABOU_IN_PLAJA::OutputConstraintsAppFilter::~OutputConstraintsAppFilter() = default;

// TODO Can we optimize encoding on Marabou-level (e.g., given bounds of query, context?)

std::unique_ptr<ConjunctionInMarabou> MARABOU_IN_PLAJA::OutputConstraintsAppFilter::compute(OutputIndex_type output_index, StepIndex_type step) const {
    const auto* nn_interface = modelMarabou->get_interface();
    const auto do_locs = not modelMarabou->ignore_locs();

    PLAJA_ASSERT(output_index < nn_interface->get_num_output_features())

    // PLAJA_ASSERT(output_index < maxConstraintsPerLabelPair.size())
    // const auto* max_constraints_per_label = step == 0 ? &maxConstraintsPerLabelPair[output_index] : nullptr;
    const auto& nn_in_marabou = modelMarabou->get_nn_in_marabou(step);
    const MarabouVarIndex_type output_var = nn_in_marabou.get_output_var(output_index);
    const auto output_layer_size = nn_in_marabou.get_output_layer_size();

    auto& context = nn_in_marabou._context();

    const bool opt_slack_var = queryConstruct and context.do_opt_slack_for_label_sel();
    const bool do_slack_var = PLAJA_GLOBAL::marabouDisBaselineSupport ? not opt_slack_var and queryConstruct and context.do_slack_for_label_sel() : false;
    std::unique_ptr<ConjunctionInMarabou> selection_constraint(new ConjunctionInMarabou(context));

    // Add guard disjunction:

    if (not(opApp and opApp->known_self_applicability())) { modelMarabou->guards_to_marabou(nn_interface->get_output_label(output_index), step)->move_to_conjunction(*selection_constraint); }
    else { PLAJA_ASSERT(opApp->get_self_applicability()) }

    // Add output constraint:

    for (OutputIndex_type neuron_index = 0; neuron_index < output_layer_size; ++neuron_index) {
        if (output_index == neuron_index) { continue; }

        // Encode not g_1, ..., not g_n:
        std::list<std::unique_ptr<MarabouConstraint>> neg_guards;

        bool other_label_enabled = false; // Other label is always enabled.

        for (const auto* action_op: modelMarabou->get_action_structure(nn_interface->get_output_label(neuron_index))) {

            if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) {

                if (opApp) {

                    const auto app = opApp->get_applicability(action_op->_op_id());

                    if (app == OpApplicability::Infeasible) { continue; }

                    if (app == OpApplicability::Entailed) {
                        other_label_enabled = true;
                        break;
                    }

                }

            }

            if (queryConstruct and not context.do_per_op_app_dis() and context.do_pre_opt_dis()) { // Apply entailment early if not using per op disjunction.

                auto rlt = action_op->guard_negation_to_marabou(*queryConstruct, do_locs, step);

                if (not rlt.first) {

                    if (rlt.second) { continue; } // Guard negation is entailed.
                    else { // Guard negation is infeasible
                        other_label_enabled = true;
                        break;
                    }

                } else {
                    neg_guards.push_back(std::move(rlt.first));
                }

            } else {
                neg_guards.push_back(action_op->guard_negation_to_marabou(do_locs, step)); //->move_to_conjunction(neg_guard_conjunction);
            }

        }

        const auto other_output_var = nn_in_marabou.get_output_var(neuron_index);

        if (other_label_enabled) { // Special case other labeled is entailed to be enabled -> encode o >= o'.

            /* if (max_constraints_per_label) {
            PLAJA_ASSERT((*max_constraints_per_label)[neuron_index]->getEquations().size() == 1)
            selection_constraint->add_equation((*max_constraints_per_label)[neuron_index]->getEquations().front());
            } else { */
            selection_constraint->add_equation(compute_eq(output_var, other_output_var));
            // }

        } else {

            if (neg_guards.empty()) { continue; } // Special case other label is already inferred to be disabled.

            // Encode o >= o' or (not g'_1 and ... and not g'_n) as
            // (o >= o' or not g'_1) and ... and (o >= o' or not g'_1)
            // Top-disjunction encoding turns out to be not feasible since each "not g'_i" is usually a disjunction itself.
            // Hence, transformation to DNF results in tremendous blow-up (10^6 disjuncts on 4 Blocks).

            MarabouVarIndex_type aux_var = opt_slack_var ? queryConstruct->add_aux_var() : MARABOU_IN_PLAJA::noneVar;

            // TODO: Might check for equations with unbounded vars that do not occur in any other constraint on Marabou level and remove them during preprocessing.
            //  This covers the case where the aux-var-bound disjunct has been optimized out (e.g., due to entailment).
            if (opt_slack_var) { selection_constraint->add_equation(compute_aux_eq(output_var, other_output_var, aux_var)); }

            /**********************************************************************************************************/

            if (not context.do_per_op_app_dis()) {

                if (do_slack_var) {
                    PLAJA_ASSERT(not opt_slack_var)
                    PLAJA_ASSERT(aux_var == MARABOU_IN_PLAJA::noneVar)
                    aux_var = queryConstruct->add_aux_var();
                    selection_constraint->add_equation(compute_aux_eq(output_var, other_output_var, aux_var));
                }

                std::unique_ptr<ConjunctionInMarabou> neg_guard_conjunction(new ConjunctionInMarabou(context));

                for (auto& neg_guard: neg_guards) { neg_guard->move_to_conjunction(*neg_guard_conjunction); }

                DisjunctionInMarabou dis_over_ops(context);

                if (opt_slack_var or do_slack_var) {

                    PiecewiseLinearCaseSplit case_split;
                    case_split.storeBoundTightening(compute_aux_var_tightening(aux_var));
                    dis_over_ops.add_case_split(std::move(case_split));

                } else {

                    PiecewiseLinearCaseSplit case_split;
                    case_split.addEquation(compute_eq(output_var, other_output_var));
                    dis_over_ops.add_case_split(std::move(case_split));

                }

                neg_guard_conjunction->move_to_disjunction(dis_over_ops);

                // TODO Might instead check for conflicting bounds on conjunction level (i.e., prior to adding negated guard conjunction to disjunction.)
                if (dis_over_ops.get_disjuncts().size() == 1) {
                    selection_constraint->add_case_split(dis_over_ops.get_disjuncts().first());
                } else {
                    dis_over_ops.move_to_conjunction(*selection_constraint);
                }

                continue;
            }

            /**********************************************************************************************************/

            for (auto& neg_guard: neg_guards) {

                if (do_slack_var) {
                    PLAJA_ASSERT(not opt_slack_var)
                    aux_var = queryConstruct->add_aux_var();
                    selection_constraint->add_equation(compute_aux_eq(output_var, other_output_var, aux_var));
                }

                DisjunctionInMarabou dis_per_op(context);

                if (opt_slack_var or do_slack_var) {

                    PiecewiseLinearCaseSplit case_split;
                    case_split.storeBoundTightening(compute_aux_var_tightening(aux_var));
                    dis_per_op.add_case_split(std::move(case_split));

                } else {

                    /* if (max_constraints_per_label) { dis_per_op.add_case_split(*((*max_constraints_per_label)[neuron_index])); } else { */
                    PiecewiseLinearCaseSplit case_split;
                    case_split.addEquation(compute_eq(output_var, other_output_var));
                    dis_per_op.add_case_split(std::move(case_split));
                    /* } */

                }

                neg_guard->move_to_disjunction(dis_per_op);

                dis_per_op.move_to_conjunction(*selection_constraint);

            }

        }

    }

    return selection_constraint;
}

std::unique_ptr<MarabouConstraint> MARABOU_IN_PLAJA::OutputConstraintsAppFilter::compute_negation(OutputIndex_type output_index, StepIndex_type step) const {
    const auto* nn_interface = modelMarabou->get_interface();
    const auto do_locs = not modelMarabou->ignore_locs();

    PLAJA_ASSERT(output_index < nn_interface->get_num_output_features())

    const auto& nn_in_marabou = modelMarabou->get_nn_in_marabou(step);
    const MarabouVarIndex_type output_var = nn_in_marabou.get_output_var(output_index);
    const auto output_layer_size = nn_in_marabou.get_output_layer_size();

    auto& context = nn_in_marabou._context();

    const bool introduce_slack_vars = queryConstruct and context.do_opt_slack_for_label_sel();
    std::unique_ptr<ConjunctionInMarabou> exclusion_conjunction(introduce_slack_vars ? new ConjunctionInMarabou(context) : nullptr); // Needed for aux vars
    std::unique_ptr<DisjunctionInMarabou> exclusion_disjunction(new DisjunctionInMarabou(context));
    // not (o >= o' or (not g'_1 and ... and not g'_n)) or ... or not (...)
    // - >
    // (o <= o' and (g'_1 or ... or g'_n)) or ... or (...)
    // ->
    // (o <= o' and g'1) or ... (o <= o' and g'n) or ... or ()

    for (OutputIndex_type neuron_index = 0; neuron_index < output_layer_size; ++neuron_index) {

        if (output_index == neuron_index) { continue; }

        const auto other_output_var = nn_in_marabou.get_output_var(neuron_index);

        bool other_label_enabled = false; // Other label is always enabled.

        // Encode g_1, ..., not g_n:
        std::list<std::unique_ptr<MarabouConstraint>> guards;

        for (const auto* action_op: modelMarabou->get_action_structure(nn_interface->get_output_label(neuron_index))) {

            if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) {

                if (opApp) {

                    const auto app = opApp->get_applicability(action_op->_op_id());

                    if (app == OpApplicability::Infeasible) { continue; }

                    if (app == OpApplicability::Entailed) {
                        other_label_enabled = true;
                        break;
                    }

                }

            }

            guards.push_back(action_op->guard_to_marabou(do_locs, step));

        }

        if (other_label_enabled) { // Special case other labeled is entailed to be enabled -> encode o <= o'.
            PiecewiseLinearCaseSplit case_split;
            case_split.addEquation(compute_negation_eq(output_var, other_output_var));
            exclusion_disjunction->add_case_split(std::move(case_split));
            continue;
        }

        if (guards.empty()) { continue; } // Special case other label is already inferred to be disabled.

        // Encode (o <= o' and g'1):

        const MarabouVarIndex_type aux_var = introduce_slack_vars ? queryConstruct->add_aux_var() : MARABOU_IN_PLAJA::noneVar;
        if (introduce_slack_vars) { exclusion_conjunction->add_equation(compute_aux_eq(output_var, other_output_var, aux_var)); } // Add to external conjunction

        for (auto& guard: guards) {

            ConjunctionInMarabou con_label_guard_pair(context);

            if (introduce_slack_vars) { con_label_guard_pair.add_bound(compute_aux_var_tightening_negation(aux_var)); }
            else { con_label_guard_pair.add_equation(compute_negation_eq(output_var, other_output_var)); }

            guard->move_to_conjunction(con_label_guard_pair);

            con_label_guard_pair.move_to_disjunction(*exclusion_disjunction);

        }

    }

    PLAJA_ASSERT_EXPECT(modelMarabou->get_op_applicability()->empty())

    // Or label is not applicable:
    // TODO is this feasible in the cases of interest?
    if (not(opApp and opApp->known_self_applicability())) { modelMarabou->guards_to_marabou(nn_interface->get_output_label(output_index), step)->move_to_negation()->move_to_disjunction(*exclusion_disjunction); }
    else { PLAJA_ASSERT(opApp->get_self_applicability()) } // case where applicability is false, is not of interest

    if (introduce_slack_vars) {
        exclusion_disjunction->move_to_conjunction(*exclusion_conjunction);
        return exclusion_conjunction;
    }

    return exclusion_disjunction;

}

std::unique_ptr<MarabouConstraint> MARABOU_IN_PLAJA::OutputConstraintsAppFilter::compute_negation_other_label(OutputIndex_type output_index, StepIndex_type step) const {
    const auto* nn_interface = modelMarabou->get_interface();
    const auto do_locs = not modelMarabou->ignore_locs();

    PLAJA_ASSERT(output_index < nn_interface->get_num_output_features())

    const auto& nn_in_marabou = modelMarabou->get_nn_in_marabou(step);
    const MarabouVarIndex_type output_var = nn_in_marabou.get_output_var(output_index);
    const auto output_layer_size = nn_in_marabou.get_output_layer_size();

    auto& context = nn_in_marabou._context();

    // Similar to negation
    // (o <= o' and (g'_1 or ... or g'_n)) or ... or (...)
    // where o <= o' is encoded with tolerance
    // output_var <= output_var' + epsilon.

    const bool introduce_slack_vars = queryConstruct and context.do_opt_slack_for_label_sel();
    std::unique_ptr<ConjunctionInMarabou> other_label_conjunction(new ConjunctionInMarabou(context));
    std::unique_ptr<DisjunctionInMarabou> other_label_disjunction(new DisjunctionInMarabou(nn_in_marabou._context()));

    for (OutputIndex_type neuron_index = 0; neuron_index < output_layer_size; ++neuron_index) {

        if (output_index == neuron_index) { continue; }

        const auto other_output_var = nn_in_marabou.get_output_var(neuron_index);

        bool other_label_enabled = false; // Other label is always enabled.

        // Encode g_1, ..., not g_n:
        std::list<std::unique_ptr<MarabouConstraint>> guards;

        for (const auto* action_op: modelMarabou->get_action_structure(nn_interface->get_output_label(neuron_index))) {

            if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) {

                if (opApp) {

                    const auto app = opApp->get_applicability(action_op->_op_id());

                    if (app == OpApplicability::Infeasible) { continue; }

                    if (app == OpApplicability::Entailed) {
                        other_label_enabled = true;
                        break;
                    }

                }

            }

            guards.push_back(action_op->guard_to_marabou(do_locs, step));

        }

        if (other_label_enabled) { // Special case other labeled is entailed to be enabled -> encode o <= o'.
            PiecewiseLinearCaseSplit case_split;
            case_split.addEquation(compute_negation_other_label_eq(output_var, nn_in_marabou.get_output_var(neuron_index)));
            other_label_disjunction->add_case_split(std::move(case_split));
        }

        // Encode (o <= o' and g'1):

        const MarabouVarIndex_type aux_var = introduce_slack_vars ? queryConstruct->add_aux_var() : MARABOU_IN_PLAJA::noneVar;
        if (introduce_slack_vars) { other_label_conjunction->add_equation(compute_aux_eq(other_output_var, output_var, aux_var)); } // Add to external conjunction

        for (auto& guard: guards) {

            ConjunctionInMarabou con_label_guard_pair(context);

            if (introduce_slack_vars) { con_label_guard_pair.add_bound(compute_aux_var_tightening(aux_var)); } // Do not use compute_aux_var_tightening_other!
            else { con_label_guard_pair.add_equation(compute_negation_other_label_eq(output_var, other_output_var)); }

            guard->move_to_conjunction(con_label_guard_pair);

            con_label_guard_pair.move_to_disjunction(*other_label_disjunction);

        }

    }

    // Other label selection constraint:
    // We are interested in the case where the label is applicable in the first.
    if (not(opApp and opApp->known_self_applicability())) { modelMarabou->guards_to_marabou(nn_interface->get_output_label(output_index), step)->move_to_conjunction(*other_label_conjunction); }
    else { PLAJA_ASSERT(opApp->get_self_applicability()) }

    other_label_disjunction->move_to_conjunction(*other_label_conjunction);

    return other_label_conjunction;

}

inline void MARABOU_IN_PLAJA::OutputConstraintsAppFilter::pre_optimization(MARABOU_IN_PLAJA::QueryConstructable* query) const {
    queryConstruct = query;
    if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) { opApp = modelMarabou->get_op_applicability(); }
    PLAJA_ASSERT(PLAJA_GLOBAL::enableApplicabilityCache or not opApp) // NOLINT(*-dcl03-c, *-static-assert)
}

inline void MARABOU_IN_PLAJA::OutputConstraintsAppFilter::post_optimization() const {
    if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) { opApp = nullptr; }
    queryConstruct = nullptr;
}

void MARABOU_IN_PLAJA::OutputConstraintsAppFilter::add(MARABOU_IN_PLAJA::QueryConstructable& query, OutputIndex_type output_index, StepIndex_type step) const {
    pre_optimization(&query);
    compute(output_index, step)->move_to_query(query);
    post_optimization();
}

void MARABOU_IN_PLAJA::OutputConstraintsAppFilter::add_negation(MARABOU_IN_PLAJA::QueryConstructable& query, OutputIndex_type output_index, StepIndex_type step) const {
    pre_optimization(&query);
    compute_negation(output_index, step)->move_to_query(query);
    post_optimization();
}

void MARABOU_IN_PLAJA::OutputConstraintsAppFilter::add_negation_other_label(MARABOU_IN_PLAJA::QueryConstructable& query, OutputIndex_type output_index, StepIndex_type step) const {
    pre_optimization(&query);
    compute_negation_other_label(output_index, step)->move_to_query(query);
    post_optimization();
}

#endif
