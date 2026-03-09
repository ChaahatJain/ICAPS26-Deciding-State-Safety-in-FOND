//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ACTION_H
#define PLAJA_ACTION_H

#include <memory>
#include "ast_element.h"
#include "commentable.h"
#include "../using_parser.h"

/** Class of a *non-silent/non-null* action. */
class Action final: public AstElement, public Commentable {
private:
    std::string name;
    ActionID_type id; // Defaults to invalid null action.
public:
    explicit Action(std::string name, ActionID_type id = ACTION::nullAction);
    ~Action() override;
    DELETE_CONSTRUCTOR(Action)

    /* Setter. */

    inline void set_name(std::string name_) { name = std::move(name_); }

    inline void set_id(ActionID_type action_id) { id = action_id; }

    /* Getter. */

    [[nodiscard]] inline const std::string& get_name() const { return name; }

    [[nodiscard]] inline ActionID_type get_id() const { return id; }

    [[nodiscard]] std::unique_ptr<Action> deepCopy() const;

    /* Override. */
    void accept(AstVisitor* astVisitor) override;
    void accept(AstVisitorConst* astVisitor) const override;
};

namespace ACTION { extern const std::string silentName; }

#endif //PLAJA_ACTION_H
