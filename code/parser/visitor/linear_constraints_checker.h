//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_LINEAR_CONSTRAINTS_CHECKER_H
#define PLAJA_LINEAR_CONSTRAINTS_CHECKER_H

#include <unordered_set>
#include "../using_parser.h"
#include "ast_visitor_const.h"

namespace LINEAR_CONSTRAINTS_CHECKER {

    enum class NFRequest { NONE, TRUE, FALSE }; // normal form?
    enum class SpecialAstRequest { NONE, TRUE, FALSE };

    struct Specification {

        bool allowEquality;
        bool allowStrictOp;
        bool allowArrayAccess;
        bool allowBools;
        bool needsAddends; // sum needs addends?
        bool asPredicate;
        NFRequest nfRequest;
        SpecialAstRequest specialRequest;

        Specification():
            allowEquality(true)
            , allowStrictOp(true)
            , allowArrayAccess(true)
            , allowBools(true)
            , needsAddends(false)
            , asPredicate(false)
            , nfRequest(NFRequest::NONE)
            , specialRequest(SpecialAstRequest::FALSE) {}

        // short-cuts
        inline static Specification set_nf(NFRequest request) {
            Specification specification;
            specification.nfRequest = request;
            return specification;
        }

        inline static Specification set_special(SpecialAstRequest request) {
            Specification specification;
            specification.specialRequest = request;
            return specification;
        }

        inline static Specification set_strict_op(bool strict_op) {
            Specification specification;
            specification.allowStrictOp = strict_op;
            return specification;
        }

        inline static Specification set_bools(bool allow_bools) {
            Specification specification;
            specification.allowBools = allow_bools;
            return specification;
        }

        inline static Specification as_predicate(bool as_predicate) {
            Specification specification;
            specification.asPredicate = as_predicate;
            return specification;
        }

        //
        inline static Specification set_special_and_predicate(SpecialAstRequest request, bool as_predicate) {
            Specification specification;
            specification.specialRequest = request;
            specification.asPredicate = as_predicate;
            return specification;
        }

        inline static Specification set_nf_and_predicate(NFRequest request, bool as_predicate) {
            Specification specification;
            specification.nfRequest = request;
            specification.asPredicate = as_predicate;
            return specification;
        }

    };
}

/**
 * Visitor-like class to check if expression is linear.
 * Assumes static semantics are valid.
 */
class LinearConstraintsChecker final: public AstVisitorConst {

public:
    enum LinearType { NONE, VAR, SCALAR, ADDEND, TRUTH_VALUE, LITERAL, CON_BOOL, DIS_BOOL, CONSTRAINT, CON, DIS, SUM, ASSIGNMENT, CONDITION, };

private:
    LinearType type;
    LinearType requestedType;
    const LINEAR_CONSTRAINTS_CHECKER::Specification* specification;
    bool isNf;
    unsigned int seenConsts;
    std::unordered_set<VariableIndex_type> seenAddends; // to check for duplicate variables; to infer "isNf"

    [[nodiscard]] inline bool has_addend() const { return not seenAddends.empty(); }

    [[nodiscard]] inline bool is_nf() const { return isNf; }

    inline void inc_seen_constants() { if (++seenConsts > 1) { isNf = false; } }

    inline void add_to_seen_vars(VariableIndex_type var) { if (not seenAddends.insert(var).second) { isNf = false; } }

    [[nodiscard]] bool is(LinearType expr_type, const Expression& expr, LinearType super_type) const;
    [[nodiscard]] static bool is_internally_possible(LinearType expr_type, LinearType requested_type);

    // bool is_state_variable_index_internal(const Expression& expr);
    // bool is_addend_internal(const Expression& expr);
    // bool is_linear_sum_internal(const Expression& expr);

    void visit(const ArrayAccessExpression* exp) override;
    void visit(const ArrayConstructorExpression* exp) override; // not supported expressions must explicitly set to NONE
    void visit(const ArrayValueExpression* exp) override;
    void visit(const BinaryOpExpression* exp) override;
    void visit(const BoolValueExpression* exp) override;
    void visit(const DistributionSamplingExpression* exp) override;
    void visit(const IntegerValueExpression* exp) override;
    void visit(const ITE_Expression* exp) override;
    void visit(const RealValueExpression* exp) override;
    void visit(const UnaryOpExpression* exp) override;
    void visit(const VariableExpression* exp) override;
    // non-standard:
    void visit(const ConstantArrayAccessExpression* exp) override;
    void visit(const LetExpression* exp) override;
    void visit(const LocationValueExpression* exp) override;
    void visit(const PredicatesExpression* exp) override;
    void visit(const StateConditionExpression* exp) override;
    void visit(const StateValuesExpression* exp) override;
    void visit(const StatesValuesExpression* exp) override;
    void visit(const VariableValueExpression* exp) override;
    // special cases:
    void visit(const LinearExpression* exp) override;
    void visit(const NaryExpression* exp) override;

    explicit LinearConstraintsChecker(LinearType requested_type, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification);
public:
    ~LinearConstraintsChecker() override;
    DELETE_CONSTRUCTOR(LinearConstraintsChecker)

    /**
    * Is variable x or A[c] where A is an array and c evaluates to a constant index?
    */
    inline static bool is_state_variable_index(const Expression& expr, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification = {}) { return is_linear(expr, specification, VAR); }

    /**
     * Is of form x or !x , where x is a (boolean) state variable index?
     */
    inline static bool is_literal(const Expression& expr, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification = {}) { return is_linear(expr, specification, LITERAL); }

    /**
       * Is of form [literal_1] && ... && [literal_1], for n >= 1?
    */
    inline static bool is_conjunction_bool(const Expression& expr, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification = {}) { return is_linear(expr, specification, CON_BOOL); }

    /**
       * Is of form [literal_1] || ... || [literal_1], for n >= 1?
    */
    inline static bool is_disjunction_bool(const Expression& expr, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification = {}) { return is_linear(expr, specification, DIS_BOOL); }

    /**
     * Is exp a scalar c, i.e., does evaluate to a numeric constant?
     */
    inline static bool is_scalar(const Expression& expr) { return is_linear(expr, {}, SCALAR); }

    /**
     * Is exp a factor c (to an addend), i.e., does evaluate to an *non-zero* numeric constant?
     */
    static bool is_factor(const Expression& expr);

    /**
     * Is c*x or x, where x is a state variable index and c a scalar?
     */
    inline static bool is_addend(const Expression& expr, LINEAR_CONSTRAINTS_CHECKER::Specification specification = {}) { return is_linear(expr, specification, ADDEND); }

    /**
     * Is of form (factor * ([addend_11|scalar_11] + ... + [addend_1n|scalar_1n])) + ... + (factor * ([addend_m1|scalar_m1] + ... + [addend_mn|scalar_mn])),
        * for m,n >= 0?
     */
    inline static bool is_linear_sum(const Expression& expr, LINEAR_CONSTRAINTS_CHECKER::Specification specification = {}) { return is_linear(expr, specification, SUM); }

    /**
    * Is linear sum or literal?
    */
    inline static bool is_linear_assignment(const Expression& expr, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification = {}) { return is_linear(expr, specification, ASSIGNMENT); }

    /**
     * Is of form [linear_sum_1] {<=,<,==,>,>=} [linear_sum_2] ?
     */
    inline static bool is_linear_constraint(const Expression& expr, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification = {}) { return is_linear(expr, specification, CONSTRAINT); }

    /**
     * Is of form [linear_constraint_1|conjunction_bool_1|disjunction_bool_1] && ... && [linear_constraint_n|conjunction_bool_n|disjunction_bool_n] for n >= 1?
     */
    inline static bool is_linear_conjunction(const Expression& expr, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification = {}) { return is_linear(expr, specification, CON); }

    /**
     * Is of form [linear_constraint_1|conjunction_bool_1|disjunction_bool_1] || ... || [linear_constraint_n|conjunction_bool_n|disjunction_bool_n] for n >= 1?
     */
    inline static bool is_linear_disjunction(const Expression& expr, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification = {}) { return is_linear(expr, specification, DIS); }

    /**
     * Check whether a constraint is linear according to the requirements of the implementation,
     * (currently i.p. w.r.t. Marabou), i.e.,
     * a conjunction [linear_constraint_1|conjunction_bool_1] && ... && [linear_constraint_n|conjunction_bool_n]?
     */
    static bool is_linear(const Expression& expr, const LINEAR_CONSTRAINTS_CHECKER::Specification& specification = {}, LinearType type = CONDITION);

};

#endif //PLAJA_LINEAR_CONSTRAINTS_CHECKER_H
