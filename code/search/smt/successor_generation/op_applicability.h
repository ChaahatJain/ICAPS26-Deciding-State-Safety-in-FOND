//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_OP_APPLICABILITY_H
#define PLAJA_OP_APPLICABILITY_H

#include <memory>
#include <unordered_map>
#include "../../../utils/default_constructors.h"
#include "../../factories/forward_factories.h"
#include "../../using_search.h"

/**
 * Cache to be used at expansion time.
 * Cache applicability of operator guard given a set of SMT-constraints (usually a PA state).
 * To optimize applicability filtering.
 */
class OpApplicability final {

public:

    enum Applicability {
        Unknown,
        Infeasible,
        Feasible, /* Satisfiable, i.e., there exists at least one solution to SMT constraints such that op is applicable. */
        Entailed, /* Op is applicable under any solution. */
    };

private:
    std::unordered_map<ActionOpID_type, Applicability> opApp;
    bool checkEntailed;
    mutable std::unique_ptr<bool> selfApplicability; // Temporary field to specify whether one of the label guards is known to be entailed (or even added).

public:
    OpApplicability();
    ~OpApplicability();
    DELETE_CONSTRUCTOR(OpApplicability)

    static std::unique_ptr<OpApplicability> construct(const PLAJA::Configuration& config);

    [[nodiscard]] inline bool check_entailed() const { return checkEntailed; }

    inline void reset() { opApp.clear(); }

    [[nodiscard]] inline bool empty() { return opApp.empty(); }

    void set_applicability(ActionOpID_type op, Applicability value);

    inline void set_applicability(ActionOpID_type op, bool value) { set_applicability(op, value ? Applicability::Feasible : Applicability::Infeasible); }

    [[nodiscard]] Applicability get_applicability(ActionOpID_type op) const;

    [[nodiscard]] inline bool is_unknown(ActionOpID_type op) const { return get_applicability(op) == Applicability::Unknown; }

    [[nodiscard]] inline bool is_infeasible(ActionOpID_type op) const { return get_applicability(op) == Applicability::Infeasible; }

    [[nodiscard]] inline bool is_feasible(ActionOpID_type op) const {
        const auto app = get_applicability(op);
        return app == Applicability::Feasible or app == Applicability::Entailed;
    }

    [[nodiscard]] bool is_entailed(ActionOpID_type op) const { return get_applicability(op) == Applicability::Entailed; }

    /******************************************************************************************************************/

    inline void set_self_applicability(bool value) const { selfApplicability = std::make_unique<bool>(value); }

    [[nodiscard]] inline bool known_self_applicability() const { return selfApplicability.get(); }

    [[nodiscard]] inline bool get_self_applicability() const { return *selfApplicability; }

    inline void unset_self_applicability() const { selfApplicability = nullptr; }

};

#endif //PLAJA_OP_APPLICABILITY_H
