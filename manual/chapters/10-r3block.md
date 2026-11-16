# 10. R3Block: Reformulation, Relaxation, Restriction {#ch-10}

[Source: [`SMS++/include/Block.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h) (R3 Block methods),
[`AbstractBlock.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/AbstractBlock.h), [`UpdateSolver.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/UpdateSolver.h);
[`CapacitatedFacilityLocationBlock/include/CapacitatedFacilityLocationBlock.h`](https://gitlab.com/smspp/capacitatedfacilitylocationblock/-/blob/develop/include/CapacitatedFacilityLocationBlock.h)]

## 10.1 Concept {#sec-10-1}

A model may have to be *transformed* for algorithmic reasons, and SMS++
supports this by letting a `Block` produce a different `Block` that is
related to it in one of three ways ([Source: [`Block.h:67-85`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L67-85)]). Let the
original `Block` encode the problem

$$
(P) \qquad v(P) = \min \{\, c(x) : x \in X \,\}
$$

(a maximization is treated with the obvious changes of sign), and let
the new `Block` encode $(R)\; v(R) = \min \{\, c'(z) : z \in Z \,\}$ in
its own `Variable`s $z$. Then $(R)$ is a *Reformulation* of $(P)$ if
its optimal solutions can be mapped to optimal solutions of $(P)$
(the two are equivalent, but one may be easier to work with), and a
*Relaxation* if $v(R) \le v(P)$ holds for every instance of the data,
while $(R)$ is hopefully easier to solve. It is a *Restriction* if
$Z$ maps into a strict subset of $X$, which again may make $(R)$ easier
to solve, at the price of possibly cutting off the optimal solutions of
$(P)$. The three are collectively called the *R3 Blocks* of the
original `Block`.

The usual way of proving that $(R)$ is a Relaxation, which is the one
used in [§10.4](10-r3block.md#sec-10-4) and
[§10.5](10-r3block.md#sec-10-5), is the following classical argument,
stated here once for the whole chapter.

**Lemma 10.1.** *Let $\phi : X \to Z$ be a map such that
$c'(\phi(x)) \le c(x)$ for all $x \in X$. Then $v(R) \le v(P)$. If,
furthermore, $\psi : Z \to X$ satisfies $c(\psi(z)) \le c'(z)$ for all
$z \in Z$, then $v(R) = v(P)$, and $\psi$ maps every optimal solution of
$(R)$ to an optimal solution of $(P)$.*

*Proof.* For every $x \in X$ one has $v(R) \le c'(\phi(x)) \le c(x)$, and
taking the infimum over $X$ gives $v(R) \le v(P)$ (the inequality being
trivially true if $X = \emptyset$, i.e., $v(P) = +\infty$). Symmetrically,
the second map gives $v(P) \le v(R)$, hence equality. If $z^*$ is optimal
for $(R)$, then $\psi(z^*) \in X$ and
$c(\psi(z^*)) \le c'(z^*) = v(R) = v(P)$, hence $\psi(z^*)$ is optimal
for $(P)$. $\square$

In words, a Relaxation needs a map from the original to the new
`Variable`s that does not increase the cost, and a Reformulation needs a
second map back with the same property; the map back is what
`map_back_solution()` implements ([§10.3](10-r3block.md#sec-10-3)).

The set of R3 Blocks of a given `Block` is decided by the `Block`
itself, and the base `Block` class provides no general one. A concrete
`:Block` that supports an R3 Block implements it explicitly (there is
no automatic mechanism, "forget 'auto' here", as the slide decks put
it), with the one exception of the copy of the abstract representation
that an `AbstractBlock` can make of any `Block`
([§10.4](10-r3block.md#sec-10-4)).

## 10.2 Why an R3 Block, and not a copy of the `Variable`s {#sec-10-2}

A dedicated mechanism, rather than "just copy the relevant `Variable`s
and `Constraint`s", is needed because of the identity rule of
[§4.2](04-block.md#sec-4-2): the *name* of a `Variable` is its memory
address, and therefore a copy of a `Variable` is necessarily a
*different* `Variable`. An R3 Block thus lives in its own set of
`Variable`s and `Constraint`s, disjoint from those of the original. This
is what makes an R3 Block useful for algorithmic purposes (to branch,
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

virtual void map_forward_Modifications( Block * R3B , Lst_sp_Mod lmod ,
                                        Configuration * r3bc = nullptr ,
                                        ModParam issuePMod = eNoBlck ,
                                        ModParam issueAMod = eModBlck );
virtual void map_back_Modifications( Block * R3B , Lst_sp_Mod lmod ,
                                     Configuration * r3bc = nullptr ,
                                     ModParam issuePMod = eNoBlck ,
                                     ModParam issueAMod = eModBlck );
```

and a concrete `:Block` overrides the ones it chooses to support. Their
meaning is as follows.

- `get_R3_Block()` produces an R3 Block; `r3bc` selects *which* one (a
  `:Block` may offer several), and `base` and `father` are discussed
  below. A `:Block` states in its documentation which R3 Blocks it can
  produce, and the caller checks the returned pointer for `nullptr`
  when in doubt.
- `map_back_solution()` takes the solution currently held in the
  `Variable`s of the R3 Block `R3B` and writes the corresponding
  solution into the `Variable`s of the original `Block`. Its `solc` selects which part of the solution to map (primal, dual,
  both, or a subset of the `Variable`s), and if it is `nullptr` the
  corresponding field of the `BlockConfig` is used, all being mapped
  if that is not set either.
- `map_forward_solution()` does the reverse, from the original to the
  R3 Block.
- `map_forward_Modification()` takes a `Modification` that occurred on
  the original `Block` and applies the corresponding change to the R3
  Block, so that the two stay in sync. It returns `true` if the
  `Modification` has been mapped and `false` if it has not (an
  unsupported `Modification` is not an error), so that one may hand it
  every `Modification` without first checking which ones are supported.
  `issuePMod` and `issueAMod` say how the physical and the abstract
  `Modification`s that the change of the R3 Block produces are issued
  ([§8.5](08-modification-janus.md#sec-8-5)). A `:Block` should map
  either its physical or its abstract `Modification`s, never both,
  since every change of a `Block` with both representations puts both
  in flight ([Chapter 8](08-modification-janus.md#ch-8)).
- `map_back_Modification()` is the reverse direction, from the R3 Block
  to the original.
- `map_forward_Modifications()` and `map_back_Modifications()` take a
  whole list instead of one `Modification`, which allows a `:Block` to
  map the net effect of many changes rather than each of them in turn. The default implementation is meant to call the single-`Modification`
  method on each element of the list, from the first to the last (we
  remark that, at the time of writing, the default advances its
  iterator before using it, and it should not be relied upon).

The base class gives every method a default. `get_R3_Block()` returns `base`
if the `Block` has no sub-`Block`. Otherwise it requires a non-null `base`,
and throws `std::invalid_argument` if there is none, since `Block` is abstract
and the method cannot allocate the right concrete type; it then empties the
sub-`Block`s of `base` and fills them with the R3 Blocks of the sub-`Block`s
of `this`, each produced with `father = base` and, if `r3bc` is a
`SimpleConfiguration< std::vector< Configuration * > >`, with the $i$-th
element of the vector as its own `r3bc` (`nullptr` otherwise, which by
convention asks for the copy; [`Block.h:5167-5196`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L5167-5196)). The default
`map_back_solution()` and `map_forward_solution()` serve the mirror of
[§10.4](10-r3block.md#sec-10-4) and throw `std::invalid_argument` for any
other R3 Block. The default `map_forward_Modification()` serves the mirror
first; otherwise it returns `false` for a `Modification` of the `Block`
itself, and hands a `Modification` of a sub-`Block` to the same sub-`Block`,
paired with the sub-`Block` of `R3B` in the same position (and with the
corresponding element of `r3bc`, as above). The default
`map_back_Modification()` does the same, without the mirror. Hence a leaf
`:Block` that wants any R3 Block other than the mirror has to implement it in
full, the copy included.

In every case the `Configuration` passed to a mapping method must be
the *same* (or an identical) `Configuration` that was used to produce
`R3B` in `get_R3_Block()`, which is how the `Block` knows which kind of
R3 Block it is dealing with.

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
`B1::get_R3_Block( r3bc , that_object )`, so that `B1` fills the part of
the new object that it knows of, while `B2` takes care of the rest. In
other words, `base` is an existing object that the method populates
rather than allocates. A `:Block` that accepts a `base` has requirements
on its concrete type, depending on `r3bc`: for the copy, `base` must be
of a class derived from the one whose `get_R3_Block()` is called (any
`B1`-or-derived for `B1::get_R3_Block()`, hence a `B2` qualifies).

The parameter `father` serves the converse situation, in which `base`
is not supplied and the R3 Block has to be allocated inside the method:
`father` is then the father `Block` of the new R3 Block, passed to its
constructor so that the R3 Block is correctly nested in the surrounding
tree.

### Dynamic `Variable`s and `Constraint`s in an R3 Block

The semantics of the dynamic `Constraint`s and `Variable`s of
[§4.3](04-block.md#sec-4-3) interacts with R3 Blocks in a way that is
easy to get wrong. A dynamic `Constraint` is conceptually "there even
when it is not there": it contributes to defining the feasible region of
the `Block`, and therefore every feasible solution must satisfy all of
them, whether or not they have been constructed. Only the subset that is
useful for algorithmic reasons (say, the ones that a separation
procedure has found to be violated) is generated, since there can be
very many. Likewise, every dynamic `Variable` that has not been
generated is assumed to be in the `Block` at its default value (most
often zero), under the assumption that this does not invalidate the
feasibility of the generated part of the solution; hence, only the
dynamic `Variable`s needed to encode an optimal solution need be
constructed.

An R3 Block may therefore inherit either all the dynamic `Variable`s
and `Constraint`s of the original, or only those that the original has
generated when the R3 Block is produced. The second case arises when the
R3 Block works only with the abstract representation of the original,
since generating dynamic objects may require separation or pricing
procedures that are available to the original `Block` but not
necessarily to the R3 Block. If they are available, the R3 Block may
generate dynamic objects of its own, which the original may import with
`map_back_Modification()` ([`Block.h:5110-5144`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L5110-5144)); if they are not, the
dynamic objects can only be generated by the original and then, where
supported, pushed forward to the R3 Block with
`map_forward_Modification()`. A `:Block` author who provides an R3 Block
of a model with dynamic components decides, and documents, which of the
two directions the R3 Block supports. Accordingly, `map_back_solution()`
is allowed to do a best-effort job (and the solution it writes may then
be infeasible) when the R3 Block holds dynamic objects that the original
has not generated, a full map being always possible if the two are
brought in sync with the `Modification` mappings before the call.

## 10.4 The trivial case: the copy {#sec-10-4}

The simplest R3 Block is the *copy* (clone): a new `Block` of the same
type, holding the same data, in its own `Variable`s and `Constraint`s.
The copy is a Reformulation in the trivial sense (it encodes the same
problem, Lemma 10.1 holding with $\phi$ and $\psi$ the two
correspondences of the `Variable`s) and it should be offered by any
reasonable `:Block`. Even so, the copy is not free: because of the
identity rule the `Variable`s of the copy are distinct objects, and
`map_back_solution()` from the copy to the original is a value-by-value
transfer, not an aliasing of pointers.

For `MCFBlock`, `get_R3_Block( nullptr )` produces a copy, the only R3
Block it supports ([`MCFBlock.h:1205-1214`](https://gitlab.com/smspp/mcfblock/-/blob/develop/include/MCFBlock.h#L1205-1214)); `map_back_solution()` and
`map_forward_solution()` move the flows, the potentials and the reduced
costs between the two, `solc` being a `SimpleConfiguration< int >` with
value 1 for "primal only", 2 for "dual only" and anything else for both
(the same format applying to the unbounded primal and dual rays).

### A copy for every `Block`: the mirror

A `:Block` that implements no R3 Block at all still has one copy
available, made not by the `:Block` itself but by an `AbstractBlock`
([Source: [`AbstractBlock.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/AbstractBlock.h), "Mirroring the abstract representation of
another Block"]). `AbstractBlock::mirror( Block * blck )` builds the
(empty) `AbstractBlock` as a copy of the abstract representation of
`blck`: one `ColVariable` per `ColVariable`, one `Constraint` per
`Constraint`, the `Objective`, and one inner `AbstractBlock` per
sub-`Block`, recursively, the groups keeping their shape and their
order. A `Variable` living outside the mirrored subtree (one in which a
`Constraint` coupling `blck` to the rest is written) is not copied but
shared. What can be copied is what the abstract representation is made
of, i.e., `ColVariable`s, the `OneVarConstraint` family, and the
`FRowConstraint`s and the `FRealObjective` whose `Function` is a
`LinearFunction` or a `DQuadFunction`; anything else is skipped and
recorded in `get_mirror_issues()`, a vector with one string per skipped
object. A row whose `Function` cannot be copied is kept with an empty
`LinearFunction` and infinite sides, a `Constraint` group of a type the
mirror does not know is left empty, and an `Objective` that cannot be
copied is left out. The weights of the `Objective` and of the
`Variable`s of `blck` and of its sub-`Block`s are carried over. The
`AbstractBlock` must be empty, and mirroring twice is an error.

The copy knows which object of the original each of its own corresponds
to (`mirror_of()` from the original to the copy, `mirrored_of()` from
the copy to the original, `get_mirrored()` for the mirrored `Block`).
The exact relation between the two problems is the following.

**Lemma 10.2.** *Let $A$ be an `AbstractBlock` on which
`mirror( blck )` has been called, and let $\sigma$ be the map
`mirror_of()` from the `ColVariable`s of the subtree of `blck` to those
of $A$, extended as the identity to the `ColVariable`s outside that
subtree. (i) If `get_mirror_issues()` is empty, a point $x$ is feasible
for `blck` if and only if $\sigma(x)$ is feasible for $A$, with the same
objective value, and hence $A$ is a Reformulation of `blck`. (ii) If
every issue concerns a row or a `Constraint` group, $\sigma$ maps the
feasible points of `blck` into feasible points of $A$ with the same
objective value, and $A$ is a Relaxation of `blck`. (iii) If the
`Objective` has been left out, (ii) still holds for the feasible
regions, but $A$ has no objective and provides no bound.*

*Proof.* By construction, each `ColVariable` of $A$ has the type, the
bounds and the fixing of its counterpart, and each row of $A$ that is
not an issue has the `Function` of its counterpart written on $\sigma$
of its `Variable`s, with the same sides; the same holds for the
`Objective` and for the weights. For (i), substituting $\sigma(x)$ for
$x$ therefore leaves the value of every row and of the `Objective`
unchanged, and $\sigma$ is a bijection between the two sets of
`ColVariable`s (the identity outside the subtree), which gives both
directions and the equality of the objective values; Lemma 10.1 with
$\phi = \sigma$ and $\psi = \sigma^{-1}$ gives the Reformulation. For
(ii), the rows that are issues have infinite sides, and the groups that
are issues are empty, in $A$: hence the constraints of $A$ are a subset
of the images of those of `blck`, and every $\sigma(x)$ with $x$
feasible is feasible for $A$, with the same value; Lemma 10.1 gives
$v(A) \le v(\mathtt{blck})$. For (iii), the argument on the feasible
regions does not involve the `Objective`. $\square$

The case (iii) is the reason why a copy with issues should not be
called a relaxation without qualification: the caller should read the
issues before using the value of the copy as a bound.

The base `Block` class dispatches `map_back_solution()`,
`map_forward_solution()` and `map_forward_Modification()` to the mirror
whenever the R3 Block it is given is an `AbstractBlock` that mirrors the
`Block` itself, calling `mirror_write()`, `mirror_read()` and
`mirror_forward_Modification()` respectively. Note that this
`map_back_solution()` copies the primal values only and ignores `solc`:
the dual values of the copy are written into the original by a separate
call to `mirror_write_duals()`. Hence a mirror is an R3 Block of any
`Block`, an `UpdateSolver` ([§10.6](10-r3block.md#sec-10-6)) attached to
the original keeps it in sync like any other, and the original has not
had to implement anything. `mirror_forward_Modification()` follows the
changes of the coefficients of a `LinearFunction` or of a
`DQuadFunction` and of the `Variable`s it has, of the sides of a
`RowConstraint` and of the `Variable` of a `OneVarConstraint`, the
relaxing and enforcing of a `Constraint`, the type and the fixing of a
`ColVariable`, and the sense of the `Objective`; it also follows dynamic `Constraint`s
added to or removed from a `std::list`, as well as dynamic
`ColVariable`s added to one. A dynamic `Variable` removed, a group made
of a `boost::multi_array` and an `NBModification` cannot be followed,
and `false` is returned, meaning "the copy is stale": `mirror()` then
has to be called again on a new `AbstractBlock`. A physical
`Modification` says nothing about the abstract representation, and
there is nothing to do for it. The changes of the copy are made with
`eNoMod`, which is right for a copy that no `Solver` has loaded; we
remark that the dispatch from `map_forward_Modification()` does not
pass on its `issueAMod`, and therefore a `Solver` registered to the
mirror is not told of the changes forwarded by an `UpdateSolver`.

### A sized copy

A mirror can also be *sized*. A `:Block` may have a size parameter $k$
that multiplies some of its data (a row $A_i x \le b_i$ becoming
$A_i x \le k\, b_i$, or a cost becoming $k$ times itself). When $k$ is a column of the model, rather than a datum, it is a `Variable`: one
that the `Block` owns, declares with `set_owned_size_variable()` and
returns from `get_size_variable()`, or one of another `Block`, normally
the father, that it is given with `set_size_variable( Variable *
size_var , c_ModParam issueAMod = eNoBlck )`, which returns `false`, and
changes nothing, if the `Block` takes no such `Variable`, as the base
class does ([Source: [`Block.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h), "Sizing of this Block"]).
`AbstractBlock::set_size_variable( v )` writes into a mirror the
mirrored `Block` scaled by $v$. Every row $l \le a x \le u$ becomes
$a x - u v \le 0$ and $a x - l v \ge 0$ (one row when $l = u$ or a side
is infinite), the finite nonzero sides of a `OneVarConstraint` become
such rows, a `ColVariable` fixed to $c$ is unfixed and given the row
$x - c v = 0$, and the constant $c_0$ of the `Objective` becomes the
term $c_0 v$, the added rows and `ColVariable`s being in the last
dynamic group of each kind. The mirrored `Block` is not touched, and
its `Modification`s are written in the copy in this form. The method
returns `false`, changing nothing, if the `AbstractBlock` is not a
mirror, already has another size `Variable`, has mirror issues, or if
the mirrored `Block` has a row or an `Objective` whose `Function` is not
a `LinearFunction`, or an integer `ColVariable`; calling it again with
the same `Variable` changes nothing, and `nullptr` gives back the plain
copy. `get_given_size_variable()` returns the size `Variable` of the
copy. Since every scaled row is the original one with its sides
multiplied by $v$, for $v > 0$ a point $x$ is feasible for the
original if and only if $v x$ is feasible for the sized copy (the
perspective of the original feasible region, on which, e.g., a
capacity-expansion master can be built).

## 10.5 The non-trivial case: the MCF flow relaxation of CFL {#sec-10-5}

The instructive R3 Block is the one offered by
`CapacitatedFacilityLocationBlock`: besides the copy, it can produce an
`MCFBlock` that represents the continuous flow relaxation of the CFL
problem ([`CapacitatedFacilityLocationBlock.h:1112-1184`](https://gitlab.com/smspp/capacitatedfacilitylocationblock/-/blob/develop/include/CapacitatedFacilityLocationBlock.h#L1112-1184)). With the
notation of the header, there are $m$ facilities $i \in I$ with capacity
$Q_i > 0$ and fixed cost $F_i$, and $n$ customers $j \in J$ with demand
$D_j > 0$; $C_{ij}$ is the cost of serving all the demand of $j$ from
$i$, $X_{ij} \in [0, 1]$ the fraction of it that is served, and
$Y_i \in \{0, 1\}$ says whether $i$ is open. The problem reads

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
- 2: the same flow relaxation, with extra artificial arcs that keep it
  feasible for every pattern of open facilities;

and whose `double`, if present, scales the cost of the artificial arcs
(100 by default); any other value throws `std::invalid_argument`. If
`base` is given it must be a `CapacitatedFacilityLocationBlock` for the
copy and an `MCFBlock` otherwise.

Call a facility *paid* if its fixed cost is paid whatever the flow, i.e.,
if it is fixed open or if $F_i < 0$ (`MCF_paid()`), and let
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
deficit of the constant node forces to carry one unit each, so that
these arcs carry the constant $\sum_{i \in P} F_i$ of the objective. The
facility arcs of the facilities fixed closed are closed. In variant 2,
$n$ further arcs go from the super-source to each customer, with
infinite capacity and a cost of the scale factor times one plus the
largest cost of a path to that customer, which absorb any demand that
the open facilities cannot serve; their cost is not updated when the
costs of the CFL change.

That this `MCFBlock` is a Relaxation in the sense of
[§10.1](10-r3block.md#sec-10-1) is the following statement, where the
*weak continuous relaxation* of (10.1) is the problem obtained by
replacing $Y \in \{0,1\}^m$ with $Y \in [0,1]^m$ (the fixings being
kept).

**Lemma 10.3.** *The optimal value of the flow relaxation of variant 1
equals the optimal value of the weak continuous relaxation of (10.1),
and therefore it is a lower bound on the optimal value of (10.1). The
optimal value of variant 2 is not larger than that of variant 1, and
variant 2 always has a feasible flow.*

*Proof.* Let $(X, Y)$ be feasible for the weak relaxation and define the
flow $\varphi_{ij} = D_j X_{ij}$ on the transportation arcs,
$\sum_j \varphi_{ij}$ on the facility arc of $i$, and 1 on each arc of
the constant node. Conservation at customer $j$ is
$\sum_i D_j X_{ij} = D_j$, i.e., the assignment constraint of (10.1);
conservation at facility $i$ holds by definition; the flow on the
facility arc is $\sum_j D_j X_{ij} \le Q_i Y_i \le Q_i$, and it is 0 if
$i$ is fixed closed ($Y_i = 0$); conservation at the super-source and at
the constant node follows from the others. Its cost is

$$
\sum_{i,j} \frac{C_{ij}}{D_j} D_j X_{ij} +
\sum_{i \notin P} \frac{F_i}{Q_i} \sum_j D_j X_{ij} +
\sum_{i \in P} F_i
\;\le\; \sum_{i,j} C_{ij} X_{ij} + \sum_{i \in I} F_i Y_i ,
$$

since $F_i \ge 0$ and $\sum_j D_j X_{ij} \le Q_i Y_i$ for $i \notin P$,
while for $i \in P$ either $Y_i = 1$ (fixed open) or $F_i < 0$ and
$Y_i \le 1$, hence $F_i \le F_i Y_i$. Conversely, let $\varphi$ be a
feasible flow, and set $X_{ij} = \varphi_{ij} / D_j$, $Y_i = 1$ for
$i \in P$ and $Y_i = \sum_j \varphi_{ij} / Q_i$ otherwise. Then
$X \ge 0$ and $\sum_i X_{ij} = 1$ by conservation at $j$, whence
$X_{ij} \le 1$; $Y_i \in [0, 1]$ by the capacity of the facility arc, a
facility fixed closed has $Y_i = 0$ and one fixed open is in $P$; the
capacity constraint holds with equality for $i \notin P$, and for
$i \in P$ since $\sum_j \varphi_{ij} \le Q_i$; and the cost of $(X, Y)$
in (10.1) equals that of $\varphi$, term by term. The two maps satisfy
the hypotheses of Lemma 10.1, which gives the equality; the weak
relaxation is a Relaxation of (10.1) by Lemma 10.1 with $\phi$ the
identity. Every flow of variant 1 is a flow of variant 2 with the
artificial arcs at 0 and the same cost, which gives the second inequality. Finally, in variant 2 the flow that sends $D_j$ from the
super-source to each customer $j$ on its artificial arc, 1 on each arc
of the constant node and 0 elsewhere is feasible. $\square$

The proof also says how a flow is read as a solution of the CFL, which
is what `map_back_solution()` does: $X_{ij} = \varphi_{ij} / D_j$, and
$Y_i = 1$ for a paid facility and $Y_i = \varphi_i / Q_i$ for the others,
$\varphi_i$ being the flow on its facility arc. The weak relaxation is
weaker than the one with the strong constraints $X_{ij} \le Y_i$ (cf.
the formulations of [§3.5](03-mental-model.md#sec-3-5)), which the flow
relaxation cannot express; it is, however, a min-cost flow rather than a
MILP, and it is solved much faster.

This construction does double duty in the framework. It is the R3 Block
described here, and it is also the engine of the "Benders friendly"
formulation of CFL (BenForm, [§3.5](03-mental-model.md#sec-3-5)), where
the `MCFBlock` is wrapped in a `BendersBFunction` to produce Benders cuts
([Chapter 14](14-lag-benders-bfunction.md#ch-14),
[Recipe R5](R5-cfl-benders.md#rec-R5)); there, the facility arcs cost 0,
the fixed costs being in the master, and the constant node is absent.

`CapacitatedFacilityLocationBlock` implements `map_back_solution()`,
`map_forward_solution()` and `map_forward_Modification()` for both the
copy and the MCF R3 Block, `solc` being a `SimpleConfiguration< int >`
whose bit 0 selects $Y$ and bit 1 selects $X$ (3, all, if `solc` is `nullptr`). `map_forward_Modification()` maps the physical
`Modification`s only (the abstract ones being intercepted and turned
into physical ones, [Chapter 8](08-modification-janus.md#ch-8)), and it
ignores the change between the splittable and the unsplittable version
when mapping to the MCF R3 Block, which is the same for both.
`map_back_Modification()` is implemented for the copy only, and returns
`false` for the MCF R3 Block. All the mapping methods require the
abstract representation of the `CapacitatedFacilityLocationBlock` to
have been generated, and throw otherwise.

## 10.6 Keeping an R3 Block in sync: `UpdateSolver` {#sec-10-6}

When an algorithm holds both an original `Block` and one of its R3
Blocks and changes the original, the R3 Block has to be updated to
match. Doing this by hand (intercepting every `Modification` of the
original and calling `map_forward_Modification()`) is mechanical and
error-prone, and the framework packages it as a `:Solver`,
[`UpdateSolver`](https://smspp.gitlab.io/smspp-project/d4/d0f/class_s_m_spp__di__unipi__it_1_1_update_solver.html),
whose only job is to forward `Modification`s. Its constructor is

```cpp
UpdateSolver( Block * R3B , Configuration * r3bc = nullptr ,
              int options = 0 , ModParam issuePMod = eNoBlck ,
              ModParam issueAMod = eModBlck );
```

where `R3B` is the "other" `Block` (it throws `std::invalid_argument` if
it is `nullptr`), `r3bc` the `Configuration` that produced the R3 Block,
and `issuePMod` and `issueAMod` are passed on to the mapping methods
([`UpdateSolver.h:78-129`](https://gitlab.com/smspp/smspp/-/blob/develop/include/UpdateSolver.h#L78-129)). The `options` are bit-wise. Bit 0 chooses the
direction: with 0, the `Block` the `UpdateSolver` is registered to is the
original, and every `Modification` is mapped forward to `R3B`; with 1, it
is the R3 Block, `R3B` is the original, and every `Modification` is
mapped back to it. Bit 1 passes the `Modification` to `R3B` unchanged instead
of mapping it, bits 2 and 3 restrict the forwarding to the
`Modification`s of the `Block` itself or of its sub-`Block`s, and bits 4
and 5 to those with `concerns_Block()` true or false. The forwarding is
immediate: `add_Modification()` locks `R3B` (throwing
`std::logic_error` if it cannot), calls the mapping method and unlocks
it, and nothing is stored; `compute()` does nothing and returns
`kError`.

**Lemma 10.4.** *Let an `UpdateSolver` with bits 0 to 5 of `options` equal to
0 be registered to a `Block` $B$, with `R3B` $= R$ produced by
`B->get_R3_Block( r3bc )` and the same `r3bc`. Suppose that, for every
`Modification` $\mu$ issued by $B$,
`map_forward_Modification( R , mu , r3bc )` either returns `false` or changes
$R$ into the R3 Block that `get_R3_Block( r3bc )` would produce from $B$ after
the change described by $\mu$. If every call returns `true`, then after any
sequence of changes of $B$, made while the `UpdateSolver` is registered and
with `Modification`s not inhibited, $R$ is the R3 Block of the current $B$.*

*Proof.* By induction on the number $k$ of changes. For $k = 0$, $R$ is
the R3 Block of $B$ by construction. Each change of $B$ issues its
`Modification`s to every registered `Solver`, and hence to the
`UpdateSolver`, whose `add_Modification()` calls
`map_forward_Modification()` at once, before any further change of $B$. By hypothesis that call returns `true`, and therefore it turns the R3
Block of $B$ before the change, which $R$ is by the inductive
hypothesis, into the R3 Block of $B$ after it. $\square$

The hypothesis is the contract that a `:Block` signs when it implements
`map_forward_Modification()`; the lemma stops holding as soon as one
call returns `false`, and `add_Modification()` ignores the returned
value, so that a `Modification` that the `:Block` cannot map is dropped
silently. Whoever relies on an `UpdateSolver` must therefore know which
`Modification`s the `:Block` maps (for the mirror, those listed in
[§10.4](10-r3block.md#sec-10-4); for CFL, those listed in its header).

The idiom used by the CFL test ([Recipe R3](R3-cfl-three-ways.md#rec-R3))
is to build the original `Block`, produce its R3 Block with
`get_R3_Block()`, register on the original an `UpdateSolver` pointing
at the R3 Block, and from then on change only the original. The R3 Block follows, which keeps a relaxation faithful to the model it relaxes while
the model changes (as in a slope-scaling or branch-and-bound loop).

## 10.7 Example: a CFL flow relaxation kept in sync {#sec-10-7}

The following program reads a CFL instance, produces its flow
relaxation, and runs a few iterations of slope scaling, each of which
solves the relaxation, maps its solution back, and spreads the fixed
cost of each open facility on its use.

```cpp
#include "CapacitatedFacilityLocationBlock.h"
#include "MCFBlock.h"
#include "MCFSolver.h"
#include "MCFSimplex.h"
#include "UpdateSolver.h"

#include <fstream>

using namespace SMSpp_di_unipi_it;

int main( int argc , char ** argv )
{
 CapacitatedFacilityLocationBlock cfl;
 std::ifstream in( argv[ 1 ] );  // a CFL instance in ORLib format
 cfl.load( in );
 cfl.generate_abstract_variables();  // map_back_solution() writes them

 // the MCF flow relaxation (R3 Block, "always feasible" variant)
 SimpleConfiguration< int > r3bc( 2 );
 Block * R3B = cfl.get_R3_Block( &r3bc );  // an MCFBlock
 R3B->generate_abstract_variables();
 auto mcfs = new MCFSolver< MCFSimplex >();
 R3B->register_Solver( mcfs );

 // every Modification of cfl is mapped forward to the MCFBlock
 auto upd = new UpdateSolver( R3B , &r3bc );
 cfl.register_Solver( upd );

 auto m = cfl.get_NFacilities();
 const auto F = cfl.get_Fixed_Costs();  // the original fixed costs
 CapacitatedFacilityLocationBlock::CntSolution y( m );

 for( int it = 0 ; it < 10 ; ++it ) {  // slope scaling
  if( mcfs->compute() != Solver::kOK )
   break;
  mcfs->get_var_solution();             // flows into the MCFBlock
  cfl.map_back_solution( R3B , &r3bc );  // and from there into cfl
  cfl.get_facility_solution( y.begin() );

  // the fixed cost of an open facility, spread on its actual use
  auto NF = F;
  for( decltype( m ) i = 0 ; i < m ; ++i )
   if( y[ i ] > 1e-6 )
    NF[ i ] = F[ i ] / y[ i ];

  // forwarded to the MCFBlock by upd, through map_forward_Modification()
  cfl.chg_facility_costs( Block::MF_dbl_sp( NF ) );
  }

 cfl.unregister_Solver( upd , true );
 R3B->unregister_Solver( mcfs , true );
 delete R3B;
 return( 0 );
 }
```

`get_R3_Block()` returns an independent `MCFBlock`, whose `Variable`s
are not those of `cfl`; `get_var_solution()` writes the flows found by
the `MCFSolver` into the `Variable`s of the `MCFBlock`, and
`map_back_solution()` reads them from there into `cfl`, with the
formulae given after Lemma 10.3. The `UpdateSolver` removes the need to
forward each change of cost: `chg_facility_costs()` issues its
`Modification` (the default `eNoBlck` issues it, since a `Solver` is
registered), and the `UpdateSolver` maps it to a change of the costs of
the facility arcs of the `MCFBlock`. The fixed costs are copied before
the loop, since `get_Fixed_Costs()` returns a reference to the data of
`cfl`, which `chg_facility_costs()` overwrites.
[Recipe R3](R3-cfl-three-ways.md#rec-R3) develops this into a complete
program and compares it with a `CapacitatedFacilityLocationBlock` R3
Block solved by a `:MILPSolver` or by a `LagrangianDualSolver`.

## 10.8 Idioms {#sec-10-8}

*The same `Configuration` for every call.* The `Block` uses `r3bc` to
know which R3 Block it is dealing with, and using in
`map_back_solution()` a `Configuration` different from the one given to
`get_R3_Block()` is an error.

*An `UpdateSolver` rather than forwarding by hand.* Whenever an R3
Block must track the changes of its original (or vice versa), an
`UpdateSolver` is less code than intercepting the `Modification`s by
hand, and it gets right the direction, the locking, and the filters on
`concerns_Block()`; its limits are those stated after Lemma 10.4.

*Independent lifetimes.* An R3 Block has its own `Variable`s,
`Constraint`s and `Solver`s, and its own lifetime: it is destroyed
explicitly (unless it was created with a `father` that owns it), and a
solution read off it means nothing to the original until it has been
mapped back.

*A Relaxation gives a bound.* The MCF relaxation of CFL gives, fast, a
lower bound (Lemma 10.3) and a solution that is typically fractional; it
does not give the optimal solution of the CFL, and mapping its solution
back is the start of a rounding or branching procedure, not the end of
one.
