//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_MODEL_H
#define PLAJA_MODEL_H

#include <memory>
#include <unordered_set>
#include <vector>
#include "../../search/information/forward_information.h"
#include "../using_parser.h"
#include "expression/forward_expression.h"
#include "ast_element.h"
#include "commentable.h"
#include "forward_ast.h"

class Model final: public AstElement {
    friend class ModelIterator;

public:
    enum ModelType { LTS, /*DTMC, CTMC,*/ MDP, /*CTMDP, MA,*/ TA, PTA, STA, HA, PHA, SHA };
    enum ModelFeature { ARRAYS, /*DATATYPES,*/ DERIVED_OPERATORS, /*EDGE_PRIORITIES, FUNCTIONS, HYPERBOLIC_FUNCTIONS, NAMED_EXPRESSIONS, NONDET_SELECTION,*/ STATE_EXIT_REWARDS, /*TRADEOFF_PROPERTIES, TRIGONOMETRIC_FUNCTIONS*/ };

private:
    /* Ast. */
    unsigned short janiVersion;
    std::string name;
    ModelType modelType;
    std::vector<ModelFeature> modelFeatures;
    std::vector<std::unique_ptr<Action>> actions;
    std::vector<std::unique_ptr<ConstantDeclaration>> constants;
    std::vector<std::unique_ptr<VariableDeclaration>> variables;

    /* Restrict initial. */
    std::unique_ptr<Expression> restrictInitialExpression;
    FIELD_IF_COMMENT_PARSING(std::string restrictInitialComment;)

    std::vector<std::unique_ptr<Property>> properties;
    std::unique_ptr<Composition> system;
    std::vector<AutomatonIndex_type> automata; // the description list of automata via the index of a instance in automataInstances (for proper model output)
    std::vector<std::unique_ptr<Automaton>> automataInstances; // instances of automata as described via system

    FIELD_IF_COMMENT_PARSING(std::unique_ptr<Metadata> metaData;)

    /* Additional data. */
    std::unique_ptr<ModelInformation> modelInformation;

public:
    explicit Model(ModelType model_type);
    ~Model() override;
    DELETE_CONSTRUCTOR(Model)

    /* Static */
    static const std::string& model_type_to_str(ModelType type);
    static std::unique_ptr<ModelType> str_to_model_type(const std::string& type_str);
    static const std::string& model_feature_to_str(ModelFeature feature);
    static std::unique_ptr<ModelFeature> str_to_model_feature(const std::string& feature_str);

    /* Construction. */
    void reserve(std::size_t model_features_cap, std::size_t actions_cap, std::size_t constants_cap, std::size_t variables_cap, std::size_t properties_cap);
    void reserve_automata(std::size_t automata_cap, std::size_t automata_instances_cap);
    void add_model_feature(ModelFeature model_feature);
    void add_action(std::unique_ptr<Action>&& action);
    void add_constant(std::unique_ptr<ConstantDeclaration>&& constant);
    void add_variable(std::unique_ptr<VariableDeclaration>&& variable);
    void add_property(std::unique_ptr<Property>&& prop);

    /* Automata. */
    /** Note: may only add description with a corresponding instance already in the model. */
    void add_automaton(AutomatonIndex_type instance_index);
    void add_automatonInstance(std::unique_ptr<Automaton>&& automaton_instance);

    /* Setter. */

    void set_jani_version(unsigned short jani_version) { janiVersion = jani_version; }

    inline void set_name(std::string name_c) { name = std::move(name_c); }

    [[maybe_unused]] inline void set_model_type(ModelType model_type) { modelType = model_type; }

    [[maybe_unused]] inline void set_model_feature(ModelFeature model_feature, std::size_t index) {
        PLAJA_ASSERT(index < modelFeatures.size())
        modelFeatures[index] = model_feature;
    }

    [[maybe_unused]] void set_action(std::unique_ptr<Action>&& action, std::size_t index);

    [[maybe_unused]] std::unique_ptr<ConstantDeclaration> set_constant(std::unique_ptr<ConstantDeclaration>&& constant, std::size_t index);

    std::unique_ptr<VariableDeclaration> set_variable(std::unique_ptr<VariableDeclaration>&& var, std::size_t index);

    void set_restrict_initial_expression(std::unique_ptr<Expression>&& restrict_init_exp);

    inline void set_restrict_initial_comment(const std::string& restrict_init_comment) { SET_IF_COMMENT_PARSING(restrictInitialComment, restrict_init_comment) }

    [[maybe_unused]] void set_property(std::unique_ptr<Property>&& prop, std::size_t index);

    void set_system(std::unique_ptr<Composition>&& sys);

    void set_metadata(std::unique_ptr<Metadata>&& meta_data);

    /* Automata. */
    [[maybe_unused]] void set_automaton(AutomatonIndex_type instance, std::size_t index);
    [[maybe_unused]] void set_automatonInstance(std::unique_ptr<Automaton>&& automaton_instance, AutomatonIndex_type instance_index);

    /* Getter. */

    [[nodiscard]] inline unsigned short get_jani_version() const { return janiVersion; }

    [[nodiscard]] inline const std::string& get_name() const { return name; }

    [[nodiscard]] inline ModelType get_model_type() const { return modelType; }

    [[nodiscard]] inline std::size_t get_number_model_features() const { return modelFeatures.size(); }

    [[nodiscard]] inline ModelFeature get_model_feature(std::size_t index) const {
        PLAJA_ASSERT(index < modelFeatures.size())
        return modelFeatures[index];
    }

    [[nodiscard]] inline const std::vector<ModelFeature>& get_model_features() const { return modelFeatures; }

    [[nodiscard]] inline std::size_t get_number_actions() const { return actions.size(); }

    [[nodiscard]] inline Action* get_action(ActionID_type action_id) {
        PLAJA_ASSERT(action_id < actions.size())
        return actions[action_id].get();
    }

    [[nodiscard]] inline const Action* get_action(ActionID_type action_id) const {
        PLAJA_ASSERT(action_id < actions.size())
        return actions[action_id].get();
    }

    [[nodiscard]] inline std::size_t get_number_constants() const { return constants.size(); }

    [[nodiscard]] inline ConstantDeclaration* get_constant(std::size_t index) {
        PLAJA_ASSERT(index < constants.size())
        return constants[index].get();
    }

    [[nodiscard]] inline const ConstantDeclaration* get_constant(std::size_t index) const {
        PLAJA_ASSERT(index < constants.size())
        return constants[index].get();
    }

    [[nodiscard]] inline std::size_t get_number_variables() const { return variables.size(); }

    [[nodiscard]] inline VariableDeclaration* get_variable(std::size_t index) {
        PLAJA_ASSERT(index < variables.size())
        return variables[index].get();
    }

    [[nodiscard]] inline const VariableDeclaration* get_variable(std::size_t index) const {
        PLAJA_ASSERT(index < variables.size())
        return variables[index].get();
    }

    [[nodiscard]] inline Expression* get_restrict_initial_expression() { return restrictInitialExpression.get(); }

    [[nodiscard]] inline const Expression* get_restrict_initial_expression() const { return restrictInitialExpression.get(); }

    [[nodiscard]] inline const std::string& get_restrict_initial_comment() const { return GET_IF_COMMENT_PARSING(restrictInitialComment); }

    [[nodiscard]] inline std::size_t get_number_properties() const { return properties.size(); }

    [[nodiscard]] inline Property* get_property(std::size_t index) { return properties[index].get(); }

    [[nodiscard]] inline const Property* get_property(std::size_t index) const { return properties[index].get(); }

    [[nodiscard]] inline Composition* get_system() { return system.get(); }

    [[nodiscard]] inline const Composition* get_system() const { return system.get(); }

    [[nodiscard]] inline Metadata* get_metadata() { return GET_IF_COMMENT_PARSING_ALT(metaData.get(), nullptr); }

    [[nodiscard]] inline const Metadata* get_metadata() const { return GET_IF_COMMENT_PARSING_ALT(metaData.get(), nullptr); }

    // automata
    [[nodiscard]] inline std::size_t get_number_automata() const { return automata.size(); } // description ...

    /** @return An instance of the automaton description. */
    [[nodiscard]] const Automaton* get_automaton(std::size_t index) const;

    [[nodiscard]] inline std::size_t get_number_automataInstances() const { return automataInstances.size(); } // instances ...
    inline Automaton* get_automatonInstance(AutomatonIndex_type automaton_index) { return automataInstances[automaton_index].get(); }

    [[nodiscard]] inline const Automaton* get_automatonInstance(AutomatonIndex_type automaton_index) const {
        PLAJA_ASSERT(automaton_index < automataInstances.size())
        return automataInstances[automaton_index].get();
    }

    FCT_IF_DEBUG([[nodiscard]] const Automaton* get_automaton_by_name(const std::string& automaton_name) const;)

    /** Deep copy of a model. */
    [[nodiscard]] std::unique_ptr<Model> deep_copy() const;

    /* Override. */
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;

    /* Further (auxiliary) routines. */

    [[nodiscard]] const std::string& get_action_name(ActionID_type action_id) const;

    [[nodiscard]] std::size_t get_number_all_variables() const;

    /**
     * id od the variable instance in the model's system
     * (*not* the variable state index: the identification of the variable in the context of states)
     */
    VariableDeclaration* get_variable_by_id(VariableID_type id);
    [[nodiscard]] const VariableDeclaration* get_variable_by_id(VariableID_type id) const;
    [[nodiscard]] VariableIndex_type get_variable_index_by_id(VariableID_type id) const;
    [[nodiscard]] AutomatonIndex_type get_variable_automaton_instance(VariableID_type id) const;
    // ID must be valid according to some invariant; currently: global variables, then the local variables in ascending order of the system's automaton index.
#ifndef NDEBUG
    [[nodiscard]] const VariableDeclaration& get_variable_by_name(const std::string& var_name) const;
    [[nodiscard]] VariableID_type get_variable_id_by_name(const std::string& var_name) const;
    [[nodiscard]] VariableIndex_type get_variable_index_by_name(const std::string& var_name) const;
#endif
#ifdef RUNTIME_CHECKS
    [[nodiscard]] const VariableDeclaration& get_variable_by_index(VariableIndex_type var_index) const;
    [[nodiscard]] std::string gen_state_index_str(VariableIndex_type index) const;
#endif

    [[nodiscard]] std::unique_ptr<Expression> gen_loc_value_expr(AutomatonIndex_type location, PLAJA::integer value) const;
    [[nodiscard]] std::unique_ptr<Expression> gen_var_expr(VariableIndex_type state_index, const VariableDeclaration* var_decl) const;

    /* Var to const. */
    [[nodiscard]] std::unique_ptr<VariableDeclaration> remove_variable(std::size_t index);
    [[nodiscard]] std::unique_ptr<VariableDeclaration> remove_variable(AutomatonIndex_type automaton_index, std::size_t index);
    [[nodiscard]] static std::unique_ptr<ConstantDeclaration> transform_variable_to_const(VariableDeclaration& var, ConstantIdType id);
    const ConstantDeclaration* transform_variable_to_const(VariableDeclaration& var);

    [[nodiscard]] std::unordered_set<VariableIndex_type> collect_guard_variables(bool include_locs) const;

    /* After all checks. */
    void compute_model_information();

    [[nodiscard]] const ModelInformation& get_model_information() const {
        PLAJA_ASSERT(modelInformation);
        return *modelInformation;
    }

    /******************************************************************************************************************/

    class VariableIterator {
        friend Model;

    private:
        const Model& model;
        VariableID_type currentId;
        const std::size_t numVars;

        explicit VariableIterator(const Model& model):
            model(model)
            , currentId(0)
            , numVars(model.get_number_all_variables()) {}

    public:
        inline void operator++() { ++currentId; }

        [[nodiscard]] inline bool end() const { return currentId >= numVars; }

        [[nodiscard]] inline VariableID_type variable_id() const { return currentId; }

        [[nodiscard]] inline const VariableDeclaration* variable() const { return model.get_variable_by_id(currentId); }

        [[nodiscard]] VariableIndex_type variable_index() const;
    };

    [[nodiscard]] inline VariableIterator variableIterator() const { return VariableIterator(*this); }

};

#endif //PLAJA_MODEL_H
