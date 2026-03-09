//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <memory>
#include <unordered_set>
#include <json.hpp>
#include "jani_2_nnet_parser.h"
#include "../../../exception/parser_exception.h"
#include "../../../include/ct_config_const.h"
#include "../../../option_parser/option_parser.h"
#include "../../../option_parser/plaja_options.h"
#include "../../../parser/ast/iterators/model_iterator.h"
#include "../../../parser/ast/action.h"
#include "../../../parser/ast/automaton.h"
#include "../../../parser/ast/composition.h"
#include "../../../parser/ast/edge.h"
#include "../../../parser/ast/variable_declaration.h"
#include "../../../parser/jani_words.h"
#include "../../../parser/parser_utils.h"
#include "../../../utils/utils.h"
#include "../input_feature_to_jani_derived.h"
#include "jani_2_nnet.h"

namespace JANI_2_NNET { const std::string error { "Jani2NNet error" }; } // NOLINT(cert-err58-cpp)

/*********************************************************************************************************************/

Jani2NNetParser::Jani2NNetParser(const Model& model_):
    model(model_)
    , jani2NNetFile()
    , loadedJSON(nullptr) {

}

Jani2NNetParser::~Jani2NNetParser() = default;

std::unique_ptr<Jani2NNet> Jani2NNetParser::load_file(const std::string& nn_interface_file) {
    jani2NNetFile = nn_interface_file;
    loadedJSON = std::make_unique<nlohmann::json>();
    if (not PARSER::parse_json_file(nn_interface_file, *loadedJSON)) { throw ParserException(JANI_2_NNET::error); }

    std::unique_ptr<Jani2NNet> nn_interface(new Jani2NNet(model));
    // interface:
    nn_interface->interfaceFile = nn_interface_file;
    // nnet:
    if (PLAJA_GLOBAL::optionParser->has_value_option(PLAJA_OPTION::nn)) {
        nn_interface->nnFile = PLAJA_GLOBAL::optionParser->get_value_option_string(PLAJA_OPTION::nn);
    } else {
        nn_interface->nnFile = get_nn_file();
    }
    // pytorch:
    if constexpr (PLAJA_GLOBAL::useTorch) {
        if (PLAJA_GLOBAL::optionParser->has_value_option(PLAJA_OPTION::torch_model)) {
            nn_interface->torchFile = PLAJA_GLOBAL::optionParser->get_value_option_string(PLAJA_OPTION::torch_model);
        } else {
            auto torch_file = get_torch_file();
            nn_interface->torchFile = torch_file ? *torch_file : PLAJA_UTILS::extract_directory_path(nn_interface->nnFile) + PLAJA_UTILS::extract_filename(nn_interface->nnFile, false) + ".pt";
        }
    } else { nn_interface->torchFile = PLAJA_UTILS::emptyString; }

    // applicability filtering ?
    const auto app_filter_option = PLAJA_GLOBAL::enableApplicabilityFiltering ? PLAJA_GLOBAL::optionParser->get_int_option(PLAJA_OPTION::applicability_filtering) : 0;
    if (app_filter_option > 0) { nn_interface->doApplicabilityFiltering = true; }
    else if (app_filter_option < 0) { nn_interface->doApplicabilityFiltering = false; }
    else {
        PLAJA_ASSERT(app_filter_option == 0)
        nn_interface->doApplicabilityFiltering = applicability_filtering_enabled();
    }

    // inputs:
    nn_interface->inputFeatureMapping = parse_input_interface();

    // hidden layers:
    nn_interface->hiddenLayerSizes = parse_hidden_layer_structure();

    // outputs:
    nn_interface->outputsToLabels = parse_output_interface();

    // finalize:
    nn_interface->compute_labels_to_outputs();
    nn_interface->compute_layer_sizes();
    nn_interface->compute_extended_inputs();

    return nn_interface;
}

void Jani2NNetParser::dump_nn_interface(const Jani2NNet& nn_interface) {
    PLAJA_ASSERT(not nn_interface.interfaceFile.empty())

    nlohmann::json interface_json;

    // files:
    interface_json.emplace(JANI::FILE, PLAJA_UTILS::extract_filename(nn_interface.get_policy_file(), true));
    interface_json.emplace(JANI::TORCH_FILE, PLAJA_UTILS::extract_filename(nn_interface.get_torch_file(), true));

    // filter ?
    interface_json.emplace(JANI::FILTER, nn_interface._do_applicability_filtering());

    // inputs:
    nlohmann::json inputs;
    for (const auto& input_feature: nn_interface.inputFeatureMapping) { inputs.push_back(input_feature_to_json(*input_feature)); }
    interface_json.emplace(JANI::INPUT, std::move(inputs));

    // hidden layers:
    nlohmann::json hidden_layers;
    for (const auto& hidden_layer_size: nn_interface.get_hidden_layer_sizes()) { hidden_layers.push_back(hidden_layer_size); }
    interface_json.emplace(JANI::ELEMENTS, std::move(hidden_layers));

    // outputs:
    nlohmann::json outputs;
    for (const auto& output_label: nn_interface.outputsToLabels) { outputs.push_back(model.get_action_name(output_label)); }
    interface_json.emplace(JANI::OUTPUT, std::move(outputs));

    PLAJA_UTILS::make_directory(PLAJA_UTILS::extract_directory_path(nn_interface.interfaceFile));
    PLAJA_UTILS::write_to_file(nn_interface.interfaceFile, interface_json.dump(4), true);
}

namespace JANI_2_NNET {

    std::unique_ptr<Jani2NNet> load_nn_interface(const Model& model, const std::string& nn_interface) {
        Jani2NNetParser parser(model);
        return parser.load_file(nn_interface);
    }

    void dump_nn_interface(const Jani2NNet& nn_interface) {
        Jani2NNetParser parser(nn_interface.get_model());
        parser.dump_nn_interface(nn_interface);
    }

}

/**********************************************************************************************************************/

AutomatonIndex_type Jani2NNetParser::parse_automaton_instance(const nlohmann::json& input) const {
    PARSER::check_object(input);
    PARSER::check_for_unexpected_element(input, 2);

    // name
    PARSER::check_contains(input, JANI::NAME);
    const nlohmann::json& name = input.at(JANI::NAME);
    PARSER::check_string(name, input);

    // index
    PARSER::check_contains(input, JANI::INDEX);
    const nlohmann::json& index = input.at(JANI::INDEX);
    PARSER::throw_parser_exception_if(not index.is_number_integer() or index < 0 or model.get_number_automataInstances() <= index, index, input, PARSER::invalid);

    // check name matches index
    PARSER::throw_parser_exception_if(model.get_automatonInstance(index)->get_name() != name, input, (*loadedJSON), PARSER::invalid);

    return index;
}

std::pair<AutomatonIndex_type, EdgeID_type> Jani2NNetParser::parse_edge(const nlohmann::json& input) const {
    PARSER::check_object(input);
    PARSER::check_for_unexpected_element(input, 2);

    // automaton
    PARSER::check_contains(input, JANI::AUTOMATON);
    const AutomatonIndex_type automaton_index = parse_automaton_instance(input.at(JANI::AUTOMATON));
    const Automaton* automaton = model.get_automatonInstance(automaton_index);

    // edge
    PARSER::check_contains(input, JANI::INDEX);
    const nlohmann::json& index = input.at(JANI::INDEX);
    PARSER::throw_parser_exception_if(not index.is_number_unsigned() or index < 0 or automaton->get_number_edges() <= index, index, input, PARSER::invalid);

    return { automaton_index, automaton->get_edge(index)->get_id() };

}

#include "../model_information.h"

ActionOpID_type Jani2NNetParser::parse_action_operator_structure(const nlohmann::json& input) const {

    if (true) { PLAJA_ABORT }

    PARSER::check_object(input);
    PARSER::check_for_unexpected_element(input, 2);
    ActionOpStructure op_str;

    // sync index
    PARSER::check_contains(input, JANI::INDEX);
    const nlohmann::json& index = input.at(JANI::INDEX);
    if (index.is_null()) {
        op_str.syncID = SYNCHRONISATION::nonSyncID;
    } else {
        PARSER::throw_parser_exception_if(not index.is_number_unsigned() or index < 0 or model.get_system()->get_number_syncs() <= index, index, input, PARSER::invalid);
        op_str.syncID = ModelInformation::sync_index_to_id(index);
    }

    // edges
    PARSER::check_contains(input, JANI::EDGES);
    const nlohmann::json& edges = input.at(JANI::EDGES);
    PARSER::check_array(edges, input);
    PARSER::throw_parser_exception_if(edges.empty(), edges, input, PARSER::at_least_one_element);
    for (const auto& edge: edges) {
        op_str.participatingEdges.push_back(parse_edge(edge));
    }

    return model.get_model_information().compute_action_op_id(op_str);
}

/**********************************************************************************************************************/

std::unique_ptr<InputFeatureToJANI> Jani2NNetParser::parse_InputFeatureToJANI(const nlohmann::json& input) const {
    PARSER::check_object(input);
    if (input.contains(JANI::NAME) and input.contains(JANI::AUTOMATON)) {
        if (input.contains(JANI::INDEX)) { // ArrayVar
            return parse_InputFeatureToArrayVariable(input);
        } else { // ValVar
            return parse_InputFeatureToValueVariable(input);
        }
    } else { // Location
        return parse_InputFeatureToLocation(input);
    }
}

std::unique_ptr<InputFeatureToJANI> Jani2NNetParser::parse_InputFeatureToLocation(const nlohmann::json& input) const {
    std::unique_ptr<InputFeatureToLocation> input_feature_to(new InputFeatureToLocation(parse_automaton_instance(input))); // TODO actually problematic, model info should store the mapping invariant
    return input_feature_to;
}

// auxiliary:

#define PARSE_VARIABLE(REF, NAME)\
    const std::size_t l = (REF).get_number_variables();\
    for (std::size_t i = 0; i < l; ++i) {\
        const VariableDeclaration* var_decl = (REF).get_variable(i);\
        if (var_decl->get_name() == (NAME)) {\
            std::unique_ptr<InputFeatureToVariable> input_feature_to(new InputFeatureToVariable(*var_decl));\
            return input_feature_to;\
        }\
    }\
    PARSER::throw_parser_exception(input, (*loadedJSON), PARSER::invalid);

#define PARSE_ARRAY_VARIABLE(REF, NAME, INDEX)\
    const std::size_t l = (REF).get_number_variables();\
    for (std::size_t i = 0; i < l; ++i) {\
        const VariableDeclaration* var_decl = (REF).get_variable(i);\
        if (var_decl->get_name() == (NAME)) {\
            std::unique_ptr<InputFeatureToArray> input_feature_to(new InputFeatureToArray(*var_decl, (INDEX)));\
            PARSER::throw_parser_exception_if(var_decl->get_size() < (INDEX), input, (*loadedJSON), PARSER::invalid);\
            return input_feature_to;\
        }\
    }\
    PARSER::throw_parser_exception(input, (*loadedJSON), PARSER::invalid);
//

std::unique_ptr<InputFeatureToJANI> Jani2NNetParser::parse_InputFeatureToValueVariable(const nlohmann::json& input) const {
    PARSER::check_object(input);

    // name
    PARSER::check_contains(input, JANI::NAME);
    const nlohmann::json& name = input.at(JANI::NAME);
    PARSER::check_string(name, input);

    // automaton
    PARSER::check_contains(input, JANI::AUTOMATON);
    const nlohmann::json& automaton_str = input.at(JANI::AUTOMATON);
    if (automaton_str.is_null()) { // global
        PARSE_VARIABLE(model, name)
    } else {
        const Automaton& automaton = *model.get_automatonInstance(parse_automaton_instance(automaton_str));
        PARSE_VARIABLE(automaton, name)
    }

}

std::unique_ptr<InputFeatureToJANI> Jani2NNetParser::parse_InputFeatureToArrayVariable(const nlohmann::json& input) const {
    PARSER::check_object(input);

    // name
    PARSER::check_contains(input, JANI::NAME);
    const nlohmann::json& name = input.at(JANI::NAME);
    PARSER::check_string(name, input);

    // index
    PARSER::check_contains(input, JANI::INDEX);
    const nlohmann::json& index = input.at(JANI::INDEX);
    PARSER::throw_parser_exception_if(not index.is_number_integer() or index < 0, index, input, PARSER::invalid);

    // automaton
    PARSER::check_contains(input, JANI::AUTOMATON);
    const nlohmann::json& automaton_str = input.at(JANI::AUTOMATON);
    if (automaton_str.is_null()) { // global
        PARSE_ARRAY_VARIABLE(model, name, index)
    } else {
        const Automaton& automaton = *model.get_automatonInstance(parse_automaton_instance(automaton_str));
        PARSE_ARRAY_VARIABLE(automaton, name, index)
    }
}

/**********************************************************************************************************************/

std::string Jani2NNetParser::get_nn_file() const {
    PARSER::check_contains((*loadedJSON), JANI::FILE);
    const nlohmann::json& file = loadedJSON->at(JANI::FILE);
    PARSER::check_string(file, (*loadedJSON));
    return PLAJA_UTILS::extract_directory_path(jani2NNetFile).append(file);
}

std::unique_ptr<std::string> Jani2NNetParser::get_torch_file() const {
    if (loadedJSON->contains(JANI::TORCH_FILE)) {
        const nlohmann::json& torch_file = loadedJSON->at(JANI::TORCH_FILE);
        PARSER::check_string(torch_file, (*loadedJSON));
        return std::make_unique<std::string>(PLAJA_UTILS::extract_directory_path(jani2NNetFile).append(torch_file));
    } else {
        return nullptr;
    }
}

bool Jani2NNetParser::applicability_filtering_enabled() const {
    if (loadedJSON->contains(JANI::FILTER)) {
        const nlohmann::json& filter = loadedJSON->at(JANI::FILTER);
        PARSER::throw_parser_exception_if(not filter.is_boolean(), filter, (*loadedJSON), PARSER::invalid);
        PARSER::throw_parser_exception_if(not PLAJA_GLOBAL::enableApplicabilityFiltering and filter, PARSER::to_json(JANI::FILTER, filter), (*loadedJSON), PARSER::invalid);
        return filter;
    } else {
        return false;
    }
}

std::vector<std::unique_ptr<InputFeatureToJANI>> Jani2NNetParser::parse_input_interface() const {
    PARSER::check_contains((*loadedJSON), JANI::INPUT);
    const nlohmann::json& input = loadedJSON->at(JANI::INPUT);
    PARSER::check_array(input, (*loadedJSON));
    PARSER::throw_parser_exception_if(input.empty(), input, (*loadedJSON), PARSER::at_least_one_element);

    std::vector<std::unique_ptr<InputFeatureToJANI>> input_feature_mapping;
    input_feature_mapping.reserve(input.size());
    for (const auto& input_feature_to: input) {
        input_feature_mapping.push_back(parse_InputFeatureToJANI(input_feature_to));
    }

    return input_feature_mapping;
}

std::vector<unsigned int> Jani2NNetParser::parse_hidden_layer_structure() const {
    PARSER::check_contains((*loadedJSON), JANI::ELEMENTS);
    const nlohmann::json& elements = loadedJSON->at(JANI::ELEMENTS);
    PARSER::check_array(elements, (*loadedJSON));
    PARSER::throw_parser_exception_if(elements.empty(), elements, (*loadedJSON), PARSER::at_least_one_element);

    std::vector<unsigned int> hidden_layer_sizes;
    hidden_layer_sizes.reserve(elements.size());
    for (const auto& element: elements) {
        PARSER::throw_parser_exception_if(not(element.is_number_integer() and element > 0), elements, (*loadedJSON), PARSER::invalid);
        hidden_layer_sizes.push_back(element);
    }
    return hidden_layer_sizes;
}

std::vector<ActionLabel_type> Jani2NNetParser::parse_output_interface() const {
    PARSER::check_contains((*loadedJSON), JANI::OUTPUT);
    const nlohmann::json& output = loadedJSON->at(JANI::OUTPUT);
    PARSER::check_array(output, (*loadedJSON));
    PARSER::throw_parser_exception_if(output.empty(), output, (*loadedJSON), PARSER::at_least_one_element);

    // cache actions
    std::unordered_map<std::string, ActionID_type> action_name_to_id;
    for (auto it = ModelIterator::actionIterator(model); !it.end(); ++it) { action_name_to_id[it->get_name()] = it->get_id(); }

    std::vector<ActionLabel_type> output_to_labels;
    output_to_labels.reserve(output.size());
    std::unordered_set<ActionLabel_type> output_to_labels_cache; // for duplicate detection
    for (const auto& action: output) {
        PARSER::check_string(action, output);
        PARSER::throw_parser_exception_if(not action_name_to_id.count(action), action, output, PARSER::unknown_name);
        output_to_labels.push_back(action_name_to_id.at(action));
        PARSER::throw_parser_exception_if(output_to_labels_cache.count(output_to_labels.back()), action, output, PARSER::duplicate_name);
        output_to_labels_cache.insert(output_to_labels.back());
    }

    return output_to_labels;
}

/**********************************************************************************************************************/

nlohmann::json Jani2NNetParser::input_feature_to_json(const InputFeatureToJANI& input_feature) const {
    nlohmann::json input_feature_json;

    if (PLAJA_UTILS::is_dynamic_type<InputFeatureToLocation>(input_feature)) {

        const auto& input_feature_loc = PLAJA_UTILS::cast_ref<InputFeatureToLocation>(input_feature);
        input_feature_json.emplace(JANI::NAME, model.get_automatonInstance(PLAJA_UTILS::cast_numeric<AutomatonIndex_type>(input_feature_loc.stateIndex))->get_name());
        input_feature_json.emplace(JANI::INDEX, input_feature_loc.stateIndex);

    } else if (PLAJA_UTILS::is_dynamic_type<InputFeatureToVariable>(input_feature)) {

        const auto& input_feature_var = PLAJA_UTILS::cast_ref<InputFeatureToVariable>(input_feature);
        input_feature_json.emplace(JANI::NAME, model.get_variable_by_id(input_feature_var.get_variable_id())->get_name());
        const AutomatonIndex_type automaton_index = model.get_variable_automaton_instance(input_feature_var.get_variable_id());

        if (automaton_index == AUTOMATON::invalid) { input_feature_json.emplace(JANI::AUTOMATON, nullptr); }
        else {
            nlohmann::json automaton;
            automaton.emplace(JANI::NAME, model.get_automatonInstance(automaton_index)->get_name());
            automaton.emplace(JANI::INDEX, automaton_index);
            input_feature_json.emplace(JANI::AUTOMATON, automaton);
        }

    } else if (PLAJA_UTILS::is_dynamic_type<InputFeatureToArray>(input_feature)) {

        const auto& input_feature_arr = PLAJA_UTILS::cast_ref<InputFeatureToArray>(input_feature);
        input_feature_json.emplace(JANI::NAME, model.get_variable_by_id(input_feature_arr.get_variable_id())->get_name());
        input_feature_json.emplace(JANI::INDEX, input_feature_arr.get_array_index());

        const AutomatonIndex_type automaton_index = model.get_variable_automaton_instance(input_feature_arr.get_variable_id());
        if (automaton_index == AUTOMATON::invalid) { input_feature_json.emplace(JANI::AUTOMATON, nullptr); }
        else {
            nlohmann::json automaton;
            automaton.emplace(JANI::NAME, model.get_automatonInstance(automaton_index)->get_name());
            automaton.emplace(JANI::INDEX, automaton_index);
            input_feature_json.emplace(JANI::AUTOMATON, automaton);
        }
    }

    return input_feature_json;
}