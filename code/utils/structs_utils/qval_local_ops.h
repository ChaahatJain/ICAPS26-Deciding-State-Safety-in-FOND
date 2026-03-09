//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_QVAL_LOCAL_OPS_H
#define PLAJA_QVAL_LOCAL_OPS_H

#include <utility>
#include <vector>
#include "../../search/using_search.h"

struct QValLocalOps {
    QValue_type qValue;
    LocalOpIndex_type localOpIndex;
    std::vector<LocalOpIndex_type> localOpIndexes; // local operators greedy with respect to qValue

    QValLocalOps(): qValue(0), localOpIndex(ACTION::noneLocalOp), localOpIndexes() {}
    QValLocalOps(const QValLocalOp& q_value_op, std::vector<LocalOpIndex_type> local_op_indexes):
        qValue(q_value_op.qValue), localOpIndex(q_value_op.localOpIndex), localOpIndexes(std::move(local_op_indexes)) {}
    ~QValLocalOps() = default;
};

#endif //PLAJA_QVAL_LOCAL_OPS_H
