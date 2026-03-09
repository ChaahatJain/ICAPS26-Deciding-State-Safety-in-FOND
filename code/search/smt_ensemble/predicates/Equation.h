//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef __Veritas_Equation_h__
#define __Veritas_Equation_h__

#include <vector>
#include <map>
#include <set>
#include <string>
#include "../../../utils/default_constructors.h"
#include "addtree.hpp"
#include "../using_veritas.h"
#include "../forward_smt_veritas.h"

/*
   A class representing a single equation.
   An equation is interpreted as:

   sum( coefficient * variable ) op scalar

   Where op is either =, <= or >=
*/

namespace VERITAS_IN_PLAJA { class Context; }
namespace VERITAS_IN_PLAJA {
class Equation {
public:
    enum EquationType {
        EQ = 0,
        GE = 1,
        LE = 2
    };

    struct Addend
    {
    public:
        Addend( double coefficient, unsigned variable );

        double _coefficient;
        unsigned _variable;

        bool operator==( const Addend &other ) const;
    };

    explicit Equation();
    Equation( EquationType type );
    DEFAULT_CONSTRUCTOR(Equation)
    ~Equation() = default;
    // DELETE_ASSIGNMENT(Equation);

    void addAddend( double coefficient, unsigned variable );
    void setScalar( double scalar );
    void setType( EquationType type );

    /*
      Go over the addends and rename variable oldVar to newVar.
      If, as a result, there are two addends with the same variable,
      unite them.
    */
    void updateVariableIndex( unsigned oldVar, unsigned newVar );

    /*
      Return true iff the variable is a "variable merging equation",
      i.e. an equation of the form x = y. If true is returned, x1 and
      x2 are the merged variables.
    */
    bool isVariableMergingEquation( unsigned &x1, unsigned &x2 ) const;

    /*
      Get the set of indices of all variables that participate in this
      equation.
    */
    std::set<unsigned> getParticipatingVariables() const;
    std::vector<unsigned> getListParticipatingVariables() const;

    /*
      Retrieve the coefficient of a variable
    */
    double getCoefficient( unsigned variable ) const;
    /*
     Set the coefficient of a variable
    */
    void setCoefficient( unsigned variable, double newCoefficient );

    std::vector<Addend> _addends;
    double _scalar;
    EquationType _type;

    bool operator==( const Equation &other ) const;
    bool equivalent( const Equation &other ) const;
    void dump() const;

    /* PlaJA **********************************************************************************************************/

    void updateVariableIndices( const std::map<unsigned int, unsigned int> &oldIndexToNewIndex );

    /* An equation in standardized iff addends are sorted by increasing variable index and first variable has positive coefficient. */
    // void standardize();

    [[nodiscard]] bool isStandardized() const;

    /* Hash support. */

    [[nodiscard]] bool equals( const Equation* other ) const;

    [[nodiscard]] bool equalsAddends( const Equation* other ) const;

    [[nodiscard]] int num_addends() const {return _addends.size(); }

    [[nodiscard]] veritas::AddTree to_trees(VERITAS_IN_PLAJA::Context& context, bool updates = false, veritas::AddTree policy = veritas::AddTree(0, veritas::AddTreeType::REGR)) const;

    [[nodiscard]] std::tuple<veritas::FeatId, double, double> get_tighter_bounds(VERITAS_IN_PLAJA::Context& context) const;

    [[nodiscard]] veritas::AddTree to_applicability_trees(const VERITAS_IN_PLAJA::Context& context, veritas::AddTree policy, veritas::FeatId indicator_var) const;

    [[nodiscard]] veritas::AddTree bound_src_vars(const VERITAS_IN_PLAJA::Context& context) const;

    std::tuple<veritas::FeatId, std::vector<std::pair<veritas::FloatT, veritas::FeatId>>, veritas::FloatT> get_update() const {
      /**
       * Store the update computation. We need the target variable, a list of <coefficient, src variables> as well as the scalar. 
       * Everything is already scaled wrt to the coefficient of the target variable.
       * **/
      veritas::FeatId target = _addends[0]._variable;
      std::vector<std::pair<veritas::FloatT, veritas::FeatId>> source;
      for (int i = 1; i < _addends.size(); ++i) {
        veritas::FloatT coeff = -1*_addends[i]._coefficient/_addends[0]._coefficient;
        veritas::FeatId src = _addends[i]._variable;
        source.push_back({coeff, src});
      }
      veritas::FloatT scalar = _scalar/_addends[0]._coefficient;
      std::tuple<veritas::FeatId, std::vector<std::pair<veritas::FloatT, veritas::FeatId>>, veritas::FloatT> update(target, source, scalar);
      return update;
    }

    /**
     * Given a policy, return the relevant splits for this policy.
     * The splits are of the form: feature < value, where value is a scalar
     * in the policy.
     */
    std::map<veritas::FeatId, std::vector<veritas::FloatT>> get_relevant_splits(veritas::AddTree policy) const;
    struct Bound {
      veritas::FeatId _variable;
      veritas::FloatT lb;
      veritas::FloatT ub;
    };
    struct Assignment {
      std::vector<Bound> bound_per_feature;
      bool satisfying;
    };
    bool is_satisfying(std::vector<Bound> bounds_per_feature) const;
    std::vector<Assignment> get_all_assignments(veritas::AddTree policy) const;
    veritas::Tree real_assignment_to_tree(Assignment assignment, veritas::FeatId indicator_var) const;
    /******************************************************************************************************************/

  };
}

#endif // __Veritas_Equation_h__


