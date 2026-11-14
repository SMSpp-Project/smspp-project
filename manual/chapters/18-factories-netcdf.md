# 18. Factories and netCDF serialisation {#ch-18}

[Source: `SMS++/include/SMSFactory.h` (factory macros),
`SMSnetCDF.h`, `SMSTypedefs.h`, `Block.h`, `Configuration.h`,
`Solution.h`, `Change.h`]

Two cross-cutting mechanisms have been referred to throughout the
manual without being treated head-on: the *class-name factories*
that let an object be created from a string, and the *netCDF
serialisation* that lets a whole `Block` tree be written to and
read from disk. This chapter covers both, and discharges the
promise of [§15.7](15-methods-factory.md#sec-15-7) about the linker.

## 18.1 The class-name factories {#sec-18-1}

Six of the framework's base classes carry a *factory*: a static
map from a class name (a `std::string`) to a constructor for that
class. The entry points are:

- `Block::new_Block(name, father)`
- `Solver::new_Solver(name)`
- `Configuration::new_Configuration(...)`
- `Solution::new_Solution(name)`
- `Change::new_Change(name)`
- `State::new_State(name)`

Each returns a freshly constructed object of the concrete type
named by the string, as a pointer to the base class. This is what
makes it possible to read "the inner `Block` is a `UCBlock`", or
"attach a `CPXMILPSolver`", or "this serialised object is a
`BinaryKnapsackBlockRngdChange`" from a configuration file or a
netCDF group and *build the right object* without the reading code
having any compile-time dependency on the concrete type. It is the
companion of the *methods* factory of [Chapter 15](15-methods-factory.md#ch-15): that one
*mutates* an existing object by method name, this one *creates*
objects by class name.

### Registering a class

A concrete class joins its factory through two macros
(`SMSFactory.h:56-132`):

- in the class declaration (private part of the `.h`):
  `SMSpp_insert_in_factory_h;`
- in *exactly one* `.cpp` (usually the class's own):
  `SMSpp_insert_in_factory_cpp_0( ClassName );` if the base-class
  constructor takes no argument, or
  `SMSpp_insert_in_factory_cpp_1( ClassName );` if it takes one
  (e.g. a `:Block`, whose constructor takes the father `Block*` —
  this is why `MCFBlock.cpp` uses the `_1` form, [§4.5](04-block.md#sec-4-5)). *Template*
  classes use the `_t` variants
  (`SMSpp_insert_in_factory_cpp_0_t` / `_1_t`), which add the
  `template<>` specialisation the compiler requires; this is
  exactly the case of the `SimpleConfiguration<T>` family of
  [Chapter 11](11-configuration.md#ch-11), whose instantiations (e.g. `SimpleConfiguration<
  int >`) are registered with a `_t` macro. If the class name
  contains commas (a template instantiation with more than one
  parameter), it is wrapped in extra parentheses so the
  preprocessor does not read them as macro-argument separators.

The `_h` macro defines a tiny private `_init` class with a single
static member `_initializer`; the `_cpp` macro defines that
member. When the program starts, the `_initializer`'s constructor
runs and does two things: it registers the class in the factory of
its base, **and** it calls the class's `static_initialization()`
method — which is exactly where the methods-factory registrations
of [Chapter 15](15-methods-factory.md#ch-15) belong. So the two factory systems are wired up by
the same one-time static initialisation.

## 18.2 The dark side, discharged: factories versus the linker {#sec-18-2}

This is the mechanism [§15.7](15-methods-factory.md#sec-15-7) warned about, now in full.

The registration code lives inside the static `_initializer` of a
translation unit, and *nothing in the rest of the program
references that translation unit by symbol* — the program only
ever asks the factory for the class by string. An aggressive
linker, seeing no references into the unit (especially when it
sits in a dynamic library), may drop it from the executable. The
`_initializer` then never runs, and the program fails at runtime
with

```
terminate called after throwing an instance of 'std::invalid_argument'
  what():  XXXX not present in YYYY factory
```

even though the class is present in the source
(`SMSFactory.h:79-92`). The cause is *never* a missing
registration in the source; it is a translation unit the linker
has optimised away.

There are three remedies:

1. **Force an object into the executable.** Instantiate an object
   of the missing type somewhere reachable from `main()`. This
   requires `#include`-ing the type's header and is therefore the
   least clean option.
2. **Disable the linker optimisation.** Pass the linker flag that
   keeps unreferenced units: `--no-as-needed` for `g++` (as
   `-Wl,--no-as-needed` when linking through the compiler driver),
   `-all_load` for `clang++`. Note that the default behaviour
   varies across toolchains and even across OS distributions of
   the *same* toolchain, and that these flags can have unwanted
   side effects (they retain *everything*).
3. **Link the module through its CMake target.** The `_cpp`
   macros also define, for each registered class, an empty
   `extern "C"` function `SMSpp_force_load_<ClassName>`, and the
   CMake target of each SMS++ module (built as a shared library)
   tells whoever links it to keep the symbol named after the
   module (`/INCLUDE:` with MSVC, `-u` with the linkers of macOS
   and MinGW), or the library as a whole where naming the symbol
   is not enough (`--no-as-needed` with the ELF linker). A program
   that links a module with `target_link_libraries()` therefore
   gets its registrations without doing anything; the first two
   remedies are for a program built otherwise. (The
   `SMSpp_ensure_load( ClassName )` macro that earlier versions
   provided for this purpose, and that the comment in
   `SMSFactory.h` still mentions, no longer exists.)

The phenomenon is not a defect of SMS++ but the price of resolving
types by name at runtime; the manual flags it because the symptom
("not present in factory") is mystifying until one knows the
cause.

## 18.3 Why netCDF {#sec-18-3}

SMS++ serialises `Block`s, `Configuration`s, `Solution`s,
`State`s and `Change`s to
[netCDF](https://www.unidata.ucar.edu/software/netcdf/), a binary,
self-describing, hierarchical file format. The choice fits the
framework's structure for two reasons:

- netCDF files are organised into nested **groups**, which map
  directly onto the nested structure of a `Block` tree: a `Block`
  serialises into a group, its sub-`Block`s into nested
  sub-groups, recursively. The on-disk shape mirrors the in-memory
  shape.
- netCDF stores **native binary arrays** efficiently, which suits
  the large numeric data (cost vectors, constraint matrices,
  scenario trees) of the problems SMS++ targets, far better than a
  text format such as JSON would.

A practical bonus is tooling: because netCDF is a widely adopted
standard rather than a SMS++ invention, a SMS++ serialised file
can be inspected with general-purpose, third-party tools, no
SMS++-specific viewer required. The standard `ncdump` utility
turns any such file into readable text; `ncview` visualises array
data; and
[Panoply](https://www.giss.nasa.gov/tools/panoply/) — an
open-source graphical viewer available on all platforms, developed
by NASA's
[Goddard Institute for Space Studies](https://www.giss.nasa.gov/)
— offers a richer interactive exploration of the file's groups and
arrays.

## 18.4 The dark side of netCDF: groups are not free {#sec-18-4}

netCDF buys the structural fit of [§18.3](18-factories-netcdf.md#sec-18-3) at a price that users must
know about: **creating a file with a very large number of groups
is inefficient — in fact, very inefficient.** netCDF groups are
designed to be a moderate-cardinality organising device, not a
container one instantiates by the thousand; the cost of writing a
file grows badly with the number of groups in it.

The naïve, "canonical" serialisation of a deeply or widely nested
`Block` tree can fall into exactly this trap. The standard example
is the `Solution` of a `UCBlock`: it contains, among other things,
the `Solution` of a `NetworkBlock` *for each time instant*, and
the canonical encoding would put each `NetworkBlockSolution` in
its own group. But the number of time instants can be large — 8760
for the hours of a year — and writing 8760 (or many more) groups
per solution makes solution files unbearably slow to produce.

Two further facts make the matter worse. First, netCDF-C (up to
4.9.3 at least) crashes when a file gets its 32768-th group, so
that past some size the one-group-per-object encoding is not slow
but impossible. Second, in a netCDF-4 file each passage from the
define mode to the data mode, which netCDF does by itself at the
first write after a definition, writes the metadata of the file
and flushes every dataset open in the process; a `serialize()`
that writes its variables one at a time, as those of `Block` and
`Solution` do, therefore costs a time quadratic in the number of
the variables of the file.

To cope with this, `SMSnetCDF.h` provides a few general functions
that any `:Block` or `:Solution` can use to write a group made of
many parts (`SMSnetCDF.h:2203-2360`):

- `write_via_memory()` has a given function write each part into
  an in-memory netCDF file of its own (an `NcMemFile`), and then
  copies all of them into the destination group with
  `copy_group()`, which makes all the definitions before writing
  any value; the file is thus written in a time linear in its
  size, at the price of holding all the parts in memory together;
- `write_stacked()` writes the parts in the same way, but then
  *stacks* those whose netCDF trees are identical (values apart)
  into a single group, each variable of which gets a leading
  dimension `Stacked` indexed by the part (one stack per distinct
  tree, with the variables `PartStack` and `PartPosition` telling
  where each part is when there are several); a file of many parts
  of few kinds then has the groups of one part of each kind,
  however many the parts are. Stacks can be nested (e.g., the
  `Solution` of a multi-stage `Block` whose scenarios have stacked
  scenarios), the dimension of a stack being then `Stacked_n`;
- `read_stacked()` gives back the i-th part of a stack, rebuilt in
  an in-memory file, and `stacked_size()` the number of the parts
  in it.

The standard example above uses them: a `UCBlockSolution` writes
the `NetworkBlockSolution`s of all the time instants stacked in a
single group `NetworkBlock` (and the solutions of its units
stacked in the group `UnitBlock`, one stack per type of unit), and
the `Solution` of a `TwoStageStochasticBlock` writes those of its
scenarios stacked in the group `ScenarioSolution`; their
`deserialize()` reads both the stacked and the one-group-per-part
encodings.

It is important to be clear about the scope of this remedy: the
functions are a facility that a `:Block`/`:Solution` author calls
in its own `serialize()` and `deserialize()`, not something that
the framework applies by itself. A high-cardinality `:Block` or
`:Solution` written in the naïve one-group-per-object way still
pays the cost described above, and its author has to decide to
write it stacked; and since the stacked encoding is a different
file format, the readers of the file (including the ones outside
SMS++) have to know it.

## 18.5 The serialisation contract {#sec-18-5}

Serialisation is symmetric and recursive. On the writing side,
`serialize(netCDF::NcGroup&)` writes the object into the given
group; a `Block`'s `serialize` writes its own data and then calls
`serialize` on each sub-`Block` into a nested group, so the whole
tree is written by one top-level call. There are convenience
overloads that take a filename or an `NcFile` and create the group
structure for you.

On the reading side, the contract has two levels. The *static*
`deserialize(netCDF::NcGroup&)` reads the mandatory string
attribute `"type"` from the group, passes it to the factory
(`new_Block`, `new_Configuration`, ...) to construct an empty
object of the right concrete type, and then calls that object's
*virtual* `deserialize` to fill it in. This is why the factory and
the serialisation are inseparable: deserialising an object of a
type not known at compile time *requires* the class-name factory
of [§18.1](18-factories-netcdf.md#sec-18-1) to turn the stored `"type"` string into the right object.

The same `"type"`-tag-plus-factory pattern is what lets a
`Configuration` file name an `RBlockSolverConfig` ([Chapter 11](11-configuration.md#ch-11)), a
serialised solution name a `CapacitatedFacilityLocationSolution`
([Chapter 9](09-solution.md#ch-9)), and a shipped `Change` name a
`BinaryKnapsackBlockRngdChange` ([Chapter 16](16-change.md#ch-16)): in every case the
reader constructs the right concrete object from the stored class
name, with no compile-time knowledge of the type.

At the top level, a SMS++ netCDF file carries an `int` attribute
`SMS++_file_type`, one of the values of the enum
`smspp_netCDF_file_type` (`SMSTypedefs.h:110-167`), which says how
its child groups are organised: `eProbFile` (child groups
`Prob_0`, `Prob_1`, ..., each with the three child groups `Block`,
`BlockConfig` and `BlockSolver`), `eBlockFile` (`Block_0`,
`Block_1`, ...), `eConfigFile` (`Config_0`, ...), `eSolutionFile`
(`Solution_0`, ...) and `eStateFile` (`State_0`, ...); the
filename-taking `deserialize()` can be told which of the objects
to read by appending its index in square brackets to the name. The
group of a `Block` has, besides the mandatory `"type"`, the
optional attributes `"name"`, `"ObjectiveWeight"` and
`"VariableWeight"` (the weights of [§12.2](12-sub-block.md#sec-12-2), written only when they
are not 1), and whatever the specific `:Block` needs; the formats of
the existing `:Block`s are described in the SMS++ File Format Manual
in the `doc/` folder of the umbrella project. Finally, a `Block`
can also be written to, and read from, an image of a netCDF-4 file
in memory, `Block::serialize( std::vector< unsigned char > & ,
int )` and `Block::deserialize( std::span< const unsigned char > ,
unsigned int , Block * )`, e.g., to copy a `Block` as it is now,
its changes included, without passing through the filesystem.

## 18.6 Idioms {#sec-18-6}

**Register every concrete class in its factory.** A `:Block`,
`:Solver`, `:Configuration`, `:Solution` or `:Change` that is
meant to be created from a string (and almost all are) must carry
the `SMSpp_insert_in_factory_h` / `_cpp_k` macro pair. Forgetting
it makes the class invisible to `new_X(name)`.

**When a class is "not present in factory", suspect the linker
first.** The error almost never means a missing registration in
the source; it means the registering translation unit was
optimised away. Link the module through its CMake target, which keeps
the registering units, or use the linker flags of
[§18.2](18-factories-netcdf.md#sec-18-2) as the blunt fix.

**Serialise the tree, not the pieces.** Because `serialize` /
`deserialize` recurse, a whole `Block` tree (with its
`Configuration`s and `Solution`s) round-trips through a single
top-level call. Build on that rather than serialising each
sub-`Block` separately.

**Use `ncdump` to debug serialised files.** When a serialised
`Block` does not deserialise as expected, `ncdump` on the file is
the fastest way to see what was actually written, including the
all-important `"type"` attributes.
