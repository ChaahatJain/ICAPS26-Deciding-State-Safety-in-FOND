//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <memory>
#include <unordered_map>
#include "constraints_in_marabou.h"
#include "../../include/marabou_include/disjunction_constraint.h"
#include "../../option_parser/option_parser.h"
#include "../../utils/floating_utils.h"
#include "../../utils/utils.h"
#include "../../globals.h"
#include "solver/smt_solver_marabou.h"
#include "marabou_context.h"

MarabouConstraint::MarabouConstraint(MARABOU_IN_PLAJA::Context& c):
    context(&c) {}

MarabouConstraint::~MarabouConstraint() = default;

/* */

bool MarabouConstraint::is_trivially_true() const { return false; }

bool MarabouConstraint::is_trivially_false() const { return false; }

#if 0
void MarabouConstraint::set_trivially_true() { PLAJA_ABORT }

void MarabouConstraint::set_trivially_false() { PLAJA_ABORT }
#endif

/* */

std::unique_ptr<MarabouConstraint> MarabouConstraint::to_substitute(const std::unordered_map<MarabouVarIndex_type, MarabouVarIndex_type>& mapping) const {
    auto constraint = copy();
    constraint->substitute(mapping);
    return constraint;
}

bool MarabouConstraint::is_bound() const { return false; }

bool MarabouConstraint::is_linear() const { return true; }

/* */

void MarabouConstraint::add_to_solver(PLAJA::SmtSolver& solver) const { add_to_query(PLAJA_UTILS::cast_ref<MARABOU_IN_PLAJA::SMTSolver>(solver)); }

void MarabouConstraint::add_negation_to_solver(PLAJA::SmtSolver& solver) const { compute_negation()->move_to_query(PLAJA_UTILS::cast_ref<MARABOU_IN_PLAJA::SMTSolver>(solver)); }

/**********************************************************************************************************************/
/**********************************************************************************************************************/

PredicateConstraint::PredicateConstraint(std::unique_ptr<MarabouConstraint>&& constraint_):
    constraint(std::move(constraint_))
    , negation(constraint->compute_negation()) {

}

PredicateConstraint::~PredicateConstraint() = default;

void PredicateConstraint::dump() const {
    PLAJA_LOG("Predicate Constraint")
    constraint->dump();
    PLAJA_LOG("Negation")
    negation->dump();
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/

EquationConstraint::EquationConstraint(MARABOU_IN_PLAJA::Context& c, Equation eq):
    MarabouConstraint(c)
    , equation(std::move(eq)) {}

EquationConstraint::~EquationConstraint() = default;

bool EquationConstraint::operator==(const EquationConstraint& other) const { return _context() == other._context() and equation.equivalent(other.equation); }

void EquationConstraint::add_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) const {
    PLAJA_ASSERT(_context() == query._context())
    query.add_equation(equation);
}

void EquationConstraint::move_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) {
    PLAJA_ASSERT(_context() == query._context())
    query.add_equation(std::move(equation));
}

void EquationConstraint::add_to_conjunction(ConjunctionInMarabou& conjunction) const {
    PLAJA_ASSERT(_context() == conjunction._context())
    conjunction.add_equation(equation);
}

void EquationConstraint::move_to_conjunction(ConjunctionInMarabou& conjunction) {
    PLAJA_ASSERT(_context() == conjunction._context())
    conjunction.add_equation(std::move(equation));
}

void EquationConstraint::add_to_disjunction(DisjunctionInMarabou& disjunction) const {
    PLAJA_ASSERT(_context() == disjunction._context())
    disjunction.add_case_split(equation);
}

void EquationConstraint::move_to_disjunction(DisjunctionInMarabou& disjunction) {
    PLAJA_ASSERT(_context() == disjunction._context())
    disjunction.add_case_split(std::move(equation));
}

std::unique_ptr<MarabouConstraint> EquationConstraint::copy() const { return std::make_unique<EquationConstraint>(*this); }

/**********************************************************************************************************************/

void EquationConstraint::substitute(Equation& eq, const std::unordered_map<MarabouVarIndex_type, MarabouVarIndex_type>& mapping) {
    for (auto it = eq._addends.begin(); it == eq._addends.end(); ++it) {
        auto it_sub = mapping.find(it->_variable);
        if (it_sub != mapping.end()) { it->_variable = it_sub->second; }
    }
}

void EquationConstraint::substitute(const std::unordered_map<MarabouVarIndex_type, MarabouVarIndex_type>& mapping) { substitute(equation, mapping); }

/**********************************************************************************************************************/

inline MarabouFloating_type EquationConstraint::compute_negation_offset(const MARABOU_IN_PLAJA::Context& c, const Equation& eq) {

    // is integer ?
    auto is_integer = PLAJA_FLOATS::is_integer(eq._scalar, MARABOU_IN_PLAJA::integerPrecision);
    //
    for (auto addend: eq._addends) {
        if (not is_integer) { break; }
        is_integer = c.is_marked_integer(addend._variable) and PLAJA_FLOATS::is_integer(addend._coefficient, MARABOU_IN_PLAJA::integerPrecision);
    }

    return is_integer ? 1 : MARABOU_IN_PLAJA::strictnessPrecision;
}

inline void EquationConstraint::negate_to_ge(Equation& eq, MarabouFloating_type scalar_offset) {
    PLAJA_ASSERT(eq._type != Equation::GE)

    // negation(x =< c) -> (x >= c + 1) for c integer
    eq.setType(Equation::GE);
    eq.setScalar(eq._scalar + scalar_offset);
}

inline void EquationConstraint::negate_to_le(Equation& eq, MarabouFloating_type scalar_offset) {
    PLAJA_ASSERT(eq._type != Equation::LE)

    // negation(x >= c) -> (x <= c - 1) for c integer
    eq.setType(Equation::LE);
    eq.setScalar(eq._scalar - scalar_offset);
}

inline void EquationConstraint::negate_non_eq(const MARABOU_IN_PLAJA::Context& c, Equation& eq) {
    PLAJA_ASSERT(eq._type != Equation::EQ)

    switch (eq._type) {

        case Equation::EQ: { PLAJA_ABORT }

        case Equation::GE: {
            negate_to_le(eq, compute_negation_offset(c, eq));
            break;
        }

        case Equation::LE: {
            negate_to_ge(eq, compute_negation_offset(c, eq));
            break;
        }

    }

}

inline std::unique_ptr<DisjunctionInMarabou> EquationConstraint::negate_eq(MARABOU_IN_PLAJA::Context& c, Equation eq) {
    PLAJA_ASSERT(eq._type == Equation::EQ)

    const auto scalar_offset = compute_negation_offset(c, eq);

    std::unique_ptr<DisjunctionInMarabou> negation(new DisjunctionInMarabou(c));
    auto& disjuncts_view = negation->disjuncts_std_view();

    // negation(x = c) -> x <= c - 1 or x >= c + 1

    // x <= c - 1
    negation->add_case_split(static_cast<const Equation&>(eq));
    negate_to_le(disjuncts_view.back()._equations.back(), scalar_offset);

    // x >= c + 1
    negation->add_case_split(std::move(eq));
    negate_to_ge(disjuncts_view.back()._equations.back(), scalar_offset);

    return negation;
}

/**********************************************************************************************************************/

std::unique_ptr<MarabouConstraint> EquationConstraint::compute_negation() const {

    const auto scalar_offset = compute_negation_offset(*context, equation);

    switch (equation._type) {

        case Equation::EQ: { return negate_eq(_context(), equation); }

        case Equation::GE: {

            std::unique_ptr<EquationConstraint> negation(new EquationConstraint(*this));
            negate_to_le(negation->equation, scalar_offset);
            return negation;

        }

        case Equation::LE: {

            std::unique_ptr<EquationConstraint> negation(new EquationConstraint(*this));
            negate_to_ge(negation->equation, scalar_offset);
            return negation;

        }

        default: PLAJA_ABORT

    }

    PLAJA_ABORT
}

std::unique_ptr<MarabouConstraint> EquationConstraint::move_to_negation() {

    switch (equation._type) {

        case Equation::EQ: { return negate_eq(_context(), std::move(equation)); }

        case Equation::GE: {
            negate_to_le(equation, compute_negation_offset(*context, equation));
            return std::make_unique<EquationConstraint>(std::move(*this));
        }

        case Equation::LE: {
            negate_to_ge(equation, compute_negation_offset(*context, equation));
            return std::make_unique<EquationConstraint>(std::move(*this));
        }

        default: PLAJA_ABORT

    }

    PLAJA_ABORT
}

/**********************************************************************************************************************/

std::unique_ptr<MarabouConstraint> EquationConstraint::optimize_for_query(const MARABOU_IN_PLAJA::QueryConstructable& query, DisjunctionConstraintBase::EntailmentValue& entailment_value) const {
    entailment_value = DisjunctionConstraintBase::checkEntailment(equation, query.get_input_query());

    switch (entailment_value) {

        case DisjunctionConstraintBase::Unknown: { return copy(); }
        case DisjunctionConstraintBase::Infeasible:
        case DisjunctionConstraintBase::Entailed: { return nullptr; }

        case DisjunctionConstraintBase::GeEntailed: {
            entailment_value = DisjunctionConstraintBase::Unknown;
            auto optimized_eq = std::make_unique<EquationConstraint>(*context, equation);
            optimized_eq->equation._type = Equation::LE;
            return optimized_eq;
        }

        case DisjunctionConstraintBase::LeEntailed: {
            entailment_value = DisjunctionConstraintBase::Unknown;
            auto optimized_eq = std::make_unique<EquationConstraint>(*context, equation);
            optimized_eq->equation._type = Equation::GE;
            return optimized_eq;
        }

    }

    PLAJA_ABORT
}

DisjunctionConstraintBase::EntailmentValue EquationConstraint::optimize_for_query(const MARABOU_IN_PLAJA::QueryConstructable& query, std::unique_ptr<MarabouConstraint>& /*optimized_constraint*/) {
    const auto entailment_value = DisjunctionConstraintBase::checkEntailment(equation, query.get_input_query());

    switch (entailment_value) {

        case DisjunctionConstraintBase::Unknown:
        case DisjunctionConstraintBase::Infeasible:
        case DisjunctionConstraintBase::Entailed: { return entailment_value; }

        case DisjunctionConstraintBase::GeEntailed: {
            equation._type = Equation::LE;
            return DisjunctionConstraintBase::Unknown;
        }

        case DisjunctionConstraintBase::LeEntailed: {
            equation._type = Equation::GE;
            return DisjunctionConstraintBase::Unknown;
        }

    }

    PLAJA_ABORT
}

void EquationConstraint::dump() const {
    PLAJA_LOG("EquationConstraint");
    equation.dump();
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/


BoundConstraint::BoundConstraint(MARABOU_IN_PLAJA::Context& c, MarabouVarIndex_type var, Tightening::BoundType type, MarabouFloating_type scalar):
    MarabouConstraint(c)
    , tightening(var, scalar, type) {
}

BoundConstraint::BoundConstraint(MARABOU_IN_PLAJA::Context& c, const Tightening& tightening):
    MarabouConstraint(c)
    , tightening(tightening) {
    PLAJA_ASSERT(tightening._type == Tightening::LB or tightening._type == Tightening::UB)
}

BoundConstraint::~BoundConstraint() = default;

std::unique_ptr<MarabouConstraint> BoundConstraint::construct_equality_constraint(MARABOU_IN_PLAJA::Context& c, MarabouVarIndex_type var, MarabouFloating_type scalar) {
    std::unique_ptr<ConjunctionInMarabou> equality_constraint(new ConjunctionInMarabou(c));
    equality_constraint->add_equality_constraint(var, scalar);
    return equality_constraint;
}

std::unique_ptr<MarabouConstraint> BoundConstraint::construct(MARABOU_IN_PLAJA::Context& c, MarabouVarIndex_type var, Equation::EquationType op, MarabouFloating_type scalar) {

    switch (op) {

        case Equation::EQ: { return construct_equality_constraint(c, var, scalar); }

        case Equation::GE: { return std::make_unique<BoundConstraint>(c, var, Tightening::LB, scalar); }

        case Equation::LE: { return std::make_unique<BoundConstraint>(c, var, Tightening::UB, scalar); }
    }

    PLAJA_ABORT

}

bool BoundConstraint::operator==(const BoundConstraint& other) const {
    return _context() == other._context() and tightening == other.tightening;
}

void BoundConstraint::add_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) const {
    PLAJA_ASSERT(_context() == query._context())

    switch (tightening._type) {

        case Tightening::LB: {
            query.tighten_lower_bound(tightening._variable, tightening._value);
            return;
        }

        case Tightening::UB: {
            query.tighten_upper_bound(tightening._variable, tightening._value);
            return;
        }

    }

}

void BoundConstraint::move_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) { add_to_query(query); } // Nothing to move.

void BoundConstraint::add_to_conjunction(ConjunctionInMarabou& conjunction) const {
    PLAJA_ASSERT(_context() == conjunction._context())
    conjunction.add_bound(tightening);
}

void BoundConstraint::move_to_conjunction(ConjunctionInMarabou& conjunction) { add_to_conjunction(conjunction); } // There is nothin to move.

void BoundConstraint::add_to_disjunction(DisjunctionInMarabou& disjunction) const {
    PLAJA_ASSERT(_context() == disjunction._context())
    disjunction.add_case_split(tightening);
}

void BoundConstraint::move_to_disjunction(DisjunctionInMarabou& disjunction) { add_to_disjunction(disjunction); }

std::unique_ptr<MarabouConstraint> BoundConstraint::copy() const { return std::make_unique<BoundConstraint>(*this); }

/**********************************************************************************************************************/

void BoundConstraint::substitute(Tightening& tightening, const std::unordered_map<MarabouVarIndex_type, MarabouVarIndex_type>& mapping) {
    auto it = mapping.find(tightening._variable);
    if (it != mapping.cend()) { tightening._variable = it->second; }
}

void BoundConstraint::substitute(const std::unordered_map<MarabouVarIndex_type, MarabouVarIndex_type>& mapping) { substitute(tightening, mapping); }

/**********************************************************************************************************************/

std::unique_ptr<MarabouConstraint> BoundConstraint::compute_negation() const {

    const auto is_integer = context->is_marked_integer(tightening._variable) and PLAJA_FLOATS::is_integer(tightening._value, MARABOU_IN_PLAJA::integerPrecision);
    const auto scalar_offset = is_integer ? 1 : MARABOU_IN_PLAJA::strictnessPrecision;

    switch (tightening._type) {

        case Tightening::LB: { return std::make_unique<BoundConstraint>(_context(), tightening._variable, Tightening::UB, tightening._value - scalar_offset); }
        case Tightening::UB: { return std::make_unique<BoundConstraint>(_context(), tightening._variable, Tightening::LB, tightening._value + scalar_offset); }

    }

    PLAJA_ABORT

}

std::unique_ptr<MarabouConstraint> BoundConstraint::move_to_negation() { return compute_negation(); } // There is nothing to move.

/**********************************************************************************************************************/

std::unique_ptr<MarabouConstraint> BoundConstraint::optimize_for_query(const MARABOU_IN_PLAJA::QueryConstructable& query, DisjunctionConstraintBase::EntailmentValue& entailment_value) const {
    entailment_value = DisjunctionConstraintBase::checkEntailment(tightening, query.get_input_query());
    PLAJA_ASSERT(entailment_value == DisjunctionConstraintBase::Unknown or entailment_value == DisjunctionConstraintBase::Infeasible or entailment_value == DisjunctionConstraintBase::Entailed)
    return entailment_value == DisjunctionConstraintBase::Unknown ? copy() : nullptr;
}

DisjunctionConstraintBase::EntailmentValue BoundConstraint::optimize_for_query(const MARABOU_IN_PLAJA::QueryConstructable& query, std::unique_ptr<MarabouConstraint>& /*optimized_constraint*/) {
    const auto entailment_value = DisjunctionConstraintBase::checkEntailment(tightening, query.get_input_query());
    PLAJA_ASSERT(entailment_value == DisjunctionConstraintBase::Unknown or entailment_value == DisjunctionConstraintBase::Infeasible or entailment_value == DisjunctionConstraintBase::Entailed)
    return entailment_value;
}

bool BoundConstraint::is_bound() const { return true; }

void BoundConstraint::dump() const {
    PLAJA_LOG("BoundConstraint")
    tightening.dump();

}

/**********************************************************************************************************************/
/**********************************************************************************************************************/

DisjunctionInMarabou::DisjunctionInMarabou(MARABOU_IN_PLAJA::Context& c):
    MarabouConstraint(c) {}

DisjunctionInMarabou::~DisjunctionInMarabou() = default;

bool DisjunctionInMarabou::operator==(const DisjunctionInMarabou& other) const { return _context() == other._context() and disjuncts() == other.disjuncts(); }

bool DisjunctionInMarabou::is_trivially_true() const {
    if (disjuncts().size() != 1) { return false; }
    const auto& case_split = _disjuncts.view_std().front();
    return case_split.getBoundTightenings().empty() and case_split.getEquations().empty();
}

bool DisjunctionInMarabou::is_trivially_false() const { return empty(); }

#if 0
void DisjunctionInMarabou::set_trivially_true() {
    disjuncts().clear();
    disjuncts_std_view().reserve(1);
    disjuncts_std_view().emplace_back();
    disjuncts_std_view().shrink_to_fit();
}

void DisjunctionInMarabou::set_trivially_false() { disjuncts().clear(); }
#endif

void DisjunctionInMarabou::add_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) const {
    PLAJA_ASSERT(_context() == query._context())

    PLAJA_ASSERT(disjuncts().size() > 1) // Otherwise should not be Disjunction.

    std::vector<PiecewiseLinearCaseSplit> disjuncts_vec;

    DisjunctionConstraintBase::EntailmentValue entailment_value = DisjunctionConstraintBase::Unknown;

    if (context->do_pre_opt_dis()) {
        entailment_value = DisjunctionConstraintBase::optimizeForQuery(*this, disjuncts_vec, query.get_input_query());
    } else {
        for (auto& disjunct: disjuncts()) { disjuncts_vec.push_back(disjunct); }
    }

    /* Entailed ? */
    if (entailment_value == DisjunctionConstraintBase::Entailed) { return; }

    /* All disjuncts are entailed unsat, hence the disjunction is too, and so is the query? */
    if (entailment_value == DisjunctionConstraintBase::Infeasible) {
        PLAJA_ASSERT(disjuncts_vec.empty())
        context->trivially_false()->move_to_query(query);
        return;
    }

    /* All but one disjunct are entailed unsat. Add this disjunct as constraint. */
    if (disjuncts_vec.size() == 1) {
        auto& case_split = disjuncts_vec[0];
        for (auto bound: case_split._bounds) { query.set_tightening<false>(bound); }
        for (auto& equation: case_split._equations) { query.add_equation(std::move(equation)); }
        return;
    }

    query.add_disjunction(new DisjunctionConstraint(std::move(disjuncts_vec), PLAJA_UTILS::cast_numeric<PLConstraintId>(query.get_num_pl_constraints())));

    // TODO what about conflict learning?
    //  The conflict learning prototype assumes that (pl-)constraints are added in a fixed order.
    //  Hence a conflict learned from a previous query (represented by pl-constraint-id and phase-status) can still be reused for later queries.

}

void DisjunctionInMarabou::move_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) {
    PLAJA_ASSERT(_context() == query._context())

    PLAJA_ASSERT(disjuncts().size() > 1) // Otherwise should not be Disjunction.

    std::vector<PiecewiseLinearCaseSplit> disjuncts_vec;

    DisjunctionConstraintBase::EntailmentValue entailment_value = DisjunctionConstraintBase::Unknown;

    if (context->do_pre_opt_dis()) {
        entailment_value = DisjunctionConstraintBase::optimizeForQuery(*this, disjuncts_vec, query.get_input_query());
    } else {
        for (auto& disjunct: disjuncts()) { disjuncts_vec.push_back(std::move(disjunct)); }
    }

    /* Entailed ? */
    if (entailment_value == DisjunctionConstraintBase::Entailed) { return; }

    /* All disjuncts are entailed unsat, hence the disjunction is too, and so is the query? */
    if (entailment_value == DisjunctionConstraintBase::Infeasible) {
        PLAJA_ASSERT(disjuncts_vec.empty())
        context->trivially_false()->move_to_query(query);
        return;
    }

    /* All but one disjunct are entailed unsat. Add this disjunct as constraint. */
    if (disjuncts_vec.size() == 1) {
        auto& case_split = disjuncts_vec[0];
        for (auto bound: case_split._bounds) { query.set_tightening<false>(bound); }
        for (auto& equation: case_split._equations) { query.add_equation(std::move(equation)); }
        return;
    }

    query.add_disjunction(new DisjunctionConstraint(std::move(disjuncts_vec), PLAJA_UTILS::cast_numeric<PLConstraintId>(query.get_num_pl_constraints())));

}

void DisjunctionInMarabou::add_to_conjunction(ConjunctionInMarabou& conjunction) const {
    PLAJA_ASSERT(_context() == conjunction._context())
    PLAJA_ASSERT_EXPECT(disjuncts().size() > 1) // sanity check, otherwise should be conjunction
    conjunction.add_disjunction(*this);
}

void DisjunctionInMarabou::move_to_conjunction(ConjunctionInMarabou& conjunction) {
    PLAJA_ASSERT(_context() == conjunction._context())
    PLAJA_ASSERT_EXPECT(disjuncts().size() > 1) // sanity check, otherwise should be conjunction
    conjunction.add_disjunction(std::move(*this));
}

void DisjunctionInMarabou::add_to_disjunction(DisjunctionInMarabou& disjunction) const {
    PLAJA_ASSERT(_context() == disjunction._context())
    disjunction.reserve_for_additional(disjuncts().size());
    for (const auto& case_split: disjuncts()) { disjunction.add_case_split(case_split); }
}

void DisjunctionInMarabou::move_to_disjunction(DisjunctionInMarabou& disjunction) {
    PLAJA_ASSERT(_context() == disjunction._context())

    disjunction.reserve_for_additional(disjuncts().size());
    for (auto& case_split: disjuncts()) { disjunction.add_case_split(std::move(case_split)); }

    // Just move list.
    // disjunction.disjuncts.splice(disjunction.disjuncts.end(), this->disjuncts);

    PLAJA_ASSERT(std::all_of(disjuncts().begin(), disjuncts().end(), [](const PiecewiseLinearCaseSplit& case_split) { return case_split.getBoundTightenings().size() + case_split.getEquations().size() == 0; }))

}

std::unique_ptr<MarabouConstraint> DisjunctionInMarabou::copy() const { return std::make_unique<DisjunctionInMarabou>(*this); }

void DisjunctionInMarabou::substitute(const std::unordered_map<MarabouVarIndex_type, MarabouVarIndex_type>& mapping) {

    for (auto& case_split: disjuncts()) {

        std::unordered_set<MarabouVarIndex_type> participating_vars;

        for (const auto& bound: case_split.getBoundTightenings()) { participating_vars.insert(bound._variable); }

        for (const auto& eq: case_split.getEquations()) { for (auto var: eq.getParticipatingVariables()) { participating_vars.insert(var); } }

        for (auto var: participating_vars) {
            auto it_sub = mapping.find(var);
            if (it_sub != mapping.cend()) { case_split.updateVariableIndex(var, it_sub->second); }
        }

    }
}

// not ( (e_1 && b_1) || (e_2 && b_2) ) -> not (e_1 && b_1) && not (e_2 && b_2) -> (not e_1 || not b_1) and (not e_2 || not b_2)
std::unique_ptr<MarabouConstraint> DisjunctionInMarabou::compute_negation() const {
    std::unique_ptr<ConjunctionInMarabou> conjunction(new ConjunctionInMarabou(_context()));

    for (const auto& case_split: disjuncts()) {

        const auto& bounds = case_split.getBoundTightenings();
        const auto& equations = case_split.getEquations();

        if (bounds.size() + equations.size() == 1) { // Special case: Only one atom, that may be added as equation/bound constraint.

            for (const auto& bound: bounds) { BoundConstraint(_context(), bound).compute_negation()->move_to_conjunction(*conjunction); }
            for (const auto& equation: equations) { EquationConstraint(_context(), equation).compute_negation()->move_to_conjunction(*conjunction); }

        } else { // General case: case split becomes a disjunction.

            DisjunctionInMarabou disjunction(_context());

            for (const auto& bound: bounds) { BoundConstraint(_context(), bound).compute_negation()->move_to_disjunction(disjunction); }
            for (const auto& equation: equations) { EquationConstraint(_context(), equation).compute_negation()->move_to_disjunction(disjunction); }

            disjunction.move_to_conjunction(*conjunction);

        }

    }

    return conjunction;
}

std::unique_ptr<MarabouConstraint> DisjunctionInMarabou::move_to_negation() {
    std::unique_ptr<ConjunctionInMarabou> conjunction(new ConjunctionInMarabou(_context()));

    for (auto& case_split: disjuncts()) {

        auto& bounds = case_split._bounds;
        auto& equations = case_split._equations;

        if (bounds.size() + equations.size() == 1) { // Special case: Only one atom, that may be added as equation/bound constraint.

            for (auto& bound: bounds) { BoundConstraint(_context(), bound).move_to_negation()->move_to_conjunction(*conjunction); }
            for (auto& equation: equations) { EquationConstraint(_context(), std::move(equation)).move_to_negation()->move_to_conjunction(*conjunction); }

        } else { // General case: case split becomes a disjunction.

            DisjunctionInMarabou disjunction(_context());

            for (auto& bound: bounds) { BoundConstraint(_context(), bound).move_to_negation()->move_to_disjunction(disjunction); }
            for (auto& equation: equations) { EquationConstraint(_context(), std::move(equation)).move_to_negation()->move_to_disjunction(disjunction); }

            disjunction.move_to_conjunction(*conjunction);

        }

    }

    return conjunction;
}

std::unique_ptr<MarabouConstraint> DisjunctionInMarabou::optimize_for_query(const MARABOU_IN_PLAJA::QueryConstructable& query, DisjunctionConstraintBase::EntailmentValue& entailment_value) const {
    std::vector<PiecewiseLinearCaseSplit> optimized_disjuncts;

    entailment_value = DisjunctionConstraintBase::optimizeForQuery(*this, optimized_disjuncts, query.get_input_query());

    if (entailment_value != DisjunctionConstraintBase::Unknown) {
        PLAJA_ASSERT(entailment_value == DisjunctionConstraintBase::Entailed or entailment_value == DisjunctionInMarabou::Infeasible)
        return nullptr;
    }

    PLAJA_ASSERT(not optimized_disjuncts.empty())

    /* All but one disjunct are entailed unsat. */
    if (optimized_disjuncts.size() == 1) {

        std::unique_ptr<ConjunctionInMarabou> optimized_constraint(new ConjunctionInMarabou(*context));
        optimized_constraint->add_case_split(std::move(optimized_disjuncts[0]));
        return optimized_constraint;

    }

    std::unique_ptr<DisjunctionInMarabou> optimized_disjunction(new DisjunctionInMarabou(*context));
    optimized_disjunction->_disjuncts = std::move(optimized_disjuncts);
    PLAJA_ASSERT(not optimized_disjunction->empty())
    return optimized_disjunction;

}

DisjunctionConstraintBase::EntailmentValue DisjunctionInMarabou::optimize_for_query(const MARABOU_IN_PLAJA::QueryConstructable& query, std::unique_ptr<MarabouConstraint>& optimized_constraint) {
    std::vector<PiecewiseLinearCaseSplit> optimized_disjuncts;
    const auto entailment_value = DisjunctionConstraintBase::optimizeForQuery(*this, optimized_disjuncts, query.get_input_query());

    if (entailment_value != DisjunctionConstraintBase::Unknown) {
        PLAJA_ASSERT(entailment_value == DisjunctionConstraintBase::Entailed or entailment_value == DisjunctionInMarabou::Infeasible)
        _disjuncts.clear();
        return entailment_value;
    }

    PLAJA_ASSERT_EXPECT(not _disjuncts.empty()) // The case splits may be moved, but the case split list is not.
    PLAJA_ASSERT(not optimized_disjuncts.empty())

    /* All but one disjunct are entailed unsat. */
    if (optimized_disjuncts.size() == 1) {
        optimized_constraint = std::make_unique<ConjunctionInMarabou>(*context);
        auto* optimized_conjunction = PLAJA_UTILS::cast_ptr<ConjunctionInMarabou>(optimized_constraint.get());
        optimized_conjunction->add_case_split(std::move(optimized_disjuncts[0]));
    } else {
        _disjuncts = std::move(optimized_disjuncts);
    }

    return entailment_value;
}

bool DisjunctionInMarabou::is_linear() const { return false; }

void DisjunctionInMarabou::dump() const {
    PLAJA_LOG("Disjunction in Marabou");
    for (const auto& case_split: disjuncts()) { case_split.dump(); }
}

void DisjunctionInMarabou::add_case_split(const Equation& equation) {
    auto& disjuncts_view = disjuncts_std_view();
    disjuncts_view.emplace_back();
    disjuncts_view.back().addEquation(equation);
}

void DisjunctionInMarabou::add_case_split(Equation&& equation) {
    auto& disjuncts_view = disjuncts_std_view();
    disjuncts_view.emplace_back();
    disjuncts_view.back().addEquation(std::move(equation));
}

void DisjunctionInMarabou::add_case_split(const Tightening& tightening) {
    auto& disjuncts_view = disjuncts_std_view();
    disjuncts_view.emplace_back();
    disjuncts_view.back().storeBoundTightening(tightening);
}

/**********************************************************************************************************************/

void DisjunctionInMarabou::compute_global_bounds(std::unordered_map<MarabouVarIndex_type, MarabouFloating_type>& lower_bounds_global, std::unordered_map<MarabouVarIndex_type, MarabouFloating_type>& upper_bounds_global) const {
    PLAJA_ASSERT(lower_bounds_global.empty())
    PLAJA_ASSERT(upper_bounds_global.empty())

    auto disjunct_index = 0;

    for (const auto& disjunct: disjuncts()) {

        // Collect bounds of disjunct:

        std::unordered_map<MarabouVarIndex_type, MarabouFloating_type> lower_bounds_disjunct;
        std::unordered_map<MarabouVarIndex_type, MarabouFloating_type> upper_bounds_disjunct;

        for (const auto& bound: disjunct.getBoundTightenings()) {

            switch (bound._type) {

                case Tightening::LB: {
                    auto it = lower_bounds_disjunct.find(bound._variable);

                    if (it == lower_bounds_disjunct.end()) {
                        lower_bounds_disjunct.emplace(bound._variable, bound._value);
                    } else {
                        it->second = std::max(it->second, bound._value); // Use tightest bound in case split (i.e., there may be redundancies like x >= 0 and x >= 2).
                    }

                    break;
                }

                case Tightening::UB: {
                    auto it = upper_bounds_disjunct.find(bound._variable);

                    if (it == upper_bounds_disjunct.end()) {
                        upper_bounds_disjunct.emplace(bound._variable, bound._value);
                    } else {
                        it->second = std::min(it->second, bound._value); // Use tightest bound in case split.
                    }

                    break;
                }

            }
        }

        // Is it still possible to have any global bounds?
        if (lower_bounds_disjunct.size() + upper_bounds_disjunct.size() == 0) { return; }

        if (disjunct_index == 0) {

            // Just keep bounds of first disjunct for now.
            lower_bounds_global = std::move(lower_bounds_disjunct);
            upper_bounds_global = std::move(upper_bounds_disjunct);

        } else {

            // update lower bounds:
            for (auto it = lower_bounds_global.begin(); it != lower_bounds_global.end();) {

                auto it_disjunct = lower_bounds_disjunct.find(it->first);

                if (it_disjunct == lower_bounds_disjunct.end()) {

                    // Variable not lower bounded in all disjuncts.
                    it = lower_bounds_global.erase(it);

                } else {

                    it->second = std::min(it->second, it_disjunct->second);
                    PLAJA_ASSERT(it->second <= it_disjunct->second);
                    ++it;

                    PLAJA_ASSERT(lower_bounds_global.at(it_disjunct->first) <= lower_bounds_disjunct.at(it_disjunct->first));

                }

            }

            // update upper bounds
            for (auto it = upper_bounds_global.begin(); it != upper_bounds_global.end();) {

                auto it_disjunct = upper_bounds_disjunct.find(it->first);

                if (it_disjunct == upper_bounds_disjunct.end()) {

                    it = upper_bounds_global.erase(it);

                } else {

                    it->second = std::max(it->second, it_disjunct->second);
                    PLAJA_ASSERT(it->second >= it_disjunct->second);
                    ++it;

                    PLAJA_ASSERT(upper_bounds_global.at(it_disjunct->first) >= upper_bounds_disjunct.at(it_disjunct->first));

                }

            }

        }

        ++disjunct_index;
    }

}

/**********************************************************************************************************************/

std::unique_ptr<MarabouConstraint> DisjunctionInMarabou::optimize() {
    std::unique_ptr<ConjunctionInMarabou> optimized_conjunction(new ConjunctionInMarabou(_context()));

    /* Per case-split optimization. */
    if (not optimizeBounds()) { return context->trivially_false(); }

    /* Special case. */
    if (disjuncts().size() == 1) {
        optimized_conjunction->add_case_split(std::move(disjuncts_std_view().front()));
        return optimized_conjunction;
    }

    // Default:
    std::unique_ptr<DisjunctionInMarabou> optimized_disjunction(new DisjunctionInMarabou(_context()));

    std::unordered_map<MarabouVarIndex_type, MarabouFloating_type> lower_bounds_global;
    std::unordered_map<MarabouVarIndex_type, MarabouFloating_type> upper_bounds_global;
    compute_global_bounds(lower_bounds_global, upper_bounds_global);

    // TODO factor out equations

    // Special case "nothing to optimize":
    if (lower_bounds_global.empty() and upper_bounds_global.empty()) { return std::make_unique<DisjunctionInMarabou>(std::move(*this)); }

    // Add global bounds
    for (const auto& [var, lower_bound]: lower_bounds_global) { optimized_conjunction->add_bound(var, Tightening::LB, lower_bound); }
    for (const auto& [var, upper_bound]: upper_bounds_global) { optimized_conjunction->add_bound(var, Tightening::UB, upper_bound); }

    // Optimize disjuncts.
    for (auto& disjunct: disjuncts()) {

        // Iterate tightenings and remove entailed ones.

        auto& bounds = disjunct._bounds;
        for (auto it = bounds.begin(); it != bounds.end();) {

            const auto& bound = *it;

            switch (bound._type) {

                case Tightening::LB: {

                    auto it_global = lower_bounds_global.find(bound._variable);
                    PLAJA_ASSERT(it_global == lower_bounds_global.end() or it_global->second <= bound._value) // Sanity global lower bound must be minimal.

                    if (it_global != lower_bounds_global.end() and PLAJA_FLOATS::equal(it_global->second, bound._value, MARABOU_IN_PLAJA::floatingPrecision)) {
                        it = bounds.erase(it);
                        continue;
                    }

                    break;

                }

                case Tightening::UB: {

                    auto it_global = upper_bounds_global.find(bound._variable);
                    PLAJA_ASSERT(it_global == upper_bounds_global.end() or bound._value <= it_global->second) // Sanity global upper bound must be maximal.

                    if (it_global != upper_bounds_global.end() and PLAJA_FLOATS::equal(it_global->second, bound._value, MARABOU_IN_PLAJA::floatingPrecision)) {
                        it = bounds.erase(it);
                        continue;
                    }

                    break;

                }

                default: { PLAJA_ABORT }
            }

            ++it;

        }

        // Just keep equations for now ...
        // disjunct._equations = std::move(disjunct._equations);

        // Disjunctive is (syntactically) entailed by conjunctive constraints? And thus conjunctive constraints entailed disjunction.
        if (disjunct.getBoundTightenings().size() + disjunct.getEquations().size() == 0) {
            return optimized_conjunction;
        }

        optimized_disjunction->add_case_split(std::move(disjunct));

    }

    // Add optimized disjunction:
    optimized_disjunction->move_to_conjunction(*optimized_conjunction);

    return optimized_conjunction;
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/

ConjunctionInMarabou::ConjunctionInMarabou(MARABOU_IN_PLAJA::Context& c):
    MarabouConstraint(c) {}

ConjunctionInMarabou::~ConjunctionInMarabou() = default;

bool ConjunctionInMarabou::operator==(const ConjunctionInMarabou& other) const { return _context() == other._context() and equations == other.equations and bounds == other.bounds and disjunctions == other.disjunctions; }

bool ConjunctionInMarabou::is_trivially_true() const { return empty(); }

bool ConjunctionInMarabou::is_trivially_false() const {
    if (not equations.empty() and disjunctions.empty() and bounds.size() == 2) { return false; }

    PLAJA_ASSERT(context->get_number_of_variables() > 0)

    const auto& b1 = bounds.front();
    if (b1._type != Tightening::LB or b1._variable != 0 or b1._value != 1) { return false; }

    const auto& b2 = bounds.back();
    if (b2._type != Tightening::UB or b2._variable != 0 or b2._value != -1) { return false; }

    return true;
}

#if 0
void ConjunctionInMarabou::set_trivially_true() {
    equations.clear();
    bounds.clear();
    disjunctions.clear();
}

void ConjunctionInMarabou::set_trivially_false() {
    PLAJA_ASSERT(context->get_number_of_variables() > 0)
    equations.clear();
    bounds.clear();
    disjunctions.clear();
    add_bound(0, Tightening::LB, 1);
    add_bound(0, Tightening::UB, -1);
}
#endif

void ConjunctionInMarabou::add_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) const {
    PLAJA_ASSERT(_context() == query._context())
    for (const auto& equation: equations) { query.add_equation(equation); }
    for (const auto& bound: bounds) { query.set_tightening<false>(bound); }
    for (const auto& disjunction: disjunctions) { disjunction.add_to_query(query); }
}

void ConjunctionInMarabou::move_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) {
    PLAJA_ASSERT(_context() == query._context())
    query.add_equations(std::move(equations));
    for (const auto& bound: bounds) { query.set_tightening<false>(bound); }
    for (auto& disjunction: disjunctions) { disjunction.move_to_query(query); }
}

void ConjunctionInMarabou::add_to_conjunction(ConjunctionInMarabou& conjunction) const {
    PLAJA_ASSERT(_context() == conjunction._context())
    for (const auto& equation: equations) { conjunction.add_equation(equation); }
    for (const auto& bound: bounds) { conjunction.add_bound(bound); }
    for (const auto& disjunction: disjunctions) { conjunction.add_disjunction(disjunction); }
}

void ConjunctionInMarabou::move_to_conjunction(ConjunctionInMarabou& conjunction) {
    PLAJA_ASSERT(_context() == conjunction._context())

    conjunction.equations.splice(conjunction.equations.end(), equations);
    PLAJA_ASSERT(equations.empty())

    conjunction.bounds.splice(conjunction.bounds.end(), bounds);
    PLAJA_ASSERT(bounds.empty())

    conjunction.disjunctions.splice(conjunction.disjunctions.end(), disjunctions);
    PLAJA_ASSERT(disjunctions.empty())
}

namespace CONJUNCTION_IN_MARABOU {

    void compute_case_splits_recursively(DisjunctionInMarabou& target_disjunction, PiecewiseLinearCaseSplit&& constructor, std::list<DisjunctionInMarabou>::const_iterator it, const std::list<DisjunctionInMarabou>::const_iterator& end) { // NOLINT(misc-no-recursion)

        if (it == end) {

            if (DisjunctionConstraintBase::optimizeBounds(constructor)) {
                target_disjunction.add_case_split(std::move(constructor));
            }

        } else {

            for (const auto& case_split: it->get_disjuncts()) {

                PiecewiseLinearCaseSplit constructor_tmp(constructor);
                for (const auto& tightening: case_split.getBoundTightenings()) { constructor_tmp.storeBoundTightening(tightening); }
                for (const auto& equation: case_split.getEquations()) { constructor_tmp.addEquation(equation); }

                compute_case_splits_recursively(target_disjunction, std::move(constructor_tmp), ++it, end);
                --it; // return to old position

            }

        }

    }

}

// e_1 && b_1 && ((e_2 && b_2) || (e_3 && b_3)) -> ( e_1 && b_1 && e_2 && b_2 ) || ( e_1 && b_1 && e_3 && b_3 )
void ConjunctionInMarabou::add_to_disjunction(DisjunctionInMarabou& disjunction) const {
    PLAJA_ASSERT(_context() == disjunction._context())

    std::size_t num_disjuncts = 1;
    for (const auto& disjunction_ref: disjunctions) { num_disjuncts *= disjunction_ref.get_disjuncts().size(); }
    disjunction.reserve_for_additional(num_disjuncts);

    PiecewiseLinearCaseSplit case_split_base;
    for (const auto& equation: equations) { case_split_base.addEquation(equation); }
    for (const auto& bound: bounds) { case_split_base.storeBoundTightening(bound); }

    CONJUNCTION_IN_MARABOU::compute_case_splits_recursively(disjunction, std::move(case_split_base), disjunctions.begin(), disjunctions.cend());

}

void ConjunctionInMarabou::move_to_disjunction(DisjunctionInMarabou& disjunction) {
    PLAJA_ASSERT(_context() == disjunction._context())

    std::size_t num_disjuncts = 1;
    for (const auto& disjunction_ref: disjunctions) { num_disjuncts *= disjunction_ref.get_disjuncts().size(); }
    disjunction.reserve_for_additional(num_disjuncts);

    PiecewiseLinearCaseSplit case_split_base;
    case_split_base._equations.append(std::move(equations));
    case_split_base._bounds.append(std::move(bounds));

    CONJUNCTION_IN_MARABOU::compute_case_splits_recursively(disjunction, std::move(case_split_base), disjunctions.begin(), disjunctions.cend());

}

std::unique_ptr<MarabouConstraint> ConjunctionInMarabou::copy() const { return std::make_unique<ConjunctionInMarabou>(*this); }

void ConjunctionInMarabou::substitute(const std::unordered_map<MarabouVarIndex_type, MarabouVarIndex_type>& mapping) {
    for (auto& bound: bounds) { BoundConstraint::substitute(bound, mapping); }
    for (auto& equation: equations) { EquationConstraint::substitute(equation, mapping); }
    for (auto& disjunction: disjunctions) { disjunction.substitute(mapping); }
}

// not ( e_1 && b_1 && ((e_2 && b_2) || (e_3 && b_3)) ) -> not e_1 || not b_1 || not ((e_2 && b_2) || (e_3 && b_3))
// -> not e_1 || not b_1 || ( not (e_2 && b_2) && not (e_3 && b_3) )
// -> not e_1 || not b_1 || ( (not e_2 || not b_2) && (not e_3 || not b_3) )
// -> not e_1 || not b_1 || ( (not e_2 && not e_3) || (not e_2 && not b_3) || (not b_2 && not e_3) || (not b_2 && not b_3) )
std::unique_ptr<MarabouConstraint> ConjunctionInMarabou::compute_negation() const {

    std::unique_ptr<DisjunctionInMarabou> disjunction(new DisjunctionInMarabou(_context()));

    for (const auto& equation: equations) { EquationConstraint(_context(), equation).move_to_negation()->move_to_disjunction(*disjunction); }
    for (const auto& bound: bounds) { PLAJA_UTILS::cast_unique<BoundConstraint>(BoundConstraint(_context(), bound).move_to_negation())->move_to_disjunction(*disjunction); }
    for (const auto& disjunction_: disjunctions) { disjunction_.compute_negation()->move_to_disjunction(*disjunction); }

    return disjunction->optimize();

}

std::unique_ptr<MarabouConstraint> ConjunctionInMarabou::move_to_negation() {
    std::unique_ptr<DisjunctionInMarabou> disjunction(new DisjunctionInMarabou(_context()));

    for (auto& equation: equations) { EquationConstraint(_context(), std::move(equation)).move_to_negation()->move_to_disjunction(*disjunction); }
    for (const auto& bound: bounds) { PLAJA_UTILS::cast_unique<BoundConstraint>(BoundConstraint(_context(), bound).move_to_negation())->move_to_disjunction(*disjunction); }
    for (auto& disjunction_: disjunctions) { disjunction_.move_to_negation()->move_to_disjunction(*disjunction); }

    return disjunction->optimize();
}

std::unique_ptr<MarabouConstraint> ConjunctionInMarabou::optimize_for_query(const MARABOU_IN_PLAJA::QueryConstructable& query, DisjunctionConstraintBase::EntailmentValue& entailment_value) const {
    const auto& input_query = query.get_input_query();

    auto optimized_conjunction = std::make_unique<ConjunctionInMarabou>(*context);

    for (const auto& bound: bounds) {

        switch (DisjunctionConstraintBase::checkEntailment(bound, input_query)) {

            case DisjunctionConstraintBase::Unknown: {
                optimized_conjunction->add_bound(bound);
                continue;
            }

            case DisjunctionConstraintBase::Infeasible: {
                entailment_value = DisjunctionConstraintBase::Infeasible;
                return nullptr;
            }

            case DisjunctionConstraintBase::Entailed: { continue; }

            default: { PLAJA_ABORT }
        }

        PLAJA_ABORT

    }

    for (const auto& equation: equations) {

        switch (DisjunctionConstraintBase::checkEntailment(equation, input_query)) {

            case DisjunctionConstraintBase::Unknown: {
                optimized_conjunction->add_equation(equation);
                continue;
            }

            case DisjunctionConstraintBase::Infeasible: {
                entailment_value = DisjunctionConstraintBase::Infeasible;
                return nullptr;
            }

            case DisjunctionConstraintBase::Entailed: { continue; }

            case DisjunctionConstraintBase::GeEntailed: {
                Equation eq_optimized(equation);
                eq_optimized._type = Equation::LE;
                optimized_conjunction->add_equation(std::move(eq_optimized));
                continue;
            }

            case DisjunctionConstraintBase::LeEntailed: {
                Equation eq_optimized(equation);
                eq_optimized._type = Equation::GE;
                optimized_conjunction->add_equation(std::move(eq_optimized));
                continue;
            }

        }

        PLAJA_ABORT

    }

    for (const auto& disjunction: disjunctions) {

        DisjunctionConstraintBase::EntailmentValue entailment(DisjunctionConstraintBase::Unknown);
        auto optimized_disjunction = disjunction.optimize_for_query(query, entailment);

        switch (entailment) {

            case DisjunctionConstraintBase::Unknown: {
                optimized_disjunction->move_to_conjunction(*optimized_conjunction);
                continue;
            }

            case DisjunctionConstraintBase::Infeasible: {
                entailment_value = DisjunctionConstraintBase::Infeasible;
                return nullptr;
            }

            case DisjunctionConstraintBase::Entailed: { continue; }

            default: { PLAJA_ABORT }
        }

        PLAJA_ABORT

    }

    /* Special cases. */
    if (optimized_conjunction->bounds.empty() and optimized_conjunction->equations.empty()) {

        if (optimized_conjunction->disjunctions.empty()) {

            entailment_value = DisjunctionConstraintBase::Entailed;
            return nullptr;

        } else if (optimized_conjunction->disjunctions.size() == 1) {
            return std::make_unique<DisjunctionInMarabou>(std::move(optimized_conjunction->disjunctions.front()));
        }

    }

    return optimized_conjunction;
}

DisjunctionConstraintBase::EntailmentValue ConjunctionInMarabou::optimize_for_query(const MARABOU_IN_PLAJA::QueryConstructable& query, std::unique_ptr<MarabouConstraint>& /*optimized_constraint*/) {
    const auto& input_query = query.get_input_query();

    for (auto it = bounds.begin(); it != bounds.end();) {

        const auto& bound = *it;

        switch (DisjunctionConstraintBase::checkEntailment(bound, input_query)) {

            case DisjunctionConstraintBase::Unknown: {
                ++it;
                continue;
            }

            case DisjunctionConstraintBase::Infeasible: {
                bounds.clear();
                equations.clear();
                disjunctions.clear();
                return DisjunctionConstraintBase::Infeasible;
            }

            case DisjunctionConstraintBase::Entailed: {
                it = bounds.erase(it);
                continue;
            }

            default: { PLAJA_ABORT }
        }

        PLAJA_ABORT

    }

    for (auto it = equations.begin(); it != equations.end();) {

        auto& equation = *it;

        switch (DisjunctionConstraintBase::checkEntailment(equation, input_query)) {

            case DisjunctionConstraintBase::Unknown: {
                ++it;
                continue;
            }

            case DisjunctionConstraintBase::Infeasible: {
                bounds.clear();
                equations.clear();
                disjunctions.clear();
                return DisjunctionConstraintBase::Infeasible;
            }

            case DisjunctionConstraintBase::Entailed: {
                it = equations.erase(it);
                continue;
            }

            case DisjunctionConstraintBase::GeEntailed: {
                equation._type = Equation::LE;
                ++it;
                continue;
            }

            case DisjunctionConstraintBase::LeEntailed: {
                equation._type = Equation::GE;
                ++it;
                continue;
            }

        }

        PLAJA_ABORT

    }

    for (auto it = disjunctions.begin(); it != disjunctions.end();) {

        auto& disjunction = *it;
        std::unique_ptr<MarabouConstraint> optimized_constraint(nullptr);

        switch (disjunction.optimize_for_query(query, optimized_constraint)) {

            case DisjunctionConstraintBase::Unknown: {

                if (optimized_constraint) {
                    STMT_IF_DEBUG(const auto num_disjunctions = disjunctions.size();)
                    optimized_constraint->move_to_conjunction(*this);
                    PLAJA_ASSERT(num_disjunctions == disjunctions.size()) // Should not add new disjunction.
                    it = disjunctions.erase(it);
                } else { ++it; }

                continue;
            }

            case DisjunctionConstraintBase::Infeasible: {
                bounds.clear();
                equations.clear();
                disjunctions.clear();
                return DisjunctionConstraintBase::Infeasible;
            }

            case DisjunctionConstraintBase::Entailed: {
                it = disjunctions.erase(it);
                continue;
            }

            default: { PLAJA_ABORT }
        }

        PLAJA_ABORT

    }

    /* Special cases. */
    if (bounds.empty() and equations.empty() and disjunctions.empty()) { return DisjunctionConstraintBase::Entailed; }

    return DisjunctionConstraintBase::Unknown;
}

bool ConjunctionInMarabou::is_linear() const { return disjunctions.empty(); }

void ConjunctionInMarabou::dump() const {
    PLAJA_LOG("Conjunction in Marabou")
    for (const auto& equation: equations) { equation.dump(); }
    for (const auto& bound: bounds) { bound.dump(); }
    for (const auto& disjunction: disjunctions) { disjunction.dump(); }
}

void ConjunctionInMarabou::add_case_split(const PiecewiseLinearCaseSplit& case_split) {
    for (const auto& equation: case_split.getEquations()) { equations.push_back(equation); }
    for (const auto& tightening: case_split.getBoundTightenings()) { bounds.emplace_back(tightening); }
}

void ConjunctionInMarabou::add_case_split(PiecewiseLinearCaseSplit&& case_split) { // NOLINT(*-rvalue-reference-param-not-moved)
    equations.splice(equations.end(), case_split._equations.move_to_std());
    bounds.splice(bounds.end(), case_split._bounds.move_to_std());
}

void ConjunctionInMarabou::add_equality_constraint(MarabouVarIndex_type var, MarabouFloating_type scalar) {
    add_bound(var, Tightening::LB, scalar);
    add_bound(var, Tightening::UB, scalar);
}
