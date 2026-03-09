//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_COMMENTABLE_H
#define PLAJA_COMMENTABLE_H

#include <string>
#include "../../utils/default_constructors.h"

namespace PLAJA_UTILS { extern const std::string emptyString; }

// #define ENABLE_COMMENT_PARSING

/** Comment structure inherited by ast elements with an optional comment. */
class Commentable {

#ifdef ENABLE_COMMENT_PARSING

    protected:
        std::string comment;

        void copy_comment(const Commentable& other);

        explicit Commentable(std::string comment_ = PLAJA_UTILS::emptyString);

    public:
        void set_comment(std::string comment_);

#else

protected:
    inline void copy_comment(const Commentable& /*other*/) {}

    explicit Commentable(const std::string& comment_ = PLAJA_UTILS::emptyString);

public:
    void set_comment(const std::string& comment_);

#endif

    virtual ~Commentable() = 0;
    DELETE_CONSTRUCTOR(Commentable)

    [[nodiscard]] const std::string& get_comment() const;
};

/** macros ************************************************************************************************************/

#ifdef ENABLE_COMMENT_PARSING

#define FIELD_IF_COMMENT_PARSING(FCT) FCT
#define CONSTRUCT_IF_COMMENT_PARSING(CONSTRUCT) ,CONSTRUCT
#define COPY_IF_COMMENT_PARSING(COPY, FIELD) COPY->FIELD = FIELD;
#define SET_IF_COMMENT_PARSING(FIELD, VALUE) FIELD = VALUE;
#define GET_IF_COMMENT_PARSING(FIELD) FIELD
#define GET_IF_COMMENT_PARSING_ALT(FIELD, ALT) FIELD

#else

#define FIELD_IF_COMMENT_PARSING(FCT)
#define CONSTRUCT_IF_COMMENT_PARSING(CONSTRUCT)
#define COPY_IF_COMMENT_PARSING(COPY, FIELD)
#define SET_IF_COMMENT_PARSING(FIELD, VALUE) MAYBE_UNUSED(VALUE)
#define GET_IF_COMMENT_PARSING(FIELD) PLAJA_UTILS::emptyString
#define GET_IF_COMMENT_PARSING_ALT(FIELD, ALT) ALT

#endif

#endif //PLAJA_COMMENTABLE_H
