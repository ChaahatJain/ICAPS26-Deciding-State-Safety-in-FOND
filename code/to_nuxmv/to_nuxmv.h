//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent and (2023-2024) Chaahat Jain.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TO_NUXMV_H
#define PLAJA_TO_NUXMV_H

#include <list>
#include <string>
#include <unordered_set>
#include "../parser/ast_forward_declarations.h"
#include "../parser/nn_parser/using_nn.h"
#include "../search/fd_adaptions/search_engine.h"
#include "../search/successor_generation/forward_successor_generation.h"
#include "../search/states/forward_states.h"
#include "../search/using_search.h"
#include "addtree.hpp"
class ToNuxmv final: public SearchEngine {

private:
    std::string modelFile;
    bool useExModel;

    bool invarForReals; // TODO that actually correct?
    bool useCase;
    bool defGuards;
    bool defUpdates;
    bool defReach;
    bool useGuardPerUpdate;
    bool mergeUpdates;

    /* Legacy config. */
    bool backwardsDefOrder;
    bool backwardsBoolVars;
    bool backwardsCopyUpdates;
    bool backwardsOpExclusion;
    bool backwardsDefGoal;

    bool backwardsNnPrecision;
    bool backwardsPolSel;
    bool backwardsPolModule;
    bool backwardsPolModuleStr;
    // TODO Maybe option for non-array vars in module.

    bool backwardsSpecialTrivialBounds;

    bool defTrStructs;
    mutable std::unordered_set<VariableID_type> booleanVars;

    std::unique_ptr<std::string> nuxmvConfigFile;
    std::unique_ptr<std::string> nuxmvConfig;

    std::unique_ptr<std::string> nuxmvEx;

    std::unique_ptr<SuccessorGeneratorC> successorGenerator;

    SearchStatus initialize() override;
    SearchStatus step() override;

    /* Config to NuXmv. */

    [[nodiscard]] std::string generate_default_config(const PLAJA::Configuration& config) const;

    /* Ast to NuXmv. */

    [[nodiscard]] std::string vars_to_nuxmv() const;
    [[nodiscard]] std::unique_ptr<std::string> real_bounds_to_nuxmv(bool parenthesis) const;

    [[nodiscard]] std::string state_to_nuxmv(const StateBase& state) const;
    [[nodiscard]] std::string start_to_nuxmv() const;
    [[nodiscard]] std::string reach_to_nuxmv() const;

    /* Operators. */

    [[nodiscard]] std::unique_ptr<std::string> label_to_nuxmv(ActionLabel_type label) const;

    [[nodiscard]] std::string op_to_nuxmv(const ActionOp& op, bool parenthesis) const;
    [[nodiscard]] std::unique_ptr<std::string> guard_to_nuxmv(const ActionOp& op, bool use_define, bool parenthesis) const;
    [[nodiscard]] std::string update_to_nuxmv(ActionOpID_type op_id, UpdateIndex_type upd_index, const Update* update, bool use_define, bool parenthesis, const std::unordered_set<VariableID_type>* ignored_vars = nullptr) const;
    [[nodiscard]] std::string assignment_to_nuxmv(const Assignment& assignment, bool parenthesis) const;
    [[nodiscard]] std::string updates_to_nuxmv(const ActionOp& op, bool use_define, bool parenthesis) const;
    [[nodiscard]] std::string generate_next(const LValueExpression* ref, const Expression* value) const;

    [[nodiscard]] std::string trans_struct_def_to_nuxmv() const;
    [[nodiscard]] std::string exclude_other_same_labeled_ops_to_nuxmv(ActionLabel_type label, ActionOpID_type op_id, UpdateIndex_type upd_index) const;

    [[nodiscard]] static std::string guard_to_str(ActionOpID_type op_id);
    [[nodiscard]] static std::string update_to_str(ActionOpID_type op_id, UpdateIndex_type upd_index);

    /* */

    [[nodiscard]] std::string trans_to_nuxmv() const;

    /* Policy. */

    [[nodiscard]] std::string nn_to_nuxmv() const;
    [[nodiscard]] std::string tree_to_nuxmv(veritas::Tree tree, int tree_index) const;
    [[nodiscard]] std::string ensemble_to_nuxmv() const;
    [[nodiscard]] std::string selection_constraint_to_nuxmv(OutputIndex_type output, const std::string* array_var /*Backwards: Use instead of neuron index.*/, bool exploit_case) const;
    [[nodiscard]] std::string policy_module_to_nuxmv() const;
    [[nodiscard]] std::string selection_constraint_policy_module(OutputIndex_type output) const;

    [[nodiscard]] std::string neuron_to_str(LayerIndex_type layer, NeuronIndex_type neuron) const;
    [[nodiscard]] std::string label_app_to_str(ActionLabel_type label) const;

    [[nodiscard]] static const std::string& get_policy_module_var();
    [[nodiscard]] static const std::string& get_policy_module_arg_var();
    [[nodiscard]] static const std::string& get_policy_module_int_var();
    [[nodiscard]] static const std::string& get_policy_module_str_var();

public:
    explicit ToNuxmv(const PLAJA::Configuration& config);
    ~ToNuxmv() override;
    DELETE_CONSTRUCTOR(ToNuxmv)

    [[nodiscard]] inline bool specialize_trivial_bounds() const { return backwardsSpecialTrivialBounds; }

    [[nodiscard]] inline bool is_marked_boolean(VariableID_type var) const { return booleanVars.count(var); }

    /* Aux. */

    [[nodiscard]] inline std::string ast_to_nuxmv(const class AstElement* ast_element, bool numeric_env = false) const { return ast_to_nuxmv(*ast_element, numeric_env); }

    [[nodiscard]] std::string ast_to_nuxmv(const AstElement& ast_element, bool numeric_env = false) const;

    [[nodiscard]] std::string ast_to_nuxmv(const AstElement& ast_element, bool numeric_env, bool parenthesis) const;

    [[nodiscard]] static std::string generate_var_def(const std::string& name, const std::string& domain);

    [[nodiscard]] static std::string generate_def(const std::string& name, const std::string& def);

    [[nodiscard]] static std::string generate_next(const std::string& var, const std::string& assignment);

    [[nodiscard]] static std::string generate_to_int(const std::string& expr);

    [[nodiscard]] static std::string generate_aa(const std::string& var, std::size_t index);

    [[nodiscard]] static std::string generate_case(const std::list<std::pair<std::string, std::string>>& cases, unsigned int tabs);

};

#endif //PLAJA_TO_NUXMV_H
