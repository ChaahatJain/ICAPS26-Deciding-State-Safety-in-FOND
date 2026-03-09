//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SIMPLE_CHECKS_VISITOR_H
#define PLAJA_SIMPLE_CHECKS_VISITOR_H


#include "ast_visitor_const.h"

// forward-declaration:
class AstElement;

class SimpleChecksVisitor: public AstVisitorConst {
private:
    const Model* model;
    bool sampling_free;
    bool containsContinuous;
    bool containsClocks;
    bool foundTransients;
    explicit SimpleChecksVisitor(const Model* model_);
public:
    ~SimpleChecksVisitor() override;
    void visit(const ClockType* type_) override;
    void visit(const ContinuousType* type_) override;
    void visit(const VariableDeclaration* exp) override;
    void visit(const VariableExpression* exp) override;
    void visit(const DistributionSamplingExpression* exp) override;

    static bool contains_clocks(const AstElement* ast_el);
    static bool contains_continuous(const AstElement* ast_el);
    static bool contains_transients(const AstElement* ast_el, const Model& model);
    static bool is_sampling_free(const AstElement* ast_el);
};

#endif //PLAJA_SIMPLE_CHECKS_VISITOR_H
