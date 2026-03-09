//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_STATS_BASE_H
#define PLAJA_STATS_BASE_H

#include <list>
#include <unordered_map>
#include "../utils/utils.h"
#include "stats_int.h"
#include "stats_unsigned.h"
#include "stats_double.h"
#include "stats_string.h"


namespace PLAJA {

/*
 * Stats attributes can be collected in global enum classes (per value type).
 * The base stats class maintains an unordered map over the attributes.
 * Derived class shall init required attributes at construction time.
 */
class StatsBase {

private:
    std::unordered_map<StatsInt, int> statsInt;
    std::unordered_map<StatsUnsigned, unsigned> statsUnsigned;
    std::unordered_map<StatsDouble, double> statsDouble;
    std::unordered_map<StatsString, std::string> statsString;

    std::list<StatsInt> intAttrs;
    std::list<StatsUnsigned> unsignedAttrs;
    std::list<StatsDouble> doubleAttrs;
    std::list<StatsString> stringAttrs;
    //
    std::list<std::string> intAttrsStr;
    std::list<std::string> unsignedAttrsStr;
    std::list<std::string> doubleAttrsStr;
    std::list<std::string> stringAttrsStr;

protected:
    void init_aux(const std::list<StatsInt>& attributes, int value = 0);
    void init_aux(const std::list<StatsUnsigned>& attributes, unsigned value = 0);
    void init_aux(const std::list<StatsDouble>& attributes, double value = 0.0);
    void init_aux(const std::list<StatsString>& attributes, const std::string& value = "");

    void print_aux(const std::list<StatsInt>& attributes, const std::list<std::string>& attr_strs, bool skip_zero) const;
    void print_aux(const std::list<StatsUnsigned>& attributes, const std::list<std::string>& attr_strs, bool skip_zero) const;
    void print_aux(const std::list<StatsDouble>& attributes, const std::list<std::string>& attr_strs, bool skip_zero, double tolerance = 1.0e-2) const; // NOLINT(cppcoreguidelines-avoid-magic-numbers)
    void print_aux(const std::list<StatsString>& attributes, const std::list<std::string>& attr_strs) const;

    void stats_to_csv_aux(std::ofstream& file, const std::list<StatsInt>& attributes) const;
    void stats_to_csv_aux(std::ofstream& file, const std::list<StatsUnsigned>& attributes) const;
    void stats_to_csv_aux(std::ofstream& file, const std::list<StatsDouble>& attributes) const;
    void stats_to_csv_aux(std::ofstream& file, const std::list<StatsString>& attributes) const;


    static void stat_names_to_csv_aux(std::ofstream& file, const std::list<std::string>& attr_strs);

    inline void print_statistics_generic() const { print_aux(intAttrs, intAttrsStr, true); print_aux(unsignedAttrs, unsignedAttrsStr, true); print_aux(doubleAttrs, doubleAttrsStr, true); print_aux(stringAttrs, stringAttrsStr); }
    inline void stats_to_csv_generic(std::ofstream& file) const { stats_to_csv_aux(file, intAttrs); stats_to_csv_aux(file, unsignedAttrs); stats_to_csv_aux(file, doubleAttrs); stats_to_csv_aux(file, stringAttrs); }
    inline void stat_names_to_csv_generic(std::ofstream& file) const { stat_names_to_csv_aux(file, intAttrsStr); stat_names_to_csv_aux(file, unsignedAttrsStr); stat_names_to_csv_aux(file, doubleAttrsStr); stat_names_to_csv_aux(file, stringAttrsStr); }
    virtual void reset_generic() { init_aux(intAttrs, 0); init_aux(unsignedAttrs, 0); init_aux(doubleAttrs, 0); init_aux(stringAttrs, ""); }

    // output virtual
    virtual void print_statistics_specific() const {}
    virtual void stats_to_csv_specific(std::ofstream& /*file*/) const {}
    virtual void stat_names_to_csv_specific(std::ofstream& /*file*/) const {}
    virtual void reset_specific() {}

public:
    StatsBase();
    virtual ~StatsBase();
    DELETE_CONSTRUCTOR(StatsBase)

    virtual inline void reset() { reset_generic(); reset_specific(); }
    inline void clear() { statsInt.clear(); statsUnsigned.clear(); statsDouble.clear(); }

    inline void set_attr_int(StatsInt attr, int value) { statsInt[attr] = value; }
    inline void inc_attr_int(StatsInt attr, int inc = 1) { PLAJA_ASSERT(statsInt.count(attr)) statsInt[attr] += inc; }
    static void inc_attr_int(StatsBase* stats, StatsInt attr, int value = 1) { if (stats) { stats->inc_attr_int(attr, value); } }
    [[nodiscard]] inline int get_attr_int(StatsInt attr) const { return statsInt.at(attr); }

    inline void set_attr_unsigned(StatsUnsigned attr, unsigned value) { statsUnsigned[attr] = value; }
    inline void inc_attr_unsigned(StatsUnsigned attr, unsigned inc = 1) { PLAJA_ASSERT(statsUnsigned.count(attr)) statsUnsigned[attr] += inc; }
    static void inc_attr_unsigned(StatsBase* stats, StatsUnsigned attr, unsigned value = 1) { if (stats) { stats->inc_attr_unsigned(attr, value); } }
    [[nodiscard]] inline unsigned get_attr_unsigned(StatsUnsigned attr) const { return statsUnsigned.at(attr); }

    inline void set_attr_double(StatsDouble attr, double value) { statsDouble[attr] = value; }
    virtual inline void inc_attr_double(StatsDouble attr, double inc) { PLAJA_ASSERT(statsDouble.count(attr)) statsDouble[attr] += inc; }
    static void inc_attr_double(StatsBase* stats, StatsDouble attr, double value) { if (stats) { stats->inc_attr_double(attr, value); } }
    [[nodiscard]] inline double get_attr_double(StatsDouble attr) const { return statsDouble.at(attr); }

    inline void set_attr_string(StatsString attr, const std::string& value) { statsString[attr] = value; }
    [[nodiscard]] inline std::string get_attr_string(StatsString attr) const { return statsString.at(attr); }

    inline void add_int_stats(StatsInt attr, const std::string& attr_str, int init) { if (not statsInt.count(attr)) { intAttrs.push_back(attr); intAttrsStr.push_back(attr_str); set_attr_int(attr, init); } }
    inline void add_unsigned_stats(StatsUnsigned attr, const std::string& attr_str, unsigned init) { if (not statsUnsigned.count(attr)) { unsignedAttrs.push_back(attr); unsignedAttrsStr.push_back(attr_str); set_attr_unsigned(attr, init); } }
    inline void add_double_stats(StatsDouble attr, const std::string& attr_str, double init) { if (not statsDouble.count(attr)) { doubleAttrs.push_back(attr); doubleAttrsStr.push_back(attr_str); set_attr_double(attr, init); } }
    inline void add_string_stats(StatsString attr, const std::string& attr_str, std::string init) { if (not statsString.count(attr)) {stringAttrs.push_back(attr); stringAttrsStr.push_back(attr_str); set_attr_string(attr, init); } }

    void add_int_stats(const std::list<StatsInt>& attr, const std::list<std::string>& attr_str, int init);
    void add_unsigned_stats(const std::list<StatsUnsigned>& attr, const std::list<std::string>& attr_str, unsigned init);
    void add_double_stats(const std::list<StatsDouble>& attr, const std::list<std::string>& attr_str, double init);
    void add_string_stats(const std::list<StatsString>& attr, const std::list<std::string>& attr_str, const std::string& init);

    // output
    inline void print_statistics() const { print_statistics_specific(); print_statistics_generic(); }
    inline void stats_to_csv(std::ofstream& file) const { stats_to_csv_specific(file); stats_to_csv_generic(file); }
    inline void stat_names_to_csv(std::ofstream& file) const { stat_names_to_csv_specific(file); stat_names_to_csv_generic(file); }

};

}


#endif //PLAJA_STATS_BASE_H
