//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TO_JSON_VISITOR_H
#define PLAJA_TO_JSON_VISITOR_H

#include <json_fwd.hpp>
#include "../ast_visitor_const.h"

class AstElement;

class Commentable;
namespace TO_JSON_VISITOR { struct ToJsonIterator; }

class ToJSON_Visitor: public AstVisitorConst {
private:
    const std::string* targetDirectory; // to generate relative paths
    bool useInstances;
    std::unique_ptr<nlohmann::json> j_rlt; // return value for the subroutine
    // model data // deprecated
    // const Model* model_ref; // currently considered model
    // const Automaton* automaton_ref; // currently considered automaton
    // std::vector<std::string> freeVars;

    // auxiliaries:
    void accept(const AstElement* ast_element, const std::string& key, nlohmann::json& j);
    void accept_if(const AstElement* ast_element, const std::string& key, nlohmann::json& j);
    void accept_exp_comment_if(const AstElement* exp, const std::string& comment, const std::string& key, nlohmann::json& j);
    static void accept_comment(const Commentable* element, nlohmann::json& j);
    friend TO_JSON_VISITOR::ToJsonIterator;

    explicit ToJSON_Visitor(bool use_instances);
public:
    DELETE_CONSTRUCTOR(ToJSON_Visitor)
    ~ToJSON_Visitor() override;
    /**
     *
     * @param astElement
     * @param use_instances. If the AST element is a model, use all instances rather than the automata description.
     * Note that, due to duplicate names, the model may be not valid.
     * @return
     */
    static std::unique_ptr<nlohmann::json> to_json(const AstElement& ast_element, bool use_instances = false);
    static std::string to_string(const AstElement& ast_element, bool use_instances = false, bool intend = true);
    static void dump(const AstElement& ast_element, bool use_instances = false, bool intend = true);
    static void write_to_file(const std::string& filename, const AstElement& ast_element, bool use_instances = false, bool intend = true, bool trunc = true);

    void visit(const Action* action) override;
    void visit(const Assignment* assignment) override;
    void visit(const Automaton* automaton) override;
    void visit(const Composition* sys) override;
    void visit(const CompositionElement* sysEl) override;
    void visit(const ConstantDeclaration* constDecl) override;
    void visit(const Destination* destination) override;
    void visit(const Edge* edge) override;
    void visit(const Location* loc) override;
    void visit(const Metadata* metadata) override;
    void visit(const Model* model) override;
    void visit(const Property* prop) override;
    void visit(const PropertyInterval* propertyInterval) override;
    void visit(const RewardBound* rewardBound) override;
    void visit(const RewardInstant* rewardInstant) override;
    void visit(const Synchronisation* sync) override;
    void visit(const TransientValue* transVal) override;
    void visit(const VariableDeclaration* varDecl) override;
    // non-standard:
    void visit(const FreeVariableDeclaration* var_decl) override;
    void visit(const Properties* properties) override;
    void visit(const RewardAccumulation* reward_accumulation) override;

    // expressions:
    void visit(const ArrayAccessExpression* exp) override;
    void visit(const ArrayConstructorExpression* exp) override;
    void visit(const ArrayValueExpression* exp) override;
    void visit(const BinaryOpExpression* exp) override;
    void visit(const BoolValueExpression* exp) override;
    void visit(const ConstantExpression* exp) override;
    void visit(const DerivativeExpression* exp) override;
    void visit(const DistributionSamplingExpression* exp) override;
    void visit(const ExpectationExpression* exp) override;
    void visit(const FreeVariableExpression* exp) override;
    void visit(const FilterExpression* exp) override;
    void visit(const IntegerValueExpression* exp) override;
    void visit(const ITE_Expression* exp) override;
    void visit(const PathExpression* exp) override;
    void visit(const QfiedExpression* exp) override;
    void visit(const RealValueExpression* exp) override;
    void visit(const StatePredicateExpression* exp) override;
    void visit(const UnaryOpExpression* exp) override;
    void visit(const VariableExpression* exp) override;
    // non-standard:
    void visit(const ConstantArrayAccessExpression* op) override;
    void visit(const ActionOpTuple* op) override;
    void visit(const LetExpression* exp) override;
    void visit(const LocationValueExpression* exp) override;
    void visit(const ObjectiveExpression* exp) override;
    void visit(const PAExpression* exp) override;
    void visit(const PredicatesExpression* exp) override;
    void visit(const ProblemInstanceExpression* exp) override;
    void visit(const StateConditionExpression* exp) override;
    void visit(const StateValuesExpression* exp) override;
    void visit(const StatesValuesExpression* exp) override;
    void visit(const VariableValueExpression* exp) override;
    // special cases:
    void visit(const LinearExpression* exp) override;
    void visit(const NaryExpression* exp) override;

    // types:
    void visit(const ArrayType* type) override;
    void visit(const BoolType* type) override;
    void visit(const BoundedType* type) override;
    void visit(const ClockType* type) override;
    void visit(const ContinuousType* type) override;
    void visit(const IntType* type) override;
    void visit(const RealType* type) override;
    // non-standard:
    void visit(const LocationType* type) override;

};

#endif //PLAJA_TO_JSON_VISITOR_H
