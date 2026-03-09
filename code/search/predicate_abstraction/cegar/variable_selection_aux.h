//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_VARIABLE_SELECTION_AUX_H
#define PLAJA_VARIABLE_SELECTION_AUX_H

#include "../../../utils/structs_utils/default_map.h"

namespace VARIABLE_SELECTION {

    struct PerturbationBoundMap {

    private:
        PLAJA::DefaultMap<VariableIndex_type, std::pair<PLAJA::floating, PLAJA::floating>> mapping;

    public:
        explicit PerturbationBoundMap(PLAJA::floating default_value):
            mapping({ default_value, default_value }) {
        }

        ~PerturbationBoundMap() = default;
        DELETE_CONSTRUCTOR(PerturbationBoundMap)

        inline void set_default(PLAJA::floating value) { mapping.set_default({ value, value }); }

        // inline void set(VariableIndex_type state_index, PLAJA::floating value) { mapping.set(state_index, { value, value }); }

        inline void set(VariableIndex_type state_index, PLAJA::floating value, PerturbationBoundType type) {

            switch (type) {
                case PerturbationBoundType::Lower: {
                    mapping.access(state_index).first = value;
                    return;
                }

                case PerturbationBoundType::Upper: {
                    mapping.access(state_index).second = value;
                    return;
                }

                case PerturbationBoundType::Both: {
                    mapping.set(state_index, { value, value });
                    return;
                }

                default: { PLAJA_ABORT }
            }

            PLAJA_ABORT
        }

        inline void unset(VariableIndex_type state_index) { mapping.unset(state_index); }

        inline void clear() { mapping.clear(); }

        [[nodiscard]] inline PLAJA::floating get_default() const { return mapping.get_default().first; }

        [[nodiscard]] inline const std::pair<PLAJA::floating, PLAJA::floating>& at(VariableIndex_type state_index) const { return mapping.at(state_index); }

        [[nodiscard]] inline PLAJA::floating at(VariableIndex_type state_index, PerturbationBoundType type) const {
            const auto& value = at(state_index);

            switch (type) {
                case PerturbationBoundType::Lower: { return value.first; }

                case PerturbationBoundType::Upper: { return value.second; }

                case PerturbationBoundType::Both: {
                    PLAJA_ASSERT(value.first == value.second)
                    return value.first;
                }

                default: PLAJA_ABORT

            }

            PLAJA_ABORT
        }

        FCT_IF_DEBUG([[nodiscard]] static std::string type_to_str(PerturbationBoundType type) {

            switch (type) {
                case PerturbationBoundType::Lower: { return "[Lower]"; }

                case PerturbationBoundType::Upper: { return "[Upper]"; }

                case PerturbationBoundType::Both: { return "[Lower & Upper]"; }

                default: PLAJA_ABORT

            }

            PLAJA_ABORT

        })

    };

/**********************************************************************************************************************/

    struct VarInfo {

        enum PerturbationPoint {
            Lower,
            Special,
            Upper,
        };

    private:
        VariableIndex_type stateIndex;

        PLAJA::floating currentLb;
        PLAJA::floating currentUb;

        PLAJA::floating perturbationPoint; // NOLINT(*-use-default-member-init)
        PerturbationPoint perturbationPointFlag; // NOLINT(*-use-default-member-init)
        PLAJA::floating perturbationInterval;

        bool isInt;

    public:
        VarInfo(VariableIndex_type state_index, bool is_int, PLAJA::floating lb, PLAJA::floating ub):
            stateIndex(state_index)
            , currentLb(lb)
            , currentUb(ub)
            , perturbationPoint((lb + ub) / 2)
            , perturbationPointFlag(Special)
            , perturbationInterval(perturbationPoint - lb)
            , isInt(is_int) {
            PLAJA_ASSERT(currentLb <= currentUb)
        }

        ~VarInfo() = default;

        DEFAULT_CONSTRUCTOR_ONLY(VarInfo)
        DELETE_ASSIGNMENT(VarInfo)

        /* Setter. */

        inline void set_current_bounds(PLAJA::floating current_lb, PLAJA::floating current_ub) { // NOLINT(*-easily-swappable-parameters)
            currentLb = current_lb;
            currentUb = current_ub;
            PLAJA_ASSERT(currentLb <= currentUb)
        }

        inline void set_perturbation_point(PLAJA::floating perturbation_point) {
            perturbationPoint = perturbation_point;
            perturbationPointFlag = PerturbationPoint::Special;
            PLAJA_ASSERT(currentLb <= perturbationPoint)
            PLAJA_ASSERT(currentUb >= perturbationPoint)
            perturbationInterval = std::max(perturbationPoint - currentLb, currentUb - perturbationPoint);
        }

        inline void set_perturbation_point(PerturbationPoint perturbation_point) {
            PLAJA_ASSERT(perturbation_point == PerturbationPoint::Lower or perturbation_point == PerturbationPoint::Upper)
            perturbationPointFlag = perturbation_point;
            perturbationPoint = perturbation_point == PerturbationPoint::Lower ? currentLb : currentUb;
            perturbationInterval = currentUb - currentLb;
        }

        /* Getter. */

        [[nodiscard]] inline VariableIndex_type get_var() const { return stateIndex; }

        [[nodiscard]] inline bool is_int() const { return isInt; }

        [[nodiscard]] inline PLAJA::floating get_current_lb() const { return currentLb; }

        [[nodiscard]] inline PLAJA::floating get_current_ub() const { return currentUb; }

        [[nodiscard]] inline PLAJA::floating get_perturbation_point() const { return perturbationPoint; }

        [[nodiscard]] inline PerturbationPoint get_perturbation_point_flag() const { return perturbationPointFlag; }

        [[nodiscard]] inline VARIABLE_SELECTION::PerturbationBoundType perturb_towards() const {
            switch (get_perturbation_point_flag()) {
                case PerturbationPoint::Lower: { return VARIABLE_SELECTION::PerturbationBoundType::Upper; }
                case PerturbationPoint::Upper: { return VARIABLE_SELECTION::PerturbationBoundType::Lower; }
                case PerturbationPoint::Special: { return VARIABLE_SELECTION::PerturbationBoundType::Both; }
                default: PLAJA_ABORT
            }
            PLAJA_ABORT
        };

        [[nodiscard]] inline PLAJA::floating get_perturbation_interval() const { return perturbationInterval; }

        [[nodiscard]] inline PLAJA::floating propose_perturbation_precision(PLAJA::floating default_precision) const {

            if (isInt) { // For integer domain we can fix a perturbation bound precision.
                PLAJA_ASSERT(std::round(perturbationInterval) >= 1)
                return 1 / perturbationInterval;
            }

            return default_precision;

        }

        /** Lower perturbation bound (s(v) - v) / normalization <= epsilon for || s'(v) - s(v) ||_inf <= epsilon. */
        [[nodiscard]] inline PLAJA::floating compute_lower_perturbation_bound(PLAJA::floating perturbation_rel) const {
            auto perturbation_abs = -perturbationInterval * perturbation_rel + perturbationPoint;
            if (isInt) {
                auto perturbation_abs_rounded = std::round(perturbation_abs);
                perturbation_abs = PLAJA_FLOATS::equal(perturbation_abs, perturbation_abs_rounded, PLAJA::integerPrecision) ? perturbation_abs_rounded : std::ceil(perturbation_abs);
            }
            return std::max(currentLb, perturbation_abs);
        }

        /** Upper perturbation bound (v - s(v)) / normalization <= epsilon for || s'(v) - s(v) ||_inf <= epsilon. */
        [[nodiscard]] inline PLAJA::floating compute_upper_perturbation_bound(PLAJA::floating perturbation_rel) const {
            auto perturbation_abs = perturbationInterval * perturbation_rel + perturbationPoint;
            if (isInt) {
                auto perturbation_abs_rounded = std::round(perturbation_abs);
                perturbation_abs = PLAJA_FLOATS::equal(perturbation_abs, perturbation_abs_rounded, PLAJA::integerPrecision) ? perturbation_abs_rounded : std::floor(perturbation_abs);
            }
            return std::min(currentUb, perturbation_abs);
        }

        [[nodiscard]] inline std::unique_ptr<Expression> compute_lower_perturbation_bound_absolute(PLAJA::floating perturbation_rel) const {
            auto perturbation_abs = compute_lower_perturbation_bound(perturbation_rel);
            PLAJA_ASSERT(not isInt or PLAJA_FLOATS::is_integer(perturbation_abs))
            return isInt ? PLAJA_EXPRESSION::make_int(PLAJA_UTILS::cast_numeric<PLAJA::integer>(perturbation_abs)) : PLAJA_EXPRESSION::make_real(perturbation_abs);
        }

        [[nodiscard]] inline std::unique_ptr<Expression> compute_upper_perturbation_bound_absolute(PLAJA::floating perturbation_rel) const {
            auto perturbation_abs = compute_upper_perturbation_bound(perturbation_rel);
            PLAJA_ASSERT(not isInt or PLAJA_FLOATS::is_integer(perturbation_abs))
            return isInt ? PLAJA_EXPRESSION::make_int(PLAJA_UTILS::cast_numeric<PLAJA::integer>(perturbation_abs)) : PLAJA_EXPRESSION::make_real(perturbation_abs);
        }

    };

/**********************************************************************************************************************/

    struct VarInfoVec {

    private:
        const ModelInformation* modelInfo;
        std::vector<VarInfo> varInfo;
        PLAJA::floating perturbationBoundPrecision; // As per integer vars. NOLINT(*-use-default-member-init)
        std::size_t numInts;
        std::size_t numFloats;

    public:
        explicit VarInfoVec(const ModelInformation& model_info):
            modelInfo(&model_info)
            , perturbationBoundPrecision(1)
            , numInts(model_info.get_int_state_size())
            , numFloats(model_info.get_floating_state_size()) {

            varInfo.reserve(model_info.get_state_size());

            for (auto it = model_info.stateIndexIterator(true); !it.end(); ++it) {

                if (not it.is_floating()) { varInfo.emplace_back(it.state_index(), true, it.lower_bound_int(), it.upper_bound_int()); }
                else { varInfo.emplace_back(it.state_index(), false, it.lower_bound_float(), it.upper_bound_float()); }

            }

        }

        ~VarInfoVec() = default;
        DELETE_CONSTRUCTOR(VarInfoVec)

        void load_var_info(const StateBase& state, const StateBase* ref_state) {

            perturbationBoundPrecision = 1;

            const auto state_size = varInfo.size();

            if (ref_state) {

                for (VariableIndex_type state_index = 0; state_index < state_size; ++state_index) {
                    const auto perturbation_point = state[state_index];
                    const auto ref_point = (*ref_state)[state_index];

                    auto& var_info = varInfo[state_index];

                    if (perturbation_point < ref_point) { // Check exactly have to choose one case anyway.
                        var_info.set_current_bounds(perturbation_point, ref_point);
                        var_info.set_perturbation_point(VARIABLE_SELECTION::VarInfo::PerturbationPoint::Lower);
                    } else {
                        var_info.set_current_bounds(ref_point, perturbation_point);
                        var_info.set_perturbation_point(VARIABLE_SELECTION::VarInfo::PerturbationPoint::Upper);
                    }

                    if (var_info.is_int() and var_info.get_perturbation_interval() > 0) { perturbationBoundPrecision = std::min(perturbationBoundPrecision, 1 / var_info.get_perturbation_interval()); }

                }

            } else {

                for (VariableIndex_type state_index = 0; state_index < state_size; ++state_index) {

                    auto& var_info = varInfo[state_index];

                    var_info.set_current_bounds(modelInfo->get_lower_bound(state_index), modelInfo->get_upper_bound(state_index));
                    var_info.set_perturbation_point(state[state_index]);

                    if (var_info.is_int() and var_info.get_perturbation_interval() > 0) { perturbationBoundPrecision = std::min(perturbationBoundPrecision, 1 / var_info.get_perturbation_interval()); }

                }

            }
        }

        void load_var_info(const StateBase& state, const StateValues& lower_bounds, const StateValues& upper_bounds) { // NOLINT(*-easily-swappable-parameters)

            perturbationBoundPrecision = 1;

            const auto state_size = varInfo.size();

            for (VariableIndex_type state_index = 0; state_index < state_size; ++state_index) {

                const auto perturbation_point = state[state_index];
                const auto lb = lower_bounds[state_index];
                const auto ub = upper_bounds[state_index];

                auto& var_info = varInfo[state_index];
                var_info.set_current_bounds(lb, ub);

                if (PLAJA_FLOATS::equal(perturbation_point, lb, PLAJA::floatingPrecision)) {
                    var_info.set_perturbation_point(VARIABLE_SELECTION::VarInfo::PerturbationPoint::Lower);
                } else if (PLAJA_FLOATS::equal(perturbation_point, ub, PLAJA::floatingPrecision)) {
                    var_info.set_perturbation_point(VARIABLE_SELECTION::VarInfo::PerturbationPoint::Upper);
                } else {
                    var_info.set_perturbation_point(perturbation_point);
                }

                if (var_info.is_int() and var_info.get_perturbation_interval() > 0) { perturbationBoundPrecision = std::min(perturbationBoundPrecision, 1 / var_info.get_perturbation_interval()); }

            }

        }

        [[nodiscard]] inline const VarInfo& get_info(VariableIndex_type state_index) const {
            PLAJA_ASSERT(state_index < varInfo.size())
            return varInfo[state_index];
        }

        [[nodiscard]] inline const VarInfo& operator[](VariableIndex_type state_index) const { return get_info(state_index); }

        [[nodiscard]] inline PLAJA::floating propose_perturbation_bound_precision(PLAJA::floating default_precision, bool ignore_float) const {

            PLAJA_ASSERT(not PLAJA_FLOATS::is_zero(perturbationBoundPrecision))
            PLAJA_ASSERT(PLAJA_FLOATS::lte(perturbationBoundPrecision, 1.0))

            if (ignore_float or numFloats == 0) {
                return (numInts > 0) ? perturbationBoundPrecision : default_precision;
            } else {
                PLAJA_ASSERT(numFloats > 0)
                return numInts > 0 ? std::min(perturbationBoundPrecision, default_precision) : default_precision;
            }

        }

    };

}

#endif //PLAJA_VARIABLE_SELECTION_AUX_H
