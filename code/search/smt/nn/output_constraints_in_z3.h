//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_OUTPUT_CONSTRAINTS_IN_Z3_H
#define PLAJA_OUTPUT_CONSTRAINTS_IN_Z3_H

#include <memory>
#include <vector>
#include "../../../include/ct_config_const.h"
#include "../../../utils/default_constructors.h"
#include "../../../assertions.h"
#include "../../information/jani2nnet/forward_jani_2_nnet.h"
#include "../../information/jani2nnet/using_jani2nnet.h"
#include "../utils/forward_z3.h"
#include "../forward_smt_z3.h"
#include "../using_smt.h"

namespace Z3_IN_PLAJA {

    class OutputConstraints {
    private:
        const ModelZ3* model;

    protected:
        [[nodiscard]] inline const ModelZ3& get_model() const {
            PLAJA_ASSERT(model)
            return *model;
        }

        [[nodiscard]] const Jani2NNet& get_interface() const;

        [[nodiscard]] Z3_IN_PLAJA::Context& get_context() const;

        [[nodiscard]] const NNInZ3& get_nn_in_smt(StepIndex_type step) const;

        /* Auxiliary to compute output constraint equation. */
        [[nodiscard]] static z3::expr compute_eq(OutputIndex_type output_index, const z3::expr& output_var, OutputIndex_type other_output_index, const z3::expr& other_output_var);

    public:
        explicit OutputConstraints(const ModelZ3& model);
        virtual ~OutputConstraints();
        DELETE_CONSTRUCTOR(OutputConstraints)

#ifdef ENABLE_APPLICABILITY_FILTERING
        [[nodiscard]] static std::unique_ptr<OutputConstraints> construct(const ModelZ3& model);
#else
        [[nodiscard]] static std::unique_ptr<class OutputConstraintsNoFilter> construct(const ModelZ3& model);
#endif

        inline void generate_steps() {}

        [[nodiscard]] virtual std::unique_ptr<Z3_IN_PLAJA::ConstraintExpr> compute(ActionLabel_type action_label, StepIndex_type step) const = 0;

        virtual void add(Z3_IN_PLAJA::SMTSolver& solver, ActionLabel_type action_label, StepIndex_type step) const = 0;

    };

    class OutputConstraintsNoFilter final: public OutputConstraints {

    private:
        /* Cache for step=0. */
        std::vector<std::unique_ptr<z3::expr>> constraintsPerLabel;
        /* Internal. */
        [[nodiscard]] z3::expr compute_internal(OutputIndex_type output_index, StepIndex_type step) const;

    public:
        explicit OutputConstraintsNoFilter(const ModelZ3& model);
        ~OutputConstraintsNoFilter() override;
        DELETE_CONSTRUCTOR(OutputConstraintsNoFilter)

        [[nodiscard]] std::unique_ptr<Z3_IN_PLAJA::ConstraintExpr> compute(ActionLabel_type action_label, StepIndex_type step) const override;

        void add(Z3_IN_PLAJA::SMTSolver& solver, ActionLabel_type action_label, StepIndex_type step) const override;

    };

#ifdef ENABLE_APPLICABILITY_FILTERING

    class OutputConstraintsAppFilter final: public OutputConstraints {

    private:
        mutable const OpApplicability* opApp; // Quick fix to share operator applicability. See MARABOU_IN_PLAJA::QueryConstructable.

    public:
        explicit OutputConstraintsAppFilter(const ModelZ3& model);
        ~OutputConstraintsAppFilter() override;
        DELETE_CONSTRUCTOR(OutputConstraintsAppFilter)

        [[nodiscard]] std::unique_ptr<Z3_IN_PLAJA::ConstraintExpr> compute(ActionLabel_type action_label, StepIndex_type step) const override;

        void add(Z3_IN_PLAJA::SMTSolver& solver, ActionLabel_type action_label, StepIndex_type step) const override;

    };

#endif

}

#endif //PLAJA_OUTPUT_CONSTRAINTS_IN_Z3_H
