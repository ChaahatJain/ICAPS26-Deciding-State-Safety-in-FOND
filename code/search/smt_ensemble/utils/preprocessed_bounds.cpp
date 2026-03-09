//
// This file is part of the PlaJA code base (2019 - 2021).
//

#include "preprocessed_bounds.h"
#include "../../../include/marabou_include/exceptions.h"
#include "../../../include/marabou_include/preprocessor.h"
#include "../../../include/compiler_config_const.h"

namespace MARABOU_IN_PLAJA {

    void inform_constraints_of_initial_bounds(InputQuery& query_to_preprocess) {
        for (const auto& pl_constraint: query_to_preprocess.getPiecewiseLinearConstraints()) {
            for (unsigned int variable: pl_constraint->getParticipatingVariables()) {
                pl_constraint->notifyLowerBound(variable, query_to_preprocess.getLowerBound(variable));
                pl_constraint->notifyUpperBound(variable, query_to_preprocess.getUpperBound(variable));
            }
        }
    }

    std::unique_ptr<PreprocessedBounds> PreprocessedBounds::preprocess_bounds(InputQuery& input_query) {
        try {
            Preprocessor preprocessor;
            inform_constraints_of_initial_bounds(input_query);
            auto preprocessed_query = preprocessor.preprocess(input_query, false);
            // collect bounds
            auto num_vars = input_query.getNumberOfVariables();
            Map<MarabouVarIndex_type, Pair<MarabouFloating_type, MarabouFloating_type>> bounds;
            for (MarabouVarIndex_type var = 0; var < num_vars; ++var) {
                // handling merging
                auto new_index = var;
                while (preprocessor.variableIsMerged(new_index)) { new_index = preprocessor.getMergedIndex(new_index); }
                // handling fixing
                if (preprocessor.variableIsFixed(new_index)) {
                    bounds[var] = { preprocessor.getFixedValue(new_index), preprocessor.getFixedValue(new_index) };
                    continue;
                }
                //
                new_index = preprocessor.getNewIndex(new_index);
                bounds[var] = { preprocessed_query->getLowerBound(new_index), preprocessed_query->getUpperBound(new_index) };
            }
            //
            return std::make_unique<PreprocessedBounds>(std::move(bounds));

        } catch (const InfeasibleQueryException&) { return nullptr; }
    }

}
