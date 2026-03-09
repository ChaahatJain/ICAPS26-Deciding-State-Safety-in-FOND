//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//


#include <iostream>
#include "jani_2_nnet.h"
#include "../../../exception/constructor_exception.h"
#include "../../../exception/parser_exception.h"
#include "../../../option_parser/option_parser.h"
#include "../../../parser/nn_parser/neural_network.h"
#include "../../../parser/ast/type/declaration_type.h"
#include "../../../parser/ast/model.h"
#include "../../../parser/ast/variable_declaration.h"
#include "../../non_prob_search/policy/nn_policy.h"
#include "../racetrack/racetrack_model_info.h"
#include "../model_information.h"
#include "../input_feature_to_jani_derived.h"

// extern:
namespace PLAJA_OPTION {
    extern const std::string nn_interface;
    extern const std::string nn_for_racetrack;
}

namespace JANI_2_NNET {
    const std::string layerSizeError { "Jani2NNet layer size error" }; // NOLINT(cert-err58-cpp)

    extern std::unique_ptr<Jani2NNet> load_nn_interface(const Model& model, const std::string& nn_interface);
    extern void dump_nn_interface(const Jani2NNet& nn_interface);
}

/**********************************************************************************************************************/

// Racetrack hardcoding

class Racetrack2NNet: public Jani2NNet {
private:
    const std::unique_ptr<RacetrackModelInfo> racetrackModelInfo;
public:
    Racetrack2NNet(const Model& model, const std::string& nn_file):
        Jani2NNet(model)
        , racetrackModelInfo(new RacetrackModelInfo(model)) {
        // use old nn:
        nnFile = nn_file;
        torchFile = PLAJA_UTILS::extract_directory_path(get_policy_file()) + PLAJA_UTILS::extract_filename(get_policy_file(), false) + ".pth";

        const RacetrackModelInfo& racetrack_model_info = *racetrackModelInfo; // view

        // inputs (y and x are inverted in the learning infrastructure!!!)
        inputFeatureMapping.reserve(4);
        // yPos
        inputFeatureMapping.emplace_back(new InputFeatureToVariable(*racetrack_model_info._y_pos_var()));
        // xPos
        inputFeatureMapping.emplace_back(new InputFeatureToVariable(*racetrack_model_info._x_pos_var()));
        // yVel
        inputFeatureMapping.emplace_back(new InputFeatureToVariable(*racetrack_model_info._y_vel_var()));
        // xVel
        inputFeatureMapping.emplace_back(new InputFeatureToVariable(*racetrack_model_info._x_vel_var()));

        // outputs
        outputsToLabels = racetrack_model_info._non_deterministic_labels();
        compute_labels_to_outputs();

        // hidden layers:
        if (PLAJA_UTILS::contains(nn_file, "_2_16_")) {
            hiddenLayerSizes = std::vector<unsigned int> { 16, 16 };
        } else if (PLAJA_UTILS::contains(nn_file, "_2_24_")) {
            hiddenLayerSizes = std::vector<unsigned int> { 24, 24 };
        } else if (PLAJA_UTILS::contains(nn_file, "_2_32_")) {
            hiddenLayerSizes = std::vector<unsigned int> { 32, 32 };
        } else if (PLAJA_UTILS::contains(nn_file, "_2_64_")) {
            hiddenLayerSizes = std::vector<unsigned int> { 64, 64 };
        } else {
#ifndef NDEBUG // for testing instances
            if (PLAJA_UTILS::contains(nn_file, "_1")) {
                hiddenLayerSizes = std::vector<unsigned int> { 1 };
            } else
#endif
            {
                throw ConstructorException("Warning: Racetrack2NNet could not derive hidden layer sizes from file: " + nn_file + PLAJA_UTILS::dotString);
            }
        }

        // all layer sizes:
        compute_layer_sizes();
    }

    ~Racetrack2NNet() override = default;

    DELETE_CONSTRUCTOR(Racetrack2NNet)

};

/**********************************************************************************************************************/

Jani2NNet::~Jani2NNet() = default;

Jani2NNet::Jani2NNet(const Jani2NNet& jani_2_interface): Jani2Interface() {
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

Jani2NNet::Jani2NNet(const Model& model_): Jani2Interface() {
    this->model = &model_;
    this->interfaceFile = "";
    this->policyFile = "";
    this->doApplicabilityFiltering = false;
}

std::unique_ptr<Jani2NNet> Jani2NNet::load_interface(const Model& model) {
    return load_interface(model, PLAJA_GLOBAL::optionParser->get_value_option_string(PLAJA_OPTION::nn_interface));
}

std::unique_ptr<Jani2NNet> Jani2NNet::load_interface(const Model& model, const std::string& jani_2_nnet_file) {
    if (PLAJA_GLOBAL::optionParser->has_value_option(PLAJA_OPTION::nn_for_racetrack)) {
        return std::make_unique<Racetrack2NNet>(model, jani_2_nnet_file);
    } else {
        try { return JANI_2_NNET::load_nn_interface(model, jani_2_nnet_file); }
        catch (ParserException& e) { throw ConstructorException(e.what()); }
    }
}

std::unique_ptr<Jani2Interface> Jani2NNet::copy_interface(const std::string& path_to_directory, const std::string* filename_prefix, bool dump) const {
    auto copy = std::unique_ptr<Jani2NNet>(new Jani2NNet(*this));
    copy->interfaceFile = PLAJA_UTILS::join_path(path_to_directory, { (filename_prefix ? *filename_prefix : PLAJA_UTILS::emptyString) + PLAJA_UTILS::extract_filename(interfaceFile, true) });
    copy->nnFile = PLAJA_UTILS::join_path(path_to_directory, { (filename_prefix ? *filename_prefix : PLAJA_UTILS::emptyString) + PLAJA_UTILS::extract_filename(nnFile, true) });
    copy->torchFile = PLAJA_UTILS::join_path(path_to_directory, { (filename_prefix ? *filename_prefix : PLAJA_UTILS::emptyString) + PLAJA_UTILS::extract_filename(torchFile, true) });
    if (dump) { JANI_2_NNET::dump_nn_interface(*copy); }
    return copy;
}

// loading NN

const PLAJA::NeuralNetwork& Jani2NNet::load_network() const {
    if (nn) { return *nn; }
    std::cout << "Loading NN file ..." << nnFile << std::endl;
    try {

        nn = std::make_unique<PLAJA::NeuralNetwork>(nnFile); // unfortunately parser results in segmentation fault for invalid but existing NN files

        // sanity checks
        auto num_layers = layerSizes.size();
        if (nn->getNumLayers() != num_layers - 1) { throw ConstructorException(JANI_2_NNET::layerSizeError); } // -1, does not count input
        for (LayerIndex_type layer_index = 0; layer_index < num_layers; ++layer_index) {
            if (nn->getLayerSize(layer_index) != layerSizes[layer_index]) { throw ConstructorException(JANI_2_NNET::layerSizeError); }
        }

        // sanity check bounds (NN vs. state variable bounds as per JANI model)
        bool differing_bounds = false;
        for (auto input_it = init_input_feature_iterator(false); !input_it.end(); ++input_it) {
            auto input_index = input_it._input_index();
            auto state_index = input_it.get_input_feature_index();
            if (nn->getInputLowerBound(input_index) != get_model_info().get_lower_bound(state_index) or nn->getInputUpperBound(input_index) != get_model_info().get_upper_bound(state_index)) {
                differing_bounds = true;
            }
            // always use state variable bounds:
            nn->min[input_index] = get_model_info().get_lower_bound(state_index);
            nn->max[input_index] = get_model_info().get_upper_bound(state_index);
        }
        if (differing_bounds) { std::cout << "Warning: NN input bounds and state variable bounds differ (latter will be used)." << std::endl; }
        std::cout << "Successfully loaded nnet" << std::endl;
        PLAJA_ASSERT(nn)
        return *nn;

    } catch (ParserException& e) { throw ConstructorException("Error while parsing NN file."); }
}

const Policy& Jani2NNet::load_policy(const PLAJA::Configuration& config) const {
    if (not nnPolicy) { nnPolicy = std::make_unique<NNPolicy>(config, *this); PLAJA_ASSERT(nnPolicy); }
    return *nnPolicy;
}

#ifndef NDEBUG

#include "../../factories/configuration.h"

ActionLabel_type Jani2NNet::eval_policy(const StateBase& state) const {
    const PLAJA::Configuration config;
    const auto& nn_policy = load_policy(config);
    return nn_policy.evaluate(state);
}

bool Jani2NNet::is_chosen(const StateBase& state, ActionLabel_type action_label) const {
    const PLAJA::Configuration config;
    const auto& nn_policy = load_policy(config);
    return nn_policy.is_chosen(state, action_label, PLAJA_NN::argMaxPrecision);
}

#endif

// further auxiliaries

void Jani2NNet::compute_labels_to_outputs() {
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

void Jani2NNet::compute_layer_sizes() {
    // layer sizes
    layerSizes.reserve(hiddenLayerSizes.size() + 2);
    layerSizes.push_back(inputFeatureMapping.size());
    for (const unsigned int size: hiddenLayerSizes) {
        layerSizes.push_back(size);
    }
    layerSizes.push_back(outputsToLabels.size());
}

void Jani2NNet::compute_extended_inputs() {
    PLAJA_ASSERT(not layerSizes.empty()) // Must be computed first.

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
