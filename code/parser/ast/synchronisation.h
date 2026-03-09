//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SYNCHRONISATION_H
#define PLAJA_SYNCHRONISATION_H

#include <utility>
#include <vector>
#include <memory>
#include "ast_element.h"
#include "commentable.h"
#include "../using_parser.h"

// forward declaration
class Action;

/**
 * Composition substructure.
 */
class Synchronisation: public AstElement, public Commentable {
private:
    std::vector<std::pair<std::string, ActionID_type>> synchronise;
    std::string resultAction;
    ActionID_type resultActionID;
    SyncID_type syncID; // ID assigned to this sync (currently by ModelInformation); 0 is reserved for "silent" sync
public:
    Synchronisation();
    ~Synchronisation() override;

    // construction:
    void reserve(std::size_t sync_cap);
    void add_sync(const std::string& sync_action, ActionID_type sync_action_id = ACTION::silentAction);

    // setter:
    [[maybe_unused]] inline void set_syncAction(std::string sync_action, AutomatonIndex_type automaton_index) { synchronise[automaton_index].first = std::move(sync_action); }
    [[maybe_unused]] inline void set_syncActionID(ActionID_type sync_action_id, AutomatonIndex_type automaton_index) { synchronise[automaton_index].second = sync_action_id; }
    inline void set_result(std::string rlt_act) { resultAction = std::move(rlt_act); }
    inline void set_resultID(ActionID_type rlt_act_id) { resultActionID = rlt_act_id; }
    inline void set_syncID(SyncID_type sync_id) { syncID = sync_id; }

    // getter:
    [[nodiscard]] inline std::size_t get_size_synchronise() const { return synchronise.size(); }
    [[nodiscard]] inline const std::string& get_syncAction(AutomatonIndex_type automaton_index) const { PLAJA_ASSERT(automaton_index < synchronise.size()) return synchronise[automaton_index].first; }
    [[nodiscard]] inline ActionID_type get_syncActionID(AutomatonIndex_type automaton_index) const { PLAJA_ASSERT(automaton_index < synchronise.size()) return synchronise[automaton_index].second; }
    [[nodiscard]] inline const std::vector<std::pair<std::string, ActionID_type>>& _synchronise() const { return synchronise; }
    [[nodiscard]] inline const std::string& get_result() const { return resultAction; }
    [[nodiscard]] inline ActionID_type get_resultID() const { return resultActionID; }
    [[nodiscard]] inline SyncID_type get_syncID() const { return syncID; }

    /**
     * Deep copy of a synchronisation.
     * @return
     */
    [[nodiscard]] std::unique_ptr<Synchronisation> deepCopy() const;

    // override:
    void accept(AstVisitor* astVisitor) override;
	void accept(AstVisitorConst* astVisitor) const override;
};


#endif //PLAJA_SYNCHRONISATION_H
