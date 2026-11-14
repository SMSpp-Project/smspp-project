# 4. Block {#ch-4}

[Doxy:
[`Block`](https://smspp.gitlab.io/smspp-project/d2/dbd/class_s_m_spp__di__unipi__it_1_1_block.html)]
&nbsp; [Source: `SMS++/include/Block.h`]

## 4.1 Concept {#sec-4-1}

The abstract class
[`Block`](https://smspp.gitlab.io/smspp-project/d2/dbd/class_s_m_spp__di__unipi__it_1_1_block.html)
represents "(a fragment of) a mathematical model with a
well-understood semantic". A `Block` contains (all of the following
being optional):

- any number of `Variable` objects, organized in *groups*, where a
  group is a single `Variable`, a `std::vector` of `Variable`s, a
  `boost::multi_array` of `Variable`s with any number of dimensions,
  a `std::vector< std::vector< Variable > >` (a two-dimensional
  arrangement whose rows may have different lengths, as opposed to
  the rectangular shape of a `boost::multi_array< Variable , 2 >`),
  or, for the dynamic case, a `std::list` of `Variable`s;
- any number of `Constraint` objects, organized in groups in the same
  sense;
- at most one `Objective` object;
- any number of sub-`Block`s, recursively, accessed through the
  vector of pointers `v_Block`;
- a pointer to a father `Block`.

"Any number" includes zero. A `Block` with no sub-`Block` is called a
*leaf*, as the running examples `MCFBlock` and `BinaryKnapsackBlock`
are; a `Block` with a father is a *sub-`Block`* of it, and one without
a father is a *root*. A sub-`Block` is conceptually part of the model
of its father, whose `Variable`s comprise those of the sub-`Block`.
Also, the `Objective` of the father is (conceptually) the sum of the
one it carries and of those of its sub-`Block`s, with the appropriate
handling of the senses and of the weights (see [Chapter
5](05-variable-constraint-objective.md#ch-5)).

Note that the base class provides no functionality specific to a
problem. A concrete `:Block` adds to it, in essence, the *physical*
representation of the instance, i.e., the data of the problem in their
natural form, stored as fields of the class. It also adds the
*abstract* representation, i.e., the groups of `Variable`s and
`Constraint`s and the `Objective`, generated on demand by the methods
`generate_abstract_variables()`, `generate_abstract_constraints()`,
`generate_dynamic_variables()`, `generate_dynamic_constraints()` and
`generate_objective()`, and the methods that change the physical
representation while keeping the abstract one in step, each of which
can issue the appropriate `Modification`s to inform the listening
`:Solver`s.

## 4.2 The identity rule {#sec-4-2}

A decision that pervades the framework is that the *name* of a
`Variable`, of a `Constraint` and of the other objects an algorithm
may refer to is their *memory address*: there is no separate
identifier, and `&v` distinguishes `v` from any other `Variable`.
Hence, once a `Variable` or a `Constraint` has been constructed, it
cannot be moved to a different location in memory.

This rule decides the containers of the groups. Growing a
`std::vector` beyond its capacity reallocates its storage and
invalidates the pointers to its elements, while inserting into or
erasing from a `std::list` invalidates no pointer to the other
elements (ISO/IEC 14882:2020, [vector.modifiers] and
[list.modifiers]). Therefore the dynamic `Variable`s and
`Constraint`s, whose number changes, live in `std::list`s, while the
static ones live in `std::vector`s (or `boost::multi_array`s) that are
sized once, when the group is created, and never resized afterwards,
and which can therefore be addressed by index as well.

Also, the same rule applies to a `Block`, which is referred to by
pointer (`p_Block` is `Block *`); its sub-`Block`s are stored as a
`std::vector` of pointers (`v_Block`, of type `Vec_Block`), and moving
the pointers does not move the `Block`s. The copy constructor of
`Block` is deleted: a copy, which is anyway a different `Block`, is
obtained through `get_R3_Block()`, and the `R3Block` mechanism (cf.
[Chapter 10](10-r3block.md#ch-10)) is the supported way to keep two
`Block`s "the same up to the data".

## 4.3 Static and dynamic contents {#sec-4-3}

Three kinds of contents of a `Block` admit a distinction between
static and dynamic:

| Kind | Static | Dynamic |
|---|---|---|
| `Variable` | exists for the whole lifetime of the `Block` | may appear and disappear |
| `Constraint` | exists for the whole lifetime of the `Block` | may appear and disappear |
| sub-`Block` | always static | not supported |

Sub-`Block`s are always static, i.e., their number and their concrete
types do not change after the `Block` has been built. This restriction
does not seem to limit the expressiveness much in practice, and it
simplifies considerably the internal logic of the framework. An
exception is `set_structure()`, by which a `:Block` whose tree of
sub-`Block`s is a modeling choice makes that choice; it is meant to be
called before any abstract representation is generated and any
`:Solver` is attached, and changing the tree afterwards is a "nuclear
option" that issues an `NBModification` (cf.
[§4.6](04-block.md#sec-4-6)).

One can store static contents by index; dynamic contents can only be
stored in a `std::list`, and they are addressed by an iterator or by
a name specific to the `:Block`. Dynamic contents support *column
generation* (dynamic `Variable`s) and *row generation* (dynamic
`Constraint`s).

## 4.4 Structure of the class {#sec-4-4}

The main fields of `Block` describe its place in the tree, its
listeners and its configuration. In particular, the pointer `f_Block`
to the father is `nullptr` for a root, `v_Block` is the vector of the
pointers to the sub-`Block`s, `f_name` is an optional name (useful in
diagnostic output), and `v_Solver` is the list of the `Solver`s
registered with the `Block`. The flag `f_at` caches whether some
`:Solver` is listening to an ancestor of the `Block` (Lemma 4.1
below); then come the machinery for concurrent access (`lock()`,
`read_lock()`, the identifier of the owner thread; see [Chapter
17](17-parallel.md#ch-17)), and the pointer `f_BlockConfig` to the
`BlockConfig`, the `Configuration` object that controls the optional
behavior of the `Block` (cf. [Chapter 11](11-configuration.md#ch-11)).

The main public methods, grouped by purpose, are the following:

- *navigation*: `get_f_Block()`, `set_f_Block(p_Block)`,
  `get_nested_Block(Index)`, `get_nested_Block(name)`,
  `get_number_nested_Blocks()`, `get_nested_Block_index(name)` and
  `get_nested_Block_index(Block *)`, `add_nested_Block(Block *,
  bool front)`;
- *contents*: `get_number_static_variables()`,
  `get_number_static_constraints()`, their dynamic counterparts, and
  `get_static_variable_groups()`, `get_static_constraint_groups()`
  and their dynamic counterparts
  (cf. [§5.5](05-variable-constraint-objective.md#sec-5-5));
- *construction of the abstract representation*:
  `generate_abstract_variables(Configuration *)`,
  `generate_abstract_constraints(Configuration *)`,
  `generate_dynamic_variables(Configuration *)`,
  `generate_dynamic_constraints(Configuration *)`,
  `generate_objective(Configuration *)`;
- *attachment of `Solver`s*: `register_Solver(Solver *, bool
  tofront)`, `unregister_Solver(Solver *, bool deleteold)`,
  `unregister_Solvers(bool deleteold)`, `replace_Solver(...)`,
  `get_registered_solvers()`;
- *handling of `Modification`s*: `add_Modification(sp_Mod,
  ChnlName)`, `anyone_there()`, `anyone_there_for(...)`,
  `concerned()`, `concerned_by()`, and the channels
  `open_channel()`, `close_channel()`, `clear_channel()`,
  `set_default_channel()` (cf. [Chapter 8](08-modification-janus.md#ch-8));
- *checks*: `is_feasible(bool useabstract, Configuration *)`,
  `is_optimal(bool useabstract, Configuration *)`,
  `is_sol_feasible(Solution *, Configuration *)`,
  `is_sol_optimal(Solution *, Configuration *)`
  (cf. [§7.5](07-physical-abstract.md#sec-7-5));
- *bounds and weights*: `get_objective_sense()`,
  `get_valid_upper_bound(bool conditional)`,
  `get_valid_lower_bound(bool conditional)`,
  `set_objective_weight(double)`, `set_variable_weight(double)`
  (cf. [§5.4](05-variable-constraint-objective.md#sec-5-4));
- *input and output*: `load(std::istream &, char)`,
  `load(const std::string &, char)` (which opens the named file and
  calls the former), `serialize(netCDF::NcGroup &) const`,
  `deserialize(const netCDF::NcGroup &)`,
  `print(std::ostream &, char)`;
- *reformulation*: `get_R3_Block(Configuration *, Block * base,
  Block * father)`, `map_back_solution(...)`,
  `map_forward_solution(...)`, `map_forward_Modification(...)`,
  `map_back_Modification(...)` (cf. [Chapter 10](10-r3block.md#ch-10));
- *configuration*: `set_BlockConfig(BlockConfig *, bool)`,
  `get_BlockConfig()`, and `set_structure(Configuration *)`;
- *solutions*: `get_Solution(Configuration *, bool emptys)`
  (cf. [Chapter 9](09-solution.md#ch-9)).

The complete list of the public methods, with their signatures and
their specifications, is in [Doxy:
`Block`](https://smspp.gitlab.io/smspp-project/d2/dbd/class_s_m_spp__di__unipi__it_1_1_block.html).

## 4.5 Example: constructing an `MCFBlock` {#sec-4-5}

Below we build an `MCFBlock` out of data in memory. It is
deliberately minimal, in that it neither constructs the abstract
representation nor attaches a `:Solver`; both are added in
[Chapter 6](06-solver.md#ch-6) and in [Recipe R1](R1-mcf.md#rec-R1).

```cpp
#include "MCFBlock.h"
#include <iostream>

using namespace SMSpp_di_unipi_it;

int main()
{
 // construct a root MCFBlock (no father)
 MCFBlock mcf;

 // a tiny 3-node, 3-arc instance:
 //
 //     (1) --1--> (2) --1--> (3)
 //      |                     ^
 //      +--------- 1 ---------+
 //
 // node deficits b = (-2, 0, +2)
 // arc capacities U = (3, 3, 3); arc costs C = (2, 3, 4)

 // node names go from 1 to n
 MCFBlock::Subset      pSn = { 1, 2, 1 };  // arc starting nodes
 MCFBlock::Subset      pEn = { 2, 3, 3 };  // arc ending nodes
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

Its constructor `MCFBlock( Block * father = nullptr )` produces an
empty `MCFBlock`, which is a root if no father is given. The method

```cpp
void load( Index n , Index m , c_Subset & pEn , c_Subset & pSn ,
           c_Vec_FNumber & pU = {} , c_Vec_CNumber & pC = {} ,
           c_Vec_FNumber & pB = {} ,
           Index dn = 0 , Index dm = 0 , Index mdn = 0 , Index mdm = 0 );
```

fills the physical representation. It takes the ending nodes `pEn`
*before* the starting nodes `pSn`, whose names go from 1 to `n`
(`load()` throws `std::invalid_argument` on a name outside this
range). An empty `pU` means that all the capacities are infinite, an
empty `pC` that all the costs are 0, and an empty `pB` that all the
deficits are 0 (a circulation problem); the last four arguments give
the current numbers of dynamic nodes and arcs and their maximum
numbers. With the default verbosity, `print()` writes the numbers of
nodes and arcs, the nonzero deficits and the arcs with their costs and
capacities, while with the verbosity `'C'` it writes the instance in
the DIMACS format.

No call to `generate_abstract_*()` appears in the example. Hence, the
constraints and the objective of the abstract representation do not
exist yet, and they are created only if a `:Solver` that needs them is
attached; the flow `ColVariable`s, instead, are constructed by
`load()`, for the reason explained in
[§7.6](07-physical-abstract.md#sec-7-6).

No factory is called by the user either, although the class is
registered in the factory of `Block`s. The registration is done by the
author of the `:Block`, once and for all, by the macro
`SMSpp_insert_in_factory_cpp_1( MCFBlock );` at namespace scope in
`MCFBlock.cpp`, where the suffix `_1` means that the constructor takes
one argument (the pointer to the father). Once it is registered, one
can instantiate the class by its name through the factory (cf.
[Chapter 18](18-factories-netcdf.md#ch-18)).

The same `MCFBlock` could equivalently have been built by reading a
text file in the DIMACS format (with `load(std::istream &)`) or a
netCDF file (with `deserialize(const netCDF::NcGroup &)`).

## 4.6 Idioms {#sec-4-6}

### The "nuclear option" `NBModification`

Each form of `load(...)` of an `MCFBlock` (and, by convention, of the
other concrete `:Block`s that have one) replaces the instance
altogether. If a `:Solver` is attached when `load(...)` is called, the
`:Block` issues a single `NBModification`, the "nuclear option",
meaning that everything has changed and that whatever the `:Solver`
has cached has to be thrown away. By convention this is the only
`Modification` that `load(...)` issues, while the incremental changes
of the data have their own types of `Modification` (cf. [Chapter
8](08-modification-janus.md#ch-8)). This nuclear option is always
safe, but it is meant to be used sparingly, since it prevents any form
of reoptimization.

### The cache of the listeners `f_at`

A `:Block` need not construct and dispatch a `Modification` for each
change it undergoes: if no `:Solver` is registered with the `Block` or
with any of its ancestors, nobody will react to the `Modification`,
and the work of constructing it is wasted. To avoid this waste, each
`Block` keeps the flag `f_at`, and the method `anyone_there()` returns
`f_at || ! v_Solver.empty()`. This flag is maintained by
`register_Solver()`, `unregister_Solver()`, `unregister_Solvers()` and
`add_nested_Block()`, through the protected method `anyone_there( bool
)` that a `Block` calls on each of its sub-`Block`s when the answer to
"is anyone listening to me?" changes. In what follows, a sub-`Block`
is *inserted properly* if its `f_at` is `false` before the insertion
(which holds for a newly constructed `Block`), and if it is inserted
either by `add_nested_Block()` or by a write into `v_Block` made while
no `Solver` is registered with the father or with any of its
ancestors. The property on which the `:Block`s rely is then the
following.

**Lemma 4.1** (Listeners). *Consider a tree of `Block`s whose
sub-`Block`s have all been inserted properly, and whose `Solver`s have
been registered and unregistered only through the methods of
`Block`. Then, at any time and for each `Block` $B$ of the tree,
$f_{at}(B)$ is `true` if and only if some `Solver` is registered with
a proper ancestor of $B$, and therefore `anyone_there()` is `true` on
$B$ if and only if some `Solver` is registered with $B$ or with one of
its ancestors.*

*Proof.* Let $L(B)$ denote the predicate "`! v_Solver.empty()` or
$f_{at}(B)$" for a `Block` $B$, i.e., the value of `anyone_there()` on
$B$. We prove that the invariant $f_{at}(C) = L(\mathrm{father}(C))$
holds for each non-root $C$, and that $f_{at}(R)$ is `false` for the
root $R$. Our claim then follows by induction on the depth of $B$,
since unfolding the invariant shows that $L(B)$ holds if and only if
some `Block` on the path from $B$ to the root has a registered
`Solver`.

The invariant holds when the tree is built, since all the flags are
`false` and no `Solver` is registered. Of the operations that may
change some $L(\cdot)$, consider first `register_Solver(S)` on a
`Block` $P$. If $P$ already had a `Solver`, or if $f_{at}(P)$ held,
then $L(P)$ was already `true`, and the method does not touch the
flags of the sub-`Block`s (it only appends $S$, or returns at once if
$S$ is already registered). Otherwise $L(P)$ goes from `false` to
`true`, and the method calls `anyone_there( true )` on each
sub-`Block` of $P$. Now `anyone_there( true )` on a `Block` $C$ with
$f_{at}(C)$ `false` sets $f_{at}(C)$ to `true`, which restores the
invariant at $C$. If $C$ has a `Solver`, then $L(C)$ was `true` and
remains so, and the method stops; otherwise $L(C)$ has gone from
`false` to `true`, and the call is repeated on the sub-`Block`s of
$C$. If instead $f_{at}(C)$ was already `true`, then by the invariant
$L(\mathrm{father}(C))$ was already `true`, a contradiction with the
hypothesis that it has just changed; hence this case does not arise
below a `Block` whose $L$ changes. By induction on the height of the
sub-tree of $P$, after the call the invariant holds again at each
`Block` of that sub-tree, and it is unchanged elsewhere. The case of
`unregister_Solver()` and `unregister_Solvers()` is symmetric. They
call `anyone_there( false )` on the sub-`Block`s if and only if the
last `Solver` of $P$ goes away while $f_{at}(P)$ is `false`, i.e.,
when $L(P)$ goes from `true` to `false`; then `anyone_there( false )`
propagates downwards through the `Block`s without `Solver`s, stopping
at those that have one, whose $L$ does not change. Finally,
`add_nested_Block(C)` on $P$ calls `anyone_there( true )` on $C$ if
and only if $L(P)$ holds, and $f_{at}(C)$ is `false` by hypothesis
before the call; hence after the call $f_{at}(C) = L(P)$, and the
argument above shows that the invariant also holds in the sub-tree of
$C$. A proper insertion by a direct write into `v_Block` happens while
$L(P)$ is `false`, and the invariant holds since $f_{at}(C)$ is
`false`. No other method of `Block` changes $f_{at}$ or `v_Solver`.
$\square$

The hypotheses of Lemma 4.1 are not idle. A sub-`Block` moved from a
father that is listened to into one that is not would keep a stale
`f_at == true`, since `add_nested_Block()` only raises the flag.
Conversely, one written into `v_Block` after a `Solver` has been
registered with an ancestor would keep `f_at == false`, and the
`Modification`s it issues would be dropped by
`Block::add_Modification()`, which returns at once when
`anyone_there()` is `false`.

`:Block` implementers check `anyone_there()` before paying the cost of
constructing a `Modification`. A finer test is `anyone_there_for( what
)`. Each `Modification` declares, as a bit mask of type
`Modification::ModConcern`, what it changes (`eModPhys` for the
physical representation, and `eModVarData`, `eModCnsSide`, `eModObj`
and so on for the parts of the abstract one), and each `:Solver`
declares, through `Solver::concerned_by()`, the kinds it reads. Then
`Block::concerned()` is the "or" of what the `:Solver`s of the `Block`
and of its ancestors read, and of the `concerned_by()` of the `:Block`
(typically, the abstract `Modification`s that it follows to keep its
physical representation in step; see [Chapter
8](08-modification-janus.md#ch-8)). A `:Block` can therefore avoid
constructing a `Modification` of a kind that nobody reads.

### The `father` argument of the constructor

The constructor of a concrete `:Block` typically takes one optional
argument, the pointer to the father `Block` (`nullptr` by default),
which it records as the father of the new `Block`. However, the
constructor does not put the new `Block` into the `v_Block` of the
father: this is done by the father, either by writing into its
`v_Block` or by calling `add_nested_Block()`, which also sets the
pointer to the father and the flag `f_at` (Lemma 4.1). This is the
intended way to assemble a tree of `Block`s, and it is the one of
`CapacitatedFacilityLocationBlock` in its knapsack formulation, which
constructs its `BinaryKnapsackBlock` sub-`Block`s passing itself as
the father and stores them in its `v_Block` (cf. [Chapter
12](12-sub-block.md#ch-12)).

---

With `Block` introduced, we turn in the next chapter to the three
primitives that live *inside* a `Block`: `Variable`, `Constraint` and
`Objective`.
