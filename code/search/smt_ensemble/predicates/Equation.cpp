//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "Equation.h"
#include <string>
#include "../../../assertions.h"
#include "../veritas_context.h"
#include <cmath>
#include <z3++.h>


VERITAS_IN_PLAJA::Equation::Addend::Addend( double coefficient, unsigned variable )
    : _coefficient( coefficient )
    , _variable( variable )
{
}

bool VERITAS_IN_PLAJA::Equation::Addend::operator==( const Addend &other ) const
{
    return ( _coefficient == other._coefficient ) && ( _variable == other._variable );
}

VERITAS_IN_PLAJA::Equation::Equation()
    : _scalar( 0 )
    , _type( Equation::EQ )
{
}

VERITAS_IN_PLAJA::Equation::Equation( EquationType type )
    : _scalar( 0 )
    , _type( type )
{
}

void VERITAS_IN_PLAJA::Equation::addAddend( double coefficient, unsigned variable )
{
    _addends.push_back( Addend( coefficient, variable ) );
}

void VERITAS_IN_PLAJA::Equation::setScalar( double scalar )
{
    _scalar = scalar;
}

void VERITAS_IN_PLAJA::Equation::setType( EquationType type )
{
    _type = type;
}

void VERITAS_IN_PLAJA::Equation::updateVariableIndex( unsigned oldVar, unsigned newVar )
{
    // Find oldVar's addend and update it
    std::vector<Addend>::iterator oldVarIt = _addends.begin();
    while ( oldVarIt != _addends.end() && oldVarIt->_variable != oldVar )
        ++oldVarIt;

    // OldVar doesn't exist - can stop
    if ( oldVarIt == _addends.end() )
        return;

    // Update oldVar's index
    oldVarIt->_variable = newVar;

    // Check to see if there are now two addends for newVar. If so,
    // remove one and adjust the coefficient
    std::vector<Addend>::iterator newVarIt;
    for ( newVarIt = _addends.begin(); newVarIt != _addends.end(); ++newVarIt )
    {
        if ( newVarIt == oldVarIt )
            continue;

        if ( newVarIt->_variable == newVar )
        {
            oldVarIt->_coefficient += newVarIt->_coefficient;
            _addends.erase( newVarIt );
            return;
        }
    }
}

bool VERITAS_IN_PLAJA::Equation::operator==( const Equation &other ) const
{
    return
        ( _addends == other._addends ) &&
        ( _scalar == other._scalar ) &&
        ( _type == other._type );
}

bool VERITAS_IN_PLAJA::Equation::equivalent( const Equation &other ) const
{
    if ( _scalar != other._scalar )
        return false;

    if ( _type != other._type )
        return false;

    std::map<unsigned, double> us;
    std::map<unsigned, double> them;

    for ( const auto &addend : _addends )
        us[addend._variable] = addend._coefficient;

    for ( const auto &addend : other._addends )
        them[addend._variable] = addend._coefficient;

    return us == them;
}


void VERITAS_IN_PLAJA::Equation::dump() const
{
    std::cout << "Equation: ";
    for ( const auto &addend : _addends )
    {
        if ( addend._coefficient == 0 )
            continue;

        if ( addend._coefficient > 0 )
            std::cout << ( "+" );

        std::cout << addend._coefficient << "x" << addend._variable;
    }

    switch ( _type )
    {
    case Equation::GE:
        std::cout << ( " >= " );
        break;

    case Equation::LE:
        std::cout << ( " <= " );
        break;

    case Equation::EQ:
        std::cout << ( " = " );
        break;
    }

    std::cout << _scalar << std::endl;
}

bool VERITAS_IN_PLAJA::Equation::isVariableMergingEquation( unsigned &x1, unsigned &x2 ) const
{
    if ( _addends.size() != 2 )
        return false;

    if ( _type != Equation::EQ )
        return false;

    if ( !( _scalar == 0) )
        return false;

    double coefficientOne = _addends[0]._coefficient;
    double coefficientTwo = _addends[_addends.size() - 1]._coefficient;

    if ( coefficientOne == 0 || ( coefficientTwo == 0 ) )
        return false;

    if ( coefficientOne == -coefficientTwo)
    {
        x1 = _addends[0]._variable;
        x2 = _addends[_addends.size() - 1]._variable;
        return true;
    }

    return false;
}

std::set<unsigned> VERITAS_IN_PLAJA::Equation::getParticipatingVariables() const
{
    std::set<unsigned> result;
    for ( const auto &addend : _addends )
        result.insert( addend._variable );

    return result;
}

std::vector<unsigned> VERITAS_IN_PLAJA::Equation::getListParticipatingVariables() const
{
    std::vector<unsigned> result;
    result.reserve(_addends.size());
    for ( const auto &addend : _addends )
        result.push_back( addend._variable );

    return result;
}

double VERITAS_IN_PLAJA::Equation::getCoefficient( unsigned variable ) const
{
    for ( const auto &addend : _addends )
    {
        if ( addend._variable == variable )
            return addend._coefficient;
    }

    return 0;
}

void VERITAS_IN_PLAJA::Equation::setCoefficient( unsigned variable, double newCoefficient)
{
    for ( auto &addend : _addends )
    {
        if ( addend._variable == variable )
        {
            addend._coefficient = newCoefficient;
            return;
        }
    }

    addAddend(newCoefficient, variable);
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//

/* PlaJA **************************************************************************************************************/
void VERITAS_IN_PLAJA::Equation::updateVariableIndices( const std::map<unsigned int, unsigned int> &oldIndexToNewIndex )
{

    std::map< unsigned, double > newAddends; // Just sort variables by index. Overhead should be negligible.

    for ( const auto &addend : _addends )
    {
        auto oldIt = oldIndexToNewIndex.find(addend._variable);
        auto varIndex = oldIt != oldIndexToNewIndex.end() ? oldIndexToNewIndex.at( addend._variable ) : addend._variable;

        auto it = newAddends.find( varIndex );

        if ( it == newAddends.end() ) {
            newAddends.emplace( varIndex, addend._coefficient );
        } else {
            it->second += addend._coefficient;
        }

    }

    _addends.clear();

    for ( const auto &[ variable, coefficient ]: newAddends )
    {
        PLAJA_ASSERT( coefficient != 0 )
        _addends.push_back( { coefficient, variable } );
    }

}

bool VERITAS_IN_PLAJA::Equation::isStandardized() const
{

    PLAJA_ASSERT( not _addends.empty() )

    if ( _addends.begin()->_coefficient <= 0 )
    {
        return false;
    }

    auto lastVar = _addends.begin()->_variable;

    for ( auto it = ++_addends.begin(); it != _addends.end(); ++it )
    {
        if ( it->_variable <= lastVar )
        {
            return false;
        }

        lastVar = it->_variable;
    }

    return true;

}

bool VERITAS_IN_PLAJA::Equation::equals( const Equation *other ) const
{
    PLAJA_ASSERT( other )
    PLAJA_ASSERT( isStandardized() and other->isStandardized())
    return other->_type == _type and other->_scalar == _scalar and equalsAddends( other );
}

bool VERITAS_IN_PLAJA::Equation::equalsAddends( const Equation *other ) const
{
    {

        const auto &addendsOther = other->_addends;

        if ( _addends.size() != addendsOther.size())
        {
            return false;
        }

        std::unordered_map<unsigned, double> addendsMap;

        for ( const auto &addend: _addends )
        {
            PLAJA_ASSERT( not addendsMap.count( addend._variable ))
            addendsMap[addend._variable] = addend._coefficient;
        }

        for ( const auto &addend: addendsOther )
        {
            auto it = addendsMap.find( addend._variable );

            if ( it == addendsMap.end() or it->second != addend._coefficient )
            {
                return false;
            }
        }

        return true;

    }

}

veritas::AddTree single_addend_tree(VERITAS_IN_PLAJA::Equation::Addend addend, veritas::FloatT _scalar) {
        veritas::AddTree trees(1, veritas::AddTreeType::REGR);
        auto var = static_cast<veritas::FeatId>(addend._variable);
        auto coeff = addend._coefficient;
        auto scalar = _scalar/coeff;
        veritas::Tree tree(1);
        tree.split(tree.root(), {var, scalar + 1 + VERITAS_IN_PLAJA::offset});
        tree.split(tree["l"], {var, scalar + VERITAS_IN_PLAJA::offset});
        tree.leaf_value(tree["r"], 0) = VERITAS_IN_PLAJA::negative_infinity;
        tree.leaf_value(tree["ll"], 0) = VERITAS_IN_PLAJA::negative_infinity;
        tree.leaf_value(tree["lr"], 0) = 0;
        trees.add_tree(tree);
        return trees;
}

void addIndicatorToTree(veritas::Tree& tree, std::string& prefix, veritas::FeatId indicator_var, bool satisfying = false) {
    /* Only return 0 if the indicator is True and guard is satisfying OR indicator is False and guard is not satisfying.*/
    tree.split(tree[(prefix).c_str()], {indicator_var, 1});
    tree.leaf_value(tree[(prefix + "l").c_str()], 0) = satisfying ? VERITAS_IN_PLAJA::negative_infinity : 0;
    tree.leaf_value(tree[(prefix + "r").c_str()], 0) = satisfying ? 0 : VERITAS_IN_PLAJA::negative_infinity;
}

void addToTree(veritas::Tree& tree, std::string& prefix, veritas::FeatId feat, veritas::FloatT assignment, bool last_var) {
    if (prefix.empty()) {
        tree.split(tree.root(), {feat, assignment + 1 + VERITAS_IN_PLAJA::offset});
        tree.split(tree["l"], {feat, assignment + VERITAS_IN_PLAJA::offset});
        if (last_var) {
            tree.leaf_value(tree["r"], 0) = VERITAS_IN_PLAJA::negative_infinity;
            tree.leaf_value(tree["lr"], 0) = 0; 
            tree.leaf_value(tree["ll"], 0) = VERITAS_IN_PLAJA::negative_infinity;
            prefix = "lr";
            return ;
        }
        tree.leaf_value(tree["r"], 0) = 0;
        tree.leaf_value(tree["ll"], 0) = 0;
        prefix = "lr";
        return;
    }
    tree.split(tree[prefix.c_str()], {feat, assignment + 1 + VERITAS_IN_PLAJA::offset});
    tree.split(tree[(prefix + "l").c_str()], {feat, assignment + VERITAS_IN_PLAJA::offset});

    if (last_var) {
        tree.leaf_value(tree[(prefix + "r").c_str()], 0) = VERITAS_IN_PLAJA::negative_infinity;
        tree.leaf_value(tree[(prefix + "ll").c_str()], 0) = VERITAS_IN_PLAJA::negative_infinity;
        tree.leaf_value(tree[(prefix + "lr").c_str()], 0) = 0;
        prefix = prefix + "lr";
        return ;
    }
    tree.leaf_value(tree[(prefix + "r").c_str()], 0) = 0;
    tree.leaf_value(tree[(prefix + "ll").c_str()], 0) = 0;
    prefix = prefix + "lr";
    return;
}


void falsifyingTree(veritas::Tree& tree, std::string& prefix, veritas::FeatId feat, veritas::FloatT assignment, bool last_var) {
    if (prefix.empty()) {
        tree.split(tree.root(), {feat, assignment + 1 + VERITAS_IN_PLAJA::offset});
        tree.split(tree["l"], {feat, assignment + VERITAS_IN_PLAJA::offset});
        tree.leaf_value(tree["r"], 0) = 0;
        tree.leaf_value(tree["ll"], 0) = 0;
        prefix = "lr";
        return;
    }
    tree.split(tree[prefix.c_str()], {feat, assignment + 1 + VERITAS_IN_PLAJA::offset});
    tree.split(tree[(prefix + "l").c_str()], {feat, assignment + VERITAS_IN_PLAJA::offset});

    if (last_var) {
        tree.leaf_value(tree[(prefix + "r").c_str()], 0) = 0;
        tree.leaf_value(tree[(prefix + "ll").c_str()], 0) = 0;
        tree.leaf_value(tree[(prefix + "lr").c_str()], 0) = VERITAS_IN_PLAJA::negative_infinity;
        prefix = prefix + "lr";
        return ;
    }
    tree.leaf_value(tree[(prefix + "r").c_str()], 0) = 0;
    tree.leaf_value(tree[(prefix + "ll").c_str()], 0) = 0;
    prefix = prefix + "lr";
    return;
}

veritas::Tree createTree(const std::vector<VERITAS_IN_PLAJA::Equation::Addend>& addends, std::unordered_map<veritas::FeatId, veritas::FloatT> assignment) {
    veritas::Tree tree(1);
    std::string prefix = "";
    for (int i = 0; i < addends.size(); ++i) {
        auto var = addends[i]._variable;
        auto val = assignment[var];
        falsifyingTree(tree, prefix, var, val, (i == addends.size() - 1));
    }
    return tree;
}


veritas::Tree createTreeWithIndicator(const std::vector<VERITAS_IN_PLAJA::Equation::Addend>& addends, std::unordered_map<veritas::FeatId, veritas::FloatT> assignment, bool satisfying, veritas::FeatId indicator_var = 0) {
    veritas::Tree tree(1);
    std::string prefix = "";
    for (int i = 0; i < addends.size(); ++i) {
        auto var = addends[i]._variable;
        auto val = assignment[var];
        addToTree(tree, prefix, var, val, false);
    }
    addIndicatorToTree(tree, prefix, indicator_var, satisfying);
    return tree;
}

void generateCombinations(const std::vector<VERITAS_IN_PLAJA::Equation::Addend>& addends, const VERITAS_IN_PLAJA::Context& context,
                           std::unordered_map<veritas::FeatId, veritas::FloatT>& currentAssignment, int currentVariableIndex, veritas::FloatT currentScalar,
                           veritas::AddTree& allTrees, VERITAS_IN_PLAJA::Equation::EquationType type, bool filtering = false, veritas::FeatId indicator_var = 0) {

    if (currentVariableIndex == addends.size()) {
        // Base case: All variables assigned, create a tree and add it to the vector
        veritas::FloatT temp_scalar = 0;
        // std::cout << "Assignment: {";
        for (auto addend : addends) {
            auto var = addend._variable;
            auto val = currentAssignment[var];
            temp_scalar += addend._coefficient*val;
            // std::cout << "(" << var << ", " << val << ") ";
        }
        // std::cout << "} with current Scalar being: " << currentScalar << std::endl;

        if (filtering) {
            bool correct = currentScalar == temp_scalar;
            veritas::Tree tree = createTreeWithIndicator(addends, currentAssignment, correct, indicator_var);
            allTrees.add_tree(tree);
            return ;
        }

        if (currentScalar != temp_scalar && type == VERITAS_IN_PLAJA::Equation::EquationType::EQ) { // TODO: This can also be currentScalar <= 0 or currentScalar >= 0 depending on equation
            veritas::Tree tree = createTree(addends, currentAssignment);
            allTrees.add_tree(tree);
           return ;
        }
        if (temp_scalar < currentScalar && type == VERITAS_IN_PLAJA::Equation::EquationType::GE) {
            veritas::Tree tree = createTree(addends, currentAssignment);
            allTrees.add_tree(tree);
           return ;
        }
        if (temp_scalar > currentScalar && type == VERITAS_IN_PLAJA::Equation::EquationType::LE) {
            veritas::Tree tree = createTree(addends, currentAssignment);
            allTrees.add_tree(tree);
           return ;
        }
        return ;
    }

    // Recursive case: Iterate through the assignments for the current variable
    const auto addend = addends[currentVariableIndex];
    const veritas::FeatId& currentVariable = addend._variable;
    auto lb = context.get_lower_bound(currentVariable);
    auto ub = context.get_upper_bound(currentVariable);
    // std::cout << "Lower and upper bounds for " << currentVariable << " are " << lb << ", " << ub << std::endl;
    for (auto assignment = lb; assignment <= ub; ++ assignment) {
        currentAssignment[currentVariable] = assignment;
        generateCombinations(addends, context, currentAssignment, currentVariableIndex + 1, currentScalar, allTrees, type, filtering, indicator_var);
    }
    return ;
}

std::tuple<veritas::FeatId, double, double> VERITAS_IN_PLAJA::Equation::get_tighter_bounds(VERITAS_IN_PLAJA::Context& context) const {
    // TODO: This only works when addends coefficient is 1 
    auto target_var = _addends[0]._variable;
    if (_addends.size() == 2) {
        auto src_var = _addends[1]._variable;
        auto mod_lb = context.get_lower_bound(src_var) - _scalar;
        auto mod_ub = context.get_upper_bound(src_var) - _scalar;
        auto lb = std::max(mod_lb, context.get_lower_bound(target_var));
        auto ub = std::min(mod_ub, context.get_upper_bound(target_var));
        return {target_var, lb, ub};
    }
    return {target_var, context.get_lower_bound(target_var), context.get_upper_bound(target_var)};
}

veritas::AddTree VERITAS_IN_PLAJA::Equation::bound_src_vars(const VERITAS_IN_PLAJA::Context& context) const {
    /*
        Equations in PlaJA are parsed into c0*target + scalar = c1*src
        Calculate new valid bounds for the source variables and add trees to the ensemble which refine the bounds over source variables. 
        These trees implement lb <= src < ub (open intervals) 
    */
    auto target_var = static_cast<veritas::FeatId>(_addends[0]._variable);
    auto src_var = static_cast<veritas::FeatId>(_addends[1]._variable);
    // calculate lower and upper bounds for source variables based on bounds of target.
    veritas::FloatT mod_lb, mod_ub; 
    if (_addends[1]._coefficient > 0) {
        // |a1| = a1 since positive. |a0| = -a0
        // -|a0|*x' + a1*x = c => x' = (a1*x - c)/|a0|
        // If  lb <= x' <= ub, then lb <= (a1*x - c)/|a0| <= ub
        // => (|a0|*lb + c)/a1 <= x <= (|a0|*ub + c)/a1
        // dump();
        // std::cout << "Positive coefficient: " << _addends[1]._coefficient << std::endl; 
        // std::cout << "Information: addend 0 coefficient " << _addends[0]._coefficient << ", context lb: " << context.get_lower_bound(target_var) << ", context ub:" << context.get_upper_bound(target_var) << ", scalar: " << _scalar << " addend 1 coefficient: " << _addends[1]._coefficient << std::endl;
        mod_lb = (-_addends[0]._coefficient*context.get_lower_bound(target_var) + _scalar)/_addends[1]._coefficient;
        mod_ub = (-_addends[0]._coefficient*context.get_upper_bound(target_var) + _scalar)/_addends[1]._coefficient;
    } else {
        // std::cout << "Negative coefficient: " << _addends[1]._coefficient << std::endl; 
        // std::cout << "Information: addend 0 coefficient " << _addends[0]._coefficient << ", context lb: " << context.get_lower_bound(target_var) << ", scalar: " << _scalar << " addend 1 coefficient: " << _addends[1]._coefficient << std::endl;
        // Note |a1| = -1*a1 since negative. |a0| = -a0
        // -|a0|*x' - |a1|*x = c => x' = -(|a1|*x - c)/|a0|
        // If  lb <= x' <= ub, then lb <= -(|a1|*x - c)/|a0| <= ub
        // => -lb >= (|a1|*x - c)/|a0| >= -ub
        // => (a0*lb + c)/|a1| >= x >= (a0*ub + c)/|a1| since |a0|*(-k) = a0*k
        mod_ub = -(_addends[0]._coefficient*context.get_lower_bound(target_var) + _scalar)/_addends[1]._coefficient;
        mod_lb = -(_addends[0]._coefficient*context.get_upper_bound(target_var) + _scalar)/_addends[1]._coefficient;
    }
    // Chose the tightest bound for allowed source variable values
    auto lb = std::max(mod_lb, context.get_lower_bound(src_var));
    auto ub = std::min(mod_ub, context.get_upper_bound(src_var));

    ub = context.is_marked_integer(src_var) ? ub + 1 : ub + PLAJA::floatingPrecision;
    veritas::AddTree trees(1, veritas::AddTreeType::REGR);
    // Add a decision tree to the ensemble which gives penalty if src < lb 
    veritas::Tree lb_tree(1);
    lb_tree.split(lb_tree.root(), {src_var, lb});
    lb_tree.leaf_value(lb_tree["l"], 0) = VERITAS_IN_PLAJA::negative_infinity;
    lb_tree.leaf_value(lb_tree["r"], 0) = 0;
    // Add a decision tree to the ensemble which gives penalty if src >= ub 
    veritas::Tree ub_tree(1);
    ub_tree.split(ub_tree.root(), {src_var, ub});
    ub_tree.leaf_value(ub_tree["l"], 0) = 0;
    ub_tree.leaf_value(ub_tree["r"], 0) = VERITAS_IN_PLAJA::negative_infinity;
    trees.add_tree(lb_tree);
    trees.add_tree(ub_tree);
    // std::cout << "Variable: " << src_var << " with lower bound: " << lb << " and upper bound: " << ub << std::endl;
    // for (auto t : trees) {
    //     std::cout << t << std::endl;
    // }
    return trees;
}

veritas::AddTree VERITAS_IN_PLAJA::Equation::to_trees(VERITAS_IN_PLAJA::Context& context, bool updates, veritas::AddTree policy) const {
    // dump();
    veritas::AddTree trees(1, veritas::AddTreeType::REGR);
    if (_addends.size() == 1) {
        auto addend = _addends[0];
        return single_addend_tree(addend, _scalar);
    }

    if (_addends.size() == 2 && updates) {
        // std::cout << "Two addends so just bound variables" << std::endl;
        return bound_src_vars(context); // This means equation is of the form x' = x + c. So, the bounds for source variables need to be adjusted only.
    }

    bool all_integers = true;
    for (const auto& addend : _addends) {
        if (!context.is_marked_integer(addend._variable)) {
            all_integers = false;
            break;
        }
    }

    if (all_integers) {
        std::unordered_map<veritas::FeatId, veritas::FloatT> currentAssignment;
        generateCombinations(_addends, context, currentAssignment, 0, _scalar, trees, _type);
        // std::cout << "Trees for equation " << std::endl;
        // for (auto t: trees) {
        //     std::cout << t << std::endl;
        // }
        return trees;
    } else {
        // PLAJA_LOG("Not all addends are integers")
        auto assignments = get_all_assignments(policy);
        for (auto assignment: assignments) {
            trees.add_tree(real_assignment_to_tree(assignment, -1));
        }
        return trees;
    } 
}

std::map<veritas::FeatId, std::vector<veritas::FloatT>> VERITAS_IN_PLAJA::Equation::get_relevant_splits(veritas::AddTree policy) const {
    auto all_splits = policy.get_splits();
    std::map<veritas::FeatId, std::vector<veritas::FloatT>> relevant_splits;
    for (const auto& addend : _addends) {
        veritas::FeatId feat = addend._variable;
        auto it = all_splits.find(feat);
        if (it != all_splits.end()) {
            relevant_splits[feat] = it->second;
        }
    }
    /* Sanity Check */
    // for (const auto& [feat, splits] : relevant_splits) {
    //     // print all splits for a feature
    //     std::cout << "Feature: " << feat << " has splits: ";
    //     for (const auto& split : splits) {
    //         std::cout << split << " ";
    //     }
    //     std::cout << std::endl;
    // }
    
    return relevant_splits;
}

bool VERITAS_IN_PLAJA::Equation::is_satisfying(std::vector<Bound> assignment) const {
    z3::context ctx;
    z3::solver solver(ctx);
    std::unordered_map<unsigned, z3::expr> z3_vars;
    // Step 1: Declare Z3 variables and apply bounds
    for (const Bound& b : assignment) {
        std::string var_name = "x" + std::to_string(b._variable);
        z3::expr x = ctx.real_const(var_name.c_str());
        // Insert into the map (avoids default construction)
        z3_vars.emplace(b._variable, x);
        double effective_lb = (b.lb == -INFINITY) ? -1e308 : b.lb;
        double effective_ub = (b.ub == INFINITY) ? 1e308 : b.ub;
        solver.add(x >= ctx.real_val(std::to_string(effective_lb).c_str()));
        solver.add(x < ctx.real_val(std::to_string(effective_ub).c_str())); // The upper bound is always strict in Veritas
    }


    // Step 2: Build linear expression from all addends
    z3::expr sum_expr = ctx.real_val(0);
    for (const Addend& a : _addends) {
        auto it = z3_vars.find(a._variable);
        if (it == z3_vars.end()) {
            throw std::runtime_error("Variable " + std::to_string(a._variable) + " missing from assignment.");
        }
        z3::expr x_i = it->second;
        sum_expr = sum_expr + x_i * ctx.real_val(std::to_string(a._coefficient).c_str());
    }

    // Step 3: Add equation type constraint
    switch (_type) {
        case EQ: solver.add(sum_expr == ctx.real_val(std::to_string(_scalar).c_str())); break;
        case GE: solver.add(sum_expr >= ctx.real_val(std::to_string(_scalar).c_str())); break;
        case LE: solver.add(sum_expr <= ctx.real_val(std::to_string(_scalar).c_str())); break;
        default: throw std::runtime_error("Unsupported EquationType.");
    }

    // Step 4: Check satisfiability
    return solver.check() == z3::sat;
}


std::vector<VERITAS_IN_PLAJA::Equation::Assignment> VERITAS_IN_PLAJA::Equation::get_all_assignments(veritas::AddTree policy) const {
    /**
    * Obtain the cross product of all possible intervals for variables in the ensembles (calculated using the splits in the ensemble). 
    * Then check if a satisfying solution exists for the equation within the bounds for each variable in this assignment. 
    */

    auto split_map = get_relevant_splits(policy);
    std::vector<Assignment> all_assignments;

    // Step 1: Collect all per-feature bound options
    std::vector<std::vector<Bound>> bounds_per_feature;
    std::vector<veritas::FeatId> feature_order;

    for (const auto& [feat, splits] : split_map) {
        std::vector<Bound> bounds;

        size_t num_bins = splits.size() + 1;
        for (size_t i = 0; i < num_bins; ++i) {
            veritas::FloatT lb = (i == 0) ? -std::numeric_limits<veritas::FloatT>::infinity() : splits[i - 1];
            veritas::FloatT ub = (i < splits.size()) ? splits[i] : std::numeric_limits<veritas::FloatT>::infinity();
            bounds.push_back(Bound{feat, lb, ub});
        }

        bounds_per_feature.push_back(std::move(bounds));
        feature_order.push_back(feat);
    }

    // Step 2: Recursive product generation
    std::function<void(size_t, std::vector<Bound>&)> recurse;
    recurse = [&](size_t idx, std::vector<Bound>& current) {
        if (idx == bounds_per_feature.size()) {
            bool satisfied = is_satisfying(current);
            all_assignments.push_back(Assignment{current, satisfied});
            return;
        }

        for (const Bound& b : bounds_per_feature[idx]) {
            current.push_back(b);
            recurse(idx + 1, current);
            current.pop_back();
        }
    };

    std::vector<Bound> current;
    recurse(0, current);

    return all_assignments;
}

veritas::Tree VERITAS_IN_PLAJA::Equation::real_assignment_to_tree(VERITAS_IN_PLAJA::Equation::Assignment assignment, veritas::FeatId indicator_var) const {
    veritas::Tree tree(1);
    std::string prefix = "";
    for (int i = 0; i < assignment.bound_per_feature.size(); ++i) {
        auto var = assignment.bound_per_feature[i]._variable;
        auto lb = assignment.bound_per_feature[i].lb; // Use lower bound for the tree
        auto ub = assignment.bound_per_feature[i].ub; // Use upper bound for the tree
        tree.split(tree[prefix.c_str()], {var, ub});
        tree.split(tree[(prefix + "l").c_str()], {var, lb});
        tree.leaf_value(tree[(prefix + "r").c_str()], 0) = 0; // x geq ub is out of bounds
        tree.leaf_value(tree[(prefix + "ll").c_str()], 0) = 0; // x lt lb is out of bounds
        prefix = prefix + "lr"; // Update prefix for next variable
    }
    if (indicator_var == -1) {
        tree.leaf_value(tree[(prefix).c_str()], 0) = assignment.satisfying? 0 : VERITAS_IN_PLAJA::negative_infinity; 
        // Terminate the tree with the final leaf node (0 if assignment is satisfying and penalty otherwise)
    }
    else {
        addIndicatorToTree(tree, prefix, indicator_var, assignment.satisfying); // add indicator variable to the tree
    }

    /* Sanity check on the trees */
    // std::cout << "Assignment: " << assignment.satisfying << " with bounds: ";
    //     for (const auto& bound : assignment.bound_per_feature) {
    //         std::cout << "(" << bound._variable << ", [" << bound.lb << ", " << bound.ub << "]) ";
    //     }
    //     std::cout << std::endl;
    // std::cout << tree << std::endl;
    return tree;
}


veritas::AddTree VERITAS_IN_PLAJA::Equation::to_applicability_trees(const VERITAS_IN_PLAJA::Context& context, veritas::AddTree policy, veritas::FeatId indicator_var) const {
    // std::cout << "Reached Applicability" << std::endl;
    // Check whether all addends are integers:
    bool all_integers = true;
    for (const auto& addend : _addends) {
        if (!context.is_marked_integer(addend._variable)) {
            all_integers = false;
            break;
        }
    }
    veritas::AddTree trees(1, veritas::AddTreeType::REGR);
    if (all_integers) {
        std::unordered_map<veritas::FeatId, veritas::FloatT> currentAssignment;
        generateCombinations(_addends, context, currentAssignment, 0, _scalar, trees, _type, true, indicator_var);
        return trees;
    } else {
        // PLAJA_LOG("Not all addends are integers")
        auto assignments = get_all_assignments(policy);
        for (auto assignment: assignments) {
            trees.add_tree(real_assignment_to_tree(assignment, indicator_var));
        }
        return trees;
    } 
}

