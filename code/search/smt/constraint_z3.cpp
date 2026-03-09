//
// This file is part of PlaJA (2019 - 2024).
// Copyright (c) Marcel Vinzent (2024).
//

#include "constraint_z3.h"
#include "solver/smt_solver_z3.h"
#include "visitor/extern/to_z3_visitor.h"

Z3_IN_PLAJA::Constraint::~Constraint() = default;

std::unique_ptr<Z3_IN_PLAJA::ConstraintExpr> Z3_IN_PLAJA::Constraint::to_expr_constraint() const { return std::make_unique<Z3_IN_PLAJA::ConstraintExpr>(to_expr()); }

std::unique_ptr<Z3_IN_PLAJA::ConstraintExpr> Z3_IN_PLAJA::Constraint::move_to_expr_constraint() const { return std::make_unique<Z3_IN_PLAJA::ConstraintExpr>(move_to_expr()); }

/**********************************************************************************************************************/

Z3_IN_PLAJA::ConstraintExpr::~ConstraintExpr() = default;

z3::context& Z3_IN_PLAJA::ConstraintExpr::get_cxt() const { return operator()().ctx(); }

z3::expr Z3_IN_PLAJA::ConstraintExpr::to_expr() const { return operator()(); }

z3::expr Z3_IN_PLAJA::ConstraintExpr::move_to_expr() const { return std::move(expr); }

std::unique_ptr<Z3_IN_PLAJA::Constraint> Z3_IN_PLAJA::ConstraintExpr::negate() const { return std::make_unique<ConstraintExpr>(!expr); }

void Z3_IN_PLAJA::ConstraintExpr::add_to_solver(PLAJA::SmtSolver& solver) const { PLAJA_UTILS::cast_ref<Z3_IN_PLAJA::SMTSolver>(solver).add(expr); }

void Z3_IN_PLAJA::ConstraintExpr::add_negation_to_solver(PLAJA::SmtSolver& solver) const { PLAJA_UTILS::cast_ref<Z3_IN_PLAJA::SMTSolver>(solver).add(!expr); }

/**********************************************************************************************************************/

Z3_IN_PLAJA::ConstraintTrivial::~ConstraintTrivial() = default;

bool Z3_IN_PLAJA::ConstraintTrivial::is_trivially_true() const { return value; }

bool Z3_IN_PLAJA::ConstraintTrivial::is_trivially_false() const { return not value; }

std::unique_ptr<Z3_IN_PLAJA::Constraint> Z3_IN_PLAJA::ConstraintTrivial::negate() const { return std::make_unique<ConstraintTrivial>(get_cxt(), not value); }

/**********************************************************************************************************************/

Z3_IN_PLAJA::ConstraintVec::~ConstraintVec() = default;

/* */

void Z3_IN_PLAJA::ConstraintVec::append(std::unique_ptr<Z3_IN_PLAJA::ConstraintVec>&& v) { append(v->operator()()); }

/* */

z3::expr Z3_IN_PLAJA::ConstraintVec::to_expr() const { return Z3_IN_PLAJA::to_conjunction(vec); }

z3::expr Z3_IN_PLAJA::ConstraintVec::move_to_expr() const { return to_expr(); }

z3::context& Z3_IN_PLAJA::ConstraintVec::get_cxt() const { return vec.ctx(); }

std::unique_ptr<Z3_IN_PLAJA::Constraint> Z3_IN_PLAJA::ConstraintVec::negate() const { return std::make_unique<ConstraintExpr>(!to_expr()); }

void Z3_IN_PLAJA::ConstraintVec::add_to_solver(PLAJA::SmtSolver& solver) const { PLAJA_UTILS::cast_ref<Z3_IN_PLAJA::SMTSolver>(solver).add(vec); }

void Z3_IN_PLAJA::ConstraintVec::add_negation_to_solver(PLAJA::SmtSolver& solver) const { PLAJA_UTILS::cast_ref<Z3_IN_PLAJA::SMTSolver>(solver).add(PLAJA_UTILS::cast_unique<ConstraintExpr>(negate())->operator()()); }