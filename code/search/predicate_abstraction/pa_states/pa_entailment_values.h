//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PA_ENTAILMENT_VALUES_H
#define PLAJA_PA_ENTAILMENT_VALUES_H

#include <forward_list>
#include "../../../utils/utils.h"
#include "pa_entailment_base.h"

// #define USE_FINE_GRAINED_ENTAILMENT_STACK // Only cache the predicates modified per stack layer.

#ifdef USE_FINE_GRAINED_ENTAILMENT_STACK
namespace PLAJA_GLOBAL { constexpr bool useFineGrainedEntailmentStack = true; }
#else
namespace PLAJA_GLOBAL { constexpr bool useFineGrainedEntailmentStack = false; }
#endif

class PaEntailmentValues final: public PaEntailmentBase {

private:
    /* Entailment values. */
    std::vector<PA::EntailmentValueType> entailmentValues;

    /* Stack. */
#ifdef USE_FINE_GRAINED_ENTAILMENT_STACK
    std::forward_list<std::forward_list<PredicateIndex_type>> stack;
#else
    std::forward_list<std::vector<PA::EntailmentValueType>> stack;
#endif

public:
    PaEntailmentValues(const PredicatesExpression* predicates, std::size_t num_preds);
    ~PaEntailmentValues() override;
    DELETE_CONSTRUCTOR(PaEntailmentValues)

    /******************************************************************************************************************/

    /* Getter. */

    [[nodiscard]] bool is_entailed(PredicateIndex_type pred_index) const override;

    [[nodiscard]] PA::EntailmentValueType get_value(PredicateIndex_type pred_index) const override;

    /* Setter. */

    void set(PredicateIndex_type pred_index, bool value);

    /** Shall only be called if predicate is *defined*. */
    void unset(PredicateIndex_type pred_index);

    /** stack interface ***********************************************************************************************/

    void push_layer();

    void reset_layer();

    void pop_layer();

    /******************************************************************************************************************/

    // template<bool CONST, typename PredicateState_t>
    class Iterator final: public PaEntailmentBase::Iterator {
        friend PaEntailmentValues;

    private:

        explicit Iterator(const PaEntailmentValues& entailment_state):
            PaEntailmentBase::Iterator(entailment_state, 0) {
        }

    public:
        ~Iterator() override = default;
        DELETE_CONSTRUCTOR(Iterator)

        [[nodiscard]] inline const PaEntailmentValues& cast() const { return PLAJA_UTILS::cast_ref<PaEntailmentValues>(PaEntailmentBase::Iterator::operator()()); }

        // [[nodiscard]] inline typename std::enable_if<is_const, void>::type set(PredicateIndex_type pred_index, bool value) { predicateState.set(pred_index, value); }

    };

    [[nodiscard]] inline Iterator init_iterator_pa_entailment_values() const { return Iterator(*this); }

};

#endif //PLAJA_PA_ENTAILMENT_VALUES_H
