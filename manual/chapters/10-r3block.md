# 10. R3 Blocks: Reformulation, Relaxation, Restriction {#ch-10}

[Source: [`SMS++/include/Block.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h) (R3 Block methods),
[`AbstractBlock.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/AbstractBlock.h), [`UpdateSolver.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/UpdateSolver.h),
[`R3BSolver.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/R3BSolver.h);
[`CapacitatedFacilityLocationBlock/include/CapacitatedFacilityLocationBlock.h`](https://gitlab.com/smspp/capacitatedfacilitylocationblock/-/blob/develop/include/CapacitatedFacilityLocationBlock.h);
the configuration files `R3BSCfg-*.txt` and the test `CFL_test` of
[`tests/CapacitatedFacilityLocationBlock`](https://gitlab.com/smspp/tests/-/tree/develop/CapacitatedFacilityLocationBlock)]

We are concerned in this chapter with the *R3 Blocks* of a `Block`, i.e., the
Reformulations, Relaxations and Restrictions that a `Block` can produce of
itself, and with the methods that carry solutions and `Modification`s between
a `Block` and one of its R3 Blocks. We define the three relations, with the
classical argument by which a Relaxation is recognized
([§10.1](10-r3block.md#sec-10-1)), and we explain why a separate mechanism
is needed instead of a copy of the `Variable`s
([§10.2](10-r3block.md#sec-10-2)). Then we give the interface
([§10.3](10-r3block.md#sec-10-3)), the copy that any `Block` has through
the mirror of its abstract representation ([§10.4](10-r3block.md#sec-10-4)),
and the flow relaxation of the CFL, a non-trivial case
([§10.5](10-r3block.md#sec-10-5)). The rest of the chapter concerns an R3
Block that follows the changes of its original. We describe the
`UpdateSolver`, which keeps the two in sync
([§10.6](10-r3block.md#sec-10-6)), we give slope scaling on the flow
relaxation as an example ([§10.7](10-r3block.md#sec-10-7)), and after the
idioms ([§10.8](10-r3block.md#sec-10-8)) we describe the `R3BSolver` (since
SMS++ 0.9.0), which solves a `Block` through one of its R3 Blocks configured
from file alone
([§10.9](10-r3block.md#sec-10-9)).

## 10.1 Concept {#sec-10-1}

A model may have to be *transformed* for algorithmic reasons, and SMS++
supports this by letting a `Block` produce a different `Block` that is related
to it by one of three relations (cf.
[`Block.h:67-85`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L67-85)).
Let the original `Block` encode the problem

$$
(P) \qquad v(P) = \min \{\, c(x) : x \in X \,\}
$$

(a maximization is treated with the obvious changes of sign), and let
the new `Block` encode $(R)\; v(R) = \min \{\, c'(z) : z \in Z \,\}$ in
its `Variable`s $z$. Then $(R)$ is a *Reformulation* of $(P)$ if
its optimal solutions can be mapped to optimal solutions of $(P)$
(the two are equivalent, but one may be easier to work with), and a
*Relaxation* if $v(R) \le v(P)$ holds for all the instances of the data,
while $(R)$ is hopefully easier to solve. It is a *Restriction* if
$Z$ maps into a strict subset of $X$, which again may make $(R)$ easier
to solve, at the price of possibly cutting off the optimal solutions of
$(P)$. The three are collectively called the *R3 Blocks* of the
original `Block`.

The usual way of proving that $(R)$ is a Relaxation, which is the one
used in [§10.4](10-r3block.md#sec-10-4) and
[§10.5](10-r3block.md#sec-10-5), is the following classical argument,
stated here once for the chapter.

<span id="lem-10-1"></span>**Lemma 10.1.** *Let $\phi : X \to Z$ be a map such
that $c'(\phi(x)) \le c(x)$ for all $x \in X$. Then $v(R) \le v(P)$. If,
furthermore, $\psi : Z \to X$ satisfies $c(\psi(z)) \le c'(z)$ for all $z \in
Z$, then $v(R) = v(P)$, and $\psi$ maps each optimal solution of $(R)$ to an
optimal solution of $(P)$.*

*Proof.* For all $x \in X$ one has $v(R) \le c'(\phi(x)) \le c(x)$, and
taking the infimum over $X$ gives $v(R) \le v(P)$ (the inequality is
trivially true if $X = \emptyset$, i.e., $v(P) = +\infty$). Symmetrically,
the second map gives $v(P) \le v(R)$, hence equality. If $z^*$ is optimal
for $(R)$, then $\psi(z^*) \in X$ and
$c(\psi(z^*)) \le c'(z^*) = v(R) = v(P)$, hence $\psi(z^*)$ is optimal
for $(P)$. $\square$

In words, a Relaxation needs a map from the original to the new
`Variable`s that does not increase the cost, and a Reformulation needs a
second map back with the same property, which `map_back_solution()`
implements ([§10.3](10-r3block.md#sec-10-3)).

The set of R3 Blocks of a given `Block` is decided by the `Block`, and the
base `Block` class provides no general one. A concrete
`:Block` that supports an R3 Block implements it explicitly (there is
no automatic mechanism), with the only exception of the copy of the abstract
representation that an `AbstractBlock` can make of any `Block`
([§10.4](10-r3block.md#sec-10-4)).

## 10.2 Why an R3 Block, and not a copy of the `Variable`s {#sec-10-2}

A dedicated mechanism, rather than "just copy the relevant `Variable`s
and `Constraint`s", is needed because of the identity rule of
[§4.2](04-block.md#sec-4-2): the *name* of a `Variable` is its memory
address, and therefore a copy of a `Variable` is necessarily a
*different* `Variable`. An R3 Block thus lives in a separate set of
`Variable`s and `Constraint`s, disjoint from those of the original, and
this makes an R3 Block useful for algorithmic purposes (to branch,
fix, relax, or solve a separate copy in a separate thread): it is
independent, and operating on it does not perturb the original.

Independence has a price, however: a solution found on the R3 Block
lives in the `Variable`s of the R3 Block, and it is of no direct use to
a `:Solver` working on the original. The framework provides a small
family of *mapping* methods that move solution and modification
information back and forth between an original `Block` and one of its
R3 Blocks.

## 10.3 The R3 Block API {#sec-10-3}

All the following are virtual methods of `Block`, declared as

```cpp
virtual Block * get_R3_Block( Configuration * r3bc = nullptr ,
                              Block * base = nullptr ,
                              Block * father = nullptr );

virtual void map_back_solution( Block * R3B ,
                                Configuration * r3bc = nullptr ,
                                Configuration * solc = nullptr );
virtual void map_forward_solution( Block * R3B ,
                                   Configuration * r3bc = nullptr ,
                                   Configuration * solc = nullptr );

virtual bool map_forward_Modification( Block * R3B , c_p_Mod mod ,
                                       Configuration * r3bc = nullptr ,
                                       ModParam issuePMod = eNoBlck ,
                                       ModParam issueAMod = eModBlck );
virtual bool map_back_Modification( Block * R3B , c_p_Mod mod ,
                                    Configuration * r3bc = nullptr ,
                                    ModParam issuePMod = eNoBlck ,
                                    ModParam issueAMod = eModBlck );

virtual void map_forward_Modifications( Block * R3B , Lst_sp_Mod & lmod ,
                                        Configuration * r3bc = nullptr ,
                                        ModParam issuePMod = eNoBlck ,
                                        ModParam issueAMod = eModBlck );
virtual void map_back_Modifications( Block * R3B , Lst_sp_Mod & lmod ,
                                     Configuration * r3bc = nullptr ,
                                     ModParam issuePMod = eNoBlck ,
                                     ModParam issueAMod = eModBlck );
```

and a concrete `:Block` overrides the ones it chooses to support. Their
meaning is as follows.

- `get_R3_Block()` produces an R3 Block; `r3bc` selects *which* one (a
  `:Block` may offer several), and `base` and `father` are discussed below. A
  `:Block` states in its documentation which R3 Blocks it can produce, and the
  caller checks the returned pointer for `nullptr` when in doubt.
- `map_back_solution()` takes the solution currently held in the `Variable`s
  of the R3 Block `R3B` and writes the corresponding solution into the
  `Variable`s of the original `Block`. Its `solc` selects which part of the
  solution to map (primal, dual, both, or a subset of the `Variable`s). If it
  is `nullptr` the corresponding field of the `BlockConfig` is used, and all
  the solution is mapped if that is not set either.
- `map_forward_solution()` does the reverse, from the original to the R3
  Block.
- `map_forward_Modification()` takes a `Modification` that occurred on the
  original `Block` and applies the corresponding change to the R3 Block, so
  that the two stay in sync. It returns `true` if the `Modification` has been
  mapped and `false` if it has not (an unsupported `Modification` is not an
  error); hence, one may hand it each `Modification` without first checking
  which ones are supported. `issuePMod` and `issueAMod` say how the physical
  and the abstract `Modification`s that the change of the R3 Block produces
  are issued ([§8.5](08-modification-janus.md#sec-8-5)). A `:Block` should map
  either its physical or its abstract `Modification`s, never both, since a
  change of a `Block` with both representations puts both in flight ([Chapter
  8](08-modification-janus.md#ch-8)). Mapping some `Modification`s may only be
  possible if it is done immediately after the original `Block` has changed:
  e.g., a dynamic `Constraint` that is changed and then removed leaves the
  original with no information on the change that a later mapping would need.
  A `:Block` states in its documentation which `Modification`s, if any, it
  can map only immediately
  ([`Block.h:5458-5471`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L5458-5471)).
- `map_back_Modification()` is the reverse direction, from the R3 Block to the
  original.
- `map_forward_Modifications()` and `map_back_Modifications()` take a list
  instead of one `Modification`, which allows a `:Block` to map the net effect
  of many changes instead of each of them in turn. The list is passed by
  reference: the `Modification`s that have been mapped are removed from it,
  and those that have not are left there. The default implementation calls the
  method for a single `Modification` once on each element of the list, from
  the first to the last, and removes from the list of the caller those for
  which it returns `true`.

The base class gives each of these methods a default. `get_R3_Block()` returns
`base` if the `Block` has no sub-`Block`. Otherwise it requires a non-null
`base`, and throws `std::invalid_argument` if there is none, since `Block` is
abstract and the method cannot allocate the right concrete type. It then
empties the sub-`Block`s of `base` and fills them with the R3 Blocks of the
sub-`Block`s of `this`, each produced with `father = base` and, if `r3bc` is a
`SimpleConfiguration< std::vector< Configuration * > >`, with the $i$-th
element of the vector as its `r3bc` (`nullptr` otherwise, or if the vector
has no $i$-th element, which by convention selects the copy;
[`Block.h:5187-5215`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L5187-5215)).
The default `map_back_solution()` and `map_forward_solution()` serve the
mirror of [§10.4](10-r3block.md#sec-10-4) and throw `std::invalid_argument`
for any other R3 Block. The default `map_forward_Modification()` serves the
mirror first; otherwise it returns `false` for a `Modification` of the `Block`
on which it is called, and hands a `Modification` of one of its *direct*
sub-`Block`s to that sub-`Block`, paired with the sub-`Block` of `R3B` in the
same position (and with the corresponding element of `r3bc`, as above), while
it returns `false` for a `Modification` of a deeper descendant. The default
`map_back_Modification()` does the same, without the mirror. Hence a leaf
`:Block` that wants any R3 Block other than the mirror has to implement it in
full, the copy included.

In all cases the `Configuration` passed to a mapping method must be
the *same* (or an identical) `Configuration` that was used to produce
`R3B` in `get_R3_Block()`. In fact, this is how the `Block` knows which kind
of R3 Block it is dealing with.

### The `base` and `father` parameters

The two trailing parameters of `get_R3_Block()` make the construction
of an R3 Block composable along a class hierarchy and along a `Block`
tree.

The parameter `base` lets the construction happen piecemeal. Consider a
class `B1 : Block` that already provides some R3 Block (say, the copy)
and a derived class `B2 : B1`. It is sensible for `B2` to offer the same
R3 Block, and wasteful for it to re-implement the part that `B1` already
knows how to handle. Hence, `B2::get_R3_Block()` allocates the R3 Block
(an object of class `B2`, in the case of the copy) and then calls
`B1::get_R3_Block( r3bc , that_object )`; `B1` fills the part of the new
object that it knows of, while `B2` takes care of the rest. In
other words, `base` is an existing object that the method populates
instead of allocating one. A `:Block` that accepts a `base` has requirements
on its concrete type, depending on `r3bc`: for the copy, `base` must be
of a class derived from the one whose `get_R3_Block()` is called (any
`B1`-or-derived for `B1::get_R3_Block()`, hence a `B2` qualifies).

The parameter `father` serves the converse situation, in which `base`
is not supplied and the R3 Block has to be allocated inside the method.
Then `father` is the father `Block` of the new R3 Block, passed to its
constructor, and the R3 Block is correctly nested in the surrounding
tree.

### Dynamic `Variable`s and `Constraint`s in an R3 Block

The semantics of the dynamic `Constraint`s and `Variable`s of
[§4.3](04-block.md#sec-4-3) interacts with R3 Blocks in a way that is
easy to get wrong. A dynamic `Constraint` is conceptually "there even
when it is not there": it contributes to defining the feasible region of
the `Block`, and therefore each feasible solution must satisfy all of
them, whether or not they have been constructed. Only the subset that is
useful for algorithmic reasons (say, the ones that a separation
procedure has found to be violated) is generated, since there can be
very many. Likewise, each dynamic `Variable` that has not been
generated is assumed to be in the `Block` at its default value (most
often zero), under the assumption that this does not invalidate the
feasibility of the generated part of the solution. Hence, only the
dynamic `Variable`s needed to encode an optimal solution need be
constructed.

An R3 Block may therefore inherit either all the dynamic `Variable`s and
`Constraint`s of the original, or only those that the original has generated
when the R3 Block is produced. The second case arises when the R3 Block works
only with the abstract representation of the original, since generating
dynamic objects may require separation or pricing procedures that are
available to the original `Block` but not necessarily to the R3 Block. If they
are available, the R3 Block may generate further dynamic objects, which the
original may import with `map_back_Modification()`
([`Block.h:5130-5164`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L5130-5164));
if they are not, the dynamic objects can only be generated by the original and
then, where supported, pushed forward to the R3 Block with
`map_forward_Modification()`. A `:Block` author who provides an R3 Block of a
model with dynamic components decides, and documents, which of the two
directions the R3 Block supports. Accordingly, `map_back_solution()` is
allowed to do a best-effort job (and the solution it writes may then be
infeasible) when the R3 Block holds dynamic objects that the original has not
generated. A full map is always possible if the two are brought in sync with
the `Modification` mappings before the call.

## 10.4 The trivial case: the copy {#sec-10-4}

The simplest R3 Block is the *copy* (clone): a new `Block` of the same type,
holding the same data, in separate `Variable`s and `Constraint`s. The copy is
a Reformulation in the trivial sense (it encodes the same problem, [Lemma
10.1](10-r3block.md#lem-10-1) holding with $\phi$ and $\psi$ the two
correspondences of the `Variable`s) and it should be offered by any reasonable
`:Block`. Even so, the copy is not free: because of the identity rule the
`Variable`s of the copy are distinct objects, and `map_back_solution()` from
the copy to the original is a value-by-value transfer, not an aliasing of
pointers.

For `MCFBlock`, `get_R3_Block( nullptr )` produces a copy, which is the only
R3 Block it supports
([`MCFBlock.h:1205-1214`](https://gitlab.com/smspp/mcfblock/-/blob/develop/include/MCFBlock.h#L1205-1214));
`map_back_solution()` and `map_forward_solution()` move the flows, the
potentials and the reduced costs between the two. Their `solc` is a
`SimpleConfiguration< int >` with value 1 for "primal only", 2 for "dual only"
and anything else for both (the same format applying to the unbounded primal
and dual rays).

### A copy for every `Block`: the mirror

A `:Block` that implements no R3 Block at all still has one copy available,
made not by the `:Block` but by an `AbstractBlock` ([Source:
[`AbstractBlock.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/AbstractBlock.h),
"Mirroring the abstract representation of another Block"]).
`AbstractBlock::mirror( Block * blck )` builds the (empty) `AbstractBlock` as
a copy of the abstract representation of `blck`: one `ColVariable` per
`ColVariable`, one `Constraint` per `Constraint`, the `Objective`, and one
inner `AbstractBlock` per sub-`Block`, recursively, the groups keeping their
shape and their order. A `Variable` living outside the mirrored subtree (one
in which a `Constraint` coupling `blck` to the rest is written) is not copied
but shared. The objects that can be copied are those of which the abstract
representation is made, i.e., `ColVariable`s, the `OneVarConstraint` family,
and the `FRowConstraint`s and the `FRealObjective` whose `Function` is a
`LinearFunction` or a `DQuadFunction` (a `QuadFunction`, which derives from
the latter, is copied as a `QuadFunction`, off-diagonal terms included).
Anything else is skipped and recorded in `get_mirror_issues()`, a vector with
one string per skipped object. A row whose `Function` cannot be copied is kept
with an empty `LinearFunction` and infinite sides, a `Constraint` group of a
type the mirror does not know is left empty, and so is a `Variable` group that
the copy cannot make, while an `Objective` that cannot be copied is left out;
a `Variable` group not made of `ColVariable`s, and a group whose shape the
copy does not reproduce, are recorded as well. The weights of the `Objective`
and of the `Variable`s of `blck` and of its sub-`Block`s are carried over. The
`AbstractBlock` must be empty, and mirroring twice is an error.

The copy knows to which object of the original each of its objects
corresponds (`mirror_of()` from the original to the copy, `mirrored_of()` from
the copy to the original, `get_mirrored()` for the mirrored `Block`).
The exact relation between the two problems is the following.

<span id="lem-10-2"></span>**Lemma 10.2.** *Let $A$ be an `AbstractBlock` on
which `mirror( blck )` has been called, and let $\sigma$ be the map
`mirror_of()` from the `ColVariable`s of the subtree of `blck` to those of
$A$, extended as the identity to the `ColVariable`s outside that subtree. (i)
If `get_mirror_issues()` is empty, a point $x$ is feasible for `blck` if and
only if $\sigma(x)$ is feasible for $A$, with the same objective value, and
hence $A$ is a Reformulation of `blck`. (ii) If each issue is a row kept with
infinite sides or a `Constraint` group left empty, $\sigma$ maps the feasible
points of `blck` into feasible points of $A$ with the same objective value,
and $A$ is a Relaxation of `blck`. (iii) If the `Objective` has been left out,
(ii) still holds for the feasible regions, but $A$ has no objective and
provides no bound.*

*Proof.* By construction, each `ColVariable` of $A$ has the type, the bounds
and the fixing of its counterpart, and each row of $A$ that is not an issue
has the `Function` of its counterpart written on $\sigma$ of its `Variable`s,
with the same sides. The same holds for the `Objective` and for the weights.
For (i), substituting $\sigma(x)$ for $x$ therefore leaves the value of each
row and of the `Objective` unchanged, and $\sigma$ is a bijection between the
two sets of `ColVariable`s (the identity outside the subtree), which gives
both directions and the equality of the objective values. [Lemma
10.1](10-r3block.md#lem-10-1) with $\phi = \sigma$ and $\psi = \sigma^{-1}$
gives the Reformulation. For (ii), the rows that are issues have infinite
sides, and the groups that are issues are empty, in $A$: hence the constraints
of $A$ are a subset of the images of those of `blck`, and each $\sigma(x)$
with $x$ feasible is feasible for $A$, with the same value. [Lemma
10.1](10-r3block.md#lem-10-1) gives $v(A) \le v(\mathtt{blck})$. For (iii),
the argument on the feasible regions does not involve the `Objective`.
$\square$

The case (iii) is the reason why a copy with issues should not be
called a relaxation without qualification: the caller should read the
issues before using the value of the copy as a bound.

The base `Block` class dispatches `map_back_solution()`,
`map_forward_solution()` and `map_forward_Modification()` to the mirror
whenever the R3 Block it is given is an `AbstractBlock` that mirrors the same
`Block`, calling `mirror_write()`, `mirror_read()` and
`mirror_forward_Modification()` respectively. Note that this
`map_back_solution()` copies the primal values only and ignores `solc`: the
dual values of the copy are written into the original by a separate call to
`mirror_write_duals()`. Hence a mirror is an R3 Block of any `Block` that does
not override these methods, or whose overrides call those of the base class
first, and an `UpdateSolver` ([§10.6](10-r3block.md#sec-10-6)) attached to the
original keeps it in sync like any other, the original having had to implement
nothing. The overrides of `MCFBlock` and of `CapacitatedFacilityLocationBlock`
do not call the base class, and they throw `std::invalid_argument` when given
a mirror. An `R3BSolver` ([§10.9](10-r3block.md#sec-10-9)), instead, can solve
through it only if the `:Block` returns the mirror from its `get_R3_Block()`,
since the base class does not. `mirror_forward_Modification()` follows the
changes of the coefficients of a `LinearFunction` or of a `DQuadFunction` and
of the `Variable`s it has, of the sides of a `RowConstraint` and of the
`Variable` of a `OneVarConstraint`, the relaxing and enforcing of a
`Constraint`, the type and the fixing of a `ColVariable`, and the sense of the
`Objective`. It also follows dynamic `Constraint`s added to or removed from a
`std::list`, as well as dynamic `ColVariable`s added to one. A dynamic
`Variable` removed, a group made of a `boost::multi_array` and an
`NBModification` cannot be followed, and `false` is returned, meaning "the
copy is stale": `mirror()` then has to be called again on a new
`AbstractBlock`. A physical `Modification` does not concern the abstract
representation, and it is ignored. The copy issues the `Modification`s of its
changes as prescribed by the `issueAMod` of `map_forward_Modification()`
(`eModBlck` by default). Hence, a `Solver` registered to the mirror is
informed of the changes forwarded by an `UpdateSolver` or by an `R3BSolver`.
Called directly, instead, `mirror_forward_Modification()` makes them with
`eNoMod` by default, which suits a copy that no `Solver` has loaded.

### A sized copy

A mirror can also be *sized*. A `:Block` may have a size parameter $k$ that
multiplies some of its data (a row $A_i x \le b_i$ becoming $A_i x \le k\,
b_i$, or a cost $c$ becoming $k\, c$). When $k$ is a column of the model,
instead of a datum, it is a `Variable`: one that the `Block` owns, declares
with `set_owned_size_variable()` and returns from `get_size_variable()`, or
one of another `Block`, normally the father, that it is given with
`set_size_variable( Variable * size_var , c_ModParam issueAMod = eNoBlck )`,
which returns `false`, with no change, if the `Block` takes no such
`Variable`, as the base class does ([Source:
[`Block.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h),
"Sizing of this Block"]). `AbstractBlock::set_size_variable( v )` writes into
a mirror the mirrored `Block` scaled by $v$. Every row $l \le a x \le u$
becomes $a x - u v \le 0$ and $a x - l v \ge 0$ (one row when $l = u$ or a
side is infinite), the finite nonzero sides of a `OneVarConstraint` become
such rows (a `OneVarConstraint` whose type does not let a side change being
moved onto a `ColVariable` of its own), the bounds $-1$ and $1$ of a unitary
`ColVariable` become rows, the `ColVariable` taking the type without them, a
`ColVariable` fixed to $c$ is unfixed and given the row $x - c v = 0$, and the
constant $c_0$ of the `Objective` becomes the term $c_0 v$, the added rows and
`ColVariable`s being in the last dynamic group of each kind. The mirrored
`Block` is not touched, and its `Modification`s are written in the copy in
this form. The method returns `false`, with no change, if the `AbstractBlock`
is not a mirror, already has (or owns, or has kept from the mirrored `Block`)
another size `Variable`, has mirror issues, or if the mirrored `Block` has a
row or an `Objective` whose `Function` is not a `LinearFunction`, or an
integer `ColVariable`. Calling it again with the same `Variable` has no
effect, and `nullptr` gives back the plain copy. Note that `mirror()` keeps
the size `Variable` of the mirrored `Block`, if any (its copy if the `Block`
owns it, the same `Variable` otherwise), and `get_given_size_variable()`
returns the size `Variable` of the copy, given or kept. Each scaled row is the
original one with its sides multiplied by $v$. Hence, for $v > 0$ a point $x$
is feasible for the original if and only if $v x$ is feasible for the sized
copy, whose feasible region is thus the perspective of the original one (on
which, e.g., a capacity-expansion master can be built).

## 10.5 The non-trivial case: the MCF flow relaxation of CFL {#sec-10-5}

A non-trivial R3 Block is the one offered by
`CapacitatedFacilityLocationBlock`: besides the copy, it can produce an
`MCFBlock` that represents the continuous flow relaxation of the CFL problem
([`CapacitatedFacilityLocationBlock.h:1112-1184`](https://gitlab.com/smspp/capacitatedfacilitylocationblock/-/blob/develop/include/CapacitatedFacilityLocationBlock.h#L1112-1184)).
With the notation of the header, there are $m$ facilities $i \in I$ with
capacity $Q_i > 0$ and fixed cost $F_i$, and $n$ customers $j \in J$ with
demand $D_j > 0$; $C_{ij}$ is the cost of serving all the demand of $j$ from
$i$, $X_{ij} \in [0, 1]$ the fraction of it that is served, and $Y_i \in \{0,
1\}$ is 1 if $i$ is open. The problem reads

$$
\begin{aligned}
\min \;& \sum_{i \in I} \sum_{j \in J} C_{ij} X_{ij} +
\sum_{i \in I} F_i Y_i \\
& \sum_{i \in I} X_{ij} = 1 && j \in J , \\
& \sum_{j \in J} D_j X_{ij} \le Q_i Y_i && i \in I , \\
& X \in [0,1]^{m \times n} ,\; Y \in \{0,1\}^m
\end{aligned}
\tag{10.1}
$$

where some $Y_i$ may be fixed to 0 or to 1. The R3 Block is selected by
`r3bc`, a `SimpleConfiguration< int >` or a
`SimpleConfiguration< std::pair< int , double > >` whose `int` is

- 0 (also assumed if `r3bc` is `nullptr`): the copy, another
  `CapacitatedFacilityLocationBlock`;
- 1: an `MCFBlock` flow relaxation;
- 2: the same flow relaxation, with extra slack arcs that keep it feasible
  for every pattern of open facilities;

and whose `double`, if present, scales the cost of the slack arcs
(100 by default); any other value throws `std::invalid_argument`. If
`base` is given it must be a `CapacitatedFacilityLocationBlock` for the
copy and an `MCFBlock` otherwise.

Call a facility *paid* if its fixed cost is paid whatever the flow, i.e.,
if it is fixed open, or if it is not fixed and $F_i < 0$ (`MCF_paid()`), and
let
$P \subseteq I$ be the set of the paid facilities. The graph has $m + n +
2$ nodes: one per facility (deficit 0), one per customer (deficit
$D_j$), a super-source (deficit $-\sum_j D_j - m$) and a "constant" node
(deficit $m$). It has at least $m(n + 2)$ arcs, i.e., (i) a facility
arc from the super-source to each facility $i$, with capacity $Q_i$ and
cost $F_i / Q_i$, or 0 if $i \in P$, and (ii) a transportation arc from
each facility $i$ to each customer $j$, with infinite capacity and cost
$C_{ij} / D_j$, the transportation cost per unit of demand. Last, there
is (iii) one arc per facility from the super-source to the constant node, with
capacity 1 and cost $F_i$ if $i \in P$ and 0 otherwise, which the
deficit of the constant node forces to carry one unit each. Hence,
these arcs carry the constant $\sum_{i \in P} F_i$ of the objective. The
facility arcs of the facilities fixed closed are closed. In variant 2,
$n$ further slack arcs go from the super-source to each customer, with
infinite capacity and a cost of the scale factor times one plus the
largest cost of a path to that customer, which absorb any demand that
the open facilities cannot serve. Their cost is not updated when the
costs of the CFL change.

That this `MCFBlock` is a Relaxation in the sense of
[§10.1](10-r3block.md#sec-10-1) is the following statement, where the
*weak continuous relaxation* of (10.1) is the problem obtained by
replacing $Y \in \{0,1\}^m$ with $Y \in [0,1]^m$ (with the same
fixings).

<span id="lem-10-3"></span>**Lemma 10.3.** *The optimal value of the flow
relaxation of variant 1 equals the optimal value of the weak continuous
relaxation of (10.1), and therefore it is a lower bound on the optimal value
of (10.1). The optimal value of variant 2 is not larger than that of variant
1, and variant 2 always has a feasible flow.*

*Proof.* Let $(X, Y)$ be feasible for the weak relaxation and define the
flow $\varphi_{ij} = D_j X_{ij}$ on the transportation arcs,
$\sum_j \varphi_{ij}$ on the facility arc of $i$, and 1 on each arc of
the constant node. Conservation at customer $j$ is
$\sum_i D_j X_{ij} = D_j$, i.e., the assignment constraint of (10.1);
conservation at facility $i$ holds by definition. The flow on the
facility arc is $\sum_j D_j X_{ij} \le Q_i Y_i \le Q_i$, and it is 0 if
$i$ is fixed closed ($Y_i = 0$); conservation at the super-source and at
the constant node follows from the others. Its cost is

$$
\sum_{i,j} \frac{C_{ij}}{D_j} D_j X_{ij} +
\sum_{i \notin P} \frac{F_i}{Q_i} \sum_j D_j X_{ij} +
\sum_{i \in P} F_i
\;\le\; \sum_{i,j} C_{ij} X_{ij} + \sum_{i \in I} F_i Y_i ,
$$

since, for $i \notin P$, either $i$ is fixed closed and both sides are 0, or
$F_i \ge 0$ and $\sum_j D_j X_{ij} \le Q_i Y_i$, while for $i \in P$ either
$Y_i = 1$ (fixed open) or $F_i < 0$ and $Y_i \le 1$, hence $F_i \le F_i Y_i$.
Conversely, let $\varphi$ be a feasible flow, and set
$X_{ij} = \varphi_{ij} / D_j$, $Y_i = 1$ for $i \in P$ and
$Y_i = \sum_j \varphi_{ij} / Q_i$ otherwise. Then $X \ge 0$ and
$\sum_i X_{ij} = 1$ by conservation at $j$, whence $X_{ij} \le 1$;
$Y_i \in [0, 1]$ by the capacity of the facility arc, a facility fixed closed
has $Y_i = 0$ and one fixed open is in $P$. The capacity constraint holds with
equality for $i \notin P$, and for $i \in P$ since
$\sum_j \varphi_{ij} \le Q_i$; and the cost of $(X, Y)$ in (10.1) equals that
of $\varphi$, term by term. The two maps satisfy the hypotheses of [Lemma
10.1](10-r3block.md#lem-10-1), which gives the equality; the weak relaxation
is a Relaxation of (10.1) by [Lemma 10.1](10-r3block.md#lem-10-1) with $\phi$
the identity. Every flow of variant 1 is a flow of variant 2 with the
slack arcs at 0 and the same cost, which gives the second inequality.
Finally, in variant 2 the flow that sends $D_j$ from the super-source to each
customer $j$ on its slack arc, 1 on each arc of the constant node and 0
elsewhere is feasible. $\square$

The proof also shows how a flow is read as a solution of the CFL, as
`map_back_solution()` does: $X_{ij} = \varphi_{ij} / D_j$, and
$Y_i = 1$ for a paid facility and $Y_i = \varphi_i / Q_i$ for the others,
$\varphi_i$ being the flow on its facility arc. The weak relaxation is
weaker than the one with the strong constraints $X_{ij} \le Y_i$ (cf.
the formulations of [§3.5](03-mental-model.md#sec-3-5)), which the flow
relaxation cannot express; it is, however, a min-cost flow instead of a
MILP, and it is usually solved faster.

This construction serves two purposes in the framework: it is the R3 Block
described here, and it is also the engine of the "Benders friendly"
formulation of CFL (BenForm, [§3.5](03-mental-model.md#sec-3-5)), where
the `MCFBlock` is wrapped in a `BendersBFunction` to produce Benders cuts
([Chapter 14](14-lag-benders-bfunction.md#ch-14),
[Recipe R5](R5-cfl-benders.md#rec-R5)). There, the facility arcs cost 0,
since the fixed costs are in the master, and the constant node is absent.

`CapacitatedFacilityLocationBlock` implements `map_back_solution()`,
`map_forward_solution()` and `map_forward_Modification()` for both the copy
and the MCF R3 Block, with `solc` a `SimpleConfiguration< int >` whose bit 0
selects $Y$ and bit 1 selects $X$ (3, all, if `solc` is `nullptr`).
`map_forward_Modification()` maps the physical `Modification`s only (the
abstract ones are intercepted and turned into physical ones, [Chapter
8](08-modification-janus.md#ch-8)), and it ignores the change between the
splittable and the unsplittable version when mapping to the MCF R3 Block,
which is the same for both. `map_back_Modification()` is implemented for the
copy only, and returns `false` for the MCF R3 Block. `map_back_solution()` and
`map_forward_solution()` read or write the abstract `Variable`s of the
`CapacitatedFacilityLocationBlock`, and they throw `std::invalid_argument` if
these have not been generated. Instead, `get_R3_Block()` and
`map_forward_Modification()` work on the data of the `:Block`, and they do not
require them.

## 10.6 Keeping an R3 Block in sync: `UpdateSolver` {#sec-10-6}

When an algorithm holds both an original `Block` and one of its R3 Blocks and
changes the original, the R3 Block has to be updated to match. Doing this by
hand (intercepting each `Modification` of the original and calling
`map_forward_Modification()`) is mechanical and error-prone, and the framework
packages it as a `:Solver`,
[`UpdateSolver`](https://smspp.gitlab.io/smspp-project/d4/d0f/class_s_m_spp__di__unipi__it_1_1_update_solver.html),
whose only job is to forward `Modification`s. Its constructor is

```cpp
UpdateSolver( Block * R3B , Configuration * r3bc = nullptr ,
              int options = 0 , ModParam issuePMod = eNoBlck ,
              ModParam issueAMod = eModBlck );
```

where `R3B` is the "other" `Block` (it throws `std::invalid_argument` if it is
`nullptr`), `r3bc` the `Configuration` that produced the R3 Block, and
`issuePMod` and `issueAMod` are passed on to the mapping methods
([`UpdateSolver.h:78-129`](https://gitlab.com/smspp/smspp/-/blob/develop/include/UpdateSolver.h#L78-129)).
The `options` are bit-wise. Bit 0 chooses the direction: with 0, the `Block`
the `UpdateSolver` is registered to is the original, and each `Modification`
is mapped forward to `R3B`; with 1, it is the R3 Block, `R3B` is the original,
and each `Modification` is mapped back to it. Bit 1 passes the `Modification`
to `R3B` unchanged instead of mapping it, bits 2 and 3 restrict the forwarding
to the `Modification`s of the `Block` or to those of its sub-`Block`s, and
bits 4 and 5 to those with `concerns_Block()` true or false. The forwarding is
immediate: `add_Modification()` applies the filters of bits 2 to 5, passes the
`Modification` unchanged if bit 1 is set, and otherwise locks `R3B` (throwing
`std::logic_error` if it cannot), calls the mapping method and unlocks it,
with no `Modification` stored; `compute()` only returns `kError`. The mapping
of some `Modification`s may in fact require this immediate forwarding
([§10.3](10-r3block.md#sec-10-3)).

<span id="lem-10-4"></span>**Lemma 10.4.** *Let an `UpdateSolver` with bits 0
to 5 of `options` equal to 0 be registered to a `Block` $B$, with `R3B` $= R$
produced by `B->get_R3_Block( r3bc )` and the same `r3bc`. Suppose that, for
each `Modification` $\mu$ issued by $B$, `map_forward_Modification( R , mu ,
r3bc )` either returns `false` or changes $R$ into the R3 Block that
`get_R3_Block( r3bc )` would produce from $B$ after the change described by
$\mu$. If all these calls return `true`, then after any sequence of changes of
$B$, made while the `UpdateSolver` is registered, with `Modification`s not
inhibited and with no channel open at $B$ or at its ancestors (nor a default
channel set with `set_default_channel()`), $R$ is the R3 Block of the current
$B$.*

*Proof.* By induction on the number $k$ of changes. For $k = 0$, $R$ is the R3
Block of $B$ by construction. Each change of $B$ issues its `Modification`s to
each registered `Solver`, and hence to the `UpdateSolver`, whose
`add_Modification()` calls `map_forward_Modification()` at once, before any
further change of $B$, since no channel gathers the `Modification`s into a
group delivered later. By hypothesis that call returns `true`, and therefore
it turns the R3 Block of $B$ before the change, which $R$ is by the inductive
hypothesis, into the R3 Block of $B$ after it. $\square$

The hypothesis is the contract that a `:Block` signs when it implements
`map_forward_Modification()`; the lemma stops holding as soon as one
call returns `false`, and `add_Modification()` ignores the returned
value, so that a `Modification` that the `:Block` cannot map is dropped
silently. Whoever relies on an `UpdateSolver` must therefore know which
`Modification`s the `:Block` maps (for the mirror, those listed in
[§10.4](10-r3block.md#sec-10-4); for CFL, those listed in its header).

The idiom is to build the original `Block`, produce its R3 Block with
`get_R3_Block()`, register on the original an `UpdateSolver` pointing at the
R3 Block, and from then on change only the original. The R3 Block follows,
which keeps a relaxation faithful to the model it relaxes while the model
changes (as in a slope-scaling or branch-and-bound loop). When the R3 Block
only serves to be solved, the `R3BSolver` of [§10.9](10-r3block.md#sec-10-9)
makes the R3 Block, its `:Solver` and the forwarding out of a configuration
file, and the CFL test ([Recipe R3](R3-cfl-three-ways.md#rec-R3)) uses it.

## 10.7 Example: a CFL flow relaxation kept in sync {#sec-10-7}

Slope scaling is a simple heuristic for the CFL that solves its flow
relaxation ([§10.5](10-r3block.md#sec-10-5)) repeatedly, changing the fixed
costs between one solve and the next. In the solution of the relaxation, $Y_i
= \varphi_i / Q_i$ may be much smaller than 1, and hence a facility may be
used while paying only a small part of $F_i$. The heuristic therefore charges
each open facility the cost $F_i / Y_i \ge F_i$, by which its current use
pays $F_i$ in full, and it solves again, hopefully obtaining a solution that
concentrates the flow on fewer facilities. Each solution, with the facilities
it uses opened, is feasible for (10.1), and it gives an upper bound.

<span id="alg-10-1"></span>**Algorithm 10.1** (slope scaling on the flow
relaxation of a CFL).

```text
 1  procedure SLOPE-SCALING(B, R, k_max)   ▷ B a CFL, R its flow relaxation
 2     F ← the fixed costs of B;  U ← +∞
 3     solve R;  ℓ ← the optimal value of R
 4     for k = 1, …, k_max do
 5        map the solution of R back into (X, Y) of B
 6        Ŷ_i ← 1 if Y_i > 0 and 0 otherwise, for i ∈ I
 7        U ← min{ U , cost of (X, Ŷ) in (10.1) with the fixed costs F }
 8        if the value of R repeats an earlier one then break
 9        F'_i ← F_i / Y_i if Y_i > 10⁻⁶ and F'_i ← F_i otherwise, for i ∈ I
10        change the fixed costs of B into F'      ▷ forwarded to R
11        solve R
12     restore the fixed costs F in B
13     return ℓ, U
```

By [Lemma 10.3](10-r3block.md#lem-10-3), $\ell$ at line 3 is a lower bound on
the optimal value of (10.1), while the values of $R$ at the later iterations
are not, since the costs have changed. At line 7, $(X, \hat Y)$ is feasible
for (10.1), since $\sum_j D_j X_{ij} \le Q_i Y_i \le Q_i \hat Y_i$, and hence
$U$ is an upper bound; the test opens only the facilities with
$Y_i > 10^{-6}$, as a tolerance, and its $U$ is therefore an upper bound up to
the demand, at most $10^{-6} Q_i$, served by a facility left closed. Line 8
stops a cycle, which the heuristic may well enter. The function
`SlopeScaling()` of the test `CFL_test`
([`test.cpp`](https://gitlab.com/smspp/tests/-/blob/develop/CapacitatedFacilityLocationBlock/test.cpp))
implements [Algorithm 10.1](10-r3block.md#alg-10-1) with an `R3BSolver`
([§10.9](10-r3block.md#sec-10-9)) on the flow relaxation, which makes lines 5
and 10 one call each. With `cfl` a `CapacitatedFacilityLocationBlock` to which
a `BlockSolverConfig` has registered, as its second `Solver`, an `R3BSolver`
configured by `R3BSCfg-MCF.txt`, the loop reads (without the bounds)

```cpp
auto r3bs = dynamic_cast< R3BSolver * >(
                      *std::next( cfl.get_registered_solvers().begin() ) );
const auto F = cfl.get_Fixed_Costs();  // a copy of the fixed costs
CapacitatedFacilityLocationBlock::CntSolution y( F.size() );

for( int k = 0 ; k < 10 ; ++k ) {
 if( r3bs->compute() != Solver::kOK )
  break;
 r3bs->get_var_solution();  // the flows, mapped back into cfl
 cfl.get_facility_solution( y.begin() );
 auto NF = F;
 for( Block::Index i = 0 ; i < NF.size() ; ++i )
  if( y[ i ] > 1e-6 )
   NF[ i ] = F[ i ] / y[ i ];
 cfl.chg_facility_costs( Block::MF_dbl_sp( NF ) );  // forwarded at once
 }
```

where `get_var_solution()` has the `MCFSolver` write the flows in the
`MCFBlock`, and then has `cfl` map them back into its `Variable`s, with the
formulae given after [Lemma 10.3](10-r3block.md#lem-10-3). Note that the fixed
costs are copied before the loop, since `get_Fixed_Costs()` returns a
reference to the data of `cfl`, which `chg_facility_costs()` overwrites. The
change of the costs issues its `Modification` (the default `eNoBlck` issues
it, since a `Solver` is registered), and the `R3BSolver` maps it at once to a
change of the costs of the arcs of those facilities, from the super-source and
into the constant node. The test and its
other arrangements are described in [Recipe R3](R3-cfl-three-ways.md#rec-R3).

## 10.8 Idioms {#sec-10-8}

*The same `Configuration` for all the calls.* The `Block` uses `r3bc` to
know which R3 Block it is dealing with, and using in
`map_back_solution()` a `Configuration` different from the one given to
`get_R3_Block()` is an error.

*An `UpdateSolver` instead of forwarding by hand.* Whenever an R3 Block must
track the changes of its original (or vice versa), an `UpdateSolver` is less
code than intercepting the `Modification`s by hand, and it gets right the
direction, the locking, and the filters on `concerns_Block()`. Its limits are
those stated after [Lemma 10.4](10-r3block.md#lem-10-4). When the R3 Block is
only there to be solved, an `R3BSolver` does the same from a configuration
file ([§10.9](10-r3block.md#sec-10-9)).

*Independent lifetimes.* An R3 Block has separate `Variable`s,
`Constraint`s and `Solver`s, and a separate lifetime: it is destroyed
explicitly (unless it was created with a `father` that owns it, or by an
`R3BSolver`, which deletes it), and a solution read off it is of no use to
the original until it has been mapped back.

*A Relaxation gives a bound.* The MCF relaxation of CFL gives, fast, a lower
bound ([Lemma 10.3](10-r3block.md#lem-10-3)) and a solution that is typically
fractional. It does not give the optimal solution of the CFL, and mapping its
solution back is the start of a rounding or branching procedure, as in
[Algorithm 10.1](10-r3block.md#alg-10-1).

## 10.9 Solving a `Block` through an R3 Block: `R3BSolver` {#sec-10-9}

The `R3BSolver` is a `Solver` of the core that does not
solve the `Block` it is attached to directly, but through one of its R3 Blocks
([`R3BSolver.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/R3BSolver.h)).
When it has both a `Block` and its configuration, i.e., at the later of
`set_Block()` and `set_ComputeConfig()`, it gets the R3 Block from the
`Block`, applies a `BlockConfig` to it, generates its abstract `Variable`s
(some sub-`Block`s may only appear then) and registers to it the `Solver`s of
a `BlockSolverConfig`, the first `Solver` registered directly to the R3 Block
being the *inner* `Solver`. From then
on, each `Modification` of the original `Block` is mapped to the R3 Block as
soon as it reaches the `R3BSolver`, as an `UpdateSolver` does, and hence it
reaches the inner `Solver` as a `Modification` of the R3 Block. Furthermore,
(i) `compute()` is that of the inner `Solver`, (ii) `get_var_solution()` has
the inner `Solver` write its solution in the R3 Block, and the original
`Block` map it back into its `Variable`s, and (iii) `get_lb()` and `get_ub()`
are those of the inner `Solver`, but only on the side on which the R3 Block
gives a valid bound for the original `Block`.

The R3 Block belongs to the `R3BSolver`, which deletes it, with all the
`Solver`s that the `BlockSolverConfig` has registered to it, when it is
detached from the `Block` or deleted. It is made again, from the current
state of the `Block`, when the `R3BSolver` is attached to another `Block` and
when a new extra `Configuration` is given. A differential `ComputeConfig` with
no extra `Configuration` keeps it, while a non-differential one (or
`nullptr`) with no extra `Configuration` deletes it and leaves the
`R3BSolver` with none.
Hence, the `Modification`s issued before the R3 Block is made are not needed.
A `Modification` that the `Block` does not map (cf. the returned value of
`map_forward_Modification()`) is ignored, as in an `UpdateSolver`, and it is
up to the `Block` to map all those that matter for the R3 Block it makes.

The `R3BSolver` is configured from file only, through the extra
`Configuration` of its `ComputeConfig`
([§11.5](11-configuration.md#sec-11-5)). This is a
`SimpleConfiguration< std::vector< Configuration * > >` with three elements:
(i) the `Configuration` that selects the R3 Block (the `r3bc` of
`get_R3_Block()`, `map_forward_Modification()` and `map_back_solution()`),
which may be `nullptr` for the R3 Block that the `:Block` makes by default
(its copy, for the `:Block`s that offer one; the base class makes none, and
the `R3BSolver` then throws `std::invalid_argument`); (ii) the `BlockConfig`
of the R3 Block, which may be `nullptr`, in which case the R3 Block is left as
the `Block` makes it; and (iii) the `BlockSolverConfig` that registers the
`Solver`s to the R3 Block, which must be there, and must register at least one
`Solver` directly to the R3 Block. Other `Solver`s may be registered as well,
e.g., to the sub-`Block`s of the R3 Block, as the inner `Solver` may need; all
of them receive the `Modification`s of the R3 Block, but only the inner one is
computed by the `R3BSolver`. Its only specific parameter is `intR3BKind`,
which declares the relation between the optimal values. It is 0 (the default)
if $v(R) = v(B)$, as for a Reformulation that keeps the objective, 1 if $R$ is
a Relaxation ($v(R) \le v(B)$ for a minimization) and 2 if $v(R) \ge v(B)$, as
for a Restriction with the same objective (any other value is refused with
`std::invalid_argument`). Note that the definitions of a
Reformulation and of a Restriction in [§10.1](10-r3block.md#sec-10-1) do not
give these relations, since, e.g., a copy without the constant of the
objective is a Reformulation.

<span id="tab-10-1"></span>**Table 10.1.** The bounds of the inner `Solver`
that the `R3BSolver` passes on, according to `intR3BKind` and to the sense of
the `Block` (−∞ or +∞ is returned in place of a bound that does not pass).

| `intR3BKind` | minimization | maximization |
|---|---|---|
| 0, Reformulation | `get_lb()`, `get_ub()` | `get_lb()`, `get_ub()` |
| 1, Relaxation | `get_lb()` | `get_ub()` |
| 2, Restriction | `get_ub()` | `get_lb()` |

The table is correct by the following statement, where we let $B$ be the
`Block`, $R$ its R3 Block, $v(\cdot)$ the optimal value and $\ell \le v(R)
\le u$ the bounds that the inner `Solver` gives for $R$.

<span id="lem-10-5"></span>**Lemma 10.5.** *Let $R$ be the R3 Block of the
current $B$, and let $v(R) = v(B)$ if `intR3BKind` is 0, $v(R) \le v(B)$ if it
is 1 and $v(R) \ge v(B)$ if it is 2, for a minimization (with the two
inequalities exchanged for a maximization). Then each bound that [Table
10.1](10-r3block.md#tab-10-1) passes on is a valid bound for $v(B)$ on the
same side.*

*Proof.* For a minimization, a Reformulation gives $\ell \le v(R) = v(B) \le
u$, a Relaxation gives $\ell \le v(R) \le v(B)$, and a Restriction gives $v(B)
\le v(R) \le u$. For a maximization, a Relaxation gives $v(B) \le v(R) \le u$
and a Restriction $\ell \le v(R) \le v(B)$. $\square$

The hypothesis that $R$ is the R3 Block of the current $B$ holds by the
argument of [Lemma 10.4](10-r3block.md#lem-10-4), with the `R3BSolver` in
place of the `UpdateSolver`, as long as the `Block` maps all the
`Modification`s it issues. The hypothesis on $v(R)$ is declared by the user
with `intR3BKind`, and it is not checked. In the same way, a solution of a
Relaxation that is mapped back need not be feasible for the `Block`, and
therefore `is_var_feasible()` returns `false` for a Relaxation, while
`compute()`, `has_var_solution()`, `new_var_solution()` and `get_var_value()`
are those of the inner `Solver`, i.e., they refer to the R3 Block.

The configuration files of the CFL test show the three elements. In
`R3BSCfg-MCF.txt` the `ComputeConfig` sets `intR3BKind 1`. Its extra
`Configuration` names `R3BCfg-MCF.txt`, a `SimpleConfiguration< int >` of
value 1 that selects the flow relaxation of [§10.5](10-r3block.md#sec-10-5),
then `MCFBCfg.txt` for the `BlockConfig` of the `MCFBlock`, and last an inline
`BlockSolverConfig` with the single `Solver` `MCFSolver<MCFSimplex>`:

```text
SimpleConfiguration<std::vector<Configuration*>>
3  # how the R3Block is made, its BlockConfig, its BlockSolverConfig
*R3BCfg-MCF.txt         # how it is made
*MCFBCfg.txt            # its BlockConfig
BlockSolverConfig  # exact type of the Configuration object
1                  # the BlockSolverConfig is a "differential" one
1                  # number of (the names of) Solver
MCFSolver<MCFSimplex>
1                  # number of ComputeConfig
*DfltCfg.txt
```

The `BlockSolverConfig` `CFLBSCfg-LP.txt` of the CFL registers a
`HiGHSMILPSolver`, which solves the continuous relaxation of the CFL, and
seven `R3BSolver`s: the first is configured by `R3BSCfg-MCF.txt`, the other
six by the files `R3BSCfg-MCF-*.txt`, each a differential override
`*R3BSCfg-MCF.txt +` that differs only in the inner `Solver` (three
`MCFSolver`s and four `MCFLemonSolver`s in all). The test solves them all, and
it compares their lower bounds with each other within the tolerance of each
`Solver`. Among the others, the files `R3BSCfg-cuts.txt`, `R3BSCfg-ld.txt` and
`R3BSCfg-ben.txt` make, in the same way, a copy of the CFL (`R3BCfg-copy.txt`)
in another formulation, solved respectively by a `:MILPSolver` with
separation, by a `LagrangianDualSolver`, and by a `:MILPSolver` that receives
the Benders cuts of the formulation.
