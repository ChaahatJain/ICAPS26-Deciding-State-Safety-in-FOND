## References & Credits

This package adapts algorithmic methods from [PRISM](https://github.com/prismmodelchecker/prism)
at commit [8394df8e1a0058cec02f47b0437b185e3ae667d7](https://github.com/prismmodelchecker/prism/tree/8394df8e1a0058cec02f47b0437b185e3ae667d7) (March 20, 2019),
specifically in [successor_generator.cpp](successor_generator.cpp) & [action_op_init.cpp](action_op_init.cpp),
respectively [action_op_construction.h](base/action_op_construction.h) & [action_op.cpp](action_op.cpp).

From a more general perspective, the [successor generation](successor_generator.h) 
that constructs per-source-state [action operators](action_op_init.h) on the fly
has been designed under the impression of PRISM's [explicit-state successor generation](https://github.com/prismmodelchecker/prism/tree/8394df8e1a0058cec02f47b0437b185e3ae667d7/prism/src/simulator).
The [successor generation](successor_generator_c.h) caching [action operators](action_op.h) at initialization time inherits these ideas,
(especially towards operator construction). 
