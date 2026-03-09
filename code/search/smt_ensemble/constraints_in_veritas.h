//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_CONSTRAINTS_IN_VERITAS_H
#define PLAJA_CONSTRAINTS_IN_VERITAS_H

#include <memory>
#include <unordered_map>
#include "../../parser/using_parser.h"
#include "../../assertions.h"
#include "../smt/base/smt_constraint.h"
#include "forward_smt_veritas.h"
#include "using_veritas.h"
#include "predicates/Equation.h"
#include "predicates/Tightening.h"
#include "predicates/Disjunct.h"
#include "../../utils/utils.h"
#include <list>
#include "../using_search.h"
/**
 * Word of caution: "Move" functions do invalidate the object.
 * They aim to move as much data of the called object to the created object to avoid copying.
 */

class VeritasConstraint : public PLAJA::SmtConstraint {
protected:
    VERITAS_IN_PLAJA::Context* context;
    explicit VeritasConstraint(VERITAS_IN_PLAJA::Context& c);

public:
    virtual ~VeritasConstraint() = 0;
    DEFAULT_CONSTRUCTOR_ONLY(VeritasConstraint)
    DELETE_ASSIGNMENT(VeritasConstraint)

    [[nodiscard]] inline VERITAS_IN_PLAJA::Context& _context() const { return *context; }

    virtual void add_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates = false) const = 0;
    virtual void move_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates = false) = 0;
    virtual void add_to_conjunction(ConjunctionInVeritas& conjunction) const = 0;
    virtual void move_to_conjunction(ConjunctionInVeritas& conjunction) = 0;
    virtual void add_to_disjunction(DisjunctionInVeritas& disjunction) const = 0;
    virtual void move_to_disjunction(DisjunctionInVeritas& disjunction) = 0;

    [[nodiscard]] virtual std::unique_ptr<VeritasConstraint> copy() const = 0;
    [[nodiscard]] virtual std::unique_ptr<VeritasConstraint> compute_negation() const = 0;
    [[nodiscard]] virtual std::unique_ptr<VeritasConstraint> move_to_negation() = 0;
    virtual void substitute(const std::unordered_map<VeritasVarIndex_type, VeritasVarIndex_type>& mapping) = 0;
    [[nodiscard]] std::unique_ptr<VeritasConstraint> to_substitute(const std::unordered_map<VeritasVarIndex_type, VeritasVarIndex_type>& mapping) const;

    [[nodiscard]] virtual bool is_bound() const;
    [[nodiscard]] virtual bool is_linear() const;

    virtual void dump() const = 0;

    /* Generic SMT interface: */
    void add_to_solver(PLAJA::SmtSolver& solver) const override;
    void add_negation_to_solver(PLAJA::SmtSolver& solver) const override;
};

namespace VERITAS_IN_PLAJA {
// Veritas constraints for a single predicate
class PredicateConstraint final {
    friend class ToVeritasBoundPredicateTest; //
    friend class ToVeritasConjunctionPredicateTest; //
    friend class ToVeritasEquationPredicateTest;

private:
    std::unique_ptr<VeritasConstraint> constraint;
    std::unique_ptr<VeritasConstraint> negation;
public:
    explicit PredicateConstraint(std::unique_ptr<VeritasConstraint>&& constraint_);
    ~PredicateConstraint();
    DELETE_CONSTRUCTOR(PredicateConstraint)

    inline void add_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates = false) const { constraint->add_to_query(query, updates); }

    inline void move_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates = false) { constraint->move_to_query(query, updates); }

    inline void add_negation_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates = false) const { negation->add_to_query(query, updates); }

    [[nodiscard]] inline bool is_bound() const { return constraint->is_bound(); }

    [[nodiscard]] inline bool is_linear() const { return constraint->is_linear(); }

    void dump() const;
};

/**********************************************************************************************************************/
class EquationConstraint final: public VeritasConstraint {
private:
    VERITAS_IN_PLAJA::Equation equation;

    /* auxiliaries */

    [[nodiscard]] static VeritasFloating_type compute_negation_offset(const VERITAS_IN_PLAJA::Context& c, const VERITAS_IN_PLAJA::Equation& eq);
public:
    static void negate_to_ge(VERITAS_IN_PLAJA::Equation& eq, VeritasFloating_type scalar_offset);
    static void negate_to_le(VERITAS_IN_PLAJA::Equation& eq, VeritasFloating_type scalar_offset);
    static void negate_non_eq(const VERITAS_IN_PLAJA::Context& c, VERITAS_IN_PLAJA::Equation& eq);
    [[nodiscard]] static std::unique_ptr<DisjunctionInVeritas> negate_eq(VERITAS_IN_PLAJA::Context& c, VERITAS_IN_PLAJA::Equation eq);

    static void substitute(VERITAS_IN_PLAJA::Equation& eq, const std::unordered_map<VeritasVarIndex_type, VeritasVarIndex_type>& mapping);

    /* */

    explicit EquationConstraint(VERITAS_IN_PLAJA::Context& c, VERITAS_IN_PLAJA::Equation eq);
    ~EquationConstraint() final;
    DEFAULT_CONSTRUCTOR_ONLY(EquationConstraint)
    DELETE_ASSIGNMENT(EquationConstraint)

    /* getter */
    [[nodiscard]] inline const VERITAS_IN_PLAJA::Equation& get_equation() const { return equation; }

    [[nodiscard]] bool operator==(const EquationConstraint& other) const;

    // override:
    void add_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates = false) const override;
    void move_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates = false) override;
    void add_to_conjunction(ConjunctionInVeritas& conjunction) const override;
    void move_to_conjunction(ConjunctionInVeritas& conjunction) override;
    void add_to_disjunction(DisjunctionInVeritas& disjunction) const override;
    void move_to_disjunction(DisjunctionInVeritas& disjunction) override;

    [[nodiscard]] std::unique_ptr<VeritasConstraint> copy() const override;
    [[nodiscard]] std::unique_ptr<VeritasConstraint> compute_negation() const override;
    [[nodiscard]] std::unique_ptr<VeritasConstraint> move_to_negation() override;
    void substitute(const std::unordered_map<VeritasVarIndex_type, VeritasVarIndex_type>& mapping) override;

    void dump() const override;

};

/**********************************************************************************************************************/

class BoundConstraint final: public VeritasConstraint {

private:
    VERITAS_IN_PLAJA::Tightening tightening;

public:
    BoundConstraint(VERITAS_IN_PLAJA::Context& c, VeritasVarIndex_type var, VERITAS_IN_PLAJA::Tightening::BoundType type, VeritasFloating_type scalar);
    BoundConstraint(VERITAS_IN_PLAJA::Context& c, const VERITAS_IN_PLAJA::Tightening& tightening);
    ~BoundConstraint() final;
    DEFAULT_CONSTRUCTOR_ONLY(BoundConstraint)
    DELETE_ASSIGNMENT(BoundConstraint)

    /* auxiliary */

    [[nodiscard]] static std::unique_ptr<VeritasConstraint> construct_equality_constraint(VERITAS_IN_PLAJA::Context& c, VeritasVarIndex_type var, VeritasFloating_type scalar);
    [[nodiscard]] static std::unique_ptr<VeritasConstraint> construct(VERITAS_IN_PLAJA::Context& c, VeritasVarIndex_type var, VERITAS_IN_PLAJA::Equation::EquationType op, VeritasFloating_type scalar);

    static void substitute(VERITAS_IN_PLAJA::Tightening& tightening, const std::unordered_map<VeritasVarIndex_type, VeritasVarIndex_type>& mapping);

    /* getter */

    [[nodiscard]] inline VeritasVarIndex_type get_var() const { return tightening._variable; }

    [[nodiscard]] inline VERITAS_IN_PLAJA::Tightening::BoundType get_type() const { return tightening._type; }

    [[nodiscard]] inline VeritasFloating_type get_value() const { return tightening._value; }

    /* */

    [[nodiscard]] bool operator==(const BoundConstraint& other) const;

    void add_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates = false) const override;
    void move_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates = false) override;
    void add_to_conjunction(ConjunctionInVeritas& conjunction) const override;
    void move_to_conjunction(ConjunctionInVeritas& conjunction) override;
    void add_to_disjunction(DisjunctionInVeritas& disjunction) const override;
    void move_to_disjunction(DisjunctionInVeritas& disjunction) override;

    [[nodiscard]] std::unique_ptr<VeritasConstraint> copy() const override;
    [[nodiscard]] std::unique_ptr<VeritasConstraint> compute_negation() const override;
    [[nodiscard]] std::unique_ptr<VeritasConstraint> move_to_negation() override;
    void substitute(const std::unordered_map<VeritasVarIndex_type, VeritasVarIndex_type>& mapping) override;

    [[nodiscard]] bool is_bound() const override;

    void dump() const override;

};
}

/***********************************************************************************************************************/

class DisjunctionInVeritas final: public VeritasConstraint {
    friend VERITAS_IN_PLAJA::EquationConstraint; // Faster construction ...
    friend VERITAS_IN_PLAJA::BoundConstraint; //

private:

    std::vector<VERITAS_IN_PLAJA::Disjunct> disjuncts;
    /** Compute bounds that hold globally over all disjuncts. */
    void compute_global_bounds(std::unordered_map<VeritasVarIndex_type, VeritasFloating_type>& lower_bounds, std::unordered_map<VeritasVarIndex_type, VeritasFloating_type>& upper_bounds) const;

public:
    explicit DisjunctionInVeritas(VERITAS_IN_PLAJA::Context& c);
    ~DisjunctionInVeritas() final;
    DEFAULT_CONSTRUCTOR_ONLY(DisjunctionInVeritas)
    DELETE_ASSIGNMENT(DisjunctionInVeritas)

    [[nodiscard]] bool operator==(const DisjunctionInVeritas& other) const;

    void add_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates = false) const override;
    void move_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates = false) override;
    void add_to_conjunction(ConjunctionInVeritas& conjunction) const override;
    void move_to_conjunction(ConjunctionInVeritas& conjunction) override;
    void add_to_disjunction(DisjunctionInVeritas& disjunction) const override;
    void move_to_disjunction(DisjunctionInVeritas& disjunction) override;

    [[nodiscard]] std::unique_ptr<VeritasConstraint> copy() const override;
    [[nodiscard]] std::unique_ptr<VeritasConstraint> compute_negation() const override;
    [[nodiscard]] std::unique_ptr<VeritasConstraint> move_to_negation() override;
    void substitute(const std::unordered_map<VeritasVarIndex_type, VeritasVarIndex_type>& mapping) override;

    [[nodiscard]] bool is_linear() const override;

    void dump() const override;

    /* */

    inline void reserve_for_additional(std::size_t additional_disjuncts) { disjuncts.reserve(disjuncts.size() + additional_disjuncts); }

    inline void shrink_to_fit() { disjuncts.shrink_to_fit(); }

    inline void add_case_split(const VERITAS_IN_PLAJA::Disjunct& case_split) { disjuncts.push_back(case_split); }

    inline void add_case_split(VERITAS_IN_PLAJA::Disjunct&& case_split) { disjuncts.push_back(std::move(case_split)); }

    void add_case_split(const VERITAS_IN_PLAJA::Equation& equation);

    void add_case_split(VERITAS_IN_PLAJA::Equation&& equation);

    void add_case_split(const VERITAS_IN_PLAJA::Tightening& tightening);

    [[nodiscard]] inline const std::vector<VERITAS_IN_PLAJA::Disjunct>& get_disjuncts() const { return disjuncts; }

    [[nodiscard]] inline bool empty() const { return disjuncts.empty(); }

    /**
     * Factor out min-max bounds.
     * Move structures as far as possible.
     */

};


/**********************************************************************************************************************/

class ConjunctionInVeritas final: public VeritasConstraint {
    friend VERITAS_IN_PLAJA::BoundConstraint; // Faster construction ...

private:
    std::list<VERITAS_IN_PLAJA::Equation> equations;
    std::list<VERITAS_IN_PLAJA::Tightening> bounds;
    std::list<DisjunctionInVeritas> disjunctions;

public:
    explicit ConjunctionInVeritas(VERITAS_IN_PLAJA::Context& c);
    ~ConjunctionInVeritas() final;
    DEFAULT_CONSTRUCTOR_ONLY(ConjunctionInVeritas)
    DELETE_ASSIGNMENT(ConjunctionInVeritas)

    [[nodiscard]] bool operator==(const ConjunctionInVeritas& other) const;

    void add_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates = false) const override;
    void move_to_query(VERITAS_IN_PLAJA::QueryConstructable& query, bool updates = false) override;
    void add_to_conjunction(ConjunctionInVeritas& conjunction) const override;
    void move_to_conjunction(ConjunctionInVeritas& conjunction) override;
    void add_to_disjunction(DisjunctionInVeritas& disjunction) const override;
    void move_to_disjunction(DisjunctionInVeritas& disjunction) override;
    [[nodiscard]] bool is_linear() const override;

    [[nodiscard]] std::unique_ptr<VeritasConstraint> copy() const override;
    [[nodiscard]] std::unique_ptr<VeritasConstraint> compute_negation() const override;
    [[nodiscard]] std::unique_ptr<VeritasConstraint> move_to_negation() override { PLAJA_LOG("Disjunction not supported yet"); PLAJA_ABORT;};
    void substitute(const std::unordered_map<VeritasVarIndex_type, VeritasVarIndex_type>& mapping) override;

    void dump() const override;

    /* */
    void add_equality_constraint(VeritasVarIndex_type var, VeritasFloating_type scalar);

    inline void add_equation(const VERITAS_IN_PLAJA::Equation& equation) { equations.push_back(equation); }

    inline void add_equation(VERITAS_IN_PLAJA::Equation&& equation) { equations.push_back(std::move(equation)); }

    inline void add_bound(const VERITAS_IN_PLAJA::Tightening& bound) { bounds.push_back(bound); }

    inline void add_bound(VeritasVarIndex_type var, VERITAS_IN_PLAJA::Tightening::BoundType type, VeritasFloating_type scalar) { bounds.emplace_back(var, scalar, type); }

    inline void add_disjunction(const DisjunctionInVeritas& disjunction) { disjunctions.push_back(disjunction); }

    inline void add_equation(const DisjunctionInVeritas&& disjunction) { disjunctions.push_back(std::move(disjunction)); }

    [[nodiscard]] inline bool empty() const { return equations.empty() && bounds.empty(); }

    veritas::AddTree add_operator_applicability(ActionOpID_type action_op, veritas::AddTree policy) const;
    veritas::Tree get_bound_tightenings_tree(std::list<VERITAS_IN_PLAJA::Tightening> tightenings, VeritasVarIndex_type indicator_var) const;

};

#endif //PLAJA_CONSTRAINTS_IN_VERITAS_H
