//
// Created by Daniel Sherbakov on 2024.
//

#include "wp_bias.h"

#include "../../../parser/ast/expression/constant_expression.h"

#include "../../../parser/ast/expression/special_cases/linear_expression.h"

#include "../../../search/smt/vars_in_z3.h"

#include "../../../parser/ast/assignment.h"
#include "../../../parser/visitor/variables_of.h"
#include "../../../globals.h"
#include "../../../search/smt/visitor/to_z3_visitor.h"
#include "../../successor_generation/action_op.h"
#include "../../successor_generation/successor_generator_c.h"
#include "bias_to_z3.h"
#include <algorithm>

/**
* idea: compute the weakest precondition bias for a given avoid condition and an action label,
* then compute produce distance function to each of the wps take min.
*/

z3::expr WeakestPreconditionBias::get_wp_bias(const Expression* avoid, const std::shared_ptr<Z3_IN_PLAJA::Context>& ctx, const std::shared_ptr<const ModelZ3>& model) {
    z3::expr guards_disjunction = ctx->operator()().bool_val(false);// g1 || g2 || ... || gn
    z3::expr wps_disjunction = ctx->operator()().bool_val(false);   // wp1 || wp2 || ... || wpn

    // iterate over all action labels and extract guards and updates per operator
    const auto& generator = model->get_successor_generator();
    for (auto it_action = generator.init_action_id_it(false); !it_action.end(); ++it_action) {// iterate over action labels
        const auto action_label = it_action.get_label();
        // const auto& action_name = PLAJA_GLOBAL::currentModel->get_action_name(it_action.get_id());

        z3::expr wp_per_action = ctx->operator()().bool_val(false);
        // iterate over operators for the action_label
        for (auto it_op = generator.init_action_it_static(action_label); !it_op.end(); ++it_op) {
            const ActionOp& action_op = it_op.operator*();
            auto wps_per_op = get_wps_of_operator(const_cast<ActionOp&>(action_op), avoid, ctx, model);

            if (wps_per_op.empty()) {
                continue;
            }

            // disjunct wps of all updates in an operator (disj over non-deterministic updates)
            z3::expr disjunction_wp = ctx->operator()().bool_val(false);
            for (const auto& wp: wps_per_op) {
                disjunction_wp = (disjunction_wp || wp).simplify();
            }

            auto guards_of_op = extract_guards(const_cast<ActionOp&>(action_op), ctx, model);

            // conjunct guards with wp of
            guards_disjunction = (guards_disjunction || guards_of_op).simplify();
            wps_disjunction = (wps_disjunction || disjunction_wp).simplify();
        }
    }
    wps_disjunction = wps_disjunction && guards_disjunction.simplify();
    auto distance_to_wp = BiasToZ3::distance_to_condition(wps_disjunction.simplify(), ctx).simplify();
    return distance_to_wp.simplify();
}

/**
    * Iterates over all updates 'u' of an operator 'o' and computes the wp. <br>
    * operators consist of updates (or single update if deterministic), <br>
    * which consist of assignments e.g., x' = x + 1, y' = y + 2 etc. <br>
    * we compute wp of each assignment with respect to the avoid condition.
    * @return a vector of wps per update per assignement.
    */
std::vector<z3::expr> WeakestPreconditionBias::get_wps_of_operator(ActionOp& action_op, const Expression* avoid, const std::shared_ptr<Z3_IN_PLAJA::Context>& ctx, const std::shared_ptr<const ModelZ3>& model) {
    std::vector<z3::expr> wps_per_update;
    for (auto it_upd = action_op.updateIterator(); !it_upd.end(); ++it_upd) {
        const auto& update = action_op.get_update(it_upd.update_index());

        // if update does not affect avoid condition, skip.
        if (!isAffecting(update, *avoid)) {
            continue;
        }

        // compute wp for each assignment in update and conjunct them.
        z3::expr wp_conjunction = ctx->operator()().bool_val(true);

        // reverse assignments to apply the sequence rule of weakest precondition.
        // https://en.wikipedia.org/wiki/Predicate_transformer_semantics#Sequence
        std::vector<const Assignment*> assignments;
        for (auto it_assignment = update.assignmentIterator(0); !it_assignment.end(); ++it_assignment) {
            assignments.push_back(it_assignment.operator()());
        }
        std::reverse(assignments.begin(), assignments.end());

        auto avoid_vars = VARIABLES_OF::vars_of(avoid, false);
        auto state_vars = model->get_state_vars();
        z3::expr intermidiate_wp = to_z3_condition(*avoid, state_vars);
        for (auto it_assignment: assignments) {
            auto updated_var = it_assignment->get_updated_var_id();
            if (!avoid_vars.count(updated_var)) { continue; }// skip unaffecting assignments
            const Expression* assignment = it_assignment->get_value();
            auto var_id = it_assignment->get_updated_var_id();
            intermidiate_wp = compute_wp(assignment, var_id, &intermidiate_wp, ctx, model);
        }
        wps_per_update.push_back(intermidiate_wp);
    }
    return wps_per_update;
}

/**
    * Extracts the guards from an action operator.
    * each operator has a conjunction of guards g := g1 && g2 && ... && gn
    * @return a conjunction of guards (g1 && g2 && ... && gn).
    */
z3::expr WeakestPreconditionBias::extract_guards(ActionOp& action_op, const std::shared_ptr<Z3_IN_PLAJA::Context>& ctx, const std::shared_ptr<const ModelZ3>& model) {
    z3::expr guards = ctx->operator()().bool_val(true);
    auto state_vars = model->get_state_vars();
    // accumulate guards
    for (auto it_guard = action_op.guardIterator(); !it_guard.end(); ++it_guard) {
        const Expression* guard = it_guard.operator()();
        auto z3_guard = to_z3_condition(*guard, state_vars);
        guards = guards && z3_guard;
    }
    return guards.simplify();
}

z3::expr WeakestPreconditionBias::compute_wp(const Expression* assignment, VariableID_type var_id, z3::expr* avoid, const std::shared_ptr<Z3_IN_PLAJA::Context>& ctx, const std::shared_ptr<const ModelZ3>& model) {
    z3::expr wp = ctx->operator()().int_val(0);// init wp expression in z3.

    // translate PLAJA::Expression to z3::expr.
    auto state_vars = model->get_state_vars();
    auto z3_update = to_z3_condition(*assignment, state_vars);

    // substitution initialization.
    z3::expr_vector from(ctx->operator()());
    z3::expr_vector to(ctx->operator()());

    // get updated variable.
    const z3::expr& var_to_substitute = ctx->get_var(var_id);
    from.push_back(var_to_substitute);
    to.push_back(z3_update);

    z3::expr sub = avoid->substitute(from, to);// x+1; x<5 -> x+1<5
    z3::expr rlt = sub.simplify();             // x+1<5 -> x<4

    return rlt;
}

/**
     * Checks if update affects variable in the given expression.
     * @return true if there exists an assignment in update that affects
     * a variable in expression.
     */
bool WeakestPreconditionBias::isAffecting(const Update& update, const Expression& expression) {
    auto vars = VARIABLES_OF::vars_of(&expression, false);
    for (auto it_assignment = update.assignmentIterator(0); !it_assignment.end(); ++it_assignment) {
        auto updated_var = it_assignment->get_updated_var_id();
        if (vars.count(updated_var)) { return true; }
    }
    return false;
}

BinaryOpExpression::BinaryOp WeakestPreconditionBias::get_op(const std::string& op) {
    switch (op[0]) {
        case '<':
            return (op.size() == 2 && op[1] == '=') ? BinaryOpExpression::LE : BinaryOpExpression::LT;
        case '>':
            return (op.size() == 2 && op[1] == '=') ? BinaryOpExpression::GE : BinaryOpExpression::GT;
        case '=':
            return BinaryOpExpression::EQ;
        default:
            throw std::runtime_error("Unsupported operator");
    }
}
