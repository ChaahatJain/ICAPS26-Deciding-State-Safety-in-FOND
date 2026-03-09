//
// This file is part of the PlaJA code base (2019 - 2021).
//

#ifndef PLAJA_VERITAS_ENCODING_INFORMATION_H
#define PLAJA_VERITAS_ENCODING_INFORMATION_H

#include <memory>

// forward declaration:
namespace PLAJA { class OptionParser; }

namespace VERITAS_TO_Z3 {

    /**
    * Information how to encode Ensemble in Z3 (using bounds).
    */
    struct EncodingInformation {
    public:
        enum BoundEncoding {
            ALL,
            NONE,
        };
    private:
        BoundEncoding boundEncoding; // which bounds to provide to Z3
        bool mipEncoding;

        EncodingInformation(BoundEncoding bound_encoding, bool mip_encoding): boundEncoding(bound_encoding), mipEncoding(mip_encoding) {}
    public:
        ~EncodingInformation() = default;

        static std::unique_ptr<EncodingInformation> construct_encoding_information(const PLAJA::OptionParser& option_parser);
        //
        inline void set_bound_encoding(BoundEncoding bound_encoding) { boundEncoding = bound_encoding; };
        inline void set_mip_encoding(bool mip_encoding) { mipEncoding = mip_encoding; }

        [[nodiscard]] inline BoundEncoding _bound_encoding() const { return boundEncoding; }
        [[nodiscard]] inline bool _mip_encoding() const { return mipEncoding; }
    };

    extern std::string print_supported_bound_encodings();

}


#endif //PLAJA_VERITAS_ENCODING_INFORMATION_H
