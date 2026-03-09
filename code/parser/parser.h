//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PARSER_H
#define PLAJA_PARSER_H

#include <memory>
#include <unordered_map>
#include <json_fwd.hpp>
#include "ast_forward_declarations.h"
#include "using_parser.h"

namespace PLAJA { class OptionParser; }

/*
 * ISSUES
 * #1 Automata not used in the composition are not parsed and thus not checked, conversely syntactic and static semantic checks are applied twice to automata instance of the same automaton.
 * #2 Free variables in the declaration of a variable/constant can have the name of a subsequently declared variable or constant.
 */

/**
 * Model correctness is checked on a syntactical level.
 * This includes also checks reflecting whether a variable is local or global.
 * The order in which the substructures of the model are parsed is relevant.
 */
class Parser {
private:
    const PLAJA::OptionParser& optionParser;

    /* Just to reject model using substructures not allowed for the given model type technical: ModelType is enum currently defined in model.h. */
    const Model* model_ref;
    /* */
    std::unordered_map<std::string, const VariableDeclaration*> globalVars;
    std::unordered_map<std::string, const ConstantDeclaration*> constants;
    std::unordered_map<std::string, const Action*> actions;
    std::unordered_map<std::string, const Automaton*> automata;
    std::unordered_map<std::string, const Property*> properties;
    std::unordered_map<std::string, const VariableDeclaration*> localVars; // For *current* automaton.
    std::unordered_map<std::string, const Location*> locations; // For *current* automaton.
    std::unordered_map<std::string, const FreeVariableDeclaration*> freeVars; // For current scope, e.g., free variable of array. // TODO might wanna have an actual push-pop stack.
    /* Parsing utilities. */
    std::unordered_map<std::string, const nlohmann::json*> labels; // Model and property files may define a collection of labeled (to reduce duplication).
    std::unordered_map<std::string, std::unique_ptr<const nlohmann::json>> externalStructures; // Model and property files may reference json files defining single structures (to reduce duplication).

    /* Model data. */
    VariableID_type variable_counter; // For global variable ID.
    FreeVariableID_type free_variable_counter; // For free variable ID (maintained globally).
    AutomatonIndex_type automaton_counter; // For system's automaton index.
    const Automaton* currentAutomaton;
    std::string current_directory_path; // Some properties use relative paths to other files, hence we need to determine an absolute path or a relative path from the current execution directory.

    /* Auxiliaries for labels & externals. */
    void parse_labels(const nlohmann::json& input);
    void parse_external(const nlohmann::json& filename);
    const nlohmann::json& load_label(const nlohmann::json& input);
    const nlohmann::json& load_external(const nlohmann::json& input);
    const nlohmann::json& load_label_or_external_if(const nlohmann::json& input);

    bool doConstantsChecks;
    void check_constants();

    /**
     * @param allow_locals, flag parameter to indicate whether unambiguous local variables are allowed (violating JANI standard for additional properties).
     */
    void parse_additional_properties(Model* model, bool allow_locals);

public:
    explicit Parser(const PLAJA::OptionParser& option_parser);
    ~Parser();

    void set_constants_checks(bool value) { doConstantsChecks = value; }

    std::unique_ptr<Model> parse(const std::string* filename = nullptr); // Filename argument to allow for parsing of additional models.
    /* For testing. */
#ifndef NDEBUG
    [[maybe_unused]] AutomatonIndex_type set_automaton_index(AutomatonIndex_type automaton_index); // AUTOMATON::invalid to make parsed variable global, returns overridden value.
    std::unique_ptr<ConstantDeclaration> parse_ConstDeclStr(const std::string& const_decl_str);
    std::unique_ptr<VariableDeclaration> parse_VarDeclStr(const std::string& var_decl_str);
    std::unique_ptr<VariableDeclaration> parse_var_decl_str(const std::string& var_decl_str, VariableIndex_type index);
    std::unique_ptr<Expression> parse_ExpStr(const std::string& exp_str);
#endif
    void load_local_variables(); // To make local variables usable outside automaton.

    std::unique_ptr<Action> parse_Action(const nlohmann::json& input);
    std::unique_ptr<Assignment> parse_Assignment(const nlohmann::json& input);
    std::unique_ptr<Automaton> parse_Automaton(const nlohmann::json& input);
    std::unique_ptr<Composition> parse_Composition(const nlohmann::json& input);
    [[nodiscard]] std::unique_ptr<CompositionElement> parse_CompositionElement(const nlohmann::json& input) const;
    std::unique_ptr<ConstantDeclaration> parse_ConstantDeclaration(const nlohmann::json& input);
    std::unique_ptr<Destination> parse_Destination(const nlohmann::json& input);
    std::unique_ptr<Edge> parse_Edge(const nlohmann::json& input);
    std::unique_ptr<Location> parse_Location(const nlohmann::json& input);
    std::unique_ptr<Model> parse_Model(const nlohmann::json& input);
    std::unique_ptr<Property> parse_Property(const nlohmann::json& input);
    std::unique_ptr<PropertyInterval> parse_PropertyInterval(const nlohmann::json& input);
    std::unique_ptr<RewardInstant> parse_RewardInstant(const nlohmann::json& input);
    std::unique_ptr<RewardBound> parse_RewardBound(const nlohmann::json& input);
    std::unique_ptr<Synchronisation> parse_Synchronisation(const nlohmann::json& input);
    std::unique_ptr<TransientValue> parse_TransientValue(const nlohmann::json& input);
    std::unique_ptr<VariableDeclaration> parse_VariableDeclaration(const nlohmann::json& input);
    /* Non-standard. */
    std::unique_ptr<FreeVariableDeclaration> parse_FreeVariableDeclaration(const nlohmann::json& input);
    static std::unique_ptr<RewardAccumulation> parse_reward_accumulation(const nlohmann::json& input);

    /* Expressions. */
    std::unique_ptr<PropertyExpression> parse_PropertyExpression(const nlohmann::json& input, bool within_filter);
    std::unique_ptr<Expression> parse_Expression(const nlohmann::json& input);
    std::unique_ptr<LValueExpression> parse_LValueExpression(const nlohmann::json& input);
    std::unique_ptr<ArrayAccessExpression> parse_array_access_expression(const nlohmann::json& input);
    std::unique_ptr<ArrayConstructorExpression> parse_array_constructor_expression(const nlohmann::json& input);
    std::unique_ptr<ArrayValueExpression> parse_array_value_expression(const nlohmann::json& input);
    std::unique_ptr<BinaryOpExpression> parse_BinaryOpExpression(const nlohmann::json& input);
    std::unique_ptr<ConstantExpression> parse_ConstantExpression(const nlohmann::json& input);
    std::unique_ptr<DerivativeExpression> parse_DerivativeExpression(const nlohmann::json& input);
    static std::unique_ptr<DistributionSamplingExpression> parse_DistributionSamplingExpression(const nlohmann::json& input);
    std::unique_ptr<ExpectationExpression> parse_ExpectationExpression(const nlohmann::json& input);
    std::unique_ptr<FilterExpression> parse_FilterExpression(const nlohmann::json& input);
    std::unique_ptr<ITE_Expression> parse_ITE_Expression(const nlohmann::json& input);
    std::unique_ptr<PathExpression> parse_PathExpression(const nlohmann::json& input);
    std::unique_ptr<QfiedExpression> parse_QfiedExpression(const nlohmann::json& input);
    [[nodiscard]] std::unique_ptr<RealValueExpression> parse_RealValueExpression(const nlohmann::json& input) const;
    std::unique_ptr<UnaryOpExpression> parse_UnaryOpExpression(const nlohmann::json& input);
    std::unique_ptr<VariableExpression> parse_VariableExpression(const nlohmann::json& input);
    static std::unique_ptr<BoolValueExpression> parse_BooleanValueExpression(const nlohmann::json& input);
    static std::unique_ptr<IntegerValueExpression> parse_IntegerValueExpression(const nlohmann::json& input);
    [[maybe_unused]] static std::unique_ptr<StatePredicateExpression> parse_StatePredicateExpression(const nlohmann::json& input);
    /* Non-standard. */
    [[nodiscard]] std::unique_ptr<ActionOpTuple> parse_action_op_tuple(const nlohmann::json& input) const;
    [[nodiscard]] std::unique_ptr<ConstantArrayAccessExpression> parse_constant_array_access_expression_if(const nlohmann::json& input);
    std::unique_ptr<LetExpression> parse_LetExpression(const nlohmann::json& input);
    std::unique_ptr<LocationValueExpression> parse_location_value_expression(const nlohmann::json& input);
    std::unique_ptr<ObjectiveExpression> parse_ObjectiveExpression(const nlohmann::json& input);
    std::unique_ptr<PAExpression> parse_PAExpression(const nlohmann::json& input);
    std::unique_ptr<PredicatesExpression> parse_PredicatesExpression(const nlohmann::json& input);
    std::unique_ptr<ProblemInstanceExpression> parse_problem_instance_expression(const nlohmann::json& input);
    std::unique_ptr<StateConditionExpression> parse_StateConditionExpression(const nlohmann::json& input);
    std::unique_ptr<StateValuesExpression> parse_StateValuesExpression(const nlohmann::json& input);
    std::unique_ptr<StatesValuesExpression> parse_StatesValuesExpression(const nlohmann::json& input);
    std::unique_ptr<VariableValueExpression> parse_VariableValueExpression(const nlohmann::json& input);

    /* Types. */
    std::unique_ptr<DeclarationType> parse_DeclarationType(const nlohmann::json& input);
    static std::unique_ptr<BasicType> parse_BasicType(const nlohmann::json& input);
    std::unique_ptr<ArrayType> parse_ArrayType(const nlohmann::json& input);
    std::unique_ptr<BoundedType> parse_BoundedType(const nlohmann::json& input);
    [[maybe_unused]] static std::unique_ptr<BoolType> parse_BoolType(const nlohmann::json& input);
    [[maybe_unused]] std::unique_ptr<ClockType> parse_ClockType(const nlohmann::json& input);
    [[maybe_unused]] std::unique_ptr<ContinuousType> parse_ContinuousType(const nlohmann::json& input);
    [[maybe_unused]] static std::unique_ptr<IntType> parse_IntType(const nlohmann::json& input);
    [[maybe_unused]] static std::unique_ptr<RealType> parse_RealType(const nlohmann::json& input);

};

/* To enable parsing without including all the parser stuff. */
namespace PARSER {
    extern std::unique_ptr<Model> parse(const std::string* filename, const PLAJA::OptionParser* option_parser, bool check_constants);
    extern std::unique_ptr<Model> parse(const std::string* filename, const PLAJA::OptionParser* option_parser);
}

#endif //PLAJA_PARSER_H
