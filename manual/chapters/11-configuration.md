# 11. Configuration

[Source: `SMS++/include/Configuration.h`, `Block.h` (`BlockConfig`),
`RBlockConfig.h`, `BlockSolverConfig.h`,
`ThinComputeInterface.h` (`ComputeConfig`)]

## 11.1 Concept

Almost every behaviour of a `Block` and of a `Solver` that is not
fixed by the problem instance is controlled by a
[`Configuration`](https://smspp.gitlab.io/smspp-project/d2/d32/class_s_m_spp__di__unipi__it_1_1_configuration.html)
object: which parts of the abstract representation to generate and
with which options, which `:Solver`s to attach and with which
parameters, which part of a solution to keep, what tolerances to
use in the feasibility checks, and so on. A `Configuration` is a
first-class object: it can be constructed programmatically, loaded
from a text file, or `[de]serialize`-d to netCDF, and it mirrors
the tree structure of the `Block` it configures.

The chapters so far have used `Configuration` objects in passing —
to select a CFL formulation (§3.5, §7.3), to gate the construction
of `MCFBlock`'s bound constraints (§7.3), to select an R3 Block
(§10.5), to pick which part of a `Solution` to keep (§9.1). This
chapter gives the systematic treatment.

## 11.2 `SimpleConfiguration<T>`

The simplest concrete `Configuration` is the template
[`SimpleConfiguration<T>`](https://smspp.gitlab.io/smspp-project/d9/d88/class_s_m_spp__di__unipi__it_1_1_simple_configuration.html),
which wraps a single value of type `T` in a public field
`f_value`. `T` is typically `int`, `double`, a `std::pair<>`, or a
`std::vector<>` of such; nesting is allowed, so a
`SimpleConfiguration< std::vector< Configuration * > >` is itself
a perfectly good `Configuration` carrying a list of sub-configurations.

`SimpleConfiguration<T>` is the workhorse for the many places
where a `:Block` needs "one number" to gate a decision. The four
formulations of `CapacitatedFacilityLocationBlock` are selected by
a `SimpleConfiguration< int >` whose `f_value` is the bit-mask
`wf` (§3.5); the sparsity option of `MCFBlock`'s objective is a
`SimpleConfiguration< double >`; the R3 Block selector of §10.5 is
a `SimpleConfiguration< int >`. Each `:Block` documents the exact
type and meaning it expects.

Two container instantiations deserve special mention because they
recur as the way to configure a *whole `Block` tree* at once:

- `SimpleConfiguration< std::vector< Configuration * > >` — a
  *positional* list: the i-th element configures the i-th
  sub-`Block` (this is, for instance, the form `get_R3_Block`
  accepts to pass a distinct sub-configuration to each inner
  `Block`, §10.3). Its drawback is that the caller must know the
  exact position of each `Block` in the tree.
- `SimpleConfiguration< std::map< std::string , Configuration * > >`
  — a *non-positional*, by-name alternative that is far more
  convenient and is used extensively in the `tools/` and `tests/`
  directories for **meta-configuration**: "apply this
  `Block[Solver]Config` to *every* `Block` of a given type",
  keyed by the class name. It removes the need to know where in
  the tree the `Block`s of that type reside, and is typically
  sufficient, since it is rare for two sub-`Block`s of the same
  type within a given parent to require *different*
  `Block[Solver]Config`s. This by-type meta-configuration is the
  idiom to reach for when configuring a large or
  programmatically-built tree.

## 11.3 `BlockConfig` and the `[C/O/R]` family

A
[`BlockConfig`](https://smspp.gitlab.io/smspp-project/d6/d34/class_s_m_spp__di__unipi__it_1_1_block_config.html)
gathers, in one object, every `Configuration` slot that a `Block`
exposes. Its fields (`Block.h:8516-8525`) are:

- `f_static_variables_Configuration` — passed to
  `generate_abstract_variables()`;
- `f_static_constraints_Configuration` — passed to
  `generate_abstract_constraints()`;
- `f_dynamic_variables_Configuration` — passed to
  `generate_dynamic_variables()`;
- `f_dynamic_constraints_Configuration` — passed to
  `generate_dynamic_constraints()`;
- `f_objective_Configuration` — passed to `generate_objective()`;
- `f_is_feasible_Configuration` — the tolerance / options used by
  `is_feasible()`;
- `f_is_optimal_Configuration` — the tolerance / options used by
  `is_optimal()`;
- `f_solution_Configuration` — which part of the solution
  `get_Solution()` keeps;
- `f_extra_Configuration` — a `:Block`-specific catch-all (used,
  for instance, by `CapacitatedFacilityLocationBlock` in BenForm
  to configure the hidden `BendersBFunction`; §11.6).

A `BlockConfig` carries a `f_diff` flag indicating whether it is a
*differential* configuration — one that changes only the slots it
explicitly sets, leaving the others untouched — as opposed to a
complete one that resets every slot.

The nine slots above are what the *base* `BlockConfig` carries,
and they configure *one* `Block`, the one the object is applied
to. The `[C/O/R]` family of derived classes adds two further,
distinct capabilities on top of that base.

The first is recursion. The second is the configuration of the
`ComputeConfig` of the `Objective` and of individual
`Constraint`s. This second capability is worth dwelling on,
because it is easy to confuse with the named slots above. Recall
(§5.1) that `Objective` and `Constraint` both derive from
`ThinComputeInterface`: their value / satisfaction must be
`compute()`-d, and that computation may in principle need its own
parameters — i.e. a `ComputeConfig` (§11.5). This is rare, if it
happens at all, in the current `:Block` catalogue, because the
`Objective`s and `Constraint`s in use are cheap to evaluate; but
it *can* arise for a `Constraint` or `Objective` whose value is
itself the result of an expensive sub-computation, and the
framework provides for it. The named slot
`f_objective_Configuration` gates the *generation* of the
`Objective`; the `O` handler, by contrast, carries the
*`ComputeConfig`* the `Objective` will use when it is
`compute()`-d. The two are different things at different points
of the lifecycle.

Three internal handlers (`RBlockConfig.h:100-779`) implement the
three capabilities, and the concrete classes are the non-empty
combinations of the corresponding letters (always written in the
fixed order O, C, R):

- **O** — `OHandler`: carries one `ComputeConfig` for the
  `Block`'s single `Objective` (`set_Config_Objective(...)`).
- **C** — `CHandler`: carries a *list* of `ComputeConfig`s, one
  per selected `Constraint`. Because a `Block` may have many
  `Constraint`s in many groups, each entry must *address* the
  `Constraint` it configures, by a pair
  `(constraint_group_id, constraint_index)` — where
  `constraint_group_id` is either the name or the index of the
  group, and `constraint_index` is the position of the
  `Constraint` within that group (the `Block::ConstraintID`
  convention; `RBlockConfig.h:374-418`). A `C`-flavoured config
  thus says, in effect, "give *this* `ComputeConfig` to the
  `Constraint` at index `j` of group `g`".
- **R** — `RHandler`: applies the whole configuration recursively
  to the sub-`Block`s as well.

| Class | O | C | R | Adds |
|---|:-:|:-:|:-:|---|
| `OBlockConfig`   | ● |   |   | `ComputeConfig` of the `Objective`, this `Block` |
| `CBlockConfig`   |   | ● |   | `ComputeConfig`s of selected `Constraint`s, this `Block` |
| `RBlockConfig`   |   |   | ● | recursion of the base slots into sub-`Block`s |
| `OCBlockConfig`  | ● | ● |   | both `Objective` and `Constraint` `ComputeConfig`s |
| `ORBlockConfig`  | ● |   | ● | `Objective` `ComputeConfig`, recursively |
| `CRBlockConfig`  |   | ● | ● | `Constraint` `ComputeConfig`s, recursively |
| `OCRBlockConfig` | ● | ● | ● | both, recursively |

Every one of these is *also* a `BlockConfig`, so it carries the
nine named slots in addition to the handler-specific data; the
prefix only says which extra capabilities it brings.

The practical upshot: to apply one configuration to a `Block` and
all its sub-`Block`s in one operation, use the `R`-flavoured
variant; to touch only this `Block`, use the non-`R` one; to act
only on the objective, only on the constraints, or on both, pick
the `O`, `C`, or `OC` prefix accordingly.

## 11.4 `BlockSolverConfig` and `RBlockSolverConfig`

While a `BlockConfig` configures the *model*, a
[`BlockSolverConfig`](https://smspp.gitlab.io/smspp-project/d1/de4/class_s_m_spp__di__unipi__it_1_1_block_solver_config.html)
configures the *solving*: it specifies which `:Solver`(s) are
registered with a `Block` and, for each, the `ComputeConfig` (the
solver parameters, §11.5) to apply. Applying a `BlockSolverConfig`
to a `Block` registers the listed `:Solver`s; clearing it
unregisters them. This is the object that makes "switching solver
is a one-line change" literally true: the choice of `:Solver` is a
line in a text file read into a `BlockSolverConfig`.

[`RBlockSolverConfig`](https://smspp.gitlab.io/smspp-project/d8/d23/class_s_m_spp__di__unipi__it_1_1_r_block_solver_config.html)
is the recursive variant: it carries, in addition to the
`:Solver`s for the `Block` itself, a list of `BlockSolverConfig`s
to be applied to the sub-`Block`s, recursively. This is how a
whole `Block` tree gets its `:Solver`s attached in one operation —
for instance, a `LagrangianDualSolver` on the master and a
`DPBinaryKnapsackSolver` on each leaf sub-`Block` of CFL/KskForm
(Recipe R4).

## 11.5 `ComputeConfig`

A
[`ComputeConfig`](https://smspp.gitlab.io/smspp-project/da/daf/class_s_m_spp__di__unipi__it_1_1_compute_config.html)
configures any `ThinComputeInterface` — so any `Solver`, but also
any `Function` or other computing object. It bundles the integer,
double and string parameters introduced in §6.4 (`intMaxIter`,
`dblMaxTime`, `dblRelAcc`, ...) into one serialisable object,
plus an optional "extra" `Configuration` for object-specific
settings. A `BlockSolverConfig` holds one `ComputeConfig` per
`:Solver` it manages; `Solver::set_ComputeConfig(ComputeConfig*)`
applies one directly.

## 11.6 The `f_extra_Configuration` escape hatch

Most of a `Block`'s configurable behaviour fits the named slots of
§11.3. For the cases that do not, `BlockConfig` carries
`f_extra_Configuration`, a slot whose meaning is entirely
`:Block`-specific.

The instructive use is `CapacitatedFacilityLocationBlock` in
BenForm. There, the transportation sub-problem is held in a
`BendersBFunction` wrapping a hidden `MCFBlock` that is *not* a
sub-`Block` of the CFL `Block` (it has `father == nullptr`). The
recursive `[C/O/R]BlockConfig` and `RBlockSolverConfig` machinery
therefore cannot reach it. The configuration of that hidden
`BendersBFunction` and of its inner `MCFBlock` — including the
`BlockSolverConfig` that attaches a `:Solver` to the inner
`MCFBlock`, which BenForm requires — is passed through
`f_extra_Configuration`, in the precise format documented at
`CapacitatedFacilityLocationBlock.h:756-818`
(a `SimpleConfiguration< std::pair< Configuration*, Configuration* > >`
whose first element is the R3-Block configuration of the inner
`MCFBlock` and whose second is the `ComputeConfig` of the
`BendersBFunction`). This is exactly the kind of situation
`f_extra_Configuration` exists for: a configurable component that
is logically part of the `Block` but structurally outside its
sub-`Block` tree.

## 11.7 Loading and serialising

A `Configuration` (of any of the above kinds) can be:

- constructed programmatically, by setting its fields directly;
- loaded from a text file with `Configuration::load(std::istream&)`
  — the form used by the configuration files shipped in the
  `tests/` directories (the `BPar*.txt`, `BSPar*.txt` files of the
  CFL tests, Recipe R3 / R4 / R5);
- `[de]serialize`-d to / from a netCDF group, like every other
  first-class SMS++ object.

The factory `Configuration::new_Configuration(...)` reconstructs
the right concrete `:Configuration` from a class name, which is
what lets a text or netCDF file specify, say, an
`RBlockSolverConfig` without the reading code knowing the type at
compile time (Chapter 18).

## 11.8 Inline example: selecting a CFL formulation and a solver

```cpp
#include "CapacitatedFacilityLocationBlock.h"
#include "BlockSolverConfig.h"

using namespace SMSpp_di_unipi_it;

int main()
{
 CapacitatedFacilityLocationBlock cfl;
 cfl.load( /* ... a CFL instance ... */ );

 // configure the MODEL: select the Knapsack Formulation (wf = 1)
 // by setting f_static_variables_Configuration on a BlockConfig
 auto * bc = new BlockConfig;
 bc->f_static_variables_Configuration = new SimpleConfiguration< int >( 1 );
 cfl.set_BlockConfig( bc );   // the Block takes ownership

 // configure the SOLVING: attach a LagrangianDualSolver, reading
 // the chosen solver and its parameters from a text file
 auto * bsc = new BlockSolverConfig;
 std::ifstream cfg( "BSPar-LDS.txt" );
 bsc->load( cfg );
 bsc->apply( & cfl );         // registers (and constructs) the Solver(s)
 bsc->clear();                // empty the config but keep it for cleanup

 // ... solve, read solution ...

 // cleanup: a clear()-ed (hence empty, non-differential) config,
 // re-applied, unregisters and deletes the Solver(s) it had created
 bsc->apply( & cfl );
 delete bsc;
 return 0;
}
```

The two halves are independent: `set_BlockConfig` decides *what
model* `cfl` exposes (here, KskForm with its `BinaryKnapsackBlock`
sub-`Block`s), while the `BlockSolverConfig` decides *how it is
solved* (here, by whatever `:Solver` `BSPar-LDS.txt` names).
Changing the formulation is a one-line change to the
`SimpleConfiguration<int>` value; changing the solver is a change
to the text file. Recipe R3 shows the same `CFL_test` executable
producing three completely different solution strategies purely by
swapping the configuration files.

## 11.9 Idioms

**Separate model configuration from solver configuration.**
`BlockConfig` answers "what model"; `BlockSolverConfig` answers
"how solved". Keep them in distinct objects (and distinct files);
it is what makes the formulation and the solver independently
swappable.

**Reach for the `R` variant to configure a tree.** When the same
choice should propagate to sub-`Block`s, use `RBlockConfig` /
`RBlockSolverConfig` rather than walking the tree by hand.

**Use `f_extra_Configuration` only for what the named slots cannot
express.** It is the documented escape hatch for components that
are logically part of the `Block` but structurally outside its
sub-`Block` tree (the BenForm hidden `BendersBFunction` being the
canonical case); it should not be used to smuggle in configuration
that a named slot already covers.

**Let configuration come from files.** The framework's intended
workflow is that formulations and solver stacks live in text or
netCDF configuration files, read at runtime, not hard-coded. The
`tests/` directories are the reference examples of this style.
