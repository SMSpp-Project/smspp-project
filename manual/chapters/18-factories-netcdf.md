# 18. Factories and netCDF serialization {#ch-18}

[Source: [`SMS++/include/SMSFactory.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/SMSFactory.h) (factory macros),
[`SMSnetCDF.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/SMSnetCDF.h), [`SMSTypedefs.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/SMSTypedefs.h), [`Block.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h), [`Configuration.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Configuration.h),
[`Solution.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Solution.h), [`Change.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Change.h),
[`SMS++/src/SMSFactory.cpp`](https://gitlab.com/smspp/smspp/-/blob/develop/src/SMSFactory.cpp), [`SMS++/cmake/SMSppModule.cmake`](https://gitlab.com/smspp/smspp/-/blob/develop/cmake/SMSppModule.cmake)]

We are concerned in this chapter with two mechanisms to which the rest of the
manual refers without treating them head-on: the *class-name factories*,
which let an object be created out of the name of its class, and the
*netCDF serialization*, which lets a whole `Block` tree be written to and
read from disk; we also discharge the promise of
[§15.7](15-methods-factory.md#sec-15-7) about the linker.

We first describe the factories and the registration of a class in them
([§18.1](18-factories-netcdf.md#sec-18-1)), and the reasons why a
registered class may still be missing at run time, with their remedies
([§18.2](18-factories-netcdf.md#sec-18-2)). Then we turn to netCDF: why
it is used ([§18.3](18-factories-netcdf.md#sec-18-3)), the cost of its
groups and the functions that avoid it
([§18.4](18-factories-netcdf.md#sec-18-4)), and the contract of the
serialization ([§18.5](18-factories-netcdf.md#sec-18-5)), followed by the
idioms of both mechanisms ([§18.6](18-factories-netcdf.md#sec-18-6)).
Finally, we describe how the factories are kept unique, and their classes
kept in the program, when the modules are shared libraries
([§18.7](18-factories-netcdf.md#sec-18-7)). Such a library can also be
loaded while the program runs, when the factory is asked for one of its
classes ([§18.8](18-factories-netcdf.md#sec-18-8)).

## 18.1 The class-name factories {#sec-18-1}

Six base classes of the framework carry a *factory*: a static map from
a class name (a `std::string`) to a constructor for that class. The
entry points are

```cpp
static Block * Block::new_Block( const std::string & classname ,
                                 Block * father = nullptr );
static Solver * Solver::new_Solver( const std::string & classname );
static Configuration * Configuration::new_Configuration(
                                     const std::string & classname );
static Solution * Solution::new_Solution( const std::string & classname );
static Change * Change::new_Change( const std::string & classname );
static State * State::new_State( const std::string & classname );
```

with an overload taking a `netCDF::NcGroup` for `Block`, `Configuration`,
`Solution`, `Change` and `State`, which reads the class name from the group
([§18.5](18-factories-netcdf.md#sec-18-5)). Outside the core, other families
have the same structure, e.g., those of `ScenarioGenerator`
(`StochasticBlock`), of `Stepsize` and `Deflection` (`SubGradSolver`) and of
`NetworkBlock::NetworkData` (`UCBlock`). Each map is defined once in the whole
program ([§18.7](18-factories-netcdf.md#sec-18-7)), and since SMS++ 0.9.0
`new_Block()` and `new_Solver()` load the shared library that registers a
class not yet in their factory ([§18.8](18-factories-netcdf.md#sec-18-8)).

Each returns a freshly constructed object of the concrete type
named by the string, as a pointer to the base class. This is what
makes it possible to read "the inner `Block` is a `UCBlock`", or
"attach a `CPXMILPSolver`", or "this serialized object is a
`BinaryKnapsackBlockRngdChange`" from a configuration file or a
netCDF group and *build the right object* without the reading code
having any compile-time dependency on the concrete type. It is the
companion of the *methods* factory of [Chapter 15](15-methods-factory.md#ch-15): that one
*mutates* an existing object by method name, this one *creates*
objects by class name.

### Registering a class

A concrete class joins its factory through two macros
([`SMSFactory.h:56-143`](https://gitlab.com/smspp/smspp/-/blob/develop/include/SMSFactory.h#L56-143) and [`SMSFactory.h:394-522`](https://gitlab.com/smspp/smspp/-/blob/develop/include/SMSFactory.h#L394-522)):

- in the class declaration (private part of the `.h`):
  `SMSpp_insert_in_factory_h;`
- in *exactly one* `.cpp` (usually the class's own):
  `SMSpp_insert_in_factory_cpp_0( ClassName );` if the base-class
  constructor takes no argument, or
  `SMSpp_insert_in_factory_cpp_1( ClassName );` if it takes one
    (e.g., a `:Block`, whose constructor takes the father `Block *`,
  which is why [`MCFBlock.cpp`](https://gitlab.com/smspp/mcfblock/-/blob/develop/src/MCFBlock.cpp) uses the `_1` form,
  [§4.5](04-block.md#sec-4-5)). *Template*
  classes use the `_t` variants
  (`SMSpp_insert_in_factory_cpp_0_t` / `_1_t`), which add the
  `template<>` specialization the compiler requires; this is the case of
the `SimpleConfiguration< T >` family of
[Chapter 11](11-configuration.md#ch-11), whose instantiations (e.g.,
`SimpleConfiguration< int >`) are registered with a `_t` macro. If the class name
  contains commas (a template instantiation with more than one
  parameter), it is wrapped in extra parentheses so the
  preprocessor does not read them as macro-argument separators.

The `_h` macro defines a tiny private `_init` class with a single
static member `_initializer`; the `_cpp` macro defines that
member. When the program starts (or, for a library loaded while the program
runs, when it is loaded), the `_initializer`'s constructor runs and does two
things: it registers the class in the factory of its
base, and it calls the `static_initialization()` method of the class,
which is where the registrations in the methods factory of
[Chapter 15](15-methods-factory.md#ch-15) belong. Hence, the two
factory systems are wired up by the same one-time static
initialization.

## 18.2 Factories and the linker {#sec-18-2}

The registration code lives inside the static `_initializer` of a
translation unit, and *nothing in the rest of the program
references that translation unit by symbol*, since the program only
ever asks the factory for the class by string. From a static library the
linker takes only the objects that resolve a symbol asked for, and therefore
it leaves out an object whose only business is a registration. A shared
library is taken whole or not at all; however, some linkers (in particular
the ELF ones under `--as-needed`, the default of several distributions)
record it as needed only if a regular object of the link asks for one of its
symbols.
Either way, the `_initializer` then never runs, and the program fails at
run time with

```
terminate called after throwing an instance of 'std::invalid_argument'
  what():  XXXX not present in YYYY factory
```

even though the class is present in the source ([`SMSFactory.h:80-123`](https://gitlab.com/smspp/smspp/-/blob/develop/include/SMSFactory.h#L80-123)).
If the class does carry the two macros, the cause is a translation unit
that the linker has left out or, with shared libraries, a module that the
program does not link and that no table of the plugins names
([§18.8](18-factories-netcdf.md#sec-18-8)).

There are four remedies:

1. *Forcing an object into the executable.* Instantiate an object
   of the missing type somewhere reachable from `main()`. This
   requires `#include`-ing the type's header and is therefore the
   least clean option.
2. *Passing the linker the options that keep the library*, which depend on
   its kind. A static library is linked whole by `-Wl,--whole-archive`
   (closed by `-Wl,--no-whole-archive`) with the GNU linkers, by
   `-force_load` followed by the archive (or `-all_load`, for all of them)
   with `ld64`, and by `/WHOLEARCHIVE:` with MSVC. A shared library is kept
   by `-Wl,--no-as-needed` with the ELF linkers and, where the import of a
   DLL is recorded only for the symbols asked for (MinGW and MSVC), by
   asking for one of its symbols, i.e., by `-u` followed by the name of the
   function of remedy 3 with MinGW and by `/INCLUDE:` with MSVC, while
   `ld64` keeps a dylib unless `-dead_strip_dylibs` is given. Note that the
   default behavior varies across toolchains and even across OS
   distributions of the *same* toolchain, and that the options for a static
   library retain *everything* in it.
3. *Linking the module through its CMake target.* The `_cpp`
   macros also define, for each registered non-template class, an empty
   `extern "C"` function `SMSpp_force_load_<ClassName>`, and (since SMS++
   0.9.0) the CMake target of each SMS++ module makes whoever links it call
   the one named after the module, while the static libraries are linked
   whole ([§18.7](18-factories-netcdf.md#sec-18-7)). A program that links a
   module with `target_link_libraries()` therefore gets its registrations
   without doing anything; the first two remedies are for a program
   built otherwise.
4. *Letting the factory load the module.* With shared libraries, a
   `:Block` or a `:Solver` of a module that the program does not link is
   constructed all the same if a table of the plugins names it
   ([§18.8](18-factories-netcdf.md#sec-18-8)). The error then means that the
   module has not been built or installed, or that its table is not on the
   search path.

The phenomenon is the price of resolving types by name at run time, and the
symptom ("not present in factory") is hard to interpret until one knows the
cause.

## 18.3 Why netCDF {#sec-18-3}

SMS++ serializes `Block`s, `Configuration`s, `Solution`s,
`State`s and `Change`s to
[netCDF](https://www.unidata.ucar.edu/software/netcdf/), a binary,
self-describing, hierarchical file format. The choice fits the
framework's structure for two reasons:

- netCDF files are organized into nested *groups*, which map
  directly onto the nested structure of a `Block` tree: a `Block`
  serializes into a group, its sub-`Block`s into nested
  sub-groups, recursively. The on-disk shape mirrors the in-memory
  shape.
- netCDF stores *native binary arrays* efficiently, which suits
  the large numeric data (cost vectors, constraint matrices,
  scenario trees) of the problems SMS++ targets, far better than a
  text format such as JSON would.

Also, since netCDF is a widely adopted standard, an SMS++ file can be
inspected with general-purpose tools: the standard `ncdump` utility turns it
into readable text, `ncview` visualizes array data, and
[Panoply](https://www.giss.nasa.gov/tools/panoply/), a graphical viewer, shows
the groups and the arrays of a file.

## 18.4 The cost of netCDF groups {#sec-18-4}

netCDF buys the structural fit of [§18.3](18-factories-netcdf.md#sec-18-3) at a price that users must
know about: creating a file with a very large number of groups is
very inefficient. netCDF groups are
designed to be a moderate-cardinality organizing device, not a
container one instantiates by the thousand; the cost of writing a
file grows badly with the number of groups in it.

The canonical serialization of a deeply or widely nested
`Block` tree can fall into exactly this trap. The standard example
is the `Solution` of a `UCBlock`: it contains, among other things,
the `Solution` of a `NetworkBlock` *for each time instant*, and the
canonical encoding would put each `NetworkBlockSolution` in its own
group. However, the number of time instants can be large (8760 for the
hours of a year), and writing 8760 (or many more) groups per solution
makes solution files very slow to produce.

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

To cope with this, [`SMSnetCDF.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/SMSnetCDF.h) provides a few general functions
that any `:Block` or `:Solution` can use to write a group made of
many parts ([`SMSnetCDF.h:2203-2360`](https://gitlab.com/smspp/smspp/-/blob/develop/include/SMSnetCDF.h#L2203-2360)):

- `write_via_memory()` has a given function write each part into
  an in-memory netCDF file of its own (an `NcMemFile`), and then
  copies all of them into the destination group with
  `copy_group()`, which makes all the definitions before writing
any value, at the price of holding all the parts in memory together
([Lemma 18.1](18-factories-netcdf.md#lem-18-1) below gives the cost);
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

With a netCDF-C earlier than 4.6.2, which has no in-memory files,
`write_via_memory()` writes each part directly in its sub-group,
`write_stacked()` does the same and returns `false`, and `read_stacked()`
throws `std::logic_error`. Also, `copy_group()` throws `std::length_error`,
without copying, if the destination file would have more than 32767 groups.

The standard example above uses them: a `UCBlockSolution` writes
the `NetworkBlockSolution`s of all the time instants stacked in a
single group `NetworkBlock` when there is one for every time instant, and
one group per time instant (by `write_via_memory()`) otherwise
([`UCBlock.cpp:3691-3712`](https://gitlab.com/smspp/ucblock/-/blob/develop/src/UCBlock.cpp#L3691-3712)), and the solutions of its units
stacked in the group `UnitBlock`, one stack per type of unit; the
`Solution` of a `TwoStageStochasticBlock` writes those of its
scenarios stacked in the group `ScenarioSolution`; their
`deserialize()` reads both the stacked and the one-group-per-part
encodings.

The cost model behind these functions is the following, where we count
only the passages from the define mode to the data mode, each of which
costs a time proportional to the number of the datasets open in the process
(it writes the metadata of the file and flushes those datasets, the ones of
the other files too).

<span id="lem-18-1"></span>**Lemma 18.1.** *Let a group of $n$ variables be
made of $p$ parts of $n_1, \ldots, n_p$ variables, and let no dataset other
than those written here be open in the process while the group is written
(in particular, let the destination file hold no variable yet). Writing the
variables one at a time, each definition followed by its values, costs
$\Theta(n^2)$; writing each part into an in-memory file of its own in the
same way, and then copying all of them with all the definitions first, costs
$O\big(\sum_{k} n_k^2 + n\big)$, which is $O(n)$ when the size of the parts
is bounded by a constant.*

*Proof.* In the first case the $k$-th variable is defined when $k - 1$
are already there, and its first write causes a passage that costs
$\Theta(k)$; summing over $k = 1, \ldots, n$ gives $\Theta(n^2)$. In
the second case, part $k$ is written in a file of its own, whose
passages cost $\Theta(1 + \cdots + n_k) = \Theta(n_k^2)$ by the same
argument, since the files of the previous parts have been closed and, by
hypothesis, no other dataset is open; the copy makes
all the definitions and then one passage, of cost $\Theta(n)$, and
writes the values in a time linear in their size. If $n_k \le c$ for
all $k$, then $\sum_k n_k^2 \le c \sum_k n_k = c\, n$. $\square$

This is why the header calls the writing linear: the parts (the solution
of a time instant, of a unit, of a scenario) are small and of bounded
size, while their number grows with the instance. The stacking of
`write_stacked()` is a different saving, on the number of groups: by
construction, a file of $p$ parts whose netCDF trees fall into $q$
distinct shapes has the groups of $q$ parts, whatever $p$ is, which
also keeps the file far from the limit of 32767 groups.

We remark that these functions are a facility that a `:Block`/`:Solution`
author calls in its own `serialize()` and `deserialize()`, not something that
the framework applies by itself. A high-cardinality `:Block` or `:Solution`
written in the one-group-per-object way still pays the cost described above,
and its author has to decide to write it stacked; and since the stacked
encoding is a different file format, the readers of the file (including the
ones outside SMS++) have to know it.

## 18.5 The serialization contract {#sec-18-5}

Serialization is symmetric and recursive. On the writing side,
`serialize( netCDF::NcGroup & group ) const` writes the object into the
given group; the `serialize()` of a `:Block` writes its own data and
then that of each sub-`Block` into a nested group, so that the whole
tree is written by one top-level call. The recursion is the business of
each `:Block`, since the base `Block::serialize()` and
`Block::deserialize()` only handle the attributes common to every
`Block` (the sub-`Block`s may be structured in groups with different
properties), and the `serialize()` of a `:Block` calls the one of the
base. There are convenience overloads that take a filename and create
the group structure of an SMS++ file.

On the reading side, the contract has two levels. The *static* `deserialize(
const netCDF::NcGroup & )` (or `new_Block()` with a group) reads the string
attribute `"type"` from the group, passes it to the factory (`new_Block`,
`new_Configuration`, ...) to construct an empty object of the right concrete
type, and then calls that object's *virtual* `deserialize` to fill it in. This
is why the factory and the serialization are inseparable: deserializing an
object of a type not known at compile time *requires* the class-name factory
of [§18.1](18-factories-netcdf.md#sec-18-1) to turn the stored `"type"` string
into the right object. A group may also describe a `Block` *indirectly*, with
no `"type"` and a string attribute `"filename"`, which names the file (netCDF,
where the name can also give the position of the object, or text) from which
the `Block` is read. If both attributes are there, `"type"` takes precedence;
the same holds for a `Configuration`, a `Solution` and a `State`. Note that
`new_Block()` with a group returns `nullptr`, instead of throwing, if anything
goes wrong
([`Block.h:1291-1322`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L1291-1322)).

The same `"type"`-tag-plus-factory pattern is what lets a
`Configuration` file name an `RBlockSolverConfig` ([Chapter 11](11-configuration.md#ch-11)), a
serialized solution name a `CapacitatedFacilityLocationSolution`
([Chapter 9](09-solution.md#ch-9)), and a shipped `Change` name a
`BinaryKnapsackBlockRngdChange` ([Chapter 16](16-change.md#ch-16)): in every case the reader constructs the right concrete object from the stored class
name, with no compile-time knowledge of the type.

At the top level, an SMS++ netCDF file carries an `int` attribute
`SMS++_file_type`, one of the values of the enum `smspp_netCDF_file_type`
([`SMSTypedefs.h:162-219`](https://gitlab.com/smspp/smspp/-/blob/develop/include/SMSTypedefs.h#L162-219)),
which says how its child groups are organized: `eProbFile` (child groups
`Prob_0`, `Prob_1`, ..., each with the three child groups `Block`,
`BlockConfig` and `BlockSolver`), `eBlockFile` (`Block_0`, `Block_1`, ...),
`eConfigFile` (`Config_0`, ...), `eSolutionFile` (`Solution_0`, ...) and
`eStateFile` (`State_0`, ...); the filename-taking `deserialize()` can be told
which of the objects to read by appending its index in square brackets to the
name. The group of a `Block` has, besides `"type"`, the optional attributes
`"name"`, `"ObjectiveWeight"` and `"VariableWeight"` (the weights of
[§12.2](12-sub-block.md#sec-12-2), written only when they are not 1, and 1
when absent), and whatever the specific `:Block` needs; the formats of the
existing `:Block`s are described in the SMS++ File Format Manual in the `doc/`
folder of the umbrella project. Finally, a `Block` can also be written to, and
read from, an image of a netCDF-4 file in memory, with `void serialize(
std::vector< unsigned char > & image , int type = eProbFile ) const` and the
static `Block::deserialize( std::span< const unsigned char > image , unsigned
int idx = 0 , Block * father = nullptr , std::function< void( Block * ) > * f
= nullptr )` (which need netCDF-C 4.6.2 or later), e.g., to copy a `Block` as
it is now, its changes included, without passing through the filesystem. The
latter returns `nullptr` if anything goes wrong. Through a pointer to a
`:Block` that redefines `serialize( netCDF::NcGroup & )`, which hides the
other forms, the former is called as `myBlock->Block::serialize( image )`
([`Block.h:1258-1287`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L1258-1287)).

## 18.6 Idioms {#sec-18-6}

*Every concrete class in its factory.* A `:Block`, `:Solver`,
`:Configuration`, `:Solution`, `:Change` or `:State` that is meant to be
created from a string (and almost all are) must carry the
`SMSpp_insert_in_factory_h` / `_cpp_k` macro pair. Forgetting it makes the
class invisible to `new_X( name )`.

*"Not present in factory": the linker first, then the plugins.* If the
class carries the two macros, the error means that the registering
translation unit was left out by the linker or, for a module that the
program does not link, that no table of the plugins names the class. Link
the module through its CMake target, which keeps the registering units, or
use the linker options of [§18.2](18-factories-netcdf.md#sec-18-2); for a
plugin, check that the module is built or installed and
that its table is on the search path of
[§18.8](18-factories-netcdf.md#sec-18-8). `Solver::has_Solver()` answers,
without throwing, whether a `:Solver` can be constructed, which allows a
program to leave out one whose module (or external library) is missing.

*The tree, not the pieces.* Since `serialize()` and `deserialize()`
recurse, a whole `Block` tree (with its
`Configuration`s and `Solution`s) round-trips through a single
top-level call. Build on that rather than serializing each
sub-`Block` separately.

*`ncdump` for debugging.* When a serialized
`Block` does not deserialize as expected, `ncdump` on the file is
the fastest way to see what was actually written, including the `"type"`
attributes.

## 18.7 Factories across shared libraries {#sec-18-7}

When the modules are built as shared libraries (DLLs on Windows), the core
and each module are separate units of the dynamic linker, and two
requirements follow: every library has to reach the same map of each
factory, and the library of a module has to be kept by the linker of every
program that links it, although the program never names any of its symbols.

*One map per family.* A static member of a class, or a static local variable
of an inline function or of a template, that a header defines may exist once
per library that uses it. This is the case of the DLLs, each of which would
then have a map of its own, so that a class registered by the DLL of a module
would go into a map that the core never reads. Hence each map is a static
local variable of a function defined once, in a source file of the core:
`f_factory()` of `Block`, `Solver`, `Configuration`, `Solution`, `Change` and
`State` is in `Block.cpp`, `Solver.cpp`, `Configuration.cpp`, `Solution.cpp`,
`Change.cpp` and `ThinComputeInterface.cpp`, respectively, and the maps of the
methods factory of [Chapter 15](15-methods-factory.md#ch-15) are in
`Block.cpp`. The DLLs built by the umbrella export all their functions
(`CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS`), but not their data, and therefore every
library calls the same function and reaches the same map. The factories of
the modules are defined in the same way, in a source file of the module: that
of `ScenarioGenerator` in
[`DiscreteScenarioSet.cpp`](https://gitlab.com/smspp/stochasticblock/-/blob/develop/src/DiscreteScenarioSet.cpp#L613-622),
those of `Stepsize` and `Deflection` in `SubGradSolver.cpp`, and that of
`NetworkBlock::NetworkData` in `NetworkBlock.cpp`.

*Keeping a library.* For each non-template class, the macros
`SMSpp_insert_in_factory_cpp_0` and `_1` define the empty function `extern "C"
void SMSpp_force_load_<ClassName>( void )`. The function `smspp_keep_module(
<target> [<name> ...] )` of `SMSppModule.cmake` generates a source, compiled
into every target that links `<target>`, whose static object calls
`SMSpp_force_load_<name>()` for each `<name>` (`<target>` if none is given)
when the program starts
([`SMSppModule.cmake:56-90`](https://gitlab.com/smspp/smspp/-/blob/develop/cmake/SMSppModule.cmake#L56-90));
a module that has no non-template class named after it (e.g.,
`MCFClassSolver`, whose classes are `MCFSolver< ... >` and `MCFSolverState`)
defines that function by `SMSpp_define_force_load( <module> )`, as
`MCFSolver.cpp` does, or passes the name of one of its classes as `<name>`, as
`LagrangianDualSolver` does for the target of
`LagrangianDualRelaxationSolverML`. A library that provides a symbol to a
regular object of the link is kept by every linker, and therefore the library
of the module, with all its registrations, is loaded with the program. The
source is installed with the package configuration file of the module, and
hence a project that finds the module with `find_package()` and links it with
`target_link_libraries()` gets it as well. A static library, instead, loses
the objects that nothing names: `smspp_keep_modules()` makes each program and
library of the directory in which it is called (and of those below it) link
whole the static SMS++ libraries that it reaches through its links, by the
`WHOLE_ARCHIVE` feature of CMake
([`SMSppModule.cmake:92-167`](https://gitlab.com/smspp/smspp/-/blob/develop/cmake/SMSppModule.cmake#L92-167)).
This needs CMake 3.30 or later; an older one stops the configuration with an
error, instead of linking an archive whole twice. The makefiles do the same
with the options of remedy 2 of [§18.2](18-factories-netcdf.md#sec-18-2)
(`--no-as-needed` on Linux, `-u` on Windows).

## 18.8 Plugins {#sec-18-8}

A `:Block` or a `:Solver` may also be in a shared library that the program
does not link at all, a *plugin*. When `new_Block()` or `new_Solver()` is
asked for a class that their factory does not hold, they look for the plugin
that registers it and load it, which registers its classes as if it were
linked, and then look again. The entry points are

```cpp
bool SMSpp_load_plugin( const std::string & classname );
std::shared_mutex & SMSpp_factory_mutex( void );

static bool Solver::has_Solver( const std::string & classname );
static std::vector< std::string > Block::get_factory_names( void );
static std::vector< std::string > Solver::get_factory_names( void );
```

*Tables.* The plugin that registers a class is found in the *tables*, the
files with extension `.smspp`, each line of which is the name of a class and
the file of its plugin, relative to the directory of the table. The build
writes them after the link, by the tool `smspp_plugin_names` of the core,
which loads the shared libraries that the plugin links (whose classes are
not its own) and then the plugin, and lists each `:Block` and `:Solver` that
the plugin has added to the factories. The shared library of every module is
a plugin, whose table is written in the directory of the plugins of the build
tree and next to the library, and is installed with it. Also,
`smspp_plugin( <target> )` makes a plugin of a library that nothing links:
`MILPSolver` does so with each of its back-ends when the libraries are shared,
so that a program built with no MILP solver runs with those that are there.
A static library is no plugin. The tables are looked for in a *search path*
made of the directories of the environment variable `SMSpp_PLUGIN_PATH`
(separated as in `PATH`), the directory of the plugins of the build tree,
that of the library of SMS++ and (where this is a DLL among the programs)
that of the libraries of its installation, in this order.

<span id="alg-18-1"></span>**Algorithm 18.1** (loading the plugin of a
class).

```text
 1  function LOAD-PLUGIN(c)
 2     lock the factories exclusively
 3     if the tables have not been read yet then
 4        for each directory d of the search path, in order do
 5           for each table t in d do
 6              for each line (name, file) of t do
 7                 if name ∉ T then T[name] ← the file, as a path in d
 8        mark the tables as read
 9     if c ∉ T or T[c] has been loaded then return false
10     load T[c], its symbols global and resolved at once
11     if the loading fails then throw runtime_error
12     record T[c] as loaded;  return true
```

<span id="alg-18-2"></span>**Algorithm 18.2** (constructing a `:Block` or a
`:Solver` by name).

```text
 1  function NEW(c)
 2     c ← c without whitespace and enclosing parentheses
 3     F ← the constructor of c in the factory, read under the shared lock
 4     if F = none then
 5        LOAD-PLUGIN(c)
 6        F ← the constructor of c in the factory, read under the shared lock
 7     if F = none then throw invalid_argument
 8     return F()                             ▷ out of the lock
```

[Algorithm 18.1](18-factories-netcdf.md#alg-18-1) is implemented by `SMSpp_load_plugin()`
([`SMSFactory.cpp:75-174`](https://gitlab.com/smspp/smspp/-/blob/develop/src/SMSFactory.cpp#L75-174)), and [Algorithm 18.2](18-factories-netcdf.md#alg-18-2) by `Block::new_Block()`
and `Solver::new_Solver()`; `Solver::has_Solver()` runs lines 2-6 and returns
whether $F$ is there, and `false` if LOAD-PLUGIN throws. The registration of
the classes happens at line 10, where loading the library runs its static
initialization ([§18.1](18-factories-netcdf.md#sec-18-1)). A plugin is never
unloaded, since its classes may be in use (in the factories and in the
static objects) until the end of the program. Only the factories of `Block`
and `Solver` load plugins: the classes of `Configuration`, `Solution`,
`Change` and `State` are registered when the library that holds them is
loaded, by linking it or as the plugin of one of its `:Block`s or `:Solver`s.

The registrations of the classes linked into the program are made before
`main()`, with no lock. Every other write into the factories of `Block` and
`Solver` is made at line 10, under the exclusive lock, while their readers
(lines 3 and 6 of [Algorithm 18.2](18-factories-netcdf.md#alg-18-2), and `get_factory_names()`) hold the lock
shared. The construction of the object (line 8) is made out of the lock,
which therefore protects only the maps. The following statement collects
what this gives to concurrent callers.

<span id="lem-18-2"></span>**Lemma 18.2** (Construction on demand). *Let $c$
be the name of a class that the program does not link and that only the
library $f$ registers. Let the first directory of the search path whose
tables name $c$ have only one table that names it, and let $f$ be the file
that this table gives for it, as the tables are when they are first read in
the run. Then, however the steps of the threads that call `NEW` and
`has_Solver()` interleave, (i) no thread reads a map of the
two factories while another one writes it, (ii) $f$ is loaded at most once,
and (iii) `NEW`($c$) returns an object of class $c$ if $f$ can be loaded,
and throws `std::runtime_error` if it cannot.*

*Proof.* (i) After `main()` has started, a map is written only by the static
initialization of a library, which runs at line 10 of [Algorithm
18.1](18-factories-netcdf.md#alg-18-1) under the exclusive lock, while every
read is made under the shared lock, and the two exclude each other. (ii) The
test of line 9 and the record of line 12 are made in the same critical
section, and the record follows every successful load. (iii) By the choice of
$f$ and since the tables are read once, $T[c] = f$ at every call. If $c$ is
found at line 3 of [Algorithm 18.2](18-factories-netcdf.md#alg-18-2) the claim
holds, since only $f$ registers $c$. Otherwise, let $f$ be loadable. The call
of LOAD-PLUGIN at line 5 either loads it, registering $c$ before the lock is
released, or finds it loaded by another thread, which has registered $c$
before releasing the lock that this call has acquired afterwards; in both
cases line 6 finds $c$. If $f$ cannot be loaded, it is never recorded at line
12, and every call of LOAD-PLUGIN($c$) throws at line 11. $\square$

If no table names $c$, LOAD-PLUGIN returns `false` and `NEW` throws
`std::invalid_argument`, as it would without plugins. Since the tables are
read once per run, a table written after that is not seen until the next
run. The hypothesis that only $f$ registers $c$ matters: a class registered
by two libraries is constructed by the one of them that has been loaded
last, whose registration replaces the other. So does the hypothesis on the
tables, since those of a directory are read in an unspecified order and the
first one that names $c$ is kept. The same static initialization writes
the factories of `Configuration`, `Solution`, `Change` and `State`, those of
the modules of [§18.1](18-factories-netcdf.md#sec-18-1) (e.g., of
`ScenarioGenerator` and `Stepsize`) and the methods factory, which are instead
read with no lock; hence, a program that
loads plugins from several threads has to construct those objects by name
only when no plugin is being loaded.

Consequently, a program linked to the core alone can construct any `:Block`
and any `:Solver` of the modules whose tables are on the search path, e.g.,
those built in the same build tree or installed with the core. Also, a
program that applies a configuration naming a `:Solver` can ask
`has_Solver()` first, and leave out (with a message) one whose module, or
external library, is not there.
