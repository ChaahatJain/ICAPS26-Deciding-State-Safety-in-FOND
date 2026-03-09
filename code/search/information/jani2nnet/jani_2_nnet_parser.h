//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_JANI_2_NNET_PARSER_H
#define PLAJA_JANI_2_NNET_PARSER_H

#include <json_fwd.hpp>
#include "using_jani2nnet.h"
#include "forward_jani_2_nnet.h"

class Model;

class Jani2NNetParser {
private:
    const Model& model;
    std::string jani2NNetFile; // file loaded
    std::unique_ptr<nlohmann::json> loadedJSON; // may be an invalid json document if load_file fails

    [[nodiscard]] AutomatonIndex_type parse_automaton_instance(const nlohmann::json& input) const;
    [[nodiscard]] std::pair<AutomatonIndex_type, EdgeID_type> parse_edge(const nlohmann::json& input) const;

    [[nodiscard]] ActionOpID_type parse_action_operator_structure(const nlohmann::json& input) const;

    [[nodiscard]] std::unique_ptr<InputFeatureToJANI> parse_InputFeatureToJANI(const nlohmann::json& input) const;
    [[nodiscard]] std::unique_ptr<InputFeatureToJANI> parse_InputFeatureToLocation(const nlohmann::json& input) const;
    [[nodiscard]] std::unique_ptr<InputFeatureToJANI> parse_InputFeatureToValueVariable(const nlohmann::json& input) const;
    [[nodiscard]] std::unique_ptr<InputFeatureToJANI> parse_InputFeatureToArrayVariable(const nlohmann::json& input) const;

    [[nodiscard]] std::string get_nn_file() const;
    [[nodiscard]] std::unique_ptr<std::string> get_torch_file() const;
    [[nodiscard]] bool applicability_filtering_enabled() const;
    [[nodiscard]] std::vector<std::unique_ptr<InputFeatureToJANI>> parse_input_interface() const;
    [[nodiscard]] std::vector<unsigned int> parse_hidden_layer_structure() const;
    [[nodiscard]] std::vector<ActionLabel_type> parse_output_interface() const;

    [[nodiscard]] nlohmann::json input_feature_to_json(const InputFeatureToJANI& input_feature) const;

public:
    explicit Jani2NNetParser(const Model& model_);
    ~Jani2NNetParser();

    std::unique_ptr<Jani2NNet> load_file(const std::string& nn_interface); // return value to indicate success
    void dump_nn_interface(const Jani2NNet& nn_interface);

};

namespace JANI_2_NNET {

    extern std::unique_ptr<Jani2NNet> load_nn_interface(const Model& model, const std::string& nn_interface);
    extern void dump_nn_interface(const Jani2NNet& nn_interface);

}

#endif //PLAJA_JANI_2_NNET_PARSER_H
