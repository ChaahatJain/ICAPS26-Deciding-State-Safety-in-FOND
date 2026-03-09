//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_RESTRICTIONS_CHECKER_H
#define PLAJA_RESTRICTIONS_CHECKER_H


#include "ast_visitor_const.h"

// forward declaration:
class AstElement;

/***
 * Restrictions checker over the AST.
 * This class checks the restrictions of the current implementation on a syntactical level.
 * That is, it restricts to a subset of the currently implemented AST fragment, which is itself a JANI subset (w.r.t. JANI extensions), that is naturally enforced at parse time.
 * This restrictions checker shall be maintained as a *standard* which language fragments the implementation supports to any extend *beyond* the AST structure (i.a., state space structure & successor generation, engines).
 * The implementation of routines subsequently run (i.p. SemanticsChecker) can but do not have to restrict to the restrictions enforced by this class (i.e., the may implement a fragment potentially allowed in the future).
 *
 * The restrictions checks are enforced on a parsed model not semantically checked. Hence some semantic restrictions are enforced subsequently (SemanticsChecker).
 * Moreover, some syntactical restrictions may *redundantly* be enforced at parse time already to avoid implementing finally unused parsing routines.
 *
 */
class RestrictionsChecker: public AstVisitorConst {

private:
    bool hasFloatingVar; // so far, we only support a restricted ordering when using floating vars (first discrete, then floating, only global)

    RestrictionsChecker();
public:
    ~RestrictionsChecker() override;
    DELETE_CONSTRUCTOR(RestrictionsChecker)

    /**
     * @param ast_element
     * @param catch_exception, whether not-supported-exceptions shall be caught.
     * @return true if no restrictions are violated, false otherwise.
     */
    static bool check_restrictions(const AstElement* ast_element, bool catch_exception = true);

    void visit(const Automaton* automaton) override;
    void visit(const CompositionElement* sys_el) override;
    void visit(const ConstantDeclaration* const_decl) override;
    void visit(const Edge* edge) override;
    void visit(const Location* loc) override;
    // void visit(const Metadata* metadata) override; // not supported by parser, but as only metadata no need to explicitly mark
    void visit(const Model* model) override;
    void visit(const Property* property) override;
    void visit(const PropertyInterval* property_interval) override;
    void visit(const RewardBound* reward_bound) override;
    void visit(const RewardInstant* reward_instant) override;
    void visit(const VariableDeclaration* var_decl) override;
    // non-standard:
    void visit(const FreeVariableDeclaration* var_decl) override;
    void visit(const RewardAccumulation* reward_acc) override;

    // expressions:
    void visit(const ArrayConstructorExpression* exp) override;
    void visit(const ConstantExpression* exp) override;
    void visit(const DerivativeExpression* exp) override;
    void visit(const DistributionSamplingExpression* exp) override;
    void visit(const ExpectationExpression* exp) override;
    void visit(const FreeVariableExpression* exp) override;
    void visit(const PathExpression* exp) override;
    void visit(const QfiedExpression* exp) override;

    void visit(const BoundedType* type) override;
};


#endif //PLAJA_RESTRICTIONS_CHECKER_H
