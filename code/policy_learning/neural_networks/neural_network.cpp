//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <fstream>
#include "neural_network.h"
#include "../../parser/nn_parser/neural_network.h"
#include "../../utils/utils.h"
#include "../../assertions.h"

TORCH_IN_PLAJA::NeuralNetwork::NeuralNetwork(std::vector<unsigned int> layers, double reductionThreshold, std::vector<TORCH_IN_PLAJA::Floating> lower_bounds, std::vector<TORCH_IN_PLAJA::Floating> upper_bounds, bool use_batch_norms):
    torch::nn::Module()
    , layerSizes(std::move(layers))
    , useBatchNorms(use_batch_norms)
    , lowerBounds(lower_bounds)
    , upperBounds(upper_bounds)
    , reductionThreshold(reductionThreshold) {

    auto num_layers = layerSizes.size();

    PLAJA_ASSERT(num_layers >= 3) // Input, output + at least one hidden layer.
    inputSize = layerSizes[0];
    outputSize = layerSizes[num_layers - 1];
    totalPasses = 0;
    PLAJA_ASSERT(inputSize == lowerBounds.size())
    PLAJA_ASSERT(inputSize == upperBounds.size())

    /* Initialize forwarding functions. */
    for (std::size_t i = 0; i < num_layers - 1; ++i) {
        auto pre_layer_size = layerSizes[i];
        auto post_layer_size = layerSizes[i + 1];
        std::vector<int64_t> row_indices;
        for (unsigned j = 0; j < post_layer_size; ++j) {
            row_indices.push_back(0);
        }
        zeroFrequencies.push_back(row_indices);
        std::stringstream oss;
        if (i != 0) {
            oss << "Hidden layer " << i << ";" << " Size: " << layerSizes[i];
            layerDesc.push_back(oss.str());
            oss.clear();
        }
        oss.clear();
        forwardingFuncs.emplace_back(register_module("forwarding_" + std::to_string(i) + "_" + std::to_string(i + 1), torch::nn::Linear(pre_layer_size, post_layer_size)));
        if (useBatchNorms && i != num_layers - 2) batchNorms.emplace_back(register_module("batch_norm_" + std::to_string(i) + "_" + std::to_string(i + 1), torch::nn::BatchNorm1d(post_layer_size)));
    }
    zeroFrequencies.pop_back();

}

TORCH_IN_PLAJA::NeuralNetwork::~NeuralNetwork() = default;

/**********************************************************************************************************************/

void TORCH_IN_PLAJA::NeuralNetwork::resetTracking() {
    totalPasses = 0;
    zeroFrequencies.clear();
    for (std::size_t i = 0; i < layerSizes.size() - 1; ++i) {
        auto post_layer_size = layerSizes[i + 1];
        std::vector<int64_t> row_indices;
        for (unsigned j = 0; j < post_layer_size; ++j) {
            row_indices.push_back(0);
        }
        zeroFrequencies.push_back(row_indices);
    }
    zeroFrequencies.pop_back();
}

/**********************************************************************************************************************/

std::vector<std::string> TORCH_IN_PLAJA::NeuralNetwork::get_LayerDesc() const {
    std::vector<std::string> copy;
    copy.reserve(layerDesc.size());
    for (const auto& desc : layerDesc) {
        copy.push_back(desc);
    }
    return copy;
}

unsigned int TORCH_IN_PLAJA::NeuralNetwork::get_totalPasses() const {
    return totalPasses;
}

std::vector<std::vector<int64_t>> TORCH_IN_PLAJA::NeuralNetwork::get_zeroFrequencies() {
    std::vector<std::vector<int64_t>> copy;
    copy.reserve(zeroFrequencies.size());
    for (const std::vector<int64_t>& inner_vec : zeroFrequencies) {
        copy.push_back(inner_vec);
    }
    return copy;
}

/**********************************************************************************************************************/

bool TORCH_IN_PLAJA::NeuralNetwork::isNeuronZeroed(unsigned int layer, unsigned int row) {
    if ((forwardingFuncs[layer]->weight[row].nonzero().size(0) == 0)
        && (forwardingFuncs[layer]->bias[row].nonzero().size(0) == 0)) {
        for (unsigned int i = 0; i < forwardingFuncs[layer + 1]->weight.size(0); ++i) {
            if (forwardingFuncs[layer + 1]->weight[i][row].nonzero().size(0) > 0) {
                return false;
            }
        }
        return true;
    } else {
        return false;
    }
}

void TORCH_IN_PLAJA::NeuralNetwork::setNeuronToZero(unsigned int layer, unsigned int row, bool compact) {
    if (compact) {
        torch::Tensor weight = forwardingFuncs[layer]->weight;
        torch::Tensor bias = forwardingFuncs[layer]->bias;
        torch::Tensor next_weight = forwardingFuncs[layer + 1]->weight;
        torch::Tensor top;
        torch::Tensor bottom;

        torch::Tensor new_weight;
        torch::Tensor new_bias;
        torch::Tensor new_next_weight;
        if ((row + 1) >= weight.size(0)) {
            new_weight = weight.slice(0, 0, row);
            new_bias = bias.slice(0, 0, row);
            new_next_weight = next_weight.slice(1, 0, row);
        } else {
            top = weight.slice(0, 0, row);
            bottom = weight.slice(0, row + 1, weight.size(0));
            new_weight = torch::cat({top, bottom}, 0);

            top = bias.slice(0, 0, row);
            bottom = bias.slice(0, row + 1, bias.size(0));
            new_bias = torch::cat({top, bottom}, 0);

            top = next_weight.slice(1, 0, row);
            bottom = next_weight.slice(1, row + 1, next_weight.size(1));
            new_next_weight = torch::cat({top, bottom}, 1);
        }

        layerSizes[layer + 1] -= 1;
        forwardingFuncs[layer] = replace_module("forwarding_" + std::to_string(layer) + "_" + std::to_string(layer + 1), torch::nn::Linear(layerSizes[layer], layerSizes[layer + 1]));
        forwardingFuncs[layer]->weight = new_weight.clone();
        forwardingFuncs[layer]->bias = new_bias.clone();

        if ((layer + 2) < layerSizes.size()) {
            torch::Tensor old_bias = forwardingFuncs[layer + 1]->bias.clone();
            forwardingFuncs[layer + 1] = replace_module("forwarding_" + std::to_string(layer + 1) + "_" + std::to_string(layer + 2), torch::nn::Linear(layerSizes[layer + 1], layerSizes[layer + 2]));
            forwardingFuncs[layer + 1]->bias = old_bias;
            forwardingFuncs[layer + 1]->weight = new_next_weight.clone();
        }
        zeroFrequencies[layer].erase(zeroFrequencies[layer].begin() + row);
    } else {
        forwardingFuncs[layer]->weight[row].data() = torch::zeros(forwardingFuncs[layer]->weight[row].size(0));
        forwardingFuncs[layer]->bias[row].data() = 0;
        for (unsigned int i = 0; i < forwardingFuncs[layer + 1]->weight.size(0); ++i) {
            forwardingFuncs[layer + 1]->weight[i][row].data() = 0;
        }
    }
}

/**********************************************************************************************************************/

torch::Tensor TORCH_IN_PLAJA::NeuralNetwork::forward(torch::Tensor x) {
    totalPasses += 1;
    std::size_t output_forwarding = forwardingFuncs.size() - 1;
    torch::Tensor temp;
    for (std::size_t i = 0; i < output_forwarding; ++i) {
        temp = useBatchNorms ? batchNorms[i]->forward(forwardingFuncs[i]->forward(x)) : forwardingFuncs[i]->forward(x);
        x = torch::relu(temp);
        if (x.dim() > 1) {
            ;
        } else {
            for (int64_t j = 0; j < x.size(0); ++j) {
                if (x[j].item<float>() <= reductionThreshold) {
                    zeroFrequencies.at(i).at(j) += 1;
                }
            }
        }
    }
    PLAJA_ASSERT(!forwardingFuncs.empty())
    return forwardingFuncs[output_forwarding]->forward(x);
}

/**********************************************************************************************************************/

void TORCH_IN_PLAJA::NeuralNetwork::load_from_nnet(const std::string& file_name) {
    torch::NoGradGuard no_grad; // Simulates enclosing with:
    // torch::autograd::GradMode::set_enabled(false);
    // torch::autograd::GradMode::set_enabled(true);

    PLAJA::NeuralNetwork nn_nnet(file_name);
    PLAJA_ASSERT(nn_nnet.getNumLayers() == forwardingFuncs.size()) // input layer is not counted
    auto src_layer_index = 0;
    auto target_layer_index = 1;

    for (auto layer_fun: forwardingFuncs) {

        auto src_layer_size = nn_nnet.getLayerSize(src_layer_index);
        auto target_layer_size = nn_nnet.getLayerSize(target_layer_index);

        for (auto target_neuron_index = 0; target_neuron_index < target_layer_size; ++target_neuron_index) {

            /* Bias. */
            layer_fun->bias[target_neuron_index] = nn_nnet.getBias(target_layer_index, target_neuron_index);

            /* Weights. */
            for (auto src_neuron_index = 0; src_neuron_index < src_layer_size; ++src_neuron_index) {
                layer_fun->weight[target_neuron_index][src_neuron_index] = nn_nnet.getWeight(src_layer_index, src_neuron_index, target_neuron_index);
            }
        }

        ++src_layer_index;
        ++target_layer_index;
    }

}

/**********************************************************************************************************************/

namespace TORCH_IN_PLAJA_NEURAL_NETWORK {

    void print_weight(std::ofstream& nnet_file, const torch::Tensor& weight) {
        for (int64_t row_i = 0; row_i < weight.size(0); ++row_i) {
            for (int64_t col_i = 0; col_i < weight.size(1); ++col_i) {
                nnet_file << weight[row_i][col_i].item<TORCH_IN_PLAJA::Floating>() << PLAJA_UTILS::commaString;
            }
            nnet_file << PLAJA_UTILS::lineBreakString;
        }
    }

    void print_bias(std::ofstream& nnet_file, const torch::Tensor& bias) {
        for (int64_t i = 0; i < bias.size(0); ++i) {
            nnet_file << bias[i].item<TORCH_IN_PLAJA::Floating>() << PLAJA_UTILS::commaString << PLAJA_UTILS::lineBreakString;
        }
    }

}

void TORCH_IN_PLAJA::NeuralNetwork::to_nnet(const std::string& file_name) {
    std::ofstream nnet_file;
    nnet_file.open(file_name, std::ios::trunc);
    // nnet_file << "// The contents of this file are licensed under the Creative Commons" << PLAJA_UTILS::lineBreakString;
    // nnet_file << "// Attribution 4.0 International License: https://creativecommons.org/licenses/by/4.0/" << PLAJA_UTILS::lineBreakString;
    nnet_file << "// Neural Network File Format by Kyle Julian, Stanford 2016" << PLAJA_UTILS::lineBreakString;

    /* #layers (in terms of #forwarding), #inputs, #outputs, maximum layer size (including input layer; output layer anyway). */
    nnet_file << forwardingFuncs.size()
              << PLAJA_UTILS::commaString << inputSize
              << PLAJA_UTILS::commaString << outputSize
              << PLAJA_UTILS::commaString << *std::max_element(layerSizes.begin(), layerSizes.end()) << PLAJA_UTILS::commaString << PLAJA_UTILS::lineBreakString;
    /* Network layer sizes. */
    for (auto layer_size: layerSizes) { nnet_file << layer_size << PLAJA_UTILS::commaString; }
    nnet_file << PLAJA_UTILS::lineBreakString;
    /* Deprecated flag. */
    nnet_file << 0 << PLAJA_UTILS::commaString << PLAJA_UTILS::lineBreakString;

    /* Lower bounds per input. */
    if (lowerBounds.empty()) {
        for (std::size_t i = 0; i < inputSize; ++i) { nnet_file << -std::numeric_limits<double>::max() << PLAJA_UTILS::commaString; }
    } else {
        for (auto lower_bound: lowerBounds) { nnet_file << lower_bound << PLAJA_UTILS::commaString; }
    }
    nnet_file << PLAJA_UTILS::lineBreakString;

    /* Upper bounds per output. */
    if (lowerBounds.empty()) {
        for (std::size_t i = 0; i < inputSize; ++i) { nnet_file << std::numeric_limits<double>::max() << PLAJA_UTILS::commaString; }
    } else {
        for (auto upper_bound: upperBounds) { nnet_file << upper_bound << PLAJA_UTILS::commaString; }
    }
    nnet_file << PLAJA_UTILS::lineBreakString;

    /* Means (inputs + one entry for outputs). */
    for (std::size_t i = 0; i <= inputSize; ++i) { nnet_file << 0 << PLAJA_UTILS::commaString; }
    nnet_file << PLAJA_UTILS::lineBreakString;
    /* Ranges (inputs + one entry for outputs). */
    for (std::size_t i = 0; i <= inputSize; ++i) { nnet_file << 1 << PLAJA_UTILS::commaString; }
    nnet_file << PLAJA_UTILS::lineBreakString;

    /* Forwarding weights and biases. */
    nnet_file << std::fixed << std::setprecision(8);
    for (const auto& forwarding: forwardingFuncs) {
        TORCH_IN_PLAJA_NEURAL_NETWORK::print_weight(nnet_file, forwarding->weight);
        TORCH_IN_PLAJA_NEURAL_NETWORK::print_bias(nnet_file, forwarding->bias);
    }

}

void TORCH_IN_PLAJA::NeuralNetwork::dump() const {
    const std::size_t l = forwardingFuncs.size();
    for (std::size_t i = 0; i < l; ++i) {
        std::cout << "Forwarding " << i << " to " << (i + 1) << ": " << PLAJA_UTILS::lineBreakString;
        std::cout << forwardingFuncs[i]->weight << PLAJA_UTILS::lineBreakString;
        std::cout << forwardingFuncs[i]->bias << PLAJA_UTILS::lineBreakString;
    }
}

// Useful:
// neural_network.named_parameters();
// torch::Tensor tensor = torch::rand({2, 3});
// auto options = torch::TensorOptions()
//                .requires_grad(true);
