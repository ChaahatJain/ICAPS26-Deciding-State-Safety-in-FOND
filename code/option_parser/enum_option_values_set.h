//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ENUM_OPTION_VALUES_SET_H
#define PLAJA_ENUM_OPTION_VALUES_SET_H

#include <list>
#include "../utils/default_constructors.h"
#include "enum_option_values.h"

namespace PLAJA_OPTION {

    /**
     * Restricted set of enum values for some specific option.
     * This class is not implemented for runtime efficiency but to simplify code maintenance.
     */
    class EnumOptionValuesSet final {

    private:
        std::list<EnumOptionValue> possibleValues;
        EnumOptionValue setValue;

        explicit EnumOptionValuesSet(EnumOptionValue value);
    public:
        EnumOptionValuesSet(std::list<EnumOptionValue> possible_values, EnumOptionValue default_value);
        ~EnumOptionValuesSet();
        DEFAULT_CONSTRUCTOR_ONLY(EnumOptionValuesSet)
        DELETE_ASSIGNMENT(EnumOptionValuesSet)

        [[nodiscard]] bool operator==(const EnumOptionValuesSet& other) const;

        [[nodiscard]] inline bool operator!=(const EnumOptionValuesSet& other) const { return not operator==(other); }

        [[nodiscard]] std::unique_ptr<EnumOptionValuesSet> copy_value() const;

        [[nodiscard]] inline static std::unique_ptr<EnumOptionValuesSet> construct_singleton(EnumOptionValue value) { return std::unique_ptr<EnumOptionValuesSet>(new EnumOptionValuesSet(value)); };

        void set_value(EnumOptionValue value);

        void set_value(const std::string& str);

        [[nodiscard]] inline EnumOptionValue get_value() const { return setValue; }

        [[nodiscard]] inline bool in_none() const { return setValue == EnumOptionValue::None; }

        /**
         * No support for runtime efficiency. Intended for parsing step only.
         */
        [[nodiscard]] std::unique_ptr<EnumOptionValue> str_to_enum_option_value(const std::string& str) const;
        [[nodiscard]] EnumOptionValue str_to_enum_option_value_or_exception(const std::string& str) const;

        [[nodiscard]] static std::string enum_option_value_to_str(EnumOptionValue value);

        [[nodiscard]] std::string supported_values_to_str() const;

    };

}

#endif //PLAJA_ENUM_OPTION_VALUES_SET_H
