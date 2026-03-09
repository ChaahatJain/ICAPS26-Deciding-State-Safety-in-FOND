//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent and (2023 - 2024) Chaahat Jain.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PROPERTY_INFORMATION_H
#define PLAJA_PROPERTY_INFORMATION_H

#include <memory>
#include "../../include/ct_config_const.h"
#include "../../parser/ast/expression/forward_expression.h"
#include "../../parser/ast/forward_ast.h"
#include "../../utils/default_constructors.h"
#include "jani2nnet/forward_jani_2_nnet.h"
#include "../factories/forward_factories.h"
#include "jani_2_interface.h"
#include "jani2nnet/jani_2_nnet.h"
#ifdef USE_VERITAS
#include "jani2ensemble/jani_2_ensemble.h"
#endif
#include "../../assertions.h"

/**
 * Class to extract information of properties like reachability, Pmin Pmax, and Emin, but also predicate abstraction.
 * This class will check that the properties are in a form expected by the engine assuming the current restrictions of the parser.
 * However, as we skip properties not supported, most restrictions will have to be rechecked, since the parser cannot simply reject.
 */
struct PropertyInformation final {
    enum PropertyType { NONE, GOAL, PROBMIN, PROBMAX, MINCOST, PA_PROPERTY, ProblemInstanceProperty };
private:
    PropertyType propertyType;
    const Property* property;
    const Model* model;
    std::unique_ptr<PropertyExpression> propertyOwn;
    /* */
    const Expression* startExp;
    const Expression* reachExp;
#ifdef ENABLE_TERMINAL_STATE_SUPPORT
    const Expression* terminalExp;
#endif
    const Expression* costExp;
    const PredicatesExpression* predicates;
    const ObjectiveExpression* learningObjective;
    std::unique_ptr<Jani2Interface> interface;

    // internal auxiliaries:
    static const Expression* extract_goal_exp(const PathExpression* values, /*the following are just for output*/const Property& property);
    static std::unique_ptr<PropertyInformation> analyse_prob_prop(const PLAJA::Configuration& config, const QfiedExpression* values, const Property& property, const Model& model);
    static std::unique_ptr<PropertyInformation> analyse_cost_prop(const PLAJA::Configuration& config, const ExpectationExpression* values, const Property& property, const Model& model);

    void load_interface(const PLAJA::Configuration& config);

public:
    // PropertyInformation(PropertyType property_type, const Property* property_, const Model* model, std::unique_ptr<PropertyExpression>&& property_own);
    PropertyInformation(const PLAJA::Configuration& config, PropertyType property_type, const Property* property, const Model* model);
    ~PropertyInformation();
    DELETE_CONSTRUCTOR(PropertyInformation)

    inline void set_start(const Expression* start) { startExp = start; }

    inline void set_reach(const Expression* reach) { reachExp = reach; }

#ifdef ENABLE_TERMINAL_STATE_SUPPORT

    inline void set_terminal(const Expression* terminal) { terminalExp = terminal; }

#else

    inline void set_terminal(const Expression*) { PLAJA_ABORT; }

#endif

    inline void set_cost(const Expression* cost) { costExp = cost; }

    inline void set_predicates(const PredicatesExpression* predicates_) { predicates = predicates_; }

    inline void set_learning_objective(const ObjectiveExpression* learning_objective) { learningObjective = learning_objective; }

    [[nodiscard]] inline const Property* _property() const { return property; }

    [[nodiscard]] inline PropertyType _property_type() const { return propertyType; }

    [[nodiscard]] inline bool is_non_property() const { return propertyType == PropertyType::NONE; }

    [[nodiscard]] inline bool is_reach_property() const { return propertyType == PropertyType::GOAL; }

    [[nodiscard]] inline bool is_prob_property() const { return propertyType == PropertyType::PROBMIN or propertyType == PropertyType::PROBMAX or propertyType == PropertyType::MINCOST; }

    [[nodiscard]] inline bool is_pa_property() const { return propertyType == PropertyType::PA_PROPERTY; }

    [[nodiscard]] inline const Property* get_property() const { return property; }

    [[nodiscard]] const std::string& get_property_name() const;
    [[nodiscard]] const PropertyExpression* get_property_expression() const;

    [[nodiscard]] inline const Model* get_model() const { return model; }

    [[nodiscard]] inline const Expression* get_start() const { return startExp; }

    [[nodiscard]] inline const Expression* get_reach() const { return reachExp; }

#ifdef ENABLE_TERMINAL_STATE_SUPPORT

    [[nodiscard]] inline const Expression* get_terminal() const { return terminalExp; }

#else

    [[nodiscard]] inline const Expression* get_terminal() const { PLAJA_ABORT; }

#endif

    [[nodiscard]] std::unique_ptr<Expression> get_non_terminal() const;

    [[nodiscard]] std::unique_ptr<Expression> get_non_terminal_reach() const;

    [[nodiscard]] inline const Expression* get_cost() const { return costExp; }

    [[nodiscard]] inline const PredicatesExpression* get_predicates() const { return predicates; }

    [[nodiscard]] inline const ObjectiveExpression* get_learning_objective() const { return learningObjective; }

    [[nodiscard]] inline Jani2Interface* get_interface() const { return interface.get(); }

    [[nodiscard]] bool has_nn_interface() const { auto nnInterface = dynamic_cast<Jani2NNet*>(interface.get()); return nnInterface; }


    [[nodiscard]] inline Jani2NNet* get_nn_interface() const { auto nnInterface = dynamic_cast<Jani2NNet*>(interface.get()); PLAJA_ASSERT(nnInterface); return nnInterface; }

#ifdef USE_VERITAS
    [[nodiscard]] bool has_ensemble_interface() const { auto ensembleInterface = dynamic_cast<Jani2Ensemble*>(interface.get()); return ensembleInterface; }

    [[nodiscard]] inline Jani2Ensemble* get_ensemble_interface() const { auto ensembleInterface = dynamic_cast<Jani2Ensemble*>(interface.get()); PLAJA_ASSERT(ensembleInterface); return ensembleInterface; }
#endif

    [[nodiscard]] const std::string* get_policy_file() const {return &(interface->get_policy_file()); }
    
    [[nodiscard]] const std::string* get_interface_file() const {return &(interface->get_interface_file()); }
    
    static std::unique_ptr<PropertyInformation> construct_property_information(PropertyType property_type, const Property* property, const Model* model);

    /**
     * Analyse property and load property information structure.
     * @throw PropertyAnalysisException if property is not supported by search engine.
     */
    static std::unique_ptr<PropertyInformation> analyse_property(const Property& property, const Model& model);
    static std::unique_ptr<PropertyInformation> analyse_property(const PLAJA::Configuration& config, const Property& property, const Model& model);
};

#endif //PLAJA_PROPERTY_INFORMATION_H
