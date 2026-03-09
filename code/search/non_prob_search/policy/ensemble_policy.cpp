//
// This file is part of the PlaJA code base (2019 - 2021).
//

#include <iostream>
#include <algorithm>
#include "ensemble_policy.h"
#include "../../../utils/floating_utils.h"
#include "../../../utils/utils.h"
#include "../../information/jani2ensemble/jani_2_ensemble.h"
#include "../../information/input_feature_to_jani.h"
#include "../../states/state_base.h"
#include "../../successor_generation/successor_generator_c.h"
#include "../../factories/configuration.h"

EnsemblePolicy::EnsemblePolicy(const PLAJA::Configuration& config, const Jani2Ensemble& ensemble_interface):
        veritas_tree(ensemble_interface.load_ensemble()),
        ensembleInterface(&ensemble_interface),
        successorGenerator(nullptr),
        cachedLabel(ACTION::nullAction),
        chosenWithTolerance(false) {
    // cache inputs
    std::size_t l = ensembleInterface->get_num_input_features();
    inputFeatures.reserve(l);
    for (InputIndex_type i = 0; i < l; ++i) { inputFeatures.push_back(ensembleInterface->get_input_feature(i)->stateIndex); }

    // in case of app filtering
 // in case of app filtering
    if (ensembleInterface->_do_applicability_filtering()) {

        if (not config.has_sharable(PLAJA::SharableKey::SUCC_GEN)) {
            config.set_sharable<SuccessorGeneratorC>(PLAJA::SharableKey::SUCC_GEN, std::make_shared<SuccessorGeneratorC>(config, ensembleInterface->get_model()));
        }

        successorGenerator = config.get_sharable_as_const<SuccessorGeneratorC>(PLAJA::SharableKey::SUCC_GEN);

        PLAJA_ASSERT(&successorGenerator->get_model() == &ensembleInterface->get_model())

    }
}

EnsemblePolicy::~EnsemblePolicy() = default;

// internal evaluate:

void EnsemblePolicy::evaluate_trees(const StateBase& state, std::vector<Floating_type>& outputs) const {
    // TODO: inputs.push_back only works when we assume the jani model file and the jani2nnet interface have the same ordering for variables. 
    // Otherwise, the input to the policy can be something that is not the actual state.
    
    std::vector<Floating_type> inputs;
    inputs.reserve(state.size());
    // state.dump(nullptr);
    for (auto state_index: inputFeatures) { 
        inputs.push_back(state[state_index]);
    }
    // std::cout << std::endl;
    // std::cout << "Input state is: [";
    //  for (auto i : inputs) {
    //      std::cout << i << ", ";
    //  }
    // std::cout << "]" << std::endl;
    auto num_outputs = ensembleInterface->get_num_output_features();
    outputs.resize(num_outputs);
    veritas::data d {&inputs[0], 1, inputs.size(), inputs.size(), 1};
    veritas::data<double> outdata(outputs);
    veritas_tree.eval(d.row(0), outdata);
    // PLAJA_LOG("Actual state in evaluate is: "); state.dump();
    // std::cout << "Veritas num trees: " << veritas_tree << std::endl;
    for (int o = 0; o < num_outputs; ++o) {
        outputs[o] = outdata[o];
    }
//    std::cout << "Inside evaluate, we have number of outputs: " << num_outputs << " and got outputs: [";
//    for (auto o : outputs) {
//        std::cout << o << ", ";
//    }
//    std::cout << "]" << std::endl;
}

ActionLabel_type EnsemblePolicy::evaluate_argmax(const std::vector<Floating_type>& outputs) const {

    ActionLabel_type action_label = ensembleInterface->get_output_label(0);
    Floating_type value = outputs[0];
    auto num_outputs = outputs.size();
    for (OutputIndex_type output_index = 1; output_index < num_outputs; ++output_index) {
        Floating_type tmp_value = outputs[output_index]; // NOLINT(cppcoreguidelines-narrowing-conversions)
        if (tmp_value > value) {
            action_label = ensembleInterface->get_output_label(output_index);
            value = tmp_value;
        }
    }
    return action_label;
}

bool EnsemblePolicy::is_chosen_argmax(const std::vector<Floating_type>& outputs, ActionLabel_type action_label, double tolerance) const {
    PLAJA_ASSERT(cachedLabel != ACTION::nullAction and action_label != cachedLabel)
    auto label_index = ensembleInterface->get_output_index(action_label);
    auto label_value = outputs[label_index];
    auto num_outputs = outputs.size();
    for (OutputIndex_type output_index = 0; output_index < num_outputs; ++output_index) {
        if (output_index == label_index) { continue; }
        if (not PLAJA_FLOATS::gte(label_value, outputs[output_index], tolerance)) { return false; }
    }
    return true;
}

ActionLabel_type EnsemblePolicy::evaluate_app_filtering(const StateBase& state, const std::vector<Floating_type>& outputs, std::vector<bool>& enabled_labels) const {
    PLAJA_ASSERT(enabled_labels.empty())
    // first sort in DEC order
    auto num_outputs = outputs.size();
    std::vector<std::pair<Floating_type, int>> sorted_outputs;
    sorted_outputs.reserve(num_outputs);
    enabled_labels.reserve(num_outputs);
    for (OutputIndex_type output_index = 0; output_index < num_outputs; ++output_index) {
        sorted_outputs.emplace_back(outputs[output_index], output_index);
        enabled_labels.push_back(successorGenerator->is_applicable(state, ensembleInterface->get_output_label(output_index))); // also cache enabled labels
    }
    std::sort(sorted_outputs.begin(), sorted_outputs.end(), outputSort);
    // now iterate
    for (const auto& value_index: sorted_outputs) {
        ActionLabel_type action_label = ensembleInterface->get_output_label(value_index.second);
        if (enabled_labels[value_index.second]) { return action_label; }
    }
    // no applicable op
    return ensembleInterface->get_output_label(sorted_outputs.front().second); // fall back to no-filtering
}

bool EnsemblePolicy::is_chosen_app_filtering(const std::vector<Floating_type>& outputs, const std::vector<bool>& enabled_labels, ActionLabel_type action_label, double tolerance) const {
    PLAJA_ASSERT(cachedLabel != ACTION::nullAction and action_label != cachedLabel)
    auto label_index = ensembleInterface->get_output_index(action_label);
    auto label_value = outputs[label_index];

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
}

// interface:

ActionLabel_type EnsemblePolicy::evaluate(const StateBase& state) const {
    cachedOutputs.clear();
    cachedEnabledLabels.clear();
    chosenWithTolerance = false;
    evaluate_trees(state, cachedOutputs); // evaluate NN
    return cachedLabel = ( ensembleInterface->_do_applicability_filtering() ? evaluate_app_filtering(state, cachedOutputs, cachedEnabledLabels) : evaluate_argmax(cachedOutputs) ); // select action label
}

std::vector<double> EnsemblePolicy::action_values(const StateBase& state) const {
    std::vector<Floating_type> inputs;
    inputs.reserve(state.size());
    for (auto state_index: inputFeatures) { 
        inputs.push_back(state[state_index]);
    }
    auto num_outputs = ensembleInterface->get_num_output_features();
    std::vector<double> outputs;
    outputs.resize(num_outputs);
    veritas::data d {&inputs[0], 1, inputs.size(), inputs.size(), 1};
    veritas::data<double> outdata(outputs);
    veritas_tree.eval(d.row(0), outdata);
    for (int o = 0; o < num_outputs; ++o) {
        outputs[o] = outdata[o];
    }
    return outputs;
}

bool EnsemblePolicy::is_chosen(const StateBase& state, ActionLabel_type action_label, double tolerance) const { evaluate(state); return is_chosen(action_label, tolerance); }

bool EnsemblePolicy::is_chosen(ActionLabel_type action_label, double tolerance) const {
    chosenWithTolerance = false;

    // exact ?
    if (not ensembleInterface->is_learned(action_label) or action_label == cachedLabel) { return true; }

    // tolerance?
    return ( tolerance > 0 and ( chosenWithTolerance = (ensembleInterface->_do_applicability_filtering() ? is_chosen_app_filtering(cachedOutputs, cachedEnabledLabels, action_label, tolerance) : is_chosen_argmax(cachedOutputs, action_label, tolerance)) ) );
}

#ifndef NDEBUG
#include <sstream>
void EnsemblePolicy::dump_input_features(const StateBase& state) const {
    std::stringstream ss;
    for (auto state_index: inputFeatures) { ss << state[state_index] << PLAJA_UTILS::commaString << PLAJA_UTILS::spaceString; }
    PLAJA_LOG(ss.str())
}

void EnsemblePolicy::dump_cached_outputs() const {
    std::stringstream ss;
    for (auto val: cachedOutputs) { ss << val << PLAJA_UTILS::commaString; }
    ss << PLAJA_UTILS::spaceString << cachedLabel;
    PLAJA_LOG(ss.str());
}

#endif
