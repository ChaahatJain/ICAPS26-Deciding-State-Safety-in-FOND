//
// This file is part of PlaJA (2019 - 2024).
// Copyright (c) Marcel Vinzent (2024).
//

#ifndef PLAJA_CONSTRAINT_Z3_H
#define PLAJA_CONSTRAINT_Z3_H

#include <z3++.h>
#include "base/smt_constraint.h"
#include "forward_smt_z3.h"

/**
* Encapsulation of z3::expr expression.
*/

namespace Z3_IN_PLAJA {

    class Constraint: public PLAJA::SmtConstraint {

    public:
        Constraint() = default;
        ~Constraint() override;
        DELETE_CONSTRUCTOR(Constraint)

        [[nodiscard]] virtual z3::context& get_cxt() const = 0;

        [[nodiscard]] virtual z3::expr to_expr() const = 0;

        [[nodiscard]] std::unique_ptr<ConstraintExpr> to_expr_constraint() const;

        [[nodiscard]] virtual z3::expr move_to_expr() const = 0;

        [[nodiscard]] std::unique_ptr<ConstraintExpr> move_to_expr_constraint() const;

        [[nodiscard]] virtual std::unique_ptr<Constraint> negate() const = 0;

    };

    /******************************************************************************************************************/

    class ConstraintExpr: public Constraint {

    private:
        z3::expr expr;

    public:
        explicit ConstraintExpr(z3::expr e):
            expr(std::move(e)) {
        }

        ~ConstraintExpr() override;
        DELETE_CONSTRUCTOR(ConstraintExpr)

        [[nodiscard]] inline const z3::expr& operator()() const { return expr; }

        [[nodiscard]] z3::context& get_cxt() const final;

        [[nodiscard]] z3::expr to_expr() const final;

        [[nodiscard]] z3::expr move_to_expr() const final;

        [[nodiscard]] std::unique_ptr<Constraint> negate() const override;

        void add_to_solver(PLAJA::SmtSolver& solver) const final;

        void add_negation_to_solver(PLAJA::SmtSolver& solver) const final;

    };

    /******************************************************************************************************************/

    class ConstraintTrivial final: public ConstraintExpr {

    private:
        bool value;

    public:
        explicit ConstraintTrivial(z3::context& cxt, bool value):
            ConstraintExpr(cxt.bool_val(value))
            , value(value) {
        }

        ~ConstraintTrivial() override;
        DELETE_CONSTRUCTOR(ConstraintTrivial)

        [[nodiscard]] bool is_trivially_true() const override;

        [[nodiscard]] bool is_trivially_false() const override;

        [[nodiscard]] std::unique_ptr<Constraint> negate() const override;

    };

    /******************************************************************************************************************/

    class ConstraintVec final: public Constraint {

    private:
        z3::expr_vector vec;

    public:
        explicit ConstraintVec(z3::context& c):
            vec(c) {
        }

        explicit ConstraintVec(const z3::expr_vector& v):
            vec(v) {
        }

        ~ConstraintVec() override;
        DELETE_CONSTRUCTOR(ConstraintVec)

        [[nodiscard]] inline const z3::expr_vector& operator()() const { return vec; }

        inline void append(const z3::expr& e) { return vec.push_back(e); }

        inline void append(const z3::expr_vector& v) { for (const auto& e: v) { vec.push_back(e); } }

        void append(std::unique_ptr<Z3_IN_PLAJA::ConstraintVec>&& v);

        [[nodiscard]] z3::expr to_expr() const override;

        [[nodiscard]] z3::expr move_to_expr() const override;

        [[nodiscard]] z3::context& get_cxt() const final;

        [[nodiscard]] std::unique_ptr<Constraint> negate() const final;

        void add_to_solver(PLAJA::SmtSolver& solver) const override;

        void add_negation_to_solver(PLAJA::SmtSolver& solver) const override;

    };

}

#endif //PLAJA_CONSTRAINT_Z3_H
