# Recipe R2: reoptimizing a Binary Knapsack {#rec-R2}

[Source: [`tests/BinaryKnapsackBlock/test.cpp`](https://gitlab.com/smspp/tests/-/blob/develop/BinaryKnapsackBlock/test.cpp),
[`BinaryKnapsackBlock/include/BinaryKnapsackBlock.h`](https://gitlab.com/smspp/binaryknapsackblock/-/blob/develop/include/BinaryKnapsackBlock.h),
[`DPBinaryKnapsackSolver.h`](https://gitlab.com/smspp/binaryknapsackblock/-/blob/develop/include/DPBinaryKnapsackSolver.h)]

We are concerned in this recipe with reoptimization at its smallest scale: a
Binary Knapsack is solved by the specialized `DPBinaryKnapsackSolver`, then
its data are changed *incrementally* and it is solved again, the `:Solver`
restarting its dynamic program from a stored intermediate state rather than
from the beginning. This is the smallest illustration of the
reoptimization theme of SMS++ (Chapters [8](08-modification-janus.md#ch-8)
and [13](13-function-family.md#ch-13)). The tester of `BinaryKnapsackBlock`
exercises the same `chg_*()` paths, and compares the results of all the
`:Solver`s that its `BlockSolverConfig` attaches (the exact ones must agree
on the optimal value, the relaxations must bracket it). After the concepts
used, we give the code, state the dynamic program and prove that its restart
is exact, comment the code, show the expected output and discuss a few
variations.

## Concepts used

- the construction of a `Block` and `load()`
  ([Chapter 4](04-block.md#ch-4));
- a *native* specialized `Solver` ([Chapter 6](06-solver.md#ch-6));
- the `Modification` of a physical `chg_*()` change
  ([Chapter 8](08-modification-janus.md#ch-8));
- reoptimization, and what a `Solver` may reuse across a change
  ([Chapter 13](13-function-family.md#ch-13));
- (variation) the `Change` family ([Chapter 16](16-change.md#ch-16)).

## The code

```cpp
// 4 items, capacity 5; the weights must be integer for the DP Solver
BinaryKnapsackBlock bkb;
const std::vector< double > W = { 2.0 , 3.0 , 4.0 , 1.0 };  // weights
const std::vector< double > P = { 3.0 , 4.0 , 5.0 , 1.0 };  // profits
bkb.load( 4 , /* capacity = */ 5.0 , W , P );  // maximization by default
bkb.generate_objective();  // what get_objective_value() computes

Solver * solver = Solver::new_Solver( "DPBinaryKnapsackSolver" );
solver->set_par( DPBinaryKnapsackSolver::dblReopt , 1.0 );  // keep all slices
bkb.register_Solver( solver );

auto solve_and_report = [ & ]() {
 if( solver->compute() != Solver::kOK ) {
  std::cout << "not solved\n";
  return;
  }
 solver->get_var_solution();  // write x in the Block
 std::cout << "objective = " << bkb.get_objective_value() << " ; x = (";
 for( BinaryKnapsackBlock::Index i = 0 ; i < bkb.get_NItems() ; ++i )
  std::cout << ( i ? "," : "" ) << bkb.get_x( i );
 std::cout << ")\n";
 };

solve_and_report();                       // first solve
bkb.chg_profit( 8.0 , /* item = */ 2 );   // physical change: a Modification
solve_and_report();                       // reoptimization

bkb.unregister_Solver( solver , /* deleteold = */ true );
```

## The dynamic program and its restart

The `DPBinaryKnapsackSolver` first sets the items whose value is decided a
priori, i.e., those fixed by the user, those with a nonnegative weight and a
nonpositive profit, those with a nonpositive weight and a nonnegative
profit, and the integer ones heavier than the capacity that remains; it also
turns the items with negative weight and profit into items with positive
ones. It then has $N$ items of integer weights $w_i \ge 0$ and profits
$p_i$, some of them marked as preprocessed, and a residual capacity $C$,
rounded down to an integer. The *slice* $L_i$ is the vector, indexed by the
weights $j = 0, \ldots, C$, whose entry $L_i(j)$ is the largest profit of a
subset of weight exactly $j$ of the integer items $0, \ldots, i - 1$ that
are not preprocessed ($-\infty$ if there is none). Thus, $L_0(0) = 0$ and
$L_0(j) = -\infty$ for $j > 0$, and the optimal value of the integer part is
$\max \{ L_N(j) : j \le C \}$. The parameter `dblReopt`
$\rho \in [0, 1]$ decides which slices are stored: with
$k = \lfloor \rho \log_2 N \rfloor$ and the step
$s = \lfloor N / 2^k \rfloor$, those of index multiple of $s$, and $L_N$.

<span id="alg-R2-1"></span>**Algorithm R2.1** (dynamic program with stored
slices).

```text
 1  procedure DP-KNAPSACK(i₀)        ▷ i₀ = 0 at the first call
 2     k ← ⌊ρ · log₂ N⌋;  s ← ⌊N / 2^k⌋
 3     r ← ⌊i₀ / s⌋ · s
 4     L ← the stored slice L_r, truncated to the weights 0, …, C
 5     for i = r, …, N − 1 do
 6        if i is a multiple of s then store L as L_i
 7        if item i is preprocessed or continuous then continue
 8        for j = 0, …, C do
 9           L′(j) ← max{ L(j) , L(j − wᵢ) + pᵢ }   ▷ second term if j ≥ wᵢ
10        L ← L′
11     store L as L_N
12     return max{ L(j) : 0 ≤ j ≤ C }
```

Algorithm R2.1 is implemented by `compute()` in
[`DPBinaryKnapsackSolver.cpp`](https://gitlab.com/smspp/binaryknapsackblock/-/blob/develop/src/DPBinaryKnapsackSolver.cpp),
with `process_outstanding_Modification()` (lines 2-3), `preprocessing()` (line
4) and `dynamic_programming()` (lines 5-11), which also record at line 9 which
term gives the maximum, so as to rebuild the optimal solution backwards from
$L_N$. They then add to the value the profit of the items preprocessed at 1,
and fill the continuous items, if any, by a greedy algorithm on the capacity
left by each entry of $L_N$. The index $i_0$ is the first item whose data the
pending `Modification`s change. It is set to 0 by a larger capacity, a smaller
weight (given by range; given by subset, only a smaller negative weight), an
item no longer fixed, a change of the integrality and a change of the sense of
the objective. It is set to $N$ by a decrease of the capacity with no other
change, after which the recursion restarts from the stored slice of index
$\lfloor N / s \rfloor s$ (which is $L_N$ itself when $s$ divides $N$, as with
`dblReopt` 0 or 1). The restart at line 3 is exact whenever the residual
capacity does not grow, as we now show.

<span id="lem-R2-1"></span>**Lemma R2.1.** *Let a change of the data leave
unchanged the weight, the profit, the fixing and the integrality of the items
$0, \ldots, i_0 - 1$ and the sense of the objective, and let the residual
capacity go from $C$ to $C' \le C$. Then, for every $i \le i_0$, the slice
$L'_i$ of the changed problem coincides with the slice $L_i$ of the previous
one on the weights $0, \ldots, C'$. Hence, [Algorithm
R2.1](R2-knapsack-reopt.md#alg-R2-1) started from a stored slice of index $r
\le i_0$ ends with the same slice $L'_N$, and the same optimal value, as when
started from $r = 0$.*

*Proof.* By induction on $i$, the case $i = 0$ is trivial. Let
$L'_i(j) = L_i(j)$ for $j \le C'$, and $i < i_0$. Whether item $i$ is
preprocessed depends on its own weight, profit, fixing and integrality, which
are unchanged, and on the residual capacity only through the test $w_i > C$.
If item $i$ is processed in both problems, then by line 9 the entry
$L'_{i+1}(j)$ with $j \le C'$ depends only on $w_i$, $p_i$ and the entries of
$L'_i$ at $j$ and $j - w_i$, both at most $C'$, and therefore it equals
$L_{i+1}(j)$. If it is preprocessed in both, both slices are copied. If it is
processed in the previous problem and preprocessed in the changed one, i.e.,
$C' < w_i \le C$, then the second term of line 9 concerns only the weights $j
\ge w_i > C'$; hence, $L_{i+1} = L_i$ on the weights up to $C'$, as is
$L'_{i+1} = L'_i$. The converse case would require $w_i \le C' \le C < w_i$.
For the second part, the iterations from $r$ to $N$ depend only on the data
and on the entries of $L'_r$ at the weights up to $C'$, which by the first
part are those of the stored $L_r$ truncated at line 4. $\square$

The same argument applies to the implementation, which at line 8 also skips
the weights whose entry is not larger than that of a smaller weight (such an
entry cannot lead to a better solution); indeed, the entry of $L'$ at $j$
still depends only on the entries of $L$ at the weights up to $j$. Note that
[Lemma R2.1](R2-knapsack-reopt.md#lem-R2-1) does not necessarily hold if the
residual capacity grows, since the items that were preprocessed as too heavy
may then enter the slices before $i_0$; this is why the changes listed above
set $i_0 = 0$.

## Walk-through

- `load( 4 , 5.0 , W , P )` sets the physical data of the knapsack,
  all the items being binary since the integrality vector is omitted;
  the default sense is maximization (`set_objective_sense( false )`
  would switch it to minimization). The `DPBinaryKnapsackSolver`
  requires the *weights* to be integer (they are), while the capacity
  and the profits may be real.
- `DPBinaryKnapsackSolver`, created through the factory
  ([§6.9](06-solver.md#sec-6-9)), is a *native* specialized `:Solver`
  ([Chapter 6](06-solver.md#ch-6)), i.e., [Algorithm
  R2.1](R2-knapsack-reopt.md#alg-R2-1) with no external dependency. It reads
  the physical representation, and therefore, as in [Recipe
  R1](R1-mcf.md#rec-R1), it needs no abstract representation to solve. The
  solution that `get_var_solution()` writes goes into the `ColVariable`s of
  the `Block`, which `load()` constructs. Instead, `get_objective_value()`
  computes the `Objective` of the `Block`, which has to be generated: without
  `generate_objective()` it returns $+\infty$, while `get_x( i )` works in any
  case.
- `chg_profit( 8.0 , 2 )` changes the profit of item 2 in the physical
  representation and issues a `BinaryKnapsackBlockRngdMod` on the range of
  that item (cf. [Chapter 8](08-modification-janus.md#ch-8)), which the
  registered `:Solver` receives in its list of pending `Modification`s.
- The second `compute()` processes that `Modification`, which gives $i_0 = 2$
  in [Algorithm R2.1](R2-knapsack-reopt.md#alg-R2-1). With `dblReopt` 1, as
  set above, $s = 1$ and the recursion restarts from the slice of item 2,
  while with the default `dblReopt` 0 only $L_0$ and $L_N$ are stored, $s =
  N$, and it restarts from the first item. In either case the result is the
  same ([Lemma R2.1](R2-knapsack-reopt.md#lem-R2-1)), and only the work
  differs. The parameter is set here with `set_par()` to keep the recipe
  self-contained, while a tool or a test reads it from the `ComputeConfig` of
  a configuration file ([Chapter 11](11-configuration.md#ch-11)). At a larger
  scale, the same reuse of a previous computation lets a `BundleSolver` keep
  its bundle across changes of an inner `Block` ([Chapter
  13](13-function-family.md#ch-13), [Recipe R4](R4-cfl-lagrangian.md#rec-R4)).

## Expected output

With weights (2, 3, 4, 1), profits (3, 4, 5, 1) and capacity 5, the
feasible subsets with at least two items are {0, 1} (weight 5, profit
7), {0, 3} (weight 3, profit 4), {1, 3} (weight 4, profit 5) and
{2, 3} (weight 5, profit 6), and no single item has a profit above 5;
hence, the first optimum is {0, 1}, of value 7. After the profit of
item 2 is raised to 8, {2, 3} has profit 9 and {2} has profit 8, while
the other subsets are unchanged, and the optimum becomes {2, 3}, of
value 9:

```
objective = 7 ; x = (1,1,0,0)
objective = 9 ; x = (0,0,1,1)
```

## Variations

*Several items at once, by range or subset.*
`chg_profits( MF_dbl_sp , Range )` and
`chg_profits( MF_dbl_sp , Subset && , bool )` (and the analogous
`chg_weights()`) change a contiguous interval or an arbitrary subset of
items in one call, issuing a single
`Rngd` / `Sbst` `Modification` that the solver can react to as a
unit ([Chapter 8](08-modification-janus.md#ch-8)), and
`chg_capacity( NC )` changes the capacity of the knapsack.

*The change as a `Change`.* Since `BinaryKnapsackBlock` provides a family
of `Change`s ([Chapter 16](16-change.md#ch-16)), the same edit can be
expressed as a `BinaryKnapsackBlockRngdChange`, `apply()`-ed with
`doUndo = true` to obtain the inverse; the changed `Block` can then be
explored, and the change rolled back with the returned UndoChange
([Lemma 16.1](16-change.md#lem-16-1)). This is the right tool when the edit
must be serialized, transmitted or undone, while for an ordinary in-process
re-solve the `chg_*()` path above is simpler.

*The data changed by name, through the methods factory.*
`BinaryKnapsackBlock` registers `chg_weights`, `chg_profits` and
`chg_capacity` in the methods factory
([Chapter 15](15-methods-factory.md#ch-15)), so the same edit made above
through the concrete type can also be issued by *string name* on a base
`Block *`:

```cpp
 auto fun = Block::get_method_fs( "BinaryKnapsackBlock::chg_profits" ,
                                  Block::MS_sp_dbl_rngd::args() );
 const std::vector< double > profit = { 8.0 };
 std::invoke( * fun , & bkb , Block::MF_dbl_sp( profit ) ,
              Block::Range( 2 , 3 ) , eModBlck , eModBlck );
```

This is how type-agnostic machinery (the `DataMapping`s of a
`StochasticBlock`, a meta-configuration driver) changes the data of a
`BinaryKnapsackBlock` without a compile-time dependency on its type.
