## References & Credits

This package contains code to implement probabilistic search.\
It has been built out of a subset of the [Probabilistic Fast Downward implementation](http://fai.cs.uni-saarland.de/downloads/fd-prob.zip) (February 2020), 
specifically fret.{h,cpp}, lrtdp.{h,cpp}, optimization_criteria.{h,cpp}, value_initializer.{h,cpp}, prob_search_engine.{h,cpp}, prob_search_space.{h,cpp}, prob_search_node_info.{h,cpp} and partially state_space.{h,cpp}.

The reimplemented classes were freely & significantly adapted. A few remarks concerning adaptions with respect to functionality:
- There are many simplifications, e.g., no budget planning (affects in particular prob_search_space.{h,cpp}, prob_search_node_info.{h,cpp}) as well as no tie-breaking for greedy actions, ...
- Some functionality has been moved, e.g., state expansion, and state updates to the prob_search_engine.{h,cpp} parent class (rather than lrtdp.{h,cpp}), absorbing state handling in prob_search_engine.{h,cpp} rather than optimization_criteria.{h,cpp}; the state_space.{h,cpp} has its own RNG to pick random states, ...
- Some functionality has been added, e.g., early-termination checks in lrtdp.{h,cpp}, or handling of cost (besides probability) optimization (affects i.p. optimization_criteria.{h,cpp}, value_initializer.{h,cpp}) & resulting border case handling throughout the complete package (i.p. "fixed states", "don't care states") ... 

Further adaptions were made to:
- use the functionality of the existing code base rather than FD (e.g. calls to the successor-generation-mechanism, option parser, ...),
- fit the code base's (implicit) conventions, in particular also the conventions of the JANI setting,
- make code more intuitive/readable while preserving functionality (e.g., in fret.{h,cpp}). 

Finally, a rather detailed documentation has been added.
