# 4. Block {#ch-4}

[Doxy: [`Block`](https://smspp.gitlab.io/smspp-project/d2/dbd/class_s_m_spp__di__unipi__it_1_1_block.html)]
&nbsp; [Source: `SMS++/include/Block.h`]

## 4.1 Concept {#sec-4-1}

The abstract class
[`Block`](https://smspp.gitlab.io/smspp-project/d2/dbd/class_s_m_spp__di__unipi__it_1_1_block.html)
is the cornerstone of SMS++. A `Block` is "(a fragment of) a
mathematical model with a well-understood semantic". Concretely, a
`Block` contains, all of them optional except where noted:

- any number of `Variable` objects, organised in *groups* (where a
  group is a single `Variable`, a `std::vector` of `Variable`s, a
  `boost::multi_array` of `Variable`s with any number of dimensions,
  a `std::vector< std::vector< Variable > >` (a bidimensional
  arrangement whose second dimension is allowed to vary across the
  first, as opposed to the rectangular shape of a
  `boost::multi_array< Variable , 2 >`), or a `std::list` of
  `Variable`s in the dynamic case, ...);
- any number of `Constraint` objects, organised in groups in the
  same sense;
- exactly one `Objective` object (when present at all);
- any number of sub-`Block` objects, recursively, accessed through a
  vector of pointers `v_Block`;
- a pointer to a father `Block`, if any.

The cardinality "any number" includes zero. A `Block` with no
sub-`Block` is called a *leaf* `Block`; the running examples
`MCFBlock` and `BinaryKnapsackBlock` are both leaves. A `Block` may
have a father `Block` (and is then said to be a sub-`Block` of it),
or no father (and is then said to be a *root* `Block`). A `Block`
that is a sub-`Block` of some father is, conceptually, part of the
father's model: every `Variable` of the sub-`Block` is automatically
a `Variable` of the father; the father's `Objective` is conceptually
the sum of its own `Objective` and the `Objective`s of all its
sub-`Block`s (with appropriate handling of opposite senses; see
[Chapter 5](05-variable-constraint-objective.md#ch-5)).

The base class `Block` provides no problem-specific functionality.
What a concrete `:Block` derived from it adds is, in essence:

- the **physical** representation of the problem instance: the
  problem data in its natural form, stored as member fields;
- the **abstract** representation of the problem instance: the
  groups of `Variable`s, `Constraint`s and the `Objective` that the
  abstract representation would consist of, generated on demand by
  the methods `generate_abstract_variables()`,
  `generate_abstract_constraints()`, `generate_dynamic_variables()`,
  `generate_dynamic_constraints()`, `generate_objective()`;
- methods to mutate the physical representation while keeping the
  abstract one in sync, each of them able to issue the appropriate
  `Modification` objects to inform any listening `:Solver` of the
  change.

## 4.2 The identity rule {#sec-4-2}

A design decision that pervades the framework is that the *name* of
a `Variable`, of a `Constraint`, and of any other object that may be
referred to by an algorithm, is its **memory address**. There is no
separate identifier; `&v` is what makes `v` distinguishable from
every other `Variable`.

The immediate consequence is:

> Once a `Variable` or a `Constraint` has been constructed, it
> cannot be moved to a different memory location.

This rules out, in particular, storing dynamic `Variable`s or
dynamic `Constraint`s in a `std::vector`, because growing the vector
may reallocate the underlying storage and silently invalidate every
existing reference. Dynamic `Variable`s and dynamic `Constraint`s
must therefore live in a `std::list`. Static `Variable`s and static
`Constraint`s, by contrast, are allocated once in a `std::vector`
that is never grown or shrunk, and may safely be addressed by index
into that vector.

The same rule applies to a `Block`: a `Block` is referenced by
pointer (`p_Block = Block*`), and its sub-`Block`s are stored as a
`std::vector` of pointers (`v_Block` of type `Vec_Block`). Copying a
`Block` produces a different `Block`, not the same one; if two
`Block`s must be kept "the same up to data", the `R3Block` mechanism
([Chapter 10](10-r3block.md#ch-10)) is the supported way to do it.

## 4.3 Static versus dynamic {#sec-4-3}

Three kinds of contents of a `Block` admit a static / dynamic
distinction:

| Kind | Static | Dynamic |
|---|---|---|
| `Variable` | guaranteed to exist throughout the lifetime of the `Block` | may appear and disappear |
| `Constraint` | guaranteed to exist throughout the lifetime of the `Block` | may appear and disappear |
| sub-`Block` | always static | not supported |

Sub-`Block`s are always static: their number and their concrete
type cannot change after the parent `Block` has been built. This
restriction does not hurt expressiveness in practice and considerably
simplifies the framework's internal logic.

Static contents allow storage by index; dynamic contents allow
storage in a `std::list` only, and are addressed by an iterator or
by a `Block`-specific name. Dynamic contents are the support for
*column generation* (dynamic `Variable`s) and *row generation*
(dynamic `Constraint`s).

## 4.4 Structure of the class {#sec-4-4}

The principal data members of `Block` are (with shortened types for
brevity):

- `p_Block f_Block` — pointer to the father `Block` (`nullptr` for a
  root `Block`);
- `Vec_Block v_Block` — vector of pointers to the sub-`Block`s;
- `std::string f_name` — optional human-readable name, useful for
  diagnostic prints;
- a list of `Solver*` currently registered with this `Block`
  (`v_Solver`);
- a flag `f_at` that caches "is there any `:Solver` listening to
  this `Block` or to any of its ancestor `Block`s?";
- machinery for concurrent access (`lock()`, `read_lock()`, an
  owner thread id), discussed in [Chapter 17](17-parallel.md#ch-17);
- a pointer to a `BlockConfig`, the `Configuration` object that
  controls the optional behaviour of the `Block`.

The principal public methods, grouped by purpose:

- *navigation*: `get_f_Block()`, `set_f_Block(p_Block)`,
  `get_nested_Block(Index)`, `get_nested_Block(name)`,
  `get_number_nested_Blocks()`, `get_nested_Block_index(name)`;
- *contents*: `get_number_static_constraints()`,
  `get_number_static_variables()`, and their dynamic counterparts;
- *abstract-representation construction*:
  `generate_abstract_variables(Configuration*)`,
  `generate_abstract_constraints(Configuration*)`,
  `generate_dynamic_variables(Configuration*)`,
  `generate_dynamic_constraints(Configuration*)`,
  `generate_objective(Configuration*)`;
- *Solver attachment*: `register_Solver(Solver*)`,
  `unregister_Solver(Solver*)`, `unregister_Solvers()`,
  `replace_Solver(Solver*, ...)`, `get_registered_solvers()`;
- *Modification handling*: `add_Modification(sp_Mod, ChnlName)`,
  `anyone_there()`, `concerns_Block()`;
- *checking*: `is_feasible(bool useabstract, Configuration*)`,
  `is_dual_feasible(...)`, `is_optimal(...)`;
- *bounds*: `get_objective_sense()`,
  `get_valid_upper_bound(bool conditional)`,
  `get_valid_lower_bound(bool conditional)`;
- *I/O*: `load(std::istream&, char)`, `load(std::string&, char)`,
  `serialize(netCDF::NcGroup&) const`,
  `deserialize(const netCDF::NcGroup&)`,
  `print(std::ostream&, char)`;
- *reformulation*: `get_R3_Block(Configuration*, base, father)`,
  `map_back_solution(...)`, `map_forward_solution(...)`,
  `map_forward_Modification(...)`, `map_back_Modification(...)`;
- *configuration*: `set_BlockConfig(BlockConfig*)`,
  `get_BlockConfig()`.

The full list of public methods, with their precise signatures and
their detailed specification, is in
[Doxy: `Block`](https://smspp.gitlab.io/smspp-project/d2/dbd/class_s_m_spp__di__unipi__it_1_1_block.html).

## 4.5 Inline example: constructing an `MCFBlock` {#sec-4-5}

The example below builds an `MCFBlock` from in-memory data. It is
deliberately minimal: it neither constructs the abstract
representation nor attaches a `:Solver`. Both will be added in
[Chapter 6](06-solver.md#ch-6) and in [Recipe R1](R1-mcf.md#rec-R1).

```cpp
#include "MCFBlock.h"

using namespace SMSpp_di_unipi_it;

int main()
{
 // construct a root MCFBlock (no father)
 MCFBlock mcf;

 // a tiny 3-node, 3-arc instance:
 //
 //     (0) --1--> (1) --1--> (2)
 //      |                     ^
 //      +--------- 1 ---------+
 //
 // node deficits b = (-2, 0, +2)
 // arc capacities U = (3, 3, 3); arc costs C = (2, 3, 4)

 MCFBlock::Subset      pSn = { 0, 1, 0 };  // arc starting nodes
 MCFBlock::Subset      pEn = { 1, 2, 2 };  // arc ending nodes
 MCFBlock::Vec_FNumber pU  = { 3.0, 3.0, 3.0 };
 MCFBlock::Vec_CNumber pC  = { 2.0, 3.0, 4.0 };
 MCFBlock::Vec_FNumber pB  = { -2.0, 0.0, +2.0 };

 // note the parameter order: ending nodes BEFORE starting nodes
 mcf.load( 3 /* n */ , 3 /* m */ , pEn , pSn , pU , pC , pB );

 // human-readable summary on stdout (the default verbosity prints the graph)
 mcf.print( std::cout );

 return 0;
}
```

A few things are worth noticing.

- The constructor `MCFBlock()` is the default; it produces an empty
  `MCFBlock` with no father. The variant `MCFBlock(Block* father)`
  installs the new `MCFBlock` as a sub-`Block` of `father`.
- The `load(...)` method populates the physical representation. It
  takes ending nodes (`pEn`) before starting nodes (`pSn`); this is
  the signature documented in `MCFBlock.h`.
- No call to `generate_abstract_*()` has been made. The abstract
  representation does not exist yet; it will be created lazily
  later, only if a `:Solver` that needs it is attached.
- No call to any factory function has been made by the user, even
  though the class is registered in the `Block` factory. Factory
  registration is performed by the `:Block` implementer in the
  `.cpp` file, once and for all, through the macro
  `SMSpp_insert_in_factory_cpp_1( MCFBlock );` at namespace scope
  in `MCFBlock.cpp` (the `_1` indicates that the constructor takes
  one optional argument, the father pointer). Once registered, the
  class can be instantiated by name through the `Block` factory; see
  [Chapter 18](18-factories-netcdf.md#ch-18) for the details.

The same `MCFBlock` could equivalently have been built by reading a
DIMACS-formatted text file (with `load(std::istream&)`) or a netCDF
file (with `deserialize(netCDF::NcGroup&)`); both are documented
forms of construction.

## 4.6 Common idioms {#sec-4-6}

### The "nuclear option" `NBModification`

Every form of `load(...)` on an `MCFBlock` (and, by convention, on
every concrete `:Block` that supports `load(...)`) replaces the
problem instance wholesale. If a `:Solver` is already attached when
`load(...)` is invoked, the framework issues a single
`NBModification` — the "nuclear option" — that tells the `:Solver`
"everything has changed; throw away whatever you have cached and
start over". This is the only `Modification` that `load(...)` is
allowed to issue, by convention; incremental changes to the data
have their own dedicated `Modification` types ([Chapter 8](08-modification-janus.md#ch-8)).

The "nuclear option" is the safe default; it is meant to be used
sparingly, because it prevents any form of reoptimization.

### The "anyone listening?" cache `f_at`

A `:Block` does not always need to construct and dispatch a
`Modification` for every change it undergoes. If no `:Solver` is
registered with the `Block` and no `:Solver` is registered with any
of its ancestors, no one will react to a `Modification`, and the
work of constructing it is wasted. To avoid the waste, every
`Block` maintains a boolean cache `f_at` ("anyone there?") that is
true iff some `:Solver` is currently listening to the `Block` or to
any ancestor.

The cache is updated automatically by `register_Solver()` /
`unregister_Solver()` and propagated to all sub-`Block`s
recursively. `:Block` implementers can check it through
`anyone_there()` before paying the cost of constructing a
`Modification`. The default implementation of `anyone_there()` in
the base class returns the cached value; a concrete `:Block` may
override `anyone_there()` to always return `true` when the abstract
representation is currently constructed, because abstract
`Modification`s must be issued anyway in order to keep the two
representations in sync ([Chapter 8](08-modification-janus.md#ch-8)).

### The `father` argument of the constructor

A concrete `:Block` constructor typically takes one optional
argument, a pointer to a father `Block` (default `nullptr`). Passing
a non-null father makes the new `Block` a sub-`Block` of the father;
the framework will automatically install the new `Block` in the
father's `v_Block`. This is the canonical way to assemble a `Block`
tree from the leaves up; it is exemplified by
`CapacitatedFacilityLocationBlock` in its Knapsack Formulation,
which constructs its `BinaryKnapsackBlock` sub-`Block`s by passing
itself as the father ([Chapter 12](12-sub-block.md#ch-12)).

---

With `Block` introduced, the next chapter turns to the three
primitives that live *inside* a `Block`: `Variable`, `Constraint`,
`Objective`.
