# Radixsortbidi8
This library implements an efficient stable sort on arrays using an extensible 8-bit indexed, bidirectional, least significant bit first radix sort method.
This is currently a single-file library, with some additional folders and files only used for its test suite.
Sorting functionality is available for unsigned integer, signed integer, floating-point and enumeration types.
All these sorting functions can sort ascending or descending, order forwards or reverse, and optionally filter by absolute value.
Several filters are available, such as two types of absolute, and an inverse pattern for signed integer and floating-point types.
See "Modes of operation for the template functions" for more details on that.
Implemented function optimisations include the ability to skip sorting steps, using multithreaded parallel (bidirectional) indexing and copying while sorting, and usage of platform-specific intrinsic functions.
Radix sort in general can be used to sort all array sizes, but is more efficient when applied to somewhat larger arrays compared to other efficient (and stable) comparison-based methods, like introsort.
See "Performance tests" for more details about array sizes, types and achievable improvements in speed when sorting large arrays with this library.

## Table of contents (searchable)
### Documentation block:
- MIT License
- Radixsortbidi8
- Table of contents
- Examples of using the 4 templates with simple arrays as input
- Examples of using the 4 templates with input from indirection
- Examples of using the 4 templates with input from modified indirection
- Bonus example of the longest regular item, with complete decoration of the template
- The 4 main sorting template functions that are implemented here
- Modes of operation for the template functions
- Miscellaneous notes
- Naming and tooling conventions used in this library
- Notes on ongoing research and development
- Extended filtering information for each of the 8 main modes
- Performance tests
### Compiler configuration block:
- Per-compiler function attributes
- Include statements and the last checks for compatibility
- Utilities to create an immediate member object pointer for the type and offset indirection wrapper functions
- Definition of the GetOffsetOf and getoffsetof templates
- Apply compiler- and platform-specific intrinsic functions headers
### Internal functions implementation block (rsbd8::helper namespace):
- Utilities for multithreaded concurrency
- Utilities for general purpose register count compile-time detection
- Utilities to provide piecewise support and tests for the often padded 80-bit long double types
- Utilities to either pass through a type or allow std::underlying_type to do its work for enum types
- Utilities to calculate sorting loop counts and offsets lengths
- Utilities for endianess compile-time detection
- Utilities to provide piecewise support and tests for 64- or 128-bit types
- Utilities to call the user-provided getter functions
- Utilities to split off the first parameter
- Utilities to retrieve the data sources from user-provided classes
- Utilities to get either the user-provided member object type or the user-provided member function return type
- Utilities to reinterpret a type into a sortable struct type or an unsigned type of the lowest rank with the same size
- Utilities to perform bit carry operations onto an accumulator for the operators less than, and less than or equal
- Utilities for providing portable bit scan forward functionality
- Utilities for providing portable rotate left or right by a compile-time constant amount functionality
- Utilities to implement input array data slices based on the array length divided by the thread count
- Utilities to implement the 8 combinations of absolute value, signed type and floating-point type sorting modes
- Utilities to produce the accumulated index counts from multple arrays
- Utilities to implement the index counts to offsets transforms
- Utilities to establish the tresholds for 2-, 4-, 6-, 8-, 12- and 18-way multithreading
- Function implementation templates for 80-bit-based long double types without indirection
- Function implementation templates for 80-bit-based long double types with indirection
- Function implementation templates for split up 128-bit types without indirection
- Function implementation templates for split up 128-bit types with indirection
- Function implementation templates for split up 64-bit types without indirection
- Function implementation templates for split up 64-bit types with indirection
- Function implementation templates for multi-part types without indirection
- Function implementation templates for multi-part types with indirection
- Function implementation templates for single-part types without indirection
- Function implementation templates for single-part types with indirection
- 1- to 16-way multithreading function reroutes
- Reroutes to the 1- or 2-thread functions for single-part types
- Reroutes to the 1- or 2-thread functions (variants without indirection)
- Reroutes to the 1- or 2-thread functions (variants with indirection)
- Helper class to safely moderate a std::future<std::future<void>> item on the stack
- Helper functions for converting inputs to perform unsigned comparisons in a final merging phase
- Helper functions for merging the halves from multithreading inputs without indirection
- Up to 4-way multithreading functions without indirection
- Helper functions for merging the thirds from multithreading inputs without indirection
- Up to 6-way multithreading functions without indirection
- Up to 8-way multithreading functions without indirection
- Up to 16-way multithreading functions without indirection
- Helper functions for merging the halves from multithreading inputs with indirection
- Up to 4-way multithreading functions with indirection
- Helper functions for merging the thirds from multithreading inputs with indirection
- Up to 6-way multithreading functions with indirection
- Up to 8-way multithreading functions with indirection
- Up to 16-way multithreading functions with indirection
### User-facing (inline) functions block (rsbd8 namespace):
- Generic large array allocation and deallocation functions
- Wrapper template functions for the main sorting functions in this library
### Ending:
- Library finalisation

## Examples of using the 4 templates with simple arrays as input (automatically deduced template parameters are omitted here):
The rsbd8::radixsort() and rsbd8::radixsortcopy() template wrapper functions (typically) merely allocate memory and determine the number of threads to use prior to entering the actual sorting functions.
No intermediate buffer array is required when any variant of rsbd8::radixsortcopynoalloc() is used for sorting 8-bit types, as it uses a bucket sort approach.

```C++
bool succeeded{rsbd8::radixsort(count, inputarr, pagesizeoptional)};
bool succeeded{rsbd8::radixsortcopy(count, inputarr, outputarr, pagesizeoptional)};
rsbd8::radixsortnoalloc(count, allowedthreads, pfuturesplaceholder, pzeroedindices, inputarr, bufferarr, movetobuffer)// multithreaded mode
rsbd8::radixsortcopynoalloc(count, allowedthreads, pfuturesplaceholder, pzeroedindices, inputarr, outputarr, bufferarr);// multithreaded mode
rsbd8::radixsortnoalloc(count, pzeroedindices, inputarr, bufferarr, movetobuffer)// exclusive single-threaded mode for the entire library
rsbd8::radixsortcopynoalloc(count, pzeroedindices, inputarr, outputarr, bufferarr);// exclusive single-threaded mode for the entire library
```

## Examples of using the 4 templates with input from first-level indirection (automatically deduced template parameters are omitted here):
This library includes common and much less common use scenarios to deal with input from first- and second-level indirection.
The address offset template parameters (compile-time constants) displace the pointers as acting on a flat "std::byte const *" array, so not as some sort of array indices, to handle some cases with oddly formed structures.
For the more advanced use cases, an extra argument (run-time, variadic function parameter) can be added to index the indirection when dealing with a (member) pointer/array.
These index parameters are typically used in a more straightforward manner and use regular indexing.
The variant with a getter function allows any number of extra arguments to pass on to the getter function.
Using a getter function that can throw (meaning that it lacks "noexcept") will incur some extra processing overhead.
Multithreading can be limited at compile-time by setting the macro RSBD8_THREAD_MAXIMUM to a set number of simultaneous threads, but this is only intended for debugging and temporary performance tuning.
The exclusive single-threaded mode for the entire library is enabled by setting the macro RSBD8_THREAD_MAXIMUM to 1.
There is no multithreading limit by default, but when multithreading is enabled, rsbd8::radixsort() and rsbd8::radixsortcopy() will query std::thread::hardware_concurrency() to set up that limit.
This library does not automatically cross a NUMA node boundary, and it is up to the user to set up the environment for that if desired.
This library does have merging functions to make processing across NUMA nodes feasible, but it is not implemented in the main sorting functions.
Limits for multithreading based on the input count can be partially disabled at compile-time by setting the macro RSBD8_THREAD_MINIMUM to force using a minimum number of simultaneous threads if technically possible, but this is only intended for debugging and temporary performance tuning.
This is again not enabled by default. The much lower limits for allowing multithreading at runtime at the the absolute minimum input count for the implemented multithreading functions always apply.

```C++
bool succeeded{rsbd8::radixsort<&myclass::getterfunc>(count, inputarr, pagesizeoptional)};
bool succeeded{rsbd8::radixsort<&myclass::member>(count, inputarr, pagesizeoptional)};
bool succeeded{rsbd8::radixsort<std::uint64_t, addressoffset>(count, inputarr, pagesizeoptional)};

bool succeeded{rsbd8::radixsortcopy<&myclass::getterfunc>(count, inputarr, outputarr, pagesizeoptional, getterparameters...)};
bool succeeded{rsbd8::radixsortcopy<&myclass::member>(count, inputarr, outputarr, pagesizeoptional)};
bool succeeded{rsbd8::radixsortcopy<bool, addressoffset>(count, inputarr, outputarr, pagesizeoptional)};
```

### There are only a few template functions that almost directly implement sorting with indirection here:
```C++
rsbd8::radixsortcopynoalloc()
rsbd8::radixsortnoalloc()
```
These both have 8-bit, 16/24/32/40/48/56/64-bit, (x87) 80-bit (plus padding) and 128-bit type template functions.
These all have a version that handles straightforward arrays, and a version that handles arrays with indirection.
All other items are compile-time, multithreading, or inlined wrapper helper tools for these.
### This base functionality can be expanded (like several wrapper functions do) by utilising the tools:
```C++
rsbd8::getoffsetof
rsbd8::allocatearray()
rsbd8::deallocatearray()
rsbd8::buffermemorywrapper
```

## Examples of using the 4 templates with input from second-level indirection (automatically deduced template parameters are omitted here):
These have options for using a getter function, a member object pointer, or using a pointer to an array of pointers to the actual data.
These last two will internally first retrieve a pointer to a "T" type array "T *myarray".
After that it's dereferenced at the origin (first set of examples) or indexed (second set of examples) as "myarray[indirectionindex]" to retrieve the value used for sorting.
Again, all "addressoffset" variants as template inputs displace like on a flat "std::byte const *", so not as some sort of array indices.
The "addressoffset1" item here displaces the pointer in the input array to get the secondary pointer.
The "addressoffset2" item here displaces the secondary pointer to get the (unfiltered) sorting value.

### Examples of using the 2 templates with no indexed second-level indirection:

```C++
bool succeeded{rsbd8::radixsortcopy<&myclass::getterfunc, rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::native, addressoffset2>(count, inputarr, outputarr, pagesizeoptional, getterparameters...)};
bool succeeded{rsbd8::radixsortcopy<&myclass::member, rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::native, addressoffset2>(count, inputarr, outputarr, pagesizeoptional)};
bool succeeded{rsbd8::radixsortcopy<long *, addressoffset1, rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::native, addressoffset2>(count, inputarr, outputarr, pagesizeoptional)};

bool succeeded{rsbd8::radixsort<&myclass::getterfunc, rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::native, addressoffset2>(count, inputarr, pagesizeoptional, getterparameters...)};
bool succeeded{rsbd8::radixsort<&myclass::member, rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::native, addressoffset2>(count, inputarr, pagesizeoptional)};
bool succeeded{rsbd8::radixsort<wchar_t *, addressoffset1, rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::native, addressoffset2>(count, inputarr, pagesizeoptional)};
```

### Examples of using the 2 templates with indexed second-level indirection:

```C++
bool succeeded{rsbd8::radixsortcopy<&myclass::getterfunc, rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::native, addressoffset2, true>(count, inputarr, outputarr, pagesizeoptional, indirectionindex, getterparameters...)};
bool succeeded{rsbd8::radixsortcopy<&myclass::member, rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::native, addressoffset2, true>(count, inputarr, outputarr, pagesizeoptional, indirectionindex)};
bool succeeded{rsbd8::radixsortcopy<double *, addressoffset1, rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::native, addressoffset2, true>(count, inputarr, outputarr, pagesizeoptional, indirectionindex)};

bool succeeded{rsbd8::radixsort<&myclass::getterfunc, rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::native, addressoffset2, true>(count, inputarr, pagesizeoptional, indirectionindex, getterparameters...)};
bool succeeded{rsbd8::radixsort<&myclass::member, rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::native, addressoffset2, true>(count, inputarr, pagesizeoptional, indirectionindex)};
bool succeeded{rsbd8::radixsort<unsigned long *, addressoffset1, rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::native, addressoffset2, true>(count, inputarr, pagesizeoptional, indirectionindex)};
```

### Examples of using the 2 templates with indexed first- and second-level indirection:
The variants with a getter function do not qualify for this. These can only optionally use second-level indirection.

```C++
bool succeeded{rsbd8::radixsortcopy<&myclass::member, rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::native, addressoffset2, true>(count, inputarr, outputarr, pagesizeoptional, indirectionindex1, indirectionindex2)};
bool succeeded{rsbd8::radixsortcopy<long double *, addressoffset1, rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::native, addressoffset2, true>(count, inputarr, outputarr, pagesizeoptional, indirectionindex1, indirectionindex2)};

bool succeeded{rsbd8::radixsort<&myclass::member, rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::native, addressoffset2, true>(count, inputarr, pagesizeoptional, indirectionindex1, indirectionindex2)};
bool succeeded{rsbd8::radixsort<short *, addressoffset1, rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::native, addressoffset2, true>(count, inputarr, pagesizeoptional, indirectionindex1, indirectionindex2)};
```

## The 4 main sorting template functions that are implemented here
### radixsortnoalloc():
- counted (first parameter "count", the end of arrays are no inputs to these functions unlike some sorting functions)
- limits to a set number of threads (second parameter "allowedthreads", unless multithreading is disabled)
- uses a memory buffer of (allowedthreads - 1) * sizeof(std::future<void>) with natural alignment to store the multithreading futures (third parameter "pfuturesplaceholder", unless multithreading is disabled)
- uses a memory buffer with cache line alignment to store the zeroed index counts, see rsbd8::allocatearray() for more information (fourth parameter "pzeroedindices")
- sorts an array (fifth parameter "inputarr")
- uses an array as a buffer of the same size and type (sixth parameter "bufferarr")
- with a toggle to output to either the input array or the buffer array (optional seventh parameter "movetobuffer")
- the array that is not selected for output contains garbage afterwards (typically the leftovers from an intermediate sorting stage)
- both arrays need to be writable, but when using indirection the members can be const-qualified
### radixsortcopynoalloc():
- counted (first parameter "count")
- limits to a set number of threads (second parameter "allowedthreads", unless multithreading is disabled)
- uses a memory buffer of (allowedthreads - 1) * sizeof(std::future<void>) with natural alignment to store the multithreading futures (third parameter "pfuturesplaceholder", unless multithreading is disabled)
- uses a memory buffer with cache line alignment to store the zeroed index counts, see rsbd8::allocatearray() for more information (fourth parameter "pzeroedindices")
- similar to radixsortnoalloc(), but will not write to the input array, which can be const-qualified (fourth parameter "inputarr")
- uses a dedicated output array of the same size and type (fifth parameter "outputarr")
- uses an array as a buffer of the same size and type (sixth parameter "bufferarr")
### radixsort():
- wrapper template for radixsortnoalloc()
- allocates memory for the temporary buffers and determines the amount of threads to use
- (Windows-only) large page size for VirtualAlloc() can be used if enabled with the lock memory privilege for the application enabled (optional third parameter)
- (POSIX implementing systems-only) flags for enabling pages with huge TLB functionality for mmap() can be used (optional third parameter)
### radixsortcopy():
- wrapper template for radixsortcopynoalloc()
- allocates memory for the temporary buffers and determines the amount of threads to use
- (Windows-only) large page size for VirtualAlloc() can be used if enabled with the lock memory privilege for the application enabled (optional third parameter)
- (POSIX implementing systems-only) flags for enabling pages with huge TLB functionality for mmap() can be used (optional third parameter)

## Modes of operation for the template functions
All sorting functions here are templates with a compile-time constant sorting mode and direction.
enum struct sortingmode; 5 bits as bitfields, bit 6 is used to select automatic modes (64 and greater)
### The three generic modes that can be activated are:
#### native
- automatic unsigned integer, signed integer or floating-point, depending on input type (default)
#### nativeabs
- automatic unsigned integer, absolute signed integer or absolute floating-point, depending on input type
- (no distinct effect when used on an unsigned integer input type)
#### nativetieredabs
- automatic unsigned integer, absolute signed integer or absolute floating-point, depending on input type, but negative inputs will sort just below their positive counterparts
- (no distinct effect when used on an unsigned integer input type)

### The five regular modes that can be activated are:
#### forceunsigned
- regular unsigned integer (default for unsigned input types)
#### specialsigned
- and also inside-out signed integer
- (sorts ascending from 0, maximum value, minimum value, to -1)
#### forcesigned
- regular signed integer (default for signed input types)
#### forceabssigned
- absolute signed integer:
#### forcefloatingp
- regular floating-point (default for floating-point input types)
#### forceabsfloatingp
- absolute floating-point
#### specialunsigned
- and also unsigned integer without using the top bit

### The three special modes that can be activated are:
#### specialfloatingp
- inside-out floating-point
- (sorts ascending from +0., +infinity, +NaN, -NaN, -infinity, to -0.)
#### forcetieredabsfloatingp
- absolute floating-point, but negative inputs will sort just below their positive counterparts
- (sorts ascending from -0., +0., -infinity, +infinity, to various -NaN or +NaN values at the end)
#### forcetieredabssigned
- absolute signed integer, but negative inputs will sort just below their positive counterparts
- (sorts ascending from 0, -1, 1, -2, 2, and so on, will work correctly for minimum values)

### The two reversing modes are:
enum struct sortingdirection; 2 bits as bitfields
- isdescsort (default false): reverse the sorting direction
- isrevorder (default false): reverse the array direction when sorting items with the same value (only used when dealing with indirection)
Enabling isdescsort costs next to nothing in terms of performance, isrevorder does initially take minor extra processing when handling multi-part types.
#### ascfwdorder
- isdescsort = false, isrevorder = false: stable sort, low to high (default)
#### dscrevorder
- isdescsort = true, isrevorder = true: stable sort, high to low, the complete opposite direction of the default functionality
#### dscfwdorder
- isdescsort = true, isrevorder = false: stable sort, high to low, but keeps items with the same value in the same order as in the source
#### ascrevorder
- isdescsort = false, isrevorder = false: stable sort, low to high, but reverses items of the same value compared to the order in the source
This last combination is very uncommon, but could be useful in some rare cases.

### To give an example of isdescsort = true, isrevorder = false, as it's a bit tricky to imagine without a reference:
```C++
myclass collA[]{{1, "first"}, {1, "second"}, {-5, "third"}, {2, "fourth"}};// list construct
myclass *pcollA[]{collA, collA + 1, collA + 2, collA + 3};// list pointers
bool succeeded{rsbd8::radixsort<&myclass::keyorder, rsbd8::sortingdirection::dscfwdorder>(4, pcollA)};
```
Members of "pcollA" will then get sorted according to their value "keyorder", in reverse order, while keeping the same array order.
Pointers will in this case point to: {2, "fourth"}, {1, "first"}, {1, "second"}, {-5, "third"}.
That is different from fully reversing the order when using this line instead of the above:
```C++
bool succeeded{rsbd8::radixsort<&myclass::keyorder, rsbd8::dscrevorder>(4, pcollA)};
```
Pointers will in this case point to: {2, "fourth"}, {1, "second"}, {1, "first"}, {-5, "third"}.
Notice the same reverse stable sorting here, but opposite placement when encountering the same value multiple times.

## Miscellaneous notes
### Architectural matters
Incompatibility: on the x64/x86-64/AMD64/EM64T platform minimal Prefetchw feature support by the CPU is unconditionally required for this library. This is stated in the comments as:
prefetchw can be assumed to be supported on x64 (even though not guaranteed because of rare Intel Pentium D 800 series processors from 2005), so this is only enabled for x64 and not for 32-bit x86
Currently in 2026, a few, very niche operating systems for the x64 platform will run without support for this feature, but these simply do not align with the usage case of this library.
These operating systems are for legacy support with specifically this hardware, and are unlikely to run any new software. Reminder, this is a C++17 and onwards feature set library to begin with.
This library is on top of that focussed on high-performance computing, somewhat larger memory footprints, and multithreading targets often beyond the scope of these dual-core machines.
Incompatibility: on the x86-32 platform SSE feature support by the CPU is unconditionally required for this library. SSE is available since the 1999 Intel Pentium III and 2001 AMD Athlon XP era.
For a similar reason as on x64, prefetcht0 is expected to be available to do basic prefetching for reading data from and possibly writing data to a soon to be used cache line.
### Design matters
Sorting unfiltered integer values is the fastest, followed up by floating-point values in this library.
Filtering costs vary per item, see "Utilities to establish the tresholds for 2-, 4-, 6-, 8-, 12- and 18-way multithreading" for some insights into that.
For 64-bit and larger systems complete 128-bit integer and floating-point support is available in this library, whether or not such types exist in the environment.
Unsigned 128-bit and larger integers can be sorted by sequential sorting from the bottom to the top parts as unsigned (64-bit) elements when using indirection.
Signed 128-bit and larger integers are sorted the same, with only the topmost (64-bit) element sorted as signed because of the sign bit. This does not work when filtered by the absolute modes.
Regular absolute 128-bit and larger floating-point values are sorted the same, with only the topmost (64-bit) element sorted as absolute floating-point because of the sign bit. This does not work when filtered by the default or tiered absolute modes.
Re-use the same intermediate buffer combined with radixsortnoalloc() or radixsortcopynoalloc() when sorting 128-bit and larger integers like this.
Inputs of type bool are reinterpreted as the unsigned integer type of the same size (usually the unsigned char type), but handling them is extremely efficient anyway.
Anything but 0 or 1 in bool source data will impact sorting, but this only happens if the user deliberately overrides the compiler behaviour for bool data.
The sign of type char is ambiguous, as by the original C standard, so cast inputs to char8_t * or unsigned char *, or force unsigned processing modes if unambiguously a binary sort of char characters is desired.
Floating-point NaN values are sorted before negative infinity for the typical machine-generated "undefined" QNaN (0xFFF8'0000'0000'0000 on an IEEE double).
Floating-point NaN positive values (implies not machine-generated) are sorted after positive infinity (0x7FF0'0000'0000'0001 and onward on an IEEE double).
Floating-point SNaN (signalling) values do not trigger signals inside these functions. This is similar to many other non-arithmetic functions in namespace std.

## Naming and tooling conventions used in this library
### Textual:
The base language for this library is modern British English (en-GB), but purely imported names may of course be sourced differently.
All names are lowercase, with no separators, and any sort of length, based on convenience or frequency of local usage.
Any sort of names on a global or namespace scope are longer, to provide some context.
All user-facing functions have a general description in a comment in the line above them. This is generally what any IDE will display when giving a tooltip.
The few macro definitions are all uppercase, separated by underscores, starting with "RSBD8_".
There are most certainly no limitations on the lengths of functions, lines, comment blocks or other items. Everything is just kept sensible, in proper order, never spaghettified or obscured much, feature-rich (even if a little complicated sometimes), and very much optimised.
This library undefines its macro definitions at the end of the file and does not need to expose any macro definitions externally.
This library has three imported code sections that are clearly marked at the beginning and end. These are edited sparingly, unless they get replaced again with a newer version.
### Namespaces:
rsbd8:: is radixsortbidi8, the enveloping namespace for this library
rsbd8::helper:: is the underlying namespace for helper functions and constants. These are usually not directly invoked by the user of this library. No efforts are made to actually hide items from the user though.
### Templates:
```C++
<typename T, typename U, typename V, typename W, typename X, typename M>
```
These are the basic placeholders for types.
T is used for the main input type, or the primary type being referred to.
U is the deferred type, for example the unsigned variant of T, or an unsigned general utility type that is larger than T.
V is always used as the class type of input and output arrays when dealing with indirection.
W is the wildcard type, often an automatically deduced item in templated helper functions, but also often just a companion type to U.
X is the index type, used for assigning unsigned integer indices of various sizes.
M is the mask type, used in the merging phase of 4-way and greater multithreaded sorting.
Some other template parts have defaults set up for them, especially for the longer lists of template parameters. This provides the user with often having less verbosity in their code.
Template variable arguments as a C++ feature are used extensively by this library. Function-based variable arguments passing as inherited from original C isn't needed.
All sorts of levels and configurations of template metaprogramming (C++17 and onward) are used in this library for optimisation, enhancing debugging or just making the code more concise.
### Functional:
This is an optimal performance library. Of course, minor performance setbacks may arise from just compiler interpretation or functional issues. Keep the code close to the bare metal, in the right order to easily compile to instructions and use tons of optimisation features to overcome such issues.
Even if many parts here just don't comply with most of the programming world's "clean" code rules, performance is key first, reducing the total count of functions but balanced with maintainability is second, and being descriptive of functionality in a combination of code and comments is third.
Defensive programming here is mostly left to template metaprogramming and compile-time assertions, as compile-time items don't hurt performance.
Parameter and environment checking is left to the absolute minimum outside of the debug mode. Of course, memory allocation failures are handled, and likewise the cases of potentially throwing functions.

## Notes on ongoing research and development

### TODO, add support for types larger than 128 bits
- TODO, currently all functions here are guarded with an upper limit of 8-, 64-, 80-, 96- or 128-bit (using std::enable_if sections). Future functionality will either require lifting the limits on current functions, or adding another set of functions for the larger data types. Given that radix sort variants excel at processing large data types compared to comparison-based sorting methods, do give this some priority in development.

### TODO, document computer system architecture-dependent code parts and add more options
- TODO, add support for native 128-bit for architectures that support these. (meaning: a system with 128 bits in single general-purpose registers)
- TODO, investigate SIMD, in all of its shapes and sizes. Some experimentation has been done with x64+AVX-512 in an early version, but compared to other optimisations and strategies it never yielded much for these test functions.
- TODO, add more platform-dependent, optimised code sequences here similar to the current collections in the rsbd8::helper namespace.
- TODO, test and debug this library on more machines, platforms and such. Functionality and performance should both be guaranteed.

### TODO, this is a C++17 minimum library, but more modern features are welcome
- TODO, bumping up the library to C++20 minimum isn't advised (yet), but could be in the near future.
- TODO, C++23 features currently don't add much over some of the improvements seen in C++20, but for example indexed varargs from C++26 could certainly provide some simplification in a few functions here. Adding more modern C++ features (even if as optional items for now) is welcome.

### TODO, add support for non-array inputs
- TODO, as the basic std::sort and std::stable_sort variants already support this functionality, it could be an advantage to add support for the other C++ iterable data sets.
- TODO, performance testing and use case investigation is required for this subject, as radix sort types only really work well on somewhat larger arrays, and probably other larger iterable data sets, too.

## Extended filtering information for each of the 8 main modes

### Modes of operation for the template functions
#### regular unsigned integer "forceunsigned" (default for unsigned input types)
#### and also inside-out signed integer "specialsigned"
- (sorts ascending from 0, maximum value, minimum value, to -1):
- isabsvalue = false, issignmode = false, isfltpmode = false
- lowest amount of filtering cost
- straightforward process, no filter at all
#### regular signed integer "forcesigned" (default for signed input types):
- isabsvalue = false, issignmode = true, isfltpmode = false
- lower amount of filtering cost
- no filter in the processing phases
- virtually flips the most significant bit when calculating offsets
#### absolute signed integer "forceabssigned":
- isabsvalue = true, issignmode = true, isfltpmode = false
- medium amount of filtering cost
- creates a sign bit mask, adds it to the input and uses it with xor on the input as a filter in the processing phases
#### regular floating-point "forcefloatingp" (default for floating-point input types):
- isabsvalue = false, issignmode = true, isfltpmode = true
- higher amount of filtering cost
- creates a sign bit mask and uses it with xor on the exponent and mantissa bits as a filter in the processing phases
- virtually flips the most significant bit when calculating offsets
#### absolute floating-point "forceabsfloatingp"
#### and also unsigned integer without using the top bit "specialunsigned":
- isabsvalue = true, issignmode = true, isfltpmode = true
- low amount of filtering cost
- masks out the sign bit in the processing phases

The three special modes that can be activated are:
#### inside-out floating-point "specialfloatingp"
#### (sorts ascending from +0., +infinity, +NaN, -NaN, -infinity, to -0.):
- isabsvalue = false, issignmode = false, isfltpmode = true
- higher amount of filtering cost
- creates a sign bit mask and uses it with xor on the exponent and mantissa bits as a filter in the processing phases
#### absolute floating-point, but negative inputs will sort just below their positive counterparts "forcetieredabsfloatingp"
#### (sorts ascending from -0., +0., -infinity, +infinity, to various -NaN or +NaN values at the end):
- isabsvalue = true, issignmode = false, isfltpmode = true
- medium amount of filtering cost
- bit rotates left by one to move the sign bit to the least significant bit in the processing phases
- virtually flips the least significant bit when calculating offsets
#### absolute signed integer, but negative inputs will sort just below their positive counterparts "forcetieredabssigned"
#### (sorts ascending from 0, -1, 1, -2, 2, and so on, will work correctly for minimum values):
- isabsvalue = true, issignmode = false, isfltpmode = false
- medium amount of filtering cost
- creates a sign bit mask, shifts the input left by one and uses xor on the input with the sign bit mask as a filter in the processing phases

### Enabling reverse ordering on these modes will add slightly more to the initial filtering cost.
For example, the highest amount of filtering cost would be on the floating-point full reverse mode.
Take "highest amount" with a grain of salt though, as way more complicated filtering stages can be designed than what is used for this combination of filters.

### Absolute floating-point, but negative inputs will sort just below their positive counterparts operates differently.
Bit rotate left by one is the first filtering step to make this possible.
Sort as unsigned, just with the least significant bit flipped to complete the filter.
#### As an example of this, the 8-bit sorting pattern in ascending mode:
```C++
0b1000'0000 -0.
0b0000'0000 +0.
0b1000'0001 -exp2(1 - mantissabitcount - exponentbias)
0b0000'0001 +exp2(1 - mantissabitcount - exponentbias)
...
0b1111'1111 -QNaN (maximum amount of ones)
0b0111'1111 +QNaN (maximum amount of ones)
```

## Performance tests
### This library has a performance test suite, and these tests were done with the CPU set to a steady 3.5 GHz on all cores, no throttling, no boosting for a fair 1:1 clock-per-clock comparison.
These performance test results are for multithreaded sorting on an input array of 8 GiB with random bits (with no indirection, and NaN/infinity values filtered out before the tests).
#### std::stable_sort() for floating-point and std::sort() for integers (as the function performs a faster stable sort in these cases) vs rsbd8::radixsort(), measured in 10 ns units:
```
- float : 222988022710 vs 20651487317, a factor of 10.80 in speedup
- double: 119978839926 vs 24076554843, a factor of 4.983 in speedup
- uint8 : 331191844635 vs_ 2205425221, a factor of 150.2 in speedup (comparison-based sort vs in essence a bucket sort)
- int8 _: 320164203051 vs_ 2177899202, a factor of 147.0 in speedup (comparison-based sort vs in essence a bucket sort)
- uint16: 184199968006 vs 14765943780, a factor of 12.47 in speedup
- int16 : 178957686478 vs 13933607336, a factor of 12.84 in speedup
- uint32: 119668335381 vs 19256567554, a factor of 6.214 in speedup
- int32 : 123236987453 vs 19762451250, a factor of 6.236 in speedup
- uint64:_ 57783520758 vs 23909100957, a factor of 2.417 in speedup
- int64 :_ 62324902036 vs 22796669527, a factor of 2.734 in speedup
```
Sorting with indirection on these radix sort functions, for the 64-bit types (same counts, same size, averaged) is 3.076 times slower than sorting without indirection.
#### First-level indirection std::stable_sort() vs rsbd8::radixsort(), measured in 10 ns units:
```
- float : 202471075829 vs 41583734978, a factor of 4.869 in speedup
- double: 231627536796 vs 74058455130, a factor of 3.128 in speedup
- uint8 : 120691616750 vs_ 6901998908, a factor of 17.49 in speedup (comparison-based sort vs in essence a bucket sort)
- uint16: 156500844720 vs 23766698391, a factor of 6.585 in speedup
- uint32: 198336155800 vs 40858558450, a factor of 4.854 in speedup
- uint64: 227346382912 vs 73565119718, a factor of 3.090 in speedup
```

These performance test results are for multithreaded sorting on an input array of 8 MiB with random bits (with no indirection, and NaN/infinity values filtered out before the tests).
#### std::stable_sort() for floating-point and std::sort() for integers (as the function performs a faster stable sort in these cases) vs rsbd8::radixsort(), measured in 10 ns units:
```
- float : 160558080 vs 35855927, a factor of 4.478 in speedup
- double:_ 77908610 vs 36794499, a factor of 2.117 in speedup
- uint8 : 312606500 vs_ 3904509, a factor of 80.06 in speedup (comparison-based sort vs in essence a bucket sort)
- int8 _: 304056536 vs_ 3648098, a factor of 83.35 in speedup (comparison-based sort vs in essence a bucket sort)
- uint16: 186612304 vs 24020259, a factor of 7.769 in speedup
- int16 : 177857284 vs 20839855, a factor of 8.534 in speedup
- uint32: 105141053 vs 40109296, a factor of 2.621 in speedup
- int32 : 101571298 vs 34822936, a factor of 2.917 in speedup
- uint64:_ 51337829 vs 26669600, a factor of 1.925 in speedup
- int64 :_ 49785499 vs 34822266, a factor of 1.430 in speedup
```
Sorting with indirection on these radix sort functions, for the 64-bit types (same counts, same size, averaged) is 1.374 times slower than sorting without indirection.
#### First-level indirection std::stable_sort() vs rsbd8::radixsort(), measured in 10 ns units:
```
- float :_ 95270892 vs 27683502, a factor of 3.441 in speedup
- double: 102873128 vs 43189334, a factor of 2.382 in speedup
- uint8 :_ 49337614 vs_ 6572477, a factor of 7.507 in speedup (comparison-based sort vs in essence a bucket sort)
- uint16:_ 74731070 vs 13231753, a factor of 5.648 in speedup
- uint32:_ 92453151 vs 26075116, a factor of 3.546 in speedup
- uint64:_ 95064426 vs 44039973, a factor of 2.159 in speedup
```

### This library has a performance test suite, and these tests were done with the CPU set to its regular performance mode, so the effects of throttling, boosting and all kinds of potential variance are present in each comparison.
This way of testing takes out the 1:1 clock-per-clock comparison, but does bring the tests closer to the real-world usage case.
#### std::stable_sort() for floating-point and std::sort() for integers (as the function performs a faster stable sort in these cases) vs rsbd8::radixsort(), measured in 10 ns units:
```
- float : 170753842640 vs 16546584080, a factor of 10.32 in speedup
- double:_ 87570922046 vs 21106226384, a factor of 4.149 in speedup
- uint8 : 229274088931 vs_ 1703359326, a factor of 134.6 in speedup (comparison-based sort vs in essence a bucket sort)
- int8 _: 229115806108 vs_ 1665073620, a factor of 137.6 in speedup (comparison-based sort vs in essence a bucket sort)
- uint16: 140354781152 vs 11361919492, a factor of 12.35 in speedup
- int16 : 126370557266 vs 11360401626, a factor of 11.12 in speedup
- uint32:_ 80523980797 vs 15323123626, a factor of 5.255 in speedup
- int32 :_ 80004706164 vs 16886012874, a factor of 4.738 in speedup
- uint64:_ 42659822611 vs 21355332813, a factor of 1.998 in speedup
- int64 :_ 42921349117 vs 20743735519, a factor of 2.069 in speedup
```
Sorting with indirection on these radix sort functions, for the 64-bit types (same counts, same size, averaged) is 3.456 times slower than sorting without indirection.
#### First-level indirection std::stable_sort() vs rsbd8::radixsort(), measured in 10 ns units:
```
- float : 183079629954 vs 40739923084, a factor of 4.494 in speedup
- double: 214263258181 vs 73820395030, a factor of 2.902 in speedup
- uint8 : 106478668842 vs_ 5609448537, a factor of 18.98 in speedup (comparison-based sort vs in essence a bucket sort)
- uint16: 139490364100 vs 20558607294, a factor of 6.785 in speedup
- uint32: 181590769406 vs 40427557552, a factor of 4.492 in speedup
- uint64: 211530441704 vs 72918226793, a factor of 2.901 in speedup
```

These performance test results are for multithreaded sorting on an input array of 8 MiB with random bits (with no indirection, and NaN/infinity values filtered out before the tests).
#### std::stable_sort() for floating-point and std::sort() for integers (as the function performs a faster stable sort in these cases) vs rsbd8::radixsort(), measured in 10 ns units:
```
- float : 106913763 vs 25861890, a factor of 4.134 in speedup
- double:_ 54096962 vs 19678736, a factor of 2.749 in speedup
- uint8 : 215003526 vs_ 3471186, a factor of 61.94 in speedup (comparison-based sort vs in essence a bucket sort)
- int8 _: 211433096 vs_ 2985172, a factor of 70.83 in speedup (comparison-based sort vs in essence a bucket sort)
- uint16: 128435392 vs 14855975, a factor of 8.645 in speedup
- int16 : 124225020 vs 16348204, a factor of 7.599 in speedup
- uint32:_ 74048523 vs 26781122, a factor of 2.765 in speedup
- int32 :_ 69288318 vs 26345511, a factor of 2.630 in speedup
- uint64:_ 35831585 vs 21972964, a factor of 1.631 in speedup
- int64 :_ 34120735 vs 21524322, a factor of 1.585 in speedup
```
Sorting with indirection on these radix sort functions, for the 64-bit types (same counts, same size, averaged) is 1.524 times slower than sorting without indirection.
#### First-level indirection std::stable_sort() vs rsbd8::radixsort(), measured in 10 ns units:
```
- float : 65347030 vs 20796397, a factor of 3.142 in speedup
- double: 70397421 vs 33465004, a factor of 2.104 in speedup
- uint8 : 35950738 vs_ 5536212, a factor of 6.494 in speedup (comparison-based sort vs in essence a bucket sort)
- uint16: 50941047 vs 10208071, a factor of 4.990 in speedup
- uint32: 63085420 vs 20948052, a factor of 3.012 in speedup
- uint64: 66224310 vs 30032136, a factor of 2.205 in speedup
```

### The next tests were done on smaller blocks.
There will be a minimum amount of array entries where rsbd8::radixsort() starts to get the upper hand in speed over std::stable_sort().
#### These test results were obtained by single-threaded performance testing on multiple sizes of blocks between .5 to 512 KiB, with fully random bits in unsigned integer and floating-point arrays (with no indirection):
- float : 354 array entries
- double: 426 array entries
- uint8 :_ 19 array entries
- uint16: 349 array entries
- uint32: 523 array entries
- uint64: 557 array entries
Interpreting this means that radix sort variants will be faster for somewhat larger arrays when sorting data under the given conditions.
In this case that's a sequence of just plain values in an array.
When dealing with sorting while using indirection or more filtering, test results will vary.

### System configuration data of the last performance tests:
#### The main performance tests were done on 2026-08-27 on development PC 1:
- Intel Core i9 11900K, specification string: 11th Gen Intel Core i9-11900K @ 3.50GHz
- Corsair CMK16GX4M2B3200C16 * 2, 32 GiB, 1600 MHz (XMP-3200 scheme), DDR4
- ASRock Z590 PG Velocita, UEFI version 2.01 beta
- Windows 11 Home Single Language, (25H2) build 10.0.26100.9168
- Microsoft Visual Studio 2026 (Community edition) Insiders build 12120.281, using the preview build tools and Visual C++ 2026 compiler
- In the first set of benchmarks the CPU was locked to run at 3.5 GHz on all cores in the UEFI, without boosts or throttling during testing.
- In the second set of benchmarks the CPU was allowed all throttling and boosting options in the UEFI, which is the regular, everyday performance mode.
- Some background programs were disabled, and the main testing was done by just running the test executable and letting DebugView x64 do the readout
- DebugView is also useful at keeping a record on any outputs generated by other simultaneous processes to analyse some possible disturbances. If any such disturbances were detected, the test run was discarded and re-done.