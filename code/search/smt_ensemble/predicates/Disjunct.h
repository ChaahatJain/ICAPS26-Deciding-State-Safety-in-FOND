//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef __VeritasDisjunct_h__
#define __VeritasDisjunct_h__

#include "Equation.h"
#include "Tightening.h"
#include <string>
#include <vector>
#include <map>

namespace VERITAS_IN_PLAJA {
class Disjunct
{
public:
    /*
      Store information regarding a bound tightening.
    */
    void storeBoundTightening( const VERITAS_IN_PLAJA::Tightening &tightening );
    const std::vector<VERITAS_IN_PLAJA::Tightening> getBoundTightenings() const;

    /*
      Store information regarding a new equation to be added.
    */
    void addEquation( const VERITAS_IN_PLAJA::Equation &equation );
    void addEquation( const VERITAS_IN_PLAJA::Equation&& equation ) {_equations.push_back(std::move(equation)); };

  	const std::vector<VERITAS_IN_PLAJA::Equation> getEquations() const;

    /*
      Dump the case split - for debugging purposes.
    */
    void dump() const;

    /*
      Equality operator.
    */
    bool operator==( const Disjunct &other ) const;

    /*
      Change the index of a variable that appears in this case split
    */
    void updateVariableIndex( unsigned oldIndex, unsigned newIndex );

    /* PlaJA. */
    void updateVariableIndices( const std::map<unsigned int, unsigned int> &oldIndexToNewIndex );

    /* PlaJA: Conflict learning prototype. */
    Disjunct();

    DEFAULT_CONSTRUCTOR(Disjunct)

    /* PlaJA: Move constructor. */
    void storeBoundTightening( VERITAS_IN_PLAJA::Tightening &&tightening );
    void addEquation( VERITAS_IN_PLAJA::Equation &&equation );
    [[nodiscard]] veritas::AddTree to_trees(std::shared_ptr<VERITAS_IN_PLAJA::Context> context, veritas::AddTree policy = veritas::AddTree(0, veritas::AddTreeType::REGR)) const;

private:
    /*
      Bound tightening information.
    */
    std::vector<VERITAS_IN_PLAJA::Tightening> _bounds;

    /*
      The equation that needs to be added.
    */
    std::vector<VERITAS_IN_PLAJA::Equation> _equations;

};
}

#endif // __VeritasDisjunct_h__


