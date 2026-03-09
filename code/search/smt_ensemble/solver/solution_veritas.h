//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_SOLUTION_VERITAS_H
#define PLAJA_SOLUTION_VERITAS_H

#include <memory>
#include <unordered_map>
#include "../using_veritas.h"
#include "../forward_smt_veritas.h"

namespace VERITAS_IN_PLAJA {

    struct Solution final {
    private:
        const VERITAS_IN_PLAJA::VeritasQuery* query;
        std::unordered_map<VeritasVarIndex_type, VeritasFloating_type> assignment;
        std::unordered_map<VeritasVarIndex_type, VeritasFloating_type> clippedAssignment;

    public:
        explicit Solution(const VERITAS_IN_PLAJA::VeritasQuery& query_);
        ~Solution();
        Solution(const Solution& other) = delete;
        Solution(Solution&& other) = default;
        DELETE_ASSIGNMENT(Solution)

        /* Clipping. */

        [[nodiscard]] inline bool clipped_out_of_bounds() const { return not clippedAssignment.empty(); }

        [[nodiscard]] inline bool is_clipped(VeritasVarIndex_type var) const { return clippedAssignment.count(var); }

        [[nodiscard]] inline VeritasFloating_type get_clipped_value(VeritasVarIndex_type var) const {
            PLAJA_ASSERT(clippedAssignment.count(var))
            return clippedAssignment.at(var);
        }

        /**/

        void set_value(VeritasVarIndex_type var, VeritasFloating_type val);

        /* To be used for repeatedly adapting a solution object, e.g., for naive solution enumeration. */
        inline void assign(VeritasVarIndex_type var, VeritasFloating_type val) { assignment[var] = val; }

        [[nodiscard]] inline VeritasFloating_type get_value(VeritasVarIndex_type var) const {
            PLAJA_ASSERT(assignment.count(var))
            return assignment.at(var);
        }

        [[nodiscard]] inline std::unordered_map<VeritasVarIndex_type, VeritasFloating_type>::const_iterator begin() const { return assignment.cbegin(); }

        [[nodiscard]] inline std::unordered_map<VeritasVarIndex_type, VeritasFloating_type>::const_iterator end() const { return assignment.cend(); }

        [[nodiscard]] inline bool is_integer() const;

        [[nodiscard]] std::unique_ptr<::VeritasConstraint> generate_solution_conjunction() const;

        void add_solution_conjunction(VERITAS_IN_PLAJA::QueryConstructable& query_) const;

        FCT_IF_DEBUG(void dump() const;)
    };

}

#endif //PLAJA_SOLUTION_VERITAS_H
