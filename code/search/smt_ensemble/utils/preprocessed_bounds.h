//
// This file is part of the PlaJA code base (2019 - 2021).
//

#ifndef PLAJA_PREPROCESSED_BOUNDS_H
#define PLAJA_PREPROCESSED_BOUNDS_H

#include <cmath>
#include <memory>
#include "../../../include/factory_config_const.h"
#include "../../../include/marabou_include/map.h"
#include "../../../include/marabou_include/pair.h"
#include "../using_marabou.h"

// forward declaration:
class Engine;
class InputQuery;

namespace MARABOU_IN_PLAJA {

    // cf. same private method in Engine.{h,cpp} in Marabou
    extern void inform_constraints_of_initial_bounds(InputQuery& query_to_preprocess);

    struct PreprocessedBounds {
    private:
        Map<MarabouVarIndex_type, Pair<MarabouFloating_type, MarabouFloating_type>> bounds;
    public:
        explicit PreprocessedBounds(Map<MarabouVarIndex_type, Pair<MarabouFloating_type, MarabouFloating_type>>&& bounds_): bounds(std::move(bounds_)) {};
        ~PreprocessedBounds() = default;
        static std::unique_ptr<PreprocessedBounds> preprocess_bounds(InputQuery& input_query);

        [[nodiscard]] inline const Pair<MarabouFloating_type,MarabouFloating_type>& get_bounds(MarabouVarIndex_type var) const { return bounds.at(var); }
        [[nodiscard]] inline MarabouFloating_type get_lower_bound(MarabouVarIndex_type var) const { return get_bounds(var).first(); }
        [[nodiscard]] inline MarabouFloating_type get_upper_bound(MarabouVarIndex_type var) const { return get_bounds(var).second(); }
        [[nodiscard]] inline int get_lower_bound_int(MarabouVarIndex_type var) const { return std::floor(get_lower_bound(var)); }
        [[nodiscard]] inline int get_upper_bound_int(MarabouVarIndex_type var) const { return std::ceil(get_upper_bound(var)); }
    };

#if 0
    // usually no tightening of input bounds is achieved
    struct OutputPrecondition {
    private:
        Map<OutputIndex_type, Map<InputIndex_type, Pair<MarabouFloating_type, MarabouFloating_type>>> inputBoundsPerOutput;

    public:
        explicit OutputPrecondition(const InputQuery& base_query);
        ~OutputPrecondition() = default;

        [[nodiscard]] inline const Pair<MarabouFloating_type,MarabouFloating_type>& get_bounds(OutputIndex_type output, InputIndex_type input) const { return inputBoundsPerOutput.at(output).at(input); }
        [[nodiscard]] inline MarabouFloating_type get_lower_bound(OutputIndex_type output, InputIndex_type input) const { return get_bounds(output, input).first(); }
        [[nodiscard]] inline MarabouFloating_type get_upper_bound(OutputIndex_type output, InputIndex_type input) const { return get_bounds(output, input).second(); }
        [[nodiscard]] inline int get_lower_bound_int(OutputIndex_type output, InputIndex_type input) const { return std::floor(get_lower_bound(output, input)); }
        [[nodiscard]] inline int get_upper_bound_int(OutputIndex_type output, InputIndex_type input) const { return std::ceil(get_upper_bound(output, input)); }
    };
#endif

}

#endif //PLAJA_PREPROCESSED_BOUNDS_H
