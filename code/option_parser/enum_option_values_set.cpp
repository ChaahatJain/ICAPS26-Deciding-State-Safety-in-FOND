//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <algorithm>
#include "enum_option_values_set.h"
#include "../exception/option_parser_exception.h"
#include "../utils/utils.h"

PLAJA_OPTION::EnumOptionValuesSet::EnumOptionValuesSet(PLAJA_OPTION::EnumOptionValue value):
    setValue(value) {
}

PLAJA_OPTION::EnumOptionValuesSet::EnumOptionValuesSet(std::list<EnumOptionValue> possible_values, EnumOptionValue default_value):
    possibleValues(std::move(possible_values))
    , setValue(default_value) {
    STMT_IF_DEBUG(set_value(setValue)) // To trigger assert.
}

PLAJA_OPTION::EnumOptionValuesSet::~EnumOptionValuesSet() = default;

/* */

bool PLAJA_OPTION::EnumOptionValuesSet::operator==(const PLAJA_OPTION::EnumOptionValuesSet& other) const {
    return setValue == other.get_value() and possibleValues == other.possibleValues;
}

std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet> PLAJA_OPTION::EnumOptionValuesSet::copy_value() const {
    return std::unique_ptr<EnumOptionValuesSet>(new EnumOptionValuesSet(get_value()));
}

void PLAJA_OPTION::EnumOptionValuesSet::set_value(PLAJA_OPTION::EnumOptionValue value) {
    setValue = value;
    PLAJA_ASSERT(std::any_of(possibleValues.cbegin(), possibleValues.cend(), [this](const auto possible_value) { return possible_value == setValue; }))
}

void PLAJA_OPTION::EnumOptionValuesSet::set_value(const std::string& str) { setValue = str_to_enum_option_value_or_exception(str); }

/* */

std::unique_ptr<PLAJA_OPTION::EnumOptionValue> PLAJA_OPTION::EnumOptionValuesSet::str_to_enum_option_value(const std::string& str) const {
    auto value = PLAJA_OPTION::str_to_enum_option_value(str);
    return value and std::any_of(possibleValues.cbegin(), possibleValues.cend(), [&value](const PLAJA_OPTION::EnumOptionValue possible_value) { return possible_value == *value; }) ? std::move(value) : nullptr;
}

PLAJA_OPTION::EnumOptionValue PLAJA_OPTION::EnumOptionValuesSet::str_to_enum_option_value_or_exception(const std::string& str) const {
    auto value = str_to_enum_option_value(str);
    if (not value) { throw OptionParserException("Unsupported enum option value: " + str + PLAJA_UTILS::dotString); }
    else { return *value; }
}

std::string PLAJA_OPTION::EnumOptionValuesSet::enum_option_value_to_str(PLAJA_OPTION::EnumOptionValue value) {
    return PLAJA_OPTION::enum_option_value_to_str(value);
}

std::string PLAJA_OPTION::EnumOptionValuesSet::supported_values_to_str() const {
    std::string str;
    for (const auto enum_value: possibleValues) {
        str.append(enum_option_value_to_str(enum_value));
        str.append(PLAJA_UTILS::commaString);
        str.append(PLAJA_UTILS::spaceString);
    }
    str.replace(str.size() - 2, 2, PLAJA_UTILS::emptyString); // Last comma.
    return str;
}