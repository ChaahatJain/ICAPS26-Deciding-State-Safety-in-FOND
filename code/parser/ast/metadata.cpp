//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "metadata.h"

#include <utility>
#include "../visitor/ast_visitor.h"
#include "../visitor/ast_visitor_const.h"
#include "commentable.h"

Metadata::Metadata():
    version(PLAJA_UTILS::emptyString),
    author(PLAJA_UTILS::emptyString),
    description(PLAJA_UTILS::emptyString),
    doi(PLAJA_UTILS::emptyString),
    url(PLAJA_UTILS::emptyString) {
}

Metadata::~Metadata() = default;

// setter:

[[maybe_unused]] void Metadata::set_version(std::string version_) {
    this->version = std::move(version_);
}

[[maybe_unused]] void Metadata::set_author(std::string author_) {
    this->author = std::move(author_);
}

void Metadata::set_description(std::string description_) {
    this->description = std::move(description_);
}

[[maybe_unused]] void Metadata::set_doi(std::string doi_) {
    this->doi = std::move(doi_);
}

void Metadata::set_url(std::string url_) {
    this->url = std::move(url_);
}

// getter:

const std::string& Metadata::getVersion() const {
    return version;
}

const std::string& Metadata::getAuthor() const {
    return author;
}

const std::string& Metadata::getDescription() const {
    return description;
}

const std::string& Metadata::getDoi() const {
    return doi;
}

const std::string& Metadata::getUrl() const {
    return url;
}

//

std::unique_ptr<Metadata> Metadata::deepCopy() const {
    std::unique_ptr<Metadata> copy(new Metadata());
    copy->version = version;
    copy->author = author;
    copy->description = description;
    copy->doi = doi;
    copy->url = url;
    return copy;
}

// override:

void Metadata::accept(AstVisitor* astVisitor) {
    astVisitor->visit(this);
}

void Metadata::accept(AstVisitorConst* astVisitor) const {
    astVisitor->visit(this);
}



