//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PA_ENTAILMENT_COMPOSED_H
#define PLAJA_PA_ENTAILMENT_COMPOSED_H

#include "pa_entailment_values.h"

class PaEntailmentComposed final: public PaEntailmentBase {

private:
    const PaEntailmentValues* values1;
    const PaEntailmentValues* values2;

public:
    PaEntailmentComposed(const PaEntailmentValues& values1, const PaEntailmentValues& values2);
    ~PaEntailmentComposed() override;
    DELETE_CONSTRUCTOR(PaEntailmentComposed)

    /******************************************************************************************************************/

    [[nodiscard]] bool is_entailed(PredicateIndex_type pred_index) const override;

    [[nodiscard]] PA::EntailmentValueType get_value(PredicateIndex_type pred_index) const override;

};

#endif //PLAJA_PA_ENTAILMENT_COMPOSED_H
