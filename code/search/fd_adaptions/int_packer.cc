

#include <cstring> // plaja to std::memset
#include <utility>
#include "int_packer.h"
#include "../../parser/ast/model.h"

static const unsigned int BITS_PER_BIN = sizeof(IntPacker::Bin) * 8;

static IntPacker::Bin get_bit_mask(IntPacker::Shift_type from, IntPacker::Shift_type to) {
    // Return mask with all bits in the range [from, to) set to 1.
    PLAJA_ASSERT(PLAJA_UTILS::is_non_negative(from))
    PLAJA_ASSERT(PLAJA_UTILS::is_non_negative(to))
    PLAJA_ASSERT(from <= to)
    PLAJA_ASSERT(to <= BITS_PER_BIN)
    unsigned int length = to - from;
    if (length == BITS_PER_BIN) {
        // 1U << BITS_PER_BIN has undefined behaviour in C++; e.g.
        // 1U << 32 == 1 (not 0) on 32-bit Intel platforms. Hence this
        // special case.
        assert(from == 0 && to == BITS_PER_BIN);
        return ~IntPacker::Bin(0);
    } else {
        return ((IntPacker::Bin(1) << length) - 1) << from;
    }
}

static unsigned int get_bit_size_for_range(IntPacker::Range_type range) {
    if (range <= 1) { return 1; } // plaja: fix for special case one-value location
    unsigned int num_bits = 0;
    while ((1U << num_bits) < range) { ++num_bits; }
    return num_bits;
}

inline IntPacker::Bin* generate_bin_constructor(const IntPacker& state_packer) {
    auto* state_constructor = new IntPacker::Bin[state_packer.get_num_bins()];
    std::memset(state_constructor, 0, state_packer.get_num_bins() * IntPacker::get_bin_size_in_bytes()); // plaja: memset to 0 to properly maintain state set, i.e., hash and detect duplicates
    return state_constructor;
}

// VariableInfo:

IntPacker::VariableInfo::VariableInfo(BinIndex_type bin_index_, Shift_type shift_, Range_type range_):
    bin_index(bin_index_)
    , shift(shift_)
    CONSTRUCT_IF_DEBUG(range(range_)) {
    auto bit_size = get_bit_size_for_range(range_);
    read_mask = get_bit_mask(shift, shift + bit_size);
    clear_mask = ~read_mask;
}

IntPacker::VariableInfo::VariableInfo():
    bin_index(static_cast<BinIndex_type>(-1))
    , shift(0)
    CONSTRUCT_IF_DEBUG(range(static_cast<Range_type>(-1)))
    , read_mask(0)
    , clear_mask(0) {} // Default constructor needed for resize() in pack_bins.

IntPacker::VariableInfo::~VariableInfo() = default;

// IntPacker:

IntPacker::IntPacker(const std::vector<Range_type>& ranges, std::vector<Value_type> lower_bounds, std::vector<Value_type> upper_bounds):
    num_bins(0)
    , lowerBounds(std::move(lower_bounds))
    , upperBounds(std::move(upper_bounds))
    , binConstructor(pack_bins(ranges)) {
}

IntPacker::~IntPacker() { delete[] binConstructor; }

IntPacker::Bin* IntPacker::pack_bins(const std::vector<Range_type>& ranges) {
    assert(var_infos.empty());

    const std::size_t num_vars = ranges.size();
    var_infos.resize(num_vars);

    // bits_to_vars[k] contains all variables that require exactly k
    // bits to encode. Once a variable is packed into a bin, it is
    // removed from this index.
    // Loop over the variables in reverse order to prefer variables with
    // low indices in case of ties. This might increase cache-locality.
    std::vector<std::vector<Var_type>> bits_to_vars(BITS_PER_BIN + 1);
    for (int var = PLAJA_UTILS::cast_numeric<int>(num_vars) - 1; var >= 0; --var) {
        auto bits = get_bit_size_for_range(ranges[var]);
        assert(bits <= BITS_PER_BIN);
        bits_to_vars[bits].push_back(var);
    }

    std::size_t packed_vars = 0;
    while (packed_vars != num_vars) { packed_vars += pack_one_bin(ranges, bits_to_vars); }

    return generate_bin_constructor(*this); // PlaJA: return bin constructor:
}

unsigned int IntPacker::pack_one_bin(const std::vector<Range_type>& ranges, std::vector<std::vector<Var_type>>& bits_to_vars) {
    // Returns the number of variables added to the bin. We pack each
    // bin with a greedy strategy, always adding the largest variable
    // that still fits.

    ++num_bins;
    BinIndex_type bin_index = num_bins - 1;
    unsigned int used_bits = 0;
    unsigned int num_vars_in_bin = 0;

    while (true) {
        // Determine size of largest variable that still fits into the bin.
        PLAJA_ASSERT(used_bits <= BITS_PER_BIN)
        unsigned int bits = BITS_PER_BIN - used_bits;
        while (bits > 0 && bits_to_vars[bits].empty()) { --bits; }

        if (bits == 0) {
            // No more variables fit into the bin.
            // (This also happens when all variables have been packed.)
            return num_vars_in_bin;
        }

        // We can pack another variable of size bits into the current bin.
        // Remove the variable from bits_to_vars and add it to the bin.
        auto& best_fit_vars = bits_to_vars[bits];
        Var_type var = best_fit_vars.back();
        best_fit_vars.pop_back();

        var_infos[var] = VariableInfo(bin_index, used_bits, ranges[var]);
        used_bits += bits;
        ++num_vars_in_bin;
    }
}

#ifdef RUNTIME_CHECKS

namespace PLAJA_GLOBAL { extern const Model* currentModel; }

std::string IntPacker::gen_runtime_error_str(Var_type var, Value_type value) const {
    return "out-of-bounds assignment: " + PLAJA_GLOBAL::currentModel->gen_state_index_str(var) + " <- " + std::to_string(value) + " out of bounds (" + std::to_string(lowerBounds[var]) + ", " + std::to_string(upperBounds[var]) + ").";
}

#endif
