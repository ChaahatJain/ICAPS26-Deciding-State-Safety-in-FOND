//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SOLUTION_MARABOU_H
#define PLAJA_SOLUTION_MARABOU_H

#include <memory>
#include <unordered_map>
#include "../../../include/ct_config_const.h"
#include "../using_marabou.h"
#include "../forward_smt_nn.h"

namespace MARABOU_IN_PLAJA {

    struct Solution final {
    private:
        const MARABOU_IN_PLAJA::MarabouQuery* query;
        std::unordered_map<MarabouVarIndex_type, MarabouFloating_type> assignment;
        std::unordered_map<MarabouVarIndex_type, MarabouFloating_type> clippedAssignment;

    public:
        explicit Solution(const MARABOU_IN_PLAJA::MarabouQuery& query_);
        ~Solution();
        Solution(const Solution& other) = delete;
        Solution(Solution&& other) = default;
        DELETE_ASSIGNMENT(Solution)

        /* Clipping. */

        [[nodiscard]] inline bool clipped_out_of_bounds() const { return PLAJA_GLOBAL::marabouHandleNumericIssues and not clippedAssignment.empty(); }

        [[nodiscard]] inline bool is_clipped(MarabouVarIndex_type var) const { return PLAJA_GLOBAL::marabouHandleNumericIssues and clippedAssignment.count(var); }

        [[nodiscard]] inline MarabouFloating_type get_clipped_value(MarabouVarIndex_type var) const {
            PLAJA_ASSERT(PLAJA_GLOBAL::marabouHandleNumericIssues and clippedAssignment.count(var))
            return clippedAssignment.at(var);
        }

        /**/

        void set_value(MarabouVarIndex_type var, MarabouFloating_type val);

        /* To be used for repeatedly adapting a solution object, e.g., for naive solution enumeration. */
        inline void assign(MarabouVarIndex_type var, MarabouFloating_type val) { assignment[var] = val; }

        [[nodiscard]] inline MarabouFloating_type get_value(MarabouVarIndex_type var) const {
            PLAJA_ASSERT(assignment.count(var))
            return assignment.at(var);
        }

        [[nodiscard]] inline std::unordered_map<MarabouVarIndex_type, MarabouFloating_type>::const_iterator begin() const { return assignment.cbegin(); }

        [[nodiscard]] inline std::unordered_map<MarabouVarIndex_type, MarabouFloating_type>::const_iterator end() const { return assignment.cend(); }

        [[nodiscard]] inline bool is_integer() const;

        [[nodiscard]] std::unique_ptr<::MarabouConstraint> generate_solution_conjunction() const;

        void add_solution_conjunction(MARABOU_IN_PLAJA::QueryConstructable& query_) const;

        [[nodiscard]] std::unique_ptr<::MarabouConstraint> generate_solution_exclusion() const;

        void add_solution_exclusion(MARABOU_IN_PLAJA::QueryConstructable& query_) const;

        FCT_IF_DEBUG(void dump() const;)
    };

}

#endif //PLAJA_SOLUTION_MARABOU_H
