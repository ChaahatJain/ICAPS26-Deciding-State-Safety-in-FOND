//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent and (2023-2024) Chaahat Jain.
// See README.md in the top-level directory for licensing information.
//

#include <sstream>
#include "to_nuxmv.h"

#include "../exception/constructor_exception.h"
#include "../exception/not_implemented_exception.h"
#include "../option_parser/enum_option_values_set.h"

#include "../parser/ast/expression/non_standard/objective_expression.h"
#include "../parser/ast/expression/lvalue_expression.h"
#include "../parser/ast/expression/unary_op_expression.h"
#include "../parser/ast/type/declaration_type.h"
#include "../parser/ast/assignment.h"
#include "../parser/ast/automaton.h"
#include "../parser/ast/edge.h"
#include "../parser/ast/model.h"
#include "../parser/ast/variable_declaration.h"
#include "../parser/nn_parser/neural_network.h"
#include "../parser/visitor/extern/to_normalform.h"

#include "../search/factories/configuration.h"
#include "../../search/information/jani_2_interface.h"
#include "../search/information/jani2nnet/jani_2_nnet.h"
#include "../search/information/jani2nnet/input_feature_to_jani.h"
#include "../search/information/model_information.h"
#include "../search/information/property_information.h"
#include "../search/states/state_values.h"
#include "../search/successor_generation/action_op.h"
#include "../search/successor_generation/successor_generator_c.h"
#include "../search/successor_generation/update.h"

#include "../utils/utils.h"
#include "to_nuxmv_options.h"
#include "to_nuxmv_visitor.h"
#include "using_nuxmv.h"

namespace TO_NUXMV {

    extern std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet> construct_default_configs_enum();

    std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet> construct_default_configs_enum() {
        return std::make_unique<PLAJA_OPTION::EnumOptionValuesSet>(
            std::list<PLAJA_OPTION::EnumOptionValue> { PLAJA_OPTION::EnumOptionValue::None,
                                                       PLAJA_OPTION::EnumOptionValue::Bmc, PLAJA_OPTION::EnumOptionValue::SBmc,
                                                       PLAJA_OPTION::EnumOptionValue::IpaBmc, PLAJA_OPTION::EnumOptionValue::IpaCegar,
                                                       PLAJA_OPTION::EnumOptionValue::Ic3, PLAJA_OPTION::EnumOptionValue::Cegar, PLAJA_OPTION::EnumOptionValue::Coi },
            PLAJA_OPTION::EnumOptionValue::None
        );
    }

    /******************************************************************************************************************/

    class AllActionOpIterator {

    private:
        AllActionOpIteratorBase<const ActionOp, std::unique_ptr<ActionOp>, std::vector> it;

    public:
        explicit AllActionOpIterator(const SuccessorGeneratorC& suc_gen):
            it(suc_gen.init_action_op_it_static()) {}

        virtual ~AllActionOpIterator() = default;
        DELETE_CONSTRUCTOR(AllActionOpIterator)

        virtual void operator++() { ++it; }

        [[nodiscard]] virtual bool end() const { return it.end(); }

        [[nodiscard]] virtual const ActionOp* operator()() const { return it.operator()(); }

        [[nodiscard]] virtual const ActionOp* operator->() const { return it.operator->(); }

        [[nodiscard]] virtual const ActionOp& operator*() const { return it.operator*(); }

    };

    /** For backwards compatibility. */
    class AllActionOpIteratorAst final: public AllActionOpIterator {

    private:
        const SuccessorGeneratorC* sucGen;
        const ModelInformation* modelInfo;
        const SynchronisationInformation* syncInfo;
        AstConstIterator<Edge> it;
        ActionOpStructure opStruct;

        bool init_op_struct() {

            if (it.end()) { return true; }

            if (not it->get_action()) {
                opStruct.syncID = SYNCHRONISATION::nonSyncID;
            } else {
                const auto& syncs_for_edge = syncInfo->relevantSyncs[opStruct.participatingEdges.front().first][it->get_action_id_labeled()];
                if (syncs_for_edge.empty()) { return false; }
                PLAJA_ASSERT(syncs_for_edge.size() == 1)
                opStruct.syncID = ModelInformation::sync_index_to_id(syncs_for_edge.front());
            }
            opStruct.participatingEdges.front().second = it->get_id();

            return true;
        }

    public:
        AllActionOpIteratorAst(const Automaton& automaton, const SuccessorGeneratorC& suc_gen):
            AllActionOpIterator(suc_gen)
            , sucGen(&suc_gen)
            , modelInfo(&sucGen->get_model_information())
            , syncInfo(&modelInfo->get_synchronisation_information())
            , it(automaton.edgeIterator()) {
            opStruct.syncID = SYNCHRONISATION::nonSyncID;
            opStruct.participatingEdges = { { automaton.get_index(), EDGE::nullEdge } };
            while (not init_op_struct()) { ++it; }
        }

        ~AllActionOpIteratorAst() override = default;
        DELETE_CONSTRUCTOR(AllActionOpIteratorAst)

        void operator++() override {
            ++it;
            while (not init_op_struct()) { ++it; }
        }

        [[nodiscard]] bool end() const override { return it.end(); }

        [[nodiscard]] const ActionOp* operator()() const override { return &sucGen->get_action_op(opStruct); }

        [[nodiscard]] const ActionOp* operator->() const override { return &sucGen->get_action_op(opStruct); }

        [[nodiscard]] const ActionOp& operator*() const override { return sucGen->get_action_op(opStruct); }

    };

    [[nodiscard]] std::unique_ptr<AllActionOpIterator> init_all_op_it(const Model& model, SuccessorGeneratorC& suc_gen, bool ast_order) {
        if (ast_order and model.get_number_automataInstances() == 1) {
            return std::make_unique<AllActionOpIteratorAst>(*model.get_automatonInstance(0), suc_gen);
        } else { return std::make_unique<AllActionOpIterator>(suc_gen); }
    }

};

/**********************************************************************************************************************/

ToNuxmv::ToNuxmv(const PLAJA::Configuration& config):
    SearchEngine(config)
    , modelFile(config.get_option(PLAJA_OPTION::nuxmv_model))
    , useExModel(config.get_bool_option(PLAJA_OPTION::nuxmv_use_ex_model) and PLAJA_UTILS::file_exists(modelFile))
    , invarForReals(config.get_bool_option(PLAJA_OPTION::nuxmv_invar_for_reals))
    , useCase(config.get_bool_option(PLAJA_OPTION::nuxmv_use_case))
    , defGuards(config.get_bool_option(PLAJA_OPTION::nuxmv_def_guards) or (propertyInfo->get_interface() and propertyInfo->get_interface()->_do_applicability_filtering()))
    , defUpdates(config.get_bool_option(PLAJA_OPTION::nuxmv_def_updates))
    , defReach(config.get_bool_option(PLAJA_OPTION::nuxmv_def_reach))
    , useGuardPerUpdate(config.get_bool_option(PLAJA_OPTION::nuxmv_use_guard_per_update))
    , mergeUpdates(not useGuardPerUpdate and config.get_bool_option(PLAJA_OPTION::nuxmv_merge_updates))
    /**/
    , backwardsDefOrder(config.get_bool_option(PLAJA_OPTION::nuxmv_backwards_def_order))
    , backwardsBoolVars(config.get_bool_option(PLAJA_OPTION::nuxmv_backwards_bool_vars))
    , backwardsCopyUpdates(config.get_bool_option(PLAJA_OPTION::nuxmv_backwards_copy_updates))
    , backwardsOpExclusion(defGuards and defUpdates and config.get_bool_option(PLAJA_OPTION::nuxmv_backwards_op_exclusion))
    , backwardsDefGoal(config.get_bool_option(PLAJA_OPTION::nuxmv_backwards_def_goal))
    /**/
    , backwardsNnPrecision(config.get_bool_option(PLAJA_OPTION::nuxmv_backwards_nn_precision))
    , backwardsPolSel(config.get_bool_option(PLAJA_OPTION::nuxmv_backwards_policy_selection))
    , backwardsPolModule(config.get_bool_option(PLAJA_OPTION::nuxmv_backwards_policy_module))
    , backwardsPolModuleStr(config.get_bool_option(PLAJA_OPTION::nuxmv_backwards_policy_module_str))
    /**/
    , backwardsSpecialTrivialBounds(config.get_bool_option(PLAJA_OPTION::nuxmv_backwards_special_trivial_bounds))
    /**/
    , defTrStructs(defGuards or defUpdates or defReach or backwardsDefGoal)
    , nuxmvConfigFile(config.has_value_option(PLAJA_OPTION::nuxmv_config_file) ? std::make_unique<std::string>(config.get_value_option_string(PLAJA_OPTION::nuxmv_config_file)) : nullptr)
    , nuxmvConfig(nullptr)
    , nuxmvEx(config.has_value_option(PLAJA_OPTION::nuxmv_ex) ? std::make_unique<std::string>(config.get_value_option_string(PLAJA_OPTION::nuxmv_ex)) : nullptr)
    , successorGenerator(new SuccessorGeneratorC(config, *model)) {

    if (model->get_model_information().compute_location_space_size() != 1) { throw NotImplementedException(__PRETTY_FUNCTION__); }

    /* Prepare config. */

    if (nuxmvConfigFile) {

        const bool has_nuxmv_config = config.has_value_option(PLAJA_OPTION::nuxmv_config);
        const bool has_nuxmv_default_config = config.has_enum_option(PLAJA_OPTION::nuxmv_default_engine, true);

        if (not(has_nuxmv_config or has_nuxmv_default_config)) {
            throw ConstructorException(PLAJA_UTILS::string_f(R"(Config file %s requires specification via "%s" or "%s".)", nuxmvConfigFile->c_str(), PLAJA_OPTION::nuxmv_config.c_str(), PLAJA_OPTION::nuxmv_default_engine.c_str()));
        } else if (has_nuxmv_config and has_nuxmv_default_config) {
            throw ConstructorException(PLAJA_UTILS::string_f("Specify config either via \"%s\" or \"%s\".", PLAJA_OPTION::nuxmv_config.c_str(), PLAJA_OPTION::nuxmv_default_engine.c_str()));
        }

        std::stringstream ss;
        ss << "read_model -i " << modelFile;

        if (has_nuxmv_config) {
            const auto splits = PLAJA_UTILS::split(config.get_value_option_string(PLAJA_OPTION::nuxmv_config), "--");
            for (const auto& split: splits) { ss << PLAJA_UTILS::lineBreakString + split; }
        } else {
            PLAJA_ASSERT(has_nuxmv_default_config)
            ss << PLAJA_UTILS::lineBreakString << "go_msat";
            ss << PLAJA_UTILS::lineBreakString << "set bmc_length " << config.get_int_option(PLAJA_OPTION::nuxmv_bmc_len);
            ss << PLAJA_UTILS::lineBreakString << "echo $bmc_length";
            ss << generate_default_config(config);
            ss << PLAJA_UTILS::lineBreakString << "echo $bmc_length";
            ss << PLAJA_UTILS::lineBreakString << "quit";
        }

        nuxmvConfig = std::make_unique<std::string>(ss.str());

    }

}

ToNuxmv::~ToNuxmv() = default;

/**********************************************************************************************************************/

SearchEngine::SearchStatus ToNuxmv::initialize() {

    /* Generate nuxmv model. */

    if (not useExModel) {

        PLAJA_LOG_IF(PLAJA_UTILS::file_exists(modelFile), PLAJA_UTILS::to_red_string(PLAJA_UTILS::string_f("Overriding existing nuXmv model \"%s\".", modelFile.c_str())))

        std::stringstream ss;

        if (backwardsPolModule) { ss << policy_module_to_nuxmv() << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::lineBreakString; }

        ss << "MODULE main" << PLAJA_UTILS::lineBreakString;

        ss << vars_to_nuxmv();
        ss << PLAJA_UTILS::lineBreakString;

        if (invarForReals) {
            auto real_var_bounds = real_bounds_to_nuxmv(false);
            if (real_var_bounds) {
                ss << "INVAR";
                ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << *real_var_bounds;
                ss << PLAJA_UTILS::lineBreakString;
            }
        }

        if (PLAJA_GLOBAL::enableTerminalStateSupport and propertyInfo->get_terminal()) {
            ss << "INVAR";
            ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << ast_to_nuxmv(*propertyInfo->get_non_terminal());
            ss << PLAJA_UTILS::lineBreakString;
        }

        ss << start_to_nuxmv();
        ss << PLAJA_UTILS::lineBreakString;

        if (defTrStructs and not backwardsDefOrder) { ss << trans_struct_def_to_nuxmv() << PLAJA_UTILS::lineBreakString; }

        if (propertyInfo->has_nn_interface()) { ss << nn_to_nuxmv() << PLAJA_UTILS::lineBreakString; }

        if (propertyInfo->has_ensemble_interface()) { ss << ensemble_to_nuxmv() << PLAJA_UTILS::lineBreakString; }

        if (defTrStructs and backwardsDefOrder) { ss << trans_struct_def_to_nuxmv() << PLAJA_UTILS::lineBreakString; }

        ss << trans_to_nuxmv();

        ss << PLAJA_UTILS::lineBreakString;

        ss << reach_to_nuxmv();
        ss << PLAJA_UTILS::lineBreakString;

        PLAJA_UTILS::write_to_file(modelFile, ss.str(), true);

    } else {
        PLAJA_LOG(PLAJA_UTILS::string_f("Reusing existing nuXmv model \"%s\".", modelFile.c_str()))
    }

    /* Generate config file. */
    if (nuxmvConfigFile) { PLAJA_UTILS::write_to_file(*nuxmvConfigFile, *nuxmvConfig, true); }

    return SearchEngine::IN_PROGRESS;
}

SearchEngine::SearchStatus ToNuxmv::step() {
    if (not nuxmvEx) { return SearchEngine::SOLVED; }
    PLAJA_LOG("Executing NuXmv: ")
    PLAJA_LOG("------- (PlaJA end) -------")
    if (nuxmvConfig) { std::system((*nuxmvEx + PLAJA_UTILS::spaceString + "-source " + *nuxmvConfigFile).c_str()); }
    else { std::system((*nuxmvEx + PLAJA_UTILS::spaceString + modelFile).c_str()); }
    PLAJA_LOG("------- (PlaJA begin) -------")
    return SearchEngine::SOLVED;
}

/**********************************************************************************************************************/

std::string ToNuxmv::generate_default_config(const PLAJA::Configuration& config) const {

    const auto& additional_config = config.has_value_option(PLAJA_OPTION::nuxmv_engine_config) ? config.get_value_option_string(PLAJA_OPTION::nuxmv_engine_config) : PLAJA_UTILS::emptyString;

    switch (config.get_enum_option(PLAJA_OPTION::nuxmv_default_engine).get_value()) {

        case PLAJA_OPTION::EnumOptionValue::Bmc: {
            return PLAJA_UTILS::lineBreakString + "msat_check_ltlspec_bmc -P \"safety\" " + additional_config;
            // Options: -k for bmc_length, "msat_check_invar_bmc"?
        }

        case PLAJA_OPTION::EnumOptionValue::SBmc: {
            return PLAJA_UTILS::lineBreakString + "msat_check_ltlspec_sbmc_inc -P \"safety\" " + additional_config;
            // Options: -c(omplete) -- used ---
            // -N en/disable virtual unrolling, -k for bmc_length.
        }

        case PLAJA_OPTION::EnumOptionValue::IpaBmc: {
            return PLAJA_UTILS::lineBreakString + "msat_check_invar_bmc_implabs -P \"invarsafety\" " + additional_config;
            // Options: -a enable/disable abstraction *used as set*, -k for bmc_length.

        }

        case PLAJA_OPTION::EnumOptionValue::IpaCegar: {
            return PLAJA_UTILS::lineBreakString + "msat_check_invar_bmc_cegar_implabs -P \"invarsafety\" " + additional_config;
            // Options: -i k-induction-method: f(ull), b(ackward) only, n(one) -- used --
            // -m(inimize) added predicates, -c: fresh restart after refinement, -k for bmc_length.
        }

        case PLAJA_OPTION::EnumOptionValue::Ic3: {
            return PLAJA_UTILS::lineBreakString + "check_ltlspec_ic3 -P \"safety\" " + additional_config;
            // Options:
            // -a enable (1)/disable (0) abstraction *used*,
            // -K "k-liveness" (1) or "liveness-to-safety" (0) *used*,
            // -k for bmc_length,
            // -l k-liveness bound,
            // -L disable complementation of k-liveness with BMC
            // -u num: property unrolling/target enlargement, default 4.
            // "check_invar_ic3"?
            //
            // check_ltlspec_ic3 -i -a 1 -K 1 -P "safety"
            // check_ltlspec_ic3 -i -a 0 -K 1 -P "safety"
            // check_ltlspec_ic3 -i -a 1 -K 0 -P "safety"
            // check_ltlspec_ic3 -i -a 0 -K 0 -P "safety"
        }

        case PLAJA_OPTION::EnumOptionValue::Cegar: {
            return PLAJA_UTILS::lineBreakString + "build_abstract_model" + PLAJA_UTILS::lineBreakString + "check_invar_cegar_predabs -P \"invarsafety\" " + additional_config;
            // Options: -l bounds number of cycles.
        }

        case PLAJA_OPTION::EnumOptionValue::Coi: {
            return PLAJA_UTILS::lineBreakString + "msat_check_ltlspec_inc_coi -P \"safety\" " + additional_config;
            // Options: -k for bmc_length, "msat_check_invar_inc_coi"?
        }

        default: { PLAJA_ABORT }

    }

    PLAJA_ABORT
}

/**********************************************************************************************************************/

std::string ToNuxmv::vars_to_nuxmv() const {
    const auto& model_info = model->get_model_information();

    std::stringstream ss;
    ss << "VAR";

    for (auto it = model->variableIterator(); !it.end(); ++it) {
        ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString;

        const auto& var_decl = *it.variable();
        const auto var_index = var_decl.get_index();

        switch (var_decl.get_type()->get_kind()) {

            case DeclarationType::Bool: {
                ss << generate_var_def(var_decl.get_name(), "boolean");
                break;
            }

            case DeclarationType::Bounded: {
                if (var_decl.get_type()->is_floating_type()) {
                    ss << generate_var_def(var_decl.get_name(), "real");
                } else if (backwardsBoolVars and model_info.get_range(var_index) == 2) {
                    booleanVars.insert(var_decl.get_id());
                    ss << generate_var_def(var_decl.get_name(), "boolean");
                } else {
                    ss << generate_var_def(var_decl.get_name(), std::to_string(model_info.get_lower_bound_int(var_index)) + ".." + std::to_string(model_info.get_upper_bound_int(var_index)));
                }
                break;
            }

            default: { throw NotImplementedException(__PRETTY_FUNCTION__); };
        }

    }

    if (backwardsPolModule) { ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << generate_var_def(get_policy_module_var(), "argmax" + PLAJA_UTILS::lParenthesisString + get_policy_module_arg_var() + PLAJA_UTILS::rParenthesisString); }

    return ss.str();
}

std::unique_ptr<std::string> ToNuxmv::real_bounds_to_nuxmv(bool parenthesis) const {

    if (model->get_model_information().get_floating_state_size() == 0) { return nullptr; }
    else {

        const auto& model_info = model->get_model_information();

        std::stringstream ss;
        bool conjunct(false);
        for (auto it = model->variableIterator(); !it.end(); ++it) {
            const auto& var_decl = it.variable();
            if (not model_info.is_floating(var_decl->get_index())) { continue; }
            ss << (conjunct ? " & " : PLAJA_UTILS::emptyString) << PLAJA_UTILS::lParenthesisString << model_info.get_lower_bound_float(var_decl->get_index()) << " <= " << var_decl->get_name() << " & " << var_decl->get_name() << " <= " << model_info.get_upper_bound_float(var_decl->get_index()) << PLAJA_UTILS::rParenthesisString;
            conjunct = true;
        }

        return std::make_unique<std::string>(parenthesis ? PLAJA_UTILS::lParenthesisString + ss.str() + PLAJA_UTILS::rParenthesisString : ss.str());
    }

}

/* */

std::string ToNuxmv::state_to_nuxmv(const StateBase& state) const {

    PLAJA_ASSERT(model->get_model_information().compute_location_space_size() == 1)

    std::stringstream ss;

    auto it = model->variableIterator();

    /* 1st. */
    {
        const auto& var = it.variable();
        PLAJA_ASSERT(var->is_scalar())
        ss << var->get_name() << " = " << state.get_int(var->get_index());
    }

    /* 2.. */
    for (++it; !it.end(); ++it) {
        const auto& var = it.variable();
        PLAJA_ASSERT(var->is_scalar())
        ss << " & " << var->get_name() << " = " << state.get_int(var->get_index());
    }

    return ss.str();

}

std::string ToNuxmv::start_to_nuxmv() const {
    const auto& model_info = model->get_model_information();

    std::stringstream ss;
    ss << "INIT" << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString;

    if (not propertyInfo->get_start()) { ss << state_to_nuxmv(model->get_model_information().get_initial_values()); }
    else {
        auto real_var_bounds = invarForReals ? std::unique_ptr<std::string>(nullptr) : real_bounds_to_nuxmv(true);
        auto start = ast_to_nuxmv(*propertyInfo->get_start(), false, real_var_bounds.get());
        if (real_var_bounds) { start = start + " & " + *real_var_bounds; }

        if (propertyInfo->get_start()->evaluate_integer(model_info.get_initial_values())) { ss << start; }
        else { ss << PLAJA_UTILS::lParenthesisString << state_to_nuxmv(model->get_model_information().get_initial_values()) << PLAJA_UTILS::lParenthesisString << " | " << PLAJA_UTILS::lParenthesisString << start << PLAJA_UTILS::rParenthesisString; }
    }

    return ss.str();

}

std::string ToNuxmv::reach_to_nuxmv() const {
    std::stringstream ss;

    /* Reach to str. */
    PLAJA_ASSERT(propertyInfo->get_reach())
    const auto reach_str = defReach ? std::string("reach") : PLAJA_UTILS::lParenthesisString + ast_to_nuxmv(propertyInfo->get_reach()) + PLAJA_UTILS::rParenthesisString;

    /* Not-Reach to str. */ // TODO reach is normally state condition, need special case ...
    /* std::unique_ptr<Expression> reach_neg = std::make_unique<UnaryOpExpression>(UnaryOpExpression::NOT);
    PLAJA_UTILS::cast_ptr<UnaryOpExpression>(reach_neg.get())->set_operand(reach->deepCopy_Exp());
    reach_neg->determine_type();
    TO_NORMALFORM::push_down_negation(reach_neg);
    reach_neg->dump(); */

    ss << "LTLSPEC NAME safety := G !" << reach_str << PLAJA_UTILS::lineBreakString;
    ss << "-- LTLSPEC NAME unsafety := F " << reach_str << PLAJA_UTILS::lineBreakString;
    ss << "-- LTLSPEC NAME safetydual := !(F " << reach_str << ")" << PLAJA_UTILS::lineBreakString;
    ss << "INVARSPEC NAME invarsafety := !" << reach_str << "";

    return ss.str();
}

/**********************************************************************************************************************/

std::unique_ptr<std::string> ToNuxmv::label_to_nuxmv(ActionLabel_type label) const {
    std::stringstream ss;

    auto action_ops = successorGenerator->extract_ops_per_label(label);
    if (action_ops.empty()) { return std::unique_ptr<std::string>(nullptr); } // NOLINT(*-return-braced-init-list)

    /* 1st. */

    ss << op_to_nuxmv(*action_ops.front(), not backwardsOpExclusion);
    /* 2nd. */
    for (action_ops.pop_front(); not action_ops.empty(); action_ops.pop_front()) { ss << " | " << op_to_nuxmv(*action_ops.front(), not backwardsOpExclusion); }

    return std::make_unique<std::string>(ss.str());
}

std::string ToNuxmv::op_to_nuxmv(const ActionOp& op, bool parenthesis) const {

    auto guard_str = guard_to_nuxmv(op, defGuards, not defGuards);

    std::stringstream ss;

    if (backwardsOpExclusion) {

        bool disjunct(false);

        for (auto it_upd = op.updateIterator(); !it_upd.end(); ++it_upd) {
            if (disjunct) { ss << " | "; }
            else { disjunct = true; }

            ss << PLAJA_UTILS::lParenthesisString;
            if (guard_str) { ss << PLAJA_UTILS::lParenthesisString << *guard_str << " & "; }
            ss << update_to_nuxmv(op._op_id(), it_upd.update_index(), &it_upd.update(), defUpdates, not defUpdates);
            if (guard_str) { ss << PLAJA_UTILS::rParenthesisString; }
            ss << exclude_other_same_labeled_ops_to_nuxmv(op._action_label(), op._op_id(), it_upd.update_index());
            ss << PLAJA_UTILS::rParenthesisString;
        }

        if (backwardsCopyUpdates and op.size() <= 1) {

            if (disjunct) { ss << " | "; }

            ss << PLAJA_UTILS::lParenthesisString;
            if (guard_str) { ss << PLAJA_UTILS::lParenthesisString << *guard_str << " & "; }
            ss << update_to_nuxmv(op._op_id(), 1, nullptr, defUpdates, not defUpdates);
            if (guard_str) { ss << PLAJA_UTILS::rParenthesisString; }
            ss << exclude_other_same_labeled_ops_to_nuxmv(op._action_label(), op._op_id(), 1);
            ss << PLAJA_UTILS::rParenthesisString;
        }

    } else {

        /* Guard. */
        if (not useGuardPerUpdate and guard_str) { ss << *guard_str; }

        /* Update(s). */
        if (mergeUpdates) {

            PLAJA_ASSERT(not useGuardPerUpdate)
            if (guard_str) { ss << " & "; }
            ss << updates_to_nuxmv(op, defUpdates, false);

        } else {

            auto it_upd = op.updateIterator();
            if (not useGuardPerUpdate and guard_str) { ss << " & " << PLAJA_UTILS::lParenthesisString; }
            /* 1st. */
            PLAJA_ASSERT(op.size() >= 1)
            if (useGuardPerUpdate and guard_str) { ss << PLAJA_UTILS::lParenthesisString << *guard_str << " & "; }
            ss << update_to_nuxmv(op._op_id(), it_upd.update_index(), &it_upd.update(), defUpdates, not defUpdates);
            if (useGuardPerUpdate and guard_str) { ss << PLAJA_UTILS::rParenthesisString; }
            /* 2.. */
            for (++it_upd; !it_upd.end(); ++it_upd) {
                ss << " | ";
                if (useGuardPerUpdate and guard_str) { ss << PLAJA_UTILS::lParenthesisString << *guard_str << " & "; }
                ss << update_to_nuxmv(op._op_id(), it_upd.update_index(), &it_upd.update(), defUpdates, not defUpdates);
                if (useGuardPerUpdate and guard_str) { ss << PLAJA_UTILS::rParenthesisString; }
            }
            /* Copy. */
            if (backwardsCopyUpdates and op.size() <= 1) {
                ss << " | ";
                if (useGuardPerUpdate and guard_str) { ss << PLAJA_UTILS::lParenthesisString << *guard_str << " & "; }
                ss << update_to_nuxmv(op._op_id(), 1, nullptr, defUpdates, not defUpdates);
                if (useGuardPerUpdate and guard_str) { ss << PLAJA_UTILS::rParenthesisString; }
            }

            if (not useGuardPerUpdate and guard_str) { ss << PLAJA_UTILS::rParenthesisString; }

        }

    }

    return parenthesis ? "(" + ss.str() + ")" : ss.str();

}

std::unique_ptr<std::string> ToNuxmv::guard_to_nuxmv(const ActionOp& op, bool use_define, bool parenthesis) const {

    auto guard = op.guard_to_conjunction(); // TODO, would be more efficient to cache "defined" guards, but time is not really a critical resource here.

    if (guard) {

        if (use_define) {
            PLAJA_ASSERT(defGuards)
            PLAJA_ASSERT(not parenthesis)
            return std::make_unique<std::string>(guard_to_str(op._op_id()));
        } else {
            return std::make_unique<std::string>(parenthesis ? PLAJA_UTILS::lParenthesisString + ast_to_nuxmv(*guard) + PLAJA_UTILS::rParenthesisString : ast_to_nuxmv(*guard));
        }

    }

    return nullptr;
}

std::string ToNuxmv::update_to_nuxmv(ActionOpID_type op_id, UpdateIndex_type upd_index, const Update* update, bool use_define, bool parenthesis, const std::unordered_set<VariableID_type>* ignored_vars) const {

    if (use_define) {
        PLAJA_ASSERT(defUpdates)
        PLAJA_ASSERT(not parenthesis)
        return update_to_str(op_id, upd_index);
    }

    std::stringstream ss;

    std::unordered_set<VariableID_type> updated_vars;
    if (update) {

        PLAJA_ASSERT(update->get_num_sequences() == 1)
        auto it = update->assignmentIterator(0);

        /* 1st */
        ss << assignment_to_nuxmv(*it, false);
        /* 2.. */
        for (++it; !it.end(); ++it) { ss << " & " << assignment_to_nuxmv(*it, false); }

        updated_vars = update->collect_updated_vars(0);

    }

    /* Copy constraints. */
    bool conjunct(not updated_vars.empty());
    for (auto it_var = model->variableIterator(); !it_var.end(); ++it_var) {
        PLAJA_ASSERT(not it_var.variable()->is_array())
        const auto var_id = it_var.variable_id();
        if (updated_vars.count(var_id) or (ignored_vars and ignored_vars->count(var_id))) { continue; }
        ss << (conjunct ? " & " : PLAJA_UTILS::emptyString) << generate_next(it_var.variable()->get_name(), it_var.variable()->get_name());
        conjunct = true;
    }

    return parenthesis ? PLAJA_UTILS::lParenthesisString + ss.str() + PLAJA_UTILS::rParenthesisString : ss.str();
}

std::string ToNuxmv::assignment_to_nuxmv(const Assignment& assignment, bool parenthesis) const {
    std::stringstream ss;

    if (assignment.is_deterministic()) { ss << generate_next(assignment.get_ref(), assignment.get_value()); }
    else {
        PLAJA_ASSERT(not backwardsBoolVars)
        const auto& model_info = model->get_model_information();
        const auto index = assignment.get_ref()->get_variable_index();

        const auto ref_str = "next" + PLAJA_UTILS::lParenthesisString + ast_to_nuxmv(assignment.get_ref(), false) + PLAJA_UTILS::rParenthesisString;

        const auto lower_bound_str = model_info.is_floating(index) ? std::to_string(model_info.get_lower_bound_float(index)) : std::to_string(model_info.get_lower_bound_int(index));
        const auto upper_bound_str = model_info.is_floating(index) ? std::to_string(model_info.get_upper_bound_float(index)) : std::to_string(model_info.get_upper_bound_int(index));

        // Always add variable bounds.
        if (not invarForReals) { ss << PLAJA_UTILS::lParenthesisString << lower_bound_str << " <= " << ref_str << " & " << ref_str << " <= " << upper_bound_str << PLAJA_UTILS::rParenthesisString << " & "; }
        // Actual assignment.
        ss << PLAJA_UTILS::lParenthesisString << ast_to_nuxmv(assignment.get_lower_bound(), false) << " <= " << ref_str << " & " << ref_str << " <= " << ast_to_nuxmv(assignment.get_upper_bound(), false) << PLAJA_UTILS::rParenthesisString;
    }

    return parenthesis ? "(" + ss.str() + ")" : ss.str();
}

std::string ToNuxmv::updates_to_nuxmv(const ActionOp& op, bool use_define, bool parenthesis) const {

    if (use_define) {
        PLAJA_ASSERT(defUpdates)
        PLAJA_ASSERT(not parenthesis)
        return update_to_str(op._op_id(), ACTION::noneUpdate);
    }

    const auto& non_updated_vars = op.infer_non_updates_vars();

    std::stringstream ss;

    if (not non_updated_vars.empty()) { ss << PLAJA_UTILS::lParenthesisString; }

    auto it_upd = op.updateIterator();
    /* 1st. */
    PLAJA_ASSERT(op.size() >= 1)
    ss << update_to_nuxmv(op._op_id(), it_upd.update_index(), &it_upd.update(), false, true, &non_updated_vars);
    /* 2.. */
    for (++it_upd; !it_upd.end(); ++it_upd) {
        ss << " | " << update_to_nuxmv(op._op_id(), it_upd.update_index(), &it_upd.update(), false, true, &non_updated_vars);
    }

    /* Global copies. */
    if (not non_updated_vars.empty()) {
        ss << PLAJA_UTILS::rParenthesisString << " & " << PLAJA_UTILS::lParenthesisString;
        bool conjunct(false);
        for (auto it_var = model->variableIterator(); !it_var.end(); ++it_var) {
            PLAJA_ASSERT(not it_var.variable()->is_array())
            const auto var_id = it_var.variable_id();
            if (not non_updated_vars.count(var_id)) { continue; } // Do not iterate set directly to preserve order for readability.
            ss << (conjunct ? " & " : PLAJA_UTILS::emptyString) << generate_next(it_var.variable()->get_name(), it_var.variable()->get_name());
            conjunct = true;
        }
        ss << PLAJA_UTILS::rParenthesisString;
    }

    return parenthesis ? "(" + ss.str() + ")" : ss.str();
}

std::string ToNuxmv::generate_next(const LValueExpression* ref, const Expression* value) const {

    if (backwardsBoolVars) {
        if (booleanVars.count(ref->get_variable_id())) {
            PLAJA_ASSERT(value->is_constant())
            return generate_next(ast_to_nuxmv(ref, false), value->evaluate_integer_const() ? "TRUE" : "FALSE");
        } else { return generate_next(ast_to_nuxmv(ref, false), ast_to_nuxmv(value)); }
    }

    return generate_next(ast_to_nuxmv(ref), ast_to_nuxmv(value));
}

std::string ToNuxmv::trans_struct_def_to_nuxmv() const {
    std::stringstream ss;
    ss << "DEFINE";

    if (backwardsDefGoal and propertyInfo->get_learning_objective() and propertyInfo->get_learning_objective()->get_goal()) {
        auto goal = propertyInfo->get_learning_objective()->get_goal()->deepCopy_Exp();
        TO_NORMALFORM::to_nary(goal); // This is an unstructured junction.
        ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << generate_def("goal", ast_to_nuxmv(*goal));
    }

    if (defReach) {
        PLAJA_ASSERT(propertyInfo->get_reach())
        ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << generate_def("reach", ast_to_nuxmv(propertyInfo->get_reach()));
    }

    if (defGuards) {
        for (auto it = TO_NUXMV::init_all_op_it(*model, *successorGenerator, backwardsDefOrder); !it->end(); it->operator++()) {
            const auto& op = **it;
            auto guard = guard_to_nuxmv(op, false, false);
            if (not guard) { continue; }
            ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << generate_def(guard_to_str(op._op_id()), *guard);
        }
    }

    if (defUpdates) {
        /* Updates. */
        for (auto it = TO_NUXMV::init_all_op_it(*model, *successorGenerator, backwardsDefOrder); !it->end(); it->operator++()) {
            const auto& op = **it;

            if (mergeUpdates) {
                ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << generate_def(update_to_str(op._op_id(), ACTION::noneUpdate), updates_to_nuxmv(op, false, false));
                continue;
            }

            for (auto it_upd = op.updateIterator(); !it_upd.end(); ++it_upd) {
                ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << generate_def(update_to_str(op._op_id(), it_upd.update_index()), update_to_nuxmv(ACTION::noneOp, ACTION::noneUpdate, &it_upd.update(), false, false));
            }

            /* Special backwards case: Add copy for deterministic operators. */
            if (backwardsCopyUpdates and op.size() <= 1) {
                ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << generate_def(update_to_str(op._op_id(), 1), update_to_nuxmv(ACTION::noneOp, ACTION::noneUpdate, nullptr, false, false));
            }

        }
    }

    return ss.str();
}

/* DO NOT USE. */
std::string ToNuxmv::exclude_other_same_labeled_ops_to_nuxmv(ActionLabel_type label, ActionOpID_type op_id, UpdateIndex_type upd_index) const {
    PLAJA_ASSERT(backwardsOpExclusion)
    PLAJA_ASSERT(defGuards and defUpdates)
    PLAJA_ASSERT(not mergeUpdates)

    std::stringstream ss;

    for (auto it = successorGenerator->init_action_it_static(label); !it.end(); ++it) {
        for (auto it_upd = it->updateIterator(); !it_upd.end(); ++it_upd) {
            if (it->_op_id() == op_id and it_upd.update_index() == upd_index) { continue; }
            ss << " & !(" << guard_to_str(it->_op_id()) << " & " << update_to_str(it->_op_id(), it_upd.update_index()) << PLAJA_UTILS::rParenthesisString;
        }

        if (backwardsCopyUpdates and it->size() <= 1 and (it->_op_id() != op_id or upd_index != 1)) { ss << " & !(" << guard_to_str(it->_op_id()) << " & " << update_to_str(it->_op_id(), 1) << PLAJA_UTILS::rParenthesisString; }

    }

    return ss.str();
}

std::string ToNuxmv::guard_to_str(ActionOpID_type op_id) { return "guard_" + std::to_string(op_id); }

std::string ToNuxmv::update_to_str(ActionOpID_type op_id, UpdateIndex_type upd_index) { return "update_" + (upd_index == ACTION::noneUpdate ? std::to_string(op_id) : std::to_string(op_id) + "." + std::to_string(upd_index)); }

/**********************************************************************************************************************/

std::string ToNuxmv::trans_to_nuxmv() const {
    std::stringstream ss;
    ss << "TRANS";

    if (propertyInfo->get_interface()) {

        const auto& policy_interface = *propertyInfo->get_interface();

        std::list<std::unique_ptr<std::string>> unlearned_labels;
        for (const auto unlearned_label: policy_interface.get_unlearned_labels()) {
            unlearned_labels.push_back(label_to_nuxmv(unlearned_label));
            if (not unlearned_labels.back()) { unlearned_labels.pop_back(); }
        }

        if (useCase and unlearned_labels.empty()) {

            ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString;

            std::list<std::pair<std::string, std::string>> cases;
            for (auto it = policy_interface.init_output_feature_iterator(); !it.end(); ++it) {
                auto label_str = label_to_nuxmv(it.get_output_label());
                if (not label_str) { continue; }
                cases.emplace_back(backwardsPolModule ? selection_constraint_policy_module(it._output_index()) : selection_constraint_to_nuxmv(it._output_index(), nullptr, true), *label_str);
            }
            if (not backwardsPolSel) { cases.reverse(); }
            cases.emplace_back("TRUE", "FALSE");

            ss << generate_case(cases, 1);

        } else {

            bool disjunct(false);
            for (auto it = policy_interface.init_output_feature_iterator(); !it.end(); ++it) {
                auto label_str = label_to_nuxmv(it.get_output_label());
                if (not label_str) { continue; }
                ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << (disjunct ? " | " : PLAJA_UTILS::emptyString) << PLAJA_UTILS::lParenthesisString << PLAJA_UTILS::lParenthesisString << selection_constraint_to_nuxmv(it._output_index(), nullptr, false) << PLAJA_UTILS::rParenthesisString << " & " << PLAJA_UTILS::lParenthesisString << *label_str << PLAJA_UTILS::rParenthesisString << PLAJA_UTILS::rParenthesisString;
                disjunct = true;
            }

            for (const auto& unlearned_label: unlearned_labels) {
                ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << (disjunct ? " | " : PLAJA_UTILS::emptyString) << PLAJA_UTILS::lParenthesisString << *unlearned_label << PLAJA_UTILS::rParenthesisString;
                disjunct = true;
            }

        }

    } else {

        static_assert(ACTION::silentLabel == -1);
        bool disjunct(false);
        for (ActionLabel_type action_label = ACTION::silentLabel; action_label < PLAJA_UTILS::cast_numeric<ActionLabel_type>(model->get_number_actions()); ++action_label) {
            auto label_str = label_to_nuxmv(action_label);
            if (not label_str) { continue; }
            ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << (disjunct ? " | " : PLAJA_UTILS::emptyString) << PLAJA_UTILS::lParenthesisString << *label_str << PLAJA_UTILS::rParenthesisString;
            disjunct = true;
        }

    }

    return ss.str();
}

/**********************************************************************************************************************/

std::string ToNuxmv::nn_to_nuxmv() const {

    PLAJA_ASSERT(propertyInfo->get_nn_interface())

    const auto& policy_interface = *propertyInfo->get_nn_interface();
    const auto& nn = policy_interface.load_network();

    std::stringstream ss;

    ss << "DEFINE";

    if (backwardsPolModule) {
        for (auto it = policy_interface.init_output_feature_iterator(); !it.end(); ++it) {
            ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << generate_def(generate_aa(get_policy_module_arg_var(), it._output_index()), neuron_to_str(policy_interface.get_num_layers() - 1, it._output_index()));
        }
    }

    for (LayerIndex_type target_layer = 1; target_layer < policy_interface.get_num_layers(); ++target_layer) {
        const auto sry_layer = target_layer - 1;

        for (NeuronIndex_type target_neuron = 0; target_neuron < nn.getLayerSize(target_layer); ++target_neuron) {

            ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << neuron_to_str(target_layer, target_neuron) << " := ";

            if (target_layer < nn.getNumLayers()) { ss << "max("; } // Not for output layer since nn.getNumLayers does not count input layer.

            for (NeuronIndex_type src_neuron = 0; src_neuron < nn.getLayerSize(sry_layer); ++src_neuron) {
                ss << PLAJA_UTILS::to_string_with_precision(nn.getWeight(sry_layer, src_neuron, target_neuron), USING_NUXMV::float_precision, not backwardsNnPrecision) << " * " << neuron_to_str(sry_layer, src_neuron) << " + ";
            }

            ss << PLAJA_UTILS::to_string_with_precision(nn.getBias(target_layer, target_neuron), USING_NUXMV::float_precision, not backwardsNnPrecision);

            if (target_layer < nn.getNumLayers()) { ss << ", 0)"; } // Not for output layer.

            ss << PLAJA_UTILS::semicolonString;

        }

    }

    if (policy_interface._do_applicability_filtering()) {
        PLAJA_ASSERT(defGuards)

        // Define label applicability:
        for (auto it = policy_interface.init_output_feature_iterator(); !it.end(); ++it) {

            std::stringstream guards_disjunctions;
            bool added_guard(false);

            for (auto it_ops = successorGenerator->init_label_action_it_static(it.get_output_label()); !it_ops.end(); ++it_ops) {
                auto guard = it_ops->guard_to_conjunction();
                if (guard) {
                    guards_disjunctions << (added_guard ? " | " : PLAJA_UTILS::emptyString) << guard_to_str(it_ops->_op_id());
                    added_guard = true;
                }
            }

            ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << generate_def(label_app_to_str(it.get_output_label()), added_guard ? guards_disjunctions.str() : "FALSE");
        }

    }

    return ss.str();

}
void get_splits(std::vector<std::pair<std::vector<std::pair<veritas::LtSplit, bool>>, veritas::FloatT>>& splits, std::pair<std::vector<std::pair<veritas::LtSplit, bool>>, veritas::FloatT> split, veritas::Tree tree, veritas::NodeId id, int depth, int label) {
    int i = 1;
    for (; i < depth; ++i)
        split.first.shrink_to_fit();
    if (tree.is_leaf(id)) {
        split.second = tree.leaf_value(id, label);
        splits.push_back(split);
    } else {
        auto split_left = split;
        auto split_right = split;
        split_left.first.push_back({tree.get_split(id), true});
        split_right.first.push_back({tree.get_split(id), false});
        get_splits(splits, split_left, tree, tree.left(id), depth+1, label);
        get_splits(splits, split_right, tree, tree.right(id), depth+1, label);
    }
}

std::vector<std::pair<std::vector<std::pair<veritas::LtSplit, bool>>, veritas::FloatT>> get_splits_for_tree(veritas::Tree tree, int label) {
    std::vector<std::pair<std::vector<std::pair<veritas::LtSplit, bool>>, veritas::FloatT>> splits;
    splits.reserve(tree.num_leaves());
    std::vector<std::pair<veritas::LtSplit, bool>> aux(0);
    std::pair<std::vector<std::pair<veritas::LtSplit, bool>>, veritas::FloatT> split = {aux, 0};
    get_splits(splits, split, tree, tree.root(), tree.depth(tree.root()), label);
    return splits;
}

std::string ToNuxmv::tree_to_nuxmv(veritas::Tree tree, int tree_index) const {
    std::string tree_string_default = "tree_" + std::to_string(tree_index) + "_";
    std::stringstream ss;
    for (int i = 0; i < tree.num_leaf_values(); ++i) {
        auto tree_string = tree_string_default + std::to_string(i);
        ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << tree_string << " := ";
        auto paths = get_splits_for_tree(tree, i);
        for (int p = 0; p < paths.size() - 1; ++p) {
            auto path = paths[p];
            auto splits = path.first;
            auto leaf_value = path.second;
            for (int j = 0; j < splits.size(); ++j) {
                auto split = splits[j];
                auto feat_id = split.first.feat_id;
                auto split_var = split.first.split_value;
                ss << (split.second ? "(": "!(") << neuron_to_str(0, feat_id) << " < " << std::to_string(split_var) << ")" << (j == splits.size() - 1 ? " ? " : " & ");
            }
            ss << std::to_string(leaf_value) << " : " << (p == paths.size() - 2 ? std::to_string(paths[p+1].second) : "");
        }
        ss << PLAJA_UTILS::semicolonString;
    }
    return ss.str();
}


std::string ToNuxmv::ensemble_to_nuxmv() const {

    PLAJA_ASSERT(propertyInfo->get_ensemble_interface())

    const auto& policy_interface = *propertyInfo->get_ensemble_interface();

    std::stringstream ss;

    ss << "DEFINE";

    if (backwardsPolModule) {
        for (auto it = policy_interface.init_output_feature_iterator(); !it.end(); ++it) {
            ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << generate_def(generate_aa(get_policy_module_arg_var(), it._output_index()), neuron_to_str(1, it._output_index()));
        }
    }

 // Propagation
    auto policy = policy_interface.load_ensemble();
    std::string tree_string_default = "tree_";
    for (int i = 0; i < policy.size(); ++i) {
        auto tree = policy[i];
        std::cout << tree << std::endl;
        ss << tree_to_nuxmv(tree, i);
        std::cout << ss.str() << std::endl;
        PLAJA_ASSERT(false)
    }
    for (int i = 0; i < policy.num_leaf_values(); ++i) {
        ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << neuron_to_str(1, i) << ":= ";
        for (int tree = 0; tree < policy.size(); ++tree) {
            std::string tree_string = tree_string_default + std::to_string(tree) + "_" + std::to_string(i);
            ss << tree_string << (tree == policy.size() - 1 ? "" : " + ");
        }
        ss << PLAJA_UTILS::semicolonString;
    }

    if (policy_interface._do_applicability_filtering()) {
        PLAJA_ASSERT(defGuards)

        // Define label applicability:
        for (auto it = policy_interface.init_output_feature_iterator(); !it.end(); ++it) {

            std::stringstream guards_disjunctions;
            bool added_guard(false);

            for (auto it_ops = successorGenerator->init_label_action_it_static(it.get_output_label()); !it_ops.end(); ++it_ops) {
                auto guard = it_ops->guard_to_conjunction();
                if (guard) {
                    guards_disjunctions << (added_guard ? " | " : PLAJA_UTILS::emptyString) << guard_to_str(it_ops->_op_id());
                    added_guard = true;
                }
            }

            ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << generate_def(label_app_to_str(it.get_output_label()), added_guard ? guards_disjunctions.str() : "FALSE");
        }

    }

    return ss.str();

}

std::string ToNuxmv::selection_constraint_to_nuxmv(OutputIndex_type output, const std::string* array_var, bool exploit_case) const {
    PLAJA_ASSERT(propertyInfo->get_interface())

    const auto& policy_interface = *propertyInfo->get_interface();
    const LayerIndex_type output_layer = propertyInfo->has_nn_interface() ? propertyInfo->get_nn_interface()->get_num_layers() - 1 : 1;

    const bool do_app = policy_interface._do_applicability_filtering();

    if (exploit_case) {
        if (backwardsPolSel) { if (output + 1 == policy_interface.get_num_output_features()) { return "TRUE"; } }
        else { if (output == 0) { return "TRUE"; } }
    }

    std::stringstream ss;

    bool conjunct(false);
    for (OutputIndex_type output_other = 0; output_other < policy_interface.get_num_output_features(); ++output_other) {
        if (output_other == output) { continue; }
        if (exploit_case) {
            if (backwardsPolSel) { if (output_other < output) { continue; } }
            else { if (output_other > output) { continue; } }
        }

        if (conjunct) { ss << " & "; }
        else { conjunct = true; }

        ss << (do_app ? PLAJA_UTILS::lParenthesisString : PLAJA_UTILS::emptyString)
           << (array_var ? generate_aa(*array_var, output) : neuron_to_str(output_layer, output))
           << (output_other < output or backwardsPolSel ? " > " : " >= ")
           << (array_var ? generate_aa(*array_var, output_other) : neuron_to_str(output_layer, output_other))
           << (do_app ? (" | !" + label_app_to_str(policy_interface.get_output_label(output_other)) + PLAJA_UTILS::rParenthesisString) : PLAJA_UTILS::emptyString);
    }

    return ss.str();
}

std::string ToNuxmv::policy_module_to_nuxmv() const {

    PLAJA_ASSERT(propertyInfo->get_interface())
    const auto& policy_interface = *propertyInfo->get_interface();

    static const std::string action_var_arg("arr");

    std::stringstream ss;

    ss << "MODULE argmax(" << action_var_arg << ")";

    ss << PLAJA_UTILS::lineBreakString << "VAR";
    ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << generate_var_def(get_policy_module_int_var(), "0.." + std::to_string(policy_interface.get_num_output_features() - 1));
    if (backwardsPolModuleStr) {
        ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << get_policy_module_str_var() << " : {";
        {
            bool conjunct(false);
            for (auto it = policy_interface.init_output_feature_iterator(); !it.end(); ++it) {
                if (conjunct) { ss << PLAJA_UTILS::spaceString << PLAJA_UTILS::commaString << PLAJA_UTILS::spaceString; }
                else { conjunct = true; }
                ss << model->get_action_name(ModelInformation::action_label_to_id(it.get_output_label()));
            }
        }
        ss << "};";
    }

    {
        ss << PLAJA_UTILS::lineBreakString;
        ss << "INVAR";
        ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << get_policy_module_int_var() << " = ";
        std::list<std::pair<std::string, std::string>> cases;
        for (auto it = policy_interface.init_output_feature_iterator(); !it.end(); ++it) {
            cases.emplace_back(selection_constraint_to_nuxmv(it._output_index(), &action_var_arg, true), std::to_string(it._output_index()));
        }
        if (not backwardsPolSel) { cases.emplace_back("TRUE", "0"); }
        ss << generate_case(cases, 2);
        ss << PLAJA_UTILS::semicolonString;
    }

    if (backwardsPolModuleStr) {
        ss << PLAJA_UTILS::lineBreakString;
        ss << "INVAR";
        ss << PLAJA_UTILS::lineBreakString << PLAJA_UTILS::tabString << get_policy_module_str_var() << " = ";
        std::list<std::pair<std::string, std::string>> cases;
        for (auto it = policy_interface.init_output_feature_iterator(); !it.end(); ++it) {
            cases.emplace_back(get_policy_module_int_var() + " = " + std::to_string(it._output_index()), model->get_action_name(ModelInformation::action_label_to_id(it.get_output_label())));
        }
        cases.emplace_back("TRUE", model->get_action_name(policy_interface.get_output_label(0))); // TODO remove if possible
        ss << generate_case(cases, 2);
        ss << PLAJA_UTILS::semicolonString;
    }

    return ss.str();
}

std::string ToNuxmv::selection_constraint_policy_module(OutputIndex_type output) const {
    return get_policy_module_var() + PLAJA_UTILS::dotString + get_policy_module_int_var() + " = " + std::to_string(output);
}

/* */

std::string ToNuxmv::neuron_to_str(LayerIndex_type layer, NeuronIndex_type neuron) const {
    /* Input layer. */
    if (layer == 0) {
        PLAJA_ASSERT(propertyInfo->get_interface())
        const auto& policy_interface = *propertyInfo->get_interface();
        PLAJA_ASSERT(policy_interface.get_input_feature(neuron)->is_value_variable())
        return ast_to_nuxmv(policy_interface.get_input_feature(neuron)->get_expression(), true);
    }

    return "layer_" + std::to_string(layer) + "_neuron_" + std::to_string(neuron);
}

std::string ToNuxmv::label_app_to_str(ActionLabel_type label) const { return "label_" + std::to_string(label) + "_is_app"; }

const std::string& ToNuxmv::get_policy_module_var() {
    static const std::string var("action");
    return var;
}

const std::string& ToNuxmv::get_policy_module_arg_var() {
    static const std::string var("a");
    return var;
}

const std::string& ToNuxmv::get_policy_module_int_var() {
    static const std::string var("action_label");
    return var;
}

const std::string& ToNuxmv::get_policy_module_str_var() {
    static const std::string var("action_name");
    return var;
}

/**********************************************************************************************************************/

std::string ToNuxmv::ast_to_nuxmv(const class AstElement& ast_element, bool numeric_env) const { return *ToNuxmvVisitor::to_nuxmv(ast_element, numeric_env, *this); }

std::string ToNuxmv::ast_to_nuxmv(const AstElement& ast_element, bool numeric_env, bool parenthesis) const { return parenthesis ? PLAJA_UTILS::lParenthesisString + ast_to_nuxmv(ast_element, numeric_env) + PLAJA_UTILS::rParenthesisString : ast_to_nuxmv(ast_element, numeric_env); }

std::string ToNuxmv::generate_var_def(const std::string& name, const std::string& domain) {
    return name + PLAJA_UTILS::spaceString + PLAJA_UTILS::colonString + PLAJA_UTILS::spaceString + domain + PLAJA_UTILS::semicolonString;
}

std::string ToNuxmv::generate_def(const std::string& name, const std::string& def) { return name + " := " + def + ";"; }

std::string ToNuxmv::generate_next(const std::string& var, const std::string& assignment) { return "next(" + var + ") = " + assignment; }

std::string ToNuxmv::generate_to_int(const std::string& expr) { return "toint" + PLAJA_UTILS::lParenthesisString + expr + PLAJA_UTILS::rParenthesisString; }

std::string ToNuxmv::generate_aa(const std::string& var, std::size_t index) { return var + "[" + std::to_string(index) + "]"; }

std::string ToNuxmv::generate_case(const std::list<std::pair<std::string, std::string>>& cases, unsigned int tabs) {
    std::stringstream ss;

    ss << "case";

    for (const auto& [condition, value]: cases) {
        ss << PLAJA_UTILS::lineBreakString;
        for (std::size_t index = 0; index < tabs; ++index) { ss << PLAJA_UTILS::tabString; }
        ss << condition << PLAJA_UTILS::spaceString << PLAJA_UTILS::colonString << PLAJA_UTILS::spaceString << value << PLAJA_UTILS::semicolonString;
    }

    ss << PLAJA_UTILS::lineBreakString;
    for (std::size_t index = 0; index < tabs; ++index) { ss << PLAJA_UTILS::tabString; }
    ss << "esac";

    return ss.str();
}