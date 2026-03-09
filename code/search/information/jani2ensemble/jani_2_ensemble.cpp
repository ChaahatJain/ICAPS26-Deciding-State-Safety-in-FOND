//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//


#include <iostream>
#include <fstream>
#include <sstream>
#include "jani_2_ensemble.h"
#include "../../../exception/constructor_exception.h"
#include "../../../exception/parser_exception.h"
#include "../../../option_parser/option_parser.h"
#include "../../../parser/nn_parser/neural_network.h"
#include "../../../parser/ast/type/declaration_type.h"
#include "../../../parser/ast/model.h"
#include "../../../parser/ast/variable_declaration.h"
#include "../../non_prob_search/policy/ensemble_policy.h"
#include "../racetrack/racetrack_model_info.h"
#include "../model_information.h"
#include "../input_feature_to_jani_derived.h"
#include "using_jani2ensemble.h"
// extern:
namespace PLAJA_OPTION {
    extern const std::string ensemble_interface;
    extern const std::string ensemble;
}

namespace JANI_2_ENSEMBLE {
    const std::string layerSizeError { "Jani2Ensemble layer size error" }; // NOLINT(cert-err58-cpp)

    extern std::unique_ptr<Jani2Ensemble> load_ensemble_interface(const Model& model, const std::string& ensemble_interface);
    extern void dump_ensemble_interface(const Jani2Ensemble& ensemble_interface);
}

/**********************************************************************************************************************/
Jani2Ensemble::~Jani2Ensemble() = default;
Jani2Ensemble::Jani2Ensemble(const Jani2Ensemble& jani_2_interface): Jani2Interface() {
    this->model = jani_2_interface.model;
    this->interfaceFile = jani_2_interface.interfaceFile;
    this->policyFile = jani_2_interface.policyFile;
    this->doApplicabilityFiltering = jani_2_interface.doApplicabilityFiltering;
    this->layerSizes = jani_2_interface.layerSizes;
    this->hiddenLayerSizes = jani_2_interface.hiddenLayerSizes;
    this->outputsToLabels = jani_2_interface.outputsToLabels;
    this->labelsToOutputs = jani_2_interface.labelsToOutputs;
    inputFeatureMapping.reserve(jani_2_interface.inputFeatureMapping.size());
    for (const auto& input_feature: jani_2_interface.inputFeatureMapping) { inputFeatureMapping.emplace_back(input_feature->deep_copy()); }
}

Jani2Ensemble::Jani2Ensemble(const Model& model_): Jani2Interface() {
        this->model = &model_;
        this->interfaceFile = "";
        this->policyFile = "";
        this->doApplicabilityFiltering = false;
    }

std::unique_ptr<Jani2Ensemble> Jani2Ensemble::load_interface(const Model& model) {
    return load_interface(model, PLAJA_GLOBAL::optionParser->get_value_option_string(PLAJA_OPTION::ensemble_interface));
}

std::unique_ptr<Jani2Ensemble> Jani2Ensemble::load_interface(const Model& model, const std::string& jani_2_ensemble_file) {
        try { return JANI_2_ENSEMBLE::load_ensemble_interface(model, jani_2_ensemble_file); }
        catch (ParserException& e) { throw ConstructorException(e.what()); }
}

std::unique_ptr<Jani2Interface> Jani2Ensemble::copy_interface(const std::string& path_to_directory, const std::string* filename_prefix, bool dump) const {
    auto copy = std::unique_ptr<Jani2Ensemble>(new Jani2Ensemble(*this));
    copy->interfaceFile = PLAJA_UTILS::join_path(path_to_directory, { (filename_prefix ? *filename_prefix : PLAJA_UTILS::emptyString) + PLAJA_UTILS::extract_filename(interfaceFile, true) });
    copy->ensembleFile = PLAJA_UTILS::join_path(path_to_directory, { (filename_prefix ? *filename_prefix : PLAJA_UTILS::emptyString) + PLAJA_UTILS::extract_filename(ensembleFile, true) });
    if (dump) { JANI_2_ENSEMBLE::dump_ensemble_interface(*copy); }
    return copy;
}

// loading NN

const veritas::AddTree& Jani2Ensemble::load_ensemble() const {
    try {
        PLAJA_ASSERT(PLAJA_GLOBAL::optionParser->has_value_option(PLAJA_OPTION::ensemble))
        auto json_path = PLAJA_GLOBAL::optionParser->get_value_option_string(PLAJA_OPTION::ensemble);
        std::ifstream jsonFile(json_path);
        if (jsonFile.is_open()) {
            std::cout << "Opening File: " << json_path << std::endl;
            // Read the JSON file contents into a string
            std::string jsonString((std::istreambuf_iterator<char>(jsonFile)),
                                std::istreambuf_iterator<char>());

            // Convert the JSONString into something usable
            std::istringstream iss(jsonString);
            ensemble = veritas::addtree_from_json<veritas::AddTree>(iss); 
            std::cout << "Leafs: " << ensemble.num_leafs() << " Nodes: " << ensemble.num_nodes() << std::endl;       
        } else {
            std::cerr << "Failed to open the JSON file." << std::endl;
            PLAJA_ASSERT(false);
        }
        return ensemble;
    } catch (ParserException& e) { throw ConstructorException("Error while parsing Ensemble file."); }
}


const Policy& Jani2Ensemble::load_policy(const PLAJA::Configuration& config) const {
    if (not ensemblePolicy) { ensemblePolicy = std::make_unique<EnsemblePolicy>(config, *this); }
    return *ensemblePolicy;
}

#ifndef NDEBUG

#include "../../factories/configuration.h"

ActionLabel_type Jani2Ensemble::eval_policy(const StateBase& state) const {
    const PLAJA::Configuration config;
    const auto& policy = load_policy(config);
    return policy.evaluate(state);
}

bool Jani2Ensemble::is_chosen(const StateBase& state, ActionLabel_type action_label) const {
    const PLAJA::Configuration config;
    const auto& policy = load_policy(config);
    return policy.is_chosen(state, action_label, PLAJA_NN::argMaxPrecision);
}

#endif

// further auxiliaries
void Jani2Ensemble::compute_layer_sizes() {
    // layer sizes
    layerSizes.reserve(2);
    layerSizes.push_back(inputFeatureMapping.size());
    layerSizes.push_back(outputsToLabels.size());
}
void Jani2Ensemble::compute_labels_to_outputs() {
    for (OutputIndex_type output_index = 0; output_index < outputsToLabels.size(); ++output_index) {
        labelsToOutputs[outputsToLabels[output_index]] = output_index;
    }
    // also cache unlearned labels:
    unlearnedLabels.push_back(ACTION::silentLabel);
    auto num_actions = model->get_number_actions();
    unlearnedLabels.reserve(num_actions - get_num_output_features() + 1);
    for (ActionLabel_type action_label = 0; action_label < num_actions; ++action_label) {
        if (!is_learned(action_label)) { unlearnedLabels.push_back(action_label); }
    }
}

void Jani2Ensemble::compute_extended_inputs() {

    if (not doApplicabilityFiltering) { return; }

    const auto& model_info = get_model_info();

    // Include locs?
    bool include_locs = false;
    for (auto loc_it = model_info.locationIndexIterator(); !loc_it.end(); ++loc_it) {
        if (loc_it.range() > 1) {
            include_locs = true;
            break;
        }
    }

    std::unordered_set<VariableIndex_type> actual_input_features;

    for (auto it = init_input_feature_iterator(false); !it.end(); ++it) { actual_input_features.insert(it.get_input_feature_index()); }

    const auto guard_vars = model->collect_guard_variables(include_locs);

    for (auto it = model_info.stateIndexIterator(include_locs); !it.end(); ++it) {
        const auto state_index = it.state_index();

        if (not actual_input_features.count(state_index) and guard_vars.count(state_index)) {

            if (it.is_location()) {
                inputFeatureMapping.push_back(std::make_unique<InputFeatureToLocation>(it.location_index()));
            } else {

                const auto id = model_info.get_id(state_index);

                if (model_info.is_array(id)) {
                    const auto& var_decl = *model->get_variable_by_id(id);
                    inputFeatureMapping.push_back(std::make_unique<InputFeatureToArray>(var_decl, state_index - var_decl.get_index()));
                } else { inputFeatureMapping.push_back(std::make_unique<InputFeatureToVariable>(*model->get_variable_by_id(id))); }

            }

        }

    }
}