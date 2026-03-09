//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include <unordered_map>
#include "constraints_in_veritas.h"
#include "../../option_parser/option_parser.h"
#include "../../utils/floating_utils.h"
#include "../../utils/utils.h"
#include "solver/solver.h"
#include "../../globals.h"
#include "veritas_context.h"
#include "veritas_query.h"
#include "predicates/Equation.h"
#include "predicates/Disjunct.h"

/**********************************************************************************************************************/

VeritasConstraint::VeritasConstraint(VERITAS_IN_PLAJA::Context& c):
    context(&c) {}

VeritasConstraint::~VeritasConstraint() = default;

std::unique_ptr<VeritasConstraint> VeritasConstraint::to_substitute(const std::unordered_map<VeritasVarIndex_type, VeritasVarIndex_type>& mapping) const {
    auto constraint = copy();
    constraint->substitute(mapping);
    return constraint;
}

bool VeritasConstraint::is_bound() const { return false; }

bool VeritasConstraint::is_linear() const { return true; }

void VeritasConstraint::add_to_solver(PLAJA::SmtSolver& solver) const { add_to_query(PLAJA_UTILS::cast_ref<VERITAS_IN_PLAJA::Solver>(solver)); }

void VeritasConstraint::add_negation_to_solver(PLAJA::SmtSolver& solver) const { compute_negation()->move_to_query(PLAJA_UTILS::cast_ref<VERITAS_IN_PLAJA::Solver>(solver)); }


/**********************************************************************************************************************/
/**********************************************************************************************************************/

VERITAS_IN_PLAJA::PredicateConstraint::PredicateConstraint(std::unique_ptr<VeritasConstraint>&& constraint_):
    constraint(std::move(constraint_))
    , negation(constraint->compute_negation()) {

}

VERITAS_IN_PLAJA::PredicateConstraint::~PredicateConstraint() = default;

void VERITAS_IN_PLAJA::PredicateConstraint::dump() const {
    PLAJA_LOG("Predicate Constraint")
    constraint->dump();
    PLAJA_LOG("Negation")
    negation->dump();
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/

VERITAS_IN_PLAJA::EquationConstraint::EquationConstraint(VERITAS_IN_PLAJA::Context& c, VERITAS_IN_PLAJA::Equation eq):
    VeritasConstraint(c)
    , equation(std::move(eq)) {}

VERITAS_IN_PLAJA::EquationConstraint::~EquationConstraint() = default;

bool VERITAS_IN_PLAJA::EquationConstraint::operator==(const EquationConstraint& other) const { return _context() == other._context() and equation.equivalent(other.equation); }

void VERITAS_IN_PLAJA::EquationConstraint::add_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates) const {
    PLAJA_ASSERT(_context() == query._context())
    if (updates) {
        query.add_update(VERITAS_IN_PLAJA::Equation(equation));
    } else {
        query.add_equation(VERITAS_IN_PLAJA::Equation(equation));
    }
}

void VERITAS_IN_PLAJA::EquationConstraint::move_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates) {
    PLAJA_ASSERT(_context() == query._context())
    if (updates) {
        query.add_update(std::move(equation));
    } else {
        query.add_equation(std::move(equation));
    }
}

void VERITAS_IN_PLAJA::EquationConstraint::add_to_conjunction(ConjunctionInVeritas& conjunction) const {
    PLAJA_ASSERT(_context() == conjunction._context())
    conjunction.add_equation(equation);
}

void VERITAS_IN_PLAJA::EquationConstraint::move_to_conjunction(ConjunctionInVeritas& conjunction) {
    PLAJA_ASSERT(_context() == conjunction._context())
    conjunction.add_equation(std::move(equation));
}

void VERITAS_IN_PLAJA::EquationConstraint::add_to_disjunction(DisjunctionInVeritas& disjunction) const {
    PLAJA_ASSERT(_context() == disjunction._context())
    disjunction.add_case_split(equation);
}

void VERITAS_IN_PLAJA::EquationConstraint::move_to_disjunction(DisjunctionInVeritas& disjunction) {
    PLAJA_ASSERT(_context() == disjunction._context())
    disjunction.add_case_split(std::move(equation));
}

std::unique_ptr<VeritasConstraint> VERITAS_IN_PLAJA::EquationConstraint::copy() const { return std::make_unique<VERITAS_IN_PLAJA::EquationConstraint>(*this); }

/**********************************************************************************************************************/

inline VeritasFloating_type VERITAS_IN_PLAJA::EquationConstraint::compute_negation_offset(const VERITAS_IN_PLAJA::Context& c, const VERITAS_IN_PLAJA::Equation& eq) {

    // is integer ?
    auto is_integer = PLAJA_FLOATS::is_integer(eq._scalar, VERITAS_IN_PLAJA::integerPrecision);
    //
    for (auto addend: eq._addends) {
        if (not is_integer) { break; }
        is_integer = c.is_marked_integer(addend._variable) and PLAJA_FLOATS::is_integer(addend._coefficient, VERITAS_IN_PLAJA::integerPrecision);
    }

    return is_integer ? 1 : VERITAS_IN_PLAJA::strictnessPrecision;
}

inline void VERITAS_IN_PLAJA::EquationConstraint::negate_to_ge(VERITAS_IN_PLAJA::Equation& eq, VeritasFloating_type scalar_offset) {
    PLAJA_ASSERT(eq._type != VERITAS_IN_PLAJA::Equation::GE)

    // negation(x =< c) -> (x >= c + 1) for c integer
    eq.setType(VERITAS_IN_PLAJA::Equation::GE);
    eq.setScalar(eq._scalar + scalar_offset);
}

inline void VERITAS_IN_PLAJA::EquationConstraint::negate_to_le(VERITAS_IN_PLAJA::Equation& eq, VeritasFloating_type scalar_offset) {
    PLAJA_ASSERT(eq._type != VERITAS_IN_PLAJA::Equation::LE)

    // negation(x >= c) -> (x <= c - 1) for c integer
    eq.setType(VERITAS_IN_PLAJA::Equation::LE);
    eq.setScalar(eq._scalar - scalar_offset);
}

inline void VERITAS_IN_PLAJA::EquationConstraint::negate_non_eq(const VERITAS_IN_PLAJA::Context& c, VERITAS_IN_PLAJA::Equation& eq) {
    PLAJA_ASSERT(eq._type != VERITAS_IN_PLAJA::Equation::EQ)

    switch (eq._type) {

        case VERITAS_IN_PLAJA::Equation::EQ: { PLAJA_ABORT }

        case VERITAS_IN_PLAJA::Equation::GE: {
            negate_to_le(eq, compute_negation_offset(c, eq));
            break;
        }

        case VERITAS_IN_PLAJA::Equation::LE: {
            negate_to_ge(eq, compute_negation_offset(c, eq));
            break;
        }

    }

}

/**********************************************************************************************************************/

std::unique_ptr<VeritasConstraint> VERITAS_IN_PLAJA::EquationConstraint::compute_negation() const {

    const auto scalar_offset = compute_negation_offset(*context, equation);

    switch (equation._type) {

        case VERITAS_IN_PLAJA::Equation::EQ: { PLAJA_ABORT }

        case VERITAS_IN_PLAJA::Equation::GE: {

            std::unique_ptr<VERITAS_IN_PLAJA::EquationConstraint> negation(new VERITAS_IN_PLAJA::EquationConstraint(*this));
            negate_to_le(negation->equation, scalar_offset);
            return negation;

        }

        case VERITAS_IN_PLAJA::Equation::LE: {

            std::unique_ptr<VERITAS_IN_PLAJA::EquationConstraint> negation(new VERITAS_IN_PLAJA::EquationConstraint(*this));
            negate_to_ge(negation->equation, scalar_offset);
            return negation;

        }

        default: PLAJA_ABORT

    }

    PLAJA_ABORT
}

std::unique_ptr<VeritasConstraint> VERITAS_IN_PLAJA::EquationConstraint::move_to_negation() {

    switch (equation._type) {

        case VERITAS_IN_PLAJA::Equation::EQ: { dump(); PLAJA_ABORT }

        case VERITAS_IN_PLAJA::Equation::GE: {
            negate_to_le(equation, compute_negation_offset(*context, equation));
            return std::make_unique<VERITAS_IN_PLAJA::EquationConstraint>(std::move(*this));
        }

        case VERITAS_IN_PLAJA::Equation::LE: {
            negate_to_ge(equation, compute_negation_offset(*context, equation));
            return std::make_unique<VERITAS_IN_PLAJA::EquationConstraint>(std::move(*this));
        }

        default: PLAJA_ABORT

    }

    PLAJA_ABORT
}

/**********************************************************************************************************************/

void VERITAS_IN_PLAJA::EquationConstraint::substitute(VERITAS_IN_PLAJA::Equation& eq, const std::unordered_map<VeritasVarIndex_type, VeritasVarIndex_type>& mapping) {
    for (auto it = eq._addends.begin(); it == eq._addends.end(); ++it) {
        auto it_sub = mapping.find(it->_variable);
        if (it_sub != mapping.end()) { it->_variable = it_sub->second; }
    }
}

/**********************************************************************************************************************/

void VERITAS_IN_PLAJA::EquationConstraint::substitute(const std::unordered_map<VeritasVarIndex_type, VeritasVarIndex_type>& mapping) { substitute(equation, mapping); }

void VERITAS_IN_PLAJA::EquationConstraint::dump() const {
    PLAJA_LOG("EquationConstraint");
    equation.dump();
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/


VERITAS_IN_PLAJA::BoundConstraint::BoundConstraint(VERITAS_IN_PLAJA::Context& c, VeritasVarIndex_type var, VERITAS_IN_PLAJA::Tightening::BoundType type, VeritasFloating_type scalar):
    VeritasConstraint(c)
    , tightening(var, scalar, type) {
}

VERITAS_IN_PLAJA::BoundConstraint::BoundConstraint(VERITAS_IN_PLAJA::Context& c, const VERITAS_IN_PLAJA::Tightening& tightening):
    VeritasConstraint(c)
    , tightening(tightening) {
    PLAJA_ASSERT(tightening._type == VERITAS_IN_PLAJA::Tightening::LB or tightening._type == VERITAS_IN_PLAJA::Tightening::UB)
}

VERITAS_IN_PLAJA::BoundConstraint::~BoundConstraint() = default;

std::unique_ptr<VeritasConstraint> VERITAS_IN_PLAJA::BoundConstraint::construct_equality_constraint(VERITAS_IN_PLAJA::Context& c, VeritasVarIndex_type var, VeritasFloating_type scalar) {
    std::unique_ptr<ConjunctionInVeritas> equality_constraint(new ConjunctionInVeritas(c));
    equality_constraint->add_equality_constraint(var, scalar);
    return equality_constraint;
}

std::unique_ptr<VeritasConstraint> VERITAS_IN_PLAJA::BoundConstraint::construct(VERITAS_IN_PLAJA::Context& c, VeritasVarIndex_type var, VERITAS_IN_PLAJA::Equation::EquationType op, VeritasFloating_type scalar) {

    switch (op) {

        case VERITAS_IN_PLAJA::Equation::EQ: { return construct_equality_constraint(c, var, scalar); }

        case VERITAS_IN_PLAJA::Equation::GE: { return std::make_unique<VERITAS_IN_PLAJA::BoundConstraint>(c, var, VERITAS_IN_PLAJA::Tightening::LB, scalar); }

        case VERITAS_IN_PLAJA::Equation::LE: { return std::make_unique<VERITAS_IN_PLAJA::BoundConstraint>(c, var, VERITAS_IN_PLAJA::Tightening::UB, scalar); }
    }

    PLAJA_ABORT

}

bool VERITAS_IN_PLAJA::BoundConstraint::operator==(const VERITAS_IN_PLAJA::BoundConstraint& other) const {
    return _context() == other._context() and tightening == other.tightening;
}

void VERITAS_IN_PLAJA::BoundConstraint::add_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates) const {
    PLAJA_ASSERT(_context() == query._context() || updates)

    switch (tightening._type) {

        case VERITAS_IN_PLAJA::Tightening::LB: {
            query.tighten_lower_bound(tightening._variable, tightening._value);
            return;
        }

        case VERITAS_IN_PLAJA::Tightening::UB: {
            query.tighten_upper_bound(tightening._variable, tightening._value);
            return;
        }

    }

}

void VERITAS_IN_PLAJA::BoundConstraint::move_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates) { add_to_query(query, updates); } // Nothing to move.

void VERITAS_IN_PLAJA::BoundConstraint::add_to_conjunction(ConjunctionInVeritas& conjunction) const {
    PLAJA_ASSERT(_context() == conjunction._context())
    conjunction.add_bound(tightening);
}

void VERITAS_IN_PLAJA::BoundConstraint::move_to_conjunction(ConjunctionInVeritas& conjunction) { add_to_conjunction(conjunction); } // There is nothin to move.

void VERITAS_IN_PLAJA::BoundConstraint::add_to_disjunction(DisjunctionInVeritas& disjunction) const {
    PLAJA_ASSERT(_context() == disjunction._context())
    disjunction.add_case_split(tightening);
}

void VERITAS_IN_PLAJA::BoundConstraint::move_to_disjunction(DisjunctionInVeritas& disjunction) { add_to_disjunction(disjunction); }

std::unique_ptr<VeritasConstraint> VERITAS_IN_PLAJA::BoundConstraint::copy() const { return std::make_unique<VERITAS_IN_PLAJA::BoundConstraint>(*this); }

std::unique_ptr<VeritasConstraint> VERITAS_IN_PLAJA::BoundConstraint::compute_negation() const {

    const auto is_integer = context->is_marked_integer(tightening._variable) and PLAJA_FLOATS::is_integer(tightening._value, VERITAS_IN_PLAJA::integerPrecision);
    const auto scalar_offset = is_integer ? 1 : VERITAS_IN_PLAJA::strictnessPrecision;

    switch (tightening._type) {

        case VERITAS_IN_PLAJA::Tightening::LB: { return std::make_unique<BoundConstraint>(_context(), tightening._variable, VERITAS_IN_PLAJA::Tightening::UB, tightening._value - scalar_offset); }
        case VERITAS_IN_PLAJA::Tightening::UB: { return std::make_unique<BoundConstraint>(_context(), tightening._variable, VERITAS_IN_PLAJA::Tightening::LB, tightening._value + scalar_offset); }

    }

    PLAJA_ABORT

}

std::unique_ptr<VeritasConstraint> VERITAS_IN_PLAJA::BoundConstraint::move_to_negation() { return compute_negation(); } // There is nothing to move.

/**********************************************************************************************************************/

void VERITAS_IN_PLAJA::BoundConstraint::substitute(VERITAS_IN_PLAJA::Tightening& tightening, const std::unordered_map<VeritasVarIndex_type, VeritasVarIndex_type>& mapping) {
    auto it = mapping.find(tightening._variable);
    if (it != mapping.cend()) { tightening._variable = it->second; }
}

/**********************************************************************************************************************/

void VERITAS_IN_PLAJA::BoundConstraint::substitute(const std::unordered_map<VeritasVarIndex_type, VeritasVarIndex_type>& mapping) { substitute(tightening, mapping); }

bool VERITAS_IN_PLAJA::BoundConstraint::is_bound() const { return true; }

void VERITAS_IN_PLAJA::BoundConstraint::dump() const {
    PLAJA_LOG("BoundConstraint")
    tightening.dump();

}

/***********************************************************************************************************************/
/**********************************************************************************************************************/

DisjunctionInVeritas::DisjunctionInVeritas(VERITAS_IN_PLAJA::Context& c):
    VeritasConstraint(c) {}

DisjunctionInVeritas::~DisjunctionInVeritas() = default;

bool DisjunctionInVeritas::operator==(const DisjunctionInVeritas& other) const { return _context() == other._context() and disjuncts == other.disjuncts; }

void DisjunctionInVeritas::add_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates) const {
    PLAJA_ASSERT(_context() == query._context() || updates)
    if (disjuncts.empty()) {
        context->trivially_false()->move_to_query(query);
        return;
    }

    /* All but one disjunct are entailed unsat. Add this disjunct as constraint. */
    if (disjuncts.size() == 1) {
        auto& case_split = disjuncts[0];
        for (auto bound: case_split.getBoundTightenings()) { 
            VERITAS_IN_PLAJA::BoundConstraint boundc(_context() ,bound); 
            boundc.add_to_query(query);
        }
        for (auto& equation: case_split.getEquations()) { query.add_equation(std::move(equation)); }
        return;
    }

    PLAJA_LOG("Veritas does not support disjunctions to be added to query");
    PLAJA_ABORT
    // query.add_disjunction(new DisjunctionConstraint(std::move(disjuncts)));
}

void DisjunctionInVeritas::move_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates) {
    PLAJA_ASSERT(_context() == query._context() || updates)
    if (disjuncts.empty()) {
        context->trivially_false()->move_to_query(query);
        return;
    }

    /* All but one disjunct are entailed unsat. Add this disjunct as constraint. */
    if (disjuncts.size() == 1) {
        auto& case_split = disjuncts[0];
        for (auto bound: case_split.getBoundTightenings()) { 
            VERITAS_IN_PLAJA::BoundConstraint boundc(_context() ,bound); 
            boundc.add_to_query(query);
        }
        for (auto& equation: case_split.getEquations()) { query.add_equation(std::move(equation)); }
        return;
    }
    for (auto disjunct : disjuncts) {
        query.add_disjunct(disjunct);
    }
    PLAJA_LOG("WARNING: Since none of the benchmarks use disjunction, this has not been exhaustively tested. ");
}

void DisjunctionInVeritas::add_to_conjunction(ConjunctionInVeritas& conjunction) const {
    PLAJA_ASSERT(_context() == conjunction._context())
    PLAJA_ASSERT(disjuncts.size() > 1) // sanity check, otherwise should be conjunction
    conjunction.add_disjunction(*this);
}

void DisjunctionInVeritas::move_to_conjunction(ConjunctionInVeritas& conjunction) {
    PLAJA_ASSERT(_context() == conjunction._context())
    PLAJA_ASSERT(disjuncts.size() > 1) // sanity check, otherwise should be conjunction
    conjunction.add_disjunction(std::move(*this));
}

void DisjunctionInVeritas::add_to_disjunction(DisjunctionInVeritas& disjunction) const {
    PLAJA_ASSERT(_context() == disjunction._context())
    disjunction.reserve_for_additional(disjuncts.size());
    for (const auto& case_split: disjuncts) { disjunction.add_case_split(case_split); }
}

void DisjunctionInVeritas::move_to_disjunction(DisjunctionInVeritas& disjunction) {
    PLAJA_ASSERT(_context() == disjunction._context())

    disjunction.reserve_for_additional(disjuncts.size());
    for (auto& case_split: disjuncts) { disjunction.add_case_split(std::move(case_split)); }

    PLAJA_ASSERT(std::all_of(disjuncts.begin(), disjuncts.end(), [](const VERITAS_IN_PLAJA::Disjunct& case_split) { return case_split.getBoundTightenings().size() + case_split.getEquations().size() == 0; }))

}

std::unique_ptr<VeritasConstraint> DisjunctionInVeritas::copy() const { return std::make_unique<DisjunctionInVeritas>(*this); }

// not ( (e_1 && b_1) || (e_2 && b_2) ) -> not (e_1 && b_1) && not (e_2 && b_2) -> (not e_1 || not b_1) and (not e_2 || not b_2)
std::unique_ptr<VeritasConstraint> DisjunctionInVeritas::compute_negation() const {
    std::unique_ptr<ConjunctionInVeritas> conjunction(new ConjunctionInVeritas(_context()));

    for (const auto& case_split: disjuncts) {

        const auto& bounds = case_split.getBoundTightenings();
        const auto& equations = case_split.getEquations();

        if (bounds.size() + equations.size() == 1) { // Special case: Only one atom, that may be added as equation/bound constraint.

            for (const auto& bound: bounds) { VERITAS_IN_PLAJA::BoundConstraint(_context(), bound).compute_negation()->move_to_conjunction(*conjunction); }
            for (const auto& equation: equations) { VERITAS_IN_PLAJA::EquationConstraint(_context(), equation).compute_negation()->move_to_conjunction(*conjunction); }

        } else { // General case: case split becomes a disjunction.

            DisjunctionInVeritas disjunction(_context());

            for (const auto& bound: bounds) { VERITAS_IN_PLAJA::BoundConstraint(_context(), bound).compute_negation()->move_to_disjunction(disjunction); }
            for (const auto& equation: equations) { VERITAS_IN_PLAJA::EquationConstraint(_context(), equation).compute_negation()->move_to_disjunction(disjunction); }

            disjunction.move_to_conjunction(*conjunction);

        }

    }

    return conjunction;
}

std::unique_ptr<VeritasConstraint> DisjunctionInVeritas::move_to_negation() {
    std::unique_ptr<ConjunctionInVeritas> conjunction(new ConjunctionInVeritas(_context()));

    for (auto& case_split: disjuncts) {

        auto& bounds = case_split.getBoundTightenings();
        auto& equations = case_split.getEquations();

        if (bounds.size() + equations.size() == 1) { // Special case: Only one atom, that may be added as equation/bound constraint.

            for (auto& bound: bounds) { VERITAS_IN_PLAJA::BoundConstraint(_context(), bound).move_to_negation()->move_to_conjunction(*conjunction); }
            for (auto& equation: equations) { VERITAS_IN_PLAJA::EquationConstraint(_context(), equation).move_to_negation()->move_to_conjunction(*conjunction); }

        } else { // General case: case split becomes a disjunction.

            DisjunctionInVeritas disjunction(_context());

            for (auto& bound: bounds) { VERITAS_IN_PLAJA::BoundConstraint(_context(), bound).move_to_negation()->move_to_disjunction(disjunction); }
            for (auto& equation: equations) { VERITAS_IN_PLAJA::EquationConstraint(_context(), equation).move_to_negation()->move_to_disjunction(disjunction); }

            disjunction.move_to_conjunction(*conjunction);

        }

    }

    return conjunction;
}

void DisjunctionInVeritas::substitute(const std::unordered_map<VeritasVarIndex_type, VeritasVarIndex_type>& mapping) {

    for (auto& case_split: disjuncts) {

        std::unordered_set<VeritasVarIndex_type> participating_vars;

        for (const auto& bound: case_split.getBoundTightenings()) { participating_vars.insert(bound._variable); }

        for (const auto& eq: case_split.getEquations()) { for (auto var: eq.getParticipatingVariables()) { participating_vars.insert(var); } }

        for (auto var: participating_vars) {
            auto it_sub = mapping.find(var);
            if (it_sub != mapping.cend()) { case_split.updateVariableIndex(var, it_sub->second); }
        }

    }
}

bool DisjunctionInVeritas::is_linear() const { return false; }

void DisjunctionInVeritas::dump() const {
    PLAJA_LOG("Disjunction in Veritas");
    for (const auto& case_split: disjuncts) { case_split.dump(); }
}

void DisjunctionInVeritas::add_case_split(const VERITAS_IN_PLAJA::Equation& equation) {
    disjuncts.emplace_back();
    disjuncts.back().addEquation(equation);
}

void DisjunctionInVeritas::add_case_split(VERITAS_IN_PLAJA::Equation&& equation) {
    disjuncts.emplace_back();
    disjuncts.back().addEquation(std::move(equation));
}

void DisjunctionInVeritas::add_case_split(const VERITAS_IN_PLAJA::Tightening& tightening) {
    disjuncts.emplace_back();
    disjuncts.back().storeBoundTightening(tightening);
}

/**********************************************************************************************************************/

void DisjunctionInVeritas::compute_global_bounds(std::unordered_map<VeritasVarIndex_type, VeritasFloating_type>& lower_bounds_global, std::unordered_map<VeritasVarIndex_type, VeritasFloating_type>& upper_bounds_global) const {
    PLAJA_ASSERT(lower_bounds_global.empty())
    PLAJA_ASSERT(upper_bounds_global.empty())

    auto disjunct_index = 0;

    for (const auto& disjunct: disjuncts) {

        // Collect bounds of disjunct:

        std::unordered_map<VeritasVarIndex_type, VeritasFloating_type> lower_bounds_disjunct;
        std::unordered_map<VeritasVarIndex_type, VeritasFloating_type> upper_bounds_disjunct;

        for (const auto& bound: disjunct.getBoundTightenings()) {

            switch (bound._type) {

                case VERITAS_IN_PLAJA::Tightening::LB: {
                    auto it = lower_bounds_disjunct.find(bound._variable);

                    if (it == lower_bounds_disjunct.end()) {
                        lower_bounds_disjunct.emplace(bound._variable, bound._value);
                    } else {
                        it->second = std::max(it->second, bound._value); // Use tightest bound in case split (i.e., there may be redundancies like x >= 0 and x >= 2).
                    }

                    break;
                }

                case VERITAS_IN_PLAJA::Tightening::UB: {
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
namespace CONJUNCTION_IN_VERITAS {

    void compute_case_splits_recursively(DisjunctionInVeritas& target_disjunction, VERITAS_IN_PLAJA::Disjunct&& constructor, std::list<DisjunctionInVeritas>::const_iterator it, const std::list<DisjunctionInVeritas>::const_iterator& end) { // NOLINT(misc-no-recursion)

        if (it == end) {
            target_disjunction.add_case_split(std::move(constructor));
        } else {
            for (const auto& case_split: it->get_disjuncts()) {

                VERITAS_IN_PLAJA::Disjunct constructor_tmp(constructor);
                for (const auto& tightening: case_split.getBoundTightenings()) { constructor_tmp.storeBoundTightening(tightening); }
                for (const auto& equation: case_split.getEquations()) { constructor_tmp.addEquation(equation); }

                compute_case_splits_recursively(target_disjunction, std::move(constructor_tmp), ++it, end);
                --it; // return to old position
            }
        }
    }
}
/**********************************************************************************************************************/

ConjunctionInVeritas::ConjunctionInVeritas(VERITAS_IN_PLAJA::Context& c):
    VeritasConstraint(c) {}

ConjunctionInVeritas::~ConjunctionInVeritas() = default;

bool ConjunctionInVeritas::operator==(const ConjunctionInVeritas& other) const { return _context() == other._context() and equations == other.equations and bounds == other.bounds; }

void ConjunctionInVeritas::add_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates) const {
    PLAJA_ASSERT(_context() == query._context())
    for (const auto& equation: equations) { 
        if (updates) {
            query.add_update(equation);
        } else {
            query.add_equation(equation);
        } 
    }
    for (const auto& bound: bounds) {
        VERITAS_IN_PLAJA::BoundConstraint boundc(_context() ,bound); 
        boundc.add_to_query(query, updates);
    }
}

void ConjunctionInVeritas::move_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates) {
    PLAJA_ASSERT(_context() == query._context())
    for (const auto& eq: equations) {
        updates ? query.add_update(eq) : query.add_equation(eq);
    }
    for (const auto& bound: bounds) {
        VERITAS_IN_PLAJA::BoundConstraint boundc(_context() ,bound); 
        boundc.add_to_query(query);
    }
}

void ConjunctionInVeritas::add_to_conjunction(ConjunctionInVeritas& conjunction) const {
    PLAJA_ASSERT(_context() == conjunction._context())
    for (const auto& equation: equations) { conjunction.add_equation(equation); }
    for (const auto& bound: bounds) { conjunction.add_bound(bound); }
}

void ConjunctionInVeritas::move_to_conjunction(ConjunctionInVeritas& conjunction) {
    PLAJA_ASSERT(_context() == conjunction._context())

    conjunction.equations.splice(conjunction.equations.end(), equations);
    PLAJA_ASSERT(equations.empty())

    conjunction.bounds.splice(conjunction.bounds.end(), bounds);
    PLAJA_ASSERT(bounds.empty())
}

std::unique_ptr<VeritasConstraint> ConjunctionInVeritas::compute_negation() const {

    std::unique_ptr<DisjunctionInVeritas> disjunction(new DisjunctionInVeritas(_context()));

    for (const auto& equation: equations) { VERITAS_IN_PLAJA::EquationConstraint(_context(), equation).move_to_negation()->move_to_disjunction(*disjunction); }
    for (const auto& bound: bounds) { PLAJA_UTILS::cast_unique<VERITAS_IN_PLAJA::BoundConstraint>(VERITAS_IN_PLAJA::BoundConstraint(_context(), bound).move_to_negation())->move_to_disjunction(*disjunction); }
    for (const auto& disjunction_: disjunctions) { disjunction_.compute_negation()->move_to_disjunction(*disjunction); }

    return disjunction;

}

std::unique_ptr<VeritasConstraint> ConjunctionInVeritas::copy() const { return std::make_unique<ConjunctionInVeritas>(*this); }

void ConjunctionInVeritas::substitute(const std::unordered_map<VeritasVarIndex_type, VeritasVarIndex_type>& mapping) {
    for (auto& bound: bounds) { VERITAS_IN_PLAJA::BoundConstraint::substitute(bound, mapping); }
    for (auto& equation: equations) { VERITAS_IN_PLAJA::EquationConstraint::substitute(equation, mapping); }
}

bool ConjunctionInVeritas::is_linear() const { return true; }

void ConjunctionInVeritas::dump() const {
    PLAJA_LOG("Conjunction in Veritas")
    for (const auto& equation: equations) { equation.dump(); }
    for (const auto& bound: bounds) { bound.dump(); }
}

void ConjunctionInVeritas::add_equality_constraint(VeritasVarIndex_type var, VeritasFloating_type scalar) {
    add_bound(var, VERITAS_IN_PLAJA::Tightening::LB, scalar);
    add_bound(var, VERITAS_IN_PLAJA::Tightening::UB, scalar);
}

// e_1 && b_1 && ((e_2 && b_2) || (e_3 && b_3)) -> ( e_1 && b_1 && e_2 && b_2 ) || ( e_1 && b_1 && e_3 && b_3 )
void ConjunctionInVeritas::add_to_disjunction(DisjunctionInVeritas& disjunction) const {
    PLAJA_ASSERT(_context() == disjunction._context())
    PLAJA_LOG("Not supported adding disjunction to conjunction yet")
    PLAJA_ABORT
    std::size_t num_disjuncts = 1;
    // for (const auto& disjunction_ref: disjunctions) { num_disjuncts *= disjunction_ref.get_disjuncts().size(); }
    disjunction.reserve_for_additional(num_disjuncts);

    VERITAS_IN_PLAJA::Disjunct case_split_base;
    for (const auto& equation: equations) { case_split_base.addEquation(equation); }
    for (const auto& bound: bounds) { case_split_base.storeBoundTightening(bound); }

    // CONJUNCTION_IN_VERITAS::compute_case_splits_recursively(disjunction, std::move(case_split_base), disjunctions.begin(), disjunctions.cend());

}

void ConjunctionInVeritas::move_to_disjunction(DisjunctionInVeritas& disjunction) {
    PLAJA_ASSERT(_context() == disjunction._context())
    std::size_t num_disjuncts = 1;
    for (const auto& disjunction_ref: disjunctions) { num_disjuncts *= disjunction_ref.get_disjuncts().size(); }
    disjunction.reserve_for_additional(num_disjuncts);

    VERITAS_IN_PLAJA::Disjunct case_split_base;
    for (auto equation : equations) {
        case_split_base.addEquation(equation);
    }
    for (auto bound : bounds) {
        case_split_base.storeBoundTightening(bound);
    }
    CONJUNCTION_IN_VERITAS::compute_case_splits_recursively(disjunction, std::move(case_split_base), disjunctions.begin(), disjunctions.cend());
}

void add_indicator_var(veritas::Tree& tree, std::string prefix, VeritasVarIndex_type indicator_var) {
    tree.split(tree[prefix.c_str()], {indicator_var, 1});
    tree.leaf_value(tree[(prefix + "l").c_str()], 0) = 0;
    tree.leaf_value(tree[(prefix + "r").c_str()], 0) = VERITAS_IN_PLAJA::negative_infinity;
    return ;
}

veritas::Tree ConjunctionInVeritas::get_bound_tightenings_tree(std::list<VERITAS_IN_PLAJA::Tightening> tightenings, VeritasVarIndex_type indicator_var) const {
    /**
     * Given a list of tightening constraints, t1 = A <= 1, t2 = B >= 1 ... and an indicator variable I, create a tree such that I >= 1 iff t1 and t2 and t3 and ...
    */
    veritas::Tree tree(1);
    std::string prefix = "";
    for (const auto bound: tightenings) {
            veritas::FloatT val = bound._type == VERITAS_IN_PLAJA::Tightening::BoundType::UB ? bound._value + 1 : bound._value;
            tree.split(tree[prefix.c_str()], {static_cast<veritas::FeatId>(bound._variable), val});
            add_indicator_var(tree, prefix + (bound._type == VERITAS_IN_PLAJA::Tightening::BoundType::UB ? "r" : "l"), indicator_var);
            prefix += bound._type == VERITAS_IN_PLAJA::Tightening::BoundType::UB ? "l" : "r";
    }
    tree.split(tree[prefix.c_str()], {indicator_var, 1});
    tree.leaf_value(tree[(prefix + "l").c_str()], 0) = VERITAS_IN_PLAJA::negative_infinity;
    tree.leaf_value(tree[(prefix + "r").c_str()], 0) = 0;
    return tree;
}

veritas::AddTree ConjunctionInVeritas::add_operator_applicability(ActionOpID_type action_op, veritas::AddTree policy) const {
    /**
     * Given a guard of form: e1: x = y + c, t1: B < 1
     * Create an addTree with new indicator variables such that: e1 = 1 iff I >= 1
     * Create a tree such that: t1 and I >= 1 iff O >= 1.
    */
    veritas::AddTree trees(1, veritas::AddTreeType::REGR);
    std::list<VERITAS_IN_PLAJA::Tightening> tightenings = bounds;
    for (auto equation : equations) {
        auto indicator_var = context->add_eq_aux_var();
        tightenings.emplace_back(VERITAS_IN_PLAJA::Tightening(indicator_var, 1, VERITAS_IN_PLAJA::Tightening::BoundType::LB));
        auto app = equation.to_applicability_trees(*context, policy, indicator_var);
        trees.add_trees(app);
    }
    auto operator_aux_var = context->add_operator_indicator_var(action_op);
    trees.add_tree(get_bound_tightenings_tree(tightenings, operator_aux_var));
    return trees;
}
