//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_COMPOSITION_ELEMENT_H
#define PLAJA_COMPOSITION_ELEMENT_H

#include <vector>
#include <memory>
#include "ast_element.h"
#include "commentable.h"
#include "../using_parser.h"

// forward-declaration:
class Action;
class Automaton;

class CompositionElement: public AstElement, public Commentable {
private:
    std::string elementAutomaton;
    std::vector<std::pair<std::string, ActionID_type>> inputEnables; // yet uncertain what to use (name vs. id), currently unused anyway
public:
    explicit CompositionElement(std::string element_automaton);
    ~CompositionElement() override;

    // construction:
    void reserve(std::size_t input_enables_cap);
    void add_inputEnable(const std::string& input_enable, ActionID_type input_enabled_id = ACTION::nullAction);

    // setter:
    inline void set_elementAutomaton(std::string element_automaton) { elementAutomaton = std::move(element_automaton); }
    inline void set_inputEnabledID(ActionID_type inputEnabledID, std::size_t index) { PLAJA_ASSERT(index < inputEnables.size()) inputEnables[index].second = inputEnabledID; }

    // getter:
    [[nodiscard]] inline std::string get_elementAutomaton() const { return elementAutomaton; }
    [[nodiscard]] inline std::size_t get_number_inputEnables() const { return inputEnables.size(); }
    [[nodiscard]] inline const std::string& get_inputEnable(std::size_t index) const { PLAJA_ASSERT(index < inputEnables.size()) return inputEnables[index].first; }
    [[nodiscard]] inline ActionID_type get_inputEnableID(std::size_t index) const { PLAJA_ASSERT(index < inputEnables.size()) return inputEnables[index].second; }
    [[nodiscard]] inline const std::vector<std::pair<std::string, ActionID_type>>& _input_enables() const { return inputEnables; }

    /**
     * Deep copy of a composition element.
     * @return
     */
    [[nodiscard]] std::unique_ptr<CompositionElement> deepCopy() const;

    // override:
    void accept(AstVisitor* astVisitor) override;
	void accept(AstVisitorConst* astVisitor) const override;
};


#endif //PLAJA_COMPOSITION_ELEMENT_H
