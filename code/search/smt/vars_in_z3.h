//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_VARS_IN_Z3_H
#define PLAJA_VARS_IN_Z3_H

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "../../parser/ast/forward_ast.h"
#include "../../parser/using_parser.h"
#include "../../assertions.h"
#include "../information/forward_information.h"
#include "../states/forward_states.h"
#include "forward_smt_z3.h"
#include "using_smt.h"

namespace Z3_IN_PLAJA {

    /* Extern. */
    extern const std::string locPrefix;

    class StateVarsInZ3 final {
        friend class LocationIterator; //
        friend class VariableIterator;

    private:
        Z3_IN_PLAJA::Context* context;
        const ModelZ3* parent;

        /* Mapping information. */
        std::unordered_map<VariableID_type, Z3_IN_PLAJA::VarId_type> stateVarToZ3;
        std::unordered_map<Z3_IN_PLAJA::VarId_type, VariableID_type> z3ToStateVar;

        /* Locs. */
        std::unordered_map<VariableIndex_type, Z3_IN_PLAJA::VarId_type> locToZ3;
        std::unordered_map<Z3_IN_PLAJA::VarId_type, VariableIndex_type> z3ToLoc;

        /* Fixed? */ // Deprecated in favor of const-vars-to-consts
        // std::unordered_set<VariableID_type> fixed;

        [[nodiscard]] std::unique_ptr<z3::expr> generate_bound(const VariableDeclaration& state_var) const;
        [[nodiscard]] z3::expr generate_fix(const VariableDeclaration& state_var) const;
        [[nodiscard]] z3::expr generate_bound_loc(VariableIndex_type loc) const;

        [[nodiscard]] const Model& get_model() const;

    public:
        explicit StateVarsInZ3(const ModelZ3& parent);
        ~StateVarsInZ3();

        StateVarsInZ3(const StateVarsInZ3& other);
        StateVarsInZ3(StateVarsInZ3&& other) = delete;
        DELETE_ASSIGNMENT(StateVarsInZ3)

        [[nodiscard]] inline const ModelZ3& get_parent() const { return *parent; }

        [[nodiscard]] const ModelInformation& get_model_info() const;

        [[nodiscard]] inline Z3_IN_PLAJA::Context& get_context() const { return *context; }

        [[nodiscard]] inline std::size_t size() const { return stateVarToZ3.size() + locToZ3.size(); }

        [[nodiscard]] inline bool empty() const { return stateVarToZ3.empty() and locToZ3.empty(); }

        [[nodiscard]] inline std::size_t num_locs() const { return locToZ3.size(); }

        [[nodiscard]] inline bool has_loc() const { return not locToZ3.empty(); }

        FCT_IF_DEBUG([[nodiscard]] bool is_loc(VariableIndex_type state_index) const;)

        /* Variables. */

        [[nodiscard]] inline bool exists(VariableID_type state_var) const { return stateVarToZ3.count(state_var); }

        [[nodiscard]] inline Z3_IN_PLAJA::VarId_type to_z3(VariableID_type state_var) const {
            PLAJA_ASSERT(exists(state_var))
            return stateVarToZ3.at(state_var);
        }

        [[nodiscard]] inline bool exists_reverse(Z3_IN_PLAJA::VarId_type z3_var) const { return z3ToStateVar.count(z3_var); }

        [[nodiscard]] inline VariableID_type to_state_var(Z3_IN_PLAJA::VarId_type z3_var) const {
            PLAJA_ASSERT(exists_reverse(z3_var))
            return z3ToStateVar.at(z3_var);
        }

        /* Locations. */

        [[nodiscard]] inline bool exists_loc(VariableIndex_type loc) const { return locToZ3.count(loc); }

        [[nodiscard]] inline Z3_IN_PLAJA::VarId_type loc_to_z3(VariableIndex_type loc) const {
            PLAJA_ASSERT(exists_loc(loc))
            return locToZ3.at(loc);
        }

        [[nodiscard]] inline bool exists_reverse_loc(Z3_IN_PLAJA::VarId_type z3_var) const { return z3ToLoc.count(z3_var); }

        [[nodiscard]] inline VariableIndex_type to_loc(Z3_IN_PLAJA::VarId_type z3_var) const {
            PLAJA_ASSERT(exists_reverse_loc(z3_var))
            return z3ToLoc.at(z3_var);
        }

        /* State index. */

        [[nodiscard]] bool exists_index(VariableIndex_type state_index) const;

        /* Constants. */

        [[nodiscard]] bool exists_const(ConstantIdType const_id) const;

        [[nodiscard]] Z3_IN_PLAJA::VarId_type const_to_z3(ConstantIdType const_id) const;

        /* Add information */

        Z3_IN_PLAJA::VarId_type add(VariableID_type state_var, const std::string& postfix);
        Z3_IN_PLAJA::VarId_type add(const VariableDeclaration& state_var, const std::string& postfix);
        Z3_IN_PLAJA::VarId_type add_loc(VariableIndex_type lo, const std::string& postfix);
        void set(VariableID_type state_var, Z3_IN_PLAJA::VarId_type z3_var);
        void set_loc(VariableIndex_type loc, Z3_IN_PLAJA::VarId_type z3_var);

        /** Add constant to z3 context. */
        static Z3_IN_PLAJA::VarId_type add(const ConstantDeclaration& constant, Z3_IN_PLAJA::Context& context);

        /* To z3. */

        [[nodiscard]] const z3::expr& to_z3_expr(VariableID_type state_var) const;
        [[nodiscard]] const z3::expr& loc_to_z3_expr(VariableIndex_type loc) const;
        [[nodiscard]] z3::expr var_to_z3_expr_index(VariableIndex_type loc) const;
        [[nodiscard]] z3::expr to_z3_expr_index(VariableIndex_type state_index) const;
        [[nodiscard]] const z3::expr& const_to_z3_expr(ConstantIdType const_id) const;

        [[nodiscard]] std::unique_ptr<Z3_IN_PLAJA::StateIndexes> cache_state_indexes(bool include_locs) const;

        /* Fix. */

        // void fix(VariableID_type state_var);

        // [[nodiscard]] inline bool is_fixed(VariableID_type state_var) const { return fixed.count(state_var); }

        /* Bounds. */

        [[nodiscard]] bool has_bound(VariableID_type state_var) const;
        [[nodiscard]] const z3::expr& get_bound(VariableID_type state_var) const;
        [[nodiscard]] const z3::expr& get_bound_loc(VariableIndex_type loc) const;
        [[nodiscard]] const z3::expr& get_value_const(ConstantIdType const_id) const;
        [[nodiscard]] z3::expr_vector bounds_to_vec() const;

        /* Solution. */

        void extract_solution(const Z3_IN_PLAJA::Solution& solution, StateValues& state, bool include_locs) const;
        void extract_solution(const Z3_IN_PLAJA::Solution& solution, StateValues& state, VariableIndex_type state_index) const;
        [[nodiscard]] z3::expr encode_solution(const Z3_IN_PLAJA::Solution& solution, bool include_locs) const;
        [[nodiscard]] z3::expr encode_state(const StateValues& state, bool include_locs) const;
        [[nodiscard]] z3::expr encode_state_value(const StateBase& state, VariableIndex_type state_index) const;

        /******************************************************************************************************************/

        class LocIterator final {
            friend StateVarsInZ3;
        protected:
            const StateVarsInZ3* stateVars;
            std::unordered_map<VariableIndex_type, Z3_IN_PLAJA::VarId_type>::const_iterator it;
            const std::unordered_map<VariableIndex_type, Z3_IN_PLAJA::VarId_type>::const_iterator itEnd;

            explicit LocIterator(const StateVarsInZ3& state_vars):
                stateVars(&state_vars)
                , it(state_vars.locToZ3.cbegin())
                , itEnd(state_vars.locToZ3.cend()) {}

        public:
            ~LocIterator() = default;
            DELETE_CONSTRUCTOR(LocIterator)

            inline void operator++() { ++it; }

            [[nodiscard]] virtual inline bool end() const { return it == itEnd; } // stop *after* max step
            [[nodiscard]] inline VariableIndex_type loc() const { return it->first; }

            [[nodiscard]] inline Z3_IN_PLAJA::VarId_type loc_z3() const { return it->second; }

            [[nodiscard]] const z3::expr& loc_z3_expr() const;
        };

        [[nodiscard]] inline LocIterator locIterator() const { return LocIterator(*this); }

        class VarIterator final {
            friend StateVarsInZ3;
        protected:
            const StateVarsInZ3* stateVars;
            std::unordered_map<VariableID_type, Z3_IN_PLAJA::VarId_type>::const_iterator it;
            const std::unordered_map<VariableID_type, Z3_IN_PLAJA::VarId_type>::const_iterator itEnd;

            explicit VarIterator(const StateVarsInZ3& state_vars):
                stateVars(&state_vars)
                , it(state_vars.stateVarToZ3.cbegin())
                , itEnd(state_vars.stateVarToZ3.cend()) {}

        public:
            ~VarIterator() = default;
            DELETE_CONSTRUCTOR(VarIterator)

            inline void operator++() { ++it; }

            [[nodiscard]] inline bool end() const { return it == itEnd; } // stop *after* max step
            [[nodiscard]] inline VariableID_type var() const { return it->first; }

            [[nodiscard]] inline Z3_IN_PLAJA::VarId_type var_z3() const { return it->second; }

            [[nodiscard]] const z3::expr& var_z3_expr() const;
        };

        [[nodiscard]] inline VarIterator varIterator() const { return VarIterator(*this); }

    };

    /******************************************************************************************************************/

    class StateIndexes final {
        friend class StateVarsInZ3;

    private:
        const StateVarsInZ3* parent;
        std::unordered_map<VariableIndex_type, std::unique_ptr<z3::expr>> stateIndexes;

        explicit StateIndexes(const StateVarsInZ3& state_vars_to_z3);
    public:
        ~StateIndexes();

        StateIndexes(const StateIndexes& other);
        StateIndexes(StateIndexes&& other) = delete;
        DELETE_ASSIGNMENT(StateIndexes)

        [[nodiscard]] inline bool empty() const { return stateIndexes.empty(); }

        [[nodiscard]] inline std::size_t size() const { return stateIndexes.size(); }

        [[nodiscard]] inline std::unordered_map<VariableIndex_type, std::unique_ptr<z3::expr>>::const_iterator begin() const { return stateIndexes.cbegin(); }

        [[nodiscard]] inline std::unordered_map<VariableIndex_type, std::unique_ptr<z3::expr>>::const_iterator end() const { return stateIndexes.cend(); }

        [[nodiscard]] inline bool exists(VariableIndex_type state_index) const { return stateIndexes.count(state_index); }

        /* */

        inline const z3::expr& operator[](VariableIndex_type state_index) const {
            PLAJA_ASSERT(exists(state_index))
            return *stateIndexes.at(state_index);
        }

        void remove(VariableIndex_type state_index);

        void extract_solution(const Z3_IN_PLAJA::Solution& solution, StateValues& solution_state) const;
        [[nodiscard]] z3::expr encode_solution(const Z3_IN_PLAJA::Solution& solution) const;
        [[nodiscard]] z3::expr encode_state(const StateValues& state) const;
        // TODO might restrict extracting to occurring variables:
        // This could be handled externally setting state_indexes appropriately.
        // Alternatively, instead of silently using "model_completion" one could check if "eval" yields an integer value (i.e., if variable is present).
        // That said, source variables are usually present in the query anyway (due to the predicates, in PA context.)

        /******************************************************************************************************************/

        class Iterator final {
            friend StateIndexes;
        protected:
            std::unordered_map<VariableIndex_type, std::unique_ptr<z3::expr>>::const_iterator it;
            const std::unordered_map<VariableIndex_type, std::unique_ptr<z3::expr>>::const_iterator itEnd;

            explicit Iterator(const StateIndexes& state_indexes):
                it(state_indexes.stateIndexes.cbegin())
                , itEnd(state_indexes.stateIndexes.cend()) {}

        public:
            ~Iterator() = default;
            DELETE_CONSTRUCTOR(Iterator)

            inline void operator++() { ++it; }

            [[nodiscard]] inline bool end() const { return it == itEnd; } // stop *after* max step

            [[nodiscard]] inline VariableIndex_type state_index() const { return it->first; }

            [[nodiscard]] inline const z3::expr& state_index_z3() const { return *it->second; }
        };

        [[nodiscard]] inline Iterator iterator() const { return Iterator(*this); }

    };

}

#endif //PLAJA_VARS_IN_Z3_H
