# 12. Sub-Block and the recursive flow of Modifications {#ch-12}

[Source: `SMS++/include/Block.h`;
`CapacitatedFacilityLocationBlock/include/CapacitatedFacilityLocationBlock.h`,
`BinaryKnapsackBlock/include/BinaryKnapsackBlock.h`]

[Chapter 4](04-block.md#ch-4) introduced the `Block` tree in the abstract; [Chapter 5](05-variable-constraint-objective.md#ch-5)
fixed the composition rule for `Objective`s; [Chapter 8](08-modification-janus.md#ch-8) described
how `Modification`s travel. This chapter draws those threads
together into the single picture of a *recursive `Block` tree* and
makes explicit the consequences for data, for solving, and for
concurrency.

## 12.1 The recursive Block tree {#sec-12-1}

A `Block` may contain any number of sub-`Block`s, each of which may
in turn contain its own sub-`Block`s, to any depth. The structure
is a tree: every `Block` has at most one father (the root has
none), and the sub-`Block`s of a given `Block` are held in its
`v_Block` vector of pointers ([§4.4](04-block.md#sec-4-4)).

The defining restriction ([§4.3](04-block.md#sec-4-3)) is that **sub-`Block`s are always
static**: their number and their concrete types are fixed once the
parent `Block` has been built and cannot change afterwards. There
is no "dynamic sub-`Block`" analogous to dynamic `Variable`s or
`Constraint`s. This restriction costs little in expressiveness — a
model whose component count varies is usually better expressed
with a fixed set of sub-`Block`s some of which are "switched off"
— and it considerably simplifies the framework's bookkeeping, in
particular the locking and the `Modification` routing described
below.

## 12.2 How a model is split across the tree {#sec-12-2}

The point of the tree is that a structured model is *decomposed*
across it: each sub-`Block` carries the part of the model that is
local to it, and the parent carries only what *links* its
sub-`Block`s together.

Two composition rules, already stated in earlier chapters, make
the tree add up to a single coherent model:

- **`Variable`s** of a sub-`Block` are, conceptually, also
  `Variable`s of the parent ([§4.1](04-block.md#sec-4-1)).
- **`Objective`s** sum up the tree ([§5.4](05-variable-constraint-objective.md#sec-5-4)): the overall objective
  of a `Block` is its own `Objective` plus the `Objective`s of all
  its sub-`Block`s, recursively. A parent that carries no
  `Objective` of its own still has a well-defined overall
  objective, made up entirely of those of its sub-`Block`s.

What may, at first, look like a third rule — "a `Constraint` lives
in the parent and links the `Variable`s of its children" — is in
fact only the *most common pattern*, not a constraint imposed by
the framework. The actual rule is more permissive:

> a `Constraint` defined anywhere in the `Block` tree may
> reference `Variable`s defined anywhere in the tree.

Compositionality makes one *expect* the references to follow
inclusion — `Variable`s of two different sub-`Block`s coupled by a
`Constraint` placed in their common parent — and a well-designed
`:Block` library will largely follow that expectation, because it
keeps each `Block` understandable in isolation. But it is not a
hard requirement. A `Constraint` in the parent may legitimately
involve `Variable`s of the parent together with `Variable`s of a
child; and a `Constraint` *in a child* may even reference a
`Variable` of the parent. The latter happens, for instance, with
the `Gamma` variable in the dual formulation of
`PolyhedralFunctionBlock`, and with the `DCNetworkBlock`s nested
inside a `DesignNetworkBlock`. Such cross-references require
specific support — the sub-`Block` must expose a *setter* for the
"external" `Variable` so that the parent (or whoever builds the
model) can wire it in — but they are entirely possible and
sometimes the natural way to express a problem.

The data of the problem is split accordingly: data that belongs to
one component lives in the corresponding sub-`Block`; data that
expresses the coupling lives wherever the modeller chose to place
the linking `Constraint`s — most often, but not necessarily, in
the common parent.

## 12.3 The recursive flow of Modifications {#sec-12-3}

A `:Solver` attached to a `Block` is responsible for solving the
*entire* sub-tree rooted at that `Block` ([§6.1](06-solver.md#sec-6-1)). It follows that
such a `:Solver` must be told about changes occurring *anywhere*
in that sub-tree, not only in the `Block` it is directly attached
to.

This is exactly what the framework does: a `Modification` issued
by a `Block` travels up the chain of its ancestors, recursively up
to the root, **and is delivered to all the `:Solver`s registered
with any `Block` along that chain** — the issuing `Block` itself
and every one of its ancestors. A change deep in a leaf
sub-`Block` therefore reaches a `:Solver` attached to the root,
because the root is an ancestor of the leaf, and equally any
`:Solver` attached to any intermediate `Block` between the two. The "anyone listening" cache `f_at` ([§8.7](08-modification-janus.md#sec-8-7)) records,
at each `Block`, whether any ancestor has a `:Solver` registered,
so that a `Block` whose sub-tree no one is solving does not waste
effort constructing `Modification`s.

Conversely, a `:Solver` attached to a sub-`Block` is *not* told
about changes happening in the parent or in sibling sub-`Block`s
(they are above or outside its sub-tree, [§6.1](06-solver.md#sec-6-1)). This is what makes
it sound to attach a specialised `:Solver` to a single leaf
sub-`Block` while a different `:Solver` solves the whole tree from
the root: each sees exactly the changes relevant to its own
responsibility.

## 12.4 Locking propagates down the tree {#sec-12-4}

Concurrency control ([Chapter 17](17-parallel.md#ch-17)) is also recursive. Acquiring a
write lock on a `Block` with `lock()` automatically locks all of
its sub-`Block`s, recursively (`lock_sub_block`); the analogous
`read_lock()` / `read_lock_sub_block` acquire shared read locks
down the tree. The rationale is the same as for `Modification`
routing: a `:Solver` operating on a `Block` operates on its whole
sub-tree, so to obtain exclusive access it must lock the whole
sub-tree, not just the root of it.

This recursive locking is also what underpins the immediacy
guarantee of [§8.4](08-modification-janus.md#sec-8-4): an abstract change to a `Block` is performed
while the `Block` is locked, hence while its entire sub-tree is
locked, so no concurrent mutation can be interleaved between the
change and the `Block`'s reaction to it.

## 12.5 Inline example: CFL in the Knapsack Formulation {#sec-12-5}

The Knapsack Formulation (KskForm) of
`CapacitatedFacilityLocationBlock` is the canonical small example
of a non-trivial tree. Selected by setting
`f_static_variables_Configuration` to `SimpleConfiguration<int>(1)`
([§11.8](11-configuration.md#sec-11-8)), it makes the CFL `Block` grow `f_n_facilities`
sub-`Block`s, each a
[`BinaryKnapsackBlock`](https://smspp.gitlab.io/smspp-project/dc/d2f/class_s_m_spp__di__unipi__it_1_1_binary_knapsack_block.html),
one per facility. The split is exactly as [§12.2](12-sub-block.md#sec-12-2) describes:

- each sub-`BinaryKnapsackBlock` carries the capacity constraint
  of one facility and that facility's transportation/design
  variables (the per-facility knapsack);
- the parent
  [`CapacitatedFacilityLocationBlock`](https://smspp.gitlab.io/smspp-project/dc/d34/class_s_m_spp__di__unipi__it_1_1_capacitated_facility_location_block.html)
  carries only the customer-satisfaction *linking* constraints
  $\sum_i X_{ij} = 1$, which couple `Variable`s living in
  different sub-`Block`s;
- the overall objective is the sum of the per-facility knapsack
  objectives (the parent carries no objective of its own in this
  formulation).

```cpp
#include "CapacitatedFacilityLocationBlock.h"
#include "BinaryKnapsackBlock.h"

using namespace SMSpp_di_unipi_it;

int main()
{
 CapacitatedFacilityLocationBlock cfl;
 cfl.load( /* ... a CFL instance ... */ );

 // select the Knapsack Formulation: the Block will grow one
 // BinaryKnapsackBlock sub-Block per facility
 auto * bc = new BlockConfig;
 bc->f_static_variables_Configuration = new SimpleConfiguration< int >( 1 );
 cfl.set_BlockConfig( bc );
 cfl.generate_abstract_variables();     // builds the sub-Block tree
 cfl.generate_abstract_constraints();   // builds the linking constraints
 cfl.generate_objective();

 // the tree is now populated: navigate it
 const auto nf = cfl.get_number_nested_Blocks();   // == f_n_facilities
 for( Block::Index i = 0 ; i < nf ; ++i ) {
  auto * bk = dynamic_cast< BinaryKnapsackBlock * >( cfl.get_nested_Block( i ) );
  // bk is the knapsack sub-Block for facility i
  }

 // a change in a leaf sub-Block reaches a Solver attached at the root:
 // (suppose a Solver has been registered on cfl)
 auto * bk0 = dynamic_cast< BinaryKnapsackBlock * >( cfl.get_nested_Block( 0 ) );
 bk0->chg_weight( /* new weight */ 5.0 , /* item */ 2 , eModBlck );
 // the BinaryKnapsackBlockMod issued by bk0 travels up to cfl and is
 // delivered to every Solver registered on cfl (the root of bk0's tree)

 return 0;
}
```

The example is exactly the configuration that [Recipe R4](R4-cfl-lagrangian.md#rec-R4) solves
with a `LagrangianDualSolver`: the solver attached to the root
`cfl` dualises the linking constraints, leaving the per-facility
knapsacks as independent sub-problems, each solvable by a
`DPBinaryKnapsackSolver` attached to its sub-`Block`. The
recursive `Modification` flow is what keeps that arrangement
correct: a change to a facility's data, wherever it is made,
reaches the root solver that needs to know about it.

## 12.6 Idioms {#sec-12-6}

**Prefer coupling in the parent and locality in the children —
but know that this is a guideline, not a rule.** As [§12.2](12-sub-block.md#sec-12-2) made
precise, the framework imposes *no* restriction on where a
`Constraint` lives relative to the `Variable`s it references: any
`Constraint` may reference any `Variable`, anywhere in the tree.
The general rule is, in effect, that there is no rule. What there
*is* is a strong expectation, born of logical composability: a
sub-`Block` that is self-contained — referencing external
`Variable`s only through explicit, supported setters, and only
where the model genuinely requires it — stays understandable in
isolation and reusable across contexts, whereas one that "reaches
sideways" into a sibling for no compelling reason makes both
`Block`s harder to reason about and to reuse. Place linking
`Constraint`s where they make the decomposition clearest; that is
usually, but not always, the common parent.

**Attach the tree-solving `:Solver` at the root of the sub-tree it
must solve.** Because `Modification`s flow upward, a `:Solver` that
must account for the whole model is attached at the root; a
specialised `:Solver` for one component is attached at that
component's sub-`Block`. The two coexist precisely because each is
told only about the changes within its own responsibility.

**Do not expect dynamic sub-`Block`s.** If the number of
components must vary, model the maximum set of sub-`Block`s once
and switch components on and off through their data (fixing
`Variable`s, relaxing `Constraint`s), rather than trying to add or
remove sub-`Block`s at runtime.
