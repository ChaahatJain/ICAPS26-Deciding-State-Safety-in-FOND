//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_QVAL_LOCAL_OP_H
#define PLAJA_QVAL_LOCAL_OP_H

#include "../../search/using_search.h"

struct QValLocalOp {
    QValue_type qValue;
    LocalOpIndex_type localOpIndex;

    explicit QValLocalOp(QValue_type q_value = 0, LocalOpIndex_type local_op_index = ACTION::noneLocalOp): qValue(q_value), localOpIndex(local_op_index) {}
    ~QValLocalOp() = default;
};

#endif //PLAJA_QVAL_LOCAL_OP_H
