//
// This file is part of the PlaJA code base (2019 - 2022).
//

#ifndef PLAJA_PA_MATCH_TREE_H
#define PLAJA_PA_MATCH_TREE_H

#include <list>
#include <memory>
#include <vector>
#include "../../../utils/utils.h"
#include "../../../assertions.h"
#include "../../factories/forward_factories.h"
#include "../optimization/forward_optimization_pa.h"
#include "../pa_states/abstract_state.h"
#include "forward_inc_state_space_pa.h"

/**********************************************************************************************************************/

struct PaMtNodeInformation {
public:
    PaMtNodeInformation();
    virtual ~PaMtNodeInformation() = 0;
    DELETE_CONSTRUCTOR(PaMtNodeInformation)
};

/**********************************************************************************************************************/

class PAMatchTree final {
    friend PA_MATCH_TREE::Traverser;

    friend class PredicateTraverser;

public:

    struct Node final {
        friend PAMatchTree;
        friend PA_MATCH_TREE::TraverserConstructor;

    private:
        std::unique_ptr<PaMtNodeInformation> information;
        std::unique_ptr<Node> posNode;
        std::unique_ptr<Node> negNode;

        FIELD_IF_LAZY_PA(PredicateIndex_type predicateIndex;) // std::vector<PredicateIndex_type> predicateClosure;

        FCT_IF_LAZY_PA(
            void set_predicate_index(PredicateIndex_type predicate_index) {
                PLAJA_ASSERT(predicateIndex == PREDICATE::none)
                predicateIndex = predicate_index;
            };
        )

    public:
        Node();
        ~Node();
        DELETE_CONSTRUCTOR(Node)

        FCT_IF_LAZY_PA([[nodiscard]] PredicateIndex_type get_predicate_index() const { return predicateIndex; };)

        inline void set_information(std::unique_ptr<PaMtNodeInformation>&& info) { information = std::move(info); }

        [[nodiscard]] Node& add_successor(bool value);

        // FCT_IF_LAZY_PA(void set_predicate_closure(std::vector<PredicateIndex_type>&& predicate_closure);)

        // FCT_IF_LAZY_PA([[nodiscard]] inline const std::vector<PredicateIndex_type>& get_closure() const { return predicateClosure; })

        // FCT_IF_LAZY_PA([[nodiscard]] inline std::vector<PredicateIndex_type>& get_closure() { return predicateClosure; })

        // FCT_IF_LAZY_PA(FCT_IF_DEBUG(void dump_closure() const;))

        template<typename PaMtNodeInformation_t = PaMtNodeInformation>
        [[nodiscard]] inline const PaMtNodeInformation_t* get_information() const { return PLAJA_UTILS::cast_ptr<PaMtNodeInformation_t>(information.get()); }

        template<typename PaMtNodeInformation_t = PaMtNodeInformation>
        [[nodiscard]] inline PaMtNodeInformation_t* get_information() { return PLAJA_UTILS::cast_ptr<PaMtNodeInformation_t>(information.get()); }

        [[nodiscard]] inline const Node* access_successor(bool value) const { return value ? posNode.get() : negNode.get(); }

        [[nodiscard]] inline Node* access_successor(bool value) { return value ? posNode.get() : negNode.get(); }

        [[nodiscard]] inline const Node& get_successor(bool value) const {
            PLAJA_ASSERT(access_successor(value))
            return *access_successor(value);
        }

        [[nodiscard]] inline Node& get_successor(bool value) {
            PLAJA_ASSERT(access_successor(value))
            return *access_successor(value);
        }

        /** Some stats. */
        void compute_size(PA_MATCH_TREE::MtSize& mt_size, unsigned depth) const;

    };

private:
    const PredicatesExpression* predicates;
    FCT_IF_LAZY_PA(std::list<PredicateIndex_type> globalPredicates;)
    std::shared_ptr<PredicateRelations> predicateRelations;
    std::unique_ptr<Node> root;

    /** Auxiliary to extend MT recursively. */
    FCT_IF_LAZY_PA(void set_closure(Node& current_node, const std::vector<PredicateIndex_type>& predicate_closure, std::size_t index, PredicateState* prefix_state);)

public:
    explicit PAMatchTree(const PLAJA::Configuration& config, const PredicatesExpression& predicates);
    ~PAMatchTree();
    DELETE_CONSTRUCTOR(PAMatchTree)

    [[nodiscard]] std::size_t get_size_predicates() const;

    [[nodiscard]] inline const PredicatesExpression& get_predicates() const {
        PLAJA_ASSERT(predicates)
        return *predicates;
    }

    [[nodiscard]] inline const std::list<PredicateIndex_type>& get_global_predicates() const { // NOLINT(readability-convert-member-functions-to-static)
#ifdef LAZY_PA
        return globalPredicates;
#else
        PLAJA_ABORT
#endif
    }

    [[nodiscard]] inline const PredicateRelations* get_predicate_relations() const { return predicateRelations.get(); }

    void increment();

    void add_global_predicate(PredicateIndex_type predicate_index);

    void set_closure(const PaStateBase& pa_state, std::vector<PredicateIndex_type>&& predicate_closure);

    /** Some stats. Time consumption linear in size of tree. */
    [[nodiscard]] PA_MATCH_TREE::MtSize compute_size() const;

    /******************************************************************************************************************/

    [[nodiscard]] Node& add_path(const AbstractState& abstract_state);

    [[nodiscard]] Node& access_node(const AbstractState& abstract_state);

    template<typename PaMtNodeInformation_t = PaMtNodeInformation>
    [[nodiscard]] inline PaMtNodeInformation_t* get_information(const AbstractState& abstract_state) { return access_node(abstract_state).get_information<PaMtNodeInformation_t>(); }

    template<typename PaMtNodeInformation_t = PaMtNodeInformation>
    [[nodiscard]] inline PaMtNodeInformation_t& access_information(const AbstractState& abstract_state) {
        auto* info = get_information<PaMtNodeInformation_t>(abstract_state);
        PLAJA_ASSERT(info)
        return *info;
    }

    [[nodiscard]] const Node& retrieve_node(const AbstractState& abstract_state) const;

    template<typename PaMtNodeInformation_t = PaMtNodeInformation>
    [[nodiscard]] inline const PaMtNodeInformation_t* get_information(const AbstractState& abstract_state) const { return retrieve_node(abstract_state).get_information<PaMtNodeInformation_t>(); }

    template<typename PaMtNodeInformation_t = PaMtNodeInformation>
    [[nodiscard]] inline const PaMtNodeInformation_t* retrieve_information(const AbstractState& abstract_state) const {
        auto* info = get_information<PaMtNodeInformation_t>(abstract_state);
        PLAJA_ASSERT(info)
        return *info;
    }

};

/** iterators *********************************************************************************************************/

namespace PA_MATCH_TREE {

    class Traverser {
        friend PAMatchTree;

    protected:
        const PAMatchTree* matchTree;
        PAMatchTree::Node* currentNode;
        std::unique_ptr<PredicateState> mtPrefix;

#ifdef LAZY_PA
        // std::list<PredicateIndex_type> closure;
#else
        PredicateIndex_type currentIndex;
#endif
        FIELD_IF_DEBUG(std::list<PredicateIndex_type> closurePrefix;)

        // void update();
        // void update_entailed();

        explicit Traverser(const PAMatchTree& match_tree);
    public:
        virtual ~Traverser();
        DELETE_CONSTRUCTOR(Traverser)

        virtual void next(const PaStateBase& pa_state);

        [[nodiscard]] inline bool end() const { return not currentNode; }

        [[nodiscard]] inline bool traversed(const PaStateBase& pa_state) const { return predicate_index() == PREDICATE::none or (PLAJA_GLOBAL::lazyPA and not pa_state.is_set(predicate_index())); }

        [[nodiscard]] inline PredicateIndex_type predicate_index() const {
#ifdef LAZY_PA
            return node().get_predicate_index(); // closure.front();
#else
            return currentIndex;
#endif
        }

        [[nodiscard]] inline const PredicateState& get_prefix_state() const {
            PLAJA_ASSERT(mtPrefix)
            return *mtPrefix;
        }

        [[nodiscard]] inline const PAMatchTree::Node& node() const {
            PLAJA_ASSERT(currentNode)
            return *currentNode;
        }

        template<typename PaMtNodeInformation_t = PaMtNodeInformation>
        [[nodiscard]] inline const PaMtNodeInformation_t* get_information() const { return node().template get_information<PaMtNodeInformation_t>(); }

        // debug:

        FCT_IF_DEBUG([[nodiscard]] bool is_final() const;)

        // FCT_IF_DEBUG([[nodiscard]] inline const std::list<PredicateIndex_type>& get_closure() const { return closure; })

        FCT_IF_DEBUG([[nodiscard]] inline const std::list<PredicateIndex_type>& get_prefix() const { return closurePrefix; })

        FCT_IF_DEBUG([[nodiscard]] std::size_t get_prefix_size() const;)

        FCT_IF_DEBUG([[nodiscard]] std::size_t get_path_size() const;) // i.e., the size of the node path (possibly skipping entailed closure predicates contributing to prefix size)

        FCT_IF_DEBUG([[nodiscard]] std::size_t get_size_entailed_closure_predicates() const;)

        FCT_IF_DEBUG([[nodiscard]] inline bool is_sane() const { return get_path_size() + get_size_entailed_closure_predicates() == get_prefix_size(); })

        FCT_IF_DEBUG([[nodiscard]] bool matches_prefix(const PaStateBase& pa_state) const;)

        FCT_IF_DEBUG([[nodiscard]] bool equals_prefix(const PaStateBase& pa_state) const;)

        FCT_IF_DEBUG(void dump_closure() const;)

        FCT_IF_DEBUG(void dump_predicates_prefix() const;)

        FCT_IF_DEBUG(void dump_prefix_state() const;)

    };

    class TraverserConst final: public Traverser {
        friend PAMatchTree;

        explicit TraverserConst(const PAMatchTree& match_tree);
    public:
        ~TraverserConst() final;
        DELETE_CONSTRUCTOR(TraverserConst)

        [[nodiscard]] inline static TraverserConst init(const PAMatchTree& mt) { return TraverserConst(mt); }

    };

    class TraverserModifier final: public Traverser {
        friend PAMatchTree;

        explicit TraverserModifier(PAMatchTree& match_tree);
    public:
        ~TraverserModifier() final;
        DELETE_CONSTRUCTOR(TraverserModifier)

        [[nodiscard]] inline static TraverserModifier init(PAMatchTree& mt) { return TraverserModifier(mt); }

        [[nodiscard]] inline PAMatchTree::Node& node() {
            PLAJA_ASSERT(currentNode)
            return *currentNode;
        }

        template<typename PaMtNodeInformation_t = PaMtNodeInformation>
        [[nodiscard]] inline PaMtNodeInformation_t* get_information() { return node().template get_information<PaMtNodeInformation_t>(); }

    };

    class TraverserConstructor final: public Traverser {
        friend PAMatchTree;

    private:
        FIELD_IF_LAZY_PA(std::list<PredicateIndex_type> closure;)

        void update();

        explicit TraverserConstructor(PAMatchTree& match_tree);
    public:
        ~TraverserConstructor() final;
        DELETE_CONSTRUCTOR(TraverserConstructor)

        [[nodiscard]] inline static TraverserConstructor init(PAMatchTree& mt) { return TraverserConstructor(mt); }

        void next(const PaStateBase& pa_state) final;

        [[nodiscard]] inline PAMatchTree::Node& node() {
            PLAJA_ASSERT(currentNode)
            return *currentNode;
        }

        template<typename PaMtNodeInformation_t = PaMtNodeInformation>
        [[nodiscard]] inline PaMtNodeInformation_t* get_information() { return node().template get_information<PaMtNodeInformation_t>(); }

        FCT_IF_DEBUG(FCT_IF_LAZY_PA([[nodiscard]] inline const std::list<PredicateIndex_type>& get_closure() const { return closure; }))

    };

}

#endif //PLAJA_PA_MATCH_TREE_H
