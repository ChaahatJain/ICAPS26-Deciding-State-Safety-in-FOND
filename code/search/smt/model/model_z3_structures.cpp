//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "model_z3_structures.h"
#include <z3++.h>
#include "../../../parser/ast/edge.h"

DestinationZ3::DestinationZ3(const Destination& destination_): destination(destination_) {}

DestinationZ3::~DestinationZ3() = default;

//

EdgeZ3::EdgeZ3(const Edge& edge_, z3::context& c, std::unique_ptr<z3::expr>&& guard_): edge(edge_), context(c), guard(std::move(guard_)) {
    edgeID = edge_.get_id(); // cached for easier access
}

EdgeZ3::~EdgeZ3() = default;

//

AutomatonZ3::AutomatonZ3() = default;

AutomatonZ3::~AutomatonZ3() = default;