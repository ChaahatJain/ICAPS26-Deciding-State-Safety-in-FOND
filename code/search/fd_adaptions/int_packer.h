#ifndef INT_PACKER_H
#define INT_PACKER_H

/*FAST DOWNWARD CLASS*************************************************************************************************/

#include <vector>
#include <cstdlib>
#include "../../exception/runtime_exception.h" // plaja add-on
#include "../../utils/utils.h" // plaja add-on

/*
  Utility class to pack lots of unsigned integers (called "variables"
  in the code below) with a small domain {0, ..., range - 1}
  tightly into memory. This works like a bitfield except that the
  fields and sizes don't need to be known at compile time.

  For example, if we have 40 binary variables and 20 variables with
  range 4, storing them would theoretically require at least 80 bits,
  and this class would pack them into 12 bytes (three 4-byte "bins").

  Uses a greedy bin-packing strategy to pack the variables, which
  should be close to optimal in most cases. (See code comments for
  details.)
*/

/* PLAJA adaption: reworked signed vs unsigned int usage throughout the class. */

class IntPacker {
public:
    typedef unsigned int Bin;
    typedef unsigned int BinIndex_type;
    typedef unsigned int BinValue_type;
    typedef unsigned int Var_type;
    typedef int Value_type;
    typedef unsigned int Shift_type;
    typedef unsigned int Range_type;

private:
    // auxiliary class:
    class VariableInfo {
        BinIndex_type bin_index;
        Shift_type shift;
#ifndef NDEBUG
        Range_type range;
#endif
        Bin read_mask;
        Bin clear_mask;
    public:
        VariableInfo(BinIndex_type bin_index_, Shift_type shift_, Range_type range_);
        VariableInfo();
        ~VariableInfo();

        inline BinValue_type get(const Bin* buffer) const { return (buffer[bin_index] & read_mask) >> shift; }

        inline void set(Bin* buffer, BinValue_type value) const {
            PLAJA_ASSERT(PLAJA_UTILS::is_non_negative(value))
            PLAJA_ASSERT(value < range);
            Bin& bin = buffer[bin_index];
            bin = (bin & clear_mask) | (value << shift);
        }
    };

    std::vector<VariableInfo> var_infos;
    unsigned int num_bins;
    const std::vector<int> lowerBounds; // plaja: lower and upper bounds
    const std::vector<int> upperBounds;
    const Bin* binConstructor; // plaja: memset to 0 to properly maintain set, i.e., hash and detect duplicates

    unsigned int pack_one_bin(const std::vector<Range_type>& ranges, std::vector<std::vector<Var_type>>& bits_to_vars);
    Bin* pack_bins(const std::vector<Range_type>& ranges); // PlaJA: return bin-constructor as hack to call during construction

public:
    IntPacker(const std::vector<Range_type>& ranges, std::vector<Value_type> lower_bounds, std::vector<Value_type> upper_bounds);
    ~IntPacker();

    [[nodiscard]] inline unsigned int get_num_bins() const { return num_bins; }

    inline static std::size_t get_bin_size_in_bytes() { return sizeof(Bin); }

    [[nodiscard]] inline std::size_t get_state_size() const { return lowerBounds.size(); }

    [[nodiscard]] inline const Bin* get_bin_constructor() const { return binConstructor; }

    inline int get(const Bin* buffer, Var_type var) const {
        const auto value_unsigned = var_infos[var].get(buffer);
        PLAJA_ASSERT(PLAJA_UTILS::cast_numeric<Value_type>(value_unsigned) + lowerBounds[var] <= upperBounds[var])
        return PLAJA_UTILS::cast_numeric<Value_type>(value_unsigned) + lowerBounds[var];
    }

    FCT_IF_RUNTIME_CHECKS(std::string gen_runtime_error_str(Var_type var, Value_type value) const;)

    inline void set(Bin* buffer, Var_type var, Value_type value) const {
        // NOTE: Runtime exceptions can be caught to just ignore out of bounds, e.g., in context of abstractions.
        RUNTIME_ASSERT(lowerBounds[var] <= value && value <= upperBounds[var], gen_runtime_error_str(var, value))
        const Value_type value_unsigned = value - lowerBounds[var];
        PLAJA_ASSERT(PLAJA_UTILS::is_non_negative(value_unsigned))
        var_infos[var].set(buffer, value_unsigned);
    }

};

#endif
