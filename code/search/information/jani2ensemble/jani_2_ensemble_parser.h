//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_JANI_2_ENSEMBLE_PARSER_H
#define PLAJA_JANI_2_ENSEMBLE_PARSER_H

#include <json_fwd.hpp>
#include "forward_jani_2_ensemble.h"
#include "using_jani2ensemble.h"
class Model;

class Jani2EnsembleParser {
private:
    const Model& model;
    std::string jani2EnsembleFile; // file loaded
    std::unique_ptr<nlohmann::json> loadedJSON; // may be an invalid json document if load_file fails

    [[nodiscard]] AutomatonIndex_type parse_automaton_instance(const nlohmann::json& input) const;
    [[nodiscard]] std::pair<AutomatonIndex_type, EdgeID_type> parse_edge(const nlohmann::json& input) const;

    [[nodiscard]] ActionOpID_type parse_action_operator_structure(const nlohmann::json& input) const;

    [[nodiscard]] std::unique_ptr<InputFeatureToJANI> parse_InputFeatureToJANI(const nlohmann::json& input) const;
    [[nodiscard]] std::unique_ptr<InputFeatureToJANI> parse_InputFeatureToLocation(const nlohmann::json& input) const;
    [[nodiscard]] std::unique_ptr<InputFeatureToJANI> parse_InputFeatureToValueVariable(const nlohmann::json& input) const;
    [[nodiscard]] std::unique_ptr<InputFeatureToJANI> parse_InputFeatureToArrayVariable(const nlohmann::json& input) const;

    [[nodiscard]] std::string get_ensemble_file() const;
    [[nodiscard]] bool applicability_filtering_enabled() const;
    [[nodiscard]] std::vector<std::unique_ptr<InputFeatureToJANI>> parse_input_interface() const;
    [[nodiscard]] std::vector<ActionLabel_type> parse_output_interface() const;

    [[nodiscard]] nlohmann::json input_feature_to_json(const InputFeatureToJANI& input_feature) const;

public:
    explicit Jani2EnsembleParser(const Model& model_);
    ~Jani2EnsembleParser();

    std::unique_ptr<Jani2Ensemble> load_file(const std::string& ensemble_interface); // return value to indicate success
    void dump_ensemble_interface(const Jani2Ensemble& ensemble_interface);

};

namespace JANI_2_ENSEMBLE {

    extern std::unique_ptr<Jani2Ensemble> load_ensemble_interface(const Model& model, const std::string& ensemble_interface);
    extern void dump_ensemble_interface(const Jani2Ensemble& ensemble_interface);

}

#endif //PLAJA_JANI_2_ENSEMBLE_PARSER_H
