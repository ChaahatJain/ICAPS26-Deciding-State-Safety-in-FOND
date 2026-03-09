//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "stats_base.h"
#include "../utils/floating_utils.h"
#include <fstream>

PLAJA::StatsBase::StatsBase() = default;

PLAJA::StatsBase::~StatsBase() = default;

//

void PLAJA::StatsBase::init_aux(const std::list<StatsInt>& attributes, int value) {
    for (auto attr: attributes) { set_attr_int(attr, value); }
}
void PLAJA::StatsBase::init_aux(const std::list<StatsUnsigned>& attributes, unsigned value) {
    for (auto attr: attributes) { set_attr_unsigned(attr, value); }
}
void PLAJA::StatsBase::init_aux(const std::list<StatsDouble>& attributes, double value) {
    for (auto attr: attributes) { set_attr_double(attr, value); }
}
void PLAJA::StatsBase::init_aux(const std::list<StatsString>& attributes, const std::string& value) {
    for (auto attr: attributes) { set_attr_string(attr, value); }
}

void PLAJA::StatsBase::print_aux(
    const std::list<StatsInt>& attributes,
    const std::list<std::string>& attr_strs,
    bool skip_zero) const {
    PLAJA_ASSERT(attributes.size() == attr_strs.size())
    auto it_attr = attributes.cbegin();
    auto it_str = attr_strs.cbegin();
    auto end_attr = attributes.cend();
    for (; it_attr != end_attr; (++it_attr, ++it_str)) {
        auto value = get_attr_int(*it_attr);
        if (skip_zero and value == 0) { continue; }
        PLAJA_LOG(PLAJA_UTILS::string_f("%s: %i", it_str->c_str(), value))
    }
}

void PLAJA::StatsBase::print_aux(
    const std::list<StatsUnsigned>& attributes,
    const std::list<std::string>& attr_strs,
    bool skip_zero) const {
    PLAJA_ASSERT(attributes.size() == attr_strs.size())
    auto it_attr = attributes.cbegin();
    auto it_str = attr_strs.cbegin();
    auto end_attr = attributes.cend();
    for (; it_attr != end_attr; (++it_attr, ++it_str)) {
        auto value = get_attr_unsigned(*it_attr);
        if (skip_zero and value == 0) { continue; }
        PLAJA_LOG(PLAJA_UTILS::string_f("%s: %u", it_str->c_str(), value))
    }
}

void PLAJA::StatsBase::print_aux(
    const std::list<StatsDouble>& attributes,
    const std::list<std::string>& attr_strs,
    bool skip_zero,
    double tolerance) const {
    PLAJA_ASSERT(attributes.size() == attr_strs.size())
    auto it_attr = attributes.cbegin();
    auto it_str = attr_strs.cbegin();
    auto end_attr = attributes.cend();
    for (; it_attr != end_attr; (++it_attr, ++it_str)) {
        auto value = get_attr_double(*it_attr);
        if (skip_zero and PLAJA_FLOATS::is_zero(value, tolerance)) { continue; }
        PLAJA_LOG(PLAJA_UTILS::string_f("%s: %f", it_str->c_str(), value))
    }
}

void PLAJA::StatsBase::print_aux(const std::list<StatsString>& attributes, const std::list<std::string>& attr_strs)
    const {
    PLAJA_ASSERT(attributes.size() == attr_strs.size())
    auto it_attr = attributes.cbegin();
    auto it_str = attr_strs.cbegin();
    auto end_attr = attributes.cend();
    for (; it_attr != end_attr; (++it_attr, ++it_str)) {
        auto value = get_attr_string(*it_attr);
        PLAJA_LOG(PLAJA_UTILS::string_f("%s: %s", it_str->c_str(), value))
    }
}

void PLAJA::StatsBase::stats_to_csv_aux(std::ofstream& file, const std::list<StatsInt>& attributes) const {
    for (auto attr: attributes) { file << PLAJA_UTILS::commaString << get_attr_int(attr); }
}
void PLAJA::StatsBase::stats_to_csv_aux(std::ofstream& file, const std::list<StatsUnsigned>& attributes) const {
    for (auto attr: attributes) { file << PLAJA_UTILS::commaString << get_attr_unsigned(attr); }
}
void PLAJA::StatsBase::stats_to_csv_aux(std::ofstream& file, const std::list<StatsDouble>& attributes) const {
    for (auto attr: attributes) { file << PLAJA_UTILS::commaString << get_attr_double(attr); }
}
void PLAJA::StatsBase::stats_to_csv_aux(std::ofstream& file, const std::list<StatsString>& attributes) const {
    for (auto attr: attributes) { file << PLAJA_UTILS::commaString << get_attr_string(attr); }
}

void PLAJA::StatsBase::stat_names_to_csv_aux(std::ofstream& file, const std::list<std::string>& attr_strs) {
    for (const auto& attr_str: attr_strs) { file << PLAJA_UTILS::commaString << attr_str; }
}

void PLAJA::StatsBase::add_int_stats(
    const std::list<StatsInt>& attr,
    const std::list<std::string>& attr_str,
    int init) {
    auto it = attr.begin();
    auto it_str = attr_str.begin();
    const auto it_end = attr.end();
    for (; it != it_end;) {
        PLAJA_ASSERT(it_str != attr_str.end()) add_int_stats(*it, *it_str, init);
        ++it;
        ++it_str;
    }
}

void PLAJA::StatsBase::add_unsigned_stats(
    const std::list<StatsUnsigned>& attr,
    const std::list<std::string>& attr_str,
    unsigned init) {
    auto it = attr.begin();
    auto it_str = attr_str.begin();
    const auto it_end = attr.end();
    for (; it != it_end;) {
        PLAJA_ASSERT(it_str != attr_str.end()) add_unsigned_stats(*it, *it_str, init);
        ++it;
        ++it_str;
    }
}

void PLAJA::StatsBase::add_double_stats(
    const std::list<StatsDouble>& attr,
    const std::list<std::string>& attr_str,
    double init) {
    auto it = attr.begin();
    auto it_str = attr_str.begin();
    const auto it_end = attr.end();
    for (; it != it_end;) {
        PLAJA_ASSERT(it_str != attr_str.end()) add_double_stats(*it, *it_str, init);
        ++it;
        ++it_str;
    }
}

void PLAJA::StatsBase::add_string_stats(
    const std::list<StatsString>& attr,
    const std::list<std::string>& attr_str,
    const std::string& init) {
    auto it = attr.begin();
    auto it_str = attr_str.begin();
    const auto it_end = attr.end();
    for (; it != it_end;) {
        PLAJA_ASSERT(it_str != attr_str.end()) add_string_stats(*it, *it_str, init);
        ++it;
        ++it_str;
    }
}
