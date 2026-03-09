//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_METADATA_H
#define PLAJA_METADATA_H


#include <memory>
#include "ast_element.h"

class Metadata: public AstElement {
private:
    std::string version;
    std::string author;
    std::string description;
    std::string doi;
    std::string url;
public:
    Metadata();
    ~Metadata() override;

    // setter:
    [[maybe_unused]] void set_version(std::string version_);
    [[maybe_unused]] void set_author(std::string author_);
    void set_description(std::string description_);
    [[maybe_unused]] void set_doi(std::string doi_);
    void set_url(std::string url_);

    // getter:
    const std::string& getVersion() const;
    const std::string& getAuthor() const;
    const std::string& getDescription() const;
    const std::string& getDoi() const;
    const std::string& getUrl() const;

    /**
     * Deep copy of a metadata pack.
     * @return
     */
    std::unique_ptr<Metadata> deepCopy() const;

    // override:
    void accept(AstVisitor* astVisitor) override;
	void accept(AstVisitorConst* astVisitor) const override;
};


#endif //PLAJA_METADATA_H
