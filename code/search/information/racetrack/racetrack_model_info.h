//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_RACETRACK_MODEL_INFO_H
#define PLAJA_RACETRACK_MODEL_INFO_H


#include <unordered_set>
#include <vector>
#include "../../../parser/using_parser.h"
#include "../../using_search.h"

// forward declaration:
class Model;
class VariableDeclaration;

class RacetrackModelInfo {
private:
    const Model& model;
    std::vector<ActionID_type> nonDeterministicLabels;  // the action (labels) in the choice point of the model, i.e., the acceleration
    std::vector<ActionOpID_type> nonDeterministicOps; // the corresponding operators
    std::unordered_set<ActionID_type> nonDeterministicLabelsSet;

    // variables of interest
    const VariableDeclaration* xPosVar;
    const VariableDeclaration* yPosVar;
    const VariableDeclaration* xVelVar;
    const VariableDeclaration* yVelVar;

    [[nodiscard]] EdgeID_type extract_edge_id(AutomatonIndex_type automaton_index, ActionID_type action_id) const;

public:
    explicit RacetrackModelInfo(const Model& model);
    ~RacetrackModelInfo();

    [[nodiscard]] inline const Model& get_model() const { return model; }
    [[nodiscard]] inline const VariableDeclaration* _x_pos_var() const { return xPosVar; }
    [[nodiscard]] inline const VariableDeclaration* _y_pos_var() const { return yPosVar; }
    [[nodiscard]] inline const VariableDeclaration* _x_vel_var() const { return xVelVar; }
    [[nodiscard]] inline const VariableDeclaration* _y_vel_var() const { return yVelVar; }

    [[nodiscard]] inline const std::vector<ActionID_type>& _non_deterministic_labels() const { return nonDeterministicLabels; }
    [[nodiscard]] inline const std::vector<ActionOpID_type>& _non_deterministic_ops() const { return nonDeterministicOps; }
    [[nodiscard]] inline bool is_non_deterministic_label(ActionID_type action_id) const { return nonDeterministicLabelsSet.count(action_id); }

};


#endif //PLAJA_RACETRACK_MODEL_INFO_H
