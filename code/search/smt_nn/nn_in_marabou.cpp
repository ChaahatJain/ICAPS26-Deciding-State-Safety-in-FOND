//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "nn_in_marabou.h"
#include "../../parser/nn_parser/neural_network.h"
#include "../../utils/index_iterator.h"
#include "../../utils/utils.h"
#include "marabou_context.h"
#include "marabou_query.h"

namespace MARABOU_IN_PLAJA {

    struct NeuronInMarabou {
    protected:
        MarabouVarIndex_type marabouVar; // i.e., the input variables for hidden layer neurons
    public:
        explicit NeuronInMarabou(MarabouVarIndex_type marabou_var):
            marabouVar(marabou_var) {}

        virtual ~NeuronInMarabou() = default;
        DELETE_CONSTRUCTOR(NeuronInMarabou)

        [[nodiscard]] inline MarabouVarIndex_type _marabou_var() const { return marabouVar; }

        [[nodiscard]] MarabouVarIndex_type _neuron_input_var() const { return _marabou_var(); }

        [[nodiscard]] virtual MarabouVarIndex_type _neuron_output_var() const { return _marabou_var(); }
    };

    struct InputNeuronInMarabou final: public NeuronInMarabou {
    public:
        explicit InputNeuronInMarabou(MarabouVarIndex_type marabou_var):
            NeuronInMarabou(marabou_var) {}

        ~InputNeuronInMarabou() override = default;
        DELETE_CONSTRUCTOR(InputNeuronInMarabou)
    };

    struct WeightedInputNeuron: public NeuronInMarabou {
    private:
        Equation inputSum;
    public:
        explicit WeightedInputNeuron(MarabouVarIndex_type marabou_var):
            NeuronInMarabou(marabou_var)
            , inputSum(Equation::EQ) { inputSum.addAddend(-1, marabou_var); }

        ~WeightedInputNeuron() override = default;
        DELETE_CONSTRUCTOR(WeightedInputNeuron)

        [[nodiscard]] const Equation& _weighted_input_sum() const { return inputSum; }

        inline void set_bias(MarabouFloating_type bias) { inputSum.setScalar(-bias); }

        inline void add_weighted_input(MarabouVarIndex_type input_var, MarabouFloating_type weight) { inputSum.addAddend(weight, input_var); }
    };

    struct OutputNeuronInMarabou final: public WeightedInputNeuron {
    public:
        explicit OutputNeuronInMarabou(MarabouVarIndex_type marabou_var):
            WeightedInputNeuron(marabou_var) {}

        ~OutputNeuronInMarabou() override = default;
        DELETE_CONSTRUCTOR(OutputNeuronInMarabou)
    };

    struct HiddenNeuronInMarabou final: public WeightedInputNeuron {
    private:
        MarabouVarIndex_type outputVar;
    public:
        HiddenNeuronInMarabou(MarabouVarIndex_type input_var, MarabouVarIndex_type output_var):
            WeightedInputNeuron(input_var)
            , outputVar(output_var) {}

        ~HiddenNeuronInMarabou() override = default;
        DELETE_CONSTRUCTOR(HiddenNeuronInMarabou)

        [[nodiscard]] MarabouVarIndex_type _neuron_output_var() const override { return outputVar; }

        [[nodiscard]] inline ReluConstraint* create_activation_constraint() const { return new ReluConstraint(_neuron_input_var(), _neuron_output_var()); }
    };

}

/**********************************************************************************************************************/

NNInMarabou::NNInMarabou(const PLAJA::NeuralNetwork& nn_, MARABOU_IN_PLAJA::Context& c, const std::vector<MarabouVarIndex_type>* input_vars):
    nn(nn_)
    , context(c) {
    auto num_layers = nn.getNumLayers() + 1; // including input
    PLAJA_ASSERT(num_layers >= 3) // at least one input, one hidden and one output layer
    const auto input_size = nn.getLayerSize(0);
    // const auto output_size = nn.getLayerSize(num_layers - 1);
    PLAJA_ASSERT(not input_vars or input_vars->size() == input_size)

    std::vector<std::size_t> layer_sizes;
    layer_sizes.reserve(num_layers);
    for (LayerIndex_type layer = 0; layer < num_layers; ++layer) { layer_sizes.push_back(nn.getLayerSize(layer)); }

    // std::size_t num_hidden_layer_neurons = 0;
    // for (LayerIndex_type layer = 1; layer < num_layers - 1; ++layer) { num_hidden_layer_neurons += nn.getLayerSize(layer); }
    // std::cout << "Number of layers: " << num_layers << ". Input layer size: " << input_size << ". Output layer size: " << output_size << ". Number of ReLUs: " << num_hidden_layer_neurons << PLAJA_UTILS::dotString << std::endl;
    // auto num_vars = input_size + (2 * num_hidden_layer_neurons) + output_size;
    // std::cout << "Total number of variables: " << num_vars << PLAJA_UTILS::dotString << std::endl;

    // reserve & resize data structures
    neuronToMarabou.resize(layer_sizes.size());
    ITERATE_INDEX(layer, size, 0, layer_sizes.size(), neuronToMarabou[layer].resize(layer_sizes[layer]);)
    varToNeuron.reserve(get_nn_size());


    // input layer
    if (input_vars) { // only have to add input variables to encoding info
        NeuronIndex_type neuron = 0;
        bool tightened_bounds = false;
        for (auto input_var: *input_vars) {
            PLAJA_ASSERT(context.is_marked_input(input_var))
            tightened_bounds = context.tighten_bounds(input_var, nn.getInputLowerBound(neuron), nn.getInputUpperBound(neuron)) or tightened_bounds;
            set_neuron_to_var(0, neuron++, std::make_unique<MARABOU_IN_PLAJA::InputNeuronInMarabou>(input_var));
        }
        PLAJA_LOG_IF(tightened_bounds, "Warning: bounds of network and model differ! Using tightest combination in Marabou context...")
    } else { // add input variables to context
        for (NeuronIndex_type neuron = 0; neuron < input_size; ++neuron) {
            auto marabou_var = context.add_var(nn.getInputLowerBound(neuron), nn.getInputUpperBound(neuron), false, true, false);
            set_neuron_to_var(0, neuron, std::make_unique<MARABOU_IN_PLAJA::InputNeuronInMarabou>(marabou_var));
        }
    }

    // hidden layer: output var = input var + layer size (following Marabou here)
    for (LayerIndex_type layer = 1; layer < num_layers - 1; ++layer) {
        auto layer_size = get_layer_size(layer);
        for (NeuronIndex_type neuron = 0; neuron < layer_size; ++neuron) {
            auto marabou_var = context.add_var();
            auto neuron_in_marabou = std::make_unique<MARABOU_IN_PLAJA::HiddenNeuronInMarabou>(marabou_var, marabou_var + layer_size);
            // weighted sum
            neuron_in_marabou->set_bias(nn.getBias(layer, neuron));
            auto src_layer = layer - 1;
            auto src_layer_size = get_layer_size(src_layer);
            for (NeuronIndex_type src_neuron = 0; src_neuron < src_layer_size; ++src_neuron) {
                neuron_in_marabou->add_weighted_input(get_neuron_in_marabou(src_layer, src_neuron)._neuron_output_var(), nn.getWeight(src_layer, src_neuron, neuron));
            }
            //
            set_neuron_to_var(layer, neuron, std::move(neuron_in_marabou));
        }
        for (NeuronIndex_type neuron = 0; neuron < layer_size; ++neuron) { context.add_var(0, FloatUtils::infinity(), false, false, false); } // "marabou_var_offset += layer_size", i.e., account for activation vars
    }

    // output layer
    {
        LayerIndex_type layer = num_layers - 1;
        auto layer_size = get_layer_size(layer);
        for (NeuronIndex_type neuron = 0; neuron < layer_size; ++neuron) {
            auto neuron_in_marabou = std::make_unique<MARABOU_IN_PLAJA::OutputNeuronInMarabou>(context.add_var(FloatUtils::negativeInfinity(), FloatUtils::infinity(), false, false, true));
            // weighted sum
            neuron_in_marabou->set_bias(nn.getBias(layer, neuron));
            auto src_layer = layer - 1;
            auto src_layer_size = get_layer_size(src_layer);
            for (NeuronIndex_type src_neuron = 0; src_neuron < src_layer_size; ++src_neuron) {
                neuron_in_marabou->add_weighted_input(get_neuron_in_marabou(src_layer, src_neuron)._neuron_output_var(), nn.getWeight(src_layer, src_neuron, neuron));
            }
            //
            set_neuron_to_var(layer, neuron, std::move(neuron_in_marabou));
        }
    }

}

NNInMarabou::~NNInMarabou() = default;

// getter:

std::size_t NNInMarabou::get_nn_size() const {
    std::size_t num_neurons = 0;
    for (const auto& layer: neuronToMarabou) { num_neurons += layer.size(); }
    return num_neurons;
}

MarabouVarIndex_type NNInMarabou::get_var(LayerIndex_type layer, NeuronIndex_type neuron) const { return get_neuron_in_marabou(layer, neuron)._marabou_var(); }

MarabouVarIndex_type NNInMarabou::get_activation_var(LayerIndex_type layer, NeuronIndex_type neuron) const {
    PLAJA_ASSERT(0 < layer and layer < get_num_layers() - 1)
    return PLAJA_UTILS::cast_ref<MARABOU_IN_PLAJA::HiddenNeuronInMarabou>(get_neuron_in_marabou(layer, neuron))._neuron_output_var();
}

// setter:

void NNInMarabou::set_neuron_to_var(LayerIndex_type layer, NeuronIndex_type neuron, std::unique_ptr<MARABOU_IN_PLAJA::NeuronInMarabou>&& neuron_in_marabou) {
    PLAJA_ASSERT(layer < get_num_layers() and neuron < get_layer_size(layer))
    varToNeuron[neuron_in_marabou->_marabou_var()] = std::pair<LayerIndex_type, NeuronIndex_type>(layer, neuron);
    neuronToMarabou[layer][neuron] = std::move(neuron_in_marabou);
}

void NNInMarabou::add_to_query(MARABOU_IN_PLAJA::MarabouQuery& query) const {
    PLAJA_ASSERT(context == query._context())
    auto num_layers = get_num_layers();

#ifndef NDEBUG // should have been loaded from context

    // input variables
    for (InputIndex_type neuron = 0; neuron < get_input_layer_size(); ++neuron) {
        auto neuron_in_marabou = get_neuron_in_marabou_cast<MARABOU_IN_PLAJA::InputNeuronInMarabou>(0, neuron);
        auto marabou_var = neuron_in_marabou->_marabou_var();
        PLAJA_ASSERT(query.exists(marabou_var))
        PLAJA_ASSERT(query.is_marked_input(marabou_var))
        PLAJA_ASSERT(not query.is_marked_output(marabou_var))
        PLAJA_ASSERT(nn.getInputLowerBound(neuron) <= query.get_lower_bound(marabou_var))
        PLAJA_ASSERT(query.get_upper_bound(marabou_var) <= nn.getInputUpperBound(neuron))
    }

    // Map hidden/output layer variables
    for (LayerIndex_type layer = 1; layer < num_layers; ++layer) {
        for (NeuronIndex_type neuron = 0; neuron < get_layer_size(layer); ++neuron) {
            auto neuron_in_marabou = get_neuron_in_marabou_cast<MARABOU_IN_PLAJA::NeuronInMarabou>(layer, neuron);
            auto marabou_var = neuron_in_marabou->_marabou_var();
            PLAJA_ASSERT(query.exists(marabou_var))
            PLAJA_ASSERT(not query.is_marked_input(marabou_var))
            PLAJA_ASSERT(not query.is_marked_output(marabou_var) or layer == num_layers - 1)
        }
        if (layer < num_layers - 1) { // for hidden layers add ReLU output var (i.e., output var = input var + layer size)
            for (NeuronIndex_type neuron = 0; neuron < get_layer_size(layer); ++neuron) {
                auto neuron_in_marabou = get_neuron_in_marabou_cast<MARABOU_IN_PLAJA::HiddenNeuronInMarabou>(layer, neuron);
                auto marabou_var = neuron_in_marabou->_neuron_output_var();
                PLAJA_ASSERT(query.exists(marabou_var))
                PLAJA_ASSERT(not query.is_marked_input(marabou_var))
                PLAJA_ASSERT(not query.is_marked_output(marabou_var))
                PLAJA_ASSERT(0 <= query.get_lower_bound(marabou_var))
            }
        }
    }

#endif

    auto& input_query = query._query();

    // Add liner constraints to query
    for (LayerIndex_type layer = 1; layer < num_layers; ++layer) {
        for (NeuronIndex_type neuron = 0; neuron < get_layer_size(layer); ++neuron) {
            input_query.addEquation(get_neuron_in_marabou_cast<MARABOU_IN_PLAJA::WeightedInputNeuron>(layer, neuron)->_weighted_input_sum());
        }
    }

    // Add (ReLU) activation constraints to query
    for (LayerIndex_type layer = 1; layer < num_layers - 1; ++layer) {
        for (NeuronIndex_type neuron = 0; neuron < get_layer_size(layer); ++neuron) {
            auto* relu_constraint = get_neuron_in_marabou_cast<MARABOU_IN_PLAJA::HiddenNeuronInMarabou>(layer, neuron)->create_activation_constraint();
            relu_constraint->setConstraintId(input_query.getPiecewiseLinearConstraints().size());
            input_query.addPiecewiseLinearConstraint(relu_constraint);
        }
    }

    // Mark output variables (loaded from context; asserted above)
    // ITERATE_INDEX(neuron, layer_size, 0, get_output_layer_size(), query.mark_output(get_output_var(neuron), neuron);)

}

#if 0
/**
 * Adaption of Marabou's InputQuery generation
 * specifically https://github.com/NeuralNetworkVerification/Marabou/blob/3536269a359153510fc14efcf0eedb77d88231d1/src/input_parsers/AcasParser.cpp
 * (June 2022).
 * @param nn
 * @param input_query
 * @param input_vars
 * @return
 */
std::unique_ptr<NNInMarabou> NNInMarabou::add_nn_to_query(const PLAJA::NeuralNetwork& nn, InputQuery& input_query, const std::vector<MarabouVarIndex_type>* input_vars) {
    const auto num_layers = nn.getNumLayers() + 1; // including input
    const auto input_size = nn.getLayerSize(0);
    const auto output_size = nn.getLayerSize(num_layers - 1);
    PLAJA_ASSERT(not input_vars or input_vars->size() == input_size)

    std::size_t num_hidden_layer_neurons = 0;
    for (LayerIndex_type layer = 1; layer < num_layers - 1; ++layer) { num_hidden_layer_neurons += nn.getLayerSize(layer); }
    // std::cout << "Number of layers: " << num_layers << ". Input layer size: " << input_size << ". Output layer size: " << output_size << ". Number of ReLUs: " << num_hidden_layer_neurons << PLAJA_UTILS::dotString << std::endl;

    auto num_vars = input_size + (2 * num_hidden_layer_neurons) + output_size;
    // std::cout << "Total number of variables: " << num_vars << PLAJA_UTILS::dotString << std::endl;

    //
    std::vector<std::size_t> layer_sizes;
    layer_sizes.reserve(num_layers);
    for (LayerIndex_type layer = 0; layer < num_layers; ++layer) { layer_sizes.push_back(nn.getLayerSize(layer)); }
    std::unique_ptr<NNInMarabou> nn_info(new NNInMarabou(layer_sizes));
    //
    MarabouVarIndex_type var_offset;
    if (input_vars) {
        STMT_IF_DEBUG(for (auto input_var: *input_vars) { PLAJA_ASSERT(input_query._variableToInputIndex.exists(input_var)) })
        var_offset = input_query.getNumberOfVariables();
        input_query.setNumberOfVariables(var_offset + num_vars - input_vars->size());
        // only have to add input variables to encoding info
        NeuronIndex_type neuron = 0;
        for (auto input_var: *input_vars) { nn_info->set_neuron_to_var(0, neuron++, input_var); }
    } else {
        var_offset = 0; // for uninitialized queries getNumberOfVariables() is invalid
        input_query.setNumberOfVariables(num_vars);
        // add input variables
        for (NeuronIndex_type neuron = 0; neuron < input_size; ++neuron) {
            input_query.markInputVariable(var_offset, input_query.getNumInputVariables());
            input_query.setLowerBound(var_offset, nn.getInputLowerBound(neuron));
            input_query.setUpperBound(var_offset, nn.getInputUpperBound(neuron));
            nn_info->set_neuron_to_var(0, neuron, var_offset++);
        }
    }

    // Map hidden/output layer variables
    for (LayerIndex_type layer = 1; layer < num_layers; ++layer) {
        ITERATE_INDEX(neuron, layer_size, 0, nn.getLayerSize(layer), {
            input_query.setLowerBound(var_offset, FloatUtils::negativeInfinity());
            input_query.setUpperBound(var_offset, FloatUtils::infinity());
            nn_info->set_neuron_to_var(layer, neuron, var_offset++);
        })
        if (layer < num_layers - 1) { // for hidden layers add ReLU output var (i.e., output var = input var + layer size)
            ITERATE_INDEX(neuron, layer_size, 0, nn.getLayerSize(layer), {
                PLAJA_ASSERT(nn_info->get_activation_var(layer, neuron) == var_offset)
                input_query.setLowerBound(var_offset, 0.0);
                input_query.setUpperBound(var_offset++, FloatUtils::infinity());
            })
        }
    }

    // Add liner constraints to query
    for (LayerIndex_type src_layer = 0; src_layer < num_layers - 1; ++src_layer) {
        auto target_layer = src_layer + 1;
        ITERATE_INDEX(target, target_layer_size, 0, nn.getLayerSize(target_layer), {
            Equation equation(Equation::EQ); //   weight_sum - target_neuron = -bias
            equation.setScalar(-nn.getBias(target_layer, target));
            equation.addAddend(-1, nn_info->get_var(target_layer, target));
            ITERATE_INDEX(source, source_layer_size, 0, nn.getLayerSize(src_layer), {
                    equation.addAddend(nn.getWeight(src_layer, source, target), src_layer > 0 ? nn_info->get_activation_var(src_layer, source) : nn_info->get_var(src_layer, source));
            })
            input_query.addEquation(std::move(equation));
        })
    }

    // Add (ReLU) activation constraints to query
    for (LayerIndex_type layer = 1; layer < num_layers - 1; ++layer) {
        ITERATE_INDEX(neuron, layer_size, 0, nn.getLayerSize(layer), {
            auto relu = new ReluConstraint(nn_info->get_var(layer, neuron), nn_info->get_activation_var(layer, neuron));
            input_query.addPiecewiseLinearConstraint(relu);
        })
    }

    // Mark output variables
    ITERATE_INDEX(neuron, layer_size, 0, nn.getLayerSize(num_layers - 1), input_query.markOutputVariable(nn_info->get_output_var(neuron), input_query.getNumOutputVariables());)

    return nn_info;
}
#endif
