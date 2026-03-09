//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//
/*********************                                                        */
#ifndef __Veritas_Tightening_h__
#define __Veritas_Tightening_h__
#include <iostream>

// #include "ITableau.h" // PlaJA: disabled include for interfacing
namespace VERITAS_IN_PLAJA {

class Tightening
{
public:
	enum BoundType {
		LB = 0,
		UB = 1,
    };

    explicit Tightening( unsigned variable, double value, BoundType type )
        : _variable( variable )
        , _value( value )
        , _type( type )
    {
    }
    DEFAULT_CONSTRUCTOR(Tightening)
    // DELETE_ASSIGNMENT(Tightening);

	/*
	  The variable to tighten.
	*/
	unsigned _variable;

	/*
	  Its new value.
	*/
	double _value;

	/*
	  Whether the tightening tightens the
	  lower bound or the upper bound.
	*/
    BoundType _type;

    /*
      Equality operator.
    */
    bool operator==( const Tightening &other ) const
    {
        return
            ( _variable == other._variable ) &&
            ( _value == other._value ) &&
            ( _type == other._type );
    }

    void dump() const
    {
        std::cout << "Tightening: x" << _variable << " " << (_type == LB ? ">=" : "<=") << " " << _value << std::endl;
    }
};
}
#endif // __Veritas_Tightening_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
