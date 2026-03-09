//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//


#include <string>
#include "Disjunct.h"
#include "../veritas_context.h"
#include <cstdio>

void VERITAS_IN_PLAJA::Disjunct::storeBoundTightening( const Tightening &tightening )
{
    _bounds.push_back( tightening );
}

const std::vector<VERITAS_IN_PLAJA::Tightening> VERITAS_IN_PLAJA::Disjunct::getBoundTightenings() const
{
    return _bounds;
}

void VERITAS_IN_PLAJA::Disjunct::addEquation( const Equation &equation )
{
	_equations.push_back( equation );
}

const std::vector<VERITAS_IN_PLAJA::Equation> VERITAS_IN_PLAJA::Disjunct::getEquations() const
{
	return _equations;
}

void VERITAS_IN_PLAJA::Disjunct::dump() const
{
    std::cout << "\nDumping piecewise linear case split\n";
    std::cout << ("\tBounds are:\n");
    for ( const auto &bound : _bounds )
    {
        std::cout << "Variable: " << bound._variable << ". New bound: " << bound._value << ". BoundType: " << (bound._type == Tightening::LB ? "lower" : "upper") << std::endl;
    }

    std::cout << ( "\n\tEquations are:\n" );
    for ( const auto &equation : _equations )
    {
        // PlaJA: adapted to suppress runtime output
        equation.dump();
    }
}

bool VERITAS_IN_PLAJA::Disjunct::operator==( const VERITAS_IN_PLAJA::Disjunct &other ) const
{
    return ( _bounds == other._bounds ) && ( _equations == other._equations );
}

void VERITAS_IN_PLAJA::Disjunct::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    for ( auto &bound : _bounds )
    {
        if ( bound._variable == oldIndex )
            bound._variable = newIndex;
    }

    for ( auto &equation : _equations )
        equation.updateVariableIndex( oldIndex, newIndex );
}

/** PlaJA *************************************************************************************************************/

void VERITAS_IN_PLAJA::Disjunct::updateVariableIndices( const std::map<unsigned int, unsigned int> &oldIndexToNewIndex )
{

    for ( auto &bound: _bounds )
    {
        if ( oldIndexToNewIndex.find( bound._variable ) != oldIndexToNewIndex.end() )
        {
            bound._variable = oldIndexToNewIndex.at( bound._variable );
        }
    }

    for ( auto &equation: _equations )
    {
        equation.updateVariableIndices( oldIndexToNewIndex );
    }

}

/* Conflict learning prototype. */

VERITAS_IN_PLAJA::Disjunct::Disjunct(){}

/* Move constructor. */

void VERITAS_IN_PLAJA::Disjunct::storeBoundTightening( Tightening &&tightening )
{
    _bounds.push_back( tightening ); // std::move has no effect. Provide move call for for sake of completeness.
}

void VERITAS_IN_PLAJA::Disjunct::addEquation( Equation &&equation )
{
    _equations.push_back( std::move( equation ) );
}

/**********************************************************************************************************************/
veritas::AddTree VERITAS_IN_PLAJA::Disjunct::to_trees(std::shared_ptr<VERITAS_IN_PLAJA::Context> context, veritas::AddTree policy) const {
    veritas::AddTree trees(0, veritas::AddTreeType::REGR);
    std::vector<VERITAS_IN_PLAJA::Tightening> tightenings = _bounds;
    if (policy.num_leaf_values() > 0) {
            PLAJA_LOG("Until now, has never happened. Need to check this thoroughly when encountered.")
    }

    for (auto eq : _equations) {
        auto indicator = context->add_eq_aux_var();
        tightenings.emplace_back(VERITAS_IN_PLAJA::Tightening(indicator, 1, VERITAS_IN_PLAJA::Tightening::BoundType::LB));
        auto ts = eq.to_applicability_trees(*context, policy, indicator);
        trees.add_trees(ts);
    }

    /**
     * Given a list of tightening constraints, t1 = A <= 1, t2 = B >= 1  t1 or t2 or t3 or ...
    */
    veritas::Tree tree(1);
    std::string prefix = "";
    for (const auto bound: tightenings) {
            veritas::FloatT val = bound._type == VERITAS_IN_PLAJA::Tightening::BoundType::UB ? bound._value + 1 : bound._value;
            tree.split(tree[prefix.c_str()], {static_cast<veritas::FeatId>(bound._variable), val});
            std::string leaf_prefix = bound._type == VERITAS_IN_PLAJA::Tightening::BoundType::UB ? (prefix + "l") : (prefix + "r");
            tree.leaf_value(tree[leaf_prefix.c_str()], 0) = 0;
            prefix += bound._type == VERITAS_IN_PLAJA::Tightening::BoundType::UB ? "r" : "l";
    }
    tree.leaf_value(tree[prefix.c_str()], 0) = VERITAS_IN_PLAJA::negative_infinity;
    trees.add_tree(tree);
    return trees;
}
