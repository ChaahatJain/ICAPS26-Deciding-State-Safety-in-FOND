//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SPLIT_ON_LOCS_H
#define PLAJA_SPLIT_ON_LOCS_H

#include <list>
#include <memory>
#include "../ast_element_visitor_const.h"
#include "../ast_element_visitor.h"

/** Utility class to check whether expression is states values.*/
class CheckForStatesValues final: private AstElementVisitorConst {

private:

    int rlt; // 2 -> StatesValues, 1 -> StateValues, 0 -> none of both.

    void visit(const StateValuesExpression* exp) override;
    void visit(const StatesValuesExpression* exp) override;

    CheckForStatesValues();
public:
    ~CheckForStatesValues() override;
    DELETE_CONSTRUCTOR(CheckForStatesValues)

    [[nodiscard]] static bool is_state_values(const Expression& expr);
    [[nodiscard]] static bool is_states_values(const Expression& expr);
    [[nodiscard]] static bool is_state_or_states_values(const Expression& expr);

};

/**********************************************************************************************************************/

class ExtractOnLocs final: private AstElementVisitor {

private:
    std::unique_ptr<StateConditionExpression> stateCondition;

    void visit(StateConditionExpression* exp) override;

    ExtractOnLocs();
public:
    ~ExtractOnLocs() override;
    DELETE_CONSTRUCTOR(ExtractOnLocs)

    /** Extract location constraints in StateConditionExpression-splits and keep variable constraints in split list. */
    [[nodiscard]] static std::unique_ptr<StateConditionExpression> extract(std::list<std::unique_ptr<Expression>>& conjuncts);

};

/**********************************************************************************************************************/

class SplitOnLocs final/*: private AstVisitor*/ {

    SplitOnLocs();
public:
    ~SplitOnLocs() /*override*/;
    DELETE_CONSTRUCTOR(SplitOnLocs)

    /** Split top-conjunction into StateConditionExpression -- locations constraints and state variable constraints separated. */
    [[nodiscard]] static std::unique_ptr<StateConditionExpression> split(std::unique_ptr<Expression>&& expr);

};

#endif //PLAJA_SPLIT_ON_LOCS_H
