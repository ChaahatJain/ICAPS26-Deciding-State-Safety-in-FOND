//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_CONSTRAINTS_IN_MARABOU_H
#define PLAJA_CONSTRAINTS_IN_MARABOU_H

#include <memory>
#include <unordered_map>
#include "../../include/marabou_include/disjunction_constraint_base.h"
#include "../../include/marabou_include/equation.h"
#include "../../parser/using_parser.h"
#include "../../assertions.h"
#include "../smt/base/smt_constraint.h"
#include "forward_smt_nn.h"
#include "using_marabou.h"

/**
 * Word of caution: "Move" functions do invalidate the object.
 * They aim to move as much data of the called object to the created object to avoid copying.
 */

class MarabouConstraint: public PLAJA::SmtConstraint {
protected:
    MARABOU_IN_PLAJA::Context* context;
    explicit MarabouConstraint(MARABOU_IN_PLAJA::Context& c);

public:
    virtual ~MarabouConstraint() = 0;
    DEFAULT_CONSTRUCTOR_ONLY(MarabouConstraint)
    DELETE_ASSIGNMENT(MarabouConstraint)

    [[nodiscard]] inline MARABOU_IN_PLAJA::Context& _context() const { return *context; }

    [[nodiscard]] virtual bool is_trivially_true() const;
    [[nodiscard]] virtual bool is_trivially_false() const;
    /* virtual void set_trivially_true();
    virtual void set_trivially_false(); */

    virtual void add_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) const = 0;
    virtual void move_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) = 0;
    virtual void add_to_conjunction(ConjunctionInMarabou& conjunction) const = 0;
    virtual void move_to_conjunction(ConjunctionInMarabou& conjunction) = 0;
    virtual void add_to_disjunction(DisjunctionInMarabou& disjunction) const = 0;
    virtual void move_to_disjunction(DisjunctionInMarabou& disjunction) = 0;

    [[nodiscard]] virtual std::unique_ptr<MarabouConstraint> copy() const = 0;
    [[nodiscard]] virtual std::unique_ptr<MarabouConstraint> compute_negation() const = 0;
    [[nodiscard]] virtual std::unique_ptr<MarabouConstraint> move_to_negation() = 0;
    virtual void substitute(const std::unordered_map<MarabouVarIndex_type, MarabouVarIndex_type>& mapping) = 0;
    [[nodiscard]] std::unique_ptr<MarabouConstraint> to_substitute(const std::unordered_map<MarabouVarIndex_type, MarabouVarIndex_type>& mapping) const;

    /**
     * Optimize constraint for query.
     * @return may return nullptr if entailed.
     */
    [[nodiscard]] virtual std::unique_ptr<MarabouConstraint> optimize_for_query(const MARABOU_IN_PLAJA::QueryConstructable& query, DisjunctionConstraintBase::EntailmentValue& entailment_value) const = 0;

    /**
     * Optimize constraint for query.
     * @param optimized_constraint may contain fresh optimized constraint (usually for Disjunctions, original constraint is then invalid.
     */
    [[nodiscard]] virtual DisjunctionConstraintBase::EntailmentValue optimize_for_query(const MARABOU_IN_PLAJA::QueryConstructable& query, std::unique_ptr<MarabouConstraint>& optimized_constraint) = 0;

    [[nodiscard]] virtual bool is_bound() const;
    [[nodiscard]] virtual bool is_linear() const;

    virtual void dump() const = 0;

    /* Generic SMT interface: */
    void add_to_solver(PLAJA::SmtSolver& solver) const override;
    void add_negation_to_solver(PLAJA::SmtSolver& solver) const override;

};

// Marabou constraints for a single predicate
class PredicateConstraint final {
    friend class ToMarabouBoundPredicateTest; //
    friend class ToMarabouConjunctionPredicateTest; //
    friend class ToMarabouEquationPredicateTest;

private:
    std::unique_ptr<MarabouConstraint> constraint;
    std::unique_ptr<MarabouConstraint> negation;
public:
    explicit PredicateConstraint(std::unique_ptr<MarabouConstraint>&& constraint_);
    ~PredicateConstraint();
    DELETE_CONSTRUCTOR(PredicateConstraint)

    inline void add_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) const { constraint->add_to_query(query); }

    inline void move_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) { constraint->move_to_query(query); }

    inline void add_negation_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) const { negation->add_to_query(query); }

    [[nodiscard]] inline bool is_bound() const { return constraint->is_bound(); }

    [[nodiscard]] inline bool is_linear() const { return constraint->is_linear(); }

    void dump() const;
};

/**********************************************************************************************************************/

class EquationConstraint final: public MarabouConstraint {
private:
    Equation equation;

    /* auxiliaries */

    [[nodiscard]] static MarabouFloating_type compute_negation_offset(const MARABOU_IN_PLAJA::Context& c, const Equation& eq);
public:
    static void negate_to_ge(Equation& eq, MarabouFloating_type scalar_offset);
    static void negate_to_le(Equation& eq, MarabouFloating_type scalar_offset);
    static void negate_non_eq(const MARABOU_IN_PLAJA::Context& c, Equation& eq);
    [[nodiscard]] static std::unique_ptr<DisjunctionInMarabou> negate_eq(MARABOU_IN_PLAJA::Context& c, Equation eq);

    static void substitute(Equation& eq, const std::unordered_map<MarabouVarIndex_type, MarabouVarIndex_type>& mapping);

    /* */

    explicit EquationConstraint(MARABOU_IN_PLAJA::Context& c, Equation eq);
    ~EquationConstraint() final;
    DEFAULT_CONSTRUCTOR_ONLY(EquationConstraint)
    DELETE_ASSIGNMENT(EquationConstraint)

    /* getter */
    [[nodiscard]] inline const Equation& get_equation() const { return equation; }

    [[nodiscard]] bool operator==(const EquationConstraint& other) const;

    // override:
    void add_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) const override;
    void move_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) override;
    void add_to_conjunction(ConjunctionInMarabou& conjunction) const override;
    void move_to_conjunction(ConjunctionInMarabou& conjunction) override;
    void add_to_disjunction(DisjunctionInMarabou& disjunction) const override;
    void move_to_disjunction(DisjunctionInMarabou& disjunction) override;

    [[nodiscard]] std::unique_ptr<MarabouConstraint> copy() const override;
    void substitute(const std::unordered_map<MarabouVarIndex_type, MarabouVarIndex_type>& mapping) override;
    [[nodiscard]] std::unique_ptr<MarabouConstraint> compute_negation() const override;
    [[nodiscard]] std::unique_ptr<MarabouConstraint> move_to_negation() override;
    [[nodiscard]] std::unique_ptr<MarabouConstraint> optimize_for_query(const MARABOU_IN_PLAJA::QueryConstructable& query, DisjunctionConstraintBase::EntailmentValue& entailment_value) const override;
    [[nodiscard]] DisjunctionConstraintBase::EntailmentValue optimize_for_query(const MARABOU_IN_PLAJA::QueryConstructable& query, std::unique_ptr<MarabouConstraint>& optimized_constraint) override;

    void dump() const override;

    /* */

    inline void add_to_case_split(PiecewiseLinearCaseSplit& case_split) const { case_split.addEquation(equation); }

    inline void move_to_case_split(PiecewiseLinearCaseSplit& case_split) { case_split.addEquation(std::move(equation)); }

};

/**********************************************************************************************************************/

class BoundConstraint final: public MarabouConstraint {

private:
    Tightening tightening;

public:
    BoundConstraint(MARABOU_IN_PLAJA::Context& c, MarabouVarIndex_type var, Tightening::BoundType type, MarabouFloating_type scalar);
    BoundConstraint(MARABOU_IN_PLAJA::Context& c, const Tightening& tightening);
    ~BoundConstraint() final;
    DEFAULT_CONSTRUCTOR_ONLY(BoundConstraint)
    DELETE_ASSIGNMENT(BoundConstraint)

    /* auxiliary */

    [[nodiscard]] static std::unique_ptr<MarabouConstraint> construct_equality_constraint(MARABOU_IN_PLAJA::Context& c, MarabouVarIndex_type var, MarabouFloating_type scalar);
    [[nodiscard]] static std::unique_ptr<MarabouConstraint> construct(MARABOU_IN_PLAJA::Context& c, MarabouVarIndex_type var, Equation::EquationType op, MarabouFloating_type scalar);

    static void substitute(Tightening& tightening, const std::unordered_map<MarabouVarIndex_type, MarabouVarIndex_type>& mapping);

    /* getter */

    [[nodiscard]] inline MarabouVarIndex_type get_var() const { return tightening._variable; }

    [[nodiscard]] inline Tightening::BoundType get_type() const { return tightening._type; }

    [[nodiscard]] inline MarabouFloating_type get_value() const { return tightening._value; }

    /* */

    [[nodiscard]] bool operator==(const BoundConstraint& other) const;

    void add_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) const override;
    void move_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) override;
    void add_to_conjunction(ConjunctionInMarabou& conjunction) const override;
    void move_to_conjunction(ConjunctionInMarabou& conjunction) override;
    void add_to_disjunction(DisjunctionInMarabou& disjunction) const override;
    void move_to_disjunction(DisjunctionInMarabou& disjunction) override;

    [[nodiscard]] std::unique_ptr<MarabouConstraint> copy() const override;
    void substitute(const std::unordered_map<MarabouVarIndex_type, MarabouVarIndex_type>& mapping) override;
    [[nodiscard]] std::unique_ptr<MarabouConstraint> compute_negation() const override;
    [[nodiscard]] std::unique_ptr<MarabouConstraint> move_to_negation() override;
    [[nodiscard]] std::unique_ptr<MarabouConstraint> optimize_for_query(const MARABOU_IN_PLAJA::QueryConstructable& query, DisjunctionConstraintBase::EntailmentValue& entailment_value) const override;
    [[nodiscard]] DisjunctionConstraintBase::EntailmentValue optimize_for_query(const MARABOU_IN_PLAJA::QueryConstructable& query, std::unique_ptr<MarabouConstraint>& optimized_constraint) override;

    [[nodiscard]] bool is_bound() const override;

    void dump() const override;

};

/**********************************************************************************************************************/

class DisjunctionInMarabou final: public MarabouConstraint, public DisjunctionConstraintBase {
    friend class ToMarabouConjunctionPredicateTest; //
    friend EquationConstraint; // Faster construction ...
    friend BoundConstraint; //

private:

    inline const Vector<PiecewiseLinearCaseSplit>& disjuncts() const { return _disjuncts; }

    inline Vector<PiecewiseLinearCaseSplit>& disjuncts() { return _disjuncts; }

    inline std::vector<PiecewiseLinearCaseSplit>& disjuncts_std_view() { return _disjuncts.view_std(); }

    /** Compute bounds that hold globally over all disjuncts. */
    void compute_global_bounds(std::unordered_map<MarabouVarIndex_type, MarabouFloating_type>& lower_bounds, std::unordered_map<MarabouVarIndex_type, MarabouFloating_type>& upper_bounds) const;

public:
    explicit DisjunctionInMarabou(MARABOU_IN_PLAJA::Context& c);
    ~DisjunctionInMarabou() final;
    DEFAULT_CONSTRUCTOR_ONLY(DisjunctionInMarabou)
    DELETE_ASSIGNMENT(DisjunctionInMarabou)

    [[nodiscard]] bool operator==(const DisjunctionInMarabou& other) const;

    [[nodiscard]] bool is_trivially_true() const override;
    [[nodiscard]] bool is_trivially_false() const override;
    /* void set_trivially_true() override;
    void set_trivially_false() override; */

    void add_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) const override;
    void move_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) override;
    void add_to_conjunction(ConjunctionInMarabou& conjunction) const override;
    void move_to_conjunction(ConjunctionInMarabou& conjunction) override;
    void add_to_disjunction(DisjunctionInMarabou& disjunction) const override;
    void move_to_disjunction(DisjunctionInMarabou& disjunction) override;

    [[nodiscard]] std::unique_ptr<MarabouConstraint> copy() const override;
    void substitute(const std::unordered_map<MarabouVarIndex_type, MarabouVarIndex_type>& mapping) override;
    [[nodiscard]] std::unique_ptr<MarabouConstraint> compute_negation() const override;
    [[nodiscard]] std::unique_ptr<MarabouConstraint> move_to_negation() override;
    [[nodiscard]] std::unique_ptr<MarabouConstraint> optimize_for_query(const MARABOU_IN_PLAJA::QueryConstructable& query, DisjunctionConstraintBase::EntailmentValue& entailment_value) const override;
    [[nodiscard]] DisjunctionConstraintBase::EntailmentValue optimize_for_query(const MARABOU_IN_PLAJA::QueryConstructable& query, std::unique_ptr<MarabouConstraint>& optimized_constraint) override;

    [[nodiscard]] bool is_linear() const override;

    void dump() const override;

    /* */

    inline void reserve_for_additional(std::size_t additional_disjuncts) { disjuncts().view_std().reserve(disjuncts().size() + additional_disjuncts); }

    inline void shrink_to_fit() { disjuncts_std_view().shrink_to_fit(); }

    inline void add_case_split(const PiecewiseLinearCaseSplit& case_split) { disjuncts().append(case_split); }

    inline void add_case_split(PiecewiseLinearCaseSplit&& case_split) { disjuncts().append(std::move(case_split)); }

    void add_case_split(const Equation& equation);

    void add_case_split(Equation&& equation);

    void add_case_split(const Tightening& tightening);

    [[nodiscard]] inline const Vector<PiecewiseLinearCaseSplit>& get_disjuncts() const { return disjuncts(); }

    [[nodiscard]] inline bool empty() const { return get_disjuncts().empty(); }

    /**
     * Factor out min-max bounds.
     * Move structures as far as possible.
     */
    [[nodiscard]] std::unique_ptr<MarabouConstraint> optimize();

};

class ConjunctionInMarabou final: public MarabouConstraint {
    friend class ToMarabouConstraintTest; //
    friend class ToMarabouBoundPredicateTest; //
    friend class ToMarabouConjunctionPredicateTest; //
    friend BoundConstraint; // Faster construction ...

private:
    std::list<Equation> equations;
    std::list<Tightening> bounds;
    std::list<DisjunctionInMarabou> disjunctions;

public:
    explicit ConjunctionInMarabou(MARABOU_IN_PLAJA::Context& c);
    ~ConjunctionInMarabou() final;
    DEFAULT_CONSTRUCTOR_ONLY(ConjunctionInMarabou)
    DELETE_ASSIGNMENT(ConjunctionInMarabou)

    [[nodiscard]] bool operator==(const ConjunctionInMarabou& other) const;

    [[nodiscard]] bool is_trivially_true() const override;
    [[nodiscard]] bool is_trivially_false() const override;
    /* void set_trivially_true() override;
    void set_trivially_false() override; */

    void add_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) const override;
    void move_to_query(MARABOU_IN_PLAJA::QueryConstructable& query) override;
    void add_to_conjunction(ConjunctionInMarabou& conjunction) const override;
    void move_to_conjunction(ConjunctionInMarabou& conjunction) override;
    void add_to_disjunction(DisjunctionInMarabou& disjunction) const override;
    void move_to_disjunction(DisjunctionInMarabou& disjunction) override;
    [[nodiscard]] bool is_linear() const override;

    [[nodiscard]] std::unique_ptr<MarabouConstraint> copy() const override;
    void substitute(const std::unordered_map<MarabouVarIndex_type, MarabouVarIndex_type>& mapping) override;
    [[nodiscard]] std::unique_ptr<MarabouConstraint> compute_negation() const override;
    [[nodiscard]] std::unique_ptr<MarabouConstraint> move_to_negation() override;
    [[nodiscard]] std::unique_ptr<MarabouConstraint> optimize_for_query(const MARABOU_IN_PLAJA::QueryConstructable& query, DisjunctionConstraintBase::EntailmentValue& entailment_value) const override;
    [[nodiscard]] DisjunctionConstraintBase::EntailmentValue optimize_for_query(const MARABOU_IN_PLAJA::QueryConstructable& query, std::unique_ptr<MarabouConstraint>& optimized_constraint) override;

    void dump() const override;

    /* */

    void add_case_split(const PiecewiseLinearCaseSplit& case_split);

    void add_case_split(PiecewiseLinearCaseSplit&& case_split);

    void add_equality_constraint(MarabouVarIndex_type var, MarabouFloating_type scalar);

    inline void add_equation(const Equation& equation) { equations.push_back(equation); }

    inline void add_equation(Equation&& equation) { equations.push_back(std::move(equation)); }

    inline void add_bound(const Tightening& bound) { bounds.push_back(bound); }

    inline void add_bound(MarabouVarIndex_type var, Tightening::BoundType type, MarabouFloating_type scalar) { bounds.emplace_back(var, scalar, type); }

    inline void add_disjunction(const DisjunctionInMarabou& disjunction) { disjunctions.push_back(disjunction); }

    inline void add_disjunction(DisjunctionInMarabou&& disjunction) { disjunctions.push_back(std::move(disjunction)); }

    [[nodiscard]] inline bool empty() const { return equations.empty() && bounds.empty() && disjunctions.empty(); }

    [[nodiscard]] inline std::size_t size() const { return equations.size() + bounds.size() + disjunctions.size(); }

};

#endif //PLAJA_CONSTRAINTS_IN_MARABOU_H
