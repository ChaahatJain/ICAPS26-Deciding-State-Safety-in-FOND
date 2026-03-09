//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SUCCESSOR_GENERATOR_PA_H
#define PLAJA_SUCCESSOR_GENERATOR_PA_H

#include "successor_generator_abstract.h"
#include "../smt/forward_smt_pa.h"
#include "../forward_pa.h"
#include "../using_predicate_abstraction.h"
#include "forward_succ_gen_pa.h"

class SuccessorGeneratorPA: public SuccessorGeneratorAbstract {

private:
    std::unique_ptr<PredicateDependencyGraph> predDepGraph;
    PredicatesNumber_type lastNumPreds; // Number of predicates at last increment.

    void initialize_action_ops_pa();

    SuccessorGeneratorPA(const ModelZ3& model_z3, const PLAJA::Configuration& config);
public:
    ~SuccessorGeneratorPA() override;
    static std::unique_ptr<SuccessorGeneratorPA> construct(const ModelZ3PA& model_z3, const PLAJA::Configuration& config); // hack to allow splitting construction in derived classes

    DELETE_CONSTRUCTOR(SuccessorGeneratorPA)

    [[nodiscard]] const ModelZ3PA& get_model_z3_pa() const;

    void increment();

    [[nodiscard]] const ActionOpPA& get_action_op_pa(ActionOpID_type action_op_id) const;

    /* Iterators. */

    [[nodiscard]] inline auto init_silent_action_it_pa() const { return ActionOpIteratorBase<ActionOpPA, ActionOpAbstract*, std::list>(silentOps); }

    [[nodiscard]] inline auto init_labeled_action_it_pa(ActionLabel_type action_label) const {
        PLAJA_ASSERT(0 <= action_label and action_label < labeledOps.size())
        return ActionOpIteratorBase<ActionOpPA, ActionOpAbstract*, std::list>(labeledOps[action_label]);
    }

    [[nodiscard]] inline auto init_action_it_pa() const { return ActionLabelIteratorBase<ActionOpPA, ActionOpAbstract*, std::list>(silentOps, labeledOps); }

};

/**********************************************************************************************************************/

/* Extern for interfacing. */
namespace SUCCESSOR_GENERATOR_PA {
    [[nodiscard]] extern std::list<const ActionOpPA*> extract_ops_per_label(const SuccessorGeneratorPA& successor_generator, ActionLabel_type action_label);
};

#endif //PLAJA_SUCCESSOR_GENERATOR_PA_H
