//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "nn_in_z3.h"
#include "z3++.h"
#include "../../../parser/nn_parser/neural_network.h"
#include "../../../utils/index_iterator.h"
#include "../solver/smt_solver_z3.h"
#include "../context_z3.h"

NNInZ3::NeuronInZ3::NeuronInZ3():
    var(Z3_IN_PLAJA::noneVar)
    , aux(Z3_IN_PLAJA::noneVar)
    , binary(Z3_IN_PLAJA::noneVar) {}

NNInZ3::NeuronInZ3::NeuronInZ3(VariableID_type var_):
    var(var_)
    , aux(Z3_IN_PLAJA::noneVar)
    , binary(Z3_IN_PLAJA::noneVar) {}

NNInZ3::NeuronInZ3::NeuronInZ3(Z3_IN_PLAJA::Context& c, LayerIndex_type layer, NeuronIndex_type neuron, const std::string& postfix, Encoding mode):
    var(c.add_real_var("nn_" + std::to_string(layer) + "_" + std::to_string(neuron) + "_" + postfix))
    , aux(mode != N1V ? c.add_real_var("nn_aux_" + std::to_string(layer) + "_" + std::to_string(neuron) + "_" + postfix) : Z3_IN_PLAJA::noneVar)
    , binary(mode == MIP ? c.add_int_var("nn_var_" + std::to_string(layer) + "_" + std::to_string(neuron) + "_binary_" + postfix) : Z3_IN_PLAJA::noneVar) {
}

const z3::expr& NNInZ3::NeuronInZ3::get_var(const Z3_IN_PLAJA::Context& c) const { return c.get_var(var); }

const z3::expr& NNInZ3::NeuronInZ3::get_aux(const Z3_IN_PLAJA::Context& c) const {
    PLAJA_ASSERT(aux != Z3_IN_PLAJA::noneVar)
    return c.get_var(aux);
}

const z3::expr& NNInZ3::NeuronInZ3::get_binary(const Z3_IN_PLAJA::Context& c) const {
    PLAJA_ASSERT(binary != Z3_IN_PLAJA::noneVar)
    return c.get_var(binary);
}

/**********************************************************************************************************************/

NNInZ3::NeuronInZ3::~NeuronInZ3() = default;

NNInZ3::NNInZ3(const PLAJA::NeuralNetwork& nn_, Z3_IN_PLAJA::Context& c, const std::vector<Z3_IN_PLAJA::VarId_type>& input_vars, const std::string& postfix):
    context(&c)
    , nn(nn_)
    , mode(N1V) {
    auto num_layers = nn.getNumLayers() + 1;
    PLAJA_ASSERT(num_layers >= 3) // at least one input, one hidden and one output layer
    neuronToVar.resize(num_layers);
    ITERATE_INDEX(layer, size, 0, num_layers, neuronToVar[layer].reserve(nn.getLayerSize(layer));)
    neuronToZ3.resize(num_layers);
    ITERATE_INDEX(layer, size, 0, num_layers, neuronToZ3[layer].reserve(nn.getLayerSize(layer));)

    PLAJA_ASSERT(input_vars.size() == nn.getLayerSize(0) and not input_vars.empty())

    // compute variables:

    // input layer:
    for (const auto& input_var: input_vars) { neuronToVar[0].emplace_back(input_var); }

    // hidden layer:
    ITERATE_INDEX(layer, num_hidden_layers, 1, num_layers - 1, {
        ITERATE_INDEX(neuron, layer_size, 0, nn.getLayerSize(layer), neuronToVar[layer].emplace_back(*context, layer, neuron, postfix, mode);)
    })

    // output layer:
    const LayerIndex_type output_layer_index = nn.getNumLayers();
    ITERATE_INDEX(neuron, size, 0, nn.getLayerSize(nn.getNumLayers()), {
        neuronToVar[output_layer_index].emplace_back(*context, output_layer_index, neuron, postfix, N1V);
    })

    // compute forward structure:
    ITERATE_INDEX(layer, num_hidden_layers, 1, num_layers - 1, {
        ITERATE_INDEX(neuron, layer_size, 0, nn.getLayerSize(layer), {
            switch (mode) {

                case N1V: {
                    // na_var = ITE(sum >= 0, sum, 0)
                    neuronToZ3[layer].push_back(std::make_unique<z3::expr>(get_neuron(layer, neuron).get_var(*context) == z3::max(to_sum(layer, neuron), context->int_val(0))));
                    break;
                }

                case N2V: {
                    // nn_var_aux = sum + bias, na_var = ITE(nn_var_aux >= 0, nn_var_aux, 0)
                    const auto& var = get_neuron(layer, neuron);
                    const auto& aux_z3 = var.get_aux(*context);
                    neuronToZ3[layer].push_back(std::make_unique<z3::expr>(aux_z3 == to_sum(layer, neuron) && get_neuron(layer, neuron).get_var(*context) == z3::max(aux_z3, context->int_val(0))));
                    break;
                }

                case MIP: {
                    // bounded MIP encoding (Valera et al., Scaling Guarantees for Nearest Counterfactual Explanations, 2020):
                    /* const auto& var = get_var(layer, neuron);
                    neuronToZ3[layer].push_back( var._aux() = to_sum(layer, neuron)
                                            && 0 <= var._binary()
                                            && var._binary() <= 1
                                            && 0 <= var._var()
                                            && var._aux() <= var._var()
                                            && var._var() <= Z3_TO_REAL(c, upper_bound) * var._binary()
                                            && var._var() <= (var._aux() - Z3_TO_REAL(c, lower_bound) * (1 - var._binary()))); */
                    PLAJA_ABORT // TODO
                }

            }
        })
    })

    // output layer
    ITERATE_INDEX(neuron, layer_size, 0, nn.getLayerSize(output_layer_index), {
        neuronToZ3[output_layer_index].push_back(std::make_unique<z3::expr>(get_neuron(output_layer_index, neuron).get_var(*context) == to_sum(output_layer_index, neuron)));
    })

}

NNInZ3::~NNInZ3() = default;

z3::expr NNInZ3::to_sum(LayerIndex_type target_layer, NeuronIndex_type target, bool include_bias) {
    PLAJA_ASSERT(target_layer >= 1)
    const auto src_layer = target_layer - 1;
    z3::expr sum = context->real_val(nn.getWeight(src_layer, 0, target), Z3_IN_PLAJA::floatingPrecision) * get_neuron(src_layer, 0).get_var(*context);
    ITERATE_INDEX(source, source_layer_size, 1, nn.getLayerSize(src_layer), {
        sum = sum + context->real_val(nn.getWeight(src_layer, source, target), Z3_IN_PLAJA::floatingPrecision) * get_neuron(src_layer, source).get_var(*context);
    })
    return include_bias ? sum + context->real_val(nn.getBias(target_layer, target), Z3_IN_PLAJA::floatingPrecision) : sum;
}

void NNInZ3::add(Z3_IN_PLAJA::SMTSolver& solver) const {

    const auto num_layers = get_num_layers();
    for (auto layer_index = 1; layer_index < num_layers; ++layer_index) {

        const auto layer_size = get_layer_size(layer_index);

        for (auto neuron_index = 0; neuron_index < layer_size; ++neuron_index) {

            solver.add(get_neuron_z3(layer_index, neuron_index));

        }

    }

}

#ifndef NDEBUG

void NNInZ3::dump() const {

    std::cout << "Dumping NN" << std::endl;

    const auto num_layers = get_num_layers();
    for (auto layer_index = 1; layer_index < num_layers; ++layer_index) {

        const auto layer_size = get_layer_size(layer_index);

        for (auto neuron_index = 0; neuron_index < layer_size; ++neuron_index) {
            std::cout << get_neuron_z3(layer_index, neuron_index) << std::endl;
        }

    }

}

#endif
