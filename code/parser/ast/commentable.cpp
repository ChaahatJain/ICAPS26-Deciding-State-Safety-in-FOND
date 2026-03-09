//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "commentable.h"

Commentable::~Commentable() = default;

#ifdef ENABLE_COMMENT_PARSING

Commentable::Commentable(std::string comment_):
    comment(std::move(comment_)) {
}

void Commentable::set_comment(std::string comment_) { comment = std::move(comment_); }

const std::string& Commentable::get_comment() const { return comment; }

void Commentable::copy_comment(const Commentable& other) { set_comment(other.get_comment()); }

#else

Commentable::Commentable(const std::string& /*comment*/) {}

void Commentable::set_comment(const std::string& /*comment*/) {}

const std::string& Commentable::get_comment() const { return PLAJA_UTILS::emptyString; } // NOLINT(readability-convert-member-functions-to-static)

#endif



