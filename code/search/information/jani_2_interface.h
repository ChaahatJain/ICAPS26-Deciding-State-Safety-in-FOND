//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_JANI_2_INTERFACE_H
#define PLAJA_JANI_2_INTERFACE_H
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "../../parser/ast/expression/forward_expression.h"
#include "../../parser/ast/forward_ast.h"
#include "../../assertions.h"
#include "../factories/forward_factories.h"
#include "../non_prob_search/policy/forward_policy.h"
#include "../states/forward_states.h"
#include "forward_information.h"
#include "input_feature_to_jani.h"
#include "input_feature_to_jani_derived.h"
#include "model_information.h"
#include "../../parser/ast/model.h"
#include "jani2nnet/using_jani2nnet.h"


/**
 * This class documents the interface between a JANI model and an underlying policy stored in .nnet or.json format.
 * The interface is dependent on the current implementation of JANI models, \ie, the used indexes and ids.
 */
class Jani2Interface {
public:
    const Model* model; // underlying model
    std::string interfaceFile;
    std::string policyFile;
    bool doApplicabilityFiltering;
    std::vector<unsigned int> layerSizes;
    std::vector<unsigned int> hiddenLayerSizes;
    std::vector<std::unique_ptr<InputFeatureToJANI>> inputFeatureMapping; // For applicability filtering any variable occurring in a guard is to be considered an input to the policy (inputFeatureMapping.size() >= layerSizes[0]).
    std::vector<ActionLabel_type> outputsToLabels;
    std::unordered_map<ActionLabel_type, OutputIndex_type> labelsToOutputs;
    std::vector<ActionLabel_type> unlearnedLabels;

public:
    virtual ~Jani2Interface() = default;  // Make the destructor virtual

    virtual std::unique_ptr<Jani2Interface> copy_interface(const std::string& path_to_directory, const std::string* filename_prefix = nullptr, bool dump = true) const = 0;
    virtual const Policy& load_policy(const PLAJA::Configuration& config) const = 0;
    
    FCT_IF_DEBUG(virtual ActionLabel_type eval_policy(const StateBase& state) const = 0;)
    FCT_IF_DEBUG(virtual bool is_chosen(const StateBase& state, ActionLabel_type action_label) const = 0;)

    [[nodiscard]] inline const Model& get_model() const {return *model; }

    [[nodiscard]] inline const ModelInformation& get_model_info() const {return model->get_model_information(); }

    [[nodiscard]] inline const std::string& get_interface_file() const { return interfaceFile; }

    virtual inline const std::string& get_policy_file() const = 0;

    [[nodiscard]] inline bool _do_applicability_filtering() const {return doApplicabilityFiltering; }

    /* Input: */

    inline std::size_t get_num_input_features() const { return layerSizes[0]; }

    [[nodiscard]] inline const InputFeatureToJANI* get_input_feature(InputIndex_type index) const {
        PLAJA_ASSERT(index < inputFeatureMapping.size())
        return inputFeatureMapping[index].get();
    }

    [[nodiscard]] inline VariableIndex_type get_input_feature_index(InputIndex_type index) const { return get_input_feature(index)->stateIndex; }

    [[nodiscard]] inline const LValueExpression* get_input_feature_expression(InputIndex_type index) const { return get_input_feature(index)->get_expression(); }

    /* Output: */

    [[nodiscard]] inline std::size_t get_num_output_features() const { return outputsToLabels.size(); };

    [[nodiscard]] inline ActionLabel_type get_output_label(OutputIndex_type index) const {
        PLAJA_ASSERT(index < outputsToLabels.size())
        return outputsToLabels[index];
    }

    [[nodiscard]] inline const std::vector<ActionLabel_type>& get_unlearned_labels() const { return unlearnedLabels; }

    [[nodiscard]] inline bool is_learned(ActionLabel_type action_label) const { return labelsToOutputs.count(action_label); } // silent-labeled operators should not be learned

    [[nodiscard]] inline OutputIndex_type get_output_index(ActionLabel_type action_label) const {
        PLAJA_ASSERT(is_learned(action_label))
        return labelsToOutputs.at(action_label);
    }

/**********************************************************************************************************************/

class InputFeatureIterator {
        friend Jani2Interface;
    private:
        const Jani2Interface* interface;
        InputIndex_type currentIndex;
        std::size_t numInputs;

        explicit InputFeatureIterator(const Jani2Interface& j_interface, InputIndex_type start = 0, bool extended = false):
            interface(&j_interface)
            , currentIndex(start)
            , numInputs(extended ? interface->inputFeatureMapping.size() : interface->layerSizes[0]) {}

    public:
        ~InputFeatureIterator() = default;
        DELETE_CONSTRUCTOR(InputFeatureIterator)

        inline void operator++() { ++currentIndex; }

        [[nodiscard]] inline bool end() const { return currentIndex >= numInputs; }

        [[nodiscard]] inline InputIndex_type _input_index() const { return currentIndex; }

        [[nodiscard]] inline const InputFeatureToJANI* get_input_feature() const { return interface->get_input_feature(currentIndex); }

        [[nodiscard]] inline VariableIndex_type get_input_feature_index() const { return interface->get_input_feature_index(currentIndex); }

        [[nodiscard]] inline const LValueExpression* get_input_feature_expression() const { return interface->get_input_feature_expression(currentIndex); }
    };

    [[nodiscard]] inline InputFeatureIterator init_input_feature_iterator(bool extended = false) const { return InputFeatureIterator(*this, 0, extended); }

    [[nodiscard]] inline InputFeatureIterator init_input_feature_iterator_extended() const { return InputFeatureIterator(*this, get_num_input_features(), true); }

    class OutputFeatureIterator {
        friend Jani2Interface;
    private:
        const Jani2Interface* interface;
        OutputIndex_type currentIndex;

        explicit OutputFeatureIterator(const Jani2Interface& j_interface):
            interface(&j_interface)
            , currentIndex(0) {}

    public:
        ~OutputFeatureIterator() = default;
        DELETE_CONSTRUCTOR(OutputFeatureIterator)

        inline void operator++() { ++currentIndex; }

        [[nodiscard]] inline bool end() const { return currentIndex >= interface->get_num_output_features(); }

        [[nodiscard]] inline OutputIndex_type _output_index() const { return currentIndex; }

        [[nodiscard]] inline ActionLabel_type get_output_label() const { return interface->get_output_label(currentIndex); }

        [[nodiscard]] inline bool is_learned() const { return interface->is_learned(get_output_label()); }
    };

    [[nodiscard]] inline OutputFeatureIterator init_output_feature_iterator() const { return OutputFeatureIterator(*this); }

};


#endif //PLAJA_JANI_2_INTERFACE_H
