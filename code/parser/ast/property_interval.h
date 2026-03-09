//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PROPERTY_INTERVAL_H
#define PLAJA_PROPERTY_INTERVAL_H

#include <memory>
#include "ast_element.h"

// forward declaration:
class Expression;

class PropertyInterval: public AstElement {
private:
    std::unique_ptr<Expression> lower;
    bool lowerExclusive;
    std::unique_ptr<Expression> upper;
    bool upperExclusive;
public:
    PropertyInterval();
    ~PropertyInterval() override;

    // setter:
    void set_lower(std::unique_ptr<Expression>&& lower_);
    inline void set_lowerExclusive(bool lower_ex) {lowerExclusive = lower_ex;}
    void set_upper(std::unique_ptr<Expression>&& upper_);
    inline void set_upperExclusive(bool upper_ex) {upperExclusive = upper_ex;}

    // getter:
    inline Expression* get_lower() {return lower.get();}
    inline const Expression* get_lower() const {return lower.get();}
    inline bool is_lowerExclusive() const {return lowerExclusive;}
    Expression* get_upper() {return upper.get();}
    const Expression* get_upper() const {return upper.get();}
    inline bool is_upperExclusive() const {return upperExclusive;}

    /**
     * Deep copy of a property interval.
     * @return
     */
    std::unique_ptr<PropertyInterval> deepCopy() const;

    // override:
    void accept(AstVisitor* astVisitor) override;
	void accept(AstVisitorConst* astVisitor) const override;
};


#endif //PLAJA_PROPERTY_INTERVAL_H
