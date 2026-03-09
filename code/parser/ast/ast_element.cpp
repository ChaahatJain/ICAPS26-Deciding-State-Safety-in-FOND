//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "ast_element.h"
#include "../../exception/not_implemented_exception.h"
#include "../visitor/to_str/to_json_visitor.h"
#include "../visitor/to_str/to_str_visitor.h"

AstElement::AstElement() = default;

AstElement::~AstElement() = default;

std::unique_ptr<AstElement> AstElement::deep_copy_ast() const { throw NotImplementedException(__PRETTY_FUNCTION__); }

std::unique_ptr<AstElement> AstElement::move_ast() { throw NotImplementedException(__PRETTY_FUNCTION__); }

std::string AstElement::to_string(bool readable) const {

    if (readable) {
        auto rlt = ToStrVisitor::to_str(*this);
        if (rlt) { return *rlt; }
    }

    return ToJSON_Visitor::to_string(*this);
}

void AstElement::dump(bool readable) const {
    if (readable) { ToStrVisitor::dump(*this); } else { ToJSON_Visitor::dump(*this); }
}

void AstElement::write_to_file(const std::string& filename) const { ToJSON_Visitor::write_to_file(filename, *this); }
