//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_MODEL_Z3_STRUCTURES_H
#define PLAJA_MODEL_Z3_STRUCTURES_H

#include <memory>
#include <vector>
#include <z3++.h>
#include "../../../parser/using_parser.h"
#include "../../../assertions.h"
#include "../using_smt.h"
#include "model_z3_forward_declarations.h"


/**
 * Z3 representation of a destination.
 * Related to a Z3 model and its context.
 */
struct DestinationZ3 {
    friend class ModelZ3;
private:
    const Destination& destination;
    std::vector<z3::expr> assignments;
    std::vector<VariableID_type> assigned_ids;
    std::vector<std::unique_ptr<z3::expr>> array_indexes; // pointer to z3::expr in case of an array access, nullptr if not
    explicit DestinationZ3(const Destination& destination_);
public:
    ~DestinationZ3();
    DELETE_CONSTRUCTOR(DestinationZ3)
    [[nodiscard]] inline const Destination& _destination() const { return destination; }
    [[nodiscard]] inline std::size_t size() const { return assignments.size(); }
    [[nodiscard]] inline const z3::expr& get_assignment(std::size_t index) const { PLAJA_ASSERT( index < assignments.size()) return assignments[index]; }
    [[nodiscard]] inline VariableID_type get_assigned_id(std::size_t index) const { PLAJA_ASSERT( index < assigned_ids.size()) return assigned_ids[index]; }
    [[nodiscard]] inline const z3::expr* get_array_index(std::size_t index) const { PLAJA_ASSERT( index < array_indexes.size()) return array_indexes[index].get(); }
};


/**
 * Z3 representation of an edge.
 * Related to a Z3 model and its context.
 */
struct EdgeZ3 {
    friend class ModelZ3;
private:
    const Edge& edge;
    EdgeID_type edgeID;
    z3::context& context;

    std::unique_ptr<z3::expr> guard;
    std::vector<std::unique_ptr<DestinationZ3>> destinations;
    explicit EdgeZ3(const Edge& edge_, z3::context& c, std::unique_ptr<z3::expr>&& guard_);
public:
    ~EdgeZ3();
    DELETE_CONSTRUCTOR(EdgeZ3)

    [[nodiscard]] inline const Edge& _edge() const { return edge; }
    [[nodiscard]] inline EdgeID_type get_id() const { return edgeID; }
    [[nodiscard]] inline z3::context& get_context() const { return context; }

    [[nodiscard]] inline const z3::expr* get_guard() const { return guard.get(); }
    [[nodiscard]] inline std::size_t size() const { return destinations.size(); }
    [[nodiscard]] inline const DestinationZ3* get_destination(std::size_t index) const { PLAJA_ASSERT(index < destinations.size()) return destinations[index].get(); }
};


/**
 * Z3 representation of an automaton.
 * Related to a Z3 model and its context.
 */
struct AutomatonZ3 {
    friend class ModelZ3;
private:
    std::vector<std::unique_ptr<EdgeZ3>> edges;
    AutomatonZ3();
public:
    ~AutomatonZ3();
    DELETE_CONSTRUCTOR(AutomatonZ3)
    [[nodiscard]] inline std::size_t get_number_edges() const { return edges.size(); }
    [[nodiscard]] inline const EdgeZ3* get_edge(std::size_t index) const { PLAJA_ASSERT( index < edges.size()) return edges[index].get(); }
};


#endif //PLAJA_MODEL_Z3_STRUCTURES_H
