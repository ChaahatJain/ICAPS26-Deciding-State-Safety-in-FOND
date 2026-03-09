//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ENCODING_INFORMATION_H
#define PLAJA_ENCODING_INFORMATION_H

#include <memory>

// forward declaration:
namespace MARABOU_IN_PLAJA { struct PreprocessedBounds; }
namespace PLAJA { class OptionParser; }

namespace MARABOU_TO_Z3 {

    /**
    * Information how to encode NN in Z3 (using bounds).
    */
    struct EncodingInformation {
    public:
        enum BoundEncoding {
            ALL,
            SKIP_FIXED_RELU_INPUTS, // skip input bounds if the activation case is fixed
            ONLY_RELU_OUTPUTS, // only the output bounds after ReLU
            NONE,
        };
    private:
        BoundEncoding boundEncoding; // which bounds to provide to Z3
        bool mipEncoding;
        const MARABOU_IN_PLAJA::PreprocessedBounds* preprocessedBounds;

        EncodingInformation(BoundEncoding bound_encoding, bool mip_encoding): boundEncoding(bound_encoding), mipEncoding(mip_encoding), preprocessedBounds(nullptr) {}
    public:
        ~EncodingInformation() = default;

        static std::unique_ptr<EncodingInformation> construct_encoding_information(const PLAJA::OptionParser& option_parser);
        //
        inline void set_bound_encoding(BoundEncoding bound_encoding) { boundEncoding = bound_encoding; };
        inline void set_mip_encoding(bool mip_encoding) { mipEncoding = mip_encoding; }
        inline void set_preprocessed_bounds(const MARABOU_IN_PLAJA::PreprocessedBounds* preprocessed_bounds) { preprocessedBounds = preprocessed_bounds; }

        [[nodiscard]] inline BoundEncoding _bound_encoding() const { return boundEncoding; }
        [[nodiscard]] inline bool _mip_encoding() const { return mipEncoding; }
        [[nodiscard]] inline const MARABOU_IN_PLAJA::PreprocessedBounds* _preprocessed_bounds() const { return preprocessedBounds; }

    };

    extern std::string print_supported_bound_encodings();

}


#endif //PLAJA_ENCODING_INFORMATION_H
