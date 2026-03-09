//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <iostream>
#include <algorithm>
#include "nn_policy.h"
#include "../../../parser/nn_parser/neural_network.h"
#include "../../../utils/floating_utils.h"
#include "../../../utils/utils.h"
#include "../../factories/configuration.h"
#include "../../information/jani2nnet/jani_2_nnet.h"
#include "../../information/input_feature_to_jani.h"
#include "../../states/state_base.h"
#include "../../successor_generation/successor_generator_c.h"

NNPolicy::NNPolicy(const PLAJA::Configuration& config, const Jani2NNet& nn_interface):
    network(&(nn_interface.load_network()))
    , nnInterface(&nn_interface)
    , successorGenerator(nullptr)
    , cachedLabel(ACTION::nullAction)
    , selectionDelta(0) {

    // cache inputs
    inputFeatures.reserve(nnInterface->get_num_input_features());
    for (auto it = nnInterface->init_input_feature_iterator(); !it.end(); ++it) { inputFeatures.push_back(it.get_input_feature_index()); }

    // in case of app filtering
    if (nnInterface->_do_applicability_filtering()) {

        if (not config.has_sharable(PLAJA::SharableKey::SUCC_GEN)) {
            config.set_sharable<SuccessorGeneratorC>(PLAJA::SharableKey::SUCC_GEN, std::make_shared<SuccessorGeneratorC>(config, nnInterface->get_model()));
        }

        successorGenerator = config.get_sharable_as_const<SuccessorGeneratorC>(PLAJA::SharableKey::SUCC_GEN);

        PLAJA_ASSERT(&successorGenerator->get_model() == &nnInterface->get_model())

    }
}

NNPolicy::~NNPolicy() = default;

// internal evaluate:

void NNPolicy::evaluate_nn(const StateBase& state, std::vector<Floating_type>& outputs) const {
    std::vector<Floating_type> inputs;
    inputs.reserve(inputFeatures.size());
    for (auto state_index: inputFeatures) { inputs.push_back(state[state_index]); }
    auto num_outputs = nnInterface->get_num_output_features();
    network->evaluate(inputs, outputs, num_outputs);
}

ActionLabel_type NNPolicy::evaluate_argmax(const std::vector<Floating_type>& outputs) const {
    OutputIndex_type chosen_output = 0;
    Floating_type value = outputs[0];
    auto num_outputs = outputs.size();
    for (OutputIndex_type output_index = 1; output_index < num_outputs; ++output_index) {
        const Floating_type tmp_value = outputs[output_index]; // NOLINT(cppcoreguidelines-narrowing-conversions)
        if (tmp_value > value) {
            chosen_output = output_index;
            value = tmp_value;
        }
    }

    return nnInterface->get_output_label(chosen_output);
}

bool NNPolicy::is_chosen_argmax(ActionLabel_type action_label, double tolerance) const { // NOLINT(bugprone-easily-swappable-parameters)
    PLAJA_ASSERT(cachedLabel != ACTION::nullAction and action_label != cachedLabel)
    const auto label_value = cachedOutputs[nnInterface->get_output_index(action_label)]; // label of interest
    const auto value_chosen_label = cachedOutputs[nnInterface->get_output_index(cachedLabel)]; // label chosen with precision (0-tolerance)
    PLAJA_ASSERT(value_chosen_label >= label_value)
    selectionDelta = value_chosen_label - label_value;
    PLAJA_ASSERT(selectionDelta >= 0)
    return PLAJA_FLOATS::is_zero(selectionDelta, tolerance);

#if 0
    auto num_outputs = outputs.size();
    for (OutputIndex_type output_index = 0; output_index < num_outputs; ++output_index) {
        if (output_index == label_index) { continue; }
        if (not PLAJA_FLOATS::gte(label_value, outputs[output_index], tolerance)) { return false; }
    }
    return true;
#endif

}

ActionLabel_type NNPolicy::evaluate_app_filtering(const StateBase& state, const std::vector<Floating_type>& outputs, std::vector<bool>& enabled_labels) const {
    PLAJA_ASSERT_EXPECT(not successorGenerator->is_terminal(state))
    PLAJA_ASSERT(enabled_labels.empty())

    // first sort in DEC order
    auto num_outputs = outputs.size();
    std::vector<std::pair<Floating_type, int>> sorted_outputs;
    sorted_outputs.reserve(num_outputs);
    enabled_labels.reserve(num_outputs);
    for (OutputIndex_type output_index = 0; output_index < num_outputs; ++output_index) {
        sorted_outputs.emplace_back(outputs[output_index], output_index);
        enabled_labels.push_back(successorGenerator->is_applicable(state, nnInterface->get_output_label(output_index))); // also cache enabled labels
    }
    std::sort(sorted_outputs.begin(), sorted_outputs.end(), outputSort);

    // now iterate
    for (const auto& value_index: sorted_outputs) {
        const ActionLabel_type action_label = nnInterface->get_output_label(value_index.second);
        if (enabled_labels[value_index.second]) { return action_label; }
    }

    // no applicable op
    return nnInterface->get_output_label(sorted_outputs.front().second); // fall back to no-filtering
}

bool NNPolicy::is_chosen_app_filtering(ActionLabel_type action_label, double tolerance) const { // NOLINT(bugprone-easily-swappable-parameters)
    PLAJA_ASSERT(cachedLabel != ACTION::nullAction and action_label != cachedLabel)
    const auto label_index = nnInterface->get_output_index(action_label); // label of interest
    const auto cached_label_index = nnInterface->get_output_index(cachedLabel); // label chosen with precision (0-tolerance)
    const auto label_value = cachedOutputs[label_index];
    const auto value_chosen_label = cachedOutputs[cached_label_index];
    PLAJA_ASSERT(cachedEnabledLabels[cached_label_index] or not cachedEnabledLabels[label_index])

    if (cachedEnabledLabels[cached_label_index] and not cachedEnabledLabels[label_index]) { return false; } // if chosen label is enabled, then so must be the label of interest to be chosen with tolerance

    PLAJA_ASSERT(value_chosen_label >= label_value)
    selectionDelta = value_chosen_label - label_value;
    PLAJA_ASSERT(selectionDelta >= 0)
    return PLAJA_FLOATS::is_zero(selectionDelta, tolerance);

#if 0
    auto num_outputs = outputs.size();

    if (enabled_labels[label_index]) { // applicable if output is max over applicable labels ...

        for (OutputIndex_type output_index = 0; output_index < num_outputs; ++output_index) {
            if (output_index == label_index or not enabled_labels[output_index]) { continue; }
            if (not PLAJA_FLOATS::gte(label_value, outputs[output_index], tolerance)) { return false; }
        }

    } else { // only applicable if no other label is applicable and output is max ...

        for (OutputIndex_type output_index = 0; output_index < num_outputs; ++output_index) {
            if (output_index == label_index) { continue; }
            if (not PLAJA_FLOATS::gte(label_value, outputs[output_index], tolerance) or enabled_labels[output_index]) { return false; }
        }

    }

    return true;
#endif

}

// interface:
ActionLabel_type NNPolicy::evaluate(const StateBase& state) const {
    cachedOutputs.clear();
    cachedEnabledLabels.clear();
    selectionDelta = 0;
    evaluate_nn(state, cachedOutputs); // evaluate NN
    return cachedLabel = (nnInterface->_do_applicability_filtering() ? evaluate_app_filtering(state, cachedOutputs, cachedEnabledLabels) : evaluate_argmax(cachedOutputs)); // select action label
}

std::vector<double> NNPolicy::action_values(const StateBase& state) const {
    cachedOutputs.clear();
    cachedEnabledLabels.clear();
    selectionDelta = 0;
    evaluate_nn(state, cachedOutputs); // evaluate NN
    return cachedOutputs;
}

bool NNPolicy::is_chosen(const StateBase& state, ActionLabel_type action_label, double tolerance) const {
    evaluate(state);
    return is_chosen(action_label, tolerance);
}

bool NNPolicy::is_chosen(ActionLabel_type action_label, double tolerance) const { // NOLINT(bugprone-easily-swappable-parameters)
    selectionDelta = 0;

    // exact ?
    if (not nnInterface->is_learned(action_label) or action_label == cachedLabel) { return true; }

    // tolerance?
    return (tolerance > 0 and (nnInterface->_do_applicability_filtering() ? is_chosen_app_filtering(action_label, tolerance) : is_chosen_argmax(action_label, tolerance)));
}

#ifndef NDEBUG

#include <sstream>

void NNPolicy::dump_input_features(const StateBase& state) const {
    std::stringstream ss;
    for (auto state_index: inputFeatures) { ss << state[state_index] << PLAJA_UTILS::commaString << PLAJA_UTILS::spaceString; }
    PLAJA_LOG(ss.str())
}

void NNPolicy::dump_cached_outputs() const {
    std::stringstream ss;
    ss << "Policy output: ";
    for (auto val: cachedOutputs) { ss << val << PLAJA_UTILS::commaString; }
    ss << " chosen label: " << cachedLabel;
    ss << ", last selection delta: " << selectionDelta << PLAJA_UTILS::dotString;
    PLAJA_LOG(ss.str())
}

#endif