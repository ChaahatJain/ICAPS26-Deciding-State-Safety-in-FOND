//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PA_ENTAILMENT_BASE_H
#define PLAJA_PA_ENTAILMENT_BASE_H

#include <utility>
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../utils/default_constructors.h"
#include "../../../assertions.h"
#include "../using_predicate_abstraction.h"
#include "forward_pa_states.h"

class PaEntailmentBase {

private:
    const PredicatesExpression* predicates;
    const std::size_t numPredicates; // NOLINT(*-avoid-const-or-ref-data-members)

public:
    PaEntailmentBase(const PredicatesExpression* predicates, std::size_t num_preds);
    virtual ~PaEntailmentBase() = 0;
    DELETE_CONSTRUCTOR(PaEntailmentBase)

    /******************************************************************************************************************/

    [[nodiscard]] const PredicatesExpression* get_predicates() const { return predicates; }

    [[nodiscard]] const Expression& get_predicate(PredicateIndex_type pred_index) const;

    [[nodiscard]] inline std::size_t get_size_predicates() const { return numPredicates; }

    /******************************************************************************************************************/

    [[nodiscard]] virtual bool is_entailed(PredicateIndex_type pred_index) const = 0;

    [[nodiscard]] virtual PA::EntailmentValueType get_value(PredicateIndex_type pred_index) const = 0;

    [[nodiscard]] bool operator==(const PaEntailmentBase& other) const;

    [[nodiscard]] inline bool operator!=(const PaEntailmentBase& other) const { return not operator==(other); }

    /******************************************************************************************************************/

    class Iterator {
        friend PaEntailmentBase;

    private:
        const PaEntailmentBase* entailmentState;
        PredicateIndex_type predIndex;

    protected:
        explicit Iterator(const PaEntailmentBase& entailment_state, PredicateIndex_type start_index):
            entailmentState(&entailment_state)
            , predIndex(start_index) {
        }

    public:
        virtual ~Iterator() = default;
        DELETE_CONSTRUCTOR(Iterator)

        [[nodiscard]] inline const PaEntailmentBase& operator()() const { return *entailmentState; }

        inline void operator++() { ++predIndex; }

        [[nodiscard]] inline bool end() const { return predIndex >= entailmentState->get_size_predicates(); }

        [[nodiscard]] inline PredicateIndex_type predicate_index() const { return predIndex; }

        [[nodiscard]] const Expression& predicate() const { return entailmentState->get_predicate(predicate_index()); }

        [[nodiscard]] inline bool is_entailed() const { return entailmentState->is_entailed(predicate_index()); }

        [[nodiscard]] inline PA::EntailmentValueType get_value() const { return entailmentState->get_value(predicate_index()); }

    };

    [[nodiscard]] inline Iterator init_iterator() const { return Iterator(*this, 0); }

};

#endif //PLAJA_PA_ENTAILMENT_BASE_H
