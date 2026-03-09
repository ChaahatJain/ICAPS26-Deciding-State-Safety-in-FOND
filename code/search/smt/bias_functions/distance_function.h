//
// Created by Daniel Sherbakov on 2024.
//

#ifndef DISTANCE_FUNCTION_H
#define DISTANCE_FUNCTION_H

#include "../../../search/fd_adaptions/state.h"
#include "../model/model_z3.h"
#include "../utils/to_z3_expr.h"
#include "../../../search/smt/vars_in_z3.h"
#include <memory>

namespace Bias {

    enum class DistanceFunctionType {
        DistanceToTarget,
        DistanceToTargetWP,
    };

    // Function to map string to BiasType
    inline DistanceFunctionType get_function_type(const std::string& str) {
        std::cout << "DistanceFunctionType: " << str << std::endl;
        static const std::unordered_map<std::string, DistanceFunctionType> strToEnumMap = {
            {"avoid", DistanceFunctionType::DistanceToTarget},
            {"wp", DistanceFunctionType::DistanceToTargetWP},
        };

        if (const auto it = strToEnumMap.find(str); it != strToEnumMap.end()) {
            return it->second;
        }

        throw std::invalid_argument("Unknown BiasType: " + str);
    }

    class DistanceFunction {
    public:
        DistanceFunction(const Expression& target_condition, const ModelZ3& model, DistanceFunctionType type);
        [[nodiscard]] int evaluate(const State& state) const;

    private:
        const ModelZ3& model;
        z3::expr target_condition;
        z3::expr distance_function;
    };

}// namespace Bias

#endif //DISTANCE_FUNCTION_H
