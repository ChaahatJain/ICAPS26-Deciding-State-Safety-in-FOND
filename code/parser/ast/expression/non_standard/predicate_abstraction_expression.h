//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PREDICATE_ABSTRACTION_EXPRESSION_H
#define PLAJA_PREDICATE_ABSTRACTION_EXPRESSION_H

#include "../expression.h"
#include "../forward_expression.h"

class PAExpression: public PropertyExpression {

private:
    std::unique_ptr<Expression> start; // Complemented by initial state of model; StateConditionExpression or StatesValuesExpression.
    std::unique_ptr<Expression> reach; // Representation of states for which to check reachability, e.g., unsafety condition.
    std::unique_ptr<ObjectiveExpression> objective; // Representation of secondary "reach", e.g., the learning objective for policy learning.
    std::unique_ptr<PredicatesExpression> predicates;
    std::string nnFile; // May be nnet file or jani-2-nnet file.

public:
    explicit PAExpression();
    ~PAExpression() override;
    DELETE_CONSTRUCTOR(PAExpression)

    /* Static. */

    [[nodiscard]] static const std::string& get_op_string();

    /* Setter. */

    inline void set_start(std::unique_ptr<Expression>&& start_r) { start = std::move(start_r); }

    inline void set_reach(std::unique_ptr<Expression>&& reach_r) { reach = std::move(reach_r); }

    void set_objective(std::unique_ptr<ObjectiveExpression>&& objective);

    void set_goal_objective(std::unique_ptr<Expression>&& goal_objective);

    void set_predicates(std::unique_ptr<PredicatesExpression>&& predicates);

    inline void set_nnFile(std::string nn_file) { nnFile = std::move(nn_file); }

    /* Getter. */

    [[nodiscard]] inline const Expression* get_start() const { return start.get(); }

    [[nodiscard]] inline Expression* get_start() { return start.get(); }

    [[nodiscard]] inline const Expression* get_reach() const { return reach.get(); }

    [[nodiscard]] inline Expression* get_reach() { return reach.get(); }

    [[nodiscard]] inline const ObjectiveExpression* get_objective() const { return objective.get(); }

    [[nodiscard]] inline ObjectiveExpression* get_objective() { return objective.get(); }

    [[nodiscard]] const Expression* get_objective_goal() const;

    [[nodiscard]] inline const PredicatesExpression* get_predicates() const { return predicates.get(); }

    [[nodiscard]] inline PredicatesExpression* get_predicates() { return predicates.get(); }

    [[nodiscard]] inline const std::string& get_nnFile() const { return nnFile; }

    /* Override. */

    void accept(AstVisitor* ast_visitor) override;

    void accept(AstVisitorConst* ast_visitor) const override;

    [[nodiscard]] std::unique_ptr<PropertyExpression> deepCopy_PropExp() const override;

    /**/

    [[nodiscard]] std::unique_ptr<PAExpression> deep_copy() const;

    [[nodiscard]] bool nn_file_exists() const;

};

#endif //PLAJA_PREDICATE_ABSTRACTION_EXPRESSION_H
