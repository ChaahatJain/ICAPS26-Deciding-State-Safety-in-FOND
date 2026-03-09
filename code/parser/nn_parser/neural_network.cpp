//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <sstream>
#include "neural_network.h"
#include "../../exception/parser_exception.h"
#include "../../utils/utils.h"


/**
 * Reimplementation (of a subset) of Marabou's AcasNeuralNetwork
 * specifically https://github.com/NeuralNetworkVerification/Marabou/blob/019f16633c3b18999e253d0a325e92ed8cda3c1f/src/input_parsers/AcasNeuralNetwork.cpp
 * (April 2022).
 */

#define PARSER_ERROR_IF(CONDITION) if (CONDITION) { throw ParserException("Parsing " + path + " as NNet failed."); }
#define CHECK_EOF PARSER_ERROR_IF(file_str.eof())
#define GET_LINE(FILE_STR, LINE) CHECK_EOF std::getline(FILE_STR, LINE);

PLAJA::NeuralNetwork::NeuralNetwork(const std::string& path) {
    auto file_str = PLAJA_UTILS::file_to_ss(path);

    std::string line;

    // skip header lines
    bool header_line = true;
    while (header_line) {
        GET_LINE(file_str, line)
        header_line = (line.find("//") == 0);
    }
    // size parameters
    auto sizes = PLAJA_UTILS::split(line, PLAJA_UTILS::commaString);
    PARSER_ERROR_IF(sizes.size() != 4)
    auto num_layers = std::stoi(sizes[0]); // excluding input layer
    auto input_size = std::stoi(sizes[1]);
    // auto output_size = std::stoi(sizes[2]);
    // auto may_layer_size = std::stoi(sizes[3]);

    // layer sizes
    GET_LINE(file_str, line)
    layerSizes.reserve(num_layers + 1);
    for (const auto& layer_size: PLAJA_UTILS::split(line, PLAJA_UTILS::commaString)) {
        layerSizes.push_back(std::stoi(layer_size));
    }
    PARSER_ERROR_IF(layerSizes.size() != num_layers + 1)


    // unused flag
    GET_LINE(file_str, line)

    // min values
    GET_LINE(file_str, line)
    min.reserve(input_size);
    for (const auto& min_val: PLAJA_UTILS::split(line, PLAJA_UTILS::commaString)) { min.push_back(std::stoi(min_val)); }
    PARSER_ERROR_IF(min.size() != input_size)

    // max values
    GET_LINE(file_str, line)
    max.reserve(input_size);
    for (const auto& max_val: PLAJA_UTILS::split(line, PLAJA_UTILS::commaString)) { max.push_back(std::stoi(max_val)); }
    PARSER_ERROR_IF(max.size() != input_size)

    // means
    GET_LINE(file_str, line)
    means.reserve(input_size + 1); // + 1 for outputs
    for (const auto& mean: PLAJA_UTILS::split(line, PLAJA_UTILS::commaString)) { means.push_back(std::stoi(mean)); }
    PARSER_ERROR_IF(means.size() != input_size + 1)

    // ranges
    GET_LINE(file_str, line)
    ranges.reserve(input_size + 1); // + 1 for outputs
    for (const auto& range: PLAJA_UTILS::split(line, PLAJA_UTILS::commaString)) { ranges.push_back(std::stoi(range)); }
    PARSER_ERROR_IF(ranges.size() != input_size + 1)

    // layers
    weights.resize(num_layers);
    biases.resize(num_layers);

    for (auto layer = 0; layer < num_layers; ++layer) {
        // weights
        weights[layer].resize(getLayerSize(layer + 1));
        for (auto target_neuron = 0; target_neuron < getLayerSize(layer + 1); ++target_neuron) {
            GET_LINE(file_str, line)
            auto weight_per_src_neuron = PLAJA_UTILS::split(line, PLAJA_UTILS::commaString);
            PARSER_ERROR_IF(weight_per_src_neuron.size() != getLayerSize(layer))
            weights[layer][target_neuron].reserve(getLayerSize(layer));
            for (const auto& weight: weight_per_src_neuron) {
                weights[layer][target_neuron].push_back(std::stod(weight));
            }
        }

        // biases
        biases[layer].reserve(getLayerSize(layer + 1));
        for (auto target_neuron = 0; target_neuron < getLayerSize(layer + 1); ++target_neuron) {
            GET_LINE(file_str, line)
            biases[layer].push_back(std::stod(line));
        }
    }

    // EOF
    if (!file_str.eof()) {
        GET_LINE(file_str, line)
        PARSER_ERROR_IF(!line.empty())
    }
    PARSER_ERROR_IF(!file_str.eof())
}

PLAJA::NeuralNetwork::~NeuralNetwork() = default;

//

void PLAJA::NeuralNetwork::evaluate(const std::vector<double>& inputs, std::vector<double>& outputs, unsigned int output_size) const {
    PLAJA_ASSERT(inputs.size() == min.size())
    PLAJA_ASSERT(inputs.size() == max.size())
    PLAJA_ASSERT(inputs.size() == means.size() - 1)
    PLAJA_ASSERT(inputs.size() == ranges.size() - 1)
    PLAJA_ASSERT(output_size == getLayerSize(getNumLayers()))

    std::vector<double> input_vector; // used for propagation trough network, i.e., vector size will change
    input_vector.reserve(inputs.size());
    for (auto input: inputs) { input_vector.push_back(input); }

    bool normalize_input = false;
    bool normalize_output = false;

    /** AcasNnet inline ************************/

    if (normalize_input) { // normalize input
        for (auto i = 0; i < inputs.size(); i++) {
            if (input_vector[i] > max[i]) {
                input_vector[i] = (max[i] - means[i]) / ranges[i];
            } else if (input_vector[i] < min[i]) {
                input_vector[i] = (min[i] - means[i]) / ranges[i];
            } else {
                input_vector[i] = (input_vector[i] - means[i]) / ranges[i];
            }
        }
    }

    for (auto src_layer = 0; src_layer < getNumLayers(); src_layer++) {
        //
        auto src_layer_size = getLayerSize(src_layer);
        auto target_layer = src_layer + 1;
        auto target_layer_size = getLayerSize(target_layer);
        std::vector<double> output_vector(target_layer_size, 0.0);
        //
        for (auto target_neuron = 0; target_neuron < target_layer_size; target_neuron++) {

            // Weighted sum:
            for (auto src_neuron = 0; src_neuron < src_layer_size; src_neuron++) {
                output_vector[target_neuron] += input_vector[src_neuron] * getWeight(src_layer, src_neuron, target_neuron);
            }
            output_vector[target_neuron] += getBias(target_layer, target_neuron);

            // ReLU (for hidden layers)
            if (target_layer < getNumLayers()) { output_vector[target_neuron] = std::max(0.0, output_vector[target_neuron]); }
        }

        std::swap(input_vector, output_vector); // output of current as input to next layer
    }

    /*********************************/

    for (auto i = 0; i < output_size; i++) {
        outputs.push_back(normalize_output ? input_vector[i] * ranges.back() + means.back() : input_vector[i]);
    }
}

double PLAJA::NeuralNetwork::computeDiffSum(const std::vector<double>& inputs, const std::vector<double>& outputs_ref) const {
    PLAJA_ASSERT(not outputs_ref.empty())

    const auto num_outputs = outputs_ref.size();
    std::vector<double> outputs;
    evaluate(inputs, outputs, num_outputs);

    PLAJA_ASSERT(outputs.size() == outputs_ref.size())

    double diff = 0;
    for (std::size_t output_index = 0; output_index < num_outputs; ++output_index) {
        diff += std::abs(outputs[output_index] - outputs_ref[output_index]);
    }

    return diff;
}
