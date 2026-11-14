# 11. Configuration {#ch-11}

[Source: `SMS++/include/Configuration.h`, `Block.h` (`BlockConfig`),
`RBlockConfig.h`, `BlockSolverConfig.h`,
`ThinComputeInterface.h` (`ComputeConfig`)]

## 11.1 Concept {#sec-11-1}

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
to select a CFL formulation ([§3.5](03-mental-model.md#sec-3-5), [§7.3](07-physical-abstract.md#sec-7-3)), to gate the construction
of `MCFBlock`'s bound constraints ([§7.3](07-physical-abstract.md#sec-7-3)), to select an R3 Block
([§10.5](10-r3block.md#sec-10-5)), to pick which part of a `Solution` to keep ([§9.1](09-solution.md#sec-9-1)). This
chapter gives the systematic treatment.

## 11.2 `SimpleConfiguration<T>` {#sec-11-2}

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
`wf` ([§3.5](03-mental-model.md#sec-3-5)); the sparsity option of `MCFBlock`'s objective is a
`SimpleConfiguration< double >`; the R3 Block selector of [§10.5](10-r3block.md#sec-10-5) is
a `SimpleConfiguration< int >`. Each `:Block` documents the exact
type and meaning it expects.

Two container instantiations deserve special mention because they
recur as the way to configure a *whole `Block` tree* at once:

- `SimpleConfiguration< std::vector< Configuration * > >` — a
  *positional* list: the i-th element configures the i-th
  sub-`Block` (this is, for instance, the form `get_R3_Block`
  accepts to pass a distinct sub-configuration to each inner
  `Block`, [§10.3](10-r3block.md#sec-10-3)). Its drawback is that the caller must know the
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
  programmatically-built tree. Two `Configuration`s of the core
  carry such a map: `MetaBlockConfig` (`RBlockConfig.h`) and
  `MetaBlockSolverConfig` (`BlockSolverConfig.h`) are a
  `BlockConfig` and a `BlockSolverConfig`, respectively, for the
  `Block` they are applied to, plus the map saying which
  `BlockConfig` (`BlockSolverConfig`) is applied to each of its
  descendants according to their `classname()`, the special key
  `"*"` acting as the default for the classnames that match no
  other key. The descendants are visited father-first, so that a
  `BlockConfig` changing the structure of a `Block`
  ([§11.3](11-configuration.md#sec-11-3)) is applied before its new sub-`Block`s are looked up.

## 11.3 `BlockConfig` and the `[C/O/R]` family {#sec-11-3}

A
[`BlockConfig`](https://smspp.gitlab.io/smspp-project/d6/d34/class_s_m_spp__di__unipi__it_1_1_block_config.html)
gathers, in one object, every `Configuration` slot that a `Block`
exposes. Its fields (`Block.h:9836-9863`) are:

- `f_structure_Configuration` — passed to `set_structure()`, which
  chooses the tree of sub-`Block`s of the `Block` when this is a
  modelling choice rather than a datum of the instance; it comes
  first, since the structure comes before any part of the abstract
  representation, and `set_BlockConfig()` calls `set_structure()`
  as soon as it has digested the `BlockConfig`;
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
  to configure the hidden `BendersBFunction`; [§11.6](11-configuration.md#sec-11-6)).

A `BlockConfig` carries a `f_diff` flag indicating whether it is a
*differential* configuration — one that changes only the slots it
explicitly sets, leaving the others untouched — as opposed to a
complete one that resets every slot.

The ten slots above are what the *base* `BlockConfig` carries,
and they configure *one* `Block`, the one the object is applied
to. The `[C/O/R]` family of derived classes adds two further,
distinct capabilities on top of that base.

The first is recursion. The second is the configuration of the
`ComputeConfig` of the `Objective` and of individual
`Constraint`s. This second capability is worth dwelling on,
because it is easy to confuse with the named slots above. Recall
([§5.1](05-variable-constraint-objective.md#sec-5-1)) that `Objective` and `Constraint` both derive from
`ThinComputeInterface`: their value / satisfaction must be
`compute()`-d, and that computation may in principle need its own
parameters — i.e. a `ComputeConfig` ([§11.5](11-configuration.md#sec-11-5)). This is rare, if it
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

Three internal handlers (`RBlockConfig.h:100-778`) implement the
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
  convention; `RBlockConfig.h:373-393`). A `C`-flavoured config
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
ten named slots in addition to the handler-specific data; the
prefix only says which extra capabilities it brings.

The practical upshot: to apply one configuration to a `Block` and
all its sub-`Block`s in one operation, use the `R`-flavoured
variant; to touch only this `Block`, use the non-`R` one; to act
only on the objective, only on the constraints, or on both, pick
the `O`, `C`, or `OC` prefix accordingly.

## 11.4 `BlockSolverConfig` and `RBlockSolverConfig` {#sec-11-4}

While a `BlockConfig` configures the *model*, a
[`BlockSolverConfig`](https://smspp.gitlab.io/smspp-project/d1/de4/class_s_m_spp__di__unipi__it_1_1_block_solver_config.html)
configures the *solving*: it specifies which `:Solver`(s) are
registered with a `Block` and, for each, the `ComputeConfig` (the
solver parameters, [§11.5](11-configuration.md#sec-11-5)) to apply. How it is applied depends on
its mode: in *setting* mode (`eSetMode`) the `:Solver`s already
registered are replaced by the listed ones, in *differential* mode
(`eDiffMode`) a `:Solver` whose name is empty is kept and a
`nullptr` `ComputeConfig` leaves its parameters as they are, and in
*additive* mode (`eAddMode`) the listed `:Solver`s are registered in
addition to the existing ones, which are never touched. A
`BlockSolverConfig` remembers, per `Block`, which `:Solver`s it has
created and registered (`get_Solvers(block)`); `clear()` empties it
and puts it in setting mode, and applying the cleared object
unregisters and deletes all and only the `:Solver`s that this very
object had registered to that `Block`, leaving those put there by
anybody else alone. This is the object that makes "switching solver
is a one-line change" literally true: the choice of `:Solver` is a
line in a text file read into a `BlockSolverConfig`.

[`RBlockSolverConfig`](https://smspp.gitlab.io/smspp-project/d8/d23/class_s_m_spp__di__unipi__it_1_1_r_block_solver_config.html)
is the recursive variant: it carries, in addition to the
`:Solver`s for the `Block` itself, a list of `BlockSolverConfig`s
to be applied to the sub-`Block`s, recursively. This is how a
whole `Block` tree gets its `:Solver`s attached in one operation —
for instance, a `LagrangianDualSolver` on the master and a
`DPBinaryKnapsackSolver` on each leaf sub-`Block` of CFL/KskForm
([Recipe R4](R4-cfl-lagrangian.md#rec-R4)).

## 11.5 `ComputeConfig` {#sec-11-5}

A
[`ComputeConfig`](https://smspp.gitlab.io/smspp-project/da/daf/class_s_m_spp__di__unipi__it_1_1_compute_config.html)
configures any `ThinComputeInterface` — so any `Solver`, but also
any `Function` or other computing object. It bundles the integer,
double and string parameters introduced in [§6.4](06-solver.md#sec-6-4) (`intMaxIter`,
`dblMaxTime`, `dblRelAcc`, ...) into one serialisable object,
plus an optional "extra" `Configuration` for object-specific
settings. Like a `BlockConfig`, it is either *differential*
(`diff()`: the parameters it does not mention are left as they
are) or not (they are reset to their default value), and it can be
*relaxed* (`relax()`: the names of parameters that the object does
not know are ignored, rather than raising an exception). A
`BlockSolverConfig` holds one `ComputeConfig` per
`:Solver` it manages; `set_ComputeConfig(const ComputeConfig*)`
applies one directly.

## 11.6 The `f_extra_Configuration` escape hatch {#sec-11-6}

Most of a `Block`'s configurable behaviour fits the named slots of
[§11.3](11-configuration.md#sec-11-3). For the cases that do not, `BlockConfig` carries
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
`CapacitatedFacilityLocationBlock.h:782-843`
(a `SimpleConfiguration< std::pair< Configuration*, Configuration* > >`
whose first element is the R3-Block configuration of the inner
`MCFBlock` and whose second is the `ComputeConfig` of the
`BendersBFunction`). This is exactly the kind of situation
`f_extra_Configuration` exists for: a configurable component that
is logically part of the `Block` but structurally outside its
sub-`Block` tree.

## 11.7 Loading and serialising {#sec-11-7}

A `Configuration` (of any of the above kinds) can be:

- constructed programmatically, by setting its fields directly;
- loaded from a text file with the static
  `Configuration::deserialize(const std::string& filename)` (or
  `Configuration::deserialize(std::istream&)`, or `operator>>` on an
  object of the right type) — the form used by the configuration
  files shipped in the `tests/` directories (the `*BCfg*.txt`
  `BlockConfig`s and the `*BSCfg*.txt`, `BSPar*.txt`
  `BlockSolverConfig`s of the CFL tests, [Recipe R3](R3-cfl-three-ways.md#rec-R3) / R4 / R5);
- `[de]serialize`-d to / from a netCDF group, like every other
  first-class SMS++ object.

The factory `Configuration::new_Configuration(...)` reconstructs
the right concrete `:Configuration` from a class name, which is
what lets a text or netCDF file specify, say, an
`RBlockSolverConfig` without the reading code knowing the type at
compile time ([Chapter 18](18-factories-netcdf.md#ch-18)).

### The text format

A text file holds a `Configuration` as its classname, as known to
the factory, followed by the body that the `load()` of that class
reads; `#` starts a comment that runs to the end of the line, and
the comments of each `load()` describe its body. Wherever a
`Configuration` is expected — a slot of a `BlockConfig`, a
`ComputeConfig` of a `BlockSolverConfig`, the "extra"
`Configuration` of a `ComputeConfig`, an entry of a
meta-configuration — three forms are accepted
(`Configuration::deserialize(std::istream&)`):

- the classname and the body, written in place;
- `*` followed by whitespace, which means `nullptr` (in a
  differential object, "leave this one as it is");
- `*file.txt`, with no whitespace after the `*`, which reads the
  `Configuration` out of that file; a name not ending in `.txt` is
  taken as an SMS++ netCDF file, possibly followed by `[idx]` to
  select the idx-th `Configuration` in it, and a relative name is
  taken relative to the prefix set by
  `Configuration::set_filename_prefix()`, if any.

The third form has a differential variant, `*file.txt +`: the
`Configuration` is read out of the file and the body that follows
the `+` is merged on top of it (`merge_overrides()`), so that a
file can share a common fragment and only restate what differs.
The base class refuses it; among the `Configuration`s of the core
only `ComputeConfig` supports it, each parameter of the body
replacing the one of the same name, or being added, and the others
being kept. For instance, the second `ComputeConfig` of a
`BlockSolverConfig` that takes all the parameters of
`BX-baseCfg.txt` but one reads

```
*BX-baseCfg.txt +
1                  # differential: only what follows is changed
1                  # number of integer parameters
intSolveMethod 3
0 0 0 0 0          # no double, string, vector parameters
```

where the "extra" `Configuration` of the base file is kept since
the trailing slot is omitted; if present, it replaces that of the
base file (and `*` there makes it `nullptr`). Two more
details of the bodies are worth knowing: the body of a
`BlockConfig` starts with the differential flag and the version of
the format (currently `2`), followed by the ten slots in the order
of [§11.3](11-configuration.md#sec-11-3), and a file of the earlier format, without the version,
is refused with a message; a `double` parameter of a
`ComputeConfig` can be written `inf`, `+inf`, `-inf` or `infinity`
(in any case), and is then read as an infinite value.

## 11.8 Inline example: selecting a CFL formulation and a solver {#sec-11-8}

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
 auto * bsc = dynamic_cast< BlockSolverConfig * >(
                     Configuration::deserialize( "BSPar-LDS.txt" ) );
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
to the text file. [Recipe R3](R3-cfl-three-ways.md#rec-R3) shows the same `CFL_test` executable
producing three completely different solution strategies purely by
swapping the configuration files.

## 11.9 Idioms {#sec-11-9}

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
