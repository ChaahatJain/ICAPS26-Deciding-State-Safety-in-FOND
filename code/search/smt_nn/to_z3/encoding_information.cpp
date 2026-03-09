//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <unordered_map>
#include "encoding_information.h"
#include "../../../option_parser/option_parser.h"

// extern:
namespace PLAJA_OPTION {
    extern const std::string mip_encoding;
    extern const std::string bound_encoding;
}

namespace MARABOU_TO_Z3 {

    const std::unordered_map<std::string, EncodingInformation::BoundEncoding> stringToBoundEncoding{ { "all", EncodingInformation::ALL }, { "skip_fixed_relu_inputs", EncodingInformation::SKIP_FIXED_RELU_INPUTS }, { "only_relu_outputs", EncodingInformation::ONLY_RELU_OUTPUTS }, { "none", EncodingInformation::NONE }  }; // NOLINT(cert-err58-cpp)

    std::string print_supported_bound_encodings() { return PLAJA::OptionParser::print_supported_options(stringToBoundEncoding); }

    std::unique_ptr<EncodingInformation> EncodingInformation::construct_encoding_information(const PLAJA::OptionParser& option_parser) {
        return std::unique_ptr<EncodingInformation>( new EncodingInformation(
                option_parser.get_option_value(stringToBoundEncoding, PLAJA_OPTION::bound_encoding, EncodingInformation::NONE),
                option_parser.is_flag_set(PLAJA_OPTION::mip_encoding)) );
    }

}