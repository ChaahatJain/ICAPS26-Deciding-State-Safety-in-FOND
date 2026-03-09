//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "configuration.h"
#include "../../option_parser/enum_option_values_set.h"

PLAJA::Configuration::Configuration(const OptionParser& option_parser):
    optionParser(&option_parser) {
}

PLAJA::Configuration::Configuration():
    optionParser(PLAJA_GLOBAL::optionParser) {}

PLAJA::Configuration::Configuration(const PLAJA::Configuration& other):
    optionParser(other.optionParser)
    , flags(other.flags)
    , disabledFlags(other.disabledFlags)
    , options(other.options)
    , optionsInt(other.optionsInt)
    , optionsDouble(other.optionsDouble)
    , optionsEnum()
    , disabledOptions(other.disabledOptions)
    , sharableStructures()
    , sharableStructuresPtr()
    , sharableStructuresConst() {
    for (const auto& [option, value]: other.optionsEnum) { optionsEnum.emplace(option, std::make_unique<PLAJA_OPTION::EnumOptionValuesSet>(*value)); }

    load_sharable_from(other);
}

PLAJA::Configuration::~Configuration() = default;

/**********************************************************************************************************************/
// options

// setter

void PLAJA::Configuration::set_flag(const std::string& flag, bool value) {
    if (value) {
        flags.insert(flag);
        disabledFlags.erase(flag);
    } else {
        disabledFlags.insert(flag);
        flags.erase(flag);
    }
}

void PLAJA::Configuration::set_value_option(const std::string& option, const std::string& value) {
    disabledOptions.erase(option);
    options[option] = value;
}

void PLAJA::Configuration::set_int_option(const std::string& option, int value) {
    disabledOptions.erase(option);
    optionsInt[option] = value;
}

void PLAJA::Configuration::set_bool_option(const std::string& option, bool value) { set_int_option(option, value); }

void PLAJA::Configuration::set_double_option(const std::string& option, double value) {
    disabledOptions.erase(option);
    optionsDouble[option] = value;
}

void PLAJA::Configuration::set_enum_option(const std::string& option, std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet> value) {
    disabledOptions.erase(option);
    optionsEnum[option] = std::move(value);
}

void PLAJA::Configuration::disable_value_option(const std::string& option) {
    options.erase(option);
    disabledOptions.insert(option);
}

// getter

const std::string& PLAJA::Configuration::get_option(const std::string& option) const {
    PLAJA_ASSERT(not disabledOptions.count(option));
    PLAJA_ASSERT(has_option(option))
    auto it = options.find(option);
    return it == options.end() ? optionParser->get_option(option) : it->second;
}

int PLAJA::Configuration::get_int_option(const std::string& option) const {
    PLAJA_ASSERT(not disabledOptions.count(option));
    PLAJA_ASSERT(has_int_option(option))
    auto it = optionsInt.find(option);
    return it == optionsInt.end() ? optionParser->get_int_option(option) : it->second;
}

bool PLAJA::Configuration::get_bool_option(const std::string& option) const {
    PLAJA_ASSERT(not disabledOptions.count(option));
    PLAJA_ASSERT(has_bool_option(option))
    auto it = optionsInt.find(option);
    return it == optionsInt.end() ? optionParser->get_bool_option(option) : it->second;
}

double PLAJA::Configuration::get_double_option(const std::string& option) const {
    PLAJA_ASSERT(not disabledOptions.count(option));
    PLAJA_ASSERT(has_double_option(option))
    auto it = optionsDouble.find(option);
    return it == optionsDouble.end() ? optionParser->get_double_option(option) : it->second;
}

const PLAJA_OPTION::EnumOptionValuesSet& PLAJA::Configuration::get_enum_option(const std::string& option) const {
    PLAJA_ASSERT(not disabledOptions.count(option));
    PLAJA_ASSERT(has_enum_option(option, false))
    auto it = optionsEnum.find(option);
    return it == optionsEnum.end() ? optionParser->get_enum_option(option) : *it->second;
}

//

bool PLAJA::Configuration::is_flag_set(const std::string& flag) const {
    if (flags.count(flag)) { return true; }
    else if (disabledFlags.count(flag)) { return false; }
    else { return optionParser->is_flag_set(flag); }
}

bool PLAJA::Configuration::has_value_option(const std::string& option) const {
    return not disabledOptions.count(option) and (options.count(option) or optionParser->has_value_option(option));
}

bool PLAJA::Configuration::has_option(const std::string& option) const {
    return not disabledOptions.count(option) and (options.count(option) or optionParser->has_option(option));
}

bool PLAJA::Configuration::has_int_option(const std::string& option) const {
    return not disabledOptions.count(option) and (optionsInt.count(option) or optionParser->has_int_option(option));
}

bool PLAJA::Configuration::has_bool_option(const std::string& option) const { return has_int_option(option); }

bool PLAJA::Configuration::has_double_option(const std::string& option) const {
    return not disabledOptions.count(option) and (optionsDouble.count(option) or optionParser->has_double_option(option));
}

bool PLAJA::Configuration::has_enum_option(const std::string& option, bool ignore_none) const {
    return not disabledOptions.count(option) and (optionsEnum.count(option) or optionParser->has_enum_option(option, ignore_none));
}

const std::string& PLAJA::Configuration::get_value_option_string(const std::string& option) const {
    auto it = options.find(option);
    if (it != options.cend()) { return it->second; }
    else { return optionParser->get_value_option_string(option); }
}

/**********************************************************************************************************************/

void PLAJA::Configuration::delete_sharable(PLAJA::SharableKey key) { sharableStructures.erase(key); }

void PLAJA::Configuration::delete_sharable_ptr(PLAJA::SharableKey key) { sharableStructuresPtr.erase(key); }

void PLAJA::Configuration::delete_sharable_const(PLAJA::SharableKey key) { sharableStructuresConst.erase(key); }

void PLAJA::Configuration::load_sharable_from(const PLAJA::Configuration& other) const {
    for (const auto& [key, sharable]: other.sharableStructures) { sharableStructures.emplace(key, sharable->copy()); } // do not override existing shared structures !!!
    for (const auto& [key, sharable]: other.sharableStructuresPtr) { sharableStructuresPtr.emplace(key, sharable->copy()); }
    for (const auto& [key, sharable]: other.sharableStructuresConst) { sharableStructuresConst.emplace(key, sharable->copy()); }
}

void PLAJA::Configuration::load_sharable(PLAJA::SharableKey key, const PLAJA::Configuration& other) { sharableStructures.emplace(key, other.sharableStructures.at(key)->copy()); }

void PLAJA::Configuration::load_sharable_ptr(PLAJA::SharableKey key, const PLAJA::Configuration& other) { sharableStructuresPtr.emplace(key, other.sharableStructuresPtr.at(key)->copy()); }

void PLAJA::Configuration::load_sharable_const(PLAJA::SharableKey key, const PLAJA::Configuration& other) { sharableStructuresConst.emplace(key, other.sharableStructuresConst.at(key)->copy()); }

bool PLAJA::Configuration::has_sharable(PLAJA::SharableKey key) const { return sharableStructures.count(key); }

bool PLAJA::Configuration::has_sharable_ptr(PLAJA::SharableKey key) const { return sharableStructuresPtr.count(key); }

bool PLAJA::Configuration::has_sharable_const(PLAJA::SharableKey key) const { return sharableStructuresConst.count(key); }
