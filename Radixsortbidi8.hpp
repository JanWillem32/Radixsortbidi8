#pragma once
// MIT License
// Copyright (c) 2025 Jan-Willem Krans (janwillem32 <at> hotmail <dot> com)
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// Radixsortbidi8
// This library implements an efficient stable sort on arrays using an 8-bit indexed, bidirectional, least significant bit first radix sort method.
// This is currently a single-file library, with some additional folders and files only used for its test suite.
// Sorting functionality is available for unsigned integer, signed integer, floating-point and enumeration types.
// All these sorting functions can sort forwards and reverse, order forwards and reverse, and optionally filter by absolute value.
// Several filters are available, such as two types of absolute, and an inverse pattern for signed integer and floating point types.
// See "Modes of operation for the template functions" for more details on that.
// Implemented function optimisations include the ability to skip sorting steps, using parallel (bidirectional) indexing and copying while sorting, and usage of platform-specific intrinsic functions.
// Radix sort in general can be used to sort all array sizes, but is more efficient when applied to somewhat larger arrays compared to other efficient (and stable) comparison-based methods, like introsort.
// See "Performance tests" for more details about array sizes, types and achievable improvements in speed when sorting large arrays with this library.

// Examples of using the 4 templates with simple arrays as input (automatically deduced template parameters are omitted here):
// The rsbd8::radixsort() and rsbd8::radixsortcopy() template wrapper functions (typically) merely allocate memory prior to using the actual sorting functions.
// No intermediate buffer array is required when any variant of radixsortcopynoalloc() is used for sorting 8-bit types.
//
// bool succeeded{rsbd8::radixsort(count, inputarr, pagesizeoptional)};
// bool succeeded{rsbd8::radixsortcopy(count, inputarr, outputarr, pagesizeoptional)};
// rsbd8::radixsortnoalloc(count, inputarr, bufferarr, false);
// rsbd8::radixsortcopynoalloc(count, inputarr, outputarr, bufferarr);

// Examples of using the 4 templates with input from first-level indirection (automatically deduced template parameters are omitted here):
// The address offset template parameters displace the pointers they act on like a flat "std::byte const *", so not as some sort of array indices.
// For the more advanced use cases, an extra argument (variadic function argument) can be added to index the indirection when dealing with a (member) pointer/array.
// The variant with a getter function allows any number of extra arguments to pass on to the getter function.
// Another advanced use case can be set by a macro, RSBD8_DISABLE_ADDRESS_SUBDIVISION.
// This disables a compile-time optimisation if preferred, for example when benchmarking several indirection modes.
//
// bool succeeded{rsbd8::radixsort<&myclass::getterfunc>(count, inputarr, pagesizeoptional)};
// bool succeeded{rsbd8::radixsort<&myclass::member>(count, inputarr, pagesizeoptional)};
// bool succeeded{rsbd8::radixsort<uint64_t, addressoffset>(count, inputarr, pagesizeoptional)};
//
// bool succeeded{rsbd8::radixsortcopy<&myclass::getterfunc>(count, inputarr, outputarr, pagesizeoptional, getterparameters...)};
// bool succeeded{rsbd8::radixsortcopy<&myclass::member>(count, inputarr, outputarr, pagesizeoptional)};
// bool succeeded{rsbd8::radixsortcopy<bool, addressoffset>(count, inputarr, outputarr, pagesizeoptional)};
//
// rsbd8::radixsortnoalloc<&myclass::getterfunc>(count, inputarr, bufferarr, false, getterparameters...);
// rsbd8::radixsortnoalloc<&myclass::member>(count, inputarr, bufferarr, false);
// rsbd8::radixsortnoalloc<int16_t, addressoffset>(count, inputarr, bufferarr, false);
//
// rsbd8::radixsortcopynoalloc<&myclass::getterfunc>(count, inputarr, outputarr, bufferarr, getterparameters...);
// rsbd8::radixsortcopynoalloc<&myclass::member>(count, inputarr, outputarr, bufferarr);
// rsbd8::radixsortcopynoalloc<float, addressoffset>(count, inputarr, outputarr, bufferarr);
//
// There are only 4 template functions that almost directly implement sorting with indirection here:
// = rsbd8::radixsortcopynoalloc()
// = rsbd8::radixsortnoalloc()
// These both have an 8-bit type and a minimum 16-bit type template function.
// These 4 are also the only template functions where no attempt has been made to (force) inline them.
// All other template functions are inlined wrapper template functions for these 4.
// The user of this library can expand on the base functionality by utilising the tools:
// = rsbd8::GetOffsetOf
// = rsbd8::allocatearray()
// = rsbd8::deallocatearray()
// This library only includes the common use scenarios to deal with input from first- and second-level indirection.

// Examples of using the 4 templates with input from second-level indirection (automatically deduced template parameters are omitted here):
// As the use case for these almost always involve multi-pass techniques, the user is advised to allocate the (reusable) buffers accordingly and avoid the use of radixsortcopy() and radixsort().
// The rsbd8::allocatearray() and rsbd8::deallocatearray() inline function templates are provided for handling an intermediate buffer.
// Again, no intermediate buffer is required when radixsortcopynoalloc() is used for sorting a single-part type.
// These will internally first retrieve a pointer to a "T" type array "T *myarray".
// After that it's dereferenced at the origin (first set of examples) or indexed (second set of examples) as "myarray[indirectionindex]" to retrieve the value used for sorting.
// Again, all "addressoffset" variants as template inputs displace like on a flat "std::byte const *", so not as some sort of array indices.
// The "addressoffset1" item here displaces the pointer in the input array to get the secondary pointer.
// The "addressoffset2" item here displaces the secondary pointer to get the (unfiltered) sorting value.
//
// Examples of using the 2 templates with no indexed second-level indirection:
//
// rsbd8::radixsortcopynoalloc<&myclass::getterfunc, rsbd8::ascendingforwardordered, rsbd8::nativemode, addressoffset2>(count, inputarr, outputarr, bufferarr, getterparameters...);
// rsbd8::radixsortcopynoalloc<&myclass::member, rsbd8::ascendingforwardordered, rsbd8::nativemode, addressoffset2>(count, inputarr, outputarr, bufferarr);
// rsbd8::radixsortcopynoalloc<long *, addressoffset1, rsbd8::ascendingforwardordered, rsbd8::nativemode, addressoffset2>(count, inputarr, outputarr, bufferarr);
//
// rsbd8::radixsortnoalloc<&myclass::getterfunc, rsbd8::ascendingforwardordered, rsbd8::nativemode, addressoffset2>(count, inputarr, bufferarr, false, getterparameters...);
// rsbd8::radixsortnoalloc<&myclass::member, rsbd8::ascendingforwardordered, rsbd8::nativemode, addressoffset2>(count, inputarr, bufferarr, false);
// rsbd8::radixsortnoalloc<wchar_t *, addressoffset1, rsbd8::ascendingforwardordered, rsbd8::nativemode, addressoffset2>(count, inputarr, bufferarr, false);
//
// Examples of using the 2 templates with indexed second-level indirection:
//
// rsbd8::radixsortcopynoalloc<&myclass::getterfunc, rsbd8::ascendingforwardordered, rsbd8::nativemode, addressoffset2, true>(count, inputarr, outputarr, bufferarr, indirectionindex, getterparameters...);
// rsbd8::radixsortcopynoalloc<&myclass::member, rsbd8::ascendingforwardordered, rsbd8::nativemode, addressoffset2, true>(count, inputarr, outputarr, bufferarr, indirectionindex);
// rsbd8::radixsortcopynoalloc<double *, addressoffset1, rsbd8::ascendingforwardordered, rsbd8::nativemode, addressoffset2, true>(count, inputarr, outputarr, bufferarr, indirectionindex);
//
// rsbd8::radixsortnoalloc<&myclass::getterfunc, rsbd8::ascendingforwardordered, rsbd8::nativemode, addressoffset2, true>(count, inputarr, bufferarr, false, indirectionindex, getterparameters...);
// rsbd8::radixsortnoalloc<&myclass::member, rsbd8::ascendingforwardordered, rsbd8::nativemode, addressoffset2, true>(count, inputarr, bufferarr, false, indirectionindex);
// rsbd8::radixsortnoalloc<unsigned long *, addressoffset1, rsbd8::ascendingforwardordered, rsbd8::nativemode, addressoffset2, true>(count, inputarr, bufferarr, false, indirectionindex);
//
// Examples of using the 2 templates with indexed first- and second-level indirection:
//
// rsbd8::radixsortcopynoalloc<&myclass::member, rsbd8::ascendingforwardordered, rsbd8::nativemode, addressoffset2, true>(count, inputarr, outputarr, bufferarr, indirectionindex1, indirectionindex2);
// rsbd8::radixsortcopynoalloc<long double *, addressoffset1, rsbd8::ascendingforwardordered, rsbd8::nativemode, addressoffset2, true>(count, inputarr, outputarr, bufferarr, indirectionindex1, indirectionindex2);
//
// rsbd8::radixsortnoalloc<&myclass::member, rsbd8::ascendingforwardordered, rsbd8::nativemode, addressoffset2, true>(count, inputarr, bufferarr, false, indirectionindex1, indirectionindex2);
// rsbd8::radixsortnoalloc<short *, addressoffset1, rsbd8::ascendingforwardordered, rsbd8::nativemode, addressoffset2, true>(count, inputarr, bufferarr, false, indirectionindex1, indirectionindex2);

// The 4 main sorting template functions that are implemented here
// = radixsortnoalloc():
// --- counted (first parameter "count", the end of arrays are no inputs to these functions unlike some sorting functions)
// --- sorts an array (second parameter "input")
// --- uses an array as a buffer of the same size and type (third parameter "buffer")
// --- with a toggle to output to either the input array or the buffer array (optional fourth parameter "movetobuffer")
// --- the array that is not selected for output contains garbage afterwards (typically the leftovers from an intermediate sorting stage)
// --- both arrays need to be writable, but when using indirection the members can be const-qualified
// = radixsortcopynoalloc():
// --- counted (first parameter "count")
// --- similar to radixsortnoalloc(), but will not write to the input array, which can be const-qualified (second parameter "input")
// --- uses a dedicated output array of the same size (third parameter "output")
// --- uses a memory buffer of the same size, which contains garbage afterwards (fourth parameter)
// = radixsort():
// --- wrapper template for radixsortnoalloc()
// --- only allocates memory for the buffer parameter
// --- (Windows-only) large page size can be used if enabled with the lock memory privilege for the application enabled (optional third parameter)
// --- (POSIX implementing systems-only) flags for enabling pages with huge TLB functionality can be used (optional third parameter)
// = radixsortcopy():
// --- wrapper template for radixsortcopynoalloc()
// --- only allocates memory for the buffer parameter
// --- (Windows-only) large page size can be used if enabled with the lock memory privilege for the application enabled (optional third parameter)
// --- (POSIX implementing systems-only) flags for enabling pages with huge TLB functionality can be used (optional third parameter)

// Modes of operation for the template functions
namespace rsbd8{
// All sorting functions here are templates with a compile-time constant sorting mode and direction.
enum sortingmode : unsigned char{// 5 bits as bitfields, bit 6 is used to select automatic modes (64 and greater)
// The three generic modes that can be activated are:
	nativemode = 64,
// = automatic unsigned integer, signed integer or floating-point, depending on input type (default)
	nativeabsmode = 65,
// = automatic unsigned integer, absolute signed integer or absolute floating-point, depending on input type
// - (no distinct effect when used on an unsigned integer input type)
	nativetieredabsmode = 66,
// = automatic unsigned integer, absolute signed integer or absolute floating-point, depending on input type, but negative inputs will sort just below their positive counterparts
// - (no distinct effect when used on an unsigned integer input type)
//
// The five regular modes that can be activated are:
	forceunsignedmode = 0,
// = regular unsigned integer (default for unsigned input types)
	specialsignedmode = forceunsignedmode,
// - and also inside-out signed integer
// - (sorts ascending from 0, maximum value, minimum value, to -1)
	forcesignedmode = 1 << 1,
// = regular signed integer (default for signed input types)
	forceabssignedmode = 1 | 1 << 1,
// = absolute signed integer:
	forcefloatingpmode = 1 << 1 | 1 << 2,
// = regular floating-point (default for floating-point input types)
	forceabsfloatingpmode = 1 | 1 << 1 | 1 << 2,
// = absolute floating-point
	specialunsignedmode = forceabsfloatingpmode,
// - and also unsigned integer without using the top bit
//
// The three special modes that can be activated are:
	specialfloatingpmode = 1 << 2,
// = inside-out floating-point
// - (sorts ascending from +0., +infinity, +NaN, -NaN, -infinity, to -0.)
	forcetieredabsfloatingpmode = 1 | 1 << 2,
// = absolute floating-point, but negative inputs will sort just below their positive counterparts
// - (sorts ascending from -0., +0., -infinity, +infinity, to various -NaN or +NaN values at the end)
	forcetieredabssignedmode = 1
// = absolute signed integer, but negative inputs will sort just below their positive counterparts
// - (sorts ascending from 0, -1, 1, -2, 2, and so on, will work correctly for minimum values)
};
// The two reversing modes are:
enum sortingdirection : unsigned char{// 2 bits as bitfields
// = reversesort (default false): reverse the sorting direction
// = reverseorder (default false): reverse the array direction when sorting items with the same value (only used when dealing with indirection)
// Enabling reversesort costs next to nothing in terms of performance, reverseorder does initially take minor extra processing when handling multi-part types.
	ascendingforwardordered = 0,
// = reversesort = false, reverseorder = false: stable sort, low to high (default)
	decendingreverseordered = 1 | 1 << 1,
// = reversesort = true, reverseorder = true: stable sort, high to low, the complete opposite direction of the default functionality
	decendingforwardordered = 1,
// = reversesort = true, reverseorder = false: stable sort, high to low, but keeps items with the same value in the same order as in the source
	ascendingreverseordered = 1 << 1
// = reversesort = false, reverseorder = false: stable sort, low to high, but reverses items of the same value compared to the order in the source
// This last combination is very uncommon, but could be useful in some rare cases.
};
// To give an example of reversesort = true, reverseorder = false, as it's a bit tricky to imagine without a reference:
// myclass collA[]{{1, "first"}, {1, "second"}, {-5, "third"}, {2, "fourth"}};// list construct
// myclass *pcollA[]{collA, collA + 1, collA + 2, collA + 3};// list pointers
// rsbd8::radixsortnoalloc<&myclass::keyorder, rsbd8::decendingforwardordered>(4, pcollA, psomeunusedbuffer);
// Members of "pcollA" will then get sorted according to their value "keyorder", in reverse order, while keeping the same array order.
// Pointers will in this case point to: {2, "fourth"}, {1, "first"}, {1, "second"}, {-5, "third"}.
// That is different from fully reversing the order when using this line instead of the above:
// rsbd8::radixsortnoalloc<&myclass::keyorder, rsbd8::decendingreverseordered>(4, pcollA, psomeunusedbuffer);
// Pointers will in this case point to: {2, "fourth"}, {1, "second"}, {1, "first"}, {-5, "third"}.
// Notice the same reverse stable sorting here, but opposite placement when encountering the same value multiple times.
}// namespace rsbd8

// Miscellaneous notes
// Sorting unsigned values is the fastest, very closely followed up by signed values, followed up by floating-point values in this library.
// Unsigned 128-bit and larger integers can be sorted by sequential sorting from the bottom to the top parts as unsigned (64-bit) elements when using indirection.
// Signed (but otherwise unfiltered) 128-bit and larger integers are sorted the same, with only the topmost (64-bit) element sorted as signed because of the sign bit (assuming unfiltered input).
// Likewise, regular absolute-filtered floating-point 128-bit and larger types can be sorted like that as a top part with one or more unsigned bottom parts.
// Re-use the same intermediate buffer combined with radixsortnoalloc() or radixsortcopynoalloc() when sorting 128-bit and larger integers like this.
// Inputs of type bool are reinterpreted as the unsigned integer type of the same size, but handling them is extremely efficient anyway.
// Anything but 0 or 1 in bool source data will impact sorting, but this only happens if the user deliberately overrides the compiler behaviour for bool data.
// The sign of type char is ambiguous, as by the original C standard, so cast inputs to char8_t * or unsigned char *, or force unsigned processing modes if unambiguously a binary sort of char characters is desired.
// Floating-point -0. (implies not machine-generated) sorts below +0. by these functions in the default modes. (Several functions in namespace std can do that.)
// Floating-point NaN values are sorted before negative infinity for the typical machine-generated "undefined" QNaN (0xFFF8'0000'0000'0000 on an IEEE double).
// Floating-point NaN positive values (implies not machine-generated) are sorted after positive infinity (0x7FF0'0000'0000'0001 and onward on an IEEE double).
// Floating-point SNaN (signalling) values do not trigger signals inside these functions. (Several functions in namespace std can do that.)

// Naming and tooling conventions used in this library
// Textual:
// All names are lowercase, with no separators, and any sort of length, based on convenience or frequency of local usage.
// Any sort of global parts and namespace names are longer, to provide some context.
// All user-facing functions have a general description in a comment in the line above them. This is generally what any IDE will display when giving a tooltip.
// The few macro definitions are all uppercase, separated by underscores, starting with "RSDB8_".
// There are most certainly no limitations on the lengths of functions, lines, comment blocks or other items. Everything is just kept sensible, in proper order, never spaghettified or obscured much, feature-rich (even if a little complicated sometimes), and very much optimised.
// This library undefines its macro definitions at the end of the file and does not need to expose any macro definitions externally.
// This library has three imported code sections that are clearly marked at the beginning and end. These basically don't get edited unless they get replaced again with a newer version.
// Namespaces:
// rsbd8:: is radixsortbidi8, the enveloping namespace for this library
// rsbd8::helper:: is the underlying namespace for helper functions and constants. These are usually not directly invoked by the user of this library. No efforts are made to actually hide items from the user though.
// Templates:
// <typename T, typename U, typename V, typename W>
// These are the basic 4 placeholders for types.
// T is used for the main input type, or the primary type being referred to.
// U is the deferred type, for example the unsigned variant of T, or an unsigned general working type that is larger than T.
// V is always used as the class type of input and output arrays when dealing with indirection.
// W is the wildcard type, often an automatically deduced item in templated helper functions, but also often just a companion type to U.
// Some other template parts have defaults set up for them, especially for the longer lists of template parameters. This provides the user with often having less verbosity in their code.
// Template variable arguments as a C++ feature are used extensively by this library. Function-based variable arguments passing as seen in the original C isn't needed.
// All sorts of levels and configurations of template metaprogramming (C++17 and onward) are used in this library for optimisation, enhancing debugging or just making the code more concise.
// Functional:
// This is an optimal performance library. Of course, minor performance setbacks may arise from just compiler interpretation or functional issues. Keep the code close to the bare metal, in the right order to easily compile to instructions and use tons of optimisation features to overcome such issues.
// Even if many parts here just don't comply with most of the programming world's "clean" code rules, performance is key first, reducing the total count of functions is second, and being descriptive of functionality in a combination of code and comments is third.
// Defensive programming here is mostly left to template metaprogramming and compile-time assertions, as compile-time items don't hurt performance.
// Parameter and environment checking is left to the absolute minimum outside of the debug mode. Of course, when allocating memory failure is handled, and likewise the case of potentially throwing functions.

// Notes on ongoing research and development
//
// TODO, add support for types larger than 128 bits
// = TODO, currently all functions here are guarded with an upper limit of 8-, 64-, 80, 96- or 128-bit (using std::enable_if sections). Future functionality will either require lifting the limits on current functions, or adding another set of functions for the larger data types. Given that radix sort variants excel at processing large data types compared to comparison-based sorting methods, do give this some priority in development.
//
// TODO, document computer system architecture-dependent code parts and add more options
// = TODO, the current mode for pipelining (ILP - instruction level processing) is set to not need more than 15 integer registers, of which are 8 to 10 "hot" for parallel processing. This isn't a universally ideal option. To name two prominent systems, 32-bit x86 only has 7 available general-purpose registers, while ARM AArch64 (ARM64) has 31.
// = TODO, investigate SIMD, in all of its shapes and sizes. Some experimentation has been done with x64+AVX-512 in an early version, but compared to other optimisations and strategies it never yielded much for these test functions.
// = TODO, the current version of this library does not provide much optimisation for processing any 64-bit (or larger) types on 32-bit systems at all. This can be documented, and later on optimised.
// = TODO, similarly, 16-bit systems still exist (but these often do include a capable 32-bit capable data path line, see the history of x86 in this regard for example). If this library can be optimised for use in a reasonably current 16-bit microcontroller, document and later on optimise for it.
// = TODO, add more platform-dependent, optimised code sequences here similar to the current collections in the rsbd8::helper namespace.
// = TODO, possibly have a go at multi-threading by using a primary sorting phase, and a merging phase using a linear comparison-based method to finalise the sorted data array.
// = TODO, test and debug this library on more machines, platforms and such. Functionality and performance should both be guaranteed.
//
// TODO, this is a C++17 minimum library, but more modern features are welcome
// = TODO, bumping up the library to C++20 minimum isn't advised (yet), but could be in the near future.
// = TODO, C++23 features currently don't add much over some of the improvements seen in C++20, but for example indexed varargs from C++26 could certainly provide some simplification in a few functions here. Adding more modern C++ features (even if as optional items for now) is welcome.
//
// TODO, add support for non-array inputs
// = TODO, as the basic std::sort and std::stable_sort variants already support this functionality, it could be an advantage to add support for the other C++ iterable data sets.
// = TODO, performance testing and use case investigation is required for this subject, as radix sort types only really work well on somewhat larger arrays, and probably other larger iterable data sets, too.

// Extended filtering information for each of the 8 main modes
//
// Modes of operation for the template functions
// = regular unsigned integer "forceunsignedmode" (default for unsigned input types)
// - and also inside-out signed integer "specialsignedmode"
// - (sorts ascending from 0, maximum value, minimum value, to -1):
// --- absolute = false, issigned = false, isfloatingpoint = false
// --- lowest amount of filtering cost
// --- straightforward process, no filter at all
// = regular signed integer "forcesignedmode" (default for signed input types):
// --- absolute = false, issigned = true, isfloatingpoint = false
// --- lower amount of filtering cost
// --- no filter in the processing phases
// --- virtually flips the most significant bit when calculating offsets
// = absolute signed integer "forceabssignedmode":
// --- absolute = true, issigned = true, isfloatingpoint = false
// --- medium amount of filtering cost
// --- creates a sign bit mask, adds it to the input and uses it with xor on the input as a filter in the processing phases
// = regular floating-point "forcefloatingpmode" (default for floating-point input types):
// --- absolute = false, issigned = true, isfloatingpoint = true
// --- higher amount of filtering cost
// --- creates a sign bit mask and uses it with xor on the exponent and mantissa bits as a filter in the processing phases
// --- virtually flips the most significant bit when calculating offsets
// = absolute floating-point "forceabsfloatingpmode"
// - and also unsigned integer without using the top bit "specialunsignedmode":
// --- absolute = true, issigned = true, isfloatingpoint = true
// --- low amount of filtering cost
// --- masks out the sign bit in the processing phases
//
// The three special modes that can be activated are:
// = inside-out floating-point "specialfloatingpmode"
// - (sorts ascending from +0., +infinity, +NaN, -NaN, -infinity, to -0.):
// --- absolute = false, issigned = false, isfloatingpoint = true
// --- higher amount of filtering cost
// --- creates a sign bit mask and uses it with xor on the exponent and mantissa bits as a filter in the processing phases
// = absolute floating-point, but negative inputs will sort just below their positive counterparts "forcetieredabsfloatingpmode"
// - (sorts ascending from -0., +0., -infinity, +infinity, to various -NaN or +NaN values at the end):
// --- absolute = true, issigned = false, isfloatingpoint = true
// --- medium amount of filtering cost
// --- bit rotates left by one to move the sign bit to the least significant bit in the processing phases
// --- virtually flips the least significant bit when calculating offsets
// = absolute signed integer, but negative inputs will sort just below their positive counterparts "forcetieredabssignedmode"
// - (sorts ascending from 0, -1, 1, -2, 2, and so on, will work correctly for minimum values):
// --- absolute = true, issigned = false, isfloatingpoint = false
// --- medium amount of filtering cost
// --- creates a sign bit mask, shifts the input left by one and uses xor on the input with the sign bit mask as a filter in the processing phases
//
// Enabling reverse ordering on these modes will add slightly more to the initial filtering cost.
// For example, the highest amount of filtering cost would be on the floating-point full reverse mode.
// Take "highest amount" with a grain of salt though, as way more complicated filtering stages can be designed than what is used for this combination of filters.

// Performance tests
// This library has a performance test suite used for development.
// These performance test results are for sorting a block of 1 GiB, with fully random bits in integer and floating-point arrays (with no indirection or filtering).
// std::stable_sort() vs rsbd8::radixsort(), measured in 100 ns units:
// float :_ 58136210008 vs 5369598098, a factor of 10.83 in speedup
// double:_ 37631470938 vs 6683986582, a factor of 5.630 in speedup
// uint64:_ 36370643118 vs 6764700008, a factor of 5.377 in speedup
// int64 :_ 36754697976 vs 6773267432, a factor of 5.426 in speedup
// uint32:_ 75005874586 vs 5414093716, a factor of 13.85 in speedup
// int32 :_ 73407103112 vs 5420589916, a factor of 13.54 in speedup
// uint16: 112801542382 vs 5085844250, a factor of 22.18 in speedup
// int16 : 113114628676 vs 5090565828, a factor of 22.22 in speedup
// uint8 : 149307848836 vs 4138990724, a factor of 36.07 in speedup
// int8 _: 149440509248 vs 4135307694, a factor of 36.14 in speedup
//
// The next tests were done on smaller blocks.
// There will be a minimum amount of array entries where rsbd8::radixsort() starts to get the upper hand in speed over std::stable_sort().
// These test results were obtained by performance testing on multiple sizes of blocks between .5 to 8 KB, with again fully random bits in unsigned integer and floating-point arrays (with no indirection):
// float : 875 array entries
// double: 600 array entries
// uint64: 700 array entries
// uint32: 400 array entries
// uint16: 375 array entries
// uint8 : 300 array entries
// Interpreting this means that radix sort variants will be faster for somewhat larger arrays when sorting data under the given conditions.
// In this case that's a sequence of just plain numbers in an array.
// When dealing with sorting while using indirection or filtering, test results will vary.
//
// System configuration data:
// Performance testing was done on 2025-09-17 on development PC 1:
// = Intel Core i9 11900K, specification string: 11th Gen Intel Core i9-11900K @ 3.50GHz
// = Corsair CMK16GX4M2B3200C16 * 2, 32 GiB, 1600 MHz (XMP-3200 scheme), DDR4
// = ASRock Z590 PG Velocita, UEFI version L1.92
// = Windows 11 Home, 10.0.26100.6584
// - Some background programs were disabled, and the main testing was done by just running the test executable and letting DebugView x64 do the readout
// - (DebugView is also useful at keeping a record on any outputs generated by other simultaneous processes to analyse some possible disturbances).
//
// TODO: add performance tests that use at least first-level indirection

// Table of contents (searchable)
//
// = MIT License
// = Radixsortbidi8
// = Examples of using the 4 templates with simple arrays as input
// = Examples of using the 4 templates with input from indirection
// = Examples of using the 4 templates with input from modified indirection
// = Bonus example of the longest regular item, with complete decoration of the template
// = The 4 main sorting template functions that are implemented here
// = Modes of operation for the template functions
// = Miscellaneous notes
// = Naming and tooling conventions used in this library
// = Notes on ongoing research and development
// = Extended filtering information for each of the 8 main modes
// = Performance tests
// = Table of contents
//
// = Include statements and the last checks for compatibility
// = Per-compiler function attributes
//
// = Helper constants and functions
// = Helper functions to implement the 8 main modes
// = Helper functions to implement the offset transforms
// = Function implementation templates for multi-part types
// = Function implementation templates for single-part types
//
// = Definition of the GetOffsetOf template
// = Generic large array allocation and deallocation functions
// = Wrapper template functions for the 4 main sorting functions in this library
//
// = Library finalisation

// Include statements and the last checks for compatibility
// Compiler features minimum requirements are evaluated during compile-time if part of C++14 and newer.
// A more difficult test to implement here would be for example to detect mixed endianness between floating-point double and other data for on older ARM platforms.
// The C++20 "std::endian" parts in the "bit" header currently unfortunately don't indicate more than little, big and undefined mixed endianness.
//
// C++17 features detection
#if 201703L > __cplusplus
// Microsoft C/C++-compatible compilers don't set the __cplusplus predefined macro conforming to the standard by default for some very outdated legacy code reasons.
// /Zc:__cplusplus can correct it, but it's not part of the regular "Standards conformance" /permissive- compiler options.
// Use its internal macro here as a temporary fix.
#if !defined(_MSVC_LANG) || 201703L > _MSVC_LANG
#error Compiler does not conform to C++17 to compile this library.
#endif
#endif
#include <utility>
#include <cstring>
#include <cfloat>
#if CHAR_BIT & 8 - 1
#error This platform has an addressable unit that isn't divisible by 8. For these kinds of platforms it's better to re-write this library and not use an 8-bit indexed radix sort method.
#endif
#ifndef UINTPTR_MAX
#error This platform has no uintptr_t type, which should be near impossible. This library can be edited to get around that, however it might be more advantageous to edit the compiler.
#endif
#include <cassert>
#if 202002L <= __cplusplus || defined(_MSVC_LANG) && 202002L <= _MSVC_LANG
#include <bit>// (C++20)
// Library feature-test macros (C++20)
//
// Structured binding declaration (C++17)
#ifndef __cpp_structured_bindings
#error Compiler does not meet requirements for __cpp_structured_bindings for this library.
#endif
// std::byte (C++17)
#ifndef __cpp_lib_byte
#error Compiler does not meet requirements for __cpp_lib_byte for this library.
#endif
// std::is_nothrow_invocable_v (C++17)
// std::invoke_result_t (C++17)
#ifndef __cpp_lib_is_invocable
#error Compiler does not meet requirements for __cpp_lib_is_invocable for this library.
#endif
// std::is_same_v (C++17)
// std::is_integral_v (C++17)
// std::is_unsigned_v (C++17)
// std::is_signed_v (C++17)
// std::is_floating_point_v (C++17)
#ifndef __cpp_lib_type_trait_variable_templates
#error Compiler does not meet requirements for __cpp_lib_type_trait_variable_templates for this library.
#endif
// std::conditional_t (C++14)
// std::enable_if_t (C++14)
// std::make_signed_t (C++14)
#ifndef __cpp_lib_transformation_trait_aliases
#error Compiler does not meet requirements for __cpp_lib_transformation_trait_aliases for this library.
#endif
//
// Compiler features under consideration for usage in a newer version of the library:
// std::endian (C++20)
// __cpp_lib_endian
//
// Compiler features that are not required, but will be used conditionally:
// std::bit_width (C++20)
// __cpp_lib_int_pow2
// std::countr_zero (C++20)
// __cpp_lib_bitops
// std::bit_cast (C++20)
// __cpp_lib_bit_cast
// [[likely]] (C++20)
// __has_cpp_attribute(likely)
// [[unlikely]] (C++20)
// __has_cpp_attribute(unlikely)
// [[nodiscard]] (C++17)
// __has_cpp_attribute(nodiscard)
// [[maybe_unused]] (C++17)
// __has_cpp_attribute(maybe_unused)
#endif
#include <new>
#if !defined(_WIN32) && defined(_POSIX_C_SOURCE)// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
#include <sys/types.h>
#include <sys/mman.h>
#endif
// Start of imported section, to get compiler- and platform-specific intrinsic functions headers:
#if defined(__clang__) && (defined(__x86_64__) || defined(__i386__))
/* Clang-compatible compiler, targeting x86/x86-64 */
#include <x86intrin.h>
#elif defined(__clang__) && (defined(__ARM_NEON__) || defined(__aarch64__))
/* Clang-compatible compiler, targeting arm neon */
#include <arm_neon.h>
#elif defined(_MSC_VER)
/* Microsoft C/C++-compatible compiler */
#include <intrin.h>
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
/* GCC-compatible compiler, targeting x86/x86-64 */
#include <x86intrin.h>
#elif defined(__GNUC__) && (defined(__ARM_NEON__) || defined(__aarch64__))
/* GCC-compatible compiler, targeting ARM with NEON */
#include <arm_neon.h>
#if defined (MISSING_ARM_VLD1)
#include <ATen/cpu/vec256/missing_vld1_neon.h>
#elif defined (MISSING_ARM_VST1)
#include <ATen/cpu/vec256/missing_vst1_neon.h>
#endif
#elif defined(__GNUC__) && defined(__IWMMXT__)
/* GCC-compatible compiler, targeting ARM with WMMX */
#include <mmintrin.h>
#elif (defined(__GNUC__) || defined(__xlC__)) && \
		(defined(__VEC__) || defined(__ALTIVEC__))
/* XLC or GCC-compatible compiler, targeting PowerPC with VMX/VSX */
#include <altivec.h>
#elif defined(__GNUC__) && defined(__SPE__)
/* GCC-compatible compiler, targeting PowerPC with SPE */
#include <spe.h>
#endif
// End of imported section
namespace rsbd8{// Avoid putting any include files into this library's namespace.

// Per-compiler function attributes
// RSBD8_FUNC_INLINE is suitable to attempt force inlining of any function.
// RSBD8_FUNC_NORMAL is specifically for template functions in this header-only library, and doesn't include even a regular "inline" statement to prevent linking issues for non-template functions.
// These are the only two macros defined in this file, and #undef statements are used for them at the end.
#if defined(DEBUG) || defined(_DEBUG)// This part is debug-only. These are non-standard conforming macros, but note that the "NDEBUG" rule for detecting non-debug builds should only ever apply to runtime assert() statments in C++.
#ifdef _MSC_VER
#define RSBD8_FUNC_INLINE __declspec(noalias safebuffers) inline
#define RSBD8_FUNC_NORMAL __declspec(noalias safebuffers)
#else
#define RSBD8_FUNC_INLINE inline
#define RSBD8_FUNC_NORMAL
#endif
#else// release-only
#ifdef __clang__
#define RSBD8_FUNC_INLINE [[gnu::always_inline]] [[gnu::gnu_inline]] inline
#define RSBD8_FUNC_NORMAL
#elif defined(__GNUC__)
#define RSBD8_FUNC_INLINE [[gnu::always_inline]] inline
#define RSBD8_FUNC_NORMAL
#elif defined(__xlC__) || defined(__ghs__) || defined(__KEIL__) || defined(__CA__) || defined(__C166__) || defined(__C51__) || defined(__CX51__)
#define RSBD8_FUNC_INLINE inline __attribute__((always_inline))
#define RSBD8_FUNC_NORMAL
#elif defined(_MSC_VER)
#pragma warning(error: 4714)
#define RSBD8_FUNC_INLINE __declspec(noalias safebuffers) __forceinline
#define RSBD8_FUNC_NORMAL __declspec(noalias safebuffers)
#else
#define RSBD8_FUNC_INLINE inline
#define RSBD8_FUNC_NORMAL
#endif
#endif

// Helper constants and functions
namespace helper{// This libary defines a number of helper items, so categorise them as such.

// Integer binary logarithm of the pointer size constant
unsigned char constexpr log2ptrs{
#ifdef __cpp_lib_int_pow2// (C++20)
	static_cast<unsigned char>(std::bit_width(sizeof(void *) - 1))
#else
	32 == sizeof(void *)? 5 :
	16 == sizeof(void *)? 4 :
	8 == sizeof(void *)? 3 :
	4 == sizeof(void *)? 2 :
	2 == sizeof(void *)? 1 :
	1 == sizeof(void *)? 0 :
	static_cast<unsigned char>(~0)
#endif
};

// Endianess detection
// A dirty method that heavily relies on proper inlining and compiler optimisation of that, but it at least can detect the floating-point mixed endianness cases if used properly
template<typename T>
constexpr RSBD8_FUNC_INLINE std::enable_if_t<
	128 >= CHAR_BIT * sizeof(T) &&
	std::is_integral_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type>,
	T> generatehighbit(){
	return{static_cast<T>(1) << (CHAR_BIT * sizeof(T) - 1)};
}
template<typename T>
constexpr RSBD8_FUNC_INLINE std::enable_if_t<
	128 >= CHAR_BIT * sizeof(T) &&
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type>,
	T> generatehighbit(){
	// This will definately not work on some floating-point types from machines from the digial stone age.
	// However, this is a C++17 and onwards compatible library, and those devices have none of that.
	return{static_cast<T>(-0.)};
}

// Utility templates to create an immediate member object pointer for the type and offset indirection wrapper functions
#pragma pack(push, 1)
template<typename T, ptrdiff_t indirection1> struct memberobjectgenerator;
template<typename T>
struct memberobjectgenerator<T, 0>{
	T object;// no padding, as the object starts at the origin
};
template<typename T, ptrdiff_t indirection1>
struct memberobjectgenerator{
	std::byte padding[static_cast<size_t>(indirection1)];// this will work, but array counts are just required to be positive
	T object;// some amount of padding is used
};
#pragma pack(pop)

// Utility structs to generate tests for the often padded 80-bit long double types
// Platforms with a native 80-bit long double type are all little endian, hence that is the only implementation here.
struct alignas(16) longdoubletest128{uint_least64_t mantissa; uint_least16_t signexponent; uint_least16_t padding[3];};
struct longdoubletest96{uint_least32_t mantissa[2]; uint_least16_t signexponent; uint_least16_t padding;};
struct longdoubletest80{uint_least16_t mantissa[4]; uint_least16_t signexponent;};

// Utility templates to call the getter function while splitting off the second-level indirection index parameter
template<auto indirection1, typename V, typename... vararguments>
RSBD8_FUNC_INLINE decltype(auto) splitget(V *p, auto index2, vararguments... varparameters)noexcept(
	std::is_member_object_pointer_v<decltype(indirection1)> ||
	std::is_nothrow_invocable_v<std::conditional_t<decltype(indirection1), V *, vararguments...>>){
	static_cast<void>(index2);
	// The top option is a dummy, but it does allow the non-function indirection1 version of this to exist.
	if constexpr(std::is_member_object_pointer_v<decltype(indirection1)>) return p->*indirection1;
	else return (p->*indirection1)(varparameters...);
}
template<auto indirection1, typename V, typename... vararguments>
RSBD8_FUNC_INLINE decltype(auto) splitget(V *)noexcept{
	// This template of the function is a dummy, but it does allow the version without any extra arguments to exist.
	return nullptr;
}

// Utility templates to split off the first parameter
template<typename... vararguments>
RSBD8_FUNC_INLINE decltype(auto) splitparameter(auto first, vararguments...)noexcept{
	return first;
}
template<typename... vararguments>
RSBD8_FUNC_INLINE decltype(auto) splitparameter()noexcept{
	// This template of the function is a dummy, but it does allow the version without any extra arguments to exist.
	return nullptr;
}

// Utility template to retrieve the first-level source for full outputs
template<auto indirection1, bool isindexed2, typename T, typename V, typename... vararguments>
RSBD8_FUNC_INLINE decltype(auto) indirectinput1(V *p, vararguments... varparameters)	noexcept(
	std::is_member_object_pointer_v<decltype(indirection1)> ||
	std::is_nothrow_invocable_v<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>){
	using W = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t,// the default for all platforms
		std::conditional_t<96 == CHAR_BIT * sizeof(T), uint_least32_t,// only to support the 80-bit long double type with padding (always little endian)
		std::conditional_t<80 == CHAR_BIT * sizeof(T), uint_least16_t, void>>>;// only to support the 80-bit long double type (always little endian)
	if constexpr(std::is_member_object_pointer_v<decltype(indirection1)>){
		using U = std::remove_reference_t<decltype(std::declval<V>().*indirection1)>;
		if constexpr(!std::is_pointer_v<U>){// indirection directly to member, ignore isindexed2
			static_assert(sizeof(T) == sizeof(U), "misinterpreted indirection input type");
			if constexpr(1 == sizeof...(varparameters)){// indirection to member with an index
				if constexpr(64 < CHAR_BIT * sizeof(T)){
					std::byte const *pfinal{reinterpret_cast<std::byte const *>(p) + sizeof(T) * splitparameter(varparameters...)};
					return std::pair<uint_least64_t, W>{
						reinterpret_cast<V const *>(pfinal)->*reinterpret_cast<uint_least64_t(V:: *)>(indirection1),
						reinterpret_cast<V const *>(p + sizeof(uint_least64_t))->*reinterpret_cast<W(V:: *)>(indirection1)
					};
				}else{
					return reinterpret_cast<V const *>(reinterpret_cast<T const *>(p) + splitparameter(varparameters...))->*reinterpret_cast<T(V:: *)>(indirection1);
				}
			}else if constexpr(0 == sizeof...(varparameters)){// indirection to member without an index
				if constexpr(64 < CHAR_BIT * sizeof(T)){
					return std::pair<uint_least64_t, W>{
						p->*reinterpret_cast<uint_least64_t(V:: *)>(indirection1),
						reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(uint_least64_t))->*reinterpret_cast<W(V:: *)>(indirection1)
					};
				}else{
					return p->*reinterpret_cast<T(V:: *)>(indirection1);
				}
			}else static_assert(false, "impossible first-level indirection indexing parameter count");
		}else{
			static_assert(!std::is_pointer_v<std::remove_pointer_t<U>>, "third level indirection is not supported");
			static_assert(sizeof(T) == sizeof(std::remove_pointer_t<U>), "misinterpreted indirection input type");
			if constexpr(0 == sizeof...(varparameters)){// indirection to member with no indices, ignore isindexed2
				return reinterpret_cast<std::byte const *>(p->*indirection1);
				//return{*reinterpret_cast<T const *>(reinterpret_cast<std::byte const *>(p->*indirection1) + indirection2)};
			}else if constexpr(1 == sizeof...(varparameters)){// indirection to member with an index
				if constexpr(isindexed2){// second level extra index
					return reinterpret_cast<std::byte const *>(p->*indirection1);
					//return{reinterpret_cast<T const *>(reinterpret_cast<std::byte const *>(p->*indirection1) + indirection2)[splitparameter(varparameters...)]};
				}else{// first level extra index
					return reinterpret_cast<std::byte const *>(reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(void *) * splitparameter(varparameters...))->*indirection1);
					//return{*reinterpret_cast<T const *>(reinterpret_cast<std::byte const *>(reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(void *) * splitparameter(varparameters...))->*indirection1) + indirection2)};
				}
			}else if constexpr(2 == sizeof...(varparameters)){// indirection to member with two indices, ignore isindexed2
				std::pair indices{varparameters...};
				return reinterpret_cast<std::byte const *>(reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(void *) * indices.first)->*indirection1);
				//return{reinterpret_cast<T const *>(reinterpret_cast<std::byte const *>(reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(void *) * indices.first)->*indirection1) + indirection2)[indices.second]};
			}else static_assert(false, "impossible second-level indirection indexing parameter count");
		}
	}else if constexpr(std::is_member_function_pointer_v<decltype(indirection1)>){
		using U = std::invoke_result_t<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>;
		if constexpr(!std::is_pointer_v<U>){// indirection directly to item, ignore isindexed2
			static_assert(sizeof(T) == sizeof(U), "misinterpreted indirection input type");
			U val{(p->*indirection1)(varparameters...)};
			if constexpr(64 < CHAR_BIT * sizeof(T)){
				return std::pair<uint_least64_t, W>{
					*reinterpret_cast<uint_least64_t *>(&val),
					reinterpret_cast<W *>(&val)[sizeof(uint_least64_t) / sizeof(W)]
				};
			}else{
#ifdef __cpp_lib_bit_cast
				return std::bit_cast<T>(val);
#else
				return *reinterpret_cast<T *>(&val);
#endif
			}
		}else{// indirection to second level pointer
			static_assert(!std::is_pointer_v<std::remove_pointer_t<U>>, "third level indirection is not supported");
			static_assert(sizeof(T) == sizeof(std::remove_pointer_t<U>), "misinterpreted indirection input type");
			if constexpr(isindexed2){// second level extra index
				return reinterpret_cast<std::byte const *>(splitget<indirection1, V, vararguments...>(p, varparameters...));
				//return{reinterpret_cast<T const *>(reinterpret_cast<std::byte const *>(splitget<indirection1, V, vararguments...>(p, varparameters...)) + indirection2)[splitparameter(varparameters...)]};
			}else{// second level without an index
				return reinterpret_cast<std::byte const *>((p->*indirection1)(varparameters...));
				//return{*reinterpret_cast<T const *>(reinterpret_cast<std::byte const *>((p->*indirection1)(varparameters...)) + indirection2)};
			}
		}
	}else static_assert(false, "unsupported indirection input type");
}

// Utility templates to either retrieve the second-level source or pass though results for full outputs
template<auto indirection1, ptrdiff_t indirection2, bool isindexed2, typename T, typename... vararguments>
RSBD8_FUNC_INLINE decltype(auto) indirectinput2(std::byte const *pintermediate, vararguments... varparameters)noexcept{
	using W = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t,// the default for all platforms
		std::conditional_t<96 == CHAR_BIT * sizeof(T), uint_least32_t,// only to support the 80-bit long double type with padding (always little endian)
		std::conditional_t<80 == CHAR_BIT * sizeof(T), uint_least16_t, void>>>;// only to support the 80-bit long double type (always little endian)
	if constexpr(std::is_member_object_pointer_v<decltype(indirection1)>){
		if constexpr(0 == sizeof...(varparameters)){// indirection to member with no indices, ignore isindexed2
			if constexpr(64 < CHAR_BIT * sizeof(T)){
				return std::pair<uint_least64_t, W>{
					*reinterpret_cast<uint_least64_t const *>(pintermediate + indirection2),
					*reinterpret_cast<W const *>(pintermediate + indirection2 + sizeof(uint_least64_t))
				};
			}else{
				return *reinterpret_cast<T const *>(pintermediate + indirection2);
				//return{*reinterpret_cast<T const *>(reinterpret_cast<std::byte const *>(p->*indirection1) + indirection2)};
			}
		}else if constexpr(1 == sizeof...(varparameters)){// indirection to member with an index
			if constexpr(isindexed2){// second level extra index
				if constexpr(64 < CHAR_BIT * sizeof(T)){
					std::byte const *pfinal{pintermediate + indirection2 + sizeof(T) * splitparameter(varparameters...)};
					return std::pair<uint_least64_t, W>{
						*reinterpret_cast<uint_least64_t const *>(pfinal),
						*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
					};
				}else{
					return reinterpret_cast<T const *>(pintermediate + indirection2)[splitparameter(varparameters...)];
					//return{reinterpret_cast<T const *>(reinterpret_cast<std::byte const *>(p->*indirection1) + indirection2)[splitparameter(varparameters...)]};
				}
			}else{// first level extra index
				if constexpr(64 < CHAR_BIT * sizeof(T)){
					std::byte const *pfinal{pintermediate + indirection2};
					return std::pair<uint_least64_t, W>{
						*reinterpret_cast<uint_least64_t const *>(pfinal),
						*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
					};
				}else{
					return *reinterpret_cast<T const *>(pintermediate + indirection2);
					//return{*reinterpret_cast<T const *>(reinterpret_cast<std::byte const *>(reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(void *) * splitparameter(varparameters...))->*indirection1) + indirection2)};
				}
			}
		}else if constexpr(2 == sizeof...(varparameters)){// indirection to member with two indices, ignore isindexed2
			std::pair indices{varparameters...};
			if constexpr(64 < CHAR_BIT * sizeof(T)){
				std::byte const *pfinal{pintermediate + indirection2 + sizeof(T) * indices.second};
				return std::pair<uint_least64_t, W>{
					*reinterpret_cast<uint_least64_t const *>(pfinal),
					*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
				};
			}else{
				return reinterpret_cast<T const *>(pintermediate + indirection2)[indices.second];
				//return{reinterpret_cast<T const *>(reinterpret_cast<std::byte const *>(reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(void *) * indices.first)->*indirection1) + indirection2)[indices.second]};
			}
		}else static_assert(false, "impossible second-level indirection indexing parameter count");
	}else if constexpr(std::is_member_function_pointer_v<decltype(indirection1)>){
		if constexpr(isindexed2){// second level extra index
			if constexpr(64 < CHAR_BIT * sizeof(T)){
				std::byte const *pfinal{pintermediate + indirection2 + sizeof(T) * splitparameter(varparameters...)};
				return std::pair<uint_least64_t, W>{
					*reinterpret_cast<uint_least64_t const *>(pfinal),
					*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
				};
			}else{
				return reinterpret_cast<T const *>(pintermediate + indirection2)[splitparameter(varparameters...)];
				//return{reinterpret_cast<T const *>(reinterpret_cast<std::byte const *>(splitget<indirection1, V, vararguments...>(p, varparameters...)) + indirection2)[splitparameter(varparameters...)]};
			}
		}else{// second level without an index
			if constexpr(64 < CHAR_BIT * sizeof(T)){
				std::byte const *pfinal{pintermediate + indirection2};
				return std::pair<uint_least64_t, W>{
					*reinterpret_cast<uint_least64_t const *>(pfinal),
					*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
				};
			}else{
				return *reinterpret_cast<T const *>(pintermediate + indirection2);
				//return{*reinterpret_cast<T const *>(reinterpret_cast<std::byte const *>((p->*indirection1)(varparameters...)) + indirection2)};
			}
		}
	}else static_assert(false, "unsupported indirection input type");
}
template<auto indirection1, ptrdiff_t indirection2, bool isindexed2, typename T, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<!std::is_pointer_v<T>,
	T> indirectinput2(T passthrough, vararguments...)noexcept{
	return{passthrough};
}

// Utility template to retrieve the first-level source for partial 8-bit outputs
// Gave shifter the upgrade to a size_t, as it's all pointer logic in here.
template<auto indirection1, bool absolute, bool issigned, bool isfloatingpoint, bool isindexed2, typename T, typename V, typename... vararguments>
RSBD8_FUNC_INLINE decltype(auto) indirectinput1(V *p, size_t shifter, vararguments... varparameters)noexcept(
	std::is_member_object_pointer_v<decltype(indirection1)> ||
	std::is_nothrow_invocable_v<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>){
	using W = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t,// the default for all platforms
		std::conditional_t<96 == CHAR_BIT * sizeof(T), uint_least32_t,// only to support the 80-bit long double type with padding (always little endian)
		std::conditional_t<80 == CHAR_BIT * sizeof(T), uint_least16_t, void>>>;// only to support the 80-bit long double type (always little endian)
	static size_t constexpr highbyteoffset{
		(std::is_same_v<longdoubletest128, T> ||
		std::is_same_v<longdoubletest96, T> ||
		std::is_same_v<longdoubletest80, T> ||
		std::is_same_v<long double, T> &&
		64 == LDBL_MANT_DIG &&
		16384 == LDBL_MAX_EXP &&
		128 >= CHAR_BIT * sizeof(long double) &&
		64 < CHAR_BIT * sizeof(long double))? sizeof(longdoubletest80) - 1 : sizeof(T) - 1};
	static T constexpr highbit{generatehighbit<T>()};
	static bool constexpr isaddressingsubdivisable{
		!(absolute && !issigned) &&// the two tiered absolute types shift bits out of place during filering
#ifdef RSBD8_DISABLE_ADDRESS_SUBDIVISION
		false
#else
		8 == CHAR_BIT &&// optimisation for multi-part addressable machines only
		(std::is_member_object_pointer_v<decltype(indirection1)> ||
		std::is_pointer_v<std::invoke_result_t<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>>)
#endif
	};
	if constexpr(std::is_member_object_pointer_v<decltype(indirection1)>){
		using U = std::remove_reference_t<decltype(std::declval<V>().*indirection1)>;
		if constexpr(!std::is_pointer_v<U>){// indirection directly to member, ignore isindexed2
			static_assert(sizeof(T) == sizeof(U), "misinterpreted indirection input type");
			if constexpr(1 == sizeof...(varparameters)){// indirection to member with an index
				if constexpr(isaddressingsubdivisable){
					if constexpr(absolute != isfloatingpoint){
						std::byte const *pfinal{reinterpret_cast<std::byte const *>(p) + sizeof(T) * splitparameter(varparameters...)};
						return std::pair<unsigned char, unsigned char>{
							reinterpret_cast<V const *>(pfinal + shifter)->*reinterpret_cast<unsigned char(V:: *)>(indirection1),
							reinterpret_cast<V const *>(pfinal + highbyteoffset *
							(1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1)))// 0 on big endian, 1 on little endian
							)->*reinterpret_cast<unsigned char(V:: *)>(indirection1)
						};
					}else{
						return reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + shifter + sizeof(T) * splitparameter(varparameters...))->*reinterpret_cast<unsigned char(V:: *)>(indirection1);
					}
				}else{
					if constexpr(64 < CHAR_BIT * sizeof(T)){
						std::byte const *pfinal{reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(T) * splitparameter(varparameters...))};
						return std::pair<uint_least64_t, W>{
							reinterpret_cast<V const *>(pfinal)->*reinterpret_cast<uint_least64_t(V:: *)>(indirection1),
							reinterpret_cast<V const *>(pfinal + sizeof(uint_least64_t))->*reinterpret_cast<W(V:: *)>(indirection1)
						};
					}else{
						return reinterpret_cast<V const *>(reinterpret_cast<T const *>(p) + splitparameter(varparameters...))->*reinterpret_cast<T(V:: *)>(indirection1);
					}
				}
			}else if constexpr(0 == sizeof...(varparameters)){// indirection to member without an index
				if constexpr(isaddressingsubdivisable){
					if constexpr(absolute != isfloatingpoint){
						return std::pair<unsigned char, unsigned char>{
							reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + shifter)->*reinterpret_cast<unsigned char(V:: *)>(indirection1),
							reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + highbyteoffset *
							(1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1)))// 0 on big endian, 1 on little endian
							)->*reinterpret_cast<unsigned char(V:: *)>(indirection1)
						};
					}else{
						return reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + shifter)->*reinterpret_cast<unsigned char(V:: *)>(indirection1);
					}
				}else{
					if constexpr(64 < CHAR_BIT * sizeof(T)){
						return std::pair<uint_least64_t, W>{
							p->*reinterpret_cast<uint_least64_t(V:: *)>(indirection1),
							reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(uint_least64_t))->*reinterpret_cast<W(V:: *)>(indirection1)
						};
					}else{
						return p->*reinterpret_cast<T(V:: *)>(indirection1);
					}
				}
			}else static_assert(false, "impossible first-level indirection indexing parameter count");
		}else{
			static_assert(!std::is_pointer_v<std::remove_pointer_t<U>>, "third level indirection is not supported");
			static_assert(sizeof(T) == sizeof(std::remove_pointer_t<U>), "misinterpreted indirection input type");
			if constexpr(0 == sizeof...(varparameters)){// indirection to member with no indices, ignore isindexed2
				return reinterpret_cast<unsigned char const *>(p->*indirection1);
				//return{*reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>(p->*indirection1) + indirection2)};
			}else if constexpr(1 == sizeof...(varparameters)){// indirection to member with an index
				if constexpr(isindexed2){// second level extra index
					return reinterpret_cast<unsigned char const *>(p->*indirection1);
					//return{reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>(p->*indirection1) + indirection2)[splitparameter(varparameters...)]};
				}else{// first level extra index
					return reinterpret_cast<unsigned char const *>(reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(void *) * splitparameter(varparameters...))->*indirection1);
					//return{*reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>(reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(void *) * splitparameter(varparameters...))->*indirection1) + indirection2)};
				}
			}else if constexpr(2 == sizeof...(varparameters)){// indirection to member with two indices, ignore isindexed2
				std::pair indices{varparameters...};
				return reinterpret_cast<unsigned char const *>(reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(void *) * indices.first)->*indirection1);
				//return{reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>(reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(void *) * indices.first)->*indirection1) + indirection2)[indices.second]};
			}else static_assert(false, "impossible second-level indirection indexing parameter count");
		}
	}else if constexpr(std::is_member_function_pointer_v<decltype(indirection1)>){
		using U = std::invoke_result_t<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>;
		if constexpr(!std::is_pointer_v<U>){// indirection directly to item, ignore isindexed2
			static_assert(sizeof(T) == sizeof(U), "misinterpreted indirection input type");
			U val{(p->*indirection1)(varparameters...)};
			if constexpr(64 < CHAR_BIT * sizeof(T)){
				return std::pair<uint_least64_t, W>{
					*reinterpret_cast<uint_least64_t *>(&val),
					reinterpret_cast<W *>(&val)[sizeof(uint_least64_t) / sizeof(W)]
				};
			}else{
#ifdef __cpp_lib_bit_cast
				return std::bit_cast<T>(val);
#else
				return *reinterpret_cast<T *>(&val);
#endif
			}
		}else{// indirection to second level pointer
			static_assert(!std::is_pointer_v<std::remove_pointer_t<U>>, "third level indirection is not supported");
			static_assert(sizeof(T) == sizeof(std::remove_pointer_t<U>), "misinterpreted indirection input type");
			if constexpr(isindexed2){// second level extra index
				return reinterpret_cast<unsigned char const *>(splitget<indirection1, V, vararguments...>(p, varparameters...));
				//return{reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>(splitget<indirection1, V, vararguments...>(p, varparameters...)) + indirection2)[splitparameter(varparameters...)]};
			}else{// second level without an index
				return reinterpret_cast<unsigned char const *>((p->*indirection1)(varparameters...));
				//return{*reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>((p->*indirection1)(varparameters...)) + indirection2)};
			}
		}
	}else static_assert(false, "unsupported indirection input type");
}

// Utility templates to either retrieve the second-level source or pass though results for partial 8-bit outputs
// Gave shifter the upgrade to a size_t, as it's all pointer logic in here.
template<auto indirection1, bool absolute, bool issigned, bool isfloatingpoint, ptrdiff_t indirection2, bool isindexed2, typename T, typename V, typename... vararguments>
RSBD8_FUNC_INLINE decltype(auto) indirectinput2(unsigned char const *pintermediate, size_t shifter, vararguments... varparameters)noexcept{
	using W = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t,// the default for all platforms
		std::conditional_t<96 == CHAR_BIT * sizeof(T), uint_least32_t,// only to support the 80-bit long double type with padding (always little endian)
		std::conditional_t<80 == CHAR_BIT * sizeof(T), uint_least16_t, void>>>;// only to support the 80-bit long double type (always little endian)
	static size_t constexpr highbyteoffset{
		(std::is_same_v<longdoubletest128, T> ||
		std::is_same_v<longdoubletest96, T> ||
		std::is_same_v<longdoubletest80, T> ||
		std::is_same_v<long double, T> &&
		64 == LDBL_MANT_DIG &&
		16384 == LDBL_MAX_EXP &&
		128 >= CHAR_BIT * sizeof(long double) &&
		64 < CHAR_BIT * sizeof(long double))? sizeof(longdoubletest80) - 1 : sizeof(T) - 1};
	static T constexpr highbit{generatehighbit<T>()};
	static bool constexpr isaddressingsubdivisable{
		!(absolute && !issigned) &&// the two tiered absolute types shift bits out of place during filering
#ifdef RSBD8_DISABLE_ADDRESS_SUBDIVISION
		false
#else
		8 == CHAR_BIT &&// optimisation for multi-part addressable machines only
		(std::is_member_object_pointer_v<decltype(indirection1)> ||
		std::is_pointer_v<std::invoke_result_t<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>>)
#endif
	};
	if constexpr(std::is_member_object_pointer_v<decltype(indirection1)>){
		if constexpr(0 == sizeof...(varparameters)){// indirection to member with no indices, ignore isindexed2
			if constexpr(isaddressingsubdivisable){
				if constexpr(absolute != isfloatingpoint){
					unsigned char const *pfinal{pintermediate + indirection2};
					return std::pair<unsigned char, unsigned char>{
						pfinal[shifter],
						pfinal[highbyteoffset *
						(1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1)))// 0 on big endian, 1 on little endian
						]
					};
				}else{
					return *(pintermediate + indirection2 + shifter);
				}
			}else{
				if constexpr(64 < CHAR_BIT * sizeof(T)){
					unsigned char const *pfinal{pintermediate + indirection2};
					return std::pair<uint_least64_t, W>{
						*reinterpret_cast<uint_least64_t const *>(pfinal),
						*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
					};
				}else{
					return *reinterpret_cast<T const *>(pintermediate + indirection2);
					//return{*reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>(p->*indirection1) + indirection2)};
				}
			}
		}else if constexpr(1 == sizeof...(varparameters)){// indirection to member with an index
			if constexpr(isindexed2){// second level extra index
				if constexpr(isaddressingsubdivisable){
					if constexpr(absolute != isfloatingpoint){
						unsigned char const *pfinal{pintermediate + indirection2 + sizeof(T) * splitparameter(varparameters...)};
						return std::pair<unsigned char, unsigned char>{
							pfinal[shifter],
							pfinal[highbyteoffset *
							(1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1)))// 0 on big endian, 1 on little endian
							]
						};
					}else{
						return *(pintermediate + indirection2 + shifter + sizeof(T) * splitparameter(varparameters...));
					}
				}else{
					if constexpr(64 < CHAR_BIT * sizeof(T)){
						unsigned char const *pfinal{pintermediate + indirection2 + sizeof(T) * splitparameter(varparameters...)};
						return std::pair<uint_least64_t, W>{
							*reinterpret_cast<uint_least64_t const *>(pfinal),
							*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
						};
					}else{
						return reinterpret_cast<T const *>(pintermediate + indirection2)[splitparameter(varparameters...)];
						//return{reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>(p->*indirection1) + indirection2)[splitparameter(varparameters...)]};
					}
				}
			}else{// first level extra index
				if constexpr(isaddressingsubdivisable){
					if constexpr(absolute != isfloatingpoint){
						unsigned char const *pfinal{pintermediate + indirection2};
						return std::pair<unsigned char, unsigned char>{
							pfinal[shifter],
							pfinal[highbyteoffset *
							(1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1)))// 0 on big endian, 1 on little endian
							]
						};
					}else{
						return *(pintermediate + indirection2 + shifter);
					}
				}else{
					if constexpr(64 < CHAR_BIT * sizeof(T)){
						unsigned char const *pfinal{pintermediate + indirection2};
						return std::pair<uint_least64_t, W>{
							*reinterpret_cast<uint_least64_t const *>(pfinal),
							*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
						};
					}else{
						return *reinterpret_cast<T const *>(pintermediate + indirection2);
						//return{*reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>(reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(void *) * splitparameter(varparameters...))->*indirection1) + indirection2)};
					}
				}
			}
		}else if constexpr(2 == sizeof...(varparameters)){// indirection to member with two indices, ignore isindexed2
			std::pair indices{varparameters...};
			if constexpr(isaddressingsubdivisable){
				if constexpr(absolute != isfloatingpoint){
					unsigned char const *pfinal{pintermediate + indirection2 + sizeof(T) * indices.second};
					return std::pair<unsigned char, unsigned char>{
						pfinal[shifter],
						pfinal[highbyteoffset *
						(1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1)))// 0 on big endian, 1 on little endian
						]
					};
				}else{
					return *(pintermediate + indirection2 + shifter + sizeof(T) * indices.second);
				}
			}else{
				if constexpr(64 < CHAR_BIT * sizeof(T)){
					unsigned char const *pfinal{pintermediate + indirection2 + sizeof(T) * indices.second};
					return std::pair<uint_least64_t, W>{
						*reinterpret_cast<uint_least64_t const *>(pfinal),
						*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
					};
				}else{
					return reinterpret_cast<T const *>(pintermediate + indirection2)[indices.second];
					//return{reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>(reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(void *) * indices.first)->*indirection1) + indirection2)[indices.second]};
				}
			}
		}else static_assert(false, "impossible second-level indirection indexing parameter count");
	}else if constexpr(std::is_member_function_pointer_v<decltype(indirection1)>){
		if constexpr(isindexed2){// second level extra index
			if constexpr(isaddressingsubdivisable){
				if constexpr(absolute != isfloatingpoint){
					unsigned char const *pfinal{pintermediate + indirection2 + sizeof(T) * splitparameter(varparameters...)};
					return std::pair<unsigned char, unsigned char>{
						pfinal[shifter],
						pfinal[highbyteoffset *
						(1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1)))// 0 on big endian, 1 on little endian
						]
					};
				}else{
					return *(pintermediate + indirection2 + shifter + sizeof(T) * splitparameter(varparameters...));
				}
			}else{
				if constexpr(64 < CHAR_BIT * sizeof(T)){
					unsigned char const *pfinal{pintermediate + indirection2 + sizeof(T) * splitparameter(varparameters...)};
					return std::pair<uint_least64_t, W>{
						*reinterpret_cast<uint_least64_t const *>(pfinal),
						*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
					};
				}else{
					return reinterpret_cast<T const *>(pintermediate + indirection2)[splitparameter(varparameters...)];
					//return{reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>(splitget<indirection1, V, vararguments...>(p, varparameters...)) + indirection2)[splitparameter(varparameters...)]};
				}
			}
		}else{// second level without an index
			if constexpr(isaddressingsubdivisable){
				if constexpr(absolute != isfloatingpoint){
					unsigned char const *pfinal{pintermediate + indirection2};
					return std::pair<unsigned char, unsigned char>{
						pfinal[shifter],
						pfinal[highbyteoffset *
						(1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1)))// 0 on big endian, 1 on little endian
						]
					};
				}else{
					return *(pintermediate + indirection2 + shifter);
				}
			}else{
				if constexpr(64 < CHAR_BIT * sizeof(T)){
					unsigned char const *pfinal{pintermediate + indirection2};
					return std::pair<uint_least64_t, W>{
						*reinterpret_cast<uint_least64_t const *>(pfinal),
						*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
					};
				}else{
					return *reinterpret_cast<T const *>(pintermediate + indirection2);
					//return{*reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>((p->*indirection1)(varparameters...)) + indirection2)};
				}
			}
		}
	}else static_assert(false, "unsupported indirection input type");
}
template<auto indirection1, bool absolute, bool issigned, bool isfloatingpoint, ptrdiff_t indirection2, bool isindexed2, typename T, typename V, typename W, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<!std::is_pointer_v<W>,
	W> indirectinput2(W passthrough, size_t, vararguments...)noexcept{
	return{passthrough};
}

// Utility template to retrieve the first-level source for partial 8-bit outputs
// This variant handles the 80-bit long double types' second highest byte.
// Platforms with a native 80-bit long double type are all little endian, hence that is the only implementation here.
template<auto indirection1, bool absolute, bool issigned, bool isfloatingpoint, bool isindexed2, typename T, typename V, typename... vararguments>
RSBD8_FUNC_INLINE decltype(auto) indirectinputbelowtop1(V *p, vararguments... varparameters)noexcept(
	std::is_member_object_pointer_v<decltype(indirection1)> ||
	std::is_nothrow_invocable_v<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>){
	using W = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t,// the default for all platforms
		std::conditional_t<96 == CHAR_BIT * sizeof(T), uint_least32_t,// only to support the 80-bit long double type with padding (always little endian)
		std::conditional_t<80 == CHAR_BIT * sizeof(T), uint_least16_t, void>>>;// only to support the 80-bit long double type (always little endian)
	static size_t constexpr highbyteoffset{sizeof(longdoubletest80) - 1};
	static bool constexpr isaddressingsubdivisable{
		!(absolute && !issigned) &&// the two tiered absolute types shift bits out of place during filering
#ifdef RSBD8_DISABLE_ADDRESS_SUBDIVISION
		false
#else
		8 == CHAR_BIT &&// optimisation for multi-part addressable machines only
		(std::is_member_object_pointer_v<decltype(indirection1)> ||
		std::is_pointer_v<std::invoke_result_t<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>>)
#endif
	};
	if constexpr(std::is_member_object_pointer_v<decltype(indirection1)>){
		using U = std::remove_reference_t<decltype(std::declval<V>().*indirection1)>;
		if constexpr(!std::is_pointer_v<U>){// indirection directly to member, ignore isindexed2
			static_assert(sizeof(T) == sizeof(U), "misinterpreted indirection input type");
			if constexpr(1 == sizeof...(varparameters)){// indirection to member with an index
				if constexpr(isaddressingsubdivisable){
					if constexpr(absolute != isfloatingpoint){
						return reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + highbyteoffset - 1 + sizeof(T) * splitparameter(varparameters...))->*reinterpret_cast<uint_least16_t(V:: *)>(indirection1);
					}else{
						return reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + highbyteoffset - 1 + sizeof(T) * splitparameter(varparameters...))->*reinterpret_cast<unsigned char(V:: *)>(indirection1);
					}
				}else{
					std::byte const *pfinal{reinterpret_cast<std::byte const *>(p) + sizeof(T) * splitparameter(varparameters...)};
					return std::pair<uint_least64_t, W>{
						reinterpret_cast<V const *>(pfinal)->*reinterpret_cast<uint_least64_t(V:: *)>(indirection1),
						reinterpret_cast<V const *>(pfinal + sizeof(uint_least64_t))->*reinterpret_cast<W(V:: *)>(indirection1)
					};
				}
			}else if constexpr(0 == sizeof...(varparameters)){// indirection to member without an index
				if constexpr(isaddressingsubdivisable){
					if constexpr(absolute != isfloatingpoint){
						return reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + highbyteoffset - 1)->*reinterpret_cast<uint_least16_t(V:: *)>(indirection1);
					}else{
						return reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + highbyteoffset - 1)->*reinterpret_cast<unsigned char(V:: *)>(indirection1);
					}
				}else{
					return std::pair<uint_least64_t, W>{
						p->*reinterpret_cast<uint_least64_t(V:: *)>(indirection1),
						reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(uint_least64_t))->*reinterpret_cast<W(V:: *)>(indirection1)
					};
				}
			}else static_assert(false, "impossible first-level indirection indexing parameter count");
		}else{
			static_assert(!std::is_pointer_v<std::remove_pointer_t<U>>, "third level indirection is not supported");
			static_assert(sizeof(T) == sizeof(std::remove_pointer_t<U>), "misinterpreted indirection input type");
			if constexpr(0 == sizeof...(varparameters)){// indirection to member with no indices, ignore isindexed2
				return reinterpret_cast<unsigned char const *>(p->*indirection1);
				//return{*reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>(p->*indirection1) + indirection2)};
			}else if constexpr(1 == sizeof...(varparameters)){// indirection to member with an index
				if constexpr(isindexed2){// second level extra index
					return reinterpret_cast<unsigned char const *>(p->*indirection1);
					//return{reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>(p->*indirection1) + indirection2)[splitparameter(varparameters...)]};
				}else{// first level extra index
					return reinterpret_cast<unsigned char const *>(reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(void *) * splitparameter(varparameters...))->*indirection1);
					//return{*reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>(reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(void *) * splitparameter(varparameters...))->*indirection1) + indirection2)};
				}
			}else if constexpr(2 == sizeof...(varparameters)){// indirection to member with two indices, ignore isindexed2
				std::pair indices{varparameters...};
				return reinterpret_cast<unsigned char const *>(reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(void *) * indices.first)->*indirection1);
				//return{reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>(reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(void *) * indices.first)->*indirection1) + indirection2)[indices.second]};
			}else static_assert(false, "impossible second-level indirection indexing parameter count");
		}
	}else if constexpr(std::is_member_function_pointer_v<decltype(indirection1)>){
		using U = std::invoke_result_t<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>;
		if constexpr(!std::is_pointer_v<U>){// indirection directly to item, ignore isindexed2
			static_assert(sizeof(T) == sizeof(U), "misinterpreted indirection input type");
			U val{(p->*indirection1)(varparameters...)};
			return std::pair<uint_least64_t, W>{
				*reinterpret_cast<uint_least64_t *>(&val),
				reinterpret_cast<W *>(&val)[sizeof(uint_least64_t) / sizeof(W)]
			};
		}else{// indirection to second level pointer
			static_assert(!std::is_pointer_v<std::remove_pointer_t<U>>, "third level indirection is not supported");
			static_assert(sizeof(T) == sizeof(std::remove_pointer_t<U>), "misinterpreted indirection input type");
			if constexpr(isindexed2){// second level extra index
				return reinterpret_cast<unsigned char const *>(splitget<indirection1, V, vararguments...>(p, varparameters...));
				//return{reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>(splitget<indirection1, V, vararguments...>(p, varparameters...)) + indirection2)[splitparameter(varparameters...)]};
			}else{// second level without an index
				return reinterpret_cast<unsigned char const *>((p->*indirection1)(varparameters...));
				//return{*reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>((p->*indirection1)(varparameters...)) + indirection2)};
			}
		}
	}else static_assert(false, "unsupported indirection input type");
}

// Utility templates to either retrieve the second-level source or pass though results for partial 8-bit outputs
// This variant handles the 80-bit long double types' second highest byte.
// Platforms with a native 80-bit long double type are all little endian, hence that is the only implementation here.
template<auto indirection1, bool absolute, bool issigned, bool isfloatingpoint, ptrdiff_t indirection2, bool isindexed2, typename T, typename V, typename... vararguments>
RSBD8_FUNC_INLINE decltype(auto) indirectinputbelowtop2(unsigned char const *pintermediate, vararguments... varparameters)noexcept{
	using W = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t,// the default for all platforms
		std::conditional_t<96 == CHAR_BIT * sizeof(T), uint_least32_t,// only to support the 80-bit long double type with padding (always little endian)
		std::conditional_t<80 == CHAR_BIT * sizeof(T), uint_least16_t, void>>>;// only to support the 80-bit long double type (always little endian)
	static size_t constexpr highbyteoffset{sizeof(longdoubletest80) - 1};
	static bool constexpr isaddressingsubdivisable{
		!(absolute && !issigned) &&// the two tiered absolute types shift bits out of place during filering
#ifdef RSBD8_DISABLE_ADDRESS_SUBDIVISION
		false
#else
		8 == CHAR_BIT &&// optimisation for multi-part addressable machines only
		(std::is_member_object_pointer_v<decltype(indirection1)> ||
		std::is_pointer_v<std::invoke_result_t<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>>)
#endif
	};
	if constexpr(std::is_member_object_pointer_v<decltype(indirection1)>){
		if constexpr(0 == sizeof...(varparameters)){// indirection to member with no indices, ignore isindexed2
			if constexpr(isaddressingsubdivisable){
				if constexpr(absolute != isfloatingpoint){
					return *reinterpret_cast<uint_least16_t *>(pintermediate + indirection2 + highbyteoffset - 1)	;
				}else{
					return *(pintermediate + indirection2 + highbyteoffset - 1);
				}
			}else{
				unsigned char const *pfinal{pintermediate + indirection2};
				return std::pair<uint_least64_t, W>{
					*reinterpret_cast<uint_least64_t const *>(pfinal),
					*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
				};
			}
		}else if constexpr(1 == sizeof...(varparameters)){// indirection to member with an index
			if constexpr(isindexed2){// second level extra index
				if constexpr(isaddressingsubdivisable){
					if constexpr(absolute != isfloatingpoint){
						return *reinterpret_cast<uint_least16_t const *>(pintermediate + indirection2 + highbyteoffset - 1 + sizeof(T) * splitparameter(varparameters...));
					}else{
						return *(pintermediate + indirection2 + highbyteoffset - 1 + sizeof(T) * splitparameter(varparameters...));
					}
				}else{
					unsigned char const *pfinal{pintermediate + indirection2 + sizeof(T) * splitparameter(varparameters...)};
					return std::pair<uint_least64_t, W>{
						*reinterpret_cast<uint_least64_t const *>(pfinal),
						*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
					};
				}
			}else{// first level extra index
				if constexpr(isaddressingsubdivisable){
					if constexpr(absolute != isfloatingpoint){
						return *reinterpret_cast<uint_least16_t *>(pintermediate + indirection2 + highbyteoffset - 1);
					}else{
						return *(pintermediate + indirection2 + highbyteoffset - 1);
					}
				}else{
					unsigned char const *pfinal{pintermediate + indirection2};
					return std::pair<uint_least64_t, W>{
						*reinterpret_cast<uint_least64_t const *>(pfinal),
						*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
					};
				}
			}
		}else if constexpr(2 == sizeof...(varparameters)){// indirection to member with two indices, ignore isindexed2
			std::pair indices{varparameters...};
			if constexpr(isaddressingsubdivisable){
				if constexpr(absolute != isfloatingpoint){
					return *reinterpret_cast<uint_least16_t const *>(pintermediate + indirection2 + highbyteoffset - 1 + sizeof(T) * indices.second);
				}else{
					return *(pintermediate + indirection2 + highbyteoffset - 1 + sizeof(T) * indices.second);
				}
			}else{
				unsigned char const *pfinal{pintermediate + indirection2 + sizeof(T) * indices.second};
				return std::pair<uint_least64_t, W>{
					*reinterpret_cast<uint_least64_t const *>(pfinal),
					*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
				};
			}
		}else static_assert(false, "impossible second-level indirection indexing parameter count");
	}else if constexpr(std::is_member_function_pointer_v<decltype(indirection1)>){
		if constexpr(isindexed2){// second level extra index
			if constexpr(isaddressingsubdivisable){
				if constexpr(absolute != isfloatingpoint){
					return *reinterpret_cast<uint_least16_t const *>(pintermediate + indirection2 + highbyteoffset - 1 + sizeof(T) * splitparameter(varparameters...));
				}else{
					return *(pintermediate + indirection2 + highbyteoffset - 1 + sizeof(T) * splitparameter(varparameters...));
				}
			}else{
				unsigned char const *pfinal{pintermediate + indirection2 + sizeof(T) * splitparameter(varparameters...)};
				return std::pair<uint_least64_t, W>{
					*reinterpret_cast<uint_least64_t const *>(pfinal),
					*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
				};
			}
		}else{// second level without an index
			if constexpr(isaddressingsubdivisable){
				if constexpr(absolute != isfloatingpoint){
					return *reinterpret_cast<uint_least16_t *>(pintermediate + indirection2 + highbyteoffset - 1);
				}else{
					return *(pintermediate + indirection2 + highbyteoffset - 1);
				}
			}else{
				unsigned char const *pfinal{pintermediate + indirection2};
				return std::pair<uint_least64_t, W>{
					*reinterpret_cast<uint_least64_t const *>(pfinal),
					*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
				};
			}
		}
	}else static_assert(false, "unsupported indirection input type");
}
template<auto indirection1, bool absolute, bool issigned, bool isfloatingpoint, ptrdiff_t indirection2, bool isindexed2, typename T, typename V, typename W, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<!std::is_pointer_v<W>,
	W> indirectinputbelowtop2(W passthrough, vararguments...)noexcept{
	return{passthrough};
}

// Utility template to retrieve the first-level source for partial 8-bit outputs
// This variant handles the 80-bit long double types' highest byte.
// Platforms with a native 80-bit long double type are all little endian, hence that is the only implementation here.
template<auto indirection1, bool absolute, bool issigned, bool isfloatingpoint, bool isindexed2, typename T, typename V, typename... vararguments>
RSBD8_FUNC_INLINE decltype(auto) indirectinputtop1(V *p, vararguments... varparameters)noexcept(
	std::is_member_object_pointer_v<decltype(indirection1)> ||
	std::is_nothrow_invocable_v<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>){
	using W = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t,// the default for all platforms
		std::conditional_t<96 == CHAR_BIT * sizeof(T), uint_least32_t,// only to support the 80-bit long double type with padding (always little endian)
		std::conditional_t<80 == CHAR_BIT * sizeof(T), uint_least16_t, void>>>;// only to support the 80-bit long double type (always little endian)
	static size_t constexpr highbyteoffset{sizeof(longdoubletest80) - 1};
	static bool constexpr isaddressingsubdivisable{
		!(absolute && !issigned) &&// the two tiered absolute types shift bits out of place during filering
#ifdef RSBD8_DISABLE_ADDRESS_SUBDIVISION
		false
#else
		8 == CHAR_BIT &&// optimisation for multi-part addressable machines only
		(std::is_member_object_pointer_v<decltype(indirection1)> ||
		std::is_pointer_v<std::invoke_result_t<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>>)
#endif
	};
	if constexpr(std::is_member_object_pointer_v<decltype(indirection1)>){
		using U = std::remove_reference_t<decltype(std::declval<V>().*indirection1)>;
		if constexpr(!std::is_pointer_v<U>){// indirection directly to member, ignore isindexed2
			static_assert(sizeof(T) == sizeof(U), "misinterpreted indirection input type");
			if constexpr(1 == sizeof...(varparameters)){// indirection to member with an index
				if constexpr(isaddressingsubdivisable){
					return reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + highbyteoffset + sizeof(T) * splitparameter(varparameters...))->*reinterpret_cast<unsigned char(V:: *)>(indirection1);
				}else{
					std::byte const *pfinal{reinterpret_cast<std::byte const *>(p) + sizeof(T) * splitparameter(varparameters...)};
					return std::pair<uint_least64_t, W>{
						reinterpret_cast<V const *>(pfinal)->*reinterpret_cast<uint_least64_t(V:: *)>(indirection1),
						reinterpret_cast<V const *>(pfinal + sizeof(uint_least64_t))->*reinterpret_cast<W(V:: *)>(indirection1)
					};
				}
			}else if constexpr(0 == sizeof...(varparameters)){// indirection to member without an index
				if constexpr(isaddressingsubdivisable){
					return reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + highbyteoffset)->*reinterpret_cast<unsigned char(V:: *)>(indirection1);
				}else{
					return std::pair<uint_least64_t, W>{
						p->*reinterpret_cast<uint_least64_t(V:: *)>(indirection1),
						reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(uint_least64_t))->*reinterpret_cast<W(V:: *)>(indirection1)
					};
				}
			}else static_assert(false, "impossible first-level indirection indexing parameter count");
		}else{
			static_assert(!std::is_pointer_v<std::remove_pointer_t<U>>, "third level indirection is not supported");
			static_assert(sizeof(T) == sizeof(std::remove_pointer_t<U>), "misinterpreted indirection input type");
			if constexpr(0 == sizeof...(varparameters)){// indirection to member with no indices, ignore isindexed2
				return reinterpret_cast<unsigned char const *>(p->*indirection1);
				//return{*reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>(p->*indirection1) + indirection2)};
			}else if constexpr(1 == sizeof...(varparameters)){// indirection to member with an index
				if constexpr(isindexed2){// second level extra index
					return reinterpret_cast<unsigned char const *>(p->*indirection1);
					//return{reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>(p->*indirection1) + indirection2)[splitparameter(varparameters...)]};
				}else{// first level extra index
					return reinterpret_cast<unsigned char const *>(reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(void *) * splitparameter(varparameters...))->*indirection1);
					//return{*reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>(reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(void *) * splitparameter(varparameters...))->*indirection1) + indirection2)};
				}
			}else if constexpr(2 == sizeof...(varparameters)){// indirection to member with two indices, ignore isindexed2
				std::pair indices{varparameters...};
				return reinterpret_cast<unsigned char const *>(reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(void *) * indices.first)->*indirection1);
				//return{reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>(reinterpret_cast<V const *>(reinterpret_cast<std::byte const *>(p) + sizeof(void *) * indices.first)->*indirection1) + indirection2)[indices.second]};
			}else static_assert(false, "impossible second-level indirection indexing parameter count");
		}
	}else if constexpr(std::is_member_function_pointer_v<decltype(indirection1)>){
		using U = std::invoke_result_t<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>;
		if constexpr(!std::is_pointer_v<U>){// indirection directly to item, ignore isindexed2
			static_assert(sizeof(T) == sizeof(U), "misinterpreted indirection input type");
			U val{(p->*indirection1)(varparameters...)};
			return std::pair<uint_least64_t, W>{
				*reinterpret_cast<uint_least64_t *>(&val),
				reinterpret_cast<W *>(&val)[sizeof(uint_least64_t) / sizeof(W)]
			};
		}else{// indirection to second level pointer
			static_assert(!std::is_pointer_v<std::remove_pointer_t<U>>, "third level indirection is not supported");
			static_assert(sizeof(T) == sizeof(std::remove_pointer_t<U>), "misinterpreted indirection input type");
			if constexpr(isindexed2){// second level extra index
				return reinterpret_cast<unsigned char const *>(splitget<indirection1, V, vararguments...>(p, varparameters...));
				//return{reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>(splitget<indirection1, V, vararguments...>(p, varparameters...)) + indirection2)[splitparameter(varparameters...)]};
			}else{// second level without an index
				return reinterpret_cast<unsigned char const *>((p->*indirection1)(varparameters...));
				//return{*reinterpret_cast<T const *>(reinterpret_cast<unsigned char const *>((p->*indirection1)(varparameters...)) + indirection2)};
			}
		}
	}else static_assert(false, "unsupported indirection input type");
}

// Utility templates to either retrieve the second-level source or pass though results for partial 8-bit outputs
// This variant handles the 80-bit long double types' highest byte.
// Platforms with a native 80-bit long double type are all little endian, hence that is the only implementation here.
template<auto indirection1, bool absolute, bool issigned, bool isfloatingpoint, ptrdiff_t indirection2, bool isindexed2, typename T, typename V, typename... vararguments>
RSBD8_FUNC_INLINE decltype(auto) indirectinputtop2(unsigned char const *pintermediate, vararguments... varparameters)noexcept{
	using W = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t,// the default for all platforms
		std::conditional_t<96 == CHAR_BIT * sizeof(T), uint_least32_t,// only to support the 80-bit long double type with padding (always little endian)
		std::conditional_t<80 == CHAR_BIT * sizeof(T), uint_least16_t, void>>>;// only to support the 80-bit long double type (always little endian)
	static size_t constexpr highbyteoffset{sizeof(longdoubletest80) - 1};
	static bool constexpr isaddressingsubdivisable{
		!(absolute && !issigned) &&// the two tiered absolute types shift bits out of place during filering
#ifdef RSBD8_DISABLE_ADDRESS_SUBDIVISION
		false
#else
		8 == CHAR_BIT &&// optimisation for multi-part addressable machines only
		(std::is_member_object_pointer_v<decltype(indirection1)> ||
		std::is_pointer_v<std::invoke_result_t<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>>)
#endif
	};
	if constexpr(std::is_member_object_pointer_v<decltype(indirection1)>){
		if constexpr(0 == sizeof...(varparameters)){// indirection to member with no indices, ignore isindexed2
			if constexpr(isaddressingsubdivisable){
				return *(pintermediate + indirection2 + highbyteoffset);
			}else{
				unsigned char const *pfinal{pintermediate + indirection2};
				return std::pair<uint_least64_t, W>{
					*reinterpret_cast<uint_least64_t const *>(pfinal),
					*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
				};
			}
		}else if constexpr(1 == sizeof...(varparameters)){// indirection to member with an index
			if constexpr(isindexed2){// second level extra index
				if constexpr(isaddressingsubdivisable){
					return *(pintermediate + indirection2 + highbyteoffset + sizeof(T) * splitparameter(varparameters...));
				}else{
					unsigned char const *pfinal{pintermediate + indirection2 + sizeof(T) * splitparameter(varparameters...)};
					return std::pair<uint_least64_t, W>{
						*reinterpret_cast<uint_least64_t const *>(pfinal),
						*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
					};
				}
			}else{// first level extra index
				if constexpr(isaddressingsubdivisable){
					return *(pintermediate + indirection2 + highbyteoffset);
				}else{
					unsigned char const *pfinal{pintermediate + indirection2};
					return std::pair<uint_least64_t, W>{
						*reinterpret_cast<uint_least64_t const *>(pfinal),
						*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
					};
				}
			}
		}else if constexpr(2 == sizeof...(varparameters)){// indirection to member with two indices, ignore isindexed2
			std::pair indices{varparameters...};
			if constexpr(isaddressingsubdivisable){
				return *(pintermediate + indirection2 + highbyteoffset + sizeof(T) * indices.second);
			}else{
				unsigned char const *pfinal{pintermediate + indirection2 + sizeof(T) * indices.second};
				return std::pair<uint_least64_t, W>{
					*reinterpret_cast<uint_least64_t const *>(pfinal),
					*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
				};
			}
		}else static_assert(false, "impossible second-level indirection indexing parameter count");
	}else if constexpr(std::is_member_function_pointer_v<decltype(indirection1)>){
		if constexpr(isindexed2){// second level extra index
			if constexpr(isaddressingsubdivisable){
				return *(pintermediate + indirection2 + highbyteoffset + sizeof(T) * splitparameter(varparameters...));
			}else{
				unsigned char const *pfinal{pintermediate + indirection2 + sizeof(T) * splitparameter(varparameters...)};
				return std::pair<uint_least64_t, W>{
					*reinterpret_cast<uint_least64_t const *>(pfinal),
					*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
				};
			}
		}else{// second level without an index
			if constexpr(isaddressingsubdivisable){
				return *(pintermediate + indirection2 + highbyteoffset);
			}else{
				unsigned char const *pfinal{pintermediate + indirection2};
				return std::pair<uint_least64_t, W>{
					*reinterpret_cast<uint_least64_t const *>(pfinal),
					*reinterpret_cast<W const *>(pfinal + sizeof(uint_least64_t))
				};
			}
		}
	}else static_assert(false, "unsupported indirection input type");
}
template<auto indirection1, bool absolute, bool issigned, bool isfloatingpoint, ptrdiff_t indirection2, bool isindexed2, typename T, typename V, typename W, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<!std::is_pointer_v<W>,
	W> indirectinputtop2(W passthrough, vararguments...)noexcept{
	return{passthrough};
}

// Utility templates to get either the member object type or the member function return type
template<auto indirection1, bool isindexed2, typename V, typename dummy = void, typename... vararguments> struct memberpointerdeducebody;
// partial specialisation, by std::is_member_function_pointer_v
template<auto indirection1, bool isindexed2, typename V, typename... vararguments>
struct memberpointerdeducebody<indirection1, isindexed2, V, std::enable_if_t<std::is_member_function_pointer_v<decltype(indirection1)>>, vararguments...>{
	using type = std::invoke_result_t<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>;
};
// partial specialisation, by std::is_member_object_pointer_v
template<auto indirection1, bool isindexed2, typename V, typename... vararguments>
struct memberpointerdeducebody<indirection1, isindexed2, V, std::enable_if_t<std::is_member_object_pointer_v<decltype(indirection1)>>, vararguments...>{
	using type = decltype(std::declval<V>().*indirection1);
};
template<auto indirection1, bool isindexed2, typename V, typename... vararguments>
using memberpointerdeduce = typename memberpointerdeducebody<indirection1, isindexed2, V, void, vararguments...>::type;

// Utility template to either pass through a type or allow std::underlying_type to do its work
template<typename T>
using stripenum = typename std::conditional_t<std::is_enum_v<T>, std::underlying_type<T>, std::enable_if<true, T>>::type;

// Utility template to pick an unsigned type of the lowest rank with the same size or allow std::make_unsigned to do its work
// This does not require using stripenum first to work.
template<typename T>
using tounifunsigned = std::conditional_t<1 == sizeof(T), unsigned char,
	std::conditional_t<sizeof(short) == sizeof(T), unsigned short,
	std::conditional_t<sizeof(signed) == sizeof(T), unsigned,
	std::conditional_t<sizeof(long) == sizeof(T), unsigned long,
	std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
	typename std::conditional_t<std::is_integral_v<T> || std::is_enum_v<T>, std::make_unsigned<T>, std::enable_if<true, void>>::type>>>>>;

// Utility template to use add-with-carry operations if possible for the boolean operator less than
RSBD8_FUNC_INLINE void addcarryofless(unsigned &accumulator, size_t minuend, size_t subtrahend)noexcept{
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_subcl) && __has_builtin(__builtin_addc)
	unsigned long carry;
	__builtin_subcl(minuend, subtrahend, 0, &carry);
	unsigned checkcarry;
	accumulator = __builtin_addc(accumulator, 0, static_cast<unsigned>(carry), &checkcarry);
	static_cast<void>(checkcarry);
	assert(!checkcarry);// the chosen accumulator should be big enough to never wrap-around
#elif defined(_M_X64)
	unsigned char checkcarry{_addcarry_u32(_subborrow_u64(0, minuend, subtrahend, nullptr), accumulator, 0, &accumulator)};// cmp r, r followed by adc r, 0
	static_cast<void>(checkcarry);
	assert(!checkcarry);// the chosen accumulator should be big enough to never wrap-around
#elif defined(_M_IX86)
	unsigned char checkcarry{_addcarry_u32(_subborrow_u32(0, minuend, subtrahend, nullptr), accumulator, 0, &accumulator)};// cmp r, r followed by adc r, 0
	static_cast<void>(checkcarry);
	assert(!checkcarry);// the chosen accumulator should be big enough to never wrap-around
#else
	accumulator += minuend < subtrahend;
#endif
}

// Utility template to use add-with-carry operations if possible for the boolean operator less than or equal
RSBD8_FUNC_INLINE void addcarryoflessorequal(unsigned &accumulator, size_t minuend, size_t subtrahend)noexcept{
	// The specialised versions actually calculate greater-than-or-equal, but with everything reversed.
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_subcl) && __has_builtin(__builtin_subc)
	unsigned long carry;
	__builtin_subcl(subtrahend, minuend, 0, &carry);
	unsigned checkcarry;
	accumulator = __builtin_subc(accumulator, ~0u, static_cast<unsigned>(carry), &checkcarry);
	static_cast<void>(checkcarry);
	assert(checkcarry);// the chosen accumulator should be big enough to never wrap-around
#elif defined(_M_X64)
	unsigned char checkcarry{_subborrow_u32(_subborrow_u64(0, subtrahend, minuend, nullptr), accumulator, ~0u, &accumulator)};// cmp r, r followed by sbb r, -1
	static_cast<void>(checkcarry);
	assert(checkcarry);// the chosen accumulator should be big enough to never wrap-around
#elif defined(_M_IX86)
	unsigned char checkcarry{_subborrow_u32(_subborrow_u32(0, subtrahend, minuend, nullptr), accumulator, ~0u, &accumulator)};// cmp r, r followed by sbb r, -1
	static_cast<void>(checkcarry);
	assert(checkcarry);// the chosen accumulator should be big enough to never wrap-around
#else
	accumulator += minuend <= subtrahend;
#endif
}

// Utility template of bit scan forward
template<typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	64 >= CHAR_BIT * sizeof(T) &&
	std::is_integral_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type>,
	unsigned> bitscanforwardportable(T input)noexcept{
	assert(input);// design decision: do not allow 0 as input as neither x86/x64 bsf nor using the de Bruijn sequence supports it
#if defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))
	if constexpr(32 >= CHAR_BIT * sizeof(T)) return{static_cast<unsigned>(__builtin_ctz(input))};
	else return{static_cast<unsigned>(__builtin_ctzll(input))};
#elif defined(_M_X64)
	// will run bsf (bit scan forward) on older architectures, which is fine
	if constexpr(32 >= CHAR_BIT * sizeof(T)) return{_tzcnt_u32(input)};
	else return{static_cast<unsigned>(_tzcnt_u64(input))};
#elif defined(_M_IX86)
	// will run bsf (bit scan forward) on older architectures, which is fine
	if constexpr(32 >= CHAR_BIT * sizeof(T)) return{_tzcnt_u32(input)};
	else{// The 32-bit x86 platform hardly has any 64-bit integer support, so just split the halves.
		// This is still easy to handle in assembly (2 general options, and some instructions can be interleaved in between):
		// tzcnt r, r; add r, 32; tzcnt r, r; cmovc r, r
		// tzcnt r, r; add r, 32; bsf r, r; cmovz r, r
		DWORD hi{32 + _tzcnt_u32(static_cast<DWORD>(input >> 32))}, lo;// will run bsf (bit scan forward) on older architectures, which is fine
		return{_BitScanForward(&lo, static_cast<DWORD>(input & 0xFFFFFFFFu))? lo : hi};
	}
#elif defined(_M_ARM) || defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64) || defined(_M_ARM64EC)
	if constexpr(32 >= CHAR_BIT * sizeof(T)) return{_CountTrailingZeros(input)};
	else return{_CountTrailingZeros64(input)};
#elif defined(__cpp_lib_bitops)
	return{static_cast<unsigned>(std::countr_zero(input))};
#else// Count the consecutive zero bits (trailing) on the right with multiply and lookup
	if constexpr(32 >= CHAR_BIT * sizeof(T)){
		static unsigned char constexpr MultiplyDeBruijnBitPosition[32]{
			0,
			1,
			28, 2,
			29, 14, 24, 3,
			30, 22, 20, 15, 25, 17, 4, 8,
			31, 27, 13, 23, 21, 19, 16, 7,
			26, 12, 18, 6,
			11, 5,
			10,
			9};
		return{MultiplyDeBruijnBitPosition[static_cast<std::make_unsigned_t<T>>(input & -static_cast<std::make_signed_t<T>>(input)) * 0x077CB531u >> 27]};
	}else{
		static unsigned char constexpr MultiplyDeBruijnBitPosition[64]{
			0,
			1,
			17, 2,
			18, 50, 3, 57,
			47, 19, 22, 51, 29, 4, 33, 58,
			15, 48, 20, 27, 25, 23, 52, 41, 54, 30, 38, 5, 43, 34, 59, 8,
			63, 16, 49, 56, 46, 21, 28, 32, 14, 26, 24, 40, 53, 37, 42, 7,
			62, 55, 45, 31, 13, 39, 36, 6,
			61, 44, 12, 35,
			60, 11,
			10,
			9};
		return{MultiplyDeBruijnBitPosition[static_cast<std::make_unsigned_t<T>>(input & -static_cast<std::make_signed_t<T>>(input)) * 0x37E84A99DAE458Fu >> 58]};
	}
#endif
}

// Utility template of rotate left by a compile-time constant amount
template<unsigned char amount, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	64 >= CHAR_BIT * sizeof(T) &&
	std::is_integral_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type>,
	T> rotateleftportable(T input)noexcept{
#if defined(_M_IX86) || defined(_M_X64) || defined(_M_ARM) || defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64) || defined(_M_ARM64EC)
	if constexpr(64 == CHAR_BIT * sizeof(T)) return{_rotl64(input, amount)};
	else if constexpr(32 == CHAR_BIT * sizeof(T)) return{_rotl(input, amount)};
	else if constexpr(16 == CHAR_BIT * sizeof(T)) return{_rotl16(input, amount)};
	else if constexpr(8 == CHAR_BIT * sizeof(T)) return{_rotl8(input, amount)};
	else static_assert(false, "Implementing larger types will require additional work and optimisation for this library.");
#elif defined(__cpp_lib_bitops)
	return{std::rotl(input, amount)};
#else// revert to shifting and combining
	// Given that T might be smaller than type "int", prevent the undesired integral promotion when following C/C++ rules here.
	// By far most compilers can optimise this to a single instruction.
	T copy{input};
	input >>= CHAR_BIT * sizeof(T) - amount;
	copy <<= amount;
	input |= copy;
	return{input};
#endif
}

// Utility template of rotate right by a compile-time constant amount
template<unsigned char amount, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	64 >= CHAR_BIT * sizeof(T) &&
	std::is_integral_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type>,
	T> rotaterightportable(T input)noexcept{
#if defined(_M_IX86) || defined(_M_X64) || defined(_M_ARM) || defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64) || defined(_M_ARM64EC)
	if constexpr(64 == CHAR_BIT * sizeof(T)) return{_rotr64(input, amount)};
	else if constexpr(32 == CHAR_BIT * sizeof(T)) return{_rotr(input, amount)};
	else if constexpr(16 == CHAR_BIT * sizeof(T)) return{_rotr16(input, amount)};
	else if constexpr(8 == CHAR_BIT * sizeof(T)) return{_rotr8(input, amount)};
	else static_assert(false, "Implementing larger types will require additional work and optimisation for this library.");
#elif defined(__cpp_lib_bitops)
	return{std::rotr(input, amount)};
#else// revert to shifting and combining
	// Given that T might be smaller than type "int", prevent the undesired integral promotion when following C/C++ rules here.
	// By far most compilers can optimise this to a single instruction.
	T copy{input};
	input <<= CHAR_BIT * sizeof(T) - amount;
	copy >>= amount;
	input |= copy;
	return{input};
#endif
}

// Helper functions to implement the 8 main modes
// The filtertop8() and filtershift8() template functions are customized for the sorting phase, and have no need for variants with pointers.
// These also only output size_t (or a pair of them) for direct use as indices.
// The filterinput() template functions modify their inputs and each has a variant that write their inputs to memory either once or twice.
// There are 5 of these, handling 1, 2, 3, 4 or 8 inputs.
// Each of these have two varians that take one or two pointers per input to store each input before modification.
// = modes with no filtering here:
// --- regular unsigned integer and also inside-out signed integer
// --- regular signed integer
// = modes with one-pass filtering here:
// --- absolute floating-point and also unsigned integer without using the top bit
// --- absolute floating-point, but negative inputs will sort just below their positive counterparts
// = modes with two-pass filtering here:
// --- regular floating point
// --- inside-out floating-point
// --- absolute signed integer
// --- absolute signed integer, but negative inputs will sort just below their positive counterparts

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	(!std::is_same_v<unsigned char, T> ||
	8 < CHAR_BIT) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	size_t> filtertop8(U cur)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curp{static_cast<std::make_signed_t<T>>(cur)};
		if constexpr(isfloatingpoint || !issigned){
			T curo{static_cast<T>(cur)};
			curo += curo;
			cur = curo;
		}
		curp >>= CHAR_BIT * sizeof(T) - 1;
		U curq{static_cast<T>(curp)};
		if constexpr(isfloatingpoint) cur >>= CHAR_BIT * sizeof(T) - 8 + 1;
		else if constexpr(issigned){
			T curo{static_cast<T>(cur)};
			curo += static_cast<T>(curq);
			cur = curo;
		}
		cur ^= curq;
		if constexpr(8 < CHAR_BIT * sizeof(T)){
			if constexpr(isfloatingpoint) return{static_cast<size_t>(cur & 0xFFu)};
			else{
				cur >>= CHAR_BIT * sizeof(T) - 8;
				return{static_cast<size_t>(cur)};
			}
		}else return{static_cast<size_t>(cur)};
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		if constexpr(8 < CHAR_BIT * sizeof(T)){
			cur >>= CHAR_BIT * sizeof(T) - 8 - !issigned;
			return{static_cast<size_t>(cur & 0xFFu >> issigned)};
		}else if constexpr(issigned){
			return{static_cast<size_t>(cur & 0x7Fu)};
		}else{
			cur = rotateleftportable<1>(static_cast<T>(cur));
			return{static_cast<size_t>(cur)};
		}
	}else if constexpr(8 < CHAR_BIT * sizeof(T)){
		cur >>= CHAR_BIT * sizeof(T) - 8;
		return{static_cast<size_t>(cur)};
	}else return{static_cast<size_t>(cur)};
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_same_v<unsigned char, T> &&
	8 == CHAR_BIT &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	size_t> filtertop8(U cur)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		signed char curp{static_cast<signed char>(cur)};
		static_assert(!(absolute && !issigned), "impossible combination of partwise filtering and tiered absolute mode");
		curp >>= 8 - 1;
		U curq{static_cast<unsigned char>(curp)};
		if constexpr(isfloatingpoint) cur &= 0x7Fu;
		else if constexpr(issigned){
			T curo{static_cast<unsigned char>(cur)};
			curo += static_cast<unsigned char>(curq);
			cur = curo;
		}
		cur ^= curq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		static_assert(!issigned, "impossible combination of partwise filtering and tiered absolute mode");
		cur &= 0x7Fu;
	}
	return{static_cast<size_t>(cur)};
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	(!std::is_same_v<unsigned char, T> ||
	8 < CHAR_BIT) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	std::pair<size_t, size_t>> filtertop8(U cura, U curb)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		if constexpr(isfloatingpoint || !issigned){
			T curao{static_cast<T>(cura)};
			curao += curao;
			cura = curao;
		}
		curap >>= CHAR_BIT * sizeof(T) - 1;
		U curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		if constexpr(isfloatingpoint || !issigned){
			T curbo{static_cast<T>(curb)};
			curbo += curbo;
			curb = curbo;
		}
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		U curbq{static_cast<T>(curbp)};
		if constexpr(isfloatingpoint){
			cura >>= CHAR_BIT * sizeof(T) - 8 + 1;
			curb >>= CHAR_BIT * sizeof(T) - 8 + 1;
		}else if constexpr(issigned){
			T curao{static_cast<T>(cura)};
			T curbo{static_cast<T>(curb)};
			curao += static_cast<T>(curaq);
			curbo += static_cast<T>(curbq);
			cura = curao;
			curb = curbo;
		}
		cura ^= curaq;
		curb ^= curbq;
		if constexpr(8 < CHAR_BIT * sizeof(T)){
			if constexpr(isfloatingpoint){
				return{static_cast<size_t>(cura & 0xFFu), static_cast<size_t>(curb & 0xFFu)};
			}else{
				cura >>= CHAR_BIT * sizeof(T) - 8;
				curb >>= CHAR_BIT * sizeof(T) - 8;
				return{static_cast<size_t>(cura), static_cast<size_t>(curb)};
			}
		}else return{static_cast<size_t>(cura), static_cast<size_t>(curb)};
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		if constexpr(8 < CHAR_BIT * sizeof(T)){
			cura >>= CHAR_BIT * sizeof(T) - 8 - !issigned;
			curb >>= CHAR_BIT * sizeof(T) - 8 - !issigned;
			return{static_cast<size_t>(cura & 0xFFu >> issigned), static_cast<size_t>(curb & 0xFFu >> issigned)};
		}else if constexpr(issigned){
			return{static_cast<size_t>(cura & 0x7Fu), static_cast<size_t>(curb & 0x7Fu)};
		}else{
			cura = rotateleftportable<1>(static_cast<T>(cura));
			curb = rotateleftportable<1>(static_cast<T>(curb));
			return{static_cast<size_t>(cura), static_cast<size_t>(curb)};
		}
	}else if constexpr(8 < CHAR_BIT * sizeof(T)){
		cura >>= CHAR_BIT * sizeof(T) - 8;
		curb >>= CHAR_BIT * sizeof(T) - 8;
		return{static_cast<size_t>(cura), static_cast<size_t>(curb)};
	}else return{static_cast<size_t>(cura), static_cast<size_t>(curb)};
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_same_v<unsigned char, T> &&
	8 == CHAR_BIT &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	std::pair<size_t, size_t>> filtertop8(U cura, U curb)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		signed char curpa{static_cast<signed char>(cura)};
		static_assert(!(absolute && !issigned), "impossible combination of partwise filtering and tiered absolute mode");
		curpa >>= 8 - 1;
		U curqa{static_cast<unsigned char>(curpa)};
		signed char curpb{static_cast<signed char>(curb)};
		curpb >>= 8 - 1;
		U curqb{static_cast<unsigned char>(curpb)};
		if constexpr(isfloatingpoint){
			cura &= 0x7Fu;
			curb &= 0x7Fu;
		}else if constexpr(issigned){
			T curoa{static_cast<unsigned char>(cura)};
			curoa += static_cast<unsigned char>(curqa);
			cura = curoa;
			T curob{static_cast<unsigned char>(curb)};
			curob += static_cast<unsigned char>(curqb);
			curb = curob;
		}
		cura ^= curqa;
		curb ^= curqb;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		static_assert(!issigned, "impossible combination of partwise filtering and tiered absolute mode");
		cura &= 0x7Fu;
		curb &= 0x7Fu;
	}
	return{static_cast<size_t>(cura), static_cast<size_t>(curb)};
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<longdoubletest128, T> ||
	std::is_same_v<longdoubletest96, T> ||
	std::is_same_v<longdoubletest80, T> ||
	std::is_same_v<long double, T> &&
	64 == LDBL_MANT_DIG &&
	16384 == LDBL_MAX_EXP &&
	128 >= CHAR_BIT * sizeof(long double) &&
	64 < CHAR_BIT * sizeof(long double)) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U) &&
	8 < CHAR_BIT * sizeof(U),
	size_t> filterbelowtop8(uint_least64_t curm, U cure)noexcept{
	// Filtering is simplified if possible.
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		int_least16_t curp{static_cast<int_least16_t>(cure)};
		if constexpr(absolute && !issigned){
			uint_least16_t curo{static_cast<uint_least16_t>(cure)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carry;
			__builtin_addcll(curm, curm, 0, &carry);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carry;
			__builtin_addcl(curm, curm, 0, &carry);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carry;
			uint_least32_t curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			__builtin_addcl(curmhi, curmhi, 0, &carry);
#endif
			unsigned short checkcarry;
			curo = __builtin_addcs(curo, curo, static_cast<unsigned short>(carry), &checkcarry);
			static_cast<void>(checkcarry);
#elif defined(_M_X64)
			unsigned char checkcarry{_addcarry_u16(_addcarry_u64(0, curm, curm, nullptr), curo, curo, &curo)};
			static_cast<void>(checkcarry);
#elif defined(_M_IX86)
			uint_least32_t curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			unsigned char checkcarry{_addcarry_u16(_addcarry_u32(0, curmhi, curmhi, nullptr), curo, curo, &curo)};
			static_cast<void>(checkcarry);
#else
			uint_least64_t curmtmp{curm};
			curm += curm;
			curo += curo;
			curo += curm < curmtmp;
#endif
			cure = curo;
		}
		curp >>= 16 - 1;
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
		uint_least64_t curq{static_cast<uint_least64_t>(curp)};// sign-extend
#else
		uint_least32_t curq{static_cast<uint_least32_t>(curp)};// sign-extend
#endif
		if constexpr(!isfloatingpoint && issigned){
			uint_least16_t curo{static_cast<uint_least16_t>(cure)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carry;
			__builtin_addcll(curm, curq, 0, &carry);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carry;
			__builtin_addcl(curm, curq, 0, &carry);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carry;
			uint_least32_t curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			__builtin_addcl(curmhi, curq, 0, &carry);
#endif
			unsigned short checkcarry;
			curo = __builtin_addcs(curo, static_cast<unsigned short>(curq), static_cast<unsigned short>(carry), &checkcarry);
			static_cast<void>(checkcarry);
#elif defined(_M_X64)
			unsigned char checkcarry{_addcarry_u16(_addcarry_u64(0, curm, curq, nullptr), curo, static_cast<uint_least16_t>(curq), &curo)};
			static_cast<void>(checkcarry);
#elif defined(_M_IX86)
			uint_least32_t curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			unsigned char checkcarry{_addcarry_u16(_addcarry_u32(0, curmhi, curq, nullptr), curo, static_cast<uint_least16_t>(curq), &curo)};
			static_cast<void>(checkcarry);
#else
			uint_least64_t curmtmp{curm};
			curm += curq;
			curo += static_cast<uint_least16_t>(curq);
			curo += curm < curmtmp || curm < curq;
#endif
			cure = curo;
		}
		cure ^= static_cast<U>(curq);
	}else if constexpr(isfloatingpoint && absolute && !issigned){// one-register filtering
		uint_least16_t curo{static_cast<uint_least16_t>(cure)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
		static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
		unsigned long long carry;
		__builtin_addcll(curm, curm, 0, &carry);
#else
		static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
		unsigned long carry;
		__builtin_addcl(curm, curm, 0, &carry);
#endif
#else
		static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
		unsigned long carry;
		uint_least32_t curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
		__builtin_addcl(curmhi, curmhi, 0, &carry);
#endif
		unsigned short checkcarry;
		curo = __builtin_addcs(curo, curo, static_cast<unsigned short>(carry), &checkcarry);
		static_cast<void>(checkcarry);
#elif defined(_M_X64)
		unsigned char checkcarry{_addcarry_u16(_addcarry_u64(0, curm, curm, nullptr), curo, curo, &curo)};
		static_cast<void>(checkcarry);
#elif defined(_M_IX86)
		uint_least32_t curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
		unsigned char checkcarry{_addcarry_u16(_addcarry_u32(0, curmhi, curmhi, nullptr), curo, curo, &curo)};
		static_cast<void>(checkcarry);
#else
		uint_least64_t curmtmp{curm};
		curm += curm;
		curo += curo;
		curo += curm < curmtmp;
#endif
		cure = curo;
	}
	return{static_cast<size_t>(cure & 0xFFu)};
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<longdoubletest128, T> ||
	std::is_same_v<longdoubletest96, T> ||
	std::is_same_v<longdoubletest80, T> ||
	std::is_same_v<long double, T> &&
	64 == LDBL_MANT_DIG &&
	16384 == LDBL_MAX_EXP &&
	128 >= CHAR_BIT * sizeof(long double) &&
	64 < CHAR_BIT * sizeof(long double)) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U) &&
	8 < CHAR_BIT * sizeof(U),
	size_t> filterbelowtop8(U cure)noexcept{
	// Filtering is simplified if possible.
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		int_least16_t curp{static_cast<int_least16_t>(cure)};
		if constexpr(absolute && !issigned){
			uint_least16_t curo{static_cast<uint_least16_t>(cure)};
			curo += curo;
			cure = curo;
		}
		curp >>= 16 - 1;
		U curq{static_cast<uint_least16_t>(curp)};
		if constexpr(!isfloatingpoint && issigned){
			uint_least16_t curo{static_cast<uint_least16_t>(cure)};
			curo += static_cast<uint_least16_t>(curq);
			cure = curo;
		}
		cure ^= curq;
	}else if constexpr(isfloatingpoint && absolute && !issigned){// one-register filtering
		uint_least16_t curo{static_cast<uint_least16_t>(cure)};
		curo += curo;
		cure = curo;
	}
	return{static_cast<size_t>(cure & 0xFFu)};
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U, typename W>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<longdoubletest128, T> ||
	std::is_same_v<longdoubletest96, T> ||
	std::is_same_v<longdoubletest80, T> ||
	std::is_same_v<long double, T> &&
	64 == LDBL_MANT_DIG &&
	16384 == LDBL_MAX_EXP &&
	128 >= CHAR_BIT * sizeof(long double) &&
	64 < CHAR_BIT * sizeof(long double)) &&
	8 == CHAR_BIT &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U) &&
	8 < CHAR_BIT * sizeof(U) &&
	std::is_same_v<unsigned char, W>,
	size_t> filterbelowtop8(W cure)noexcept{
	// Filtering is simplified if possible.
	static_assert((!isfloatingpoint && !absolute && !issigned) ||
		(!isfloatingpoint && !absolute && issigned) ||
		(isfloatingpoint && absolute && issigned), "impossible combination of partwise filtering and the wrong type of filter");
	return{static_cast<size_t>(cure)};// passthrough, but ensure zero-extension
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<longdoubletest128, T> ||
	std::is_same_v<longdoubletest96, T> ||
	std::is_same_v<longdoubletest80, T> ||
	std::is_same_v<long double, T> &&
	64 == LDBL_MANT_DIG &&
	16384 == LDBL_MAX_EXP &&
	128 >= CHAR_BIT * sizeof(long double) &&
	64 < CHAR_BIT * sizeof(long double)) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U) &&
	8 < CHAR_BIT * sizeof(U),
	std::pair<size_t, size_t>> filterbelowtop8(uint_least64_t curma, U curea, uint_least64_t curmb, U cureb)noexcept{
	// Filtering is simplified if possible.
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		int_least16_t curpa{static_cast<int_least16_t>(curea)};
		int_least16_t curpb{static_cast<int_least16_t>(cureb)};
		if constexpr(absolute && !issigned){
			uint_least16_t curoa{static_cast<uint_least16_t>(curea)};
			uint_least16_t curob{static_cast<uint_least16_t>(cureb)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carrya;
			__builtin_addcll(curma, curma, 0, &carrya);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrya;
			__builtin_addcl(curma, curma, 0, &carrya);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrya;
			uint_least32_t curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			__builtin_addcl(curmhia, curmhia, 0, &carrya);
#endif
			unsigned short checkcarrya;
			curoa = __builtin_addcs(curoa, curoa, static_cast<unsigned short>(carrya), &checkcarrya);
			static_cast<void>(checkcarrya);
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			unsigned long long carryb;
			__builtin_addcll(curmb, curmb, 0, &carryb);
#else
			unsigned long carryb;
			__builtin_addcl(curmb, curmb, 0, &carryb);
#endif
#else
			unsigned long carryb;
			uint_least32_t curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			__builtin_addcl(curmhib, curmhib, 0, &carryb);
#endif
			unsigned short checkcarryb;
			curob = __builtin_addcs(curob, curob, static_cast<unsigned short>(carryb), &checkcarryb);
			static_cast<void>(checkcarryb);
#elif defined(_M_X64)
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u64(0, curma, curma, nullptr), curoa, curoa, &curoa)};
			static_cast<void>(checkcarrya);
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u64(0, curmb, curmb, nullptr), curob, curob, &curob)};
			static_cast<void>(checkcarryb);
#elif defined(_M_IX86)
			uint_least32_t curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u32(0, curmhia, curmhia, nullptr), curoa, curoa, &curoa)};
			static_cast<void>(checkcarrya);
			uint_least32_t curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u32(0, curmhib, curmhib, nullptr), curob, curob, &curob)};
			static_cast<void>(checkcarryb);
#else
			uint_least64_t curmtmpa{curma}, curmtmpb{curmb};
			curma += curma;
			curoa += curoa;
			curoa += curma < curmtmpa;
			curmb += curmb;
			curob += curob;
			curob += curmb < curmtmpb;
#endif
			curea = curoa;
			cureb = curob;
		}
		curpa >>= 16 - 1;
		curpb >>= 16 - 1;
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
		uint_least64_t curqa{static_cast<uint_least64_t>(curpa)};// sign-extend
		uint_least64_t curqb{static_cast<uint_least64_t>(curpb)};
#else
		uint_least32_t curqa{static_cast<uint_least32_t>(curpa)};// sign-extend
		uint_least32_t curqb{static_cast<uint_least32_t>(curpb)};
#endif
		if constexpr(!isfloatingpoint && issigned){
			uint_least16_t curoa{static_cast<uint_least16_t>(curea)};
			uint_least16_t curob{static_cast<uint_least16_t>(cureb)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carrya;
			__builtin_addcll(curma, curqa, 0, &carrya);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrya;
			__builtin_addcl(curma, curqa, 0, &carrya);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrya;
			uint_least32_t curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			__builtin_addcl(curmhia, curqa, 0, &carrya);
#endif
			unsigned short checkcarrya;
			curoa = __builtin_addcs(curoa, static_cast<unsigned short>(curqa), static_cast<unsigned short>(carrya), &checkcarrya);
			static_cast<void>(checkcarrya);
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			unsigned long long carryb;
			__builtin_addcll(curmb, curqb, 0, &carryb);
#else
			unsigned long carryb;
			__builtin_addcl(curmb, curqb, 0, &carryb);
#endif
#else
			unsigned long carryb;
			uint_least32_t curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			__builtin_addcl(curmhib, curqb, 0, &carryb);
#endif
			unsigned short checkcarryb;
			curob = __builtin_addcs(curob, static_cast<unsigned short>(curqb), static_cast<unsigned short>(carryb), &checkcarryb);
			static_cast<void>(checkcarryb);
#elif defined(_M_X64)
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u64(0, curma, curqa, nullptr), curoa, static_cast<uint_least16_t>(curqa), &curoa)};
			static_cast<void>(checkcarrya);
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u64(0, curmb, curqb, nullptr), curob, static_cast<uint_least16_t>(curqb), &curob)};
			static_cast<void>(checkcarryb);
#elif defined(_M_IX86)
			uint_least32_t curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u32(0, curmhia, curqa, nullptr), curoa, static_cast<uint_least16_t>(curqa), &curoa)};
			static_cast<void>(checkcarrya);
			uint_least32_t curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u32(0, curmhib, curqb, nullptr), curob, static_cast<uint_least16_t>(curqb), &curob)};
			static_cast<void>(checkcarryb);
#elif 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
			uint_least64_t curmtmpa{curma}, curmtmpb{curmb};
			curma += curqa;
			curoa += static_cast<uint_least16_t>(curqa);
			curoa += curma < curmtmpa || curma < curqa;
			curmb += curqb;
			curob += static_cast<uint_least16_t>(curqb);
			curob += curmb < curmtmpb || curmb < curqb;
#else
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			uint_least32_t curmlotmpa{curmloa}, curmhitmpa{curmhia};
			uint_least32_t curmlotmpb{curmlob}, curmhitmpb{curmhib};
			curmloa += curqa;
			curmhia += curqa;
			curmhia += curmloa < curmlotmpa || curmloa < curqa;
			curoa += static_cast<uint_least16_t>(curqa);
			curoa += curmhia < curmhitmpa || curmhia < curqa;
			curmlob += curqb;
			curmhib += curqb;
			curmhib += curmlob < curmlotmpb || curmlob < curqb;
			curob += static_cast<uint_least16_t>(curqb);
			curob += curmhib < curmhitmpb || curmhib < curqb;
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#endif
			curea = curoa;
			cureb = curob;
		}
		curea ^= static_cast<U>(curqa);
		cureb ^= static_cast<U>(curqb);
	}else if constexpr(isfloatingpoint && absolute && !issigned){// one-register filtering
		uint_least16_t curoa{static_cast<uint_least16_t>(curea)};
		uint_least16_t curob{static_cast<uint_least16_t>(cureb)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
		static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
		unsigned long long carrya;
		__builtin_addcll(curma, curma, 0, &carrya);
#else
		static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
		unsigned long carrya;
		__builtin_addcl(curma, curma, 0, &carrya);
#endif
#else
		static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
		unsigned long carrya;
		uint_least32_t curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
		__builtin_addcl(curmhia, curmhia, 0, &carrya);
#endif
		unsigned short checkcarrya;
		curoa = __builtin_addcs(curoa, curoa, static_cast<unsigned short>(carrya), &checkcarrya);
		static_cast<void>(checkcarrya);
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
		unsigned long long carryb;
		__builtin_addcll(curmb, curmb, 0, &carryb);
#else
		static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
		unsigned long carryb;
		__builtin_addcl(curmb, curmb, 0, &carryb);
#endif
#else
		static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
		unsigned long carryb;
		uint_least32_t curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
		__builtin_addcl(curmhib, curmhib, 0, &carryb);
#endif
		unsigned short checkcarryb;
		curob = __builtin_addcs(curob, curob, static_cast<unsigned short>(carryb), &checkcarryb);
		static_cast<void>(checkcarryb);
#elif defined(_M_X64)
		unsigned char checkcarrya{_addcarry_u16(_addcarry_u64(0, curma, curma, nullptr), curoa, curoa, &curoa)};
		static_cast<void>(checkcarrya);
		unsigned char checkcarryb{_addcarry_u16(_addcarry_u64(0, curmb, curmb, nullptr), curob, curob, &curob)};
		static_cast<void>(checkcarryb);
#elif defined(_M_IX86)
		uint_least32_t curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
		unsigned char checkcarrya{_addcarry_u16(_addcarry_u32(0, curmhia, curmhia, nullptr), curoa, curoa, &curoa)};
		static_cast<void>(checkcarrya);
		uint_least32_t curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
		unsigned char checkcarryb{_addcarry_u16(_addcarry_u32(0, curmhib, curmhib, nullptr), curob, curob, &curob)};
		static_cast<void>(checkcarryb);
#else
		uint_least64_t curmtmpa{curma}, curmtmpb{curmb};
		curma += curma;
		curoa += curoa;
		curoa += curma < curmtmpa;
		curmb += curmb;
		curob += curob;
		curob += curmb < curmtmpb;
#endif
		curea = curoa;
		cureb = curob;
	}
	return{static_cast<size_t>(curea & 0xFFu), static_cast<size_t>(cureb & 0xFFu)};
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<longdoubletest128, T> ||
	std::is_same_v<longdoubletest96, T> ||
	std::is_same_v<longdoubletest80, T> ||
	std::is_same_v<long double, T> &&
	64 == LDBL_MANT_DIG &&
	16384 == LDBL_MAX_EXP &&
	128 >= CHAR_BIT * sizeof(long double) &&
	64 < CHAR_BIT * sizeof(long double)) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U) &&
	8 < CHAR_BIT * sizeof(U),
	std::pair<size_t, size_t>> filterbelowtop8(U curea, U cureb)noexcept{
	// Filtering is simplified if possible.
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		int_least16_t curpa{static_cast<int_least16_t>(curea)};
		int_least16_t curpb{static_cast<int_least16_t>(cureb)};
		if constexpr(absolute && !issigned){
			uint_least16_t curoa{static_cast<uint_least16_t>(curea)};
			curoa += curoa;
			curea = curoa;
			uint_least16_t curob{static_cast<uint_least16_t>(cureb)};
			curob += curob;
			cureb = curob;
		}
		curpa >>= 16 - 1;
		U curqa{static_cast<uint_least16_t>(curpa)};
		curpb >>= 16 - 1;
		U curqb{static_cast<uint_least16_t>(curpb)};
		if constexpr(!isfloatingpoint && issigned){
			uint_least16_t curoa{static_cast<uint_least16_t>(curea)};
			curoa += static_cast<uint_least16_t>(curqa);
			curea = curoa;
			uint_least16_t curob{static_cast<uint_least16_t>(cureb)};
			curob += static_cast<uint_least16_t>(curqb);
			cureb = curob;
		}
		curea ^= curqa;
		cureb ^= curqb;
	}else if constexpr(isfloatingpoint && absolute && !issigned){// one-register filtering
		uint_least16_t curoa{static_cast<uint_least16_t>(curea)};
		curoa += curoa;
		curea = curoa;
		uint_least16_t curob{static_cast<uint_least16_t>(cureb)};
		curob += curob;
		cureb = curob;
	}
	return{static_cast<size_t>(curea & 0xFFu), static_cast<size_t>(cureb & 0xFFu)};
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U, typename W>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<longdoubletest128, T> ||
	std::is_same_v<longdoubletest96, T> ||
	std::is_same_v<longdoubletest80, T> ||
	std::is_same_v<long double, T> &&
	64 == LDBL_MANT_DIG &&
	16384 == LDBL_MAX_EXP &&
	128 >= CHAR_BIT * sizeof(long double) &&
	64 < CHAR_BIT * sizeof(long double)) &&
	8 == CHAR_BIT &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U) &&
	8 < CHAR_BIT * sizeof(U) &&
	std::is_same_v<unsigned char, W>,
	std::pair<size_t, size_t>> filterbelowtop8(W curea, W cureb)noexcept{
	// Filtering is simplified if possible.
	static_assert((!isfloatingpoint && !absolute && !issigned) ||
		(!isfloatingpoint && !absolute && issigned) ||
		(isfloatingpoint && absolute && issigned), "impossible combination of partwise filtering and the wrong type of filter");
	return{static_cast<size_t>(curea), static_cast<size_t>(cureb)};// passthrough, but ensure zero-extension
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U) &&
	8 < CHAR_BIT * sizeof(U),
	size_t> filtershift8(U cur, unsigned shift)noexcept{
	// Filtering is simplified if possible.
	// This should never filter the top part for non-absolute floating-point inputs.
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curp{static_cast<std::make_signed_t<T>>(cur)};
		if constexpr(absolute && !issigned){
			T curo{static_cast<T>(cur)};
			curo += curo;
			cur = curo;
		}
		curp >>= CHAR_BIT * sizeof(T) - 1;
		U curq{static_cast<T>(curp)};
		if constexpr(!isfloatingpoint && issigned){
			T curo{static_cast<T>(cur)};
			curo += static_cast<T>(curq);
			cur = curo;
		}
		cur ^= curq;
	}else if constexpr(isfloatingpoint && absolute && !issigned){// one-register filtering
		cur = rotateleftportable<1>(static_cast<T>(cur));
	}
	cur >>= shift;
	return{static_cast<size_t>(cur & 0xFFu)};
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<longdoubletest128, T> ||
		std::is_same_v<longdoubletest96, T> ||
		std::is_same_v<longdoubletest80, T> ||
		std::is_same_v<long double, T> &&
		64 == LDBL_MANT_DIG &&
		16384 == LDBL_MAX_EXP &&
		128 >= CHAR_BIT * sizeof(long double) &&
		64 < CHAR_BIT * sizeof(long double)) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U) &&
	8 < CHAR_BIT * sizeof(U),
	size_t> filtershift8(uint_least64_t curm, U cure, unsigned shift)noexcept{
	// Filtering is simplified if possible.
	// This should never filter the top 16 bits.
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		int_least16_t curp{static_cast<int_least16_t>(cure)};
		if constexpr(absolute && !issigned) curm += curm;
		curp >>= 16 - 1;
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
		uint_least64_t curq{static_cast<uint_least64_t>(curp)};// sign-extend
#else
		uint_least32_t curq{static_cast<uint_least32_t>(curp)};// sign-extend
#endif
		if constexpr(!isfloatingpoint && issigned){
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
			curm += curq;
#elif (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl)
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrymid, checkcarry;
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			curmlo = __builtin_addcl(curmlo, curmlo, 0, &carrymid);
			curmhi = __builtin_addcl(curmhi, curmhi, carrymid, &checkcarry);
			static_cast<void>(checkcarry);
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#elif defined(_M_IX86)
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			unsigned char checkcarry{_addcarry_u32(_addcarry_u32(0, curmlo, curmlo, &curmlo), curmhi, curmhi, &curmhi)};
			static_cast<void>(checkcarry);
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#else
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			uint_least32_t curmlotmp{curmlo};
			curmlo += curq;
			curmhi += curq;
			curmhi += curmlo < curmlotmp || curmlo < curq;
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#endif
		}
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
		curm ^= curq;
#else
		uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
		curmlo ^= curq;
		curmhi ^= curq;
		alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
		curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#endif
	}else if constexpr(isfloatingpoint && absolute && !issigned){// one-register filtering
		uint_least16_t curo{static_cast<uint_least16_t>(cure)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
		static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
		unsigned short carrysign;
		curo = __builtin_addcs(curo, curo, 0, &carrysign);
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
		unsigned long long checkcarry;
		curm = __builtin_addcll(curm, curm, static_cast<unsigned long long>(carrysign), &checkcarry);
#else
		static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
		unsigned long checkcarry;
		curm = __builtin_addcl(curm, curm, static_cast<unsigned long>(carrysign), &checkcarry);
#endif
		static_cast<void>(checkcarry);
#else
		static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
		unsigned long carrymid, checkcarry;
		uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
		curmlo = __builtin_addcl(curmlo, curmlo, static_cast<unsigned long>(carrysign), &carrymid);
		curmhi = __builtin_addcl(curmhi, curmhi, carrymid, &checkcarry);
		static_cast<void>(checkcarry);
		alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
		curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#endif
#elif defined(_M_X64)
		unsigned char checkcarry{_addcarry_u64(_addcarry_u16(0, curo, curo, &curo), curm, curm, &curm)};
		static_cast<void>(checkcarry);
#elif defined(_M_IX86)
		uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
		unsigned char checkcarry{_addcarry_u32(_addcarry_u32(_addcarry_u16(0, curo, curo, &curo), curmlo, curmlo, &curmlo), curmhi, curmhi, &curmhi)};
		static_cast<void>(checkcarry);
		alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
		curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#else
		uint_least16_t curotmp{curo};
		curo += curo;
		curm += curm;
		curm += curo < curotmp;
#endif
	}
	curm >>= shift;
	return{static_cast<size_t>(curm & 0xFFu)};
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U, typename W>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<longdoubletest128, T> ||
	std::is_same_v<longdoubletest96, T> ||
	std::is_same_v<longdoubletest80, T> ||
	std::is_same_v<long double, T> &&
	64 == LDBL_MANT_DIG &&
	16384 == LDBL_MAX_EXP &&
	128 >= CHAR_BIT * sizeof(long double) &&
	64 < CHAR_BIT * sizeof(long double)) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U) &&
	8 < CHAR_BIT * sizeof(U) &&
	std::is_unsigned_v<W> &&
	64 >= CHAR_BIT * sizeof(W) &&
	8 < CHAR_BIT * sizeof(W),
	size_t> filtershift8(std::pair<uint_least64_t, W> cur, unsigned shift)noexcept{
	// Use the function above.
	return{filtershift8<absolute, issigned, isfloatingpoint, T, U>(cur.first, cur.second, shift)};
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_same_v<unsigned char, T> &&
	8 == CHAR_BIT &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U) &&
	8 < CHAR_BIT * sizeof(U),
	size_t> filtershift8(U cur, unsigned)noexcept{
	// Filtering is simplified if possible.
	// This should never filter the top part for non-absolute floating-point inputs.
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		signed char curp{static_cast<signed char>(cur)};
		static_assert(!(absolute && !issigned), "impossible combination of partwise filtering and tiered absolute mode");
		curp >>= 8 - 1;
		U curq{static_cast<unsigned char>(curp)};
		if constexpr(!isfloatingpoint && issigned){
			unsigned char curo{static_cast<unsigned char>(cur)};
			curo += static_cast<unsigned char>(curq);
			cur = curo;
		}
		cur ^= curq;
	}
	static_assert(!(isfloatingpoint && absolute && !issigned), "impossible combination of partwise filtering and tiered absolute mode");// one-register filtering
	return{static_cast<size_t>(cur)};
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	8 == CHAR_BIT &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U) &&
	8 < CHAR_BIT * sizeof(U),
	size_t> filtershift8(std::pair<unsigned char, unsigned char> set, unsigned)noexcept{
	// Filtering is simplified if possible.
	// This should never filter the top part for non-absolute floating-point inputs.
	U cur{set.first};
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		signed char curp{static_cast<signed char>(set.second)};
		static_assert(!(absolute && !issigned), "impossible combination of partwise filtering and tiered absolute mode");
		curp >>= 8 - 1;
		U curq{static_cast<unsigned char>(curp)};
		if constexpr(!isfloatingpoint && issigned){
			unsigned char curo{static_cast<unsigned char>(cur)};
			curo += static_cast<unsigned char>(curq);
			cur = curo;
		}
		cur ^= curq;
	}
	static_assert(!(isfloatingpoint && absolute && !issigned), "impossible combination of partwise filtering and tiered absolute mode");// one-register filtering
	return{static_cast<size_t>(cur)};
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U) &&
	8 < CHAR_BIT * sizeof(U),
	std::pair<size_t, size_t>> filtershift8(U cura, U curb, unsigned shift)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		if constexpr(absolute && !issigned){
			T curao{static_cast<T>(cura)};
			curao += curao;
			cura = curao;
		}
		curap >>= CHAR_BIT * sizeof(T) - 1;
		U curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		if constexpr(absolute && !issigned){
			T curbo{static_cast<T>(curb)};
			curbo += curbo;
			curb = curbo;
		}
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		U curbq{static_cast<T>(curbp)};
		if constexpr(!isfloatingpoint && issigned){
			T curao{static_cast<T>(cura)};
			T curbo{static_cast<T>(curb)};
			curao += static_cast<T>(curaq);
			curbo += static_cast<T>(curbq);
			cura = curao;
			curb = curbo;
		}
		cura ^= curaq;
		curb ^= curbq;
	}else if constexpr(isfloatingpoint && absolute && !issigned){// one-register filtering
		cura = rotateleftportable<1>(static_cast<T>(cura));
		curb = rotateleftportable<1>(static_cast<T>(curb));
	}
	cura >>= shift;
	curb >>= shift;
	return{static_cast<size_t>(cura & 0xFFu), static_cast<size_t>(curb & 0xFFu)};
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_same_v<unsigned char, T> &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U) &&
	8 < CHAR_BIT * sizeof(U),
	std::pair<size_t, size_t>> filtershift8(U cura, U curb, unsigned)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		signed char curap{static_cast<signed char>(cura)};
		static_assert(!(absolute && !issigned), "impossible combination of partwise filtering and tiered absolute mode");
		curap >>= 8 - 1;
		U curaq{static_cast<unsigned char>(curap)};
		signed char curbp{static_cast<signed char>(curb)};
		curbp >>= 8 - 1;
		U curbq{static_cast<unsigned char>(curbp)};
		if constexpr(!isfloatingpoint && issigned){
			unsigned char curao{static_cast<unsigned char>(cura)};
			unsigned char curbo{static_cast<unsigned char>(curb)};
			curao += static_cast<unsigned char>(curaq);
			curbo += static_cast<unsigned char>(curbq);
			cura = curao;
			curb = curbo;
		}
		cura ^= curaq;
		curb ^= curbq;
	}
	static_assert(!(isfloatingpoint && absolute && !issigned), "impossible combination of partwise filtering and tiered absolute mode");// one-register filtering
	return{static_cast<size_t>(cura), static_cast<size_t>(curb)};
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	8 == CHAR_BIT &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U) &&
	8 < CHAR_BIT * sizeof(U),
	std::pair<size_t, size_t>> filtershift8(std::pair<unsigned char, unsigned char> seta, std::pair<unsigned char, unsigned char> setb, unsigned)noexcept{
	// Filtering is simplified if possible.
	// This should never filter the top part for non-absolute floating-point inputs.
	U cura{seta.first};
	U curb{setb.first};
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		signed char curap{static_cast<signed char>(seta.second)};
		static_assert(!(absolute && !issigned), "impossible combination of partwise filtering and tiered absolute mode");
		curap >>= 8 - 1;
		U curaq{static_cast<unsigned char>(curap)};
		signed char curbp{static_cast<signed char>(setb.second)};
		curbp >>= 8 - 1;
		U curbq{static_cast<unsigned char>(curbp)};
		if constexpr(!isfloatingpoint && issigned){
			unsigned char curao{static_cast<unsigned char>(cura)};
			unsigned char curbo{static_cast<unsigned char>(curb)};
			curao += static_cast<unsigned char>(curaq);
			curbo += static_cast<unsigned char>(curbq);
			cura = curao;
			curb = curbo;
		}
		cura ^= curaq;
		curb ^= curbq;
	}
	static_assert(!(isfloatingpoint && absolute && !issigned), "impossible combination of partwise filtering and tiered absolute mode");// one-register filtering
	return{static_cast<size_t>(cura), static_cast<size_t>(curb)};
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<longdoubletest128, T> ||
		std::is_same_v<longdoubletest96, T> ||
		std::is_same_v<longdoubletest80, T> ||
		std::is_same_v<long double, T> &&
		64 == LDBL_MANT_DIG &&
		16384 == LDBL_MAX_EXP &&
		128 >= CHAR_BIT * sizeof(long double) &&
		64 < CHAR_BIT * sizeof(long double)) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U) &&
	8 < CHAR_BIT * sizeof(U),
	std::pair<size_t, size_t>> filtershift8(uint_least64_t curma, U curea, uint_least64_t curmb, U cureb, unsigned shift)noexcept{
	// Filtering is simplified if possible.
	// This should never filter the top 16 bits.
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		int_least16_t curpa{static_cast<int_least16_t>(curea)};
		int_least16_t curpb{static_cast<int_least16_t>(cureb)};
		if constexpr(absolute && !issigned){
			curma += curma;
			curmb += curmb;
		}
		curpa >>= 16 - 1;
		curpb >>= 16 - 1;
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
		uint_least64_t curqa{static_cast<uint_least64_t>(curpa)};// sign-extend
		uint_least64_t curqb{static_cast<uint_least64_t>(curpb)};
#else
		uint_least32_t curqa{static_cast<uint_least32_t>(curpa)};// sign-extend
		uint_least32_t curqb{static_cast<uint_least32_t>(curpb)};
#endif
		if constexpr(!isfloatingpoint && issigned){
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
			curma += curqa;
			curmb += curqb;
#elif (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl)
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrymida, checkcarrya;
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			curmloa = __builtin_addcl(curmloa, curmloa, 0, &carrymida);
			curmhia = __builtin_addcl(curmhia, curmhia, carrymida, &checkcarrya);
			static_cast<void>(checkcarrya);
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
			unsigned long carrymidb, checkcarryb;
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			curmlob = __builtin_addcl(curmlob, curmlob, 0, &carrymidb);
			curmhib = __builtin_addcl(curmhib, curmhib, carrymidb, &checkcarryb);
			static_cast<void>(checkcarryb);
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#elif defined(_M_IX86)
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			unsigned char checkcarrya{_addcarry_u32(_addcarry_u32(0, curmloa, curmloa, &curmloa), curmhia, curmhia, &curmhia)};
			static_cast<void>(checkcarrya);
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			unsigned char checkcarryb{_addcarry_u32(_addcarry_u32(0, curmlob, curmlob, &curmlob), curmhib, curmhib, &curmhib)};
			static_cast<void>(checkcarryb);
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#else
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			uint_least32_t curmlotmpa{curmloa}, curmlotmpb{curmlob};
			curmloa += curqa;
			curmhia += curqa;
			curmhia += curmloa < curmlotmpa || curmloa < curqa;
			curmlob += curqb;
			curmhib += curqb;
			curmhib += curmlob < curmlotmpb || curmlob < curqb;
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#endif
		}
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
		curma ^= curqa;
		curmb ^= curqb;
#else
		uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
		uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
		curmloa ^= curqa;
		curmhia ^= curqa;
		curmlob ^= curqb;
		curmhib ^= curqb;
		alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
		curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
		alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
		curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#endif
	}else if constexpr(isfloatingpoint && absolute && !issigned){// one-register filtering
		uint_least16_t curoa{static_cast<uint_least16_t>(curea)};
		uint_least16_t curob{static_cast<uint_least16_t>(cureb)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
		static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
		unsigned short carrysigna;
		curoa = __builtin_addcs(curoa, curoa, 0, &carrysigna);
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
		unsigned long long checkcarrya;
		curma = __builtin_addcll(curma, curma, static_cast<unsigned long long>(carrysigna), &checkcarrya);
#else
		static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
		unsigned long checkcarrya;
		curma = __builtin_addcl(curma, curma, static_cast<unsigned long>(carrysigna), &checkcarrya);
#endif
		static_cast<void>(checkcarrya);
#else
		static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
		unsigned long carrymida, checkcarrya;
		uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
		curmloa = __builtin_addcl(curmloa, curmloa, static_cast<unsigned long>(carrysigna), &carrymida);
		curmhia = __builtin_addcl(curmhia, curmhia, carrymida, &checkcarrya);
		static_cast<void>(checkcarrya);
		alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
		curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
#endif
		unsigned short carrysignb;
		curob = __builtin_addcs(curob, curob, 0, &carrysignb);
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		unsigned long long checkcarryb;
		curmb = __builtin_addcll(curmb, curmb, static_cast<unsigned long long>(carrysignb), &checkcarryb);
#else
		unsigned long checkcarryb;
		curmb = __builtin_addcl(curmb, curmb, static_cast<unsigned long>(carrysignb), &checkcarryb);
#endif
		static_cast<void>(checkcarryb);
#else
		unsigned long carrymidb, checkcarryb;
		uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
		curmlob = __builtin_addcl(curmlob, curmlob, static_cast<unsigned long>(carrysignb), &carrymidb);
		curmhib = __builtin_addcl(curmhib, curmhib, carrymidb, &checkcarryb);
		static_cast<void>(checkcarryb);
		alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
		curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#endif
#elif defined(_M_X64)
		unsigned char checkcarrya{_addcarry_u64(_addcarry_u16(0, curoa, curoa, &curoa), curma, curma, &curma)};
		static_cast<void>(checkcarrya);
		unsigned char checkcarryb{_addcarry_u64(_addcarry_u16(0, curob, curob, &curob), curmb, curmb, &curmb)};
		static_cast<void>(checkcarryb);
#elif defined(_M_IX86)
		uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
		unsigned char checkcarrya{_addcarry_u32(_addcarry_u32(_addcarry_u16(0, curoa, curoa, &curoa), curmloa, curmloa, &curmloa), curmhia, curmhia, &curmhia)};
		static_cast<void>(checkcarrya);
		alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
		curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
		uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
		unsigned char checkcarryb{_addcarry_u32(_addcarry_u32(_addcarry_u16(0, curob, curob, &curob), curmlob, curmlob, &curmlob), curmhib, curmhib, &curmhib)};
		static_cast<void>(checkcarryb);
		alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
		curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#else
		uint_least16_t curotmpa{curoa}, curotmpb{curob};
		curoa += curoa;
		curma += curma;
		curma += curoa < curotmpa;
		curob += curob;
		curmb += curmb;
		curmb += curob < curotmpb;
#endif
	}
	curma >>= shift;
	curmb >>= shift;
	return{static_cast<size_t>(curma & 0xFFu), static_cast<size_t>(curmb & 0xFFu)};
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U, typename W>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<longdoubletest128, T> ||
	std::is_same_v<longdoubletest96, T> ||
	std::is_same_v<longdoubletest80, T> ||
	std::is_same_v<long double, T> &&
	64 == LDBL_MANT_DIG &&
	16384 == LDBL_MAX_EXP &&
	128 >= CHAR_BIT * sizeof(long double) &&
	64 < CHAR_BIT * sizeof(long double)) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U) &&
	8 < CHAR_BIT * sizeof(U) &&
	std::is_unsigned_v<W> &&
	64 >= CHAR_BIT * sizeof(W) &&
	8 < CHAR_BIT * sizeof(W),
	std::pair<size_t, size_t>> filtershift8(std::pair<uint_least64_t, W> cura, std::pair<uint_least64_t, W> curb, unsigned shift)noexcept{
	// Use the function above.
	return{filtershift8<absolute, issigned, isfloatingpoint, T, U>(cura.first, cura.second, curb.first, curb.second, shift)};
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<longdoubletest128, T> ||
	std::is_same_v<longdoubletest96, T> ||
	std::is_same_v<longdoubletest80, T> ||
	std::is_same_v<long double, T> &&
	64 == LDBL_MANT_DIG &&
	16384 == LDBL_MAX_EXP &&
	128 >= CHAR_BIT * sizeof(long double) &&
	64 < CHAR_BIT * sizeof(long double)) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(uint_least64_t &curm, U &cure)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		int_least16_t curp{static_cast<int_least16_t>(cure)};
		if constexpr(isfloatingpoint){
			uint_least16_t curo{static_cast<uint_least16_t>(cure)};
			curo += curo;
			cure = curo;
		}else if constexpr(!issigned){
			uint_least16_t curo{static_cast<uint_least16_t>(cure)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carry;
			curm = __builtin_addcll(curm, curm, 0, &carry);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carry;
			curm = __builtin_addcl(curm, curm, 0, &carry);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrymid, carry;
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			curmlo = __builtin_addcl(curmlo, curmlo, 0, &carrymid);
			curmhi = __builtin_addcl(curmhi, curmhi, carrymid, &carry);
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#endif
			unsigned short checkcarry;
			curo = __builtin_addcs(curo, curo, static_cast<unsigned short>(carry), &checkcarry);
			static_cast<void>(checkcarry);
#elif defined(_M_X64)
			unsigned char checkcarry{_addcarry_u16(_addcarry_u64(0, curm, curm, &curm), curo, curo, &curo)};
			static_cast<void>(checkcarry);
#elif defined(_M_IX86)
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			unsigned char checkcarry{_addcarry_u16(_addcarry_u32(_addcarry_u32(0, curmlo, curmlo, &curmlo), curmhi, curmhi, &curmhi), curo, curo, &curo)};
			static_cast<void>(checkcarry);
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#else
			uint_least64_t curmtmp{curm};
			curm += curm;
			curo += static_cast<uint_least16_t>(curo);
			curo += curm < curmtmp;
#endif
			cure = curo;
		}
		curp >>= 16 - 1;
		U curq{static_cast<uint_least16_t>(curp)};
		if constexpr(isfloatingpoint) cure >>= 1;
		else if constexpr(issigned){
			uint_least16_t curo{static_cast<uint_least16_t>(cure)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carry;
			curm = __builtin_addcll(curm, curq, 0, &carry);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carry;
			curm = __builtin_addcl(curm, curq, 0, &carry);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrymid, carry;
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			curmlo = __builtin_addcl(curmlo, curq, 0, &carrymid);
			curmhi = __builtin_addcl(curmhi, curq, carrymid, &carry);
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#endif
			unsigned short checkcarry;
			curo = __builtin_addcs(curo, static_cast<unsigned short>(curq), static_cast<unsigned short>(carry), &checkcarry);
			static_cast<void>(checkcarry);
#elif defined(_M_X64)
			unsigned char checkcarry{_addcarry_u16(_addcarry_u64(0, curm, curq, &curm), curo, static_cast<uint_least16_t>(curq), &curo)};
			static_cast<void>(checkcarry);
#elif defined(_M_IX86)
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			unsigned char checkcarry{_addcarry_u16(_addcarry_u32(_addcarry_u32(0, curmlo, curq, &curmlo), curmhi, curq, &curmhi), curo, static_cast<uint_least16_t>(curq), &curo)};
			static_cast<void>(checkcarry);
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#elif 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
			uint_least64_t curmtmp{curm};
			curm += curq;
			curo += static_cast<uint_least16_t>(curq);
			curo += curm < curmtmp || curm < curq;
#else
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			uint_least32_t curmlotmp{curmlo}, curmhitmp{curmhi};
			curmlo += curq;
			curmhi += curq;
			curmhi += curmlo < curmlotmp || curmlo < curq;
			curo += static_cast<uint_least16_t>(curq);
			curo += curmhi < curmhitmp || curmhi < curq;
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#endif
			cure = curo;
		}
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
		curm ^= curq;
#else
		uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
		curmlo ^= curq;
		curmhi ^= curq;
		alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
		curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#endif
		cure ^= static_cast<U>(curq);
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		if constexpr(issigned) cure &= ~static_cast<uint_least16_t>(0) >> 1;
		else{
			uint_least16_t curo{static_cast<uint_least16_t>(cure)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
			unsigned short carrysign;
			curo = __builtin_addcs(curo, curo, 0, &carrysign);
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carry;
			curm = __builtin_addcll(curm, curm, static_cast<unsigned long long>(carrysign), &carry);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carry;
			curm = __builtin_addcl(curm, curm, static_cast<unsigned long>(carrysign), &carry);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrymid, carry;
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			curmlo = __builtin_addcl(curmlo, curmlo, static_cast<unsigned long>(carrysign), &carrymid);
			curmhi = __builtin_addcl(curmhi, curmhi, carrymid, &carry);
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#endif
			unsigned short checkcarry;
			curo = __builtin_addcs(curo, 0, static_cast<unsigned short>(carry), &checkcarry);
			static_cast<void>(checkcarry);
#elif defined(_M_X64)
			unsigned char checkcarry{_addcarry_u16(_addcarry_u64(_addcarry_u16(0, curo, curo, &curo), curm, curm, &curm), curo, 0, &curo)};
			static_cast<void>(checkcarry);
#elif defined(_M_IX86)
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			unsigned char checkcarry{_addcarry_u16(_addcarry_u32(_addcarry_u32(_addcarry_u16(0, curo, curo, &curo), curmlo, curmlo, &curmlo), curmhi, curmhi, &curmhi), curo, 0, &curo)};
			static_cast<void>(checkcarry);
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#else
			uint_least16_t curotmp{curo};
			curo += curo;
			uint_least64_t curmtmp{curm};
			curm += curm;
			curm += curo < curotmp;
			curo += curm < curmtmp;
#endif
			cure = curo;
		}
	}
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<longdoubletest128, T> ||
	std::is_same_v<longdoubletest96, T> ||
	std::is_same_v<longdoubletest80, T> ||
	std::is_same_v<long double, T> &&
	64 == LDBL_MANT_DIG &&
	16384 == LDBL_MAX_EXP &&
	128 >= CHAR_BIT * sizeof(long double) &&
	64 < CHAR_BIT * sizeof(long double)) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(uint_least64_t &curm, U &cure, T *out)noexcept{
	using W = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t,
		std::conditional_t<96 == CHAR_BIT * sizeof(T), uint_least32_t,
		std::conditional_t<80 == CHAR_BIT * sizeof(T), uint_least16_t, void>>>;
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		int_least16_t curp{static_cast<int_least16_t>(cure)};
		if constexpr(isfloatingpoint){
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(out) + 1) = cure;
			uint_least16_t curo{static_cast<uint_least16_t>(cure)};
			curo += curo;
			cure = curo;
		}else if constexpr(!issigned){
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(out) + 1) = cure;
			*reinterpret_cast<uint_least64_t *>(out) = curm;
			uint_least16_t curo{static_cast<uint_least16_t>(cure)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carry;
			curm = __builtin_addcll(curm, curm, 0, &carry);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carry;
			curm = __builtin_addcl(curm, curm, 0, &carry);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrymid, carry;
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			curmlo = __builtin_addcl(curmlo, curmlo, 0, &carrymid);
			curmhi = __builtin_addcl(curmhi, curmhi, carrymid, &carry);
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#endif
			unsigned short checkcarry;
			curo = __builtin_addcs(curo, curo, static_cast<unsigned short>(carry), &checkcarry);
			static_cast<void>(checkcarry);
#elif defined(_M_X64)
			unsigned char checkcarry{_addcarry_u16(_addcarry_u64(0, curm, curm, &curm), curo, curo, &curo)};
			static_cast<void>(checkcarry);
#elif defined(_M_IX86)
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			unsigned char checkcarry{_addcarry_u16(_addcarry_u32(_addcarry_u32(0, curmlo, curmlo, &curmlo), curmhi, curmhi, &curmhi), curo, curo, &curo)};
			static_cast<void>(checkcarry);
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#else
			uint_least64_t curmtmp{curm};
			curm += curm;
			curo += static_cast<uint_least16_t>(curo);
			curo += curm < curmtmp;
#endif
			cure = curo;
		}
		curp >>= 16 - 1;
		U curq{static_cast<uint_least16_t>(curp)};
		if constexpr(isfloatingpoint){
			cure >>= 1;
			*reinterpret_cast<uint_least64_t *>(out) = curm;
		}else if constexpr(issigned){
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(out) + 1) = cure;
			*reinterpret_cast<uint_least64_t *>(out) = curm;
			uint_least16_t curo{static_cast<uint_least16_t>(cure)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carry;
			curm = __builtin_addcll(curm, curq, 0, &carry);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carry;
			curm = __builtin_addcl(curm, curq, 0, &carry);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrymid, carry;
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			curmlo = __builtin_addcl(curmlo, curq, 0, &carrymid);
			curmhi = __builtin_addcl(curmhi, curq, carrymid, &carry);
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#endif
			unsigned short checkcarry;
			curo = __builtin_addcs(curo, static_cast<unsigned short>(curq), static_cast<unsigned short>(carry), &checkcarry);
			static_cast<void>(checkcarry);
#elif defined(_M_X64)
			unsigned char checkcarry{_addcarry_u16(_addcarry_u64(0, curm, curq, &curm), curo, static_cast<uint_least16_t>(curq), &curo)};
			static_cast<void>(checkcarry);
#elif defined(_M_IX86)
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			unsigned char checkcarry{_addcarry_u16(_addcarry_u32(_addcarry_u32(0, curmlo, curq, &curmlo), curmhi, curq, &curmhi), curo, static_cast<uint_least16_t>(curq), &curo)};
			static_cast<void>(checkcarry);
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#elif 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
			uint_least64_t curmtmp{curm};
			curm += curq;
			curo += static_cast<uint_least16_t>(curq);
			curo += curm < curmtmp || curm < curq;
#else
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			uint_least32_t curmlotmp{curmlo}, curmhitmp{curmhi};
			curmlo += curq;
			curmhi += curq;
			curmhi += curmlo < curmlotmp || curmlo < curq;
			curo += static_cast<uint_least16_t>(curq);
			curo += curmhi < curmhitmp || curmhi < curq;
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#endif
			cure = curo;
		}
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
		curm ^= curq;
#else
		uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
		curmlo ^= curq;
		curmhi ^= curq;
		alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
		curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#endif
		cure ^= static_cast<U>(curq);
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(out) + 1) = cure;
		if constexpr(issigned){
			cure &= ~static_cast<uint_least16_t>(0) >> 1;
			*reinterpret_cast<uint_least64_t *>(out) = curm;
		}else{
			uint_least16_t curo{static_cast<uint_least16_t>(cure)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
			unsigned short carrysign;
			curo = __builtin_addcs(curo, curo, 0, &carrysign);
			*reinterpret_cast<uint_least64_t *>(out) = curm;
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carry;
			curm = __builtin_addcll(curm, curm, static_cast<unsigned long long>(carrysign), &carry);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carry;
			curm = __builtin_addcl(curm, curm, static_cast<unsigned long>(carrysign), &carry);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrymid, carry;
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			curmlo = __builtin_addcl(curmlo, curmlo, static_cast<unsigned long>(carrysign), &carrymid);
			curmhi = __builtin_addcl(curmhi, curmhi, carrymid, &carry);
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#endif
			unsigned short checkcarry;
			curo = __builtin_addcs(curo, 0, static_cast<unsigned short>(carry), &checkcarry);
			static_cast<void>(checkcarry);
#elif defined(_M_X64)
			unsigned char carrysign{_addcarry_u16(0, curo, curo, &curo)};
			*reinterpret_cast<uint_least64_t *>(out) = curm;
			unsigned char checkcarry{_addcarry_u16(_addcarry_u64(carrysign, curm, curm, &curm), curo, 0, &curo)};
			static_cast<void>(checkcarry);
#elif defined(_M_IX86)
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			unsigned char carrysign{_addcarry_u16(0, curo, curo, &curo)};
			*reinterpret_cast<uint_least64_t *>(out) = curm;
			unsigned char checkcarry{_addcarry_u16(_addcarry_u32(_addcarry_u32(carrysign, curmlo, curmlo, &curmlo), curmhi, curmhi, &curmhi), curo, 0, &curo)};
			static_cast<void>(checkcarry);
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#else
			uint_least16_t curotmp{curo};
			curo += curo;
			*reinterpret_cast<uint_least64_t *>(out) = curm;
			uint_least64_t curmtmp{curm};
			curm += curm;
			curm += curo < curotmp;
			curo += curm < curmtmp;
#endif
			cure = curo;
		}
	}
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<longdoubletest128, T> ||
	std::is_same_v<longdoubletest96, T> ||
	std::is_same_v<longdoubletest80, T> ||
	std::is_same_v<long double, T> &&
	64 == LDBL_MANT_DIG &&
	16384 == LDBL_MAX_EXP &&
	128 >= CHAR_BIT * sizeof(long double) &&
	64 < CHAR_BIT * sizeof(long double)) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(uint_least64_t &curm, U &cure, T *out, T *dst)noexcept{
	using W = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t,
		std::conditional_t<96 == CHAR_BIT * sizeof(T), uint_least32_t,
		std::conditional_t<80 == CHAR_BIT * sizeof(T), uint_least16_t, void>>>;
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		int_least16_t curp{static_cast<int_least16_t>(cure)};
		if constexpr(isfloatingpoint){
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(out) + 1) = cure;
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(dst) + 1) = cure;
			uint_least16_t curo{static_cast<uint_least16_t>(cure)};
			curo += curo;
			cure = curo;
		}else if constexpr(!issigned){
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(out) + 1) = cure;
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(dst) + 1) = cure;
			*reinterpret_cast<uint_least64_t *>(out) = curm;
			*reinterpret_cast<uint_least64_t *>(dst) = curm;
			uint_least16_t curo{static_cast<uint_least16_t>(cure)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carry;
			curm = __builtin_addcll(curm, curm, 0, &carry);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carry;
			curm = __builtin_addcl(curm, curm, 0, &carry);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrymid, carry;
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			curmlo = __builtin_addcl(curmlo, curmlo, 0, &carrymid);
			curmhi = __builtin_addcl(curmhi, curmhi, carrymid, &carry);
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#endif
			unsigned short checkcarry;
			curo = __builtin_addcs(curo, curo, static_cast<unsigned short>(carry), &checkcarry);
			static_cast<void>(checkcarry);
#elif defined(_M_X64)
			unsigned char checkcarry{_addcarry_u16(_addcarry_u64(0, curm, curm, &curm), curo, curo, &curo)};
			static_cast<void>(checkcarry);
#elif defined(_M_IX86)
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			unsigned char checkcarry{_addcarry_u16(_addcarry_u32(_addcarry_u32(0, curmlo, curmlo, &curmlo), curmhi, curmhi, &curmhi), curo, curo, &curo)};
			static_cast<void>(checkcarry);
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#else
			uint_least64_t curmtmp{curm};
			curm += curm;
			curo += static_cast<uint_least16_t>(curo);
			curo += curm < curmtmp;
#endif
			cure = curo;
		}
		curp >>= 16 - 1;
		U curq{static_cast<uint_least16_t>(curp)};
		if constexpr(isfloatingpoint){
			cure >>= 1;
			*reinterpret_cast<uint_least64_t *>(out) = curm;
			*reinterpret_cast<uint_least64_t *>(dst) = curm;
		}else if constexpr(issigned){
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(out) + 1) = cure;
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(dst) + 1) = cure;
			*reinterpret_cast<uint_least64_t *>(out) = curm;
			*reinterpret_cast<uint_least64_t *>(dst) = curm;
			uint_least16_t curo{static_cast<uint_least16_t>(cure)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carry;
			curm = __builtin_addcll(curm, curq, 0, &carry);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carry;
			curm = __builtin_addcl(curm, curq, 0, &carry);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrymid, carry;
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			curmlo = __builtin_addcl(curmlo, curq, 0, &carrymid);
			curmhi = __builtin_addcl(curmhi, curq, carrymid, &carry);
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#endif
			unsigned short checkcarry;
			curo = __builtin_addcs(curo, static_cast<unsigned short>(curq), static_cast<unsigned short>(carry), &checkcarry);
			static_cast<void>(checkcarry);
#elif defined(_M_X64)
			unsigned char checkcarry{_addcarry_u16(_addcarry_u64(0, curm, curq, &curm), curo, static_cast<uint_least16_t>(curq), &curo)};
			static_cast<void>(checkcarry);
#elif defined(_M_IX86)
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			unsigned char checkcarry{_addcarry_u16(_addcarry_u32(_addcarry_u32(0, curmlo, curq, &curmlo), curmhi, curq, &curmhi), curo, static_cast<uint_least16_t>(curq), &curo)};
			static_cast<void>(checkcarry);
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#elif 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
			uint_least64_t curmtmp{curm};
			curm += curq;
			curo += static_cast<uint_least16_t>(curq);
			curo += curm < curmtmp || curm < curq;
#else
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			uint_least32_t curmlotmp{curmlo}, curmhitmp{curmhi};
			curmlo += curq;
			curmhi += curq;
			curmhi += curmlo < curmlotmp || curmlo < curq;
			curo += static_cast<uint_least16_t>(curq);
			curo += curmhi < curmhitmp || curmhi < curq;
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#endif
			cure = curo;
		}
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
		curm ^= curq;
#else
		uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
		curmlo ^= curq;
		curmhi ^= curq;
		alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
		curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#endif
		cure ^= static_cast<U>(curq);
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(out) + 1) = cure;
		*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(dst) + 1) = cure;
		if constexpr(issigned){
			cure &= ~static_cast<uint_least16_t>(0) >> 1;
			*reinterpret_cast<uint_least64_t *>(out) = curm;
			*reinterpret_cast<uint_least64_t *>(dst) = curm;
		}else{
			uint_least16_t curo{static_cast<uint_least16_t>(cure)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
			unsigned short carrysign;
			curo = __builtin_addcs(curo, curo, 0, &carrysign);
			*reinterpret_cast<uint_least64_t *>(out) = curm;
			*reinterpret_cast<uint_least64_t *>(dst) = curm;
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carry;
			curm = __builtin_addcll(curm, curm, static_cast<unsigned long long>(carrysign), &carry);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carry;
			curm = __builtin_addcl(curm, curm, static_cast<unsigned long>(carrysign), &carry);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrymid, carry;
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			curmlo = __builtin_addcl(curmlo, curmlo, static_cast<unsigned long>(carrysign), &carrymid);
			curmhi = __builtin_addcl(curmhi, curmhi, carrymid, &carry);
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#endif
			unsigned short checkcarry;
			curo = __builtin_addcs(curo, 0, static_cast<unsigned short>(carry), &checkcarry);
			static_cast<void>(checkcarry);
#elif defined(_M_X64)
			unsigned char carrysign{_addcarry_u16(0, curo, curo, &curo)};
			*reinterpret_cast<uint_least64_t *>(out) = curm;
			*reinterpret_cast<uint_least64_t *>(dst) = curm;
			unsigned char checkcarry{_addcarry_u16(_addcarry_u64(carrysign, curm, curm, &curm), curo, 0, &curo)};
			static_cast<void>(checkcarry);
#elif defined(_M_IX86)
			uint_least32_t curmlo{static_cast<uint_least32_t>(curm & 0xFFFFFFFFu)}, curmhi{static_cast<uint_least32_t>(curm >> 32)};// decompose
			unsigned char carrysign{_addcarry_u16(0, curo, curo, &curo)};
			*reinterpret_cast<uint_least64_t *>(out) = curm;
			*reinterpret_cast<uint_least64_t *>(dst) = curm;
			unsigned char checkcarry{_addcarry_u16(_addcarry_u32(_addcarry_u32(carrysign, curmlo, curmlo, &curmlo), curmhi, curmhi, &curmhi), curo, 0, &curo)};
			static_cast<void>(checkcarry);
			alignas(8) uint_least32_t acurm[2]{curmlo, curmhi};
			curm = *reinterpret_cast<uint_least64_t *>(acurm);// recompose
#else
			uint_least16_t curotmp{curo};
			curo += curo;
			*reinterpret_cast<uint_least64_t *>(out) = curm;
			*reinterpret_cast<uint_least64_t *>(dst) = curm;
			uint_least64_t curmtmp{curm};
			curm += curm;
			curm += curo < curotmp;
			curo += curm < curmtmp;
#endif
			cure = curo;
		}
	}
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<longdoubletest128, T> ||
	std::is_same_v<longdoubletest96, T> ||
	std::is_same_v<longdoubletest80, T> ||
	std::is_same_v<long double, T> &&
	64 == LDBL_MANT_DIG &&
	16384 == LDBL_MAX_EXP &&
	128 >= CHAR_BIT * sizeof(long double) &&
	64 < CHAR_BIT * sizeof(long double)) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(uint_least64_t &curma, U &curea, uint_least64_t &curmb, U &cureb)noexcept{
	using W = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t,
		std::conditional_t<96 == CHAR_BIT * sizeof(T), uint_least32_t,
		std::conditional_t<80 == CHAR_BIT * sizeof(T), uint_least16_t, void>>>;
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		int_least16_t curpa{static_cast<int_least16_t>(curea)};
		int_least16_t curpb{static_cast<int_least16_t>(cureb)};
		if constexpr(isfloatingpoint){
			uint_least16_t curoa{static_cast<uint_least16_t>(curea)};
			curoa += curoa;
			curea = curoa;
			uint_least16_t curob{static_cast<uint_least16_t>(cureb)};
			curob += curob;
			cureb = curob;
		}else if constexpr(!issigned){
			uint_least16_t curoa{static_cast<uint_least16_t>(curea)};
			uint_least16_t curob{static_cast<uint_least16_t>(cureb)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carrya;
			curma = __builtin_addcll(curma, curma, 0, &carrya);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrya;
			curma = __builtin_addcl(curma, curma, 0, &carrya);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrymida, carrya;
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			curmloa = __builtin_addcl(curmloa, curmloa, 0, &carrymida);
			curmhia = __builtin_addcl(curmhia, curmhia, carrymida, &carrya);
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
#endif
			unsigned short checkcarrya;
			curoa = __builtin_addcs(curoa, curoa, static_cast<unsigned short>(carrya), &checkcarrya);
			static_cast<void>(checkcarrya);
			unsigned long carryb;
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			unsigned long long carryb;
			curmb = __builtin_addcll(curmb, curmb, 0, &carryb);
#else
			unsigned long carryb;
			curmb = __builtin_addcl(curmb, curmb, 0, &carryb);
#endif
#else
			unsigned long carrymidb, carryb;
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			curmlob = __builtin_addcl(curmlob, curmlob, 0, &carrymidb);
			curmhib = __builtin_addcl(curmhib, curmhib, carrymidb, &carryb);
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#endif
			unsigned short checkcarryb;
			curob = __builtin_addcs(curob, curob, static_cast<unsigned short>(carryb), &checkcarryb);
			static_cast<void>(checkcarryb);
#elif defined(_M_X64)
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u64(0, curma, curma, &curma), curoa, curoa, &curoa)};
			static_cast<void>(checkcarrya);
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u64(0, curmb, curmb, &curmb), curob, curob, &curob)};
			static_cast<void>(checkcarryb);
#elif defined(_M_IX86)
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u32(_addcarry_u32(0, curmloa, curmloa, &curmloa), curmhia, curmhia, &curmhia), curoa, curoa, &curoa)};
			static_cast<void>(checkcarrya);
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u32(_addcarry_u32(0, curmlob, curmlob, &curmlob), curmhib, curmhib, &curmhib), curob, curob, &curob)};
			static_cast<void>(checkcarryb);
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#else
			uint_least64_t curmtmpa{curma};
			curma += curma;
			curoa += static_cast<uint_least16_t>(curoa);
			curoa += curma < curmtmpa;
			uint_least64_t curmtmpb{curmb};
			curmb += curmb;
			curob += static_cast<uint_least16_t>(curob);
			curob += curmb < curmtmpb;
#endif
			curea = curoa;
			cureb = curob;
		}
		curpa >>= 16 - 1;
		U curqa{static_cast<uint_least16_t>(curpa)};
		curpb >>= 16 - 1;
		U curqb{static_cast<uint_least16_t>(curpb)};
		if constexpr(isfloatingpoint){
			curea >>= 1;
			cureb >>= 1;
		}else if constexpr(issigned){
			uint_least16_t curoa{static_cast<uint_least16_t>(curea)};
			uint_least16_t curob{static_cast<uint_least16_t>(cureb)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carrya;
			curma = __builtin_addcll(curma, curqa, 0, &carrya);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrya;
			curma = __builtin_addcl(curma, curqa, 0, &carrya);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrymida, carrya;
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			curmloa = __builtin_addcl(curmloa, curqa, 0, &carrymida);
			curmhia = __builtin_addcl(curmhia, curqa, carrymida, &carrya);
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
#endif
			unsigned long carryb;
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			unsigned long long carryb;
			curmb = __builtin_addcll(curmb, curqb, 0, &carryb);
#else
			unsigned long carryb;
			curmb = __builtin_addcl(curmb, curqb, 0, &carryb);
#endif
#else
			unsigned long carrymidb, carryb;
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			curmlob = __builtin_addcl(curmlob, curqb, 0, &carrymidb);
			curmhib = __builtin_addcl(curmhib, curqb, carrymidb, &carryb);
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#endif
			unsigned short checkcarryb;
			curob = __builtin_addcs(curob, static_cast<unsigned short>(curqb), static_cast<unsigned short>(carryb), &checkcarryb);
			static_cast<void>(checkcarryb);
#elif defined(_M_X64)
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u64(0, curma, curqa, &curma), curoa, static_cast<uint_least16_t>(curqa), &curoa)};
			static_cast<void>(checkcarrya);
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u64(0, curmb, curqb, &curmb), curob, static_cast<uint_least16_t>(curqb), &curob)};
			static_cast<void>(checkcarryb);
#elif defined(_M_IX86)
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u32(_addcarry_u32(0, curmloa, curqa, &curmloa), curmhia, curqa, &curmhia), curoa, static_cast<uint_least16_t>(curqa), &curoa)};
			static_cast<void>(checkcarrya);
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u32(_addcarry_u32(0, curmlob, curqb, &curmlob), curmhib, curqb, &curmhib), curob, static_cast<uint_least16_t>(curqb), &curob)};
			static_cast<void>(checkcarryb);
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#elif 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
			uint_least64_t curmtmpa{curma};
			curma += curqa;
			curoa += static_cast<uint_least16_t>(curqa);
			curoa += curma < curmtmpa || curma < curqa;
			uint_least64_t curmtmpb{curmb};
			curmb += curqb;
			curob += static_cast<uint_least16_t>(curqb);
			curob += curmb < curmtmpb || curmb < curqb;
#else
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			uint_least32_t curmlotmpa{curmloa}, curmhitmpa{curmhia};
			curmloa += curqa;
			curmhia += curqa;
			curmhia += curmloa < curmlotmpa || curmloa < curqa;
			curoa += static_cast<uint_least16_t>(curqa);
			curoa += curmhia < curmhitmpa || curmhia < curqa;
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			uint_least32_t curmlotmpb{curmlob}, curmhitmpb{curmhib};
			curmlob += curqb;
			curmhib += curqb;
			curmhib += curmlob < curmlotmpb || curmlob < curqb;
			curob += static_cast<uint_least16_t>(curqb);
			curob += curmhib < curmhitmpb || curmhib < curqb;
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#endif
			curea = curoa;
			cureb = curob;
		}
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
		curma ^= curqa;
		curmb ^= curqb;
#else
		uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
		curmloa ^= curqa;
		curmhia ^= curqa;
		alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
		curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
		uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
		curmlob ^= curqb;
		curmhib ^= curqb;
		alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
		curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#endif
		curea ^= static_cast<U>(curqa);
		cureb ^= static_cast<U>(curqb);
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		if constexpr(issigned){
			curea &= ~static_cast<uint_least16_t>(0) >> 1;
			cureb &= ~static_cast<uint_least16_t>(0) >> 1;
		}else{
			uint_least16_t curoa{static_cast<uint_least16_t>(curea)};
			uint_least16_t curob{static_cast<uint_least16_t>(cureb)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
			unsigned short carrysigna;
			curoa = __builtin_addcs(curoa, curoa, 0, &carrysigna);
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carrya;
			curma = __builtin_addcll(curma, curma, static_cast<unsigned long long>(carrysigna), &carrya);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrya;
			curma = __builtin_addcl(curma, curma, static_cast<unsigned long>(carrysigna), &carrya);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrymida, carrya;
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			curmloa = __builtin_addcl(curmloa, curmloa, static_cast<unsigned long>(carrysigna), &carrymida);
			curmhia = __builtin_addcl(curmhia, curmhia, carrymida, &carrya);
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
#endif
			unsigned short checkcarrya;
			curoa = __builtin_addcs(curoa, 0, static_cast<unsigned short>(carrya), &checkcarrya);
			static_cast<void>(checkcarrya);
			unsigned short carrysignb;
			curob = __builtin_addcs(curob, curob, 0, &carrysignb);
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			unsigned long long carryb;
			curmb = __builtin_addcll(curmb, curmb, static_cast<unsigned long long>(carrysignb), &carryb);
#else
			unsigned long carryb;
			curmb = __builtin_addcl(curmb, curmb, static_cast<unsigned long>(carrysignb), &carryb);
#endif
#else
			unsigned long carrymidb, carryb;
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			curmlob = __builtin_addcl(curmlob, curmlob, static_cast<unsigned long>(carrysignb), &carrymidb);
			curmhib = __builtin_addcl(curmhib, curmhib, carrymidb, &carryb);
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#endif
			unsigned short checkcarryb;
			curob = __builtin_addcs(curob, 0, static_cast<unsigned short>(carryb), &checkcarryb);
			static_cast<void>(checkcarryb);
#elif defined(_M_X64)
			unsigned char carrysigna{_addcarry_u16(0, curoa, curoa, &curoa)};
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u64(carrysigna, curma, curma, &curma), curoa, 0, &curoa)};
			static_cast<void>(checkcarrya);
			unsigned char carrysignb{_addcarry_u16(0, curob, curob, &curob)};
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u64(carrysignb, curmb, curmb, &curmb), curob, 0, &curob)};
			static_cast<void>(checkcarryb);
#elif defined(_M_IX86)
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			unsigned char carrysigna{_addcarry_u16(0, curoa, curoa, &curoa)};
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u32(_addcarry_u32(carrysigna, curmloa, curmloa, &curmloa), curmhia, curmhia, &curmhia), curoa, 0, &curoa)};
			static_cast<void>(checkcarrya);
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			unsigned char carrysignb{_addcarry_u16(0, curob, curob, &curob)};
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u32(_addcarry_u32(carrysignb, curmlob, curmlob, &curmlob), curmhib, curmhib, &curmhib), curob, 0, &curob)};
			static_cast<void>(checkcarryb);
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#else
			uint_least16_t curotmpa{curoa};
			curoa += curoa;
			uint_least64_t curmtmpa{curma};
			curma += curma;
			curma += curoa < curotmpa;
			curoa += curma < curmtmpa;
			uint_least16_t curotmpb{curob};
			curob += curob;
			uint_least64_t curmtmpb{curmb};
			curmb += curmb;
			curmb += curob < curotmpb;
			curob += curmb < curmtmpb;
#endif
			curea = curoa;
			cureb = curob;
		}
	}
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<longdoubletest128, T> ||
	std::is_same_v<longdoubletest96, T> ||
	std::is_same_v<longdoubletest80, T> ||
	std::is_same_v<long double, T> &&
	64 == LDBL_MANT_DIG &&
	16384 == LDBL_MAX_EXP &&
	128 >= CHAR_BIT * sizeof(long double) &&
	64 < CHAR_BIT * sizeof(long double)) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(uint_least64_t &curma, U &curea, T *outa, uint_least64_t &curmb, U &cureb, T *outb)noexcept{
	using W = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t,
		std::conditional_t<96 == CHAR_BIT * sizeof(T), uint_least32_t,
		std::conditional_t<80 == CHAR_BIT * sizeof(T), uint_least16_t, void>>>;
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		int_least16_t curpa{static_cast<int_least16_t>(curea)};
		int_least16_t curpb{static_cast<int_least16_t>(cureb)};
		if constexpr(isfloatingpoint){
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(outa) + 1) = curea;
			uint_least16_t curoa{static_cast<uint_least16_t>(curea)};
			curoa += curoa;
			curea = curoa;
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(outb) + 1) = cureb;
			uint_least16_t curob{static_cast<uint_least16_t>(cureb)};
			curob += curob;
			cureb = curob;
		}else if constexpr(!issigned){
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(outa) + 1) = curea;
			*reinterpret_cast<uint_least64_t *>(outa) = curma;
			uint_least16_t curoa{static_cast<uint_least16_t>(curea)};
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(outb) + 1) = cureb;
			*reinterpret_cast<uint_least64_t *>(outb) = curmb;
			uint_least16_t curob{static_cast<uint_least16_t>(cureb)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carrya;
			curma = __builtin_addcll(curma, curma, 0, &carrya);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrya;
			curma = __builtin_addcl(curma, curma, 0, &carrya);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrymida, carrya;
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			curmloa = __builtin_addcl(curmloa, curmloa, 0, &carrymida);
			curmhia = __builtin_addcl(curmhia, curmhia, carrymida, &carrya);
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
#endif
			unsigned short checkcarrya;
			curoa = __builtin_addcs(curoa, curoa, static_cast<unsigned short>(carrya), &checkcarrya);
			static_cast<void>(checkcarrya);
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			unsigned long long carryb;
			curmb = __builtin_addcll(curmb, curmb, 0, &carryb);
#else
			unsigned long carryb;
			curmb = __builtin_addcl(curmb, curmb, 0, &carryb);
#endif
#else
			unsigned long carrymidb, carryb;
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			curmlob = __builtin_addcl(curmlob, curmlob, 0, &carrymidb);
			curmhib = __builtin_addcl(curmhib, curmhib, carrymidb, &carryb);
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#endif
			unsigned short checkcarryb;
			curob = __builtin_addcs(curob, curob, static_cast<unsigned short>(carryb), &checkcarryb);
			static_cast<void>(checkcarryb);
#elif defined(_M_X64)
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u64(0, curma, curma, &curma), curoa, curoa, &curoa)};
			static_cast<void>(checkcarrya);
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u64(0, curmb, curmb, &curmb), curob, curob, &curob)};
			static_cast<void>(checkcarryb);
#elif defined(_M_IX86)
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u32(_addcarry_u32(0, curmloa, curmloa, &curmloa), curmhia, curmhia, &curmhia), curoa, curoa, &curoa)};
			static_cast<void>(checkcarrya);
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u32(_addcarry_u32(0, curmlob, curmlob, &curmlob), curmhib, curmhib, &curmhib), curob, curob, &curob)};
			static_cast<void>(checkcarryb);
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#else
			uint_least64_t curmtmpa{curma};
			curma += curma;
			curoa += static_cast<uint_least16_t>(curoa);
			curoa += curma < curmtmpa;
			uint_least64_t curmtmpb{curmb};
			curmb += curmb;
			curob += static_cast<uint_least16_t>(curob);
			curob += curmb < curmtmpb;
#endif
			curea = curoa;
			cureb = curob;
		}
		curpa >>= 16 - 1;
		U curqa{static_cast<uint_least16_t>(curpa)};
		curpb >>= 16 - 1;
		U curqb{static_cast<uint_least16_t>(curpb)};
		if constexpr(isfloatingpoint){
			curea >>= 1;
			cureb >>= 1;
			*reinterpret_cast<uint_least64_t *>(outa) = curma;
			*reinterpret_cast<uint_least64_t *>(outb) = curmb;
		}else if constexpr(issigned){
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(outa) + 1) = curea;
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(outb) + 1) = cureb;
			*reinterpret_cast<uint_least64_t *>(outa) = curma;
			*reinterpret_cast<uint_least64_t *>(outb) = curmb;
			uint_least16_t curoa{static_cast<uint_least16_t>(curea)};
			uint_least16_t curob{static_cast<uint_least16_t>(cureb)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carrya;
			curma = __builtin_addcll(curma, curqa, 0, &carrya);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrya;
			curma = __builtin_addcl(curma, curqa, 0, &carrya);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrymida, carrya;
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			curmloa = __builtin_addcl(curmloa, curqa, 0, &carrymida);
			curmhia = __builtin_addcl(curmhia, curqa, carrymida, &carrya);
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
#endif
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			unsigned long long carryb;
			curmb = __builtin_addcll(curmb, curqb, 0, &carryb);
#else
			unsigned long carryb;
			curmb = __builtin_addcl(curmb, curqb, 0, &carryb);
#endif
#else
			unsigned long carrymidb, carryb;
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			curmlob = __builtin_addcl(curmlob, curqb, 0, &carrymidb);
			curmhib = __builtin_addcl(curmhib, curqb, carrymidb, &carryb);
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#endif
			unsigned short checkcarryb;
			curob = __builtin_addcs(curob, static_cast<unsigned short>(curqb), static_cast<unsigned short>(carryb), &checkcarryb);
			static_cast<void>(checkcarryb);
#elif defined(_M_X64)
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u64(0, curma, curqa, &curma), curoa, static_cast<uint_least16_t>(curqa), &curoa)};
			static_cast<void>(checkcarrya);
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u64(0, curmb, curqb, &curmb), curob, static_cast<uint_least16_t>(curqb), &curob)};
			static_cast<void>(checkcarryb);
#elif defined(_M_IX86)
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u32(_addcarry_u32(0, curmloa, curqa, &curmloa), curmhia, curqa, &curmhia), curoa, static_cast<uint_least16_t>(curqa), &curoa)};
			static_cast<void>(checkcarrya);
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u32(_addcarry_u32(0, curmlob, curqb, &curmlob), curmhib, curqb, &curmhib), curob, static_cast<uint_least16_t>(curqb), &curob)};
			static_cast<void>(checkcarryb);
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#elif 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
			uint_least64_t curmtmpa{curma};
			curma += curqa;
			curoa += static_cast<uint_least16_t>(curqa);
			curoa += curma < curmtmpa || curma < curqa;
			uint_least64_t curmtmpb{curmb};
			curmb += curqb;
			curob += static_cast<uint_least16_t>(curqb);
			curob += curmb < curmtmpb || curmb < curqb;
#else
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			uint_least32_t curmlotmpa{curmloa}, curmhitmpa{curmhia};
			curmloa += curqa;
			curmhia += curqa;
			curmhia += curmloa < curmlotmpa || curmloa < curqa;
			curoa += static_cast<uint_least16_t>(curqa);
			curoa += curmhia < curmhitmpa || curmhia < curqa;
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			uint_least32_t curmlotmpb{curmlob}, curmhitmpb{curmhib};
			curmlob += curqb;
			curmhib += curqb;
			curmhib += curmlob < curmlotmpb || curmlob < curqb;
			curob += static_cast<uint_least16_t>(curqb);
			curob += curmhib < curmhitmpb || curmhib < curqb;
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#endif
			curea = curoa;
			cureb = curob;
		}
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
		curma ^= curqa;
		curmb ^= curqb;
#else
		uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
		curmloa ^= curqa;
		curmhia ^= curqa;
		alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
		curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
		uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
		curmlob ^= curqb;
		curmhib ^= curqb;
		alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
		curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#endif
		curea ^= static_cast<U>(curqa);
		cureb ^= static_cast<U>(curqb);
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(outa) + 1) = curea;
		*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(outb) + 1) = cureb;
		if constexpr(issigned){
			curea &= ~static_cast<uint_least16_t>(0) >> 1;
			cureb &= ~static_cast<uint_least16_t>(0) >> 1;
			*reinterpret_cast<uint_least64_t *>(outa) = curma;
			*reinterpret_cast<uint_least64_t *>(outb) = curmb;
		}else{
			uint_least16_t curoa{static_cast<uint_least16_t>(curea)};
			uint_least16_t curob{static_cast<uint_least16_t>(cureb)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
			unsigned short carrysigna;
			curoa = __builtin_addcs(curoa, curoa, 0, &carrysigna);
			*reinterpret_cast<uint_least64_t *>(outa) = curma;
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carrya;
			curma = __builtin_addcll(curma, curma, static_cast<unsigned long long>(carrysigna), &carrya);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrya;
			curma = __builtin_addcl(curma, curma, static_cast<unsigned long>(carrysigna), &carrya);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrymida, carrya;
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			curmloa = __builtin_addcl(curmloa, curmloa, static_cast<unsigned long>(carrysigna), &carrymida);
			curmhia = __builtin_addcl(curmhia, curmhia, carrymida, &carrya);
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
#endif
			unsigned short checkcarrya;
			curoa = __builtin_addcs(curoa, 0, static_cast<unsigned short>(carrya), &checkcarrya);
			static_cast<void>(checkcarrya);
			unsigned short carrysignb;
			curob = __builtin_addcs(curob, curob, 0, &carrysignb);
			*reinterpret_cast<uint_least64_t *>(outb) = curmb;
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			unsigned long long carryb;
			curmb = __builtin_addcll(curmb, curmb, static_cast<unsigned long long>(carrysignb), &carryb);
#else
			unsigned long carryb;
			curmb = __builtin_addcl(curmb, curmb, static_cast<unsigned long>(carrysignb), &carryb);
#endif
#else
			unsigned long carrymidb, carryb;
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			curmlob = __builtin_addcl(curmlob, curmlob, static_cast<unsigned long>(carrysignb), &carrymidb);
			curmhib = __builtin_addcl(curmhib, curmhib, carrymidb, &carryb);
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#endif
			unsigned short checkcarryb;
			curob = __builtin_addcs(curob, 0, static_cast<unsigned short>(carryb), &checkcarryb);
			static_cast<void>(checkcarryb);
#elif defined(_M_X64)
			unsigned char carrysigna{_addcarry_u16(0, curoa, curoa, &curoa)};
			*reinterpret_cast<uint_least64_t *>(outa) = curma;
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u64(carrysigna, curma, curma, &curma), curoa, 0, &curoa)};
			static_cast<void>(checkcarrya);
			unsigned char carrysignb{_addcarry_u16(0, curob, curob, &curob)};
			*reinterpret_cast<uint_least64_t *>(outb) = curmb;
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u64(carrysignb, curmb, curmb, &curmb), curob, 0, &curob)};
			static_cast<void>(checkcarryb);
#elif defined(_M_IX86)
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			unsigned char carrysigna{_addcarry_u16(0, curoa, curoa, &curoa)};
			*reinterpret_cast<uint_least64_t *>(outa) = curma;
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u32(_addcarry_u32(carrysigna, curmloa, curmloa, &curmloa), curmhia, curmhia, &curmhia), curoa, 0, &curoa)};
			static_cast<void>(checkcarrya);
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			unsigned char carrysignb{_addcarry_u16(0, curob, curob, &curob)};
			*reinterpret_cast<uint_least64_t *>(outb) = curmb;
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u32(_addcarry_u32(carrysignb, curmlob, curmlob, &curmlob), curmhib, curmhib, &curmhib), curob, 0, &curob)};
			static_cast<void>(checkcarryb);
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#else
			uint_least16_t curotmpa{curoa};
			curoa += curoa;
			*reinterpret_cast<uint_least64_t *>(outa) = curma;
			uint_least64_t curmtmpa{curma};
			curma += curma;
			curma += curoa < curotmpa;
			curoa += curma < curmtmpa;
			uint_least16_t curotmpb{curob};
			curob += curob;
			*reinterpret_cast<uint_least64_t *>(outb) = curmb;
			uint_least64_t curmtmpb{curmb};
			curmb += curmb;
			curmb += curob < curotmpb;
			curob += curmb < curmtmpb;
#endif
			curea = curoa;
			cureb = curob;
		}
	}
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<longdoubletest128, T> ||
	std::is_same_v<longdoubletest96, T> ||
	std::is_same_v<longdoubletest80, T> ||
	std::is_same_v<long double, T> &&
	64 == LDBL_MANT_DIG &&
	16384 == LDBL_MAX_EXP &&
	128 >= CHAR_BIT * sizeof(long double) &&
	64 < CHAR_BIT * sizeof(long double)) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(uint_least64_t &curma, U &curea, T *outa, T *dsta, uint_least64_t &curmb, U &cureb, T *outb, T *dstb)noexcept{
	using W = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t,
		std::conditional_t<96 == CHAR_BIT * sizeof(T), uint_least32_t,
		std::conditional_t<80 == CHAR_BIT * sizeof(T), uint_least16_t, void>>>;
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		int_least16_t curpa{static_cast<int_least16_t>(curea)};
		int_least16_t curpb{static_cast<int_least16_t>(cureb)};
		if constexpr(isfloatingpoint){
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(outa) + 1) = curea;
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(dsta) + 1) = curea;
			uint_least16_t curoa{static_cast<uint_least16_t>(curea)};
			curoa += curoa;
			curea = curoa;
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(outb) + 1) = cureb;
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(dstb) + 1) = cureb;
			uint_least16_t curob{static_cast<uint_least16_t>(cureb)};
			curob += curob;
			cureb = curob;
		}else if constexpr(!issigned){
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(outa) + 1) = curea;
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(dsta) + 1) = curea;
			*reinterpret_cast<uint_least64_t *>(outa) = curma;
			*reinterpret_cast<uint_least64_t *>(dsta) = curma;
			uint_least16_t curoa{static_cast<uint_least16_t>(curea)};
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(outb) + 1) = cureb;
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(dstb) + 1) = cureb;
			*reinterpret_cast<uint_least64_t *>(outb) = curmb;
			*reinterpret_cast<uint_least64_t *>(dstb) = curmb;
			uint_least16_t curob{static_cast<uint_least16_t>(cureb)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carrya;
			curma = __builtin_addcll(curma, curma, 0, &carrya);
#else
			unsigned long carrya;
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			curma = __builtin_addcl(curma, curma, 0, &carrya);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrymida, carrya;
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			curmloa = __builtin_addcl(curmloa, curmloa, 0, &carrymida);
			curmhia = __builtin_addcl(curmhia, curmhia, carrymida, &carrya);
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
#endif
			unsigned short checkcarrya;
			curoa = __builtin_addcs(curoa, curoa, static_cast<unsigned short>(carrya), &checkcarrya);
			static_cast<void>(checkcarrya);
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			unsigned long long carryb;
			curmb = __builtin_addcll(curmb, curmb, 0, &carryb);
#else
			unsigned long carryb;
			curmb = __builtin_addcl(curmb, curmb, 0, &carryb);
#endif
#else
			unsigned long carrymidb, carryb;
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			curmlob = __builtin_addcl(curmlob, curmlob, 0, &carrymidb);
			curmhib = __builtin_addcl(curmhib, curmhib, carrymidb, &carryb);
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#endif
			unsigned short checkcarryb;
			curob = __builtin_addcs(curob, curob, static_cast<unsigned short>(carryb), &checkcarryb);
			static_cast<void>(checkcarryb);
#elif defined(_M_X64)
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u64(0, curma, curma, &curma), curoa, curoa, &curoa)};
			static_cast<void>(checkcarrya);
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u64(0, curmb, curmb, &curmb), curob, curob, &curob)};
			static_cast<void>(checkcarryb);
#elif defined(_M_IX86)
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u32(_addcarry_u32(0, curmloa, curmloa, &curmloa), curmhia, curmhia, &curmhia), curoa, curoa, &curoa)};
			static_cast<void>(checkcarrya);
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u32(_addcarry_u32(0, curmlob, curmlob, &curmlob), curmhib, curmhib, &curmhib), curob, curob, &curob)};
			static_cast<void>(checkcarryb);
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#else
			uint_least64_t curmtmpa{curma};
			curma += curma;
			curoa += static_cast<uint_least16_t>(curoa);
			curoa += curma < curmtmpa;
			uint_least64_t curmtmpb{curmb};
			curmb += curmb;
			curob += static_cast<uint_least16_t>(curob);
			curob += curmb < curmtmpb;
#endif
			curea = curoa;
			cureb = curob;
		}
		curpa >>= 16 - 1;
		U curqa{static_cast<uint_least16_t>(curpa)};
		curpb >>= 16 - 1;
		U curqb{static_cast<uint_least16_t>(curpb)};
		if constexpr(isfloatingpoint){
			curea >>= 1;
			cureb >>= 1;
			*reinterpret_cast<uint_least64_t *>(outa) = curma;
			*reinterpret_cast<uint_least64_t *>(dsta) = curma;
			*reinterpret_cast<uint_least64_t *>(outb) = curmb;
			*reinterpret_cast<uint_least64_t *>(dstb) = curmb;
		}else if constexpr(issigned){
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(outa) + 1) = curea;
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(dsta) + 1) = curea;
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(outb) + 1) = cureb;
			*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(dstb) + 1) = cureb;
			*reinterpret_cast<uint_least64_t *>(outa) = curma;
			*reinterpret_cast<uint_least64_t *>(dsta) = curma;
			*reinterpret_cast<uint_least64_t *>(outb) = curmb;
			*reinterpret_cast<uint_least64_t *>(dstb) = curmb;
			uint_least16_t curoa{static_cast<uint_least16_t>(curea)};
			uint_least16_t curob{static_cast<uint_least16_t>(cureb)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carrya;
			curma = __builtin_addcll(curma, curqa, 0, &carrya);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrya;
			curma = __builtin_addcl(curma, curqa, 0, &carrya);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrymida, carrya;
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			curmloa = __builtin_addcl(curmloa, curqa, 0, &carrymida);
			curmhia = __builtin_addcl(curmhia, curqa, carrymida, &carrya);
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
#endif
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			unsigned long long carryb;
			curmb = __builtin_addcll(curmb, curqb, 0, &carryb);
#else
			unsigned long carryb;
			curmb = __builtin_addcl(curmb, curqb, 0, &carryb);
#endif
#else
			unsigned long carrymidb, carryb;
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			curmlob = __builtin_addcl(curmlob, curqb, 0, &carrymidb);
			curmhib = __builtin_addcl(curmhib, curqb, carrymidb, &carryb);
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#endif
			unsigned short checkcarryb;
			curob = __builtin_addcs(curob, static_cast<unsigned short>(curqb), static_cast<unsigned short>(carryb), &checkcarryb);
			static_cast<void>(checkcarryb);
#elif defined(_M_X64)
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u64(0, curma, curqa, &curma), curoa, static_cast<uint_least16_t>(curqa), &curoa)};
			static_cast<void>(checkcarrya);
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u64(0, curmb, curqb, &curmb), curob, static_cast<uint_least16_t>(curqb), &curob)};
			static_cast<void>(checkcarryb);
#elif defined(_M_IX86)
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u32(_addcarry_u32(0, curmloa, curqa, &curmloa), curmhia, curqa, &curmhia), curoa, static_cast<uint_least16_t>(curqa), &curoa)};
			static_cast<void>(checkcarrya);
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u32(_addcarry_u32(0, curmlob, curqb, &curmlob), curmhib, curqb, &curmhib), curob, static_cast<uint_least16_t>(curqb), &curob)};
			static_cast<void>(checkcarryb);
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#elif 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
			uint_least64_t curmtmpa{curma};
			curma += curqa;
			curoa += static_cast<uint_least16_t>(curqa);
			curoa += curma < curmtmpa || curma < curqa;
			uint_least64_t curmtmpb{curmb};
			curmb += curqb;
			curob += static_cast<uint_least16_t>(curqb);
			curob += curmb < curmtmpb || curmb < curqb;
#else
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			uint_least32_t curmlotmpa{curmloa}, curmhitmpa{curmhia};
			curmloa += curqa;
			curmhia += curqa;
			curmhia += curmloa < curmlotmpa || curmloa < curqa;
			curoa += static_cast<uint_least16_t>(curqa);
			curoa += curmhia < curmhitmpa || curmhia < curqa;
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			uint_least32_t curmlotmpb{curmlob}, curmhitmpb{curmhib};
			curmlob += curqb;
			curmhib += curqb;
			curmhib += curmlob < curmlotmpb || curmlob < curqb;
			curob += static_cast<uint_least16_t>(curqb);
			curob += curmhib < curmhitmpb || curmhib < curqb;
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#endif
			curea = curoa;
			cureb = curob;
		}
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
		curma ^= curqa;
		curmb ^= curqb;
#else
		uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
		curmloa ^= curqa;
		curmhia ^= curqa;
		alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
		curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
		uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
		curmlob ^= curqb;
		curmhib ^= curqb;
		alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
		curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#endif
		curea ^= static_cast<U>(curqa);
		cureb ^= static_cast<U>(curqb);
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(outa) + 1) = curea;
		*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(dsta) + 1) = curea;
		*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(outb) + 1) = cureb;
		*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(dstb) + 1) = cureb;
		if constexpr(issigned){
			curea &= ~static_cast<uint_least16_t>(0) >> 1;
			cureb &= ~static_cast<uint_least16_t>(0) >> 1;
			*reinterpret_cast<uint_least64_t *>(outa) = curma;
			*reinterpret_cast<uint_least64_t *>(dsta) = curma;
			*reinterpret_cast<uint_least64_t *>(outb) = curmb;
			*reinterpret_cast<uint_least64_t *>(dstb) = curmb;
		}else{
			uint_least16_t curoa{static_cast<uint_least16_t>(curea)};
			uint_least16_t curob{static_cast<uint_least16_t>(cureb)};
#if (defined(__GNUC__) || defined(__clang__) || defined(__xlC__) && (defined(__VEC__) || defined(__ALTIVEC__))) && defined(__has_builtin) && __has_builtin(__builtin_addcl) && __has_builtin(__builtin_addcs)
			static_assert(16 == CHAR_BIT * sizeof(short), "unexpected size of type short");
			unsigned short carrysigna;
			curoa = __builtin_addcs(curoa, curoa, 0, &carrysigna);
			*reinterpret_cast<uint_least64_t *>(outa) = curma;
			*reinterpret_cast<uint_least64_t *>(dsta) = curma;
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			static_assert(64 == CHAR_BIT * sizeof(long long), "unexpected size of type long long");
			unsigned long long carrya;
			curma = __builtin_addcll(curma, curma, static_cast<unsigned long long>(carrysigna), &carrya);
#else
			static_assert(64 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrya;
			curma = __builtin_addcl(curma, curma, static_cast<unsigned long>(carrysigna), &carrya);
#endif
#else
			static_assert(32 == CHAR_BIT * sizeof(long), "unexpected size of type long");
			unsigned long carrymida, carrya;
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			curmloa = __builtin_addcl(curmloa, curmloa, static_cast<unsigned long>(carrysigna), &carrymida);
			curmhia = __builtin_addcl(curmhia, curmhia, carrymida, &carrya);
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
#endif
			unsigned short checkcarrya;
			curoa = __builtin_addcs(curoa, 0, static_cast<unsigned short>(carrya), &checkcarrya);
			static_cast<void>(checkcarrya);
			unsigned short carrysignb;
			curob = __builtin_addcs(curob, curob, 0, &carrysignb);
			*reinterpret_cast<uint_least64_t *>(outb) = curmb;
			*reinterpret_cast<uint_least64_t *>(dstb) = curmb;
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
			unsigned long long carryb;
			curmb = __builtin_addcll(curmb, curmb, static_cast<unsigned long long>(carrysignb), &carryb);
#else
			unsigned long carryb;
			curmb = __builtin_addcl(curmb, curmb, static_cast<unsigned long>(carrysignb), &carryb);
#endif
#else
			unsigned long carrymidb, carryb;
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			curmlob = __builtin_addcl(curmlob, curmlob, static_cast<unsigned long>(carrysignb), &carrymidb);
			curmhib = __builtin_addcl(curmhib, curmhib, carrymidb, &carryb);
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#endif
			unsigned short checkcarryb;
			curob = __builtin_addcs(curob, 0, static_cast<unsigned short>(carryb), &checkcarryb);
			static_cast<void>(checkcarryb);
#elif defined(_M_X64)
			unsigned char carrysigna{_addcarry_u16(0, curoa, curoa, &curoa)};
			*reinterpret_cast<uint_least64_t *>(outa) = curma;
			*reinterpret_cast<uint_least64_t *>(dsta) = curma;
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u64(carrysigna, curma, curma, &curma), curoa, 0, &curoa)};
			static_cast<void>(checkcarrya);
			unsigned char carrysignb{_addcarry_u16(0, curob, curob, &curob)};
			*reinterpret_cast<uint_least64_t *>(outb) = curmb;
			*reinterpret_cast<uint_least64_t *>(dstb) = curmb;
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u64(carrysignb, curmb, curmb, &curmb), curob, 0, &curob)};
			static_cast<void>(checkcarryb);
#elif defined(_M_IX86)
			uint_least32_t curmloa{static_cast<uint_least32_t>(curma & 0xFFFFFFFFu)}, curmhia{static_cast<uint_least32_t>(curma >> 32)};// decompose
			unsigned char carrysigna{_addcarry_u16(0, curoa, curoa, &curoa)};
			*reinterpret_cast<uint_least64_t *>(outa) = curma;
			*reinterpret_cast<uint_least64_t *>(dsta) = curma;
			unsigned char checkcarrya{_addcarry_u16(_addcarry_u32(_addcarry_u32(carrysigna, curmloa, curmloa, &curmloa), curmhia, curmhia, &curmhia), curoa, 0, &curoa)};
			static_cast<void>(checkcarrya);
			alignas(8) uint_least32_t acurma[2]{curmloa, curmhia};
			curma = *reinterpret_cast<uint_least64_t *>(acurma);// recompose
			uint_least32_t curmlob{static_cast<uint_least32_t>(curmb & 0xFFFFFFFFu)}, curmhib{static_cast<uint_least32_t>(curmb >> 32)};// decompose
			unsigned char carrysignb{_addcarry_u16(0, curob, curob, &curob)};
			*reinterpret_cast<uint_least64_t *>(outb) = curmb;
			*reinterpret_cast<uint_least64_t *>(dstb) = curmb;
			unsigned char checkcarryb{_addcarry_u16(_addcarry_u32(_addcarry_u32(carrysignb, curmlob, curmlob, &curmlob), curmhib, curmhib, &curmhib), curob, 0, &curob)};
			static_cast<void>(checkcarryb);
			alignas(8) uint_least32_t acurmb[2]{curmlob, curmhib};
			curmb = *reinterpret_cast<uint_least64_t *>(acurmb);// recompose
#else
			uint_least16_t curotmpa{curoa};
			curoa += curoa;
			*reinterpret_cast<uint_least64_t *>(outa) = curma;
			*reinterpret_cast<uint_least64_t *>(dsta) = curma;
			uint_least64_t curmtmpa{curma};
			curma += curma;
			curma += curoa < curotmpa;
			curoa += curma < curmtmpa;
			uint_least16_t curotmpb{curob};
			curob += curob;
			*reinterpret_cast<uint_least64_t *>(outb) = curmb;
			*reinterpret_cast<uint_least64_t *>(dstb) = curmb;
			uint_least64_t curmtmpb{curmb};
			curmb += curmb;
			curmb += curob < curotmpb;
			curob += curmb < curmtmpb;
#endif
			curea = curoa;
			cureb = curob;
		}
	}
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(U &cur)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curp{static_cast<std::make_signed_t<T>>(cur)};
		if constexpr(isfloatingpoint || !issigned){
			T curo{static_cast<T>(cur)};
			curo += curo;
			cur = curo;
		}
		curp >>= CHAR_BIT * sizeof(T) - 1;
		U curq{static_cast<T>(curp)};
		if constexpr(isfloatingpoint) cur >>= 1;
		else if constexpr(issigned){
			T curo{static_cast<T>(cur)};
			curo += static_cast<T>(curq);
			cur = curo;
		}
		cur ^= curq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		if constexpr(issigned) cur &= ~static_cast<T>(0) >> 1;
		else cur = rotateleftportable<1>(static_cast<T>(cur));
	}
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(U &cur, T *out)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curp{static_cast<std::make_signed_t<T>>(cur)};
		*out = cur;
		if constexpr(isfloatingpoint || !issigned){
			T curo{static_cast<T>(cur)};
			curo += curo;
			cur = curo;
		}
		curp >>= CHAR_BIT * sizeof(T) - 1;
		U curq{static_cast<T>(curp)};
		if constexpr(isfloatingpoint) cur >>= 1;
		else if constexpr(issigned){
			T curo{static_cast<T>(cur)};
			curo += static_cast<T>(curq);
			cur = curo;
		}
		cur ^= curq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*out = cur;
		if constexpr(issigned) cur &= ~static_cast<T>(0) >> 1;
		else cur = rotateleftportable<1>(static_cast<T>(cur));
	}else *out = cur;
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(U &cur, T *out, T *dst)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curp{static_cast<std::make_signed_t<T>>(cur)};
		*out = cur;
		*dst = cur;
		if constexpr(isfloatingpoint || !issigned){
			T curo{static_cast<T>(cur)};
			curo += curo;
			cur = curo;
		}
		curp >>= CHAR_BIT * sizeof(T) - 1;
		U curq{static_cast<T>(curp)};
		if constexpr(isfloatingpoint) cur >>= 1;
		else if constexpr(issigned){
			T curo{static_cast<T>(cur)};
			curo += static_cast<T>(curq);
			cur = curo;
		}
		cur ^= curq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*out = cur;
		*dst = cur;
		if constexpr(issigned) cur &= ~static_cast<T>(0) >> 1;
		else cur = rotateleftportable<1>(static_cast<T>(cur));
	}else{
		*out = cur;
		*dst = cur;
	}
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(U &cura, U &curb)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		if constexpr(isfloatingpoint || !issigned){
			T curao{static_cast<T>(cura)};
			curao += curao;
			cura = curao;
		}
		curap >>= CHAR_BIT * sizeof(T) - 1;
		U curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		if constexpr(isfloatingpoint || !issigned){
			T curbo{static_cast<T>(curb)};
			curbo += curbo;
			curb = curbo;
		}
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		U curbq{static_cast<T>(curbp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
		}else if constexpr(issigned){
			T curao{static_cast<T>(cura)};
			T curbo{static_cast<T>(curb)};
			curao += static_cast<T>(curaq);
			curbo += static_cast<T>(curbq);
			cura = curao;
			curb = curbo;
		}
		cura ^= curaq;
		curb ^= curbq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		if constexpr(issigned){
			cura &= ~static_cast<T>(0) >> 1;
			curb &= ~static_cast<T>(0) >> 1;
		}else{
			cura = rotateleftportable<1>(static_cast<T>(cura));
			curb = rotateleftportable<1>(static_cast<T>(curb));
		}
	}
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(U &cura, T *outa, U &curb, T *outb)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		*outa = cura;
		if constexpr(isfloatingpoint || !issigned){
			T curao{static_cast<T>(cura)};
			curao += curao;
			cura = curao;
		}
		curap >>= CHAR_BIT * sizeof(T) - 1;
		U curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		*outb = curb;
		if constexpr(isfloatingpoint || !issigned){
			T curbo{static_cast<T>(curb)};
			curbo += curbo;
			curb = curbo;
		}
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		U curbq{static_cast<T>(curbp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
		}else if constexpr(issigned){
			T curao{static_cast<T>(cura)};
			T curbo{static_cast<T>(curb)};
			curao += static_cast<T>(curaq);
			curbo += static_cast<T>(curbq);
			cura = curao;
			curb = curbo;
		}
		cura ^= curaq;
		curb ^= curbq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*outa = cura;
		if constexpr(issigned) cura &= ~static_cast<T>(0) >> 1;
		else cura = rotateleftportable<1>(static_cast<T>(cura));
		*outb = curb;
		if constexpr(issigned) curb &= ~static_cast<T>(0) >> 1;
		else curb = rotateleftportable<1>(static_cast<T>(curb));
	}else{
		*outa = cura;
		*outb = curb;
	}
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(U &cura, T *outa, T *dsta, U &curb, T *outb, T *dstb)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		*outa = cura;
		*dsta = cura;
		if constexpr(isfloatingpoint || !issigned){
			T curao{static_cast<T>(cura)};
			curao += curao;
			cura = curao;
		}
		curap >>= CHAR_BIT * sizeof(T) - 1;
		U curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		*outb = curb;
		*dstb = curb;
		if constexpr(isfloatingpoint || !issigned){
			T curbo{static_cast<T>(curb)};
			curbo += curbo;
			curb = curbo;
		}
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		U curbq{static_cast<T>(curbp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
		}else if constexpr(issigned){
			T curao{static_cast<T>(cura)};
			T curbo{static_cast<T>(curb)};
			curao += static_cast<T>(curaq);
			curbo += static_cast<T>(curbq);
			cura = curao;
			curb = curbo;
		}
		cura ^= curaq;
		curb ^= curbq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*outa = cura;
		*dsta = cura;
		if constexpr(issigned) cura &= ~static_cast<T>(0) >> 1;
		else cura = rotateleftportable<1>(static_cast<T>(cura));
		*outb = curb;
		*dstb = curb;
		if constexpr(issigned) curb &= ~static_cast<T>(0) >> 1;
		else curb = rotateleftportable<1>(static_cast<T>(curb));
	}else{
		*outa = cura;
		*dsta = cura;
		*outb = curb;
		*dstb = curb;
	}
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(U &cura, U &curb, U &curc)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		if constexpr(isfloatingpoint || !issigned){
			T curao{static_cast<T>(cura)};
			curao += curao;
			cura = curao;
		}
		curap >>= CHAR_BIT * sizeof(T) - 1;
		U curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		if constexpr(isfloatingpoint || !issigned){
			T curbo{static_cast<T>(curb)};
			curbo += curbo;
			curb = curbo;
		}
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		U curbq{static_cast<T>(curbp)};
		std::make_signed_t<T> curcp{static_cast<std::make_signed_t<T>>(curc)};
		if constexpr(isfloatingpoint || !issigned){
			T curco{static_cast<T>(curc)};
			curco += curco;
			curc = curco;
		}
		curcp >>= CHAR_BIT * sizeof(T) - 1;
		U curcq{static_cast<T>(curcp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
			curc >>= 1;
		}else if constexpr(issigned){
			T curao{static_cast<T>(cura)};
			T curbo{static_cast<T>(curb)};
			T curco{static_cast<T>(curc)};
			curao += static_cast<T>(curaq);
			curbo += static_cast<T>(curbq);
			curco += static_cast<T>(curcq);
			cura = curao;
			curb = curbo;
			curc = curco;
		}
		cura ^= curaq;
		curb ^= curbq;
		curc ^= curcq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		if constexpr(issigned){
			cura &= ~static_cast<T>(0) >> 1;
			curb &= ~static_cast<T>(0) >> 1;
			curc &= ~static_cast<T>(0) >> 1;
		}else{
			cura = rotateleftportable<1>(static_cast<T>(cura));
			curb = rotateleftportable<1>(static_cast<T>(curb));
			curc = rotateleftportable<1>(static_cast<T>(curc));
		}
	}
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(U &cura, T *outa, U &curb, T *outb, U &curc, T *outc)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		*outa = cura;
		if constexpr(isfloatingpoint || !issigned){
			T curao{static_cast<T>(cura)};
			curao += curao;
			cura = curao;
		}
		curap >>= CHAR_BIT * sizeof(T) - 1;
		U curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		*outb = curb;
		if constexpr(isfloatingpoint || !issigned){
			T curbo{static_cast<T>(curb)};
			curbo += curbo;
			curb = curbo;
		}
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		U curbq{static_cast<T>(curbp)};
		std::make_signed_t<T> curcp{static_cast<std::make_signed_t<T>>(curc)};
		*outc = curc;
		if constexpr(isfloatingpoint || !issigned){
			T curco{static_cast<T>(curc)};
			curco += curco;
			curc = curco;
		}
		curcp >>= CHAR_BIT * sizeof(T) - 1;
		U curcq{static_cast<T>(curcp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
			curc >>= 1;
		}else if constexpr(issigned){
			T curao{static_cast<T>(cura)};
			T curbo{static_cast<T>(curb)};
			T curco{static_cast<T>(curc)};
			curao += static_cast<T>(curaq);
			curbo += static_cast<T>(curbq);
			curco += static_cast<T>(curcq);
			cura = curao;
			curb = curbo;
			curc = curco;
		}
		cura ^= curaq;
		curb ^= curbq;
		curc ^= curcq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*outa = cura;
		if constexpr(issigned) cura &= ~static_cast<T>(0) >> 1;
		else cura = rotateleftportable<1>(static_cast<T>(cura));
		*outb = curb;
		if constexpr(issigned) curb &= ~static_cast<T>(0) >> 1;
		else curb = rotateleftportable<1>(static_cast<T>(curb));
		*outc = curc;
		if constexpr(issigned) curb &= ~static_cast<T>(0) >> 1;
		else curc = rotateleftportable<1>(static_cast<T>(curc));
	}else{
		*outa = cura;
		*outb = curb;
		*outc = curc;
	}
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(U &cura, T *outa, T *dsta, U &curb, T *outb, T *dstb, U &curc, T *outc, T *dstc)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		*outa = cura;
		*dsta = cura;
		if constexpr(isfloatingpoint || !issigned){
			T curao{static_cast<T>(cura)};
			curao += curao;
			cura = curao;
		}
		curap >>= CHAR_BIT * sizeof(T) - 1;
		U curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		*outb = curb;
		*dstb = curb;
		if constexpr(isfloatingpoint || !issigned){
			T curbo{static_cast<T>(curb)};
			curbo += curbo;
			curb = curbo;
		}
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		U curbq{static_cast<T>(curbp)};
		std::make_signed_t<T> curcp{static_cast<std::make_signed_t<T>>(curc)};
		*outc = curc;
		*dstc = curc;
		if constexpr(isfloatingpoint || !issigned){
			T curco{static_cast<T>(curc)};
			curco += curco;
			curc = curco;
		}
		curcp >>= CHAR_BIT * sizeof(T) - 1;
		U curcq{static_cast<T>(curcp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
			curc >>= 1;
		}else if constexpr(issigned){
			T curao{static_cast<T>(cura)};
			T curbo{static_cast<T>(curb)};
			T curco{static_cast<T>(curc)};
			curao += static_cast<T>(curaq);
			curbo += static_cast<T>(curbq);
			curco += static_cast<T>(curcq);
			cura = curao;
			curb = curbo;
			curc = curco;
		}
		cura ^= curaq;
		curb ^= curbq;
		curc ^= curcq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*outa = cura;
		*dsta = cura;
		if constexpr(issigned) cura &= ~static_cast<T>(0) >> 1;
		else cura = rotateleftportable<1>(static_cast<T>(cura));
		*outb = curb;
		*dstb = curb;
		if constexpr(issigned) curb &= ~static_cast<T>(0) >> 1;
		else curb = rotateleftportable<1>(static_cast<T>(curb));
		*outc = curc;
		*dstc = curc;
		if constexpr(issigned) curc &= ~static_cast<T>(0) >> 1;
		else curc = rotateleftportable<1>(static_cast<T>(curc));
	}else{
		*outa = cura;
		*dsta = cura;
		*outb = curb;
		*dstb = curb;
		*outc = curc;
		*dstc = curc;
	}
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(U &cura, U &curb, U &curc, U &curd)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		if constexpr(isfloatingpoint || !issigned){
			T curao{static_cast<T>(cura)};
			curao += curao;
			cura = curao;
		}
		curap >>= CHAR_BIT * sizeof(T) - 1;
		U curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		if constexpr(isfloatingpoint || !issigned){
			T curbo{static_cast<T>(curb)};
			curbo += curbo;
			curb = curbo;
		}
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		U curbq{static_cast<T>(curbp)};
		std::make_signed_t<T> curcp{static_cast<std::make_signed_t<T>>(curc)};
		if constexpr(isfloatingpoint || !issigned){
			T curco{static_cast<T>(curc)};
			curco += curco;
			curc = curco;
		}
		curcp >>= CHAR_BIT * sizeof(T) - 1;
		U curcq{static_cast<T>(curcp)};
		std::make_signed_t<T> curdp{static_cast<std::make_signed_t<T>>(curd)};
		if constexpr(isfloatingpoint || !issigned){
			T curdo{static_cast<T>(curd)};
			curdo += curdo;
			curd = curdo;
		}
		curdp >>= CHAR_BIT * sizeof(T) - 1;
		U curdq{static_cast<T>(curdp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
			curc >>= 1;
			curd >>= 1;
		}else if constexpr(issigned){
			T curao{static_cast<T>(cura)};
			T curbo{static_cast<T>(curb)};
			T curco{static_cast<T>(curc)};
			T curdo{static_cast<T>(curd)};
			curao += static_cast<T>(curaq);
			curbo += static_cast<T>(curbq);
			curco += static_cast<T>(curcq);
			curdo += static_cast<T>(curdq);
			cura = curao;
			curb = curbo;
			curc = curco;
			curd = curdo;
		}
		cura ^= curaq;
		curb ^= curbq;
		curc ^= curcq;
		curd ^= curdq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		if constexpr(issigned){
			cura &= ~static_cast<T>(0) >> 1;
			curb &= ~static_cast<T>(0) >> 1;
			curc &= ~static_cast<T>(0) >> 1;
			curd &= ~static_cast<T>(0) >> 1;
		}else{
			cura = rotateleftportable<1>(static_cast<T>(cura));
			curb = rotateleftportable<1>(static_cast<T>(curb));
			curc = rotateleftportable<1>(static_cast<T>(curc));
			curd = rotateleftportable<1>(static_cast<T>(curd));
		}
	}
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(U &cura, T *outa, U &curb, T *outb, U &curc, T *outc, U &curd, T *outd)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		*outa = cura;
		if constexpr(isfloatingpoint || !issigned){
			T curao{static_cast<T>(cura)};
			curao += curao;
			cura = curao;
		}
		curap >>= CHAR_BIT * sizeof(T) - 1;
		U curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		*outb = curb;
		if constexpr(isfloatingpoint || !issigned){
			T curbo{static_cast<T>(curb)};
			curbo += curbo;
			curb = curbo;
		}
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		U curbq{static_cast<T>(curbp)};
		std::make_signed_t<T> curcp{static_cast<std::make_signed_t<T>>(curc)};
		*outc = curc;
		if constexpr(isfloatingpoint || !issigned){
			T curco{static_cast<T>(curc)};
			curco += curco;
			curc = curco;
		}
		curcp >>= CHAR_BIT * sizeof(T) - 1;
		U curcq{static_cast<T>(curcp)};
		std::make_signed_t<T> curdp{static_cast<std::make_signed_t<T>>(curd)};
		*outd = curd;
		if constexpr(isfloatingpoint || !issigned){
			T curdo{static_cast<T>(curd)};
			curdo += curdo;
			curd = curdo;
		}
		curdp >>= CHAR_BIT * sizeof(T) - 1;
		U curdq{static_cast<T>(curdp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
			curc >>= 1;
			curd >>= 1;
		}else if constexpr(issigned){
			T curao{static_cast<T>(cura)};
			T curbo{static_cast<T>(curb)};
			T curco{static_cast<T>(curc)};
			T curdo{static_cast<T>(curd)};
			curao += static_cast<T>(curaq);
			curbo += static_cast<T>(curbq);
			curco += static_cast<T>(curcq);
			curdo += static_cast<T>(curdq);
			cura = curao;
			curb = curbo;
			curc = curco;
			curd = curdo;
		}
		cura ^= curaq;
		curb ^= curbq;
		curc ^= curcq;
		curd ^= curdq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*outa = cura;
		if constexpr(issigned) cura &= ~static_cast<T>(0) >> 1;
		else cura = rotateleftportable<1>(static_cast<T>(cura));
		*outb = curb;
		if constexpr(issigned) curb &= ~static_cast<T>(0) >> 1;
		else curb = rotateleftportable<1>(static_cast<T>(curb));
		*outc = curc;
		if constexpr(issigned) curc &= ~static_cast<T>(0) >> 1;
		else curc = rotateleftportable<1>(static_cast<T>(curc));
		*outd = curd;
		if constexpr(issigned) curd &= ~static_cast<T>(0) >> 1;
		else curd = rotateleftportable<1>(static_cast<T>(curd));
	}else{
		*outa = cura;
		*outb = curb;
		*outc = curc;
		*outd = curd;
	}
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(U &cura, T *outa, T *dsta, U &curb, T *outb, T *dstb, U &curc, T *outc, T *dstc, U &curd, T *outd, T *dstd)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		*outa = cura;
		*dsta = cura;
		if constexpr(isfloatingpoint || !issigned){
			T curao{static_cast<T>(cura)};
			curao += curao;
			cura = curao;
		}
		curap >>= CHAR_BIT * sizeof(T) - 1;
		U curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		*outb = curb;
		*dstb = curb;
		if constexpr(isfloatingpoint || !issigned){
			T curbo{static_cast<T>(curb)};
			curbo += curbo;
			curb = curbo;
		}
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		U curbq{static_cast<T>(curbp)};
		std::make_signed_t<T> curcp{static_cast<std::make_signed_t<T>>(curc)};
		*outc = curc;
		*dstc = curc;
		if constexpr(isfloatingpoint || !issigned){
			T curco{static_cast<T>(curc)};
			curco += curco;
			curc = curco;
		}
		curcp >>= CHAR_BIT * sizeof(T) - 1;
		U curcq{static_cast<T>(curcp)};
		std::make_signed_t<T> curdp{static_cast<std::make_signed_t<T>>(curd)};
		*outd = curd;
		*dstd = curd;
		if constexpr(isfloatingpoint || !issigned){
			T curdo{static_cast<T>(curd)};
			curdo += curdo;
			curd = curdo;
		}
		curdp >>= CHAR_BIT * sizeof(T) - 1;
		U curdq{static_cast<T>(curdp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
			curc >>= 1;
			curd >>= 1;
		}else if constexpr(issigned){
			T curao{static_cast<T>(cura)};
			T curbo{static_cast<T>(curb)};
			T curco{static_cast<T>(curc)};
			T curdo{static_cast<T>(curd)};
			curao += static_cast<T>(curaq);
			curbo += static_cast<T>(curbq);
			curco += static_cast<T>(curcq);
			curdo += static_cast<T>(curdq);
			cura = curao;
			curb = curbo;
			curc = curco;
			curd = curdo;
		}
		cura ^= curaq;
		curb ^= curbq;
		curc ^= curcq;
		curd ^= curdq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*outa = cura;
		*dsta = cura;
		if constexpr(issigned) cura &= ~static_cast<T>(0) >> 1;
		else cura = rotateleftportable<1>(static_cast<T>(cura));
		*outb = curb;
		*dstb = curb;
		if constexpr(issigned) curb &= ~static_cast<T>(0) >> 1;
		else curb = rotateleftportable<1>(static_cast<T>(curb));
		*outc = curc;
		*dstc = curc;
		if constexpr(issigned) curc &= ~static_cast<T>(0) >> 1;
		else curc = rotateleftportable<1>(static_cast<T>(curc));
		*outd = curd;
		*dstd = curd;
		if constexpr(issigned) curd &= ~static_cast<T>(0) >> 1;
		else curd = rotateleftportable<1>(static_cast<T>(curd));
	}else{
		*outa = cura;
		*dsta = cura;
		*outb = curb;
		*dstb = curb;
		*outc = curc;
		*dstc = curc;
		*outd = curd;
		*dstd = curd;
	}
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(U &cura, U &curb, U &curc, U &curd, U &cure, U &curf, U &curg, U &curh)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		if constexpr(isfloatingpoint || !issigned){
			T curao{static_cast<T>(cura)};
			curao += curao;
			cura = curao;
		}
		curap >>= CHAR_BIT * sizeof(T) - 1;
		U curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		if constexpr(isfloatingpoint || !issigned){
			T curbo{static_cast<T>(curb)};
			curbo += curbo;
			curb = curbo;
		}
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		U curbq{static_cast<T>(curbp)};
		std::make_signed_t<T> curcp{static_cast<std::make_signed_t<T>>(curc)};
		if constexpr(isfloatingpoint || !issigned){
			T curco{static_cast<T>(curc)};
			curco += curco;
			curc = curco;
		}
		curcp >>= CHAR_BIT * sizeof(T) - 1;
		U curcq{static_cast<T>(curcp)};
		std::make_signed_t<T> curdp{static_cast<std::make_signed_t<T>>(curd)};
		if constexpr(isfloatingpoint || !issigned){
			T curdo{static_cast<T>(curd)};
			curdo += curdo;
			curd = curdo;
		}
		curdp >>= CHAR_BIT * sizeof(T) - 1;
		U curdq{static_cast<T>(curdp)};
		std::make_signed_t<T> curep{static_cast<std::make_signed_t<T>>(cure)};
		if constexpr(isfloatingpoint || !issigned){
			T cureo{static_cast<T>(cure)};
			cureo += cureo;
			cure = cureo;
		}
		curep >>= CHAR_BIT * sizeof(T) - 1;
		U cureq{static_cast<T>(curep)};
		std::make_signed_t<T> curfp{static_cast<std::make_signed_t<T>>(curf)};
		if constexpr(isfloatingpoint || !issigned){
			T curfo{static_cast<T>(curf)};
			curfo += curfo;
			curf = curfo;
		}
		curfp >>= CHAR_BIT * sizeof(T) - 1;
		U curfq{static_cast<T>(curfp)};
		std::make_signed_t<T> curgp{static_cast<std::make_signed_t<T>>(curg)};
		if constexpr(isfloatingpoint || !issigned){
			T curgo{static_cast<T>(curg)};
			curgo += curgo;
			curg = curgo;
		}
		curgp >>= CHAR_BIT * sizeof(T) - 1;
		U curgq{static_cast<T>(curgp)};
		std::make_signed_t<T> curhp{static_cast<std::make_signed_t<T>>(curh)};
		if constexpr(isfloatingpoint || !issigned){
			T curho{static_cast<T>(curh)};
			curho += curho;
			curh = curho;
		}
		curhp >>= CHAR_BIT * sizeof(T) - 1;
		U curhq{static_cast<T>(curhp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
			curc >>= 1;
			curd >>= 1;
			cure >>= 1;
			curf >>= 1;
			curg >>= 1;
			curh >>= 1;
		}else if constexpr(issigned){
			T curao{static_cast<T>(cura)};
			T curbo{static_cast<T>(curb)};
			T curco{static_cast<T>(curc)};
			T curdo{static_cast<T>(curd)};
			T cureo{static_cast<T>(cure)};
			T curfo{static_cast<T>(curf)};
			T curgo{static_cast<T>(curg)};
			T curho{static_cast<T>(curh)};
			curao += static_cast<T>(curaq);
			curbo += static_cast<T>(curbq);
			curco += static_cast<T>(curcq);
			curdo += static_cast<T>(curdq);
			cureo += static_cast<T>(cureq);
			curfo += static_cast<T>(curfq);
			curgo += static_cast<T>(curgq);
			curho += static_cast<T>(curhq);
			cura = curao;
			curb = curbo;
			curc = curco;
			curd = curdo;
			cure = cureo;
			curf = curfo;
			curg = curgo;
			curh = curho;
		}
		cura ^= curaq;
		curb ^= curbq;
		curc ^= curcq;
		curd ^= curdq;
		cure ^= cureq;
		curf ^= curfq;
		curg ^= curgq;
		curh ^= curhq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		if constexpr(issigned){
			cura &= ~static_cast<T>(0) >> 1;
			curb &= ~static_cast<T>(0) >> 1;
			curc &= ~static_cast<T>(0) >> 1;
			curd &= ~static_cast<T>(0) >> 1;
			cure &= ~static_cast<T>(0) >> 1;
			curf &= ~static_cast<T>(0) >> 1;
			curg &= ~static_cast<T>(0) >> 1;
			curh &= ~static_cast<T>(0) >> 1;
		}else{
			cura = rotateleftportable<1>(static_cast<T>(cura));
			curb = rotateleftportable<1>(static_cast<T>(curb));
			curc = rotateleftportable<1>(static_cast<T>(curc));
			curd = rotateleftportable<1>(static_cast<T>(curd));
			cure = rotateleftportable<1>(static_cast<T>(cure));
			curf = rotateleftportable<1>(static_cast<T>(curf));
			curg = rotateleftportable<1>(static_cast<T>(curg));
			curh = rotateleftportable<1>(static_cast<T>(curh));
		}
	}
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(U &cura, T *outa, U &curb, T *outb, U &curc, T *outc, U &curd, T *outd, U &cure, T *oute, U &curf, T *outf, U &curg, T *outg, U &curh, T *outh)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		*outa = cura;
		if constexpr(isfloatingpoint || !issigned){
			T curao{static_cast<T>(cura)};
			curao += curao;
			cura = curao;
		}
		curap >>= CHAR_BIT * sizeof(T) - 1;
		U curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		*outb = curb;
		if constexpr(isfloatingpoint || !issigned){
			T curbo{static_cast<T>(curb)};
			curbo += curbo;
			curb = curbo;
		}
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		U curbq{static_cast<T>(curbp)};
		std::make_signed_t<T> curcp{static_cast<std::make_signed_t<T>>(curc)};
		*outc = curc;
		if constexpr(isfloatingpoint || !issigned){
			T curco{static_cast<T>(curc)};
			curco += curco;
			curc = curco;
		}
		curcp >>= CHAR_BIT * sizeof(T) - 1;
		U curcq{static_cast<T>(curcp)};
		std::make_signed_t<T> curdp{static_cast<std::make_signed_t<T>>(curd)};
		*outd = curd;
		if constexpr(isfloatingpoint || !issigned){
			T curdo{static_cast<T>(curd)};
			curdo += curdo;
			curd = curdo;
		}
		curdp >>= CHAR_BIT * sizeof(T) - 1;
		U curdq{static_cast<T>(curdp)};
		std::make_signed_t<T> curep{static_cast<std::make_signed_t<T>>(cure)};
		*oute = cure;
		if constexpr(isfloatingpoint || !issigned){
			T cureo{static_cast<T>(cure)};
			cureo += cureo;
			cure = cureo;
		}
		curep >>= CHAR_BIT * sizeof(T) - 1;
		U cureq{static_cast<T>(curep)};
		std::make_signed_t<T> curfp{static_cast<std::make_signed_t<T>>(curf)};
		*outf = curf;
		if constexpr(isfloatingpoint || !issigned){
			T curfo{static_cast<T>(curf)};
			curfo += curfo;
			curf = curfo;
		}
		curfp >>= CHAR_BIT * sizeof(T) - 1;
		U curfq{static_cast<T>(curfp)};
		std::make_signed_t<T> curgp{static_cast<std::make_signed_t<T>>(curg)};
		*outg = curg;
		if constexpr(isfloatingpoint || !issigned){
			T curgo{static_cast<T>(curg)};
			curgo += curgo;
			curg = curgo;
		}
		curgp >>= CHAR_BIT * sizeof(T) - 1;
		U curgq{static_cast<T>(curgp)};
		std::make_signed_t<T> curhp{static_cast<std::make_signed_t<T>>(curh)};
		*outh = curh;
		if constexpr(isfloatingpoint || !issigned){
			T curho{static_cast<T>(curh)};
			curho += curho;
			curh = curho;
		}
		curhp >>= CHAR_BIT * sizeof(T) - 1;
		U curhq{static_cast<T>(curhp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
			curc >>= 1;
			curd >>= 1;
			cure >>= 1;
			curf >>= 1;
			curg >>= 1;
			curh >>= 1;
		}else if constexpr(issigned){
			T curao{static_cast<T>(cura)};
			T curbo{static_cast<T>(curb)};
			T curco{static_cast<T>(curc)};
			T curdo{static_cast<T>(curd)};
			T cureo{static_cast<T>(cure)};
			T curfo{static_cast<T>(curf)};
			T curgo{static_cast<T>(curg)};
			T curho{static_cast<T>(curh)};
			curao += static_cast<T>(curaq);
			curbo += static_cast<T>(curbq);
			curco += static_cast<T>(curcq);
			curdo += static_cast<T>(curdq);
			cureo += static_cast<T>(cureq);
			curfo += static_cast<T>(curfq);
			curgo += static_cast<T>(curgq);
			curho += static_cast<T>(curhq);
			cura = curao;
			curb = curbo;
			curc = curco;
			curd = curdo;
			cure = cureo;
			curf = curfo;
			curg = curgo;
			curh = curho;
		}
		cura ^= curaq;
		curb ^= curbq;
		curc ^= curcq;
		curd ^= curdq;
		cure ^= cureq;
		curf ^= curfq;
		curg ^= curgq;
		curh ^= curhq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*outa = cura;
		if constexpr(issigned) cura &= ~static_cast<T>(0) >> 1;
		else cura = rotateleftportable<1>(static_cast<T>(cura));
		*outb = curb;
		if constexpr(issigned) curb &= ~static_cast<T>(0) >> 1;
		else curb = rotateleftportable<1>(static_cast<T>(curb));
		*outc = curc;
		if constexpr(issigned) curc &= ~static_cast<T>(0) >> 1;
		else curc = rotateleftportable<1>(static_cast<T>(curc));
		*outd = curd;
		if constexpr(issigned) curd &= ~static_cast<T>(0) >> 1;
		else curd = rotateleftportable<1>(static_cast<T>(curd));
		*oute = cure;
		if constexpr(issigned) cure &= ~static_cast<T>(0) >> 1;
		else cure = rotateleftportable<1>(static_cast<T>(cure));
		*outf = curf;
		if constexpr(issigned) curf &= ~static_cast<T>(0) >> 1;
		else curf = rotateleftportable<1>(static_cast<T>(curf));
		*outg = curg;
		if constexpr(issigned) curg &= ~static_cast<T>(0) >> 1;
		else curg = rotateleftportable<1>(static_cast<T>(curg));
		*outh = curh;
		if constexpr(issigned) curh &= ~static_cast<T>(0) >> 1;
		else curh = rotateleftportable<1>(static_cast<T>(curh));
	}else{
		*outa = cura;
		*outb = curb;
		*outc = curc;
		*outd = curd;
		*oute = cure;
		*outf = curf;
		*outg = curg;
		*outh = curh;
	}
}

template<bool absolute, bool issigned, bool isfloatingpoint, typename T, typename U>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	std::is_unsigned_v<U> &&
	64 >= CHAR_BIT * sizeof(U),
	void> filterinput(U &cura, T *outa, T *dsta, U &curb, T *outb, T *dstb, U &curc, T *outc, T *dstc, U &curd, T *outd, T *dstd, U &cure, T *oute, T *dste, U &curf, T *outf, T *dstf, U &curg, T *outg, T *dstg, U &curh, T *outh, T *dsth)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		*outa = cura;
		*dsta = cura;
		if constexpr(isfloatingpoint || !issigned){
			T curao{static_cast<T>(cura)};
			curao += curao;
			cura = curao;
		}
		curap >>= CHAR_BIT * sizeof(T) - 1;
		U curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		*outb = curb;
		*dstb = curb;
		if constexpr(isfloatingpoint || !issigned){
			T curbo{static_cast<T>(curb)};
			curbo += curbo;
			curb = curbo;
		}
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		U curbq{static_cast<T>(curbp)};
		std::make_signed_t<T> curcp{static_cast<std::make_signed_t<T>>(curc)};
		*outc = curc;
		*dstc = curc;
		if constexpr(isfloatingpoint || !issigned){
			T curco{static_cast<T>(curc)};
			curco += curco;
			curc = curco;
		}
		curcp >>= CHAR_BIT * sizeof(T) - 1;
		U curcq{static_cast<T>(curcp)};
		std::make_signed_t<T> curdp{static_cast<std::make_signed_t<T>>(curd)};
		*outd = curd;
		*dstd = curd;
		if constexpr(isfloatingpoint || !issigned){
			T curdo{static_cast<T>(curd)};
			curdo += curdo;
			curd = curdo;
		}
		curdp >>= CHAR_BIT * sizeof(T) - 1;
		U curdq{static_cast<T>(curdp)};
		std::make_signed_t<T> curep{static_cast<std::make_signed_t<T>>(cure)};
		*oute = cure;
		*dste = cure;
		if constexpr(isfloatingpoint || !issigned){
			T cureo{static_cast<T>(cure)};
			cureo += cureo;
			cure = cureo;
		}
		curep >>= CHAR_BIT * sizeof(T) - 1;
		U cureq{static_cast<T>(curep)};
		std::make_signed_t<T> curfp{static_cast<std::make_signed_t<T>>(curf)};
		*outf = curf;
		*dstf = curf;
		if constexpr(isfloatingpoint || !issigned){
			T curfo{static_cast<T>(curf)};
			curfo += curfo;
			curf = curfo;
		}
		curfp >>= CHAR_BIT * sizeof(T) - 1;
		U curfq{static_cast<T>(curfp)};
		std::make_signed_t<T> curgp{static_cast<std::make_signed_t<T>>(curg)};
		*outg = curg;
		*dstg = curg;
		if constexpr(isfloatingpoint || !issigned){
			T curgo{static_cast<T>(curg)};
			curgo += curgo;
			curg = curgo;
		}
		curgp >>= CHAR_BIT * sizeof(T) - 1;
		U curgq{static_cast<T>(curgp)};
		std::make_signed_t<T> curhp{static_cast<std::make_signed_t<T>>(curh)};
		*outh = curh;
		*dsth = curh;
		if constexpr(isfloatingpoint || !issigned){
			T curho{static_cast<T>(curh)};
			curho += curho;
			curh = curho;
		}
		curhp >>= CHAR_BIT * sizeof(T) - 1;
		U curhq{static_cast<T>(curhp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
			curc >>= 1;
			curd >>= 1;
			cure >>= 1;
			curf >>= 1;
			curg >>= 1;
			curh >>= 1;
		}else if constexpr(issigned){
			T curao{static_cast<T>(cura)};
			T curbo{static_cast<T>(curb)};
			T curco{static_cast<T>(curc)};
			T curdo{static_cast<T>(curd)};
			T cureo{static_cast<T>(cure)};
			T curfo{static_cast<T>(curf)};
			T curgo{static_cast<T>(curg)};
			T curho{static_cast<T>(curh)};
			curao += static_cast<T>(curaq);
			curbo += static_cast<T>(curbq);
			curco += static_cast<T>(curcq);
			curdo += static_cast<T>(curdq);
			cureo += static_cast<T>(cureq);
			curfo += static_cast<T>(curfq);
			curgo += static_cast<T>(curgq);
			curho += static_cast<T>(curhq);
			cura = curao;
			curb = curbo;
			curc = curco;
			curd = curdo;
			cure = cureo;
			curf = curfo;
			curg = curgo;
			curh = curho;
		}
		cura ^= curaq;
		curb ^= curbq;
		curc ^= curcq;
		curd ^= curdq;
		cure ^= cureq;
		curf ^= curfq;
		curg ^= curgq;
		curh ^= curhq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*outa = cura;
		*dsta = cura;
		if constexpr(issigned) cura &= ~static_cast<T>(0) >> 1;
		else cura = rotateleftportable<1>(static_cast<T>(cura));
		*outb = curb;
		*dstb = curb;
		if constexpr(issigned) curb &= ~static_cast<T>(0) >> 1;
		else curb = rotateleftportable<1>(static_cast<T>(curb));
		*outc = curc;
		*dstc = curc;
		if constexpr(issigned) curc &= ~static_cast<T>(0) >> 1;
		else curc = rotateleftportable<1>(static_cast<T>(curc));
		*outd = curd;
		*dstd = curd;
		if constexpr(issigned) curd &= ~static_cast<T>(0) >> 1;
		else curd = rotateleftportable<1>(static_cast<T>(curd));
		*oute = cure;
		*dste = cure;
		if constexpr(issigned) cure &= ~static_cast<T>(0) >> 1;
		else cure = rotateleftportable<1>(static_cast<T>(cure));
		*outf = curf;
		*dstf = curf;
		if constexpr(issigned) curf &= ~static_cast<T>(0) >> 1;
		else curf = rotateleftportable<1>(static_cast<T>(curf));
		*outg = curg;
		*dstg = curg;
		if constexpr(issigned) curg &= ~static_cast<T>(0) >> 1;
		else curg = rotateleftportable<1>(static_cast<T>(curg));
		*outh = curh;
		*dsth = curh;
		if constexpr(issigned) curh &= ~static_cast<T>(0) >> 1;
		else curh = rotateleftportable<1>(static_cast<T>(curh));
	}else{
		*outa = cura;
		*dsta = cura;
		*outb = curb;
		*dstb = curb;
		*outc = curc;
		*dstc = curc;
		*outd = curd;
		*dstd = curd;
		*oute = cure;
		*dste = cure;
		*outf = curf;
		*dstf = curf;
		*outg = curg;
		*dstg = curg;
		*outh = curh;
		*dsth = curh;
	}
}

// Helper functions to implement the offset transforms

template<bool reversesort, bool absolute, bool issigned, bool isfloatingpoint, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	std::pair<unsigned, unsigned>> generateoffsetsmulti(size_t count, size_t offsets[], unsigned paritybool = 0)noexcept{
	// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
	// reversesort is frequently optimised away in this part, e.g.: reversesort * 2 - 1 generates 1 or -1
	//
	// one of the special mode operates differently: absolute floating-point, but negative inputs will sort just below their positive counterparts
	// Bit rotate left by one is the first filtering step to make this possible.
	// Sort as unsigned, just with the least significant bit flipped to complete the filter.
	// As an example of this, the 8-bit sorting pattern in ascending mode:
	// 0b1000'0000 -0.
	// 0b0000'0000 +0.
	// 0b1000'0001 -exp2(1 - mantissabitcount - exponentbias)
	// 0b0000'0001 +exp2(1 - mantissabitcount - exponentbias)
	// ...
	// 0b1111'1111 -QNaN (maximum amount of ones)
	// 0b0111'1111 +QNaN (maximum amount of ones)
	// Determining the starting point depends of several factors here.
	static size_t constexpr offsetsstride{CHAR_BIT * sizeof(T) * 256 / 8 - (absolute && issigned) * (127 + isfloatingpoint)};// shrink the offsets size if possible
	size_t *t{offsets// either aim to cache low-to-high or high-to-low
		+ reversesort * (offsetsstride - 1 - (issigned && !absolute) * 256 / 2)
		- (isfloatingpoint && !issigned && absolute) * (reversesort * 2 - 1)};
	unsigned runsteps{(1 << CHAR_BIT * sizeof(T) / 8) - 1};
	// handle the sign bit, virtually offset the top part by half the range here
	if constexpr(issigned && !absolute && reversesort){
		size_t offset{*t};// the starting point was already adjusted to the signed range
		*t-- = 0;// the first offset always starts at zero
		unsigned b{count < offset};// carry-out can only happen once per cycle here, so optimise that
		unsigned j{256 / 2 - 1};
		do{
			size_t difference{*t};
			*t = offset;
			t[offsetsstride + 1] = offset - 1;// high half
			--t;
			offset += difference;
			addcarryofless(b, count, difference);
		}while(--j);
		size_t differencemid{t[256]};
		t[256] = offset;
		t[offsetsstride + 1] = offset - 1;// high half
		t += 256 - 1;// offset to the end of the range
		offset += differencemid;
		addcarryofless(b, count, differencemid);
		j = 256 / 2 - 2;
		do{
			size_t difference{*t};
			*t = offset;
			t[offsetsstride + 1] = offset - 1;// high half
			--t;
			offset -= difference;
			addcarryofless(b, count, difference);
		}while(--j);
		addcarryofless(b, count, *t);
		*t = offset;
		t[offsetsstride] = count;// high half, the last offset always starts at the end
		t[offsetsstride + 1] = offset - 1;// high half
		t -= 256 / 2 + 1;// offset to the next part to process
		paritybool ^= b;
		runsteps ^= b << (CHAR_BIT * sizeof(T) / 8 - 1);
	}
#if defined(__has_cpp_attribute) && __has_cpp_attribute(maybe_unused)
	[[maybe_unused]]
#endif
	int k{reversesort * (CHAR_BIT * sizeof(T) / 8 - 1 - (issigned && !absolute))};
	// custom loop for the special mode: absolute floating-point, but negative inputs will sort just below their positive counterparts
	if constexpr(isfloatingpoint && !issigned && absolute) do{// starts at one removed from the initial index
		size_t offset{*t};// odd
		*t = 0;// the first offset always starts at zero
		t += reversesort * 2 - 1;// step back
		unsigned b{count < offset};// carry-out can only happen once per cycle here, so optimise that
		unsigned j{(256 - 2) / 2};// double the number of items per loop
		do{
			size_t difference{*t};// even
			*t = offset;
			t[offsetsstride - reversesort * 2 + 1] = offset - 1;// odd, high half
			offset += difference;
			addcarryofless(b, count, difference);
			difference = t[reversesort * -6 + 3];// odd
			t[reversesort * -6 + 3] = offset;
			t[offsetsstride] = offset - 1;// even, high half
			t += reversesort * -4 + 2;// step forward twice
			offset += difference;
			addcarryofless(b, count, difference);
		}while(--j);
		addcarryofless(b, count, *t);
		*t = offset;// even
		t[offsetsstride] = count;// even, high half, the last offset always starts at the end
		t[offsetsstride - reversesort * 2 + 1] = offset - 1;// odd, high half
		t += reversesort * -6 + 3;// step forward thrice
		paritybool ^= b;
		runsteps ^= b << k;
	}while(reversesort? (0 <= --k) : (static_cast<int>(CHAR_BIT * sizeof(T) / 8) > ++k));
	else do{// all other modes
		size_t offset{*t};
		*t = 0;// the first offset always starts at zero
		t -= reversesort * 2 - 1;
		unsigned b{count < offset};// carry-out can only happen once per cycle here, so optimise that
		unsigned j{256 - 2};
		do{
			size_t difference{*t};
			*t = offset;
			t[offsetsstride + reversesort * 2 - 1] = offset - 1;// high half
			t -= reversesort * 2 - 1;
			offset += difference;
			addcarryofless(b, count, difference);
		}while(--j);
		addcarryofless(b, count, *t);
		*t = offset;
		t[offsetsstride] = count;// high half, the last offset always starts at the end
		t[offsetsstride + reversesort * 2 - 1] = offset - 1;// high half
		if constexpr(16 < CHAR_BIT * sizeof(T) || !issigned || absolute || !reversesort) t -= reversesort * 2 - 1;// only the reverse sorting mode for signed 16-bit types can skip this
		paritybool ^= b;
		runsteps ^= b << k;
	}while((16 == CHAR_BIT * sizeof(T) && issigned && !absolute)? false :
		reversesort? (0 <= --k) : (static_cast<int>(CHAR_BIT * sizeof(T) / 8 - (issigned && !absolute)) > ++k));
	// handle the sign bit, virtually offset the top part by half the range here
	if constexpr(issigned && !absolute && !reversesort){
		size_t offset{t[256 / 2]};
		t[256 / 2] = 0;// the first offset always starts at zero
		t += 256 / 2 + 1;
		unsigned b{count < offset};// carry-out can only happen once per cycle here, so optimise that
		unsigned j{256 / 2 - 1};
		do{
			size_t difference{*t};
			*t = offset;
			t[offsetsstride - 1] = offset - 1;// high half
			++t;
			offset += difference;
			addcarryofless(b, count, difference);
		}while(--j);
		size_t differencemid{t[-256]};
		t[-256] = offset;
		t[offsetsstride - 1] = offset - 1;// high half
		t -= 256 - 1;// offset to the start of the range
		offset += differencemid;
		addcarryofless(b, count, differencemid);
		j = 256 / 2 - 2;
		do{
			size_t difference{*t};
			*t = offset;
			t[offsetsstride - 1] = offset - 1;// high half
			++t;
			offset += difference;
			addcarryofless(b, count, difference);
		}while(--j);
		addcarryofless(b, count, *t);
		*t = offset;
		t[offsetsstride] = count;// high half, the last offset always starts at the end
		t[offsetsstride - 1] = offset - 1;// high half
		paritybool ^= b;
		runsteps ^= b << (CHAR_BIT * sizeof(T) / 8 - 1);
	}
	return{runsteps, paritybool};// paritybool will be 1 for when the swap count is odd
}

template<bool reversesort, bool absolute, bool issigned, bool isfloatingpoint, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	8 >= CHAR_BIT * sizeof(T),
	bool> generateoffsetssingle(size_t count, size_t offsets[])noexcept{
	// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
	// reversesort is frequently optimised away in this part, e.g.: reversesort * 2 - 1 generates 1 or -1
	//
	// one of the special mode operates differently: absolute floating-point, but negative inputs will sort just below their positive counterparts
	// Bit rotate left by one is the first filtering step to make this possible.
	// Sort as unsigned, just with the least significant bit flipped to complete the filter.
	// As an example of this, the 8-bit sorting pattern in ascending mode:
	// 0b1000'0000 -0.
	// 0b0000'0000 +0.
	// 0b1000'0001 -exp2(1 - mantissabitcount - exponentbias)
	// 0b0000'0001 +exp2(1 - mantissabitcount - exponentbias)
	// ...
	// 0b1111'1111 -QNaN (maximum amount of ones)
	// 0b0111'1111 +QNaN (maximum amount of ones)
	// Determining the starting point depends of several factors here.
	static size_t constexpr offsetsstride{CHAR_BIT * sizeof(T) * 256 / 8 - (absolute && issigned) * (127 + isfloatingpoint)};// shrink the offsets size if possible
	size_t *t{offsets// either aim to cache low-to-high or high-to-low
		+ (issigned && !absolute) * (offsetsstride / 2 - reversesort)
		+ (reversesort && (!issigned || absolute)) * (offsetsstride - 1)
		- (isfloatingpoint && !issigned && absolute) * (reversesort * 2 - 1)};
	size_t offset;
	unsigned b;
	// handle the sign bit, virtually offset the top part by half the range here
	if constexpr(issigned){
		offset = *t;// the starting point was already adjusted to the signed range
		*t = 0;// the first offset always starts at zero
		t -= reversesort * 2 - 1;
		b = count < offset;// carry-out can only happen once per cycle here, so optimise that
		unsigned j{256 / 2 - 1};
		do{
			size_t difference{*t};
			*t = offset;
			t[offsetsstride + reversesort * 2 - 1] = offset - 1;// high half
			t -= reversesort * 2 - 1;
			offset += difference;
			addcarryofless(b, count, difference);
		}while(--j);
		size_t differencemid{t[256 * (reversesort * 2 - 1)]};
		t[256 * (reversesort * 2 - 1)] = offset;
		t[offsetsstride + reversesort * 2 - 1] = offset - 1;// high half
		t += (256 - 1) * (reversesort * 2 - 1);// offset to the start/end of the range
		offset += differencemid;
		addcarryofless(b, count, differencemid);
		j = 256 / 2 - 2;
		do{
			size_t difference{*t};
			*t = offset;
			t[offsetsstride + reversesort * 2 - 1] = offset - 1;// high half
			t -= reversesort * 2 - 1;
			offset -= difference * (reversesort * 2 - 1);
			addcarryofless(b, count, difference);
		}while(--j);
	}else{// unsigned
		offset = *t;
		*t = 0;// the first offset always starts at zero
		// custom loop for the special mode: absolute floating-point, but negative inputs will sort just below their positive counterparts
		if constexpr(isfloatingpoint && !issigned && absolute){// starts at one removed from the initial index
			t += reversesort * 2 - 1;// step back
			b = count < offset;// carry-out can only happen once per cycle here, so optimise that
			unsigned j{(256 - 2) / 2};// double the number of items per loop
			do{
				size_t difference{*t};// even
				*t = offset;
				t[offsetsstride - reversesort * 2 + 1] = offset - 1;// odd, high half
				offset += difference;
				addcarryofless(b, count, difference);
				difference = t[reversesort * -6 + 3];// odd
				t[reversesort * -6 + 3] = offset;
				t[offsetsstride] = offset - 1;// even, high half
				t += reversesort * -4 + 2;// step forward twice
				offset += difference;
				addcarryofless(b, count, difference);
			}while(--j);
		}else{// all other modes
			t -= reversesort * 2 - 1;
			b = count < offset;// carry-out can only happen once per cycle here, so optimise that
			unsigned j{256 - 2};
			do{
				size_t difference{*t};
				*t = offset;
				t[offsetsstride + reversesort * 2 - 1] = offset - 1;// high half
				t -= reversesort * 2 - 1;
				offset += difference;
				addcarryofless(b, count, difference);
			}while(--j);
		}
	}
	addcarryofless(b, count, *t);
	if(!b)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
		[[likely]]
#endif
	{
		*t = offset;
		t[offsetsstride] = count;// high half, the last offset always starts at the end
		// again, adjust for the special mode
		t[offsetsstride + ((isfloatingpoint && !issigned && absolute) != reversesort) * 2 - 1] = offset - 1;// high half
		return{true};
	}
	return{false};
}

// Function implementation templates for multi-part types

// radixsortcopynoalloc() function implementation template for multi-part types without indirection
template<bool reversesort, bool reverseorder, bool absolute, bool issigned, bool isfloatingpoint, typename T>
RSBD8_FUNC_NORMAL std::enable_if_t<
	std::is_unsigned_v<T> &&
	!std::is_same_v<bool, T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	void> radixsortcopynoallocmulti(size_t count, T const input[], T output[], T buffer[])noexcept{
	using U = std::conditional_t<sizeof(T) < sizeof(unsigned), unsigned, T>;// assume zero-extension to be basically free for U on basically all modern machines
	// do not pass a nullptr here, even though it's safe if count is 0
	assert(input);
	assert(output);
	assert(buffer);
	// All the code in this function is adapted for count to be one below its input value here.
	--count;
	if(0 < static_cast<ptrdiff_t>(count)){// a 0 or 1 count array is legal here
		static size_t constexpr offsetsstride{CHAR_BIT * sizeof(T) * 256 / 8 - (absolute && issigned) * (127 + isfloatingpoint)};// shrink the offsets size if possible
		size_t offsets[offsetsstride * 2];// a sizeable amount of indices, but it's worth it, and this function never calls functions either to further increase stack usage anyway
		std::memset(offsets, 0, sizeof(offsets) / 2);// only the low half

		// count the 256 configurations, all in one go
		ptrdiff_t i{static_cast<ptrdiff_t>(count)};
		if constexpr(64 == CHAR_BIT * sizeof(T)){
			if constexpr(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T const *pinput{input};
				do{
					U curhi{pinput[0]};
					U curlo{pinput[1]};
					pinput += 2;
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							curhi, output + i, buffer + i,
							curlo, output + i - 1, buffer + i - 1);
					}
					// register pressure performance issue on several platforms: first do the high half here
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					U curhi5{curhi >> (40 - log2ptrs)};
					U curhi6{curhi >> (48 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i] = static_cast<T>(curhi);
						buffer[i] = static_cast<T>(curhi);
					}
					curhi >>= 56;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					curhi6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curhi5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curhi6);
					++offsets[7 * 256 + static_cast<size_t>(curhi)];
					// register pressure performance issue on several platforms: do the low half here second
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					U curlo5{curlo >> (40 - log2ptrs)};
					U curlo6{curlo >> (48 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i - 1] = static_cast<T>(curlo);
						buffer[i - 1] = static_cast<T>(curlo);
					}
					curlo >>= 56;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					curlo6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curlo5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curlo6);
					++offsets[7 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					U cur{pinput[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, output, buffer);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					U cur5{cur >> (40 - log2ptrs)};
					U cur6{cur >> (48 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[0] = static_cast<T>(cur);
						buffer[0] = static_cast<T>(cur);
					}
					cur >>= 56;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					cur6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + cur5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + cur6);
					++offsets[7 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 64-bit, not in reverse order
				do{
					U curhi{input[i]};
					U curlo{input[i - 1]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							curhi, output + i,
							curlo, output + i - 1);
					}
					// register pressure performance issue on several platforms: first do the high half here
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					U curhi5{curhi >> (40 - log2ptrs)};
					U curhi6{curhi >> (48 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i] = static_cast<T>(curhi);
					curhi >>= 56;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					curhi6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curhi5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curhi6);
					++offsets[7 * 256 + static_cast<size_t>(curhi)];
					// register pressure performance issue on several platforms: do the low half here second
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					U curlo5{curlo >> (40 - log2ptrs)};
					U curlo6{curlo >> (48 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i - 1] = static_cast<T>(curlo);
					curlo >>= 56;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					curlo6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curlo5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curlo6);
					++offsets[7 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					U cur{input[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, output);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					U cur5{cur >> (40 - log2ptrs)};
					U cur6{cur >> (48 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[0] = static_cast<T>(cur);
					cur >>= 56;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					cur6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + cur5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + cur6);
					++offsets[7 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(56 == CHAR_BIT * sizeof(T)){
			if constexpr(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T const *pinput{input};
				do{
					U curhi{pinput[0]};
					U curlo{pinput[1]};
					pinput += 2;
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							curhi, output + i, buffer + i,
							curlo, output + i - 1, buffer + i - 1);
					}
					// register pressure performance issue on several platforms: first do the high half here
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					U curhi5{curhi >> (40 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i] = static_cast<T>(curhi);
						buffer[i] = static_cast<T>(curhi);
					}
					curhi >>= 48;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curhi5);
					++offsets[6 * 256 + static_cast<size_t>(curhi)];
					// register pressure performance issue on several platforms: do the low half here second
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					U curlo5{curlo >> (40 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i - 1] = static_cast<T>(curlo);
						buffer[i - 1] = static_cast<T>(curlo);
					}
					curlo >>= 48;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curlo5);
					++offsets[6 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					U cur{pinput[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, output, buffer);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					U cur5{cur >> (40 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[0] = static_cast<T>(cur);
						buffer[0] = static_cast<T>(cur);
					}
					cur >>= 48;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + cur5);
					++offsets[6 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 56-bit, not in reverse order
				do{
					U curhi{input[i]};
					U curlo{input[i - 1]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							curhi, output + i,
							curlo, output + i - 1);
					}
					// register pressure performance issue on several platforms: first do the high half here
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					U curhi5{curhi >> (40 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i] = static_cast<T>(curhi);
					curhi >>= 48;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curhi5);
					++offsets[6 * 256 + static_cast<size_t>(curhi)];
					// register pressure performance issue on several platforms: do the low half here second
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					U curlo5{curlo >> (40 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i - 1] = static_cast<T>(curlo);
					curlo >>= 48;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curlo5);
					++offsets[6 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					U cur{input[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, output);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					U cur5{cur >> (40 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[0] = static_cast<T>(cur);
					cur >>= 48;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + cur5);
					++offsets[6 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(48 == CHAR_BIT * sizeof(T)){
			if constexpr(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T const *pinput{input};
				do{
					U curhi{pinput[0]};
					U curlo{pinput[1]};
					pinput += 2;
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							curhi, output + i, buffer + i,
							curlo, output + i - 1, buffer + i - 1);
					}
					// register pressure performance issue on several platforms: first do the high half here
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i] = static_cast<T>(curhi);
						buffer[i] = static_cast<T>(curhi);
					}
					curhi >>= 40;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++offsets[5 * 256 + static_cast<size_t>(curhi)];
					// register pressure performance issue on several platforms: do the low half here second
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i - 1] = static_cast<T>(curlo);
						buffer[i - 1] = static_cast<T>(curlo);
					}
					curlo >>= 40;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++offsets[5 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					U cur{pinput[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, output, buffer);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[0] = static_cast<T>(cur);
						buffer[0] = static_cast<T>(cur);
					}
					cur >>= 40;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++offsets[5 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 48-bit, not in reverse order
				do{
					U curhi{input[i]};
					U curlo{input[i - 1]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							curhi, output + i,
							curlo, output + i - 1);
					}
					// register pressure performance issue on several platforms: first do the high half here
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i] = static_cast<T>(curhi);
					curhi >>= 40;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++offsets[5 * 256 + static_cast<size_t>(curhi)];
					// register pressure performance issue on several platforms: do the low half here second
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i - 1] = static_cast<T>(curlo);
					curlo >>= 40;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++offsets[5 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					U cur{input[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, output);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[0] = static_cast<T>(cur);
					cur >>= 40;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++offsets[5 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(40 == CHAR_BIT * sizeof(T)){
			if constexpr(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T const *pinput{input};
				do{
					U curhi{pinput[0]};
					U curlo{pinput[1]};
					pinput += 2;
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							curhi, output + i, buffer + i,
							curlo, output + i - 1, buffer + i - 1);
					}
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i] = static_cast<T>(curhi);
						buffer[i] = static_cast<T>(curhi);
					}
					curhi >>= 32;
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i - 1] = static_cast<T>(curlo);
						buffer[i - 1] = static_cast<T>(curlo);
					}
					curlo >>= 32;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++offsets[4 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++offsets[4 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					U cur{pinput[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, output, buffer);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[0] = static_cast<T>(cur);
						buffer[0] = static_cast<T>(cur);
					}
					cur >>= 32;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++offsets[4 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 40-bit, not in reverse order
				do{
					U curhi{input[i]};
					U curlo{input[i - 1]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							curhi, output + i,
							curlo, output + i - 1);
					}
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i] = static_cast<T>(curhi);
					curhi >>= 32;
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i - 1] = static_cast<T>(curlo);
					curlo >>= 32;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++offsets[4 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++offsets[4 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					U cur{input[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, output);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[0] = static_cast<T>(cur);
					cur >>= 32;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++offsets[4 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(32 == CHAR_BIT * sizeof(T)){
			if constexpr(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T const *pinput{input};
				do{
					U cura{pinput[0]};
					U curb{pinput[1]};
					pinput += 2;
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, output + i, buffer + i,
							curb, output + i - 1, buffer + i - 1);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					U cur2a{cura >> (16 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i] = static_cast<T>(cura);
						buffer[i] = static_cast<T>(cura);
					}
					cura >>= 24;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					U cur2b{curb >> (16 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i - 1] = static_cast<T>(curb);
						buffer[i - 1] = static_cast<T>(curb);
					}
					curb >>= 24;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					cur2a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					cur2b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2a);
					++offsets[3 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2b);
					++offsets[3 * 256 + static_cast<size_t>(curb)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					U cur{pinput[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, output, buffer);
					}
					U cur0{cur & 0xFFu};
					U cur1{static_cast<unsigned>(cur) >> (8 - log2ptrs)};
					U cur2{static_cast<unsigned>(cur) >> (16 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[0] = static_cast<T>(cur);
						buffer[0] = static_cast<T>(cur);
					}
					cur >>= 24;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++offsets[3 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 32-bit, not in reverse order
				do{
					U cura{input[i]};
					U curb{input[i - 1]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, output + i,
							curb, output + i - 1);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					U cur2a{cura >> (16 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i] = static_cast<T>(cura);
					cura >>= 24;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					U cur2b{curb >> (16 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i - 1] = static_cast<T>(curb);
					curb >>= 24;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					cur2a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					cur2b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2a);
					++offsets[3 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2b);
					++offsets[3 * 256 + static_cast<size_t>(curb)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					U cur{input[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, output);
					}
					U cur0{cur & 0xFFu};
					U cur1{static_cast<unsigned>(cur) >> (8 - log2ptrs)};
					U cur2{static_cast<unsigned>(cur) >> (16 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[0] = static_cast<T>(cur);
					cur >>= 24;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++offsets[3 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(24 == CHAR_BIT * sizeof(T)){
			if constexpr(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T const *pinput{input};
				i -= 2;
				if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					U cura{pinput[0]};
					U curb{pinput[1]};
					U curc{pinput[2]};
					pinput += 3;
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, output + i + 2, buffer + i + 2,
							curb, output + i + 1, buffer + i + 1,
							curc, output + i, buffer + i);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i + 2] = static_cast<T>(cura);
						buffer[i + 2] = static_cast<T>(cura);
					}
					cura >>= 16;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i + 1] = static_cast<T>(curb);
						buffer[i + 1] = static_cast<T>(curb);
					}
					curb >>= 16;
					U cur0c{curc & 0xFFu};
					U cur1c{curc >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i] = static_cast<T>(curc);
						buffer[i] = static_cast<T>(curc);
					}
					curc >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[cur0c];
					cur1c &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curc &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++offsets[2 * 256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1c);
					++offsets[2 * 256 + static_cast<size_t>(curc)];
					i -= 3;
				}while(0 <= i);
				if(2 & i){
					U cura{pinput[0]};
					U curb{pinput[1]};
					pinput += 2;
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, output + i + 2, buffer + i + 2,
							curb, output + i + 1, buffer + i + 1);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i + 2] = static_cast<T>(cura);
						buffer[i + 2] = static_cast<T>(cura);
					}
					cura >>= 16;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i + 1] = static_cast<T>(curb);
						buffer[i + 1] = static_cast<T>(curb);
					}
					curb >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++offsets[2 * 256 + static_cast<size_t>(curb)];
				}else if(1 & i){// possibly finalize 1 entry after the loop above
					U cur{pinput[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, output, buffer);
					}
					U cur0{cur & 0xFFu};
					U cur1{static_cast<unsigned>(cur) >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[0] = static_cast<T>(cur);
						buffer[0] = static_cast<T>(cur);
					}
					cur >>= 16;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++offsets[2 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 16-bit, not in reverse order
				i -= 3;
				if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					U cura{input[i + 2]};
					U curb{input[i + 1]};
					U curc{input[i]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, output + i + 2,
							curb, output + i + 1,
							curc, output + i);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i + 2] = static_cast<T>(cura);
					cura >>= 16;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i + 1] = static_cast<T>(curb);
					curb >>= 16;
					U cur0c{curc & 0xFFu};
					U cur1c{curc >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i] = static_cast<T>(curc);
					curc >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[cur0c];
					cur1c &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curc &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++offsets[2 * 256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1c);
					++offsets[2 * 256 + static_cast<size_t>(curc)];
					i -= 3;
				}while(0 <= i);
				if(2 & i){// possibly finalize 2 entries after the loop above
					U cura{input[i + 2]};
					U curb{input[i + 1]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, output + i + 2,
							curb, output + i + 1);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i + 2] = static_cast<T>(cura);
					cura >>= 16;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i + 1] = static_cast<T>(curb);
					curb >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++offsets[2 * 256 + static_cast<size_t>(curb)];
				}else if(1 & i){// possibly finalize 1 entry after the loop above
					U cur{input[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, output);
					}
					U cur0{cur & 0xFFu};
					U cur1{static_cast<unsigned>(cur) >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[0] = static_cast<T>(cur);
					cur >>= 16;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++offsets[2 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(16 == CHAR_BIT * sizeof(T)){
			if constexpr(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T const *pinput{input};
				i -= 3;
				if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					U cura{pinput[0]};
					U curb{pinput[1]};
					U curc{pinput[2]};
					U curd{pinput[3]};
					pinput += 4;
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, output + i + 3, buffer + i + 3,
							curb, output + i + 2, buffer + i + 2,
							curc, output + i + 1, buffer + i + 1,
							curd, output + i, buffer + i);
					}
					U cur0a{cura & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i + 3] = static_cast<T>(cura);
						buffer[i + 3] = static_cast<T>(cura);
					}
					cura >>= 8;
					U cur0b{curb & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i + 2] = static_cast<T>(curb);
						buffer[i + 2] = static_cast<T>(curb);
					}
					curb >>= 8;
					U cur0c{curc & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i + 1] = static_cast<T>(curc);
						buffer[i + 1] = static_cast<T>(curc);
					}
					curc >>= 8;
					U cur0d{curd & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i] = static_cast<T>(curd);
						buffer[i] = static_cast<T>(curd);
					}
					curd >>= 8;
					++offsets[cur0a];
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[cur0c];
					if constexpr(absolute && issigned && isfloatingpoint) curc &= 0x7Fu;
					++offsets[cur0d];
					if constexpr(absolute && issigned && isfloatingpoint) curd &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[256 + static_cast<size_t>(curc)];
					++offsets[256 + static_cast<size_t>(curd)];
					i -= 4;
				}while(0 <= i);
				if(2 & i){
					U cura{pinput[0]};
					U curb{pinput[1]};
					pinput += 2;
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, output + i + 3, buffer + i + 3,
							curb, output + i + 2, buffer + i + 2);
					}
					U cur0a{cura & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i + 3] = static_cast<T>(cura);
						buffer[i + 3] = static_cast<T>(cura);
					}
					cura >>= 8;
					U cur0b{curb & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[i + 2] = static_cast<T>(curb);
						buffer[i + 2] = static_cast<T>(curb);
					}
					curb >>= 8;
					++offsets[cur0a];
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[256 + static_cast<size_t>(curb)];
				}
				if(1 & i){// possibly finalize 1 entry after the loop above
					U cur{pinput[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, output, buffer);
					}
					U cur0{cur & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						output[0] = static_cast<T>(cur);
						buffer[0] = static_cast<T>(cur);
					}
					cur >>= 8;
					++offsets[cur0];
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cur)];
				}
			}else{// 16-bit, not in reverse order
				i -= 3;
				if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					U cura{input[i + 3]};
					U curb{input[i + 2]};
					U curc{input[i + 1]};
					U curd{input[i]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, output + i + 3,
							curb, output + i + 2,
							curc, output + i + 1,
							curd, output + i);
					}
					U cur0a{cura & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i + 3] = static_cast<T>(cura);
					cura >>= 8;
					U cur0b{curb & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i + 2] = static_cast<T>(curb);
					curb >>= 8;
					U cur0c{curc & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i + 1] = static_cast<T>(curc);
					curc >>= 8;
					U cur0d{curd & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i] = static_cast<T>(curd);
					curd >>= 8;
					++offsets[cur0a];
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[cur0c];
					if constexpr(absolute && issigned && isfloatingpoint) curc &= 0x7Fu;
					++offsets[cur0d];
					if constexpr(absolute && issigned && isfloatingpoint) curd &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[256 + static_cast<size_t>(curc)];
					++offsets[256 + static_cast<size_t>(curd)];
					i -= 4;
				}while(0 <= i);
				if(2 & i){// possibly finalize 2 entries after the loop above
					U cura{input[i + 3]};
					U curb{input[i + 2]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, output + i + 3,
							curb, output + i + 2);
					}
					U cur0a{cura & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i + 3] = static_cast<T>(cura);
					cura >>= 8;
					U cur0b{curb & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[i + 2] = static_cast<T>(curb);
					curb >>= 8;
					++offsets[cur0a];
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[256 + static_cast<size_t>(curb)];
				}
				if(1 & i){// possibly finalize 1 entry after the loop above
					U cur{input[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, output);
					}
					U cur0{cur & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) output[0] = static_cast<T>(cur);
					cur >>= 8;
					++offsets[cur0];
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cur)];
				}
			}
		}else static_assert(false, "Implementing larger types will require additional work and optimisation for this library.");

		// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
		auto[runsteps, paritybool]{generateoffsetsmulti<reversesort, absolute, issigned, isfloatingpoint, T>(count, offsets)};

		// perform the bidirectional 8-bit sorting sequence
		if(runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
			[[likely]]
#endif
		{
			T *pdst{buffer}, *pdstnext{output};// for the next iteration
			unsigned shifter{bitscanforwardportable(runsteps)};// at least 1 bit is set inside runsteps as by previous check
			T const *psrclo, *psrchi;
			if constexpr(true){// useless when not handling indirection: !reverseorder){
				psrclo = input;
				psrchi = input + count;
			}
			if(paritybool){// swap if the count of sorting actions to do is odd
				pdst = output;
				pdstnext = buffer;
			}
			// skip a step if possible
			runsteps >>= shifter;
			size_t *poffset{offsets + static_cast<size_t>(shifter) * 256};
			if constexpr(false){// useless when not handling indirection: reverseorder){
				psrclo = pdstnext;
				psrchi = pdstnext + count;
			}
			if constexpr(!absolute && isfloatingpoint) if(CHAR_BIT * sizeof(T) / 8 - 1 == shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
				[[unlikely]]
#endif
				goto handletop8;// rare, but possible
			shifter *= 8;
			for(;;){
				do{// fill the array, two at a time: one low-to-middle, one high-to-middle
					T outlo{*psrclo++};
					T outhi{*psrchi--};
					auto[curlo, curhi]{filtershift8<absolute, issigned, isfloatingpoint, T, U>(outlo, outhi, shifter)};
					size_t offsetlo{poffset[curlo]++};// the next item will be placed one higher
					size_t offsethi{poffset[curhi + offsetsstride]--};// the next item will be placed one lower
					pdst[offsetlo] = outlo;
					pdst[offsethi] = outhi;
				}while(psrclo < psrchi);
				if(psrclo == psrchi){// fill in the final item for odd counts
					T outlo{*psrclo};
					size_t curlo{filtershift8<absolute, issigned, isfloatingpoint, T, U>(outlo, shifter)};
					size_t offsetlo{poffset[curlo]};
					pdst[offsetlo] = outlo;
				}
				runsteps >>= 1;
				if(!runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
					break;
				{
#if defined(__has_cpp_attribute) && __has_cpp_attribute(maybe_unused)
					[[maybe_unused]]
#endif
					unsigned index;
					if constexpr(16 < CHAR_BIT * sizeof(T)) index = bitscanforwardportable(runsteps);// at least 1 bit is set inside runsteps as by previous check
					shifter += 8;
					poffset += 256;
					// swap the pointers for the next round, data is moved on each iteration
					psrclo = pdst;
					psrchi = pdst + count;
					pdst = pdstnext;
					pdstnext = const_cast<T *>(psrclo);// never written past the first iteration
					// skip a step if possible
					if constexpr(16 < CHAR_BIT * sizeof(T)){
						runsteps >>= index;
						shifter += index * 8;
						poffset += static_cast<size_t>(index) * 256;
					}
				}
				// handle the top part for floating-point differently
				if(!absolute && isfloatingpoint && CHAR_BIT * sizeof(T) - 8 == shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
				{
handletop8:// this prevents "!absolute && isfloatingpoint" to be made constexpr here, but that's fine
					do{// fill the array, two at a time: one low-to-middle, one high-to-middle
						T outlo{*psrclo++};
						T outhi{*psrchi--};
						auto[curlo, curhi]{filtertop8<absolute, issigned, isfloatingpoint, T, U>(outlo, outhi)};
						size_t offsetlo{offsets[curlo + (CHAR_BIT * sizeof(T) - 8) * 256 / 8]++};// the next item will be placed one higher
						size_t offsethi{offsets[curhi + (CHAR_BIT * sizeof(T) - 8) * 256 / 8 + offsetsstride]--};// the next item will be placed one lower
						pdst[offsetlo] = outlo;
						pdst[offsethi] = outhi;
					}while(psrclo < psrchi);
					if(psrclo == psrchi){// fill in the final item for odd counts
						T outlo{*psrclo};
						size_t curlo{filtertop8<absolute, issigned, isfloatingpoint, T, U>(outlo)};
						size_t offsetlo{offsets[curlo + (CHAR_BIT * sizeof(T) - 8) * 256 / 8]};
						pdst[offsetlo] = outlo;
					}
					break;// no further processing beyond the top part
				}
			}
		}
	}
}

// radixsortcopynoalloc() function implementation template for 80-bit-based long double types without indirection
// Platforms with a native 80-bit long double type are all little endian, hence that is the only implementation here.
template<bool reversesort, bool reverseorder, bool absolute, bool issigned, bool isfloatingpoint, typename T>
RSBD8_FUNC_NORMAL std::enable_if_t<
	(std::is_same_v<longdoubletest128, T> ||
	std::is_same_v<longdoubletest96, T> ||
	std::is_same_v<longdoubletest80, T> ||
	std::is_same_v<long double, T> &&
	64 == LDBL_MANT_DIG &&
	16384 == LDBL_MAX_EXP &&
	128 >= CHAR_BIT * sizeof(long double) &&
	64 < CHAR_BIT * sizeof(long double)),
	void> radixsortcopynoallocmulti(size_t count, T const input[], T output[], T buffer[])noexcept{
	using W = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t,
		std::conditional_t<96 == CHAR_BIT * sizeof(T), uint_least32_t,
		std::conditional_t<80 == CHAR_BIT * sizeof(T), uint_least16_t, void>>>;
	using U = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t, unsigned>;// assume zero-extension to be basically free for U on basically all modern machines
	// do not pass a nullptr here, even though it's safe if count is 0
	assert(input);
	assert(output);
	assert(buffer);
	// All the code in this function is adapted for count to be one below its input value here.
	--count;
	if(0 < static_cast<ptrdiff_t>(count)){// a 0 or 1 count array is legal here
		static size_t constexpr offsetsstride{80 * 256 / 8 - (absolute && issigned) * (127 + isfloatingpoint)};// shrink the offsets size if possible
		size_t offsets[offsetsstride * 2];// a sizeable amount of indices, but it's worth it, and this function never calls functions either to further increase stack usage anyway
		std::memset(offsets, 0, sizeof(offsets) / 2);// only the low half

		// count the 256 configurations, all in one go
		T *poutput{output};
		if constexpr(reverseorder && 80 < CHAR_BIT * sizeof(T)){// also reverse the array at the same time
			// reverse ordering is applied here because the padding bytes could matter, hence the check above
			T const *pinput{input + count};
			T *pbuffer{buffer};
			do{
				U cure{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(pinput) + 1)};
				uint_least64_t curm{*reinterpret_cast<uint_least64_t const *>(pinput)};
				--pinput;
				if constexpr(isfloatingpoint != absolute || absolute && !issigned){
					filterinput<absolute, issigned, isfloatingpoint, T>(curm, cure, poutput, pbuffer);
					++poutput;
					++pbuffer;
				}
				unsigned cure0{static_cast<unsigned>(cure & 0xFFu)};
				if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
					*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(poutput) + 1) = cure;
					*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(pbuffer) + 1) = cure;
				}
				cure >>= 8;
				unsigned curm0{static_cast<unsigned>(curm & 0xFFu)};
				unsigned curm1{static_cast<unsigned>(curm >> (8 - log2ptrs))};
				unsigned curm2{static_cast<unsigned>(curm >> (16 - log2ptrs))};
				unsigned curm3{static_cast<unsigned>(curm >> (24 - log2ptrs))};
				unsigned curm4{static_cast<unsigned>(curm >> (32 - log2ptrs))};
				unsigned curm5{static_cast<unsigned>(curm >> (40 - log2ptrs))};
				unsigned curm6{static_cast<unsigned>(curm >> (48 - log2ptrs))};
				if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
					*reinterpret_cast<uint_least64_t *>(poutput) = curm;
					++poutput;
					*reinterpret_cast<uint_least64_t *>(pbuffer) = curm;
					++pbuffer;
				}
				curm >>= 56;
				++offsets[8 * 256 + static_cast<size_t>(cure)];
				if constexpr(absolute && issigned && isfloatingpoint) cure &= 0x7Fu;
				++offsets[curm0];
				curm1 &= sizeof(void *) * 0xFFu;
				curm2 &= sizeof(void *) * 0xFFu;
				curm3 &= sizeof(void *) * 0xFFu;
				curm4 &= sizeof(void *) * 0xFFu;
				curm5 &= sizeof(void *) * 0xFFu;
				curm6 &= sizeof(void *) * 0xFFu;
				++offsets[7 * 256 + static_cast<size_t>(curm)];
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curm1);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curm2);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curm3);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curm4);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curm5);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curm6);
			}while(input < pinput);
		}else{// not in reverse order
			T const *pinput{input};
			ptrdiff_t i{static_cast<ptrdiff_t>(count)};
			do{
				U cure{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(pinput) + 1)};
				uint_least64_t curm{*reinterpret_cast<uint_least64_t const *>(pinput)};
				++pinput;
				if constexpr(isfloatingpoint != absolute || absolute && !issigned){
					filterinput<absolute, issigned, isfloatingpoint, T>(curm, cure, poutput);
					++poutput;
				}
				unsigned cure0{static_cast<unsigned>(cure & 0xFFu)};
				if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
					*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(poutput) + 1) = cure;
				}
				cure >>= 8;
				unsigned curm0{static_cast<unsigned>(curm & 0xFFu)};
				unsigned curm1{static_cast<unsigned>(curm >> (8 - log2ptrs))};
				unsigned curm2{static_cast<unsigned>(curm >> (16 - log2ptrs))};
				unsigned curm3{static_cast<unsigned>(curm >> (24 - log2ptrs))};
				unsigned curm4{static_cast<unsigned>(curm >> (32 - log2ptrs))};
				unsigned curm5{static_cast<unsigned>(curm >> (40 - log2ptrs))};
				unsigned curm6{static_cast<unsigned>(curm >> (48 - log2ptrs))};
				if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
					*reinterpret_cast<uint_least64_t *>(poutput) = curm;
					++poutput;
				}
				curm >>= 56;
				++offsets[8 * 256 + static_cast<size_t>(cure)];
				if constexpr(absolute && issigned && isfloatingpoint) cure &= 0x7Fu;
				++offsets[curm0];
				curm1 &= sizeof(void *) * 0xFFu;
				curm2 &= sizeof(void *) * 0xFFu;
				curm3 &= sizeof(void *) * 0xFFu;
				curm4 &= sizeof(void *) * 0xFFu;
				curm5 &= sizeof(void *) * 0xFFu;
				curm6 &= sizeof(void *) * 0xFFu;
				++offsets[7 * 256 + static_cast<size_t>(curm)];
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curm1);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curm2);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curm3);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curm4);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curm5);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curm6);
			}while(0 <= --i);
		}

		// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
		auto[runsteps, paritybool]{generateoffsetsmulti<reversesort, absolute, issigned, isfloatingpoint, T>(count, offsets)};

		// perform the bidirectional 8-bit sorting sequence
		if(runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
			[[likely]]
#endif
		{
			T *pdst{buffer}, *pdstnext{output};// for the next iteration
			unsigned shifter{bitscanforwardportable(runsteps)};// at least 1 bit is set inside runsteps as by previous check
			T const *psrclo, *psrchi;
			if constexpr(!reverseorder || 80 == CHAR_BIT * sizeof(T)){
				// no reverse ordering applied
				psrclo = input;
				psrchi = input + count;
			}
			if(paritybool){// swap if the count of sorting actions to do is odd
				pdst = output;
				pdstnext = buffer;
			}
			// skip a step if possible
			runsteps >>= shifter;
			size_t *poffset{offsets + static_cast<size_t>(shifter) * 256};
			if constexpr(reverseorder && 80 < CHAR_BIT * sizeof(T)){
				// reverse ordering is applied here because the padding bytes could matter, hence the check above
				psrclo = pdstnext;
				psrchi = pdstnext + count;
			}
			if(80 / 8 - 2 == shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
				[[unlikely]]
#endif
				goto handletop16;// rare, but possible
			if(80 / 8 - 2 < shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
				[[unlikely]]
#endif
				goto handletop8;// rare, but possible
			shifter *= 8;
			for(;;){
				do{// fill the array, two at a time: one low-to-middle, one high-to-middle
					U outloe{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(psrclo) + 1)};
					uint_least64_t outlom{*reinterpret_cast<uint_least64_t const *>(psrclo)};
					++psrclo;
					U outhie{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(psrchi) + 1)};
					uint_least64_t outhim{*reinterpret_cast<uint_least64_t const *>(psrchi)};
					--psrchi;
					auto[curlo, curhi]{filtershift8<absolute, issigned, isfloatingpoint, T, U>(outlom, outloe, outhim, outhie, shifter)};
					size_t offsetlo{poffset[curlo]++};// the next item will be placed one higher
					size_t offsethi{poffset[curhi + offsetsstride]--};// the next item will be placed one lower
					T *pwlo = pdst + offsetlo;
					T *pwhi = pdst + offsethi;
					*reinterpret_cast<W *>(*reinterpret_cast<uint_least64_t *>(pwlo) + 1) = static_cast<W>(outloe);
					*reinterpret_cast<uint_least64_t *>(pwlo) = outlom;
					*reinterpret_cast<W *>(*reinterpret_cast<uint_least64_t *>(pwhi) + 1) = static_cast<W>(outhie);
					*reinterpret_cast<uint_least64_t *>(pwhi) = outhim;
				}while(psrclo < psrchi);
				if(psrclo == psrchi){// fill in the final item for odd counts
					U outloe{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(psrclo) + 1)};
					uint_least64_t outlom{*reinterpret_cast<uint_least64_t const *>(psrclo)};
					size_t curlo{filtershift8<absolute, issigned, isfloatingpoint, T, U>(outlom, outloe, shifter)};
					size_t offsetlo{poffset[curlo]};
					T *pwlo = pdst + offsetlo;
					*reinterpret_cast<W *>(*reinterpret_cast<uint_least64_t *>(pwlo) + 1) = static_cast<W>(outloe);
					*reinterpret_cast<uint_least64_t *>(pwlo) = outlom;
				}
				runsteps >>= 1;
				if(!runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
					break;
				{
					unsigned index{bitscanforwardportable(runsteps)};// at least 1 bit is set inside runsteps as by previous check
					shifter += 8;
					poffset += 256;
					// swap the pointers for the next round, data is moved on each iteration
					psrclo = pdst;
					psrchi = pdst + count;
					pdst = pdstnext;
					pdstnext = const_cast<T *>(psrclo);// never written past the first iteration
					// skip a step if possible
					runsteps >>= index;
					shifter += index * 8;
					poffset += static_cast<size_t>(index) * 256;
				}
				// handle the top two parts differently
				if(80 - 16 <= shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
				{
					if(80 - 16 == shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
						[[likely]]
#endif
					{
handletop16:
						do{// fill the array, two at a time: one low-to-middle, one high-to-middle
							U outloe{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(psrclo) + 1)};
							uint_least64_t outlom{*reinterpret_cast<uint_least64_t const *>(psrclo)};
							++psrclo;
							U outhie{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(psrchi) + 1)};
							uint_least64_t outhim{*reinterpret_cast<uint_least64_t const *>(psrchi)};
							--psrchi;
							auto[curlo, curhi]{filterbelowtop8<absolute, issigned, isfloatingpoint, T, U>(outlom, outloe, outhim, outhie)};
							size_t offsetlo{offsets[curlo + (80 - 16) * 256 / 8]++};// the next item will be placed one higher
							size_t offsethi{offsets[curhi + (80 - 16) * 256 / 8 + offsetsstride]--};// the next item will be placed one lower
							T *pwlo = pdst + offsetlo;
							T *pwhi = pdst + offsethi;
							*reinterpret_cast<W *>(*reinterpret_cast<uint_least64_t *>(pwlo) + 1) = static_cast<W>(outloe);
							*reinterpret_cast<uint_least64_t *>(pwlo) = outlom;
							*reinterpret_cast<W *>(*reinterpret_cast<uint_least64_t *>(pwhi) + 1) = static_cast<W>(outhie);
							*reinterpret_cast<uint_least64_t *>(pwhi) = outhim;
						}while(psrclo < psrchi);
						if(psrclo == psrchi){// fill in the final item for odd counts
							U outloe{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(psrclo) + 1)};
							uint_least64_t outlom{*reinterpret_cast<uint_least64_t const *>(psrclo)};
							size_t curlo{filterbelowtop8<absolute, issigned, isfloatingpoint, T, U>(outlom, outloe)};
							size_t offsetlo{offsets[curlo + (80 - 16) * 256 / 8]};
							T *pwlo = pdst + offsetlo;
							*reinterpret_cast<W *>(*reinterpret_cast<uint_least64_t *>(pwlo) + 1) = static_cast<W>(outloe);
							*reinterpret_cast<uint_least64_t *>(pwlo) = outlom;
						}
						runsteps >>= 1;
						if(!runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
							[[unlikely]]
#endif
							break;
						// swap the pointers for the next round, data is moved on each iteration
						psrclo = pdst;
						psrchi = pdst + count;
						pdst = pdstnext;
						// unused: pdstnext = const_cast<T *>(psrclo);// never written past the first iteration
					}
handletop8:
					do{// fill the array, two at a time: one low-to-middle, one high-to-middle
						U outloe{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(psrclo) + 1)};
						uint_least64_t outlom{*reinterpret_cast<uint_least64_t const *>(psrclo)};
						++psrclo;
						U outhie{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(psrchi) + 1)};
						uint_least64_t outhim{*reinterpret_cast<uint_least64_t const *>(psrchi)};
						--psrchi;
						auto[curlo, curhi]{filtertop8<absolute, issigned, isfloatingpoint, uint_least16_t, U>(outloe, outhie)};
						size_t offsetlo{offsets[curlo + (80 - 8) * 256 / 8]++};// the next item will be placed one higher
						size_t offsethi{offsets[curhi + (80 - 8) * 256 / 8 + offsetsstride]--};// the next item will be placed one lower
						T *pwlo = pdst + offsetlo;
						T *pwhi = pdst + offsethi;
						*reinterpret_cast<W *>(*reinterpret_cast<uint_least64_t *>(pwlo) + 1) = static_cast<W>(outloe);
						*reinterpret_cast<uint_least64_t *>(pwlo) = outlom;
						*reinterpret_cast<W *>(*reinterpret_cast<uint_least64_t *>(pwhi) + 1) = static_cast<W>(outhie);
						*reinterpret_cast<uint_least64_t *>(pwhi) = outhim;
					}while(psrclo < psrchi);
					if(psrclo == psrchi){// fill in the final item for odd counts
						U outloe{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(psrclo) + 1)};
						uint_least64_t outlom{*reinterpret_cast<uint_least64_t const *>(psrclo)};
						size_t curlo{filtertop8<absolute, issigned, isfloatingpoint, uint_least16_t, U>(outloe)};
						size_t offsetlo{offsets[curlo + (80 - 8) * 256 / 8]};
						T *pwlo = pdst + offsetlo;
						*reinterpret_cast<W *>(*reinterpret_cast<uint_least64_t *>(pwlo) + 1) = static_cast<W>(outloe);
						*reinterpret_cast<uint_least64_t *>(pwlo) = outlom;
					}
					break;// no further processing beyond the top part
				}
			}
		}
	}
}

// radixsortnoalloc() function implementation template for multi-part types without indirection
template<bool reversesort, bool reverseorder, bool absolute, bool issigned, bool isfloatingpoint, typename T>
RSBD8_FUNC_NORMAL std::enable_if_t<
	std::is_unsigned_v<T> &&
	!std::is_same_v<bool, T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	void> radixsortnoallocmulti(size_t count, T input[], T buffer[], bool movetobuffer = false)noexcept{
	using U = std::conditional_t<sizeof(T) < sizeof(unsigned), unsigned, T>;// assume zero-extension to be basically free for U on basically all modern machines
	// do not pass a nullptr here, even though it's safe if count is 0
	assert(input);
	assert(buffer);
	// All the code in this function is adapted for count to be one below its input value here.
	--count;
	if(0 < static_cast<ptrdiff_t>(count)){// a 0 or 1 count array is legal here
		static size_t constexpr offsetsstride{CHAR_BIT * sizeof(T) * 256 / 8 - (absolute && issigned) * (127 + isfloatingpoint)};// shrink the offsets size if possible
		size_t offsets[offsetsstride * 2];// a sizeable amount of indices, but it's worth it, and this function never calls functions either to further increase stack usage anyway
		std::memset(offsets, 0, sizeof(offsets) / 2);// only the low half

		// count the 256 configurations, all in one go
		if constexpr(64 == CHAR_BIT * sizeof(T)){
			if constexpr(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T *pinputlo{input}, *pinputhi{input + count};
				T *pbufferlo{buffer}, *pbufferhi{buffer + count};
				do{
					U curlo{*pinputlo};
					U curhi{*pinputhi};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							curlo, pinputhi, pbufferhi,
							curhi, pinputlo, pbufferlo);
						--pinputhi;
						--pbufferhi;
						++pinputlo;
						++pbufferlo;
					}
					// register pressure performance issue on several platforms: first do the low half here
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					U curlo5{curlo >> (40 - log2ptrs)};
					U curlo6{curlo >> (48 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						*pinputhi-- = curlo;
						*pbufferhi-- = curlo;
					}
					curlo >>= 56;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					curlo6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curlo5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curlo6);
					++offsets[7 * 256 + static_cast<size_t>(curlo)];
					// register pressure performance issue on several platforms: do the high half here second
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					U curhi5{curhi >> (40 - log2ptrs)};
					U curhi6{curhi >> (48 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						*pinputlo++ = curhi;
						*pbufferlo++ = curhi;
					}
					curhi >>= 56;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					curhi6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curhi5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curhi6);
					++offsets[7 * 256 + static_cast<size_t>(curhi)];
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					U cur{*pinputlo};
					// no write to input, as this is the midpoint
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, pbufferhi);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					U cur5{cur >> (40 - log2ptrs)};
					U cur6{cur >> (48 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) *pbufferhi = cur;
					cur >>= 56;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					cur6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + cur5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + cur6);
					++offsets[7 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 64-bit, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count)};
				do{
					U curhi{input[i]};
					U curlo{input[i - 1]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							curhi, buffer + i,
							curlo, buffer + i - 1);
					}
					// register pressure performance issue on several platforms: first do the high half here
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					U curhi5{curhi >> (40 - log2ptrs)};
					U curhi6{curhi >> (48 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i] = static_cast<T>(curhi);
					curhi >>= 56;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					curhi6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curhi5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curhi6);
					++offsets[7 * 256 + static_cast<size_t>(curhi)];
					// register pressure performance issue on several platforms: do the low half here second
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					U curlo5{curlo >> (40 - log2ptrs)};
					U curlo6{curlo >> (48 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i - 1] = static_cast<T>(curlo);
					curlo >>= 56;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					curlo6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curlo5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curlo6);
					++offsets[7 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					U cur{input[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					U cur5{cur >> (40 - log2ptrs)};
					U cur6{cur >> (48 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[0] = static_cast<T>(cur);
					cur >>= 56;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					cur6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + cur5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + cur6);
					++offsets[7 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(56 == CHAR_BIT * sizeof(T)){
			if constexpr(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T *pinputlo{input}, *pinputhi{input + count};
				T *pbufferlo{buffer}, *pbufferhi{buffer + count};
				do{
					U curlo{*pinputlo};
					U curhi{*pinputhi};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							curlo, pinputhi, pbufferhi,
							curhi, pinputlo, pbufferlo);
						--pinputhi;
						--pbufferhi;
						++pinputlo;
						++pbufferlo;
					}
					// register pressure performance issue on several platforms: first do the low half here
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					U curlo5{curlo >> (40 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						*pinputhi-- = curlo;
						*pbufferhi-- = curlo;
					}
					curlo >>= 48;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curlo5);
					++offsets[6 * 256 + static_cast<size_t>(curlo)];
					// register pressure performance issue on several platforms: do the high half here second
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					U curhi5{curhi >> (40 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						*pinputlo++ = curhi;
						*pbufferlo++ = curhi;
					}
					curhi >>= 48;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curhi5);
					++offsets[6 * 256 + static_cast<size_t>(curhi)];
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					U cur{*pinputlo};
					// no write to input, as this is the midpoint
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, pbufferhi);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					U cur5{cur >> (40 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) *pbufferhi = cur;
					cur >>= 48;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + cur5);
					++offsets[6 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 56-bit, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count)};
				do{
					U curhi{input[i]};
					U curlo{input[i - 1]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							curhi, buffer + i,
							curlo, buffer + i - 1);
					}
					// register pressure performance issue on several platforms: first do the high half here
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					U curhi5{curhi >> (40 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i] = static_cast<T>(curhi);
					curhi >>= 48;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curhi5);
					++offsets[6 * 256 + static_cast<size_t>(curhi)];
					// register pressure performance issue on several platforms: do the low half here second
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					U curlo5{curlo >> (40 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i - 1] = static_cast<T>(curlo);
					curlo >>= 48;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curlo5);
					++offsets[6 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					U cur{input[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					U cur5{cur >> (40 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[0] = static_cast<T>(cur);
					cur >>= 48;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + cur5);
					++offsets[6 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(48 == CHAR_BIT * sizeof(T)){
			if constexpr(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T *pinputlo{input}, *pinputhi{input + count};
				T *pbufferlo{buffer}, *pbufferhi{buffer + count};
				do{
					U curlo{*pinputlo};
					U curhi{*pinputhi};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							curlo, pinputhi, pbufferhi,
							curhi, pinputlo, pbufferlo);
						--pinputhi;
						--pbufferhi;
						++pinputlo;
						++pbufferlo;
					}
					// register pressure performance issue on several platforms: first do the low half here
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						*pinputhi-- = curlo;
						*pbufferhi-- = curlo;
					}
					curlo >>= 40;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++offsets[5 * 256 + static_cast<size_t>(curlo)];
					// register pressure performance issue on several platforms: do the high half here second
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						*pinputlo++ = curhi;
						*pbufferlo++ = curhi;
					}
					curhi >>= 40;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++offsets[5 * 256 + static_cast<size_t>(curhi)];
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					U cur{*pinputlo};
					// no write to input, as this is the midpoint
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, pbufferhi);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) *pbufferhi = cur;
					cur >>= 40;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++offsets[5 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 48-bit, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count)};
				do{
					U curhi{input[i]};
					U curlo{input[i - 1]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							curhi, buffer + i,
							curlo, buffer + i - 1);
					}
					// register pressure performance issue on several platforms: first do the high half here
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i] = static_cast<T>(curhi);
					curhi >>= 40;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++offsets[5 * 256 + static_cast<size_t>(curhi)];
					// register pressure performance issue on several platforms: do the low half here second
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i - 1] = static_cast<T>(curlo);
					curlo >>= 40;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++offsets[5 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					U cur{input[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[0] = static_cast<T>(cur);
					cur >>= 40;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++offsets[5 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(40 == CHAR_BIT * sizeof(T)){
			if constexpr(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T *pinputlo{input}, *pinputhi{input + count};
				T *pbufferlo{buffer}, *pbufferhi{buffer + count};
				do{
					U curlo{*pinputlo};
					U curhi{*pinputhi};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							curlo, pinputhi, pbufferhi,
							curhi, pinputlo, pbufferlo);
						--pinputhi;
						--pbufferhi;
						++pinputlo;
						++pbufferlo;
					}
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						*pinputhi-- = curlo;
						*pbufferhi-- = curlo;
					}
					curlo >>= 32;
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						*pinputlo++ = curhi;
						*pbufferlo++ = curhi;
					}
					curhi >>= 32;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++offsets[4 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++offsets[4 * 256 + static_cast<size_t>(curhi)];
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					U cur{*pinputlo};
					// no write to input, as this is the midpoint
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, pbufferhi);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) *pbufferhi = cur;
					cur >>= 32;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++offsets[4 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 40-bit, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count)};
				do{
					U curhi{input[i]};
					U curlo{input[i - 1]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							curhi, buffer + i,
							curlo, buffer + i - 1);
					}
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i] = static_cast<T>(curhi);
					curhi >>= 32;
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i - 1] = static_cast<T>(curlo);
					curlo >>= 32;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++offsets[4 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++offsets[4 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					U cur{input[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[0] = static_cast<T>(cur);
					cur >>= 32;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++offsets[4 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(32 == CHAR_BIT * sizeof(T)){
			if constexpr(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T *pinputlo{input}, *pinputhi{input + count};
				T *pbufferlo{buffer}, *pbufferhi{buffer + count};
				do{
					U cura{*pinputlo};
					U curb{*pinputhi};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, pinputhi, pbufferhi,
							curb, pinputlo, pbufferlo);
						--pinputhi;
						--pbufferhi;
						++pinputlo;
						++pbufferlo;
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					U cur2a{cura >> (16 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						*pinputhi-- = cura;
						*pbufferhi-- = cura;
					}
					cura >>= 24;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					U cur2b{curb >> (16 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						*pinputlo++ = curb;
						*pbufferlo++ = curb;
					}
					curb >>= 24;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					cur2a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					cur2b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2a);
					++offsets[3 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2b);
					++offsets[3 * 256 + static_cast<size_t>(curb)];
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					U cur{*pinputlo};
					// no write to input, as this is the midpoint
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, pbufferhi);
					}
					U cur0{cur & 0xFFu};
					U cur1{static_cast<unsigned>(cur) >> (8 - log2ptrs)};
					U cur2{static_cast<unsigned>(cur) >> (16 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) *pbufferhi = cur;
					cur >>= 24;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++offsets[3 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 32-bit, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count)};
				do{
					U cura{input[i]};
					U curb{input[i - 1]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, buffer + i,
							curb, buffer + i - 1);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					U cur2a{cura >> (16 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i] = static_cast<T>(cura);
					cura >>= 24;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					U cur2b{curb >> (16 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i - 1] = static_cast<T>(curb);
					curb >>= 24;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					cur2a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					cur2b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2a);
					++offsets[3 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2b);
					++offsets[3 * 256 + static_cast<size_t>(curb)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					U cur{input[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					U cur0{cur & 0xFFu};
					U cur1{static_cast<unsigned>(cur) >> (8 - log2ptrs)};
					U cur2{static_cast<unsigned>(cur) >> (16 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[0] = static_cast<T>(cur);
					cur >>= 24;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++offsets[3 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(24 == CHAR_BIT * sizeof(T)){
			if constexpr(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T *pinputlo{input}, *pinputhi{input + count};
				T *pbufferlo{buffer}, *pbufferhi{buffer + count};
				size_t initialcount{(count + 1) % 6};
				if(4 & initialcount){// possibly initialize with 4 entries before the loop below
					U cura{pinputlo[0]};
					U curb{pinputhi[0]};
					U curc{pinputlo[1]};
					U curd{pinputhi[-1]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, pinputhi, pbufferhi,
							curb, pinputlo, pbufferlo,
							curc, pinputhi - 1, pbufferhi - 1,
							curd, pinputlo + 1, pbufferlo + 1);
						pinputhi -= 2;
						pbufferhi -= 2;
						pinputlo += 2;
						pbufferlo += 2;
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						pinputhi[0] = static_cast<T>(cura);
						pbufferhi[0] = static_cast<T>(cura);
					}
					cura >>= 16;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						pinputlo[0] = static_cast<T>(curb);
						pbufferlo[0] = static_cast<T>(curb);
					}
					curb >>= 16;
					U cur0c{curc & 0xFFu};
					U cur1c{curc >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						pinputhi[-1] = static_cast<T>(curc);
						pinputhi -= 2;
						pbufferhi[-1] = static_cast<T>(curc);
						pbufferhi -= 2;
					}
					curc >>= 16;
					U cur0d{curd & 0xFFu};
					U cur1d{curd >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						pinputlo[1] = static_cast<T>(curd);
						pinputlo += 2;
						pbufferlo[1] = static_cast<T>(curd);
						pbufferlo += 2;
					}
					curd >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[cur0c];
					cur1c &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curc &= 0x7Fu;
					++offsets[cur0d];
					cur1d &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curd &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++offsets[2 * 256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1c);
					++offsets[2 *256 + static_cast<size_t>(curc)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1d);
					++offsets[2 * 256 + static_cast<size_t>(curd)];
				}else if(2 & initialcount){// possibly initialize with 2 entries before the loop below
					U cura{*pinputlo};
					U curb{*pinputhi};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, pinputhi, pbufferhi,
							curb, pinputlo, pbufferlo);
						--pinputhi;
						--pbufferhi;
						++pinputlo;
						++pbufferlo;
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						*pinputhi-- = cura;
						*pbufferhi-- = cura;
					}
					cura >>= 16;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						*pinputlo++ = curb;
						*pbufferlo++ = curb;
					}
					curb >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++offsets[2 * 256 + static_cast<size_t>(curb)];
				}
				if(5 <= count)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					U cura{pinputlo[0]};
					U curb{pinputhi[0]};
					U curc{pinputlo[1]};
					U curd{pinputhi[-1]};
					// register pressure performance issue on several platforms: first do the high half here
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, pinputhi, pbufferhi,
							curb, pinputlo, pbufferlo,
							curc, pinputhi - 1, pbufferhi - 1);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						pinputhi[0] = static_cast<T>(cura);
						pbufferhi[0] = static_cast<T>(cura);
					}
					cura >>= 16;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						pinputlo[0] = static_cast<T>(curb);
						pbufferlo[0] = static_cast<T>(curb);
					}
					curb >>= 16;
					U cur0c{curc & 0xFFu};
					U cur1c{curc >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						pinputhi[-1] = static_cast<T>(curc);
						pbufferhi[-1] = static_cast<T>(curc);
					}
					curc >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[cur0c];
					cur1c &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curc &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++offsets[2 * 256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1c);
					++offsets[2 * 256 + static_cast<size_t>(curc)];
					U cure{pinputlo[2]};
					U curf{pinputhi[-2]};
					// register pressure performance issue on several platforms: do the low half here second
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							curd, pinputlo + 1, pbufferlo + 1,
							cure, pinputhi - 2, pbufferhi - 2,
							curf, pinputlo + 2, pbufferlo + 2);
						pinputhi -= 3;
						pbufferhi -= 3;
						pinputlo += 3;
						pbufferlo += 3;
					}
					U cur0d{curd & 0xFFu};
					U cur1d{curd >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						pinputlo[1] = static_cast<T>(curd);
						pbufferlo[1] = static_cast<T>(curd);
					}
					curd >>= 16;
					U cur0e{cure & 0xFFu};
					U cur1e{cure >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						pinputhi[-2] = static_cast<T>(cure);
						pinputhi -= 3;
						pbufferhi[-2] = static_cast<T>(cure);
						pbufferhi -= 3;
					}
					cure >>= 16;
					U cur0f{curf & 0xFFu};
					U cur1f{curf >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						pinputlo[2] = static_cast<T>(curf);
						pinputlo += 3;
						pbufferlo[2] = static_cast<T>(curf);
						pbufferlo += 3;
					}
					curf >>= 16;
					++offsets[cur0d];
					cur1d &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curd &= 0x7Fu;
					++offsets[cur0e];
					cur1e &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cure &= 0x7Fu;
					++offsets[cur0f];
					cur1f &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curf &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1d);
					++offsets[2 * 256 + static_cast<size_t>(curd)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1e);
					++offsets[2 * 256 + static_cast<size_t>(cure)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1f);
					++offsets[2 * 256 + static_cast<size_t>(curf)];
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					U cur{*pinputlo};
					// no write to input, as this is the midpoint
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, pbufferhi);
					}
					U cur0{cur & 0xFFu};
					U cur1{static_cast<unsigned>(cur) >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) *pbufferhi = cur;
					cur >>= 16;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++offsets[2 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 24-bit, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count) - 2};
				if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					U cura{input[i + 2]};
					U curb{input[i + 1]};
					U curc{input[i]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, buffer + i + 2,
							curb, buffer + i + 1,
							curc, buffer + i);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i + 3] = static_cast<T>(cura);
					cura >>= 16;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i + 2] = static_cast<T>(curb);
					curb >>= 16;
					U cur0c{curc & 0xFFu};
					U cur1c{curc >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i + 1] = static_cast<T>(curc);
					curc >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[cur0c];
					cur1c &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curc &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++offsets[2 * 256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1c);
					++offsets[2 * 256 + static_cast<size_t>(curc)];
					i -= 3;
				}while(0 <= i);
				if(2 & i){// possibly finalize 2 entries after the loop above
					U cura{input[i + 2]};
					U curb{input[i + 1]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, buffer + i + 2,
							curb, buffer + i + 1);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i + 2] = static_cast<T>(cura);
					cura >>= 16;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i + 1] = static_cast<T>(curb);
					curb >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++offsets[2 * 256 + static_cast<size_t>(curb)];
				}
				if(1 & i){// possibly finalize 1 entry after the loop above
					U cur{input[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					U cur0{cur & 0xFFu};
					U cur1{static_cast<unsigned>(cur) >> (8 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[0] = static_cast<T>(cur);
					cur >>= 16;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++offsets[2 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(16 == CHAR_BIT * sizeof(T)){
			if constexpr(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T *pinputlo{input}, *pinputhi{input + count};
				T *pbufferlo{buffer}, *pbufferhi{buffer + count};
				if(2 & count + 1){// possibly initialize with 2 entries before the loop below
					U cura{*pinputlo};
					U curb{*pinputhi};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, pinputhi, pbufferhi,
							curb, pinputlo, pbufferlo);
						--pinputhi;
						--pbufferhi;
						++pinputlo;
						++pbufferlo;
					}
					U cur0a{cura & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						*pinputhi-- = cura;
						*pbufferhi-- = cura;
					}
					cura >>= 8;
					U cur0b{curb & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						*pinputlo++ = curb;
						*pbufferlo++ = curb;
					}
					curb >>= 8;
					++offsets[cur0a];
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[256 + static_cast<size_t>(curb)];
				}
				if(3 <= count)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					U cura{pinputlo[0]};
					U curb{pinputhi[0]};
					U curc{pinputlo[1]};
					U curd{pinputhi[-1]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, pinputhi, pbufferhi,
							curb, pinputlo, pbufferlo,
							curc, pinputhi - 1, pbufferhi - 1,
							curd, pinputlo + 1, pbufferlo + 1);
						pinputhi -= 2;
						pbufferhi -= 2;
						pinputlo += 2;
						pbufferlo += 2;
					}
					U cur0a{cura & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						pinputhi[0] = static_cast<T>(cura);
						pbufferhi[0] = static_cast<T>(cura);
					}
					cura >>= 8;
					U cur0b{curb & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						pinputlo[0] = static_cast<T>(curb);
						pbufferlo[0] = static_cast<T>(curb);
					}
					curb >>= 8;
					U cur0c{curc & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						pinputhi[-1] = static_cast<T>(curc);
						pinputhi -= 2;
						pbufferhi[-1] = static_cast<T>(curc);
						pbufferhi -= 2;
					}
					curc >>= 8;
					U cur0d{curd & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
						pinputlo[1] = static_cast<T>(curd);
						pinputlo += 2;
						pbufferlo[1] = static_cast<T>(curd);
						pbufferlo += 2;
					}
					curd >>= 8;
					++offsets[cur0a];
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[cur0c];
					if constexpr(absolute && issigned && isfloatingpoint) curc &= 0x7Fu;
					++offsets[cur0d];
					if constexpr(absolute && issigned && isfloatingpoint) curd &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[256 + static_cast<size_t>(curc)];
					++offsets[256 + static_cast<size_t>(curd)];
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					U cur{*pinputlo};
					// no write to input, as this is the midpoint
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, pbufferhi);
					}
					U cur0{cur & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) *pbufferhi = cur;
					cur >>= 8;
					++offsets[cur0];
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cur)];
				}
			}else{// 16-bit, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count) - 3};
				if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					U cura{input[i + 3]};
					U curb{input[i + 2]};
					U curc{input[i + 1]};
					U curd{input[i]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, buffer + i + 3,
							curb, buffer + i + 2,
							curc, buffer + i + 1,
							curd, buffer + i);
					}
					U cur0a{cura & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i + 3] = static_cast<T>(cura);
					cura >>= 8;
					U cur0b{curb & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i + 2] = static_cast<T>(curb);
					curb >>= 8;
					U cur0c{curc & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i + 1] = static_cast<T>(curc);
					curc >>= 8;
					U cur0d{curd & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i] = static_cast<T>(curd);
					curd >>= 8;
					++offsets[cur0a];
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[cur0c];
					if constexpr(absolute && issigned && isfloatingpoint) curc &= 0x7Fu;
					++offsets[cur0d];
					if constexpr(absolute && issigned && isfloatingpoint) curd &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[256 + static_cast<size_t>(curc)];
					++offsets[256 + static_cast<size_t>(curd)];
					i -= 4;
				}while(0 <= i);
				if(2 & i){// possibly finalize 2 entries after the loop above
					U cura{input[i + 3]};
					U curb{input[i + 2]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(
							cura, buffer + i + 3,
							curb, buffer + i + 2);
					}
					U cur0a{cura & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i + 3] = static_cast<T>(cura);
					cura >>= 8;
					U cur0b{curb & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i + 2] = static_cast<T>(curb);
					curb >>= 8;
					++offsets[cur0a];
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[256 + static_cast<size_t>(curb)];
				}
				if(1 & i){// possibly finalize 1 entry after the loop above
					U cur{input[0]};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					U cur0{cur & 0xFFu};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[0] = static_cast<T>(cur);
					cur >>= 8;
					++offsets[cur0];
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cur)];
				}
			}
		}else static_assert(false, "Implementing larger types will require additional work and optimisation for this library.");

		// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
		auto[runsteps, paritybool]{generateoffsetsmulti<reversesort, absolute, issigned, isfloatingpoint, T>(count, offsets, movetobuffer)};

		// perform the bidirectional 8-bit sorting sequence
		if(runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
			[[likely]]
#endif
		{
			T *psrclo{input}, *pdst{buffer};
			unsigned shifter{bitscanforwardportable(runsteps)};// at least 1 bit is set inside runsteps as by previous check
			if(paritybool){// swap if the count of sorting actions to do is odd
				psrclo = buffer;
				pdst = input;
			}
			// skip a step if possible
			runsteps >>= shifter;
			size_t *poffset{offsets + static_cast<size_t>(shifter) * 256};
			T *psrchi{psrclo + count};
			T *pdstnext{psrclo};// for the next iteration
			if constexpr(!absolute && isfloatingpoint) if(CHAR_BIT * sizeof(T) / 8 - 1 == shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
				[[unlikely]]
#endif
				goto handletop8;// rare, but possible
			shifter *= 8;
			for(;;){
				do{// fill the array, two at a time: one low-to-middle, one high-to-middle
					T outlo{*psrclo++};
					T outhi{*psrchi--};
					auto[curlo, curhi]{filtershift8<absolute, issigned, isfloatingpoint, T, U>(outlo, outhi, shifter)};
					size_t offsetlo{poffset[curlo]++};// the next item will be placed one higher
					size_t offsethi{poffset[curhi + offsetsstride]--};// the next item will be placed one lower
					pdst[offsetlo] = outlo;
					pdst[offsethi] = outhi;
				}while(psrclo < psrchi);
				if(psrclo == psrchi){// fill in the final item for odd counts
					T outlo{*psrclo};
					size_t curlo{filtershift8<absolute, issigned, isfloatingpoint, T, U>(outlo, shifter)};
					size_t offsetlo{poffset[curlo]};
					pdst[offsetlo] = outlo;
				}
				runsteps >>= 1;
				if(!runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
					break;
				{
#if defined(__has_cpp_attribute) && __has_cpp_attribute(maybe_unused)
					[[maybe_unused]]
#endif
					unsigned index;
					if constexpr(16 < CHAR_BIT * sizeof(T)) index = bitscanforwardportable(runsteps);// at least 1 bit is set inside runsteps as by previous check
					shifter += 8;
					poffset += 256;
					// swap the pointers for the next round, data is moved on each iteration
					psrclo = pdst;
					psrchi = pdst + count;
					pdst = pdstnext;
					pdstnext = psrclo;
					// skip a step if possible
					if constexpr(16 < CHAR_BIT * sizeof(T)){
						runsteps >>= index;
						shifter += index * 8;
						poffset += static_cast<size_t>(index) * 256;
					}
				}
				// handle the top part for floating-point differently
				if(!absolute && isfloatingpoint && CHAR_BIT * sizeof(T) - 8 == shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
				{
handletop8:// this prevents "!absolute && isfloatingpoint" to be made constexpr here, but that's fine
					do{// fill the array, two at a time: one low-to-middle, one high-to-middle
						T outlo{*psrclo++};
						T outhi{*psrchi--};
						auto[curlo, curhi]{filtertop8<absolute, issigned, isfloatingpoint, T, U>(outlo, outhi)};
						size_t offsetlo{offsets[curlo + (CHAR_BIT * sizeof(T) - 8) * 256 / 8]++};// the next item will be placed one higher
						size_t offsethi{offsets[curhi + (CHAR_BIT * sizeof(T) - 8) * 256 / 8 + offsetsstride]--};// the next item will be placed one lower
						pdst[offsetlo] = outlo;
						pdst[offsethi] = outhi;
					}while(psrclo < psrchi);
					if(psrclo == psrchi){// fill in the final item for odd counts
						T outlo{*psrclo};
						size_t curlo{filtertop8<absolute, issigned, isfloatingpoint, T, U>(outlo)};
						size_t offsetlo{offsets[curlo + (CHAR_BIT * sizeof(T) - 8) * 256 / 8]};
						pdst[offsetlo] = outlo;
					}
					break;// no further processing beyond the top part
				}
			}
		}
	}
}

// radixsortnoalloc() function implementation template for 80-bit-based long double types without indirection
// Platforms with a native 80-bit long double type are all little endian, hence that is the only implementation here.
template<bool reversesort, bool reverseorder, bool absolute, bool issigned, bool isfloatingpoint, typename T>
RSBD8_FUNC_NORMAL std::enable_if_t<
	(std::is_same_v<longdoubletest128, T> ||
	std::is_same_v<longdoubletest96, T> ||
	std::is_same_v<longdoubletest80, T> ||
	std::is_same_v<long double, T> &&
	64 == LDBL_MANT_DIG &&
	16384 == LDBL_MAX_EXP &&
	128 >= CHAR_BIT * sizeof(long double) &&
	64 < CHAR_BIT * sizeof(long double)),
	void> radixsortnoallocmulti(size_t count, T input[], T buffer[], bool movetobuffer = false)noexcept{
	using W = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t,
		std::conditional_t<96 == CHAR_BIT * sizeof(T), uint_least32_t,
		std::conditional_t<80 == CHAR_BIT * sizeof(T), uint_least16_t, void>>>;
	using U = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t, unsigned>;// assume zero-extension to be basically free for U on basically all modern machines
	// do not pass a nullptr here, even though it's safe if count is 0
	assert(input);
	assert(buffer);
	// All the code in this function is adapted for count to be one below its input value here.
	--count;
	if(0 < static_cast<ptrdiff_t>(count)){// a 0 or 1 count array is legal here
		static size_t constexpr offsetsstride{CHAR_BIT * sizeof(T) * 256 / 8 - (absolute && issigned) * (127 + isfloatingpoint)};// shrink the offsets size if possible
		size_t offsets[offsetsstride * 2];// a sizeable amount of indices, but it's worth it, and this function never calls functions either to further increase stack usage anyway
		std::memset(offsets, 0, sizeof(offsets) / 2);// only the low half

		// count the 256 configurations, all in one go
		if constexpr(reverseorder && 80 < CHAR_BIT * sizeof(T)){// also reverse the array at the same time
			// reverse ordering is applied here because the padding bytes could matter, hence the check above
			T const *pinputlo{input}, *pinputhi{input + count};
			T *pbufferlo{buffer}, *pbufferhi{buffer + count};
			do{
				U curelo{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(pinputlo) + 1)};
				uint_least64_t curmlo{*reinterpret_cast<uint_least64_t const *>(pinputlo)};
				U curehi{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(pinputhi) + 1)};
				uint_least64_t curmhi{*reinterpret_cast<uint_least64_t const *>(pinputhi)};
				if constexpr(isfloatingpoint != absolute || absolute && !issigned){
					filterinput<absolute, issigned, isfloatingpoint, T>(
						curelo, curmlo, pinputhi, pbufferhi,
						curehi, curmhi, pinputlo, pbufferlo);
					--pinputhi;
					--pbufferhi;
					++pinputlo;
					++pbufferlo;
				}
				// register pressure performance issue on several platforms: first do the low half here
				unsigned curelo0{static_cast<unsigned>(curelo & 0xFFu)};
				if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
					*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(pinputhi) + 1) = curelo;
					*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(pbufferhi) + 1) = curelo;
				}
				curelo >>= 8;
				unsigned curmlo0{static_cast<unsigned>(curmlo & 0xFFu)};
				unsigned curmlo1{static_cast<unsigned>(curmlo >> (8 - log2ptrs))};
				unsigned curmlo2{static_cast<unsigned>(curmlo >> (16 - log2ptrs))};
				unsigned curmlo3{static_cast<unsigned>(curmlo >> (24 - log2ptrs))};
				unsigned curmlo4{static_cast<unsigned>(curmlo >> (32 - log2ptrs))};
				unsigned curmlo5{static_cast<unsigned>(curmlo >> (40 - log2ptrs))};
				unsigned curmlo6{static_cast<unsigned>(curmlo >> (48 - log2ptrs))};
				if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
					*reinterpret_cast<uint_least64_t *>(pinputhi) = curmlo;
					--pinputhi;
					*reinterpret_cast<uint_least64_t *>(pbufferhi) = curmlo;
					--pbufferhi;
				}
				curmlo >>= 56;
				++offsets[8 * 256 + static_cast<size_t>(curelo)];
				if constexpr(absolute && issigned && isfloatingpoint) curelo &= 0x7Fu;
				++offsets[curmlo0];
				curmlo1 &= sizeof(void *) * 0xFFu;
				curmlo2 &= sizeof(void *) * 0xFFu;
				curmlo3 &= sizeof(void *) * 0xFFu;
				curmlo4 &= sizeof(void *) * 0xFFu;
				curmlo5 &= sizeof(void *) * 0xFFu;
				curmlo6 &= sizeof(void *) * 0xFFu;
				++offsets[7 * 256 + static_cast<size_t>(curmlo)];
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curmlo1);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curmlo2);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curmlo3);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curmlo4);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curmlo5);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curmlo6);
				// register pressure performance issue on several platforms: do the high half here second
				unsigned curehi0{static_cast<unsigned>(curehi & 0xFFu)};
				if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
					*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(pinputlo) + 1) = curehi;
					*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(pbufferlo) + 1) = curehi;
				}
				curehi >>= 8;
				unsigned curmhi0{static_cast<unsigned>(curmhi & 0xFFu)};
				unsigned curmhi1{static_cast<unsigned>(curmhi >> (8 - log2ptrs))};
				unsigned curmhi2{static_cast<unsigned>(curmhi >> (16 - log2ptrs))};
				unsigned curmhi3{static_cast<unsigned>(curmhi >> (24 - log2ptrs))};
				unsigned curmhi4{static_cast<unsigned>(curmhi >> (32 - log2ptrs))};
				unsigned curmhi5{static_cast<unsigned>(curmhi >> (40 - log2ptrs))};
				unsigned curmhi6{static_cast<unsigned>(curmhi >> (48 - log2ptrs))};
				if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
					*reinterpret_cast<uint_least64_t *>(pinputlo) = curmhi;
					++pinputlo;
					*reinterpret_cast<uint_least64_t *>(pbufferlo) = curmhi;
					++pbufferlo;
				}
				curmhi >>= 56;
				++offsets[8 * 256 + static_cast<size_t>(curehi)];
				if constexpr(absolute && issigned && isfloatingpoint) curehi &= 0x7Fu;
				++offsets[curmhi0];
				curmhi1 &= sizeof(void *) * 0xFFu;
				curmhi2 &= sizeof(void *) * 0xFFu;
				curmhi3 &= sizeof(void *) * 0xFFu;
				curmhi4 &= sizeof(void *) * 0xFFu;
				curmhi5 &= sizeof(void *) * 0xFFu;
				curmhi6 &= sizeof(void *) * 0xFFu;
				++offsets[7 * 256 + static_cast<size_t>(curmhi)];
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curmhi1);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curmhi2);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curmhi3);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curmhi4);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curmhi5);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curmhi6);
			}while(pinputlo < pinputhi);
			if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
				U cure{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(pinputlo) + 1)};
				uint_least64_t curm{*reinterpret_cast<uint_least64_t const *>(pinputlo)};
				// no write to input, as this is the midpoint
				if constexpr(isfloatingpoint != absolute || absolute && !issigned){
					filterinput<absolute, issigned, isfloatingpoint, T>(cure, curm, pbufferhi);
				}
				unsigned cure0{static_cast<unsigned>(cure & 0xFFu)};
				if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
					*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(pbufferhi) + 1) = cure;
				}
				cure >>= 8;
				unsigned curm0{static_cast<unsigned>(curm & 0xFFu)};
				unsigned curm1{static_cast<unsigned>(curm >> (8 - log2ptrs))};
				unsigned curm2{static_cast<unsigned>(curm >> (16 - log2ptrs))};
				unsigned curm3{static_cast<unsigned>(curm >> (24 - log2ptrs))};
				unsigned curm4{static_cast<unsigned>(curm >> (32 - log2ptrs))};
				unsigned curm5{static_cast<unsigned>(curm >> (40 - log2ptrs))};
				unsigned curm6{static_cast<unsigned>(curm >> (48 - log2ptrs))};
				if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
					*reinterpret_cast<uint_least64_t *>(pbufferhi) = curm;
				}
				curm >>= 56;
				++offsets[8 * 256 + static_cast<size_t>(cure)];
				if constexpr(absolute && issigned && isfloatingpoint) cure &= 0x7Fu;
				++offsets[curm0];
				curm1 &= sizeof(void *) * 0xFFu;
				curm2 &= sizeof(void *) * 0xFFu;
				curm3 &= sizeof(void *) * 0xFFu;
				curm4 &= sizeof(void *) * 0xFFu;
				curm5 &= sizeof(void *) * 0xFFu;
				curm6 &= sizeof(void *) * 0xFFu;
				++offsets[7 * 256 + static_cast<size_t>(curm)];
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curm1);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curm2);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curm3);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curm4);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curm5);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curm6);
			}
		}else{// not in reverse order
			T *pinput{input};
			T *pbuffer{buffer};
			ptrdiff_t i{static_cast<ptrdiff_t>(count)};
			do{
				U cure{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(pinput) + 1)};
				uint_least64_t curm{*reinterpret_cast<uint_least64_t const *>(pinput)};
				++pinput;
				if constexpr(isfloatingpoint != absolute || absolute && !issigned){
					filterinput<absolute, issigned, isfloatingpoint, T>(curm, cure, pbuffer);
					++pbuffer;
				}
				unsigned cure0{static_cast<unsigned>(cure & 0xFFu)};
				if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
					*reinterpret_cast<W *>(reinterpret_cast<uint_least64_t *>(pbuffer) + 1) = cure;
				}
				cure >>= 8;
				unsigned curm0{static_cast<unsigned>(curm & 0xFFu)};
				unsigned curm1{static_cast<unsigned>(curm >> (8 - log2ptrs))};
				unsigned curm2{static_cast<unsigned>(curm >> (16 - log2ptrs))};
				unsigned curm3{static_cast<unsigned>(curm >> (24 - log2ptrs))};
				unsigned curm4{static_cast<unsigned>(curm >> (32 - log2ptrs))};
				unsigned curm5{static_cast<unsigned>(curm >> (40 - log2ptrs))};
				unsigned curm6{static_cast<unsigned>(curm >> (48 - log2ptrs))};
				if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)){
					*reinterpret_cast<uint_least64_t *>(pbuffer) = curm;
					++pbuffer;
				}
				curm >>= 56;
				++offsets[8 * 256 + static_cast<size_t>(cure)];
				if constexpr(absolute && issigned && isfloatingpoint) cure &= 0x7Fu;
				++offsets[curm0];
				curm1 &= sizeof(void *) * 0xFFu;
				curm2 &= sizeof(void *) * 0xFFu;
				curm3 &= sizeof(void *) * 0xFFu;
				curm4 &= sizeof(void *) * 0xFFu;
				curm5 &= sizeof(void *) * 0xFFu;
				curm6 &= sizeof(void *) * 0xFFu;
				++offsets[7 * 256 + static_cast<size_t>(curm)];
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curm1);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curm2);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curm3);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curm4);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curm5);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curm6);
			}while(0 <= --i);
		}

		// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
		auto[runsteps, paritybool]{generateoffsetsmulti<reversesort, absolute, issigned, isfloatingpoint, T>(count, offsets, movetobuffer)};

		// perform the bidirectional 8-bit sorting sequence
		if(runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
			[[likely]]
#endif
		{
			T *psrclo{input}, *pdst{buffer};
			unsigned shifter{bitscanforwardportable(runsteps)};// at least 1 bit is set inside runsteps as by previous check
			if(paritybool){// swap if the count of sorting actions to do is odd
				psrclo = buffer;
				pdst = input;
			}
			// skip a step if possible
			runsteps >>= shifter;
			size_t *poffset{offsets + static_cast<size_t>(shifter) * 256};
			T *psrchi{psrclo + count};
			T *pdstnext{psrclo};// for the next iteration
			if(80 / 8 - 2 == shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
				[[unlikely]]
#endif
				goto handletop16;// rare, but possible
			if(80 / 8 - 2 < shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
				[[unlikely]]
#endif
				goto handletop8;// rare, but possible
			shifter *= 8;
			for(;;){
				do{// fill the array, two at a time: one low-to-middle, one high-to-middle
					U outloe{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(psrclo) + 1)};
					uint_least64_t outlom{*reinterpret_cast<uint_least64_t const *>(psrclo)};
					++psrclo;
					U outhie{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(psrchi) + 1)};
					uint_least64_t outhim{*reinterpret_cast<uint_least64_t const *>(psrchi)};
					--psrchi;
					auto[curlo, curhi]{filtershift8<absolute, issigned, isfloatingpoint, T, U>(outlom, outloe, outhim, outhie, shifter)};
					size_t offsetlo{poffset[curlo]++};// the next item will be placed one higher
					size_t offsethi{poffset[curhi + offsetsstride]--};// the next item will be placed one lower
					T *pwlo = pdst + offsetlo;
					T *pwhi = pdst + offsethi;
					*reinterpret_cast<W *>(*reinterpret_cast<uint_least64_t *>(pwlo) + 1) = static_cast<W>(outloe);
					*reinterpret_cast<uint_least64_t *>(pwlo) = outlom;
					*reinterpret_cast<W *>(*reinterpret_cast<uint_least64_t *>(pwhi) + 1) = static_cast<W>(outhie);
					*reinterpret_cast<uint_least64_t *>(pwhi) = outhim;
				}while(psrclo < psrchi);
				if(psrclo == psrchi){// fill in the final item for odd counts
					U outloe{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(psrclo) + 1)};
					uint_least64_t outlom{*reinterpret_cast<uint_least64_t const *>(psrclo)};
					size_t curlo{filtershift8<absolute, issigned, isfloatingpoint, T, U>(outlom, outloe, shifter)};
					size_t offsetlo{poffset[curlo]};
					T *pwlo = pdst + offsetlo;
					*reinterpret_cast<W *>(*reinterpret_cast<uint_least64_t *>(pwlo) + 1) = static_cast<W>(outloe);
					*reinterpret_cast<uint_least64_t *>(pwlo) = outlom;
				}
				runsteps >>= 1;
				if(!runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
					break;
				{
					unsigned index{bitscanforwardportable(runsteps)};// at least 1 bit is set inside runsteps as by previous check
					shifter += 8;
					poffset += 256;
					// swap the pointers for the next round, data is moved on each iteration
					psrclo = pdst;
					psrchi = pdst + count;
					pdst = pdstnext;
					pdstnext = psrclo;
					// skip a step if possible
					runsteps >>= index;
					shifter += index * 8;
					poffset += static_cast<size_t>(index) * 256;
				}
				// handle the top two parts differently
				if(80 - 16 <= shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
				{
					if(80 - 16 == shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
						[[likely]]
#endif
					{
handletop16:
						do{// fill the array, two at a time: one low-to-middle, one high-to-middle
							U outloe{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(psrclo) + 1)};
							uint_least64_t outlom{*reinterpret_cast<uint_least64_t const *>(psrclo)};
							++psrclo;
							U outhie{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(psrchi) + 1)};
							uint_least64_t outhim{*reinterpret_cast<uint_least64_t const *>(psrchi)};
							--psrchi;
							auto[curlo, curhi]{filterbelowtop8<absolute, issigned, isfloatingpoint, T, U>(outlom, outloe, outhim, outhie)};
							size_t offsetlo{offsets[curlo + (80 - 16) * 256 / 8]++};// the next item will be placed one higher
							size_t offsethi{offsets[curhi + (80 - 16) * 256 / 8 + offsetsstride]--};// the next item will be placed one lower
							T *pwlo = pdst + offsetlo;
							T *pwhi = pdst + offsethi;
							*reinterpret_cast<W *>(*reinterpret_cast<uint_least64_t *>(pwlo) + 1) = static_cast<W>(outloe);
							*reinterpret_cast<uint_least64_t *>(pwlo) = outlom;
							*reinterpret_cast<W *>(*reinterpret_cast<uint_least64_t *>(pwhi) + 1) = static_cast<W>(outhie);
							*reinterpret_cast<uint_least64_t *>(pwhi) = outhim;
						}while(psrclo < psrchi);
						if(psrclo == psrchi){// fill in the final item for odd counts
							U outloe{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(psrclo) + 1)};
							uint_least64_t outlom{*reinterpret_cast<uint_least64_t const *>(psrclo)};
							size_t curlo{filterbelowtop8<absolute, issigned, isfloatingpoint, T, U>(outlom, outloe)};
							size_t offsetlo{offsets[curlo + (80 - 16) * 256 / 8]};
							T *pwlo = pdst + offsetlo;
							*reinterpret_cast<W *>(*reinterpret_cast<uint_least64_t *>(pwlo) + 1) = static_cast<W>(outloe);
							*reinterpret_cast<uint_least64_t *>(pwlo) = outlom;
						}
						runsteps >>= 1;
						if(!runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
							[[unlikely]]
#endif
							break;
						// swap the pointers for the next round, data is moved on each iteration
						psrclo = pdst;
						psrchi = pdst + count;
						pdst = pdstnext;
						// unused: pdstnext = psrclo;
					}
handletop8:
					do{// fill the array, two at a time: one low-to-middle, one high-to-middle
						U outloe{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(psrclo) + 1)};
						uint_least64_t outlom{*reinterpret_cast<uint_least64_t const *>(psrclo)};
						++psrclo;
						U outhie{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(psrchi) + 1)};
						uint_least64_t outhim{*reinterpret_cast<uint_least64_t const *>(psrchi)};
						--psrchi;
						auto[curlo, curhi]{filtertop8<absolute, issigned, isfloatingpoint, uint_least16_t, U>(outloe, outhie)};
						size_t offsetlo{offsets[curlo + (80 - 8) * 256 / 8]++};// the next item will be placed one higher
						size_t offsethi{offsets[curhi + (80 - 8) * 256 / 8 + offsetsstride]--};// the next item will be placed one lower
						T *pwlo = pdst + offsetlo;
						T *pwhi = pdst + offsethi;
						*reinterpret_cast<W *>(*reinterpret_cast<uint_least64_t *>(pwlo) + 1) = static_cast<W>(outloe);
						*reinterpret_cast<uint_least64_t *>(pwlo) = outlom;
						*reinterpret_cast<W *>(*reinterpret_cast<uint_least64_t *>(pwhi) + 1) = static_cast<W>(outhie);
						*reinterpret_cast<uint_least64_t *>(pwhi) = outhim;
					}while(psrclo < psrchi);
					if(psrclo == psrchi){// fill in the final item for odd counts
						U outloe{*reinterpret_cast<W const *>(reinterpret_cast<uint_least64_t const *>(psrclo) + 1)};
						uint_least64_t outlom{*reinterpret_cast<uint_least64_t const *>(psrclo)};
						size_t curlo{filtertop8<absolute, issigned, isfloatingpoint, uint_least16_t, U>(outloe)};
						size_t offsetlo{offsets[curlo + (80 - 8) * 256 / 8]};
						T *pwlo = pdst + offsetlo;
						*reinterpret_cast<W *>(*reinterpret_cast<uint_least64_t *>(pwlo) + 1) = static_cast<W>(outloe);
						*reinterpret_cast<uint_least64_t *>(pwlo) = outlom;
					}
					break;// no further processing beyond the top part
				}
			}
		}
	}
}

// radixsortcopynoalloc() function implementation template for multi-part types with indirection
template<auto indirection1, bool reversesort, bool reverseorder, bool absolute, bool issigned, bool isfloatingpoint, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
RSBD8_FUNC_NORMAL std::enable_if_t<
	(std::is_member_function_pointer_v<decltype(indirection1)> ||
	std::is_member_object_pointer_v<decltype(indirection1)> &&
	std::is_unsigned_v<std::remove_pointer_t<std::remove_cvref_t<memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>>) &&
	64 >= CHAR_BIT * sizeof(stripenum<std::remove_pointer_t<std::remove_cvref_t<memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>>) &&
	8 < CHAR_BIT * sizeof(stripenum<std::remove_pointer_t<std::remove_cvref_t<memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>>),
	void> radixsortcopynoallocmulti(size_t count, V *const input[], V *output[], V *buffer[], vararguments... varparameters)noexcept(
	std::is_member_object_pointer_v<decltype(indirection1)> ||
	std::is_nothrow_invocable_v<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>){
	using T = tounifunsigned<std::remove_pointer_t<std::remove_cvref_t<memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>>;
	using U = std::conditional_t<sizeof(T) < sizeof(unsigned), unsigned, T>;// assume zero-extension to be basically free for U on basically all modern machines
	static T constexpr highbit{generatehighbit<T>()};
	static bool constexpr isaddressingsubdivisable{
		!(absolute && !issigned) &&// the two tiered absolute types shift bits out of place during filering
#ifdef RSBD8_DISABLE_ADDRESS_SUBDIVISION
		false
#else
		8 == CHAR_BIT &&// optimisation for multi-part addressable machines only
		(std::is_member_object_pointer_v<decltype(indirection1)> ||
		std::is_pointer_v<std::invoke_result_t<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>>)
#endif
	};
	// do not pass a nullptr here, even though it's safe if count is 0
	assert(input);
	assert(output);
	assert(buffer);
	// All the code in this function is adapted for count to be one below its input value here.
	--count;
	if(0 < static_cast<ptrdiff_t>(count)){// a 0 or 1 count array is legal here
		static size_t constexpr offsetsstride{CHAR_BIT * sizeof(T) * 256 / 8 - (absolute && issigned) * (127 + isfloatingpoint)};// shrink the offsets size if possible
		size_t offsets[offsetsstride * 2];// a sizeable amount of indices, but it's worth it, and this function never calls functions either to further increase stack usage anyway
		std::memset(offsets, 0, sizeof(offsets) / 2);// only the low half

		// count the 256 configurations, all in one go
		ptrdiff_t i{static_cast<ptrdiff_t>(count)};
		if constexpr(64 == CHAR_BIT * sizeof(T)){
			if constexpr(reverseorder){// also reverse the array at the same time
				V *const *pinput{input};
				do{
					V *phi{pinput[0]};
					V *plo{pinput[1]};
					pinput += 2;
					output[i] = phi;
					buffer[i] = phi;
					auto imhi{indirectinput1<indirection1, isindexed2, T, V>(phi, varparameters...)};
					output[i - 1] = plo;
					buffer[i - 1] = plo;
					auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
					U curhi{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
					U curlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					// register pressure performance issue on several platforms: first do the high half here
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					U curhi5{curhi >> (40 - log2ptrs)};
					U curhi6{curhi >> (48 - log2ptrs)};
					curhi >>= 56;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					curhi6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curhi5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curhi6);
					++offsets[7 * 256 + static_cast<size_t>(curhi)];
					// register pressure performance issue on several platforms: do the low half here second
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					U curlo5{curlo >> (40 - log2ptrs)};
					U curlo6{curlo >> (48 - log2ptrs)};
					curlo >>= 56;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					curlo6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curlo5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curlo6);
					++offsets[7 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{pinput[0]};
					output[i] = p;
					buffer[i] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					U cur5{cur >> (40 - log2ptrs)};
					U cur6{cur >> (48 - log2ptrs)};
					cur >>= 56;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					cur6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + cur5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + cur6);
					++offsets[7 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 64-bit, not in reverse order
				do{
					V *phi{input[i]};
					V *plo{input[i - 1]};
					output[i] = phi;
					auto imhi{indirectinput1<indirection1, isindexed2, T, V>(phi, varparameters...)};
					output[i - 1] = plo;
					auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
					U curhi{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
					U curlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					// register pressure performance issue on several platforms: first do the high half here
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					U curhi5{curhi >> (40 - log2ptrs)};
					U curhi6{curhi >> (48 - log2ptrs)};
					curhi >>= 56;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					curhi6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curhi5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curhi6);
					++offsets[7 * 256 + static_cast<size_t>(curhi)];
					// register pressure performance issue on several platforms: do the low half here second
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					U curlo5{curlo >> (40 - log2ptrs)};
					U curlo6{curlo >> (48 - log2ptrs)};
					curlo >>= 56;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					curlo6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curlo5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curlo6);
					++offsets[7 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					output[0] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					U cur5{cur >> (40 - log2ptrs)};
					U cur6{cur >> (48 - log2ptrs)};
					cur >>= 56;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					cur6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + cur5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + cur6);
					++offsets[7 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(56 == CHAR_BIT * sizeof(T)){
			if constexpr(reverseorder){// also reverse the array at the same time
				V *const *pinput{input};
				do{
					V *phi{pinput[0]};
					V *plo{pinput[1]};
					pinput += 2;
					output[i] = phi;
					buffer[i] = phi;
					auto imhi{indirectinput1<indirection1, isindexed2, T, V>(phi, varparameters...)};
					output[i - 1] = plo;
					buffer[i - 1] = plo;
					auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
					U curhi{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
					U curlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					// register pressure performance issue on several platforms: first do the high half here
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					U curhi5{curhi >> (40 - log2ptrs)};
					curhi >>= 48;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curhi5);
					++offsets[6 * 256 + static_cast<size_t>(curhi)];
					// register pressure performance issue on several platforms: do the low half here second
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					U curlo5{curlo >> (40 - log2ptrs)};
					curlo >>= 48;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curlo5);
					++offsets[6 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{pinput[0]};
					output[i] = p;
					buffer[i] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					U cur5{cur >> (40 - log2ptrs)};
					cur >>= 48;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + cur5);
					++offsets[6 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 56-bit, not in reverse order
				do{
					V *phi{input[i]};
					V *plo{input[i - 1]};
					output[i] = phi;
					auto imhi{indirectinput1<indirection1, isindexed2, T, V>(phi, varparameters...)};
					output[i - 1] = plo;
					auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
					U curhi{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
					U curlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					// register pressure performance issue on several platforms: first do the high half here
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					U curhi5{curhi >> (40 - log2ptrs)};
					curhi >>= 48;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curhi5);
					++offsets[6 * 256 + static_cast<size_t>(curhi)];
					// register pressure performance issue on several platforms: do the low half here second
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					U curlo5{curlo >> (40 - log2ptrs)};
					curlo >>= 48;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curlo5);
					++offsets[6 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					output[0] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					U cur5{cur >> (40 - log2ptrs)};
					cur >>= 48;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + cur5);
					++offsets[6 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(48 == CHAR_BIT * sizeof(T)){
			if constexpr(reverseorder){// also reverse the array at the same time
				V *const *pinput{input};
				do{
					V *phi{pinput[0]};
					V *plo{pinput[1]};
					pinput += 2;
					output[i] = phi;
					buffer[i] = phi;
					auto imhi{indirectinput1<indirection1, isindexed2, T, V>(phi, varparameters...)};
					output[i - 1] = plo;
					buffer[i - 1] = plo;
					auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
					U curhi{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
					U curlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					// register pressure performance issue on several platforms: first do the high half here
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					curhi >>= 40;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++offsets[5 * 256 + static_cast<size_t>(curhi)];
					// register pressure performance issue on several platforms: do the low half here second
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					curlo >>= 40;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++offsets[5 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{pinput[0]};
					output[i] = p;
					buffer[i] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					cur >>= 40;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++offsets[5 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 48-bit, not in reverse order
				do{
					V *phi{input[i]};
					V *plo{input[i - 1]};
					output[i] = phi;
					auto imhi{indirectinput1<indirection1, isindexed2, T, V>(phi, varparameters...)};
					output[i - 1] = plo;
					auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
					U curhi{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
					U curlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					// register pressure performance issue on several platforms: first do the high half here
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					curhi >>= 40;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++offsets[5 * 256 + static_cast<size_t>(curhi)];
					// register pressure performance issue on several platforms: do the low half here second
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					curlo >>= 40;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++offsets[5 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					output[0] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					cur >>= 40;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++offsets[5 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(40 == CHAR_BIT * sizeof(T)){
			if constexpr(reverseorder){// also reverse the array at the same time
				V *const *pinput{input};
				do{
					V *phi{pinput[0]};
					V *plo{pinput[1]};
					pinput += 2;
					output[i] = phi;
					buffer[i] = phi;
					auto imhi{indirectinput1<indirection1, isindexed2, T, V>(phi, varparameters...)};
					output[i - 1] = plo;
					buffer[i - 1] = plo;
					auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
					U curhi{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
					U curlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					curhi >>= 32;
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					curlo >>= 32;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++offsets[4 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++offsets[4 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{pinput[0]};
					output[i] = p;
					buffer[i] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					cur >>= 32;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++offsets[4 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 40-bit, not in reverse order
				do{
					V *phi{input[i]};
					V *plo{input[i - 1]};
					output[i] = phi;
					auto imhi{indirectinput1<indirection1, isindexed2, T, V>(phi, varparameters...)};
					output[i - 1] = plo;
					auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
					U curhi{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
					U curlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					curhi >>= 32;
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					curlo >>= 32;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++offsets[4 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++offsets[4 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					output[0] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					cur >>= 32;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++offsets[4 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(32 == CHAR_BIT * sizeof(T)){
			if constexpr(reverseorder){// also reverse the array at the same time
				V *const *pinput{input};
				do{
					V *pa{pinput[0]};
					V *pb{pinput[1]};
					pinput += 2;
					output[i] = pa;
					buffer[i] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					output[i - 1] = pb;
					buffer[i - 1] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					U cur2a{cura >> (16 - log2ptrs)};
					cura >>= 24;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					U cur2b{curb >> (16 - log2ptrs)};
					curb >>= 24;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					cur2a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					cur2b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2a);
					++offsets[3 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2b);
					++offsets[3 * 256 + static_cast<size_t>(curb)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{pinput[0]};
					output[0] = p;
					buffer[0] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{static_cast<unsigned>(cur) >> (8 - log2ptrs)};
					U cur2{static_cast<unsigned>(cur) >> (16 - log2ptrs)};
					cur >>= 24;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++offsets[3 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 32-bit, not in reverse order
				do{
					V *pa{input[i]};
					V *pb{input[i - 1]};
					output[i] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					output[i - 1] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					U cur2a{cura >> (16 - log2ptrs)};
					cura >>= 24;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					U cur2b{curb >> (16 - log2ptrs)};
					curb >>= 24;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					cur2a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					cur2b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2a);
					++offsets[3 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2b);
					++offsets[3 * 256 + static_cast<size_t>(curb)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					output[0] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{static_cast<unsigned>(cur) >> (8 - log2ptrs)};
					U cur2{static_cast<unsigned>(cur) >> (16 - log2ptrs)};
					cur >>= 24;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++offsets[3 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(24 == CHAR_BIT * sizeof(T)){
			if constexpr(reverseorder){// also reverse the array at the same time
				V *const *pinput{input};
				i -= 2;
				if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					V *pa{pinput[0]};
					V *pb{pinput[1]};
					V *pc{pinput[2]};
					pinput += 3;
					output[i + 3] = pa;
					buffer[i + 3] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					output[i + 2] = pb;
					buffer[i + 2] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					output[i + 1] = pc;
					buffer[i + 1] = pc;
					auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					cura >>= 16;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					curb >>= 16;
					U cur0c{curc & 0xFFu};
					U cur1c{curc >> (8 - log2ptrs)};
					curc >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[cur0c];
					cur1c &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curc &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++offsets[2 * 256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1c);
					++offsets[2 * 256 + static_cast<size_t>(curc)];
					i -= 3;
				}while(0 <= i);
				if(2 & i){
					V *pa{pinput[0]};
					V *pb{pinput[1]};
					pinput += 2;
					output[i + 3] = pa;
					buffer[i + 3] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					output[i + 2] = pb;
					buffer[i + 2] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					cura >>= 16;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					curb >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++offsets[2 * 256 + static_cast<size_t>(curb)];
				}
				if(1 & i){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					output[0] = p;
					buffer[0] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{static_cast<unsigned>(cur) >> (8 - log2ptrs)};
					cur >>= 16;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++offsets[2 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 24-bit, not in reverse order
				i -= 2;
				if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					V *pa{input[i + 2]};
					V *pb{input[i + 1]};
					V *pc{input[i]};
					output[i + 2] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					output[i + 1] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					output[i] = pc;
					auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					cura >>= 16;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					curb >>= 16;
					U cur0c{curc & 0xFFu};
					U cur1c{curc >> (8 - log2ptrs)};
					curc >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[cur0c];
					cur1c &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curc &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++offsets[2 * 256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1c);
					++offsets[2 * 256 + static_cast<size_t>(curc)];
					i -= 3;
				}while(0 <= i);
				if(2 & i){// possibly finalize 2 entries after the loop above
					V *pa{input[i + 3]};
					V *pb{input[i + 2]};
					output[i + 3] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					output[i + 2] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					cura >>= 16;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					curb >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++offsets[2 * 256 + static_cast<size_t>(curb)];
				}
				if(1 & i){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					output[0] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{static_cast<unsigned>(cur) >> (8 - log2ptrs)};
					cur >>= 16;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++offsets[2 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(16 == CHAR_BIT * sizeof(T)){
			if constexpr(reverseorder){// also reverse the array at the same time
				V *const *pinput{input};
				i -= 3;
				if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					V *pa{pinput[0]};
					V *pb{pinput[1]};
					V *pc{pinput[2]};
					V *pd{pinput[3]};
					pinput += 4;
					output[i + 3] = pa;
					buffer[i + 3] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					output[i + 2] = pb;
					buffer[i + 2] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					output[i + 1] = pc;
					buffer[i + 1] = pc;
					auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
					output[i] = pd;
					buffer[i] = pd;
					auto imd{indirectinput1<indirection1, isindexed2, T, V>(pd, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
					U curd{indirectinput2<indirection1, indirection2, isindexed2, T>(imd, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
					}
					U cur0a{cura & 0xFFu};
					cura >>= 8;
					U cur0b{curb & 0xFFu};
					curb >>= 8;
					U cur0c{curc & 0xFFu};
					curc >>= 8;
					U cur0d{curd & 0xFFu};
					curd >>= 8;
					++offsets[cur0a];
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[cur0c];
					if constexpr(absolute && issigned && isfloatingpoint) curc &= 0x7Fu;
					++offsets[cur0d];
					if constexpr(absolute && issigned && isfloatingpoint) curd &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[256 + static_cast<size_t>(curc)];
					++offsets[256 + static_cast<size_t>(curd)];
					i -= 4;
				}while(0 <= i);
				if(2 & i){
					V *pa{pinput[0]};
					V *pb{pinput[1]};
					pinput += 2;
					output[i + 3] = pa;
					buffer[i + 3] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					output[i + 2] = pb;
					buffer[i + 2] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					U cur0a{cura & 0xFFu};
					cura >>= 8;
					U cur0b{curb & 0xFFu};
					curb >>= 8;
					++offsets[cur0a];
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[256 + static_cast<size_t>(curb)];
				}
				if(1 & i){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					output[0] = p;
					buffer[0] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					cur >>= 8;
					++offsets[cur0];
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cur)];
				}
			}else{// 16-bit, not in reverse order
				i -= 3;
				if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					V *pa{input[i + 3]};
					V *pb{input[i + 2]};
					V *pc{input[i + 1]};
					V *pd{input[i]};
					output[i + 3] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					output[i + 2] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					output[i + 1] = pc;
					auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
					output[i] = pd;
					auto imd{indirectinput1<indirection1, isindexed2, T, V>(pd, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
					U curd{indirectinput2<indirection1, indirection2, isindexed2, T>(imd, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
					}
					U cur0a{cura & 0xFFu};
					cura >>= 8;
					U cur0b{curb & 0xFFu};
					curb >>= 8;
					U cur0c{curc & 0xFFu};
					curc >>= 8;
					U cur0d{curd & 0xFFu};
					curd >>= 8;
					++offsets[cur0a];
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[cur0c];
					if constexpr(absolute && issigned && isfloatingpoint) curc &= 0x7Fu;
					++offsets[cur0d];
					if constexpr(absolute && issigned && isfloatingpoint) curd &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[256 + static_cast<size_t>(curc)];
					++offsets[256 + static_cast<size_t>(curd)];
					i -= 4;
				}while(0 <= i);
				if(2 & i){// possibly finalize 2 entries after the loop above
					V *pa{input[i + 3]};
					V *pb{input[i + 2]};
					output[i + 3] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					output[i + 2] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					U cur0a{cura & 0xFFu};
					cura >>= 8;
					U cur0b{curb & 0xFFu};
					curb >>= 8;
					++offsets[cur0a];
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[256 + static_cast<size_t>(curb)];
				}
				if(1 & i){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					output[0] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					cur >>= 8;
					++offsets[cur0];
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cur)];
				}
			}
		}else static_assert(false, "Implementing larger types will require additional work and optimisation for this library.");

		// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
		auto[runsteps, paritybool]{generateoffsetsmulti<reversesort, absolute, issigned, isfloatingpoint, T>(count, offsets)};

		// perform the bidirectional 8-bit sorting sequence
		if(runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
			[[likely]]
#endif
		{
			V **pdst{buffer}, **pdstnext{output};// for the next iteration
			unsigned shifter{bitscanforwardportable(runsteps)};// at least 1 bit is set inside runsteps as by previous check
			V *const *psrclo, *const *psrchi;
			if constexpr(!reverseorder){
				psrclo = input;
				psrchi = input + count;
			}
			if(paritybool){// swap if the count of sorting actions to do is odd
				pdst = output;
				pdstnext = buffer;
			}
			// skip a step if possible
			runsteps >>= shifter;
			size_t *poffset{offsets + static_cast<size_t>(shifter) * 256};
			if constexpr(reverseorder){
				psrclo = pdstnext;
				psrchi = pdstnext + count;
			}
			if constexpr(!absolute && isfloatingpoint) if(CHAR_BIT * sizeof(T) / 8 - 1 == shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
				[[unlikely]]
#endif
				goto handletop8;// rare, but possible
			if constexpr(!isaddressingsubdivisable) shifter *= 8;
			else shifter ^= ((1 << CHAR_BIT * sizeof(T) / 8) - 1) * (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1));// same as CHAR_BIT * sizeof(T) / 8 - 1 - shifter for big endian, no-op for little endian
			for(;;){
				do{// fill the array, two at a time: one low-to-middle, one high-to-middle
					V *plo{*psrclo++};
					V *phi{*psrchi--};
					auto imlo{indirectinput1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(plo, shifter, varparameters...)};
					auto imhi{indirectinput1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(phi, shifter, varparameters...)};
					auto outlo{indirectinput2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imlo, shifter, varparameters...)};
					auto outhi{indirectinput2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imhi, shifter, varparameters...)};
					auto[curlo, curhi]{filtershift8<absolute, issigned, isfloatingpoint,
						std::conditional_t<std::is_same_v<uint_least64_t, decltype(outlo)>, uint_least64_t, T>,
						U>(outlo, outhi, shifter)};
					size_t offsetlo{poffset[curlo]++};// the next item will be placed one higher
					size_t offsethi{poffset[curhi + offsetsstride]--};// the next item will be placed one lower
					pdst[offsetlo] = plo;
					pdst[offsethi] = phi;
				}while(psrclo < psrchi);
				if(psrclo == psrchi){// fill in the final item for odd counts
					V *plo{*psrclo};
					auto imlo{indirectinput1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(plo, shifter, varparameters...)};
					auto outlo{indirectinput2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imlo, shifter, varparameters...)};
					size_t curlo{filtershift8<absolute, issigned, isfloatingpoint,
						std::conditional_t<std::is_same_v<uint_least64_t, decltype(outlo)>, uint_least64_t, T>,
						U>(outlo, shifter)};
					size_t offsetlo{poffset[curlo]};
					pdst[offsetlo] = plo;
				}
				runsteps >>= 1;
				if(!runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
					break;
				{
#if defined(__has_cpp_attribute) && __has_cpp_attribute(maybe_unused)
					[[maybe_unused]]
#endif
					unsigned index;
					if constexpr(16 < CHAR_BIT * sizeof(T)) index = bitscanforwardportable(runsteps);// at least 1 bit is set inside runsteps as by previous check
					if constexpr(!isaddressingsubdivisable) shifter += 8;
					else shifter -= (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 2)) - 1;// 1 if big endian or -1 if little endian for this type
					poffset += 256;
					// swap the pointers for the next round, data is moved on each iteration
					psrclo = pdst;
					psrchi = pdst + count;
					pdst = pdstnext;
					pdstnext = const_cast<V **>(psrclo);// never written past the first iteration
					// skip a step if possible
					if constexpr(16 < CHAR_BIT * sizeof(T)){
						runsteps >>= index;
						if constexpr(!isaddressingsubdivisable) shifter += index * 8;
						else shifter -= index * ((*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 2)) - 1);// 1 if big endian or -1 if little endian for this type
						poffset += static_cast<size_t>(index) * 256;
					}
				}
				// handle the top part for floating-point differently
				if(!absolute && isfloatingpoint && (isaddressingsubdivisable?
					(sizeof(T) - 1) * (1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1))) :// 0 on big endian, and highest part on little endian
					CHAR_BIT * sizeof(T) - 8) == shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
				{
handletop8:// this prevents "!absolute && isfloatingpoint" to be made constexpr here, but that's fine
					do{// fill the array, two at a time: one low-to-middle, one high-to-middle
						V *plo{*psrclo++};
						V *phi{*psrchi--};
						// short-circuit some of the options here to only get the top part once if possible, and not twice
						// again, input 0 on big endian, and highest part on little endian
						auto imlo{indirectinput1<indirection1, true, true, true, isindexed2, T, V>(plo, (sizeof(T) - 1) * (1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1))), varparameters...)};
						auto imhi{indirectinput1<indirection1, true, true, true, isindexed2, T, V>(phi, (sizeof(T) - 1) * (1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1))), varparameters...)};
						auto outlo{indirectinput2<indirection1, true, true, true, indirection2, isindexed2, T, V>(imlo, (sizeof(T) - 1) * (1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1))))};
						auto outhi{indirectinput2<indirection1, true, true, true, indirection2, isindexed2, T, V>(imhi, (sizeof(T) - 1) * (1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1))))};
						auto[curlo, curhi]{filtertop8<absolute, issigned, isfloatingpoint, decltype(outlo), U>(outlo, outhi)};
						size_t offsetlo{offsets[curlo + (CHAR_BIT * sizeof(T) - 8) * 256 / 8]++};// the next item will be placed one higher
						size_t offsethi{offsets[curhi + (CHAR_BIT * sizeof(T) - 8) * 256 / 8 + offsetsstride]--};// the next item will be placed one lower
						pdst[offsetlo] = plo;
						pdst[offsethi] = phi;
					}while(psrclo < psrchi);
					if(psrclo == psrchi){// fill in the final item for odd counts
						V *plo{*psrclo};
						// short-circuit some of the options here to only get the top part once if possible, and not twice
						// again, input 0 on big endian, and highest part on little endian
						auto imlo{indirectinput1<indirection1, true, true, true, isindexed2, T, V>(plo, (sizeof(T) - 1) * (1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1))), varparameters...)};
						auto outlo{indirectinput2<indirection1, true, true, true, indirection2, isindexed2, T, V>(imlo, (sizeof(T) - 1) * (1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1))), varparameters...)};
						size_t curlo{filtertop8<absolute, issigned, isfloatingpoint, decltype(outlo), U>(outlo)};
						size_t offsetlo{offsets[curlo + (CHAR_BIT * sizeof(T) - 8) * 256 / 8]};
						pdst[offsetlo] = plo;
					}
					break;// no further processing beyond the top part
				}
			}
		}
	}
}

// radixsortcopynoalloc() function implementation template for 80-bit-based long double types with indirection
// Platforms with a native 80-bit long double type are all little endian, hence that is the only implementation here.
template<auto indirection1, bool reversesort, bool reverseorder, bool absolute, bool issigned, bool isfloatingpoint, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
RSBD8_FUNC_NORMAL std::enable_if_t<
	(std::is_member_function_pointer_v<decltype(indirection1)> ||
	std::is_member_object_pointer_v<decltype(indirection1)>) &&
	(std::is_same_v<longdoubletest128, std::remove_pointer_t<std::remove_cvref_t<memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>> ||
	std::is_same_v<longdoubletest96, std::remove_pointer_t<std::remove_cvref_t<memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>> ||
	std::is_same_v<longdoubletest80, std::remove_pointer_t<std::remove_cvref_t<memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>> ||
	std::is_same_v<long double, std::remove_pointer_t<std::remove_cvref_t<memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>> &&
	64 == LDBL_MANT_DIG &&
	16384 == LDBL_MAX_EXP &&
	128 >= CHAR_BIT * sizeof(long double) &&
	64 < CHAR_BIT * sizeof(long double)),
	void> radixsortcopynoallocmulti(size_t count, V *const input[], V *output[], V *buffer[], vararguments... varparameters)noexcept(
	std::is_member_object_pointer_v<decltype(indirection1)> ||
	std::is_nothrow_invocable_v<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>){
	using T = std::remove_pointer_t<std::remove_cvref_t<memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>;
	using W = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t,
		std::conditional_t<96 == CHAR_BIT * sizeof(T), uint_least32_t,
		std::conditional_t<80 == CHAR_BIT * sizeof(T), uint_least16_t, void>>>;
	using U = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t, unsigned>;// assume zero-extension to be basically free for U on basically all modern machines
	static bool constexpr isaddressingsubdivisable{
		!(absolute && !issigned) &&// the two tiered absolute types shift bits out of place during filering
#ifdef RSBD8_DISABLE_ADDRESS_SUBDIVISION
		false
#else
		8 == CHAR_BIT &&// optimisation for multi-part addressable machines only
		(std::is_member_object_pointer_v<decltype(indirection1)> ||
		std::is_pointer_v<std::invoke_result_t<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>>)
#endif
	};
	// do not pass a nullptr here, even though it's safe if count is 0
	assert(input);
	assert(output);
	assert(buffer);
	// All the code in this function is adapted for count to be one below its input value here.
	--count;
	if(0 < static_cast<ptrdiff_t>(count)){// a 0 or 1 count array is legal here
		static size_t constexpr offsetsstride{80 * 256 / 8 - (absolute && issigned) * (127 + isfloatingpoint)};// shrink the offsets size if possible
		size_t offsets[offsetsstride * 2];// a sizeable amount of indices, but it's worth it, and this function never calls functions either to further increase stack usage anyway
		std::memset(offsets, 0, sizeof(offsets) / 2);// only the low half

		// count the 256 configurations, all in one go
		ptrdiff_t i{static_cast<ptrdiff_t>(count)};
		if constexpr(reverseorder){// also reverse the array at the same time
			V *const *pinput{input};
			do{
				V *p{*pinput++};
				output[i] = p;
				buffer[i] = p;
				auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
				auto[curm, cure]{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
				if constexpr(isfloatingpoint != absolute || absolute && !issigned){
					filterinput<absolute, issigned, isfloatingpoint, T>(curm, cure);
				}
				unsigned cure0{static_cast<unsigned>(cure & 0xFFu)};
				cure >>= 8;
				unsigned curm0{static_cast<unsigned>(curm & 0xFFu)};
				unsigned curm1{static_cast<unsigned>(curm >> (8 - log2ptrs))};
				unsigned curm2{static_cast<unsigned>(curm >> (16 - log2ptrs))};
				unsigned curm3{static_cast<unsigned>(curm >> (24 - log2ptrs))};
				unsigned curm4{static_cast<unsigned>(curm >> (32 - log2ptrs))};
				unsigned curm5{static_cast<unsigned>(curm >> (40 - log2ptrs))};
				unsigned curm6{static_cast<unsigned>(curm >> (48 - log2ptrs))};
				curm >>= 56;
				++offsets[8 * 256 + static_cast<size_t>(cure)];
				if constexpr(absolute && issigned && isfloatingpoint) cure &= 0x7Fu;
				++offsets[curm0];
				curm1 &= sizeof(void *) * 0xFFu;
				curm2 &= sizeof(void *) * 0xFFu;
				curm3 &= sizeof(void *) * 0xFFu;
				curm4 &= sizeof(void *) * 0xFFu;
				curm5 &= sizeof(void *) * 0xFFu;
				curm6 &= sizeof(void *) * 0xFFu;
				++offsets[7 * 256 + static_cast<size_t>(curm)];
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curm1);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curm2);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curm3);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curm4);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curm5);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curm6);
			}while(0 <= --i);
		}else{// not in reverse order
			do{
				V *p{input[i]};
				output[i] = p;
				auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
				auto[curm, cure]{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
				if constexpr(isfloatingpoint != absolute || absolute && !issigned){
					filterinput<absolute, issigned, isfloatingpoint, T>(curm, cure);
				}
				unsigned cure0{static_cast<unsigned>(cure & 0xFFu)};
				cure >>= 8;
				unsigned curm0{static_cast<unsigned>(curm & 0xFFu)};
				unsigned curm1{static_cast<unsigned>(curm >> (8 - log2ptrs))};
				unsigned curm2{static_cast<unsigned>(curm >> (16 - log2ptrs))};
				unsigned curm3{static_cast<unsigned>(curm >> (24 - log2ptrs))};
				unsigned curm4{static_cast<unsigned>(curm >> (32 - log2ptrs))};
				unsigned curm5{static_cast<unsigned>(curm >> (40 - log2ptrs))};
				unsigned curm6{static_cast<unsigned>(curm >> (48 - log2ptrs))};
				curm >>= 56;
				++offsets[8 * 256 + static_cast<size_t>(cure)];
				if constexpr(absolute && issigned && isfloatingpoint) cure &= 0x7Fu;
				++offsets[curm0];
				curm1 &= sizeof(void *) * 0xFFu;
				curm2 &= sizeof(void *) * 0xFFu;
				curm3 &= sizeof(void *) * 0xFFu;
				curm4 &= sizeof(void *) * 0xFFu;
				curm5 &= sizeof(void *) * 0xFFu;
				curm6 &= sizeof(void *) * 0xFFu;
				++offsets[7 * 256 + static_cast<size_t>(curm)];
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curm1);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curm2);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curm3);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curm4);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curm5);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curm6);
			}while(0 <= --i);
		}

		// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
		auto[runsteps, paritybool]{generateoffsetsmulti<reversesort, absolute, issigned, isfloatingpoint, T>(count, offsets)};

		// perform the bidirectional 8-bit sorting sequence
		if(runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
			[[likely]]
#endif
		{
			V **pdst{buffer}, **pdstnext{output};// for the next iteration
			unsigned shifter{bitscanforwardportable(runsteps)};// at least 1 bit is set inside runsteps as by previous check
			V *const *psrclo, *const *psrchi;
			if constexpr(!reverseorder){
				psrclo = input;
				psrchi = input + count;
			}
			if(paritybool){// swap if the count of sorting actions to do is odd
				pdst = output;
				pdstnext = buffer;
			}
			// skip a step if possible
			runsteps >>= shifter;
			size_t *poffset{offsets + static_cast<size_t>(shifter) * 256};
			if constexpr(reverseorder){
				psrclo = pdstnext;
				psrchi = pdstnext + count;
			}
			if(80 / 8 - 2 == shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
				[[unlikely]]
#endif
			goto handletop16;// rare, but possible
			if(80 / 8 - 2 < shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
				[[unlikely]]
#endif
			goto handletop8;// rare, but possible
			if constexpr(!isaddressingsubdivisable) shifter *= 8;
			for(;;){
				do{// fill the array, two at a time: one low-to-middle, one high-to-middle
					V *plo{*psrclo++};
					V *phi{*psrchi--};
					auto imlo{indirectinput1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(plo, shifter, varparameters...)};
					auto imhi{indirectinput1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(phi, shifter, varparameters...)};
					auto outlo{indirectinput2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imlo, shifter, varparameters...)};
					auto outhi{indirectinput2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imhi, shifter, varparameters...)};
					auto[curlo, curhi]{filtershift8<absolute, issigned, isfloatingpoint,
						std::conditional_t<std::is_integral_v<decltype(outlo)>, decltype(outlo), T>,
						U>(outlo, outhi, shifter)};
					size_t offsetlo{poffset[curlo]++};// the next item will be placed one higher
					size_t offsethi{poffset[curhi + offsetsstride]--};// the next item will be placed one lower
					pdst[offsetlo] = plo;
					pdst[offsethi] = phi;
				}while(psrclo < psrchi);
				if(psrclo == psrchi){// fill in the final item for odd counts
					V *plo{*psrclo};
					auto imlo{indirectinput1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(plo, shifter, varparameters...)};
					auto outlo{indirectinput2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imlo, shifter, varparameters...)};
					size_t curlo{filtershift8<absolute, issigned, isfloatingpoint,
						std::conditional_t<std::is_integral_v<decltype(outlo)>, decltype(outlo), T>,
						U>(outlo, shifter)};
					size_t offsetlo{poffset[curlo]};
					pdst[offsetlo] = plo;
				}
				runsteps >>= 1;
				if(!runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
					break;
				{
					unsigned index{bitscanforwardportable(runsteps)};// at least 1 bit is set inside runsteps as by previous check
					if constexpr(!isaddressingsubdivisable) shifter += 8;
					else ++shifter;
					poffset += 256;
					// swap the pointers for the next round, data is moved on each iteration
					psrclo = pdst;
					psrchi = pdst + count;
					pdst = pdstnext;
					pdstnext = const_cast<V **>(psrclo);// never written past the first iteration
					// skip a step if possible
					runsteps >>= index;
					if constexpr(!isaddressingsubdivisable) shifter += index * 8;
					else shifter += index;
					poffset += static_cast<size_t>(index) * 256;
				}
				// handle the top two parts differently
				if((isaddressingsubdivisable? 80 / 8 - 2 : 80 - 16) <= shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
				{
					if((isaddressingsubdivisable? 80 / 8 - 2 : 80 - 16) == shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
						[[likely]]
#endif
					{
handletop16:
						do{// fill the array, two at a time: one low-to-middle, one high-to-middle
							V *plo{*psrclo++};
							V *phi{*psrchi--};
							auto imlo{indirectinputbelowtop1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(plo, varparameters...)};
							auto imhi{indirectinputbelowtop1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(phi, varparameters...)};
							auto outlo{indirectinputbelowtop2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imlo, varparameters...)};
							auto outhi{indirectinputbelowtop2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imhi, varparameters...)};
							auto[curlo, curhi]{filterbelowtop8<absolute, issigned, isfloatingpoint, T, U>(outlo, outhi)};
							size_t offsetlo{offsets[curlo + (80 - 16) * 256 / 8]++};// the next item will be placed one higher
							size_t offsethi{offsets[curhi + (80 - 16) * 256 / 8 + offsetsstride]--};// the next item will be placed one lower
							pdst[offsetlo] = plo;
							pdst[offsethi] = phi;
						}while(psrclo < psrchi);
						if(psrclo == psrchi){// fill in the final item for odd counts
							V *plo{*psrclo};
							auto imlo{indirectinputbelowtop1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(plo, varparameters...)};
							auto outlo{indirectinputbelowtop2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imlo, varparameters...)};
							size_t curlo{filterbelowtop8<absolute, issigned, isfloatingpoint, T, U>(outlo)};
							size_t offsetlo{offsets[curlo + (80 - 16) * 256 / 8]};
							pdst[offsetlo] = plo;
						}
						runsteps >>= 1;
						if(!runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
							[[unlikely]]
#endif
							break;
						// swap the pointers for the next round, data is moved on each iteration
						psrclo = pdst;
						psrchi = pdst + count;
						pdst = pdstnext;
						// unused: pdstnext = const_cast<V **>(psrclo);// never written past the first iteration
					}
handletop8:
					do{// fill the array, two at a time: one low-to-middle, one high-to-middle
						V *plo{*psrclo++};
						V *phi{*psrchi--};
						auto imlo{indirectinputtop1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(plo, varparameters...)};
						auto imhi{indirectinputtop1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(phi, varparameters...)};
						auto outlo{indirectinputtop2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imlo, varparameters...)};
						auto outhi{indirectinputtop2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imhi, varparameters...)};
						size_t curlo, curhi;
						if constexpr(std::is_integral_v<decltype(outlo)>){
							std::pair<size_t, size_t> cur{filtertop8<absolute, issigned, isfloatingpoint, decltype(outlo), U>(outlo, outhi)};
							curlo = cur.first, curhi = cur.second;
						}else{// only feed the exponent and sign parts to the filter
							std::pair<size_t, size_t> cur{filtertop8<absolute, issigned, isfloatingpoint, uint_least16_t, U>(outlo.second, outhi.second)};
							curlo = cur.first, curhi = cur.second;
						}
						size_t offsetlo{offsets[curlo + (80 - 8) * 256 / 8]++};// the next item will be placed one higher
						size_t offsethi{offsets[curhi + (80 - 8) * 256 / 8 + offsetsstride]--};// the next item will be placed one lower
						pdst[offsetlo] = plo;
						pdst[offsethi] = phi;
					}while(psrclo < psrchi);
					if(psrclo == psrchi){// fill in the final item for odd counts
						V *plo{*psrclo};
						auto imlo{indirectinputtop1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(plo, varparameters...)};
						auto outlo{indirectinputtop2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imlo, varparameters...)};
						size_t curlo;
						if constexpr(std::is_integral_v<decltype(outlo)>){
							curlo = filtertop8<absolute, issigned, isfloatingpoint, decltype(outlo), U>(outlo);
						}else{// only feed the exponent and sign part to the filter
							curlo = filtertop8<absolute, issigned, isfloatingpoint, uint_least16_t, U>(outlo.second);
						}
						size_t offsetlo{offsets[curlo + (80 - 8) * 256 / 8]};
						pdst[offsetlo] = plo;
					}
					break;// no further processing beyond the top part
				}
			}
		}
	}
}

// radixsortnoalloc() function implementation template for multi-part types with indirection
template<auto indirection1, bool reversesort, bool reverseorder, bool absolute, bool issigned, bool isfloatingpoint, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
RSBD8_FUNC_NORMAL std::enable_if_t<
	(std::is_member_function_pointer_v<decltype(indirection1)> ||
	std::is_member_object_pointer_v<decltype(indirection1)> &&
	std::is_unsigned_v<std::remove_pointer_t<std::remove_cvref_t<memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>>) &&
	64 >= CHAR_BIT * sizeof(stripenum<std::remove_pointer_t<std::remove_cvref_t<memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>>) &&
	8 < CHAR_BIT * sizeof(stripenum<std::remove_pointer_t<std::remove_cvref_t<memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>>),
	void> radixsortnoallocmulti(size_t count, V *input[], V *buffer[], bool movetobuffer = false, vararguments... varparameters)noexcept(
	std::is_member_object_pointer_v<decltype(indirection1)> ||
	std::is_nothrow_invocable_v<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>){
	using T = tounifunsigned<std::remove_pointer_t<std::remove_cvref_t<memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>>;
	using U = std::conditional_t<sizeof(T) < sizeof(unsigned), unsigned, T>;// assume zero-extension to be basically free for U on basically all modern machines
	static T constexpr highbit{generatehighbit<T>()};
	static bool constexpr isaddressingsubdivisable{
		!(absolute && !issigned) &&// the two tiered absolute types shift bits out of place during filering
#ifdef RSBD8_DISABLE_ADDRESS_SUBDIVISION
		false
#else
		8 == CHAR_BIT &&// optimisation for multi-part addressable machines only
		(std::is_member_object_pointer_v<decltype(indirection1)> ||
		std::is_pointer_v<std::invoke_result_t<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>>)
#endif
	};
	// do not pass a nullptr here, even though it's safe if count is 0
	assert(input);
	assert(buffer);
	// All the code in this function is adapted for count to be one below its input value here.
	--count;
	if(0 < static_cast<ptrdiff_t>(count)){// a 0 or 1 count array is legal here
		static size_t constexpr offsetsstride{CHAR_BIT * sizeof(T) * 256 / 8 - (absolute && issigned) * (127 + isfloatingpoint)};// shrink the offsets size if possible
		size_t offsets[offsetsstride * 2];// a sizeable amount of indices, but it's worth it, and this function never calls functions either to further increase stack usage anyway
		std::memset(offsets, 0, sizeof(offsets) / 2);// only the low half

		// count the 256 configurations, all in one go
		if constexpr(64 == CHAR_BIT * sizeof(T)){
			if constexpr(reverseorder){// also reverse the array at the same time
				V **pinputlo{input}, **pinputhi{input + count};
				V **pbufferlo{buffer}, **pbufferhi{buffer + count};
				do{
					V *plo{*pinputlo};
					V *phi{*pinputhi};
					*pinputhi-- = plo;
					*pbufferhi-- = plo;
					auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
					*pinputlo++ = phi;
					*pbufferlo++ = phi;
					auto imhi{indirectinput1<indirection1, isindexed2, T, V>(phi, varparameters...)};
					U curlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
					U curhi{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(curlo, curhi);
					}
					// register pressure performance issue on several platforms: first do the low half here
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					U curlo5{curlo >> (40 - log2ptrs)};
					U curlo6{curlo >> (48 - log2ptrs)};
					curlo >>= 56;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					curlo6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curlo5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curlo6);
					++offsets[7 * 256 + static_cast<size_t>(curlo)];
					// register pressure performance issue on several platforms: do the high half here second
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					U curhi5{curhi >> (40 - log2ptrs)};
					U curhi6{curhi >> (48 - log2ptrs)};
					curhi >>= 56;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					curhi6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curhi5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curhi6);
					++offsets[7 * 256 + static_cast<size_t>(curhi)];
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					V *p{*pinputlo};
					// no write to input, as this is the midpoint
					*pbufferhi = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					U cur5{cur >> (40 - log2ptrs)};
					U cur6{cur >> (48 - log2ptrs)};
					cur >>= 56;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					cur6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + cur5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + cur6);
					++offsets[7 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 64-bit, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count)};
				do{
					V *phi{input[i]};
					V *plo{input[i - 1]};
					buffer[i] = phi;
					auto imhi{indirectinput1<indirection1, isindexed2, T, V>(phi, varparameters...)};
					buffer[i - 1] = plo;
					auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
					U curhi{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
					U curlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					// register pressure performance issue on several platforms: first do the high half here
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					U curhi5{curhi >> (40 - log2ptrs)};
					U curhi6{curhi >> (48 - log2ptrs)};
					curhi >>= 56;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					curhi6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curhi5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curhi6);
					++offsets[7 * 256 + static_cast<size_t>(curhi)];
					// register pressure performance issue on several platforms: do the low half here second
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					U curlo5{curlo >> (40 - log2ptrs)};
					U curlo6{curlo >> (48 - log2ptrs)};
					curlo >>= 56;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					curlo6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curlo5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curlo6);
					++offsets[7 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					U cur5{cur >> (40 - log2ptrs)};
					U cur6{cur >> (48 - log2ptrs)};
					cur >>= 56;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					cur6 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + cur5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + cur6);
					++offsets[7 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(56 == CHAR_BIT * sizeof(T)){
			if constexpr(reverseorder){// also reverse the array at the same time
				V **pinputlo{input}, **pinputhi{input + count};
				V **pbufferlo{buffer}, **pbufferhi{buffer + count};
				do{
					V *plo{*pinputlo};
					V *phi{*pinputhi};
					*pinputhi-- = plo;
					*pbufferhi-- = plo;
					auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
					*pinputlo++ = phi;
					*pbufferlo++ = phi;
					auto imhi{indirectinput1<indirection1, isindexed2, T, V>(phi, varparameters...)};
					U curlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
					U curhi{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(curlo, curhi);
					}
					// register pressure performance issue on several platforms: first do the low half here
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					U curlo5{curlo >> (40 - log2ptrs)};
					curlo >>= 48;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curlo5);
					++offsets[6 * 256 + static_cast<size_t>(curlo)];
					// register pressure performance issue on several platforms: do the high half here second
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					U curhi5{curhi >> (40 - log2ptrs)};
					curhi >>= 48;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curhi5);
					++offsets[6 * 256 + static_cast<size_t>(curhi)];
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					V *p{*pinputlo};
					// no write to input, as this is the midpoint
					*pbufferhi = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					U cur5{cur >> (40 - log2ptrs)};
					cur >>= 48;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + cur5);
					++offsets[6 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 56-bit, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count)};
				do{
					V *phi{input[i]};
					V *plo{input[i - 1]};
					buffer[i] = phi;
					auto imhi{indirectinput1<indirection1, isindexed2, T, V>(phi, varparameters...)};
					buffer[i - 1] = plo;
					auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
					U curhi{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
					U curlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					// register pressure performance issue on several platforms: first do the high half here
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					U curhi5{curhi >> (40 - log2ptrs)};
					curhi >>= 48;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curhi5);
					++offsets[6 * 256 + static_cast<size_t>(curhi)];
					// register pressure performance issue on several platforms: do the low half here second
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					U curlo5{curlo >> (40 - log2ptrs)};
					curlo >>= 48;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curlo5);
					++offsets[6 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					U cur5{cur >> (40 - log2ptrs)};
					cur >>= 48;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + cur5);
					++offsets[6 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(48 == CHAR_BIT * sizeof(T)){
			if constexpr(reverseorder){// also reverse the array at the same time
				V **pinputlo{input}, **pinputhi{input + count};
				V **pbufferlo{buffer}, **pbufferhi{buffer + count};
				do{
					V *plo{*pinputlo};
					V *phi{*pinputhi};
					*pinputhi-- = plo;
					*pbufferhi-- = plo;
					auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
					*pinputlo++ = phi;
					*pbufferlo++ = phi;
					auto imhi{indirectinput1<indirection1, isindexed2, T, V>(phi, varparameters...)};
					U curlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
					U curhi{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(curlo, curhi);
					}
					// register pressure performance issue on several platforms: first do the low half here
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					curlo >>= 40;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++offsets[5 * 256 + static_cast<size_t>(curlo)];
					// register pressure performance issue on several platforms: do the high half here second
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					curhi >>= 40;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++offsets[5 * 256 + static_cast<size_t>(curhi)];
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					U cur{*pinputlo};
					// no write to input, as this is the midpoint
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					cur >>= 40;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++offsets[5 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 48-bit, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count)};
				do{
					V *phi{input[i]};
					V *plo{input[i - 1]};
					buffer[i] = phi;
					auto imhi{indirectinput1<indirection1, isindexed2, T, V>(phi, varparameters...)};
					buffer[i - 1] = plo;
					auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
					U curhi{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
					U curlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					// register pressure performance issue on several platforms: first do the high half here
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					U curhi4{curhi >> (32 - log2ptrs)};
					curhi >>= 40;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curhi4);
					++offsets[5 * 256 + static_cast<size_t>(curhi)];
					// register pressure performance issue on several platforms: do the low half here second
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					U curlo4{curlo >> (32 - log2ptrs)};
					curlo >>= 40;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curlo4);
					++offsets[5 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					U cur4{cur >> (32 - log2ptrs)};
					cur >>= 40;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + cur4);
					++offsets[5 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(40 == CHAR_BIT * sizeof(T)){
			if constexpr(reverseorder){// also reverse the array at the same time
				V **pinputlo{input}, **pinputhi{input + count};
				V **pbufferlo{buffer}, **pbufferhi{buffer + count};
				do{
					V *plo{*pinputlo};
					V *phi{*pinputhi};
					*pinputhi-- = plo;
					*pbufferhi-- = plo;
					auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
					*pinputlo++ = phi;
					*pbufferlo++ = phi;
					auto imhi{indirectinput1<indirection1, isindexed2, T, V>(phi, varparameters...)};
					U curlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
					U curhi{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(curlo, curhi);
					}
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					curlo >>= 32;
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					curhi >>= 32;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++offsets[4 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++offsets[4 * 256 + static_cast<size_t>(curhi)];
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					V *p{*pinputlo};
					// no write to input, as this is the midpoint
					*pbufferhi = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					cur >>= 32;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++offsets[4 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 40-bit, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count)};
				do{
					V *phi{input[i]};
					V *plo{input[i - 1]};
					buffer[i] = phi;
					auto imhi{indirectinput1<indirection1, isindexed2, T, V>(phi, varparameters...)};
					buffer[i - 1] = plo;
					auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
					U curhi{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
					U curlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					U curhi0{curhi & 0xFFu};
					U curhi1{curhi >> (8 - log2ptrs)};
					U curhi2{curhi >> (16 - log2ptrs)};
					U curhi3{curhi >> (24 - log2ptrs)};
					if constexpr(isfloatingpoint == absolute && !(absolute && !issigned)) buffer[i] = static_cast<T>(curhi);
					curhi >>= 32;
					U curlo0{curlo & 0xFFu};
					U curlo1{curlo >> (8 - log2ptrs)};
					U curlo2{curlo >> (16 - log2ptrs)};
					U curlo3{curlo >> (24 - log2ptrs)};
					curlo >>= 32;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curhi &= 0x7Fu;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curlo &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curhi3);
					++offsets[4 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curlo3);
					++offsets[4 * 256 + static_cast<size_t>(curlo)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{cur >> (8 - log2ptrs)};
					U cur2{cur >> (16 - log2ptrs)};
					U cur3{cur >> (24 - log2ptrs)};
					cur >>= 32;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + cur3);
					++offsets[4 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(32 == CHAR_BIT * sizeof(T)){
			if constexpr(reverseorder){// also reverse the array at the same time
				V **pinputlo{input}, **pinputhi{input + count};
				V **pbufferlo{buffer}, **pbufferhi{buffer + count};
				do{
					V *pa{*pinputlo};
					V *pb{*pinputhi};
					*pinputhi-- = pa;
					*pbufferhi-- = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					*pinputlo++ = pb;
					*pbufferlo++ = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					U cur2a{cura >> (16 - log2ptrs)};
					cura >>= 24;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					U cur2b{curb >> (16 - log2ptrs)};
					curb >>= 24;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					cur2a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					cur2b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2a);
					++offsets[3 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2b);
					++offsets[3 * 256 + static_cast<size_t>(curb)];
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					V *p{*pinputlo};
					// no write to input, as this is the midpoint
					*pbufferhi = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{static_cast<unsigned>(cur) >> (8 - log2ptrs)};
					U cur2{static_cast<unsigned>(cur) >> (16 - log2ptrs)};
					cur >>= 24;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++offsets[3 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 32-bit, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count)};
				do{
					V *pa{input[i]};
					V *pb{input[i - 1]};
					buffer[i] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					buffer[i - 1] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					U cur2a{cura >> (16 - log2ptrs)};
					cura >>= 24;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					U cur2b{curb >> (16 - log2ptrs)};
					curb >>= 24;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					cur2a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					cur2b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2a);
					++offsets[3 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2b);
					++offsets[3 * 256 + static_cast<size_t>(curb)];
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{static_cast<unsigned>(cur) >> (8 - log2ptrs)};
					U cur2{static_cast<unsigned>(cur) >> (16 - log2ptrs)};
					cur >>= 24;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + cur2);
					++offsets[3 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(24 == CHAR_BIT * sizeof(T)){
			if constexpr(reverseorder){// also reverse the array at the same time
				V **pinputlo{input}, **pinputhi{input + count};
				V **pbufferlo{buffer}, **pbufferhi{buffer + count};
				size_t initialcount{(count + 1) % 6};
				if(4 & initialcount){// possibly initialize with 4 entries before the loop below
					V *pa{pinputlo[0]};
					V *pb{pinputhi[0]};
					V *pc{pinputlo[1]};
					V *pd{pinputhi[-1]};
					pinputhi[0] = pa;
					pbufferhi[0] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					pinputlo[0] = pb;
					pbufferlo[0] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					pinputhi[1] = pc;
					pinputhi -= 2;
					pbufferhi[1] = pc;
					pbufferhi -= 2;
					auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
					pinputlo[-1] = pd;
					pinputlo += 2;
					pbufferlo[-1] = pd;
					pbufferlo += 2;
					auto imd{indirectinput1<indirection1, isindexed2, T, V>(pd, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
					U curd{indirectinput2<indirection1, indirection2, isindexed2, T>(imd, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					cura >>= 16;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					curb >>= 16;
					U cur0c{curc & 0xFFu};
					U cur1c{curc >> (8 - log2ptrs)};
					curc >>= 16;
					U cur0d{curd & 0xFFu};
					U cur1d{curd >> (8 - log2ptrs)};
					curd >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[cur0c];
					cur1c &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curc &= 0x7Fu;
					++offsets[cur0d];
					cur1d &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curd &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++offsets[2 * 256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1c);
					++offsets[2 * 256 + static_cast<size_t>(curc)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1d);
					++offsets[2 * 256 + static_cast<size_t>(curd)];
				}else if(2 & initialcount){// possibly initialize with 2 entries before the loop below
					V *pa{pinputlo[0]};
					V *pb{pinputhi[0]};
					*pinputhi-- = pa;
					*pbufferhi-- = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					*pinputlo++ = pb;
					*pbufferlo++ = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					cura >>= 16;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					curb >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++offsets[2 * 256 + static_cast<size_t>(curb)];
				}
				if(5 <= count)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					V *pa{pinputlo[0]};
					V *pb{pinputhi[0]};
					V *pc{pinputlo[1]};
					V *pd{pinputhi[-1]};
					pinputhi[0] = pa;
					pbufferhi[0] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					pinputlo[0] = pb;
					pbufferlo[0] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					pinputhi[1] = pc;
					pbufferhi[1] = pc;
					auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
					// register pressure performance issue on several platforms: first do the high half here
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					cura >>= 16;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					curb >>= 16;
					U cur0c{curc & 0xFFu};
					U cur1c{curc >> (8 - log2ptrs)};
					curc >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[cur0c];
					cur1c &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curc &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++offsets[2 * 256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1c);
					++offsets[2 * 256 + static_cast<size_t>(curc)];
					V *pe{pinputlo[2]};
					V *pf{pinputhi[-2]};
					pinputlo[1] = pd;
					pbufferlo[1] = pd;
					auto imd{indirectinput1<indirection1, isindexed2, T, V>(pd, varparameters...)};
					pinputhi[-2] = pe;
					pinputhi -= 3;
					pbufferhi[-2] = pe;
					pbufferhi -= 3;
					auto ime{indirectinput1<indirection1, isindexed2, T, V>(pe, varparameters...)};
					pinputlo[2] = pf;
					pinputlo += 3;
					pbufferlo[2] = pf;
					pbufferlo += 3;
					auto imf{indirectinput1<indirection1, isindexed2, T, V>(pf, varparameters...)};
					U curd{indirectinput2<indirection1, indirection2, isindexed2, T>(imd, varparameters...)};
					U cure{indirectinput2<indirection1, indirection2, isindexed2, T>(ime, varparameters...)};
					U curf{indirectinput2<indirection1, indirection2, isindexed2, T>(ime, varparameters...)};
					// register pressure performance issue on several platforms: do the low half here second
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(curd, cure, curf);
					}
					U cur0d{curd & 0xFFu};
					U cur1d{curd >> (8 - log2ptrs)};
					curd >>= 16;
					U cur0e{cure & 0xFFu};
					U cur1e{cure >> (8 - log2ptrs)};
					cure >>= 16;
					U cur0f{curf & 0xFFu};
					U cur1f{curf >> (8 - log2ptrs)};
					curf >>= 16;
					++offsets[cur0d];
					cur1d &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curd &= 0x7Fu;
					++offsets[cur0e];
					cur1e &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cure &= 0x7Fu;
					++offsets[cur0f];
					cur1f &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curf &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1d);
					++offsets[2 * 256 + static_cast<size_t>(curd)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1e);
					++offsets[2 * 256 + static_cast<size_t>(cure)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1f);
					++offsets[2 * 256 + static_cast<size_t>(curf)];
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					V *p{*pinputlo};
					// no write to input, as this is the midpoint
					*pbufferhi = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{static_cast<unsigned>(cur) >> (8 - log2ptrs)};
					cur >>= 16;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++offsets[2 * 256 + static_cast<size_t>(cur)];
				}
			}else{// 24-bit, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count) - 2};
				if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					V *pa{input[i + 2]};
					V *pb{input[i + 1]};
					V *pc{input[i]};
					buffer[i + 2] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					buffer[i + 1] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					buffer[i] = pc;
					auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					cura >>= 16;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					curb >>= 16;
					U cur0c{curc & 0xFFu};
					U cur1c{curc >> (8 - log2ptrs)};
					curc >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[cur0c];
					cur1c &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curc &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++offsets[2 * 256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1c);
					++offsets[2 * 256 + static_cast<size_t>(curc)];
					i -= 3;
				}while(0 <= i);
				if(2 & i){// possibly finalize 2 entries after the loop above
					V *pa{input[i + 2]};
					V *pb{input[i + 1]};
					buffer[i + 2] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					buffer[i + 1] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					U cur0a{cura & 0xFFu};
					U cur1a{cura >> (8 - log2ptrs)};
					cura >>= 16;
					U cur0b{curb & 0xFFu};
					U cur1b{curb >> (8 - log2ptrs)};
					curb >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1a);
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1b);
					++offsets[2 * 256 + static_cast<size_t>(curb)];
				}
				if(1 & i){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					U cur1{static_cast<unsigned>(cur) >> (8 - log2ptrs)};
					cur >>= 16;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + cur1);
					++offsets[2 * 256 + static_cast<size_t>(cur)];
				}
			}
		}else if constexpr(16 == CHAR_BIT * sizeof(T)){
			if constexpr(reverseorder){// also reverse the array at the same time
				V **pinputlo{input}, **pinputhi{input + count};
				V **pbufferlo{buffer}, **pbufferhi{buffer + count};
				if(2 & count + 1){// possibly initialize with 2 entries before the loop below
					V *pa{*pinputlo};
					V *pb{*pinputhi};
					*pinputhi-- = pa;
					*pbufferhi-- = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					*pinputlo++ = pb;
					*pbufferlo++ = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					U cur0a{cura & 0xFFu};
					cura >>= 8;
					U cur0b{curb & 0xFFu};
					curb >>= 8;
					++offsets[cur0a];
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[256 + static_cast<size_t>(curb)];
				}
				if(3 <= count)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					V *pa{pinputlo[0]};
					V *pb{pinputhi[0]};
					V *pc{pinputlo[1]};
					V *pd{pinputhi[-1]};
					pinputhi[0] = pa;
					pbufferhi[0] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					pinputlo[0] = pb;
					pbufferlo[0] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					pinputhi[-1] = pc;
					pinputhi -= 2;
					pbufferhi[-1] = pc;
					pbufferhi -= 2;
					auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
					pinputlo[1] = pd;
					pinputlo += 2;
					pbufferlo[1] = pd;
					pbufferlo += 2;
					auto imd{indirectinput1<indirection1, isindexed2, T, V>(pd, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
					U curd{indirectinput2<indirection1, indirection2, isindexed2, T>(imd, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
					}
					U cur0a{cura & 0xFFu};
					cura >>= 8;
					U cur0b{curb & 0xFFu};
					curb >>= 8;
					U cur0c{curc & 0xFFu};
					curc >>= 8;
					U cur0d{curd & 0xFFu};
					curd >>= 8;
					++offsets[cur0a];
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[cur0c];
					if constexpr(absolute && issigned && isfloatingpoint) curc &= 0x7Fu;
					++offsets[cur0d];
					if constexpr(absolute && issigned && isfloatingpoint) curd &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[256 + static_cast<size_t>(curc)];
					++offsets[256 + static_cast<size_t>(curd)];
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					V *p{*pinputlo};
					// no write to input, as this is the midpoint
					*pbufferhi = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					cur >>= 8;
					++offsets[cur0];
					if constexpr(absolute && issigned && isfloatingpoint) cur &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cur)];
				}
			}else{// 16-bit, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count) - 3};
				if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					V *pa{input[i + 3]};
					V *pb{input[i + 2]};
					V *pc{input[i + 1]};
					V *pd{input[i]};
					buffer[i + 3] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					buffer[i + 2] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					buffer[i + 1] = pc;
					auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
					buffer[i] = pd;
					auto imd{indirectinput1<indirection1, isindexed2, T, V>(pd, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
					U curd{indirectinput2<indirection1, indirection2, isindexed2, T>(imd, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
					}
					U cur0a{cura & 0xFFu};
					cura >>= 8;
					U cur0b{curb & 0xFFu};
					curb >>= 8;
					U cur0c{curc & 0xFFu};
					curc >>= 8;
					U cur0d{curd & 0xFFu};
					curd >>= 8;
					++offsets[cur0a];
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[cur0c];
					if constexpr(absolute && issigned && isfloatingpoint) curc &= 0x7Fu;
					++offsets[cur0d];
					if constexpr(absolute && issigned && isfloatingpoint) curd &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[256 + static_cast<size_t>(curc)];
					++offsets[256 + static_cast<size_t>(curd)];
					i -= 4;
				}while(0 <= i);
				if(2 & i){// possibly finalize 2 entries after the loop above
					V *pa{input[i + 3]};
					V *pb{input[i + 2]};
					buffer[i + 3] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					buffer[i + 2] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					U cur0a{cura & 0xFFu};
					cura >>= 8;
					U cur0b{curb & 0xFFu};
					curb >>= 8;
					++offsets[cur0a];
					if constexpr(absolute && issigned && isfloatingpoint) cura &= 0x7Fu;
					++offsets[cur0b];
					if constexpr(absolute && issigned && isfloatingpoint) curb &= 0x7Fu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[256 + static_cast<size_t>(curb)];
				}
				if(1 & i){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
					U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
					if constexpr(isfloatingpoint != absolute || absolute && !issigned){
						filterinput<absolute, issigned, isfloatingpoint, T>(cur);
					}
					U cur0{cur & 0xFFu};
					cur >>= 8;
					++offsets[cur0];
					++offsets[256 + static_cast<size_t>(cur)];
				}
			}
		}else static_assert(false, "Implementing larger types will require additional work and optimisation for this library.");

		// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
		auto[runsteps, paritybool]{generateoffsetsmulti<reversesort, absolute, issigned, isfloatingpoint, T>(count, offsets, movetobuffer)};

		// perform the bidirectional 8-bit sorting sequence
		if(runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
			[[likely]]
#endif
		{
			V **psrclo{input}, **pdst{buffer};
			unsigned shifter{bitscanforwardportable(runsteps)};// at least 1 bit is set inside runsteps as by previous check
			if(paritybool){// swap if the count of sorting actions to do is odd
				psrclo = buffer;
				pdst = input;
			}
			// skip a step if possible
			runsteps >>= shifter;
			size_t *poffset{offsets + static_cast<size_t>(shifter) * 256};
			V **psrchi{psrclo + count};
			V **pdstnext{psrclo};// for the next iteration
			if constexpr(!absolute && isfloatingpoint) if(CHAR_BIT * sizeof(T) / 8 - 1 == shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
				[[unlikely]]
#endif
				goto handletop8;// rare, but possible
			if constexpr(!isaddressingsubdivisable) shifter *= 8;
			else shifter ^= ((1 << CHAR_BIT * sizeof(T) / 8) - 1) * (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1));// same as CHAR_BIT * sizeof(T) / 8 - 1 - shifter for big endian, no-op for little endian
			for(;;){
				do{// fill the array, two at a time: one low-to-middle, one high-to-middle
					V *plo{*psrclo++};
					V *phi{*psrchi--};
					auto imlo{indirectinput1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(plo, shifter, varparameters...)};
					auto imhi{indirectinput1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(phi, shifter, varparameters...)};
					auto outlo{indirectinput2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imlo, shifter, varparameters...)};
					auto outhi{indirectinput2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imhi, shifter, varparameters...)};
					auto[curlo, curhi]{filtershift8<absolute, issigned, isfloatingpoint, decltype(outlo), U>(outlo, outhi, shifter)};
					size_t offsetlo{poffset[curlo]++};// the next item will be placed one higher
					size_t offsethi{poffset[curhi + offsetsstride]--};// the next item will be placed one lower
					pdst[offsetlo] = plo;
					pdst[offsethi] = phi;
				}while(psrclo < psrchi);
				if(psrclo == psrchi){// fill in the final item for odd counts
					V *plo{*psrclo};
					auto imlo{indirectinput1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(plo, shifter, varparameters...)};
					auto outlo{indirectinput2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imlo, shifter, varparameters...)};
					size_t curlo{filtershift8<absolute, issigned, isfloatingpoint, decltype(outlo), U>(outlo, shifter)};
					size_t offsetlo{poffset[curlo]};
					pdst[offsetlo] = plo;
				}
				runsteps >>= 1;
				if(!runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
					break;
				{
#if defined(__has_cpp_attribute) && __has_cpp_attribute(maybe_unused)
				[[maybe_unused]]
#endif
					unsigned index;
					if constexpr(16 < CHAR_BIT * sizeof(T)) index = bitscanforwardportable(runsteps);// at least 1 bit is set inside runsteps as by previous check
					if constexpr(!isaddressingsubdivisable) shifter += 8;
					else shifter -= (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 2)) - 1;// 1 if big endian or -1 if little endian for this type
					poffset += 256;
					// swap the pointers for the next round, data is moved on each iteration
					psrclo = pdst;
					psrchi = pdst + count;
					pdst = pdstnext;
					pdstnext = psrclo;
					// skip a step if possible
					if constexpr(16 < CHAR_BIT * sizeof(T)){
						runsteps >>= index;
						if constexpr(!isaddressingsubdivisable) shifter += index * 8;
						else shifter -= index * ((*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 2)) - 1);// 1 if big endian or -1 if little endian for this type
						poffset += static_cast<size_t>(index) * 256;
					}
				}
				// handle the top part for floating-point differently
				if(!absolute && isfloatingpoint && (isaddressingsubdivisable?
					(sizeof(T) - 1) * (1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1))) :// 0 on big endian, and highest part on little endian
					CHAR_BIT * sizeof(T) - 8) == shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
				{
handletop8:// this prevents "!absolute && isfloatingpoint" to be made constexpr here, but that's fine
					do{// fill the array, two at a time: one low-to-middle, one high-to-middle
						V *plo{*psrclo++};
						V *phi{*psrchi--};
						// short-circuit some of the options here to only get the top part once if possible, and not twice
						auto imlo{indirectinput1<indirection1, true, true, true, isindexed2, T, V>(plo, (sizeof(T) - 1) * (1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1))), varparameters...)};
						auto imhi{indirectinput1<indirection1, true, true, true, isindexed2, T, V>(phi, (sizeof(T) - 1) * (1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1))), varparameters...)};
						auto outlo{indirectinput2<indirection1, true, true, true, indirection2, isindexed2, T, V>(imlo, (sizeof(T) - 1) * (1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1))))};
						auto outhi{indirectinput2<indirection1, true, true, true, indirection2, isindexed2, T, V>(imhi, (sizeof(T) - 1) * (1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1))))};
						auto[curlo, curhi]{filtertop8<absolute, issigned, isfloatingpoint, decltype(outlo), U>(outlo, outhi)};
						size_t offsetlo{offsets[curlo + (CHAR_BIT * sizeof(T) - 8) * 256 / 8]++};// the next item will be placed one higher
						size_t offsethi{offsets[curhi + (CHAR_BIT * sizeof(T) - 8) * 256 / 8 + offsetsstride]--};// the next item will be placed one lower
						pdst[offsetlo] = plo;
						pdst[offsethi] = phi;
					}while(psrclo < psrchi);
					if(psrclo == psrchi){// fill in the final item for odd counts
						V *plo{*psrclo};
						// short-circuit some of the options here to only get the top part once if possible, and not twice
						auto imlo{indirectinput1<indirection1, true, true, true, isindexed2, T, V>(plo, (sizeof(T) - 1) * (1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1))), varparameters...)};
						auto outlo{indirectinput2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imlo, (sizeof(T) - 1) * (1 - (*reinterpret_cast<unsigned char const *>(&highbit) >> (8 - 1))), varparameters...)};
						size_t curlo{filtertop8<absolute, issigned, isfloatingpoint, decltype(outlo), U>(outlo)};
						size_t offsetlo{offsets[curlo + (CHAR_BIT * sizeof(T) - 8) * 256 / 8]};
						pdst[offsetlo] = plo;
					}
					break;// no further processing beyond the top part
				}
			}
		}
	}
}

// radixsortnoalloc() function implementation template for 80-bit-based long double types with indirection
// Platforms with a native 80-bit long double type are all little endian, hence that is the only implementation here.
template<auto indirection1, bool reversesort, bool reverseorder, bool absolute, bool issigned, bool isfloatingpoint, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
RSBD8_FUNC_NORMAL std::enable_if_t<
	(std::is_member_function_pointer_v<decltype(indirection1)> ||
	std::is_member_object_pointer_v<decltype(indirection1)>) &&
	(std::is_same_v<longdoubletest128, std::remove_pointer_t<std::remove_cvref_t<memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>> ||
	std::is_same_v<longdoubletest96, std::remove_pointer_t<std::remove_cvref_t<memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>> ||
	std::is_same_v<longdoubletest80, std::remove_pointer_t<std::remove_cvref_t<memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>> ||
	std::is_same_v<long double, std::remove_pointer_t<std::remove_cvref_t<memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>> &&
	64 == LDBL_MANT_DIG &&
	16384 == LDBL_MAX_EXP &&
	128 >= CHAR_BIT * sizeof(long double) &&
	64 < CHAR_BIT * sizeof(long double)),
	void> radixsortnoallocmulti(size_t count, V *input[], V *buffer[], bool movetobuffer = false, vararguments... varparameters)noexcept(
	std::is_member_object_pointer_v<decltype(indirection1)> ||
	std::is_nothrow_invocable_v<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>){
	using T = std::remove_pointer_t<std::remove_cvref_t<memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>;
	using W = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t,
		std::conditional_t<96 == CHAR_BIT * sizeof(T), uint_least32_t,
		std::conditional_t<80 == CHAR_BIT * sizeof(T), uint_least16_t, void>>>;
	using U = std::conditional_t<128 == CHAR_BIT * sizeof(T), uint_least64_t, unsigned>;// assume zero-extension to be basically free for U on basically all modern machines
	static bool constexpr isaddressingsubdivisable{
		!(absolute && !issigned) &&// the two tiered absolute types shift bits out of place during filering
#ifdef RSBD8_DISABLE_ADDRESS_SUBDIVISION
		false
#else
		8 == CHAR_BIT &&// optimisation for multi-part addressable machines only
		(std::is_member_object_pointer_v<decltype(indirection1)> ||
		std::is_pointer_v<std::invoke_result_t<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>>)
#endif
	};
	// do not pass a nullptr here, even though it's safe if count is 0
	assert(input);
	assert(buffer);
	// All the code in this function is adapted for count to be one below its input value here.
	--count;
	if(0 < static_cast<ptrdiff_t>(count)){// a 0 or 1 count array is legal here
		static size_t constexpr offsetsstride{80 * 256 / 8 - (absolute && issigned) * (127 + isfloatingpoint)};// shrink the offsets size if possible
		size_t offsets[offsetsstride * 2];// a sizeable amount of indices, but it's worth it, and this function never calls functions either to further increase stack usage anyway
		std::memset(offsets, 0, sizeof(offsets) / 2);// only the low half

		// count the 256 configurations, all in one go
		if constexpr(reverseorder){// also reverse the array at the same time
			V **pinputlo{input}, **pinputhi{input + count};
			V **pbufferlo{buffer}, **pbufferhi{buffer + count};
			do{
				V *plo{*pinputlo};
				V *phi{*pinputhi};
				*pinputhi-- = plo;
				*pbufferhi-- = plo;
				auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
				*pinputlo++ = phi;
				*pbufferlo++ = phi;
				auto imhi{indirectinput1<indirection1, isindexed2, T, V>(phi, varparameters...)};
				auto[curmlo, curelo]{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
				auto[curmhi, curehi]{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
				if constexpr(isfloatingpoint != absolute || absolute && !issigned){
					filterinput<absolute, issigned, isfloatingpoint, T>(curmlo, curelo, curmhi, curehi);
				}
				// register pressure performance issue on several platforms: first do the low half here
				unsigned curelo0{static_cast<unsigned>(curelo & 0xFFu)};
				curelo >>= 8;
				unsigned curmlo0{static_cast<unsigned>(curmlo & 0xFFu)};
				unsigned curmlo1{static_cast<unsigned>(curmlo >> (8 - log2ptrs))};
				unsigned curmlo2{static_cast<unsigned>(curmlo >> (16 - log2ptrs))};
				unsigned curmlo3{static_cast<unsigned>(curmlo >> (24 - log2ptrs))};
				unsigned curmlo4{static_cast<unsigned>(curmlo >> (32 - log2ptrs))};
				unsigned curmlo5{static_cast<unsigned>(curmlo >> (40 - log2ptrs))};
				unsigned curmlo6{static_cast<unsigned>(curmlo >> (48 - log2ptrs))};
				curmlo >>= 56;
				++offsets[8 * 256 + static_cast<size_t>(curelo)];
				if constexpr(absolute && issigned && isfloatingpoint) curelo &= 0x7Fu;
				++offsets[curmlo0];
				curmlo1 &= sizeof(void *) * 0xFFu;
				curmlo2 &= sizeof(void *) * 0xFFu;
				curmlo3 &= sizeof(void *) * 0xFFu;
				curmlo4 &= sizeof(void *) * 0xFFu;
				curmlo5 &= sizeof(void *) * 0xFFu;
				curmlo6 &= sizeof(void *) * 0xFFu;
				++offsets[7 * 256 + static_cast<size_t>(curmlo)];
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curmlo1);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curmlo2);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curmlo3);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curmlo4);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curmlo5);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curmlo6);
				// register pressure performance issue on several platforms: do the low half here second
				unsigned curehi0{static_cast<unsigned>(curehi & 0xFFu)};
				curehi >>= 8;
				unsigned curmhi0{static_cast<unsigned>(curmhi & 0xFFu)};
				unsigned curmhi1{static_cast<unsigned>(curmhi >> (8 - log2ptrs))};
				unsigned curmhi2{static_cast<unsigned>(curmhi >> (16 - log2ptrs))};
				unsigned curmhi3{static_cast<unsigned>(curmhi >> (24 - log2ptrs))};
				unsigned curmhi4{static_cast<unsigned>(curmhi >> (32 - log2ptrs))};
				unsigned curmhi5{static_cast<unsigned>(curmhi >> (40 - log2ptrs))};
				unsigned curmhi6{static_cast<unsigned>(curmhi >> (48 - log2ptrs))};
				curmhi >>= 56;
				++offsets[8 * 256 + static_cast<size_t>(curehi)];
				if constexpr(absolute && issigned && isfloatingpoint) curehi &= 0x7Fu;
				++offsets[curmhi0];
				curmhi1 &= sizeof(void *) * 0xFFu;
				curmhi2 &= sizeof(void *) * 0xFFu;
				curmhi3 &= sizeof(void *) * 0xFFu;
				curmhi4 &= sizeof(void *) * 0xFFu;
				curmhi5 &= sizeof(void *) * 0xFFu;
				curmhi6 &= sizeof(void *) * 0xFFu;
				++offsets[7 * 256 + static_cast<size_t>(curmhi)];
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curmhi1);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curmhi2);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curmhi3);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curmhi4);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curmhi5);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curmhi6);
			}while(pinputlo < pinputhi);
			if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
				V *p{*pinputlo};
				// no write to input, as this is the midpoint
				*pbufferhi = p;
				auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
				auto[curm, cure]{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
				if constexpr(isfloatingpoint != absolute || absolute && !issigned){
					filterinput<absolute, issigned, isfloatingpoint, T>(curm, cure);
				}
				unsigned cure0{static_cast<unsigned>(cure & 0xFFu)};
				cure >>= 8;
				unsigned curm0{static_cast<unsigned>(curm & 0xFFu)};
				unsigned curm1{static_cast<unsigned>(curm >> (8 - log2ptrs))};
				unsigned curm2{static_cast<unsigned>(curm >> (16 - log2ptrs))};
				unsigned curm3{static_cast<unsigned>(curm >> (24 - log2ptrs))};
				unsigned curm4{static_cast<unsigned>(curm >> (32 - log2ptrs))};
				unsigned curm5{static_cast<unsigned>(curm >> (40 - log2ptrs))};
				unsigned curm6{static_cast<unsigned>(curm >> (48 - log2ptrs))};
				curm >>= 56;
				++offsets[8 * 256 + static_cast<size_t>(cure)];
				if constexpr(absolute && issigned && isfloatingpoint) cure &= 0x7Fu;
				++offsets[curm0];
				curm1 &= sizeof(void *) * 0xFFu;
				curm2 &= sizeof(void *) * 0xFFu;
				curm3 &= sizeof(void *) * 0xFFu;
				curm4 &= sizeof(void *) * 0xFFu;
				curm5 &= sizeof(void *) * 0xFFu;
				curm6 &= sizeof(void *) * 0xFFu;
				++offsets[7 * 256 + static_cast<size_t>(curm)];
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curm1);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curm2);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curm3);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curm4);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curm5);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curm6);
			}
		}else{// not in reverse order
			ptrdiff_t i{static_cast<ptrdiff_t>(count)};
			do{
				V *p{input[i]};
				buffer[i] = p;
				auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
				auto[curm, cure]{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
				if constexpr(isfloatingpoint != absolute || absolute && !issigned){
					filterinput<absolute, issigned, isfloatingpoint, T>(curm, cure);
				}
				unsigned cure0{static_cast<unsigned>(cure & 0xFFu)};
				cure >>= 8;
				unsigned curm0{static_cast<unsigned>(curm & 0xFFu)};
				unsigned curm1{static_cast<unsigned>(curm >> (8 - log2ptrs))};
				unsigned curm2{static_cast<unsigned>(curm >> (16 - log2ptrs))};
				unsigned curm3{static_cast<unsigned>(curm >> (24 - log2ptrs))};
				unsigned curm4{static_cast<unsigned>(curm >> (32 - log2ptrs))};
				unsigned curm5{static_cast<unsigned>(curm >> (40 - log2ptrs))};
				unsigned curm6{static_cast<unsigned>(curm >> (48 - log2ptrs))};
				curm >>= 56;
				++offsets[8 * 256 + static_cast<size_t>(cure)];
				if constexpr(absolute && issigned && isfloatingpoint) cure &= 0x7Fu;
				++offsets[curm0];
				curm1 &= sizeof(void *) * 0xFFu;
				curm2 &= sizeof(void *) * 0xFFu;
				curm3 &= sizeof(void *) * 0xFFu;
				curm4 &= sizeof(void *) * 0xFFu;
				curm5 &= sizeof(void *) * 0xFFu;
				curm6 &= sizeof(void *) * 0xFFu;
				++offsets[7 * 256 + static_cast<size_t>(curm)];
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 256) + curm1);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 2 * 256) + curm2);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 3 * 256) + curm3);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 4 * 256) + curm4);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 5 * 256) + curm5);
				++*reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(offsets + 6 * 256) + curm6);
			}while(0 < --i);
		}

		// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
		auto[runsteps, paritybool]{generateoffsetsmulti<reversesort, absolute, issigned, isfloatingpoint, T>(count, offsets, movetobuffer)};

		// perform the bidirectional 8-bit sorting sequence
		if(runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
			[[likely]]
#endif
		{
			V **psrclo{input}, **pdst{buffer};
			unsigned shifter{bitscanforwardportable(runsteps)};// at least 1 bit is set inside runsteps as by previous check
			if(paritybool){// swap if the count of sorting actions to do is odd
				psrclo = buffer;
				pdst = input;
			}
			// skip a step if possible
			runsteps >>= shifter;
			size_t *poffset{offsets + static_cast<size_t>(shifter) * 256};
			V **psrchi{psrclo + count};
			V **pdstnext{psrclo};// for the next iteration
			if(80 / 8 - 2 == shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
				[[unlikely]]
#endif
			goto handletop16;// rare, but possible
			if(80 / 8 - 2 < shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
				[[unlikely]]
#endif
			goto handletop8;// rare, but possible
			if constexpr(!isaddressingsubdivisable) shifter *= 8;
			for(;;){
				do{// fill the array, two at a time: one low-to-middle, one high-to-middle
					V *plo{*psrclo++};
					V *phi{*psrchi--};
					auto imlo{indirectinput1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(plo, shifter, varparameters...)};
					auto imhi{indirectinput1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(phi, shifter, varparameters...)};
					auto outlo{indirectinput2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imlo, shifter, varparameters...)};
					auto outhi{indirectinput2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imhi, shifter, varparameters...)};
					auto[curlo, curhi]{filtershift8<absolute, issigned, isfloatingpoint,
						std::conditional_t<std::is_integral_v<decltype(outlo)>, decltype(outlo), T>,
						U>(outlo, outhi, shifter)};
					size_t offsetlo{poffset[curlo]++};// the next item will be placed one higher
					size_t offsethi{poffset[curhi + offsetsstride]--};// the next item will be placed one lower
					pdst[offsetlo] = plo;
					pdst[offsethi] = phi;
				}while(psrclo < psrchi);
				if(psrclo == psrchi){// fill in the final item for odd counts
					V *plo{*psrclo};
					auto imlo{indirectinput1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(plo, shifter, varparameters...)};
					auto outlo{indirectinput2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imlo, shifter, varparameters...)};
					size_t curlo{filtershift8<absolute, issigned, isfloatingpoint,
						std::conditional_t<std::is_integral_v<decltype(outlo)>, decltype(outlo), T>,
						U>(outlo, shifter)};
					size_t offsetlo{poffset[curlo]};
					pdst[offsetlo] = plo;
				}
				runsteps >>= 1;
				if(!runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
					break;
				{
					unsigned index{bitscanforwardportable(runsteps)};// at least 1 bit is set inside runsteps as by previous check
					if constexpr(!isaddressingsubdivisable) shifter += 8;
					else ++shifter;
					poffset += 256;
					// swap the pointers for the next round, data is moved on each iteration
					psrclo = pdst;
					psrchi = pdst + count;
					pdst = pdstnext;
					pdstnext = psrclo;
					// skip a step if possible
					runsteps >>= index;
					if constexpr(!isaddressingsubdivisable) shifter += index * 8;
					else shifter += index;
					poffset += static_cast<size_t>(index) * 256;
				}
				// handle the top two parts differently
				if((isaddressingsubdivisable? 80 / 8 - 2 : 80 - 16) <= shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
				{
					if((isaddressingsubdivisable? 80 / 8 - 2 : 80 - 16) == shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
						[[likely]]
#endif
					{
handletop16:
						do{// fill the array, two at a time: one low-to-middle, one high-to-middle
							V *plo{*psrclo++};
							V *phi{*psrchi--};
							auto imlo{indirectinputbelowtop1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(plo, varparameters...)};
							auto imhi{indirectinputbelowtop1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(phi, varparameters...)};
							auto outlo{indirectinputbelowtop2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imlo, varparameters...)};
							auto outhi{indirectinputbelowtop2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imhi, varparameters...)};
							auto[curlo, curhi]{filterbelowtop8<absolute, issigned, isfloatingpoint, T, U>(outlo, outhi)};
							size_t offsetlo{offsets[curlo + (80 - 16) * 256 / 8]++};// the next item will be placed one higher
							size_t offsethi{offsets[curhi + (80 - 16) * 256 / 8 + offsetsstride]--};// the next item will be placed one lower
							pdst[offsetlo] = plo;
							pdst[offsethi] = phi;
						}while(psrclo < psrchi);
						if(psrclo == psrchi){// fill in the final item for odd counts
							V *plo{*psrclo};
							auto imlo{indirectinputbelowtop1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(plo, varparameters...)};
							auto outlo{indirectinputbelowtop2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imlo, varparameters...)};
							size_t curlo{filterbelowtop8<absolute, issigned, isfloatingpoint, T, U>(outlo)};
							size_t offsetlo{offsets[curlo + (80 - 16) * 256 / 8]};
							pdst[offsetlo] = plo;
						}
						runsteps >>= 1;
						if(!runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
							[[unlikely]]
#endif
							break;
						// swap the pointers for the next round, data is moved on each iteration
						psrclo = pdst;
						psrchi = pdst + count;
						pdst = pdstnext;
						// unused: pdstnext = psrclo;
					}
handletop8:
					do{// fill the array, two at a time: one low-to-middle, one high-to-middle
						V *plo{*psrclo++};
						V *phi{*psrchi--};
						auto imlo{indirectinputtop1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(plo, varparameters...)};
						auto imhi{indirectinputtop1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(phi, varparameters...)};
						auto outlo{indirectinputtop2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imlo, varparameters...)};
						auto outhi{indirectinputtop2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imhi, varparameters...)};
						size_t curlo, curhi;
						if constexpr(std::is_integral_v<decltype(outlo)>){
							std::pair<size_t, size_t> cur{filtertop8<absolute, issigned, isfloatingpoint, decltype(outlo), U>(outlo, outhi)};
							curlo = cur.first, curhi = cur.second;
						}else{// only feed the exponent and sign parts to the filter
							std::pair<size_t, size_t> cur{filtertop8<absolute, issigned, isfloatingpoint, uint_least16_t, U>(outlo.second, outhi.second)};
							curlo = cur.first, curhi = cur.second;
						}
						size_t offsetlo{offsets[curlo + (80 - 8) * 256 / 8]++};// the next item will be placed one higher
						size_t offsethi{offsets[curhi + (80 - 8) * 256 / 8 + offsetsstride]--};// the next item will be placed one lower
						pdst[offsetlo] = plo;
						pdst[offsethi] = phi;
					}while(psrclo < psrchi);
					if(psrclo == psrchi){// fill in the final item for odd counts
						V *plo{*psrclo};
						auto imlo{indirectinputtop1<indirection1, absolute, issigned, isfloatingpoint, isindexed2, T, V>(plo, varparameters...)};
						auto outlo{indirectinputtop2<indirection1, absolute, issigned, isfloatingpoint, indirection2, isindexed2, T, V>(imlo, varparameters...)};
						size_t curlo;
						if constexpr(std::is_integral_v<decltype(outlo)>){
							curlo = filtertop8<absolute, issigned, isfloatingpoint, decltype(outlo), U>(outlo);
						}else{// only feed the exponent and sign part to the filter
							curlo = filtertop8<absolute, issigned, isfloatingpoint, uint_least16_t, U>(outlo.second);
						}
						size_t offsetlo{offsets[curlo + (80 - 8) * 256 / 8]};
						pdst[offsetlo] = plo;
					}
					break;// no further processing beyond the top part
				}
			}
		}
	}
}

// Function implementation templates for single-part types

// radixsortcopynoalloc() function implementation template for single-part types without indirection
template<bool reversesort, bool reverseorder, bool absolute, bool issigned, bool isfloatingpoint, typename T>
RSBD8_FUNC_NORMAL std::enable_if_t<
	std::is_unsigned_v<T> &&
	!std::is_same_v<bool, T> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortcopynoallocsingle(size_t count, T const input[], T output[])noexcept{
	using U = std::conditional_t<sizeof(T) < sizeof(unsigned), unsigned, T>;// assume zero-extension to be basically free for U on basically all modern machines
	// do not pass a nullptr here, even though it's safe if count is 0
	assert(input);
	assert(output);
	// All the code in this function is adapted for count to be one below its input value here.
	--count;
	if(0 < static_cast<ptrdiff_t>(count)){// a 0 or 1 count array is legal here
		static size_t constexpr offsetsstride{CHAR_BIT * sizeof(T) * 256 / 8 - (absolute && issigned) * (127 + isfloatingpoint)};// shrink the offsets size if possible
		size_t offsets[offsetsstride * 2];// a sizeable amount of indices, but it's worth it, and this function never calls functions either to further increase stack usage anyway
		std::memset(offsets, 0, sizeof(offsets) / 2);// only the low half

		// count the 256 configurations, all in one go
		ptrdiff_t i{static_cast<ptrdiff_t>(count)};
		if constexpr(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
			T const *pinput{input};
			i -= 7;
			if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
				[[likely]]
#endif
				do{
				U cura{pinput[0]};
				U curb{pinput[1]};
				U curc{pinput[2]};
				U curd{pinput[3]};
				if constexpr(absolute != isfloatingpoint){// two-register filters only
					// register pressure performance issue on several platforms: first do the high half here
					filterinput<absolute, issigned, isfloatingpoint, T>(
						cura, output + i + 7,
						curb, output + i + 6,
						curc, output + i + 5,
						curd, output + i + 4);
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
				}
				U cure{pinput[4]};
				U curf{pinput[5]};
				U curg{pinput[6]};
				U curh{pinput[7]};
				pinput += 8;
				if constexpr(absolute != isfloatingpoint){// two-register filters only
					// register pressure performance issue on several platforms: do the low half here second
					filterinput<absolute, issigned, isfloatingpoint, T>(
						cure, output + i + 3,
						curf, output + i + 2,
						curg, output + i + 1,
						curh, output + i);
					++offsets[cure];
					++offsets[curf];
					++offsets[curg];
				}else if constexpr(absolute && isfloatingpoint){// one-register filters only
					filterinput<absolute, issigned, isfloatingpoint, T>(
						cura, output + i + 7,
						curb, output + i + 6,
						curc, output + i + 5,
						curd, output + i + 4,
						cure, output + i + 3,
						curf, output + i + 2,
						curg, output + i + 1,
						curh, output + i);
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
					++offsets[cure];
					++offsets[curf];
					++offsets[curg];
				}else{
					output[i + 7] = static_cast<T>(cura);
					++offsets[cura];
					output[i + 6] = static_cast<T>(curb);
					++offsets[curb];
					output[i + 5] = static_cast<T>(curc);
					++offsets[curc];
					output[i + 4] = static_cast<T>(curd);
					++offsets[curd];
					output[i + 3] = static_cast<T>(cure);
					++offsets[cure];
					output[i + 2] = static_cast<T>(curf);
					++offsets[curf];
					output[i + 1] = static_cast<T>(curg);
					++offsets[curg];
					output[i] = static_cast<T>(curh);
				}
				++offsets[curh];
				i -= 8;
			}while(0 <= i);
			if(4 & i){
				U cura{pinput[0]};
				U curb{pinput[1]};
				U curc{pinput[2]};
				U curd{pinput[3]};
				pinput += 4;
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(
						cura, output + i + 7,
						curb, output + i + 6,
						curc, output + i + 5,
						curd, output + i + 4);
					i -= 4;// required for the "if(2 & i){" part
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
				}else{
					output[i + 7] = static_cast<T>(cura);
					++offsets[cura];
					output[i + 6] = static_cast<T>(curb);
					++offsets[curb];
					output[i + 5] = static_cast<T>(curc);
					++offsets[curc];
					output[i + 4] = static_cast<T>(curd);
					i -= 4;// required for the "if(2 & i){" part
				}
				++offsets[curd];
			}
			if(2 & i){
				U cura{pinput[0]};
				U curb{pinput[1]};
				pinput += 2;
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(
						cura, output + i + 7,
						curb, output + i + 6);
					++offsets[cura];
				}else{
					output[i + 7] = static_cast<T>(cura);
					++offsets[cura];
					output[i + 6] = static_cast<T>(curb);
				}
				++offsets[curb];
			}
			if(1 & i){// possibly finalize 1 entry after the loop above
				U cur{pinput[0]};
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(cur, output);
				}else output[0] = static_cast<T>(cur);
				++offsets[cur];
			}
		}else{// not in reverse order
			i -= 7;
			if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
				[[likely]]
#endif
				do{
				U cura{input[i + 7]};
				U curb{input[i + 6]};
				U curc{input[i + 5]};
				U curd{input[i + 4]};
				if constexpr(absolute != isfloatingpoint){// two-register filters only
					// register pressure performance issue on several platforms: first do the high half here
					filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
				}
				U cure{input[i + 3]};
				U curf{input[i + 2]};
				U curg{input[i + 1]};
				U curh{input[i]};
				if constexpr(absolute != isfloatingpoint){// two-register filters only
					// register pressure performance issue on several platforms: do the low half here second
					filterinput<absolute, issigned, isfloatingpoint, T>(cure, curf, curg, curh);
				}else{
					if constexpr(absolute && isfloatingpoint){// one-register filters only
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd, cure, curf, curg, curh);
					}
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
				}
				++offsets[cure];
				++offsets[curf];
				++offsets[curg];
				++offsets[curh];
				i -= 8;
			}while(0 <= i);
			if(4 & i){// possibly finalize 4 entries after the loop above
				U cura{input[i + 7]};
				U curb{input[i + 6]};
				U curc{input[i + 5]};
				U curd{input[i + 4]};
				i -= 4;// required for the "if(2 & i){" part
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
				}
				++offsets[cura];
				++offsets[curb];
				++offsets[curc];
				++offsets[curd];
			}
			if(2 & i){// possibly finalize 2 entries after the loop above
				U cura{input[i + 7]};
				U curb{input[i + 6]};
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb);
				}
				++offsets[cura];
				++offsets[curb];
			}
			if(1 & i){// possibly finalize 1 entry after the loop above
				U cur{input[0]};
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(cur);
				}
				++offsets[cur];
			}
		}

		// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
		if(generateoffsetssingle<reversesort, absolute, issigned, isfloatingpoint, T>(count, offsets))
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
			[[likely]]
#endif
		{

			// perform the bidirectional 8-bit sorting sequence
			T const *psrclo{input}, *psrchi{input + count};
			do{// fill the array, two at a time: one low-to-middle, one high-to-middle
				T outlo{*psrclo++};
				T outhi{*psrchi--};
				auto[curlo, curhi]{filtertop8<absolute, issigned, isfloatingpoint, decltype(outlo), U>(outlo, outhi)};
				size_t offsetlo, offsethi;// this is only allowed for the single-part version, containing just one sorting pass
				if constexpr(false){// useless when not handling indirection: reverseorder){
					offsetlo = offsets[curlo + offsetsstride]--;// the next item will be placed one lower
					offsethi = offsets[curhi]++;// the next item will be placed one higher
				}else{
					offsetlo = offsets[curlo]++;// the next item will be placed one higher
					offsethi = offsets[curhi + offsetsstride]--;// the next item will be placed one lower
				}
				output[offsetlo] = outlo;
				output[offsethi] = outhi;
			}while(psrclo < psrchi);
			if(psrclo == psrchi){// fill in the final item for odd counts
				T outlo{*psrclo};
				size_t curlo{filtertop8<absolute, issigned, isfloatingpoint, decltype(outlo), U>(outlo)};
				size_t offsetlo;// this is only allowed for the single-part version, containing just one sorting pass
				if constexpr(false){// useless when not handling indirection: reverseorder){
					offsetlo = offsets[curlo + offsetsstride];
				}else{
					offsetlo = offsets[curlo];
				}
				output[offsetlo] = outlo;
			}
		}
	}
}

// radixsortnoalloc() function implementation template for single-part types without indirection
template<bool reversesort, bool reverseorder, bool absolute, bool issigned, bool isfloatingpoint, typename T>
RSBD8_FUNC_NORMAL std::enable_if_t<
	std::is_unsigned_v<T> &&
	!std::is_same_v<bool, T> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortnoallocsingle(size_t count, T input[], T buffer[])noexcept{
	using U = std::conditional_t<sizeof(T) < sizeof(unsigned), unsigned, T>;// assume zero-extension to be basically free for U on basically all modern machines
	// do not pass a nullptr here, even though it's safe if count is 0
	assert(input);
	assert(buffer);
	// All the code in this function is adapted for count to be one below its input value here.
	--count;
	if(0 < static_cast<ptrdiff_t>(count)){// a 0 or 1 count array is legal here
		static size_t constexpr offsetsstride{CHAR_BIT * sizeof(T) * 256 / 8 - (absolute && issigned) * (127 + isfloatingpoint)};// shrink the offsets size if possible
		size_t offsets[offsetsstride * 2];// a sizeable amount of indices, but it's worth it, and this function never calls functions either to further increase stack usage anyway
		std::memset(offsets, 0, sizeof(offsets) / 2);// only the low half

		// count the 256 configurations, all in one go
		if constexpr(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
			T *pinputlo{input}, *pinputhi{input + count};
			T *pbufferlo{buffer}, *pbufferhi{buffer + count};
			if(4 & count + 1){// possibly initialize with 4 entries before the loop below
				U cura{pinputlo[0]};
				U curb{pinputhi[0]};
				U curc{pinputlo[1]};
				U curd{pinputhi[-1]};
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(
						cura, pinputhi, pbufferhi,
						curb, pinputlo, pbufferlo,
						curc, pinputhi - 1, pbufferhi - 1,
						curd, pinputlo + 1, pbufferlo + 1);
					pinputhi -= 2;
					pbufferhi -= 2;
					pinputlo += 2;
					pbufferlo += 2;
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
				}else{
					pinputhi[0] = static_cast<T>(cura);
					pbufferhi[0] = static_cast<T>(cura);
					++offsets[cura];
					pinputlo[0] = static_cast<T>(curb);
					pbufferlo[0] = static_cast<T>(curb);
					++offsets[curb];
					pinputhi[-1] = static_cast<T>(curc);
					pinputhi -= 2;
					pbufferhi[-1] = static_cast<T>(curc);
					pbufferhi -= 2;
					++offsets[curc];
					pinputlo[1] = static_cast<T>(curd);
					pinputlo += 2;
					pbufferlo[1] = static_cast<T>(curd);
					pbufferlo += 2;
				}
				++offsets[curd];
			}
			if(2 & count + 1){// possibly initialize with 2 entries before the loop below
				U cura{*pinputlo};
				U curb{*pinputhi};
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(
						cura, pinputhi, pbufferhi,
						curb, pinputlo, pbufferlo);
					--pinputhi;
					--pbufferhi;
					++pinputlo;
					++pbufferlo;
					++offsets[cura];
				}else{
					*pinputhi-- = cura;
					*pbufferhi-- = cura;
					++offsets[cura];
					*pinputlo++ = curb;
					*pbufferlo++ = curb;
				}
				++offsets[curb];
			}
			if(7 <= count)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
				[[likely]]
#endif
				do{
				U cura{pinputlo[0]};
				U curb{pinputhi[0]};
				U curc{pinputlo[1]};
				U curd{pinputhi[-1]};
				if constexpr(absolute != isfloatingpoint){// two-register filters only
					// register pressure performance issue on several platforms: first do the high half here
					filterinput<absolute, issigned, isfloatingpoint, T>(
						cura, pinputhi, pbufferhi,
						curb, pinputlo, pbufferlo,
						curc, pinputhi - 1, pbufferhi - 1,
						curd, pinputlo + 1, pbufferlo + 1);
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
				}
				U cure{pinputlo[2]};
				U curf{pinputhi[-2]};
				U curg{pinputlo[3]};
				U curh{pinputhi[-3]};
				if constexpr(absolute != isfloatingpoint){// two-register filters only
					// register pressure performance issue on several platforms: do the low half here second
					filterinput<absolute, issigned, isfloatingpoint, T>(
						cure, pinputhi - 2, pbufferhi - 2,
						curf, pinputlo + 2, pbufferlo + 2,
						curg, pinputhi - 3, pbufferhi - 3,
						curh, pinputlo + 3, pbufferlo + 3);
					pinputhi -= 4;
					pbufferhi -= 4;
					pinputlo += 4;
					pbufferlo += 4;
					++offsets[cure];
					++offsets[curf];
					++offsets[curg];
				}else if constexpr(absolute && isfloatingpoint){// one-register filters only
					filterinput<absolute, issigned, isfloatingpoint, T>(
						cura, pinputhi, pbufferhi,
						curb, pinputlo, pbufferlo,
						curc, pinputhi - 1, pbufferhi - 1,
						curd, pinputlo + 1, pbufferlo + 1,
						cure, pinputhi - 2, pbufferhi - 2,
						curf, pinputlo + 2, pbufferlo + 2,
						curg, pinputhi - 3, pbufferhi - 3,
						curh, pinputlo + 3, pbufferlo + 3);
					pinputhi -= 4;
					pbufferhi -= 4;
					pinputlo += 4;
					pbufferlo += 4;
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
					++offsets[cure];
					++offsets[curf];
					++offsets[curg];
				}else{
					pinputhi[0] = static_cast<T>(cura);
					pbufferhi[0] = static_cast<T>(cura);
					++offsets[cura];
					pinputlo[0] = static_cast<T>(curb);
					pbufferlo[0] = static_cast<T>(curb);
					++offsets[curb];
					pinputhi[-1] = static_cast<T>(curc);
					pbufferhi[-1] = static_cast<T>(curc);
					++offsets[curc];
					pinputlo[1] = static_cast<T>(curd);
					pbufferlo[1] = static_cast<T>(curd);
					++offsets[curd];
					pinputhi[-2] = static_cast<T>(cure);
					pbufferhi[-2] = static_cast<T>(cure);
					++offsets[cure];
					pinputlo[2] = static_cast<T>(curf);
					pbufferlo[2] = static_cast<T>(curf);
					++offsets[curf];
					pinputhi[-3] = static_cast<T>(curg);
					pinputhi -= 4;
					pbufferhi[-3] = static_cast<T>(curg);
					pbufferhi -= 4;
					++offsets[curg];
					pinputlo[3] = static_cast<T>(curh);
					pinputlo += 4;
					pbufferlo[3] = static_cast<T>(curh);
					pbufferlo += 4;
				}
				++offsets[curh];
			}while(pinputlo < pinputhi);
			if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
				U cur{*pinputlo};
				// no write to input, as this is the midpoint
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(cur, pbufferhi);
				}else *pbufferhi = cur;
				++offsets[cur];
			}
		}else{// not in reverse order
			ptrdiff_t i{static_cast<ptrdiff_t>(count) - 7};
			if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
				[[likely]]
#endif
				do{
				U cura{input[i + 7]};
				U curb{input[i + 6]};
				U curc{input[i + 5]};
				U curd{input[i + 4]};
				if constexpr(absolute != isfloatingpoint){// two-register filters only
					// register pressure performance issue on several platforms: first do the high half here
					filterinput<absolute, issigned, isfloatingpoint, T>(
						cura, buffer + i + 7,
						curb, buffer + i + 6,
						curc, buffer + i + 5,
						curd, buffer + i + 4);
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
				}
				U cure{input[i + 3]};
				U curf{input[i + 2]};
				U curg{input[i + 1]};
				U curh{input[i]};
				if constexpr(absolute != isfloatingpoint){// two-register filters only
					// register pressure performance issue on several platforms: do the low half here second
					filterinput<absolute, issigned, isfloatingpoint, T>(
						cure, buffer + i + 3,
						curf, buffer + i + 2,
						curg, buffer + i + 1,
						curh, buffer + i);
					++offsets[cure];
					++offsets[curf];
					++offsets[curg];
				}else if constexpr(absolute && isfloatingpoint){// one-register filters only
					filterinput<absolute, issigned, isfloatingpoint, T>(
						cura, buffer + i + 7,
						curb, buffer + i + 6,
						curc, buffer + i + 5,
						curd, buffer + i + 4,
						cure, buffer + i + 3,
						curf, buffer + i + 2,
						curg, buffer + i + 1,
						curh, buffer + i);
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
					++offsets[cure];
					++offsets[curf];
					++offsets[curg];
				}else{
					buffer[i + 7] = static_cast<T>(cura);
					++offsets[cura];
					buffer[i + 6] = static_cast<T>(curb);
					++offsets[curb];
					buffer[i + 5] = static_cast<T>(curc);
					++offsets[curc];
					buffer[i + 4] = static_cast<T>(curd);
					++offsets[curd];
					buffer[i + 3] = static_cast<T>(cure);
					++offsets[cure];
					buffer[i + 2] = static_cast<T>(curf);
					++offsets[curf];
					buffer[i + 1] = static_cast<T>(curg);
					++offsets[curg];
					buffer[i] = static_cast<T>(curh);
				}
				++offsets[curh];
				i -= 8;
			}while(0 <= i);
			if(4 & i){// possibly finalize 4 entries after the loop above
				U cura{input[i + 7]};
				U curb{input[i + 6]};
				U curc{input[i + 5]};
				U curd{input[i + 4]};
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(
						cura, buffer + i + 7,
						curb, buffer + i + 6,
						curc, buffer + i + 5,
						curd, buffer + i + 4);
					i -= 4;// required for the "if(2 & i){" part
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
				}else{
					buffer[i + 7] = static_cast<T>(cura);
					++offsets[cura];
					buffer[i + 6] = static_cast<T>(curb);
					++offsets[curb];
					buffer[i + 5] = static_cast<T>(curc);
					++offsets[curc];
					buffer[i + 4] = static_cast<T>(curd);
					i -= 4;// required for the "if(2 & i){" part
				}
				++offsets[curd];
			}
			if(2 & i){// possibly finalize 2 entries after the loop above
				U cura{input[i + 7]};
				U curb{input[i + 6]};
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(
						cura, buffer + i + 7,
						curb, buffer + i + 6);
					++offsets[cura];
				}else{
					buffer[i + 7] = static_cast<T>(cura);
					++offsets[cura];
					buffer[i + 6] = static_cast<T>(curb);
				}
				++offsets[curb];
			}
			if(1 & i){// possibly finalize 1 entry after the loop above
				U cur{input[0]};
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(cur, buffer);
				}else buffer[0] = static_cast<T>(cur);
				++offsets[cur];
			}
		}

		// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
		if(generateoffsetssingle<reversesort, absolute, issigned, isfloatingpoint, T>(count, offsets))
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
			[[likely]]
#endif
		{

			// perform the bidirectional 8-bit sorting sequence
			T *psrclo{buffer}, *psrchi{buffer + count};
			do{// fill the array, two at a time: one low-to-middle, one high-to-middle
				T outlo{*psrclo++};
				T outhi{*psrchi--};
				auto[curlo, curhi]{filtertop8<absolute, issigned, isfloatingpoint, decltype(outlo), U>(outlo, outhi)};
				size_t offsetlo, offsethi;// this is only allowed for the single-part version, containing just one sorting pass
				if constexpr(false){// useless when not handling indirection: reverseorder){
					offsetlo = offsets[curlo + offsetsstride]--;// the next item will be placed one lower
					offsethi = offsets[curhi]++;// the next item will be placed one higher
				}else{
					offsetlo = offsets[curlo]++;// the next item will be placed one higher
					offsethi = offsets[curhi + offsetsstride]--;// the next item will be placed one lower
				}
				input[offsetlo] = outlo;
				input[offsethi] = outhi;
			}while(psrclo < psrchi);
			if(psrclo == psrchi){// fill in the final item for odd counts
				T outlo{*psrclo};
				size_t curlo{filtertop8<absolute, issigned, isfloatingpoint, decltype(outlo), U>(outlo)};
				size_t offsetlo;// this is only allowed for the single-part version, containing just one sorting pass
				if constexpr(false){// useless when not handling indirection: reverseorder){
					offsetlo = offsets[curlo + offsetsstride];
				}else{
					offsetlo = offsets[curlo];
				}
				input[offsetlo] = outlo;
			}
		}
	}
}

// radixsortcopynoalloc() function implementation template for single-part types with indirection
template<auto indirection1, bool reversesort, bool reverseorder, bool absolute, bool issigned, bool isfloatingpoint, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
RSBD8_FUNC_NORMAL std::enable_if_t<// disable this if the V *buffer[] argument from a multi-part version is detected here, and do not allow active compile-time evaluation with it
	(std::is_member_function_pointer_v<decltype(indirection1)> ||
	std::is_member_object_pointer_v<decltype(indirection1)> &&
	std::is_unsigned_v<std::remove_pointer_t<std::remove_cvref_t<memberpointerdeducebody<indirection1, isindexed2, V, vararguments...>>>>) &&
	8 >= CHAR_BIT * sizeof(stripenum<std::remove_pointer_t<std::remove_cvref_t<memberpointerdeducebody<indirection1, isindexed2, V, vararguments...>>>>),
	void> radixsortcopynoallocsingle(size_t count, V *const input[], V *output[], vararguments... varparameters)
	noexcept(std::is_member_object_pointer_v<decltype(indirection1)> ||
		std::is_nothrow_invocable_v<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>){
	using T = tounifunsigned<std::remove_pointer_t<std::remove_cvref_t<memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>>;
	using U = std::conditional_t<sizeof(T) < sizeof(unsigned), unsigned, T>;// assume zero-extension to be basically free for U on basically all modern machines
	// do not pass a nullptr here, even though it's safe if count is 0
	assert(input);
	assert(output);
	// All the code in this function is adapted for count to be one below its input value here.
	--count;
	if(0 < static_cast<ptrdiff_t>(count)){// a 0 or 1 count array is legal here
		static size_t constexpr offsetsstride{offsetsstride - (absolute && issigned) * (127 + isfloatingpoint)};// shrink the offsets size if possible
		size_t offsets[offsetsstride * 2];// a sizeable amount of indices, but it's worth it, and this function never calls functions either to further increase stack usage anyway
		std::memset(offsets, 0, sizeof(offsets) / 2);// only the low half

		// count the 256 configurations, all in one go
		ptrdiff_t i{static_cast<ptrdiff_t>(count)};
		if constexpr(reverseorder){// also reverse the array at the same time
			V *const *pinput{input};
			i -= 7;
			if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
				[[likely]]
#endif
				do{
				V *pa{pinput[0]};
				V *pb{pinput[1]};
				V *pc{pinput[2]};
				V *pd{pinput[3]};
				if constexpr(absolute != isfloatingpoint){// two-register filters only
					output[i + 7] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					output[i + 6] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					output[i + 5] = pc;
					auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
					output[i + 4] = pd;
					auto imd{indirectinput1<indirection1, isindexed2, T, V>(pd, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
					U curd{indirectinput2<indirection1, indirection2, isindexed2, T>(imd, varparameters...)};
					// register pressure performance issue on several platforms: first do the high half here
					filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
				}
				V *pe{pinput[4]};
				V *pf{pinput[5]};
				V *pg{pinput[6]};
				V *ph{pinput[7]};
				pinput += 8;
				if constexpr(absolute != isfloatingpoint){// two-register filters only
					output[i + 3] = pe;
					auto ime{indirectinput1<indirection1, isindexed2, T, V>(pe, varparameters...)};
					output[i + 2] = pf;
					auto imf{indirectinput1<indirection1, isindexed2, T, V>(pf, varparameters...)};
					output[i + 1] = pg;
					auto img{indirectinput1<indirection1, isindexed2, T, V>(pg, varparameters...)};
					output[i] = ph;
					auto imh{indirectinput1<indirection1, isindexed2, T, V>(ph, varparameters...)};
					U cure{indirectinput2<indirection1, indirection2, isindexed2, T>(ime, varparameters...)};
					U curf{indirectinput2<indirection1, indirection2, isindexed2, T>(imf, varparameters...)};
					U curg{indirectinput2<indirection1, indirection2, isindexed2, T>(img, varparameters...)};
					U curh{indirectinput2<indirection1, indirection2, isindexed2, T>(imh, varparameters...)};
					// register pressure performance issue on several platforms: do the low half here second
					filterinput<absolute, issigned, isfloatingpoint, T>(cure, curf, curg, curh);
					++offsets[cure];
					++offsets[curf];
					++offsets[curg];
					++offsets[curh];
				}else{
					output[i + 7] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					output[i + 6] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					output[i + 5] = pc;
					auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
					output[i + 4] = pd;
					auto imd{indirectinput1<indirection1, isindexed2, T, V>(pd, varparameters...)};
					output[i + 3] = pe;
					auto ime{indirectinput1<indirection1, isindexed2, T, V>(pe, varparameters...)};
					output[i + 2] = pf;
					auto imf{indirectinput1<indirection1, isindexed2, T, V>(pf, varparameters...)};
					output[i + 1] = pg;
					auto img{indirectinput1<indirection1, isindexed2, T, V>(pg, varparameters...)};
					output[i] = ph;
					auto imh{indirectinput1<indirection1, isindexed2, T, V>(ph, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
					U curd{indirectinput2<indirection1, indirection2, isindexed2, T>(imd, varparameters...)};
					U cure{indirectinput2<indirection1, indirection2, isindexed2, T>(ime, varparameters...)};
					U curf{indirectinput2<indirection1, indirection2, isindexed2, T>(imf, varparameters...)};
					U curg{indirectinput2<indirection1, indirection2, isindexed2, T>(img, varparameters...)};
					U curh{indirectinput2<indirection1, indirection2, isindexed2, T>(imh, varparameters...)};
					if constexpr(absolute && isfloatingpoint){// one-register filters only
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd, cure, curf, curg, curh);
					}
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
					++offsets[cure];
					++offsets[curf];
					++offsets[curg];
					++offsets[curh];
				}
				i -= 8;
			}while(0 <= i);
			if(4 & i){
				V *pa{pinput[0]};
				V *pb{pinput[1]};
				V *pc{pinput[2]};
				V *pd{pinput[3]};
				pinput += 4;
				output[i + 7] = pa;
				auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
				output[i + 6] = pb;
				auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
				output[i + 5] = pc;
				auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
				output[i + 4] = pd;
				i -= 4;// required for the "if(2 & i){" part
				auto imd{indirectinput1<indirection1, isindexed2, T, V>(pd, varparameters...)};
				U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
				U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
				U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
				U curd{indirectinput2<indirection1, indirection2, isindexed2, T>(imd, varparameters...)};
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
				}
				++offsets[cura];
				++offsets[curb];
				++offsets[curc];
				++offsets[curd];
			}
			if(2 & i){
				V *pa{pinput[0]};
				V *pb{pinput[1]};
				pinput += 2;
				output[i + 7] = pa;
				auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
				output[i + 6] = pb;
				auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
				U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
				U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb);
				}
				++offsets[cura];
				++offsets[curb];
			}
			if(1 & i){// possibly finalize 1 entry after the loop above
				V *p{pinput[0]};
				output[0] = p;
				auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
				U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(cur);
				}
				++offsets[cur];
			}
		}else{// not in reverse order
			i -= 7;
			if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
				[[likely]]
#endif
				do{
				V *pa{input[i + 7]};
				V *pb{input[i + 6]};
				V *pc{input[i + 5]};
				V *pd{input[i + 4]};
				if constexpr(absolute != isfloatingpoint){// two-register filters only
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
					auto imd{indirectinput1<indirection1, isindexed2, T, V>(pd, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
					U curd{indirectinput2<indirection1, indirection2, isindexed2, T>(imd, varparameters...)};
					// register pressure performance issue on several platforms: first do the high half here
					filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
				}
				V *pe{input[i + 3]};
				V *pf{input[i + 2]};
				V *pg{input[i + 1]};
				V *ph{input[i]};
				if constexpr(absolute != isfloatingpoint){// two-register filters only
					auto ime{indirectinput1<indirection1, isindexed2, T, V>(pe, varparameters...)};
					auto imf{indirectinput1<indirection1, isindexed2, T, V>(pf, varparameters...)};
					auto img{indirectinput1<indirection1, isindexed2, T, V>(pg, varparameters...)};
					auto imh{indirectinput1<indirection1, isindexed2, T, V>(ph, varparameters...)};
					U cure{indirectinput2<indirection1, indirection2, isindexed2, T>(ime, varparameters...)};
					U curf{indirectinput2<indirection1, indirection2, isindexed2, T>(imf, varparameters...)};
					U curg{indirectinput2<indirection1, indirection2, isindexed2, T>(img, varparameters...)};
					U curh{indirectinput2<indirection1, indirection2, isindexed2, T>(imh, varparameters...)};
					// register pressure performance issue on several platforms: do the low half here second
					filterinput<absolute, issigned, isfloatingpoint, T>(cure, curf, curg, curh);
					++offsets[cure];
					++offsets[curf];
					++offsets[curg];
					++offsets[curh];
				}else{
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
					auto imd{indirectinput1<indirection1, isindexed2, T, V>(pd, varparameters...)};
					auto ime{indirectinput1<indirection1, isindexed2, T, V>(pe, varparameters...)};
					auto imf{indirectinput1<indirection1, isindexed2, T, V>(pf, varparameters...)};
					auto img{indirectinput1<indirection1, isindexed2, T, V>(pg, varparameters...)};
					auto imh{indirectinput1<indirection1, isindexed2, T, V>(ph, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
					U curd{indirectinput2<indirection1, indirection2, isindexed2, T>(imd, varparameters...)};
					U cure{indirectinput2<indirection1, indirection2, isindexed2, T>(ime, varparameters...)};
					U curf{indirectinput2<indirection1, indirection2, isindexed2, T>(imf, varparameters...)};
					U curg{indirectinput2<indirection1, indirection2, isindexed2, T>(img, varparameters...)};
					U curh{indirectinput2<indirection1, indirection2, isindexed2, T>(imh, varparameters...)};
					if constexpr(absolute && isfloatingpoint){// one-register filters only
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd, cure, curf, curg, curh);
					}
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
					++offsets[cure];
					++offsets[curf];
					++offsets[curg];
					++offsets[curh];
				}
				i -= 8;
			}while(0 <= i);
			if(4 & i){// possibly finalize 4 entries after the loop above
				V *pa{input[i + 7]};
				V *pb{input[i + 6]};
				V *pc{input[i + 5]};
				V *pd{input[i + 4]};
				i -= 4;// required for the "if(2 & i){" part
				auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
				auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
				auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
				auto imd{indirectinput1<indirection1, isindexed2, T, V>(pd, varparameters...)};
				U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
				U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
				U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
				U curd{indirectinput2<indirection1, indirection2, isindexed2, T>(imd, varparameters...)};
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
				}
				++offsets[cura];
				++offsets[curb];
				++offsets[curc];
				++offsets[curd];
			}
			if(2 & i){// possibly finalize 2 entries after the loop above
				V *pa{input[i + 7]};
				V *pb{input[i + 6]};
				auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
				auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
				U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
				U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb);
				}
				++offsets[cura];
				++offsets[curb];
			}
			if(1 & i){// possibly finalize 1 entry after the loop above
				V *p{input[0]};
				auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
				U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(cur);
				}
				++offsets[cur];
			}
		}

		// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
		if(generateoffsetssingle<reversesort, absolute, issigned, isfloatingpoint, T>(count, offsets))
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
			[[likely]]
#endif
		{

			// perform the bidirectional 8-bit sorting sequence
			V *const *psrclo{input}, *const *psrchi{input + count};
			do{// fill the array, two at a time: one low-to-middle, one high-to-middle
				V *plo{*psrclo++};
				V *phi{*psrchi--};
				auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
				auto imhi{indirectinput1<indirection1, isindexed2, T, V>(phi, varparameters...)};
				auto outlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
				auto outhi{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
				auto[curlo, curhi]{filtertop8<absolute, issigned, isfloatingpoint, decltype(outlo), U>(outlo, outhi)};
				size_t offsetlo, offsethi;// this is only allowed for the single-part version, containing just one sorting pass
				if constexpr(reverseorder){
					offsetlo = offsets[curlo + offsetsstride]--;// the next item will be placed one lower
					offsethi = offsets[curhi]++;// the next item will be placed one higher
				}else{
					offsetlo = offsets[curlo]++;// the next item will be placed one higher
					offsethi = offsets[curhi + offsetsstride]--;// the next item will be placed one lower
				}
				output[offsetlo] = plo;
				output[offsethi] = phi;
			}while(psrclo < psrchi);
			if(psrclo == psrchi){// fill in the final item for odd counts
				V *plo{*psrclo};
				auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
				auto outlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
				size_t curlo{filtertop8<absolute, issigned, isfloatingpoint, decltype(outlo), U>(outlo)};
				size_t offsetlo;// this is only allowed for the single-part version, containing just one sorting pass
				if constexpr(reverseorder){
					offsetlo = offsets[curlo + offsetsstride];
				}else{
					offsetlo = offsets[curlo];
				}
				output[offsetlo] = plo;
			}
		}
	}
}

// radixsortnoalloc() function implementation template for single-part types with indirection
template<auto indirection1, bool reversesort, bool reverseorder, bool absolute, bool issigned, bool isfloatingpoint, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
RSBD8_FUNC_NORMAL std::enable_if_t<// disable this if the bool movetobuffer argument from a multi-part version is detected here, and do not allow active compile-time evaluation with it
	(std::is_member_function_pointer_v<decltype(indirection1)> ||
	std::is_member_object_pointer_v<decltype(indirection1)> &&
	std::is_unsigned_v<std::remove_pointer_t<std::remove_cvref_t<memberpointerdeducebody<indirection1, isindexed2, V, vararguments...>>>>) &&
	8 >= CHAR_BIT * sizeof(stripenum<std::remove_pointer_t<std::remove_cvref_t<memberpointerdeducebody<indirection1, isindexed2, V, vararguments...>>>>),
	void> radixsortnoallocsingle(size_t count, V *input[], V *buffer[], vararguments... varparameters)
	noexcept(std::is_member_object_pointer_v<decltype(indirection1)> ||
		std::is_nothrow_invocable_v<std::conditional_t<isindexed2, decltype(splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>){
	using T = tounifunsigned<std::remove_pointer_t<std::remove_cvref_t<memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>>;
	using U = std::conditional_t<sizeof(T) < sizeof(unsigned), unsigned, T>;// assume zero-extension to be basically free for U on basically all modern machines
	// do not pass a nullptr here, even though it's safe if count is 0
	assert(input);
	assert(buffer);
	// All the code in this function is adapted for count to be one below its input value here.
	--count;
	if(0 < static_cast<ptrdiff_t>(count)){// a 0 or 1 count array is legal here
		static size_t constexpr offsetsstride{CHAR_BIT * sizeof(T) * 256 / 8 - (absolute && issigned) * (127 + isfloatingpoint)};// shrink the offsets size if possible
		size_t offsets[offsetsstride * 2];// a sizeable amount of indices, but it's worth it, and this function never calls functions either to further increase stack usage anyway
		std::memset(offsets, 0, sizeof(offsets) / 2);// only the low half

		// count the 256 configurations, all in one go
		if constexpr(reverseorder){// also reverse the array at the same time
			V **pinputlo{input}, **pinputhi{input + count};
			V **pbufferlo{buffer}, **pbufferhi{buffer + count};
			if(4 & count + 1){// possibly initialize with 4 entries before the loop below
				V *pa{pinputlo[0]};
				V *pb{pinputhi[0]};
				V *pc{pinputlo[1]};
				V *pd{pinputhi[-1]};
				pinputhi[0] = pa;
				pbufferhi[0] = pa;
				auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
				pinputlo[0] = pb;
				pbufferlo[0] = pb;
				auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
				pinputhi[-1] = pc;
				pinputhi -= 2;
				pbufferhi[-1] = pc;
				pbufferhi -= 2;
				auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
				pinputlo[1] = pd;
				pinputlo += 2;
				pbufferlo[1] = pd;
				pbufferlo += 2;
				auto imd{indirectinput1<indirection1, isindexed2, T, V>(pd, varparameters...)};
				U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
				U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
				U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
				U curd{indirectinput2<indirection1, indirection2, isindexed2, T>(imd, varparameters...)};
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
				}
				++offsets[cura];
				++offsets[curb];
				++offsets[curc];
				++offsets[curd];
			}
			if(2 & count + 1){// possibly initialize with 2 entries before the loop below
				V *pa{*pinputlo};
				V *pb{*pinputhi};
				*pinputhi-- = pa;
				*pbufferhi-- = pa;
				auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
				*pinputlo++ = pb;
				*pbufferlo++ = pb;
				auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
				U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
				U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb);
				}
				++offsets[cura];
				++offsets[curb];
			}
			if(7 <= count)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
				[[likely]]
#endif
				do{
				V *pa{pinputlo[0]};
				V *pb{pinputhi[0]};
				V *pc{pinputlo[1]};
				V *pd{pinputhi[-1]};
				if constexpr(absolute != isfloatingpoint){// two-register filters only
					pinputhi[0] = pa;
					pbufferhi[0] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					pinputlo[0] = pb;
					pbufferlo[0] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					pinputhi[-1] = pc;
					pbufferhi[-1] = pc;
					auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
					pinputlo[1] = pd;
					pbufferlo[1] = pd;
					auto imd{indirectinput1<indirection1, isindexed2, T, V>(pd, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
					U curd{indirectinput2<indirection1, indirection2, isindexed2, T>(imd, varparameters...)};
					// register pressure performance issue on several platforms: first do the high half here
					filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
				}
				V *pe{pinputlo[2]};
				V *pf{pinputhi[-2]};
				V *pg{pinputlo[3]};
				V *ph{pinputhi[-3]};
				if constexpr(absolute != isfloatingpoint){// two-register filters only
					pinputhi[-2] = pe;
					pbufferhi[-2] = pe;
					auto ime{indirectinput1<indirection1, isindexed2, T, V>(pe, varparameters...)};
					pinputlo[2] = pf;
					pbufferlo[2] = pf;
					auto imf{indirectinput1<indirection1, isindexed2, T, V>(pf, varparameters...)};
					pinputhi[-3] = pg;
					pinputhi -= 4;
					pbufferhi[-3] = pg;
					pbufferhi -= 4;
					auto img{indirectinput1<indirection1, isindexed2, T, V>(pg, varparameters...)};
					pinputlo[3] = ph;
					pinputlo += 4;
					pbufferlo[3] = ph;
					pbufferlo += 4;
					auto imh{indirectinput1<indirection1, isindexed2, T, V>(ph, varparameters...)};
					U cure{indirectinput2<indirection1, indirection2, isindexed2, T>(ime, varparameters...)};
					U curf{indirectinput2<indirection1, indirection2, isindexed2, T>(imf, varparameters...)};
					U curg{indirectinput2<indirection1, indirection2, isindexed2, T>(img, varparameters...)};
					U curh{indirectinput2<indirection1, indirection2, isindexed2, T>(imh, varparameters...)};
					// register pressure performance issue on several platforms: do the low half here second
					filterinput<absolute, issigned, isfloatingpoint, T>(cure, curf, curg, curh);
					++offsets[cure];
					++offsets[curf];
					++offsets[curg];
					++offsets[curh];
				}else{
					pinputhi[0] = pa;
					pbufferhi[0] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					pinputlo[0] = pb;
					pbufferlo[0] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					pinputhi[-1] = pc;
					pbufferhi[-1] = pc;
					auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
					pinputlo[1] = pd;
					pbufferlo[1] = pd;
					auto imd{indirectinput1<indirection1, isindexed2, T, V>(pd, varparameters...)};
					pinputhi[-2] = pe;
					pbufferhi[-2] = pe;
					auto ime{indirectinput1<indirection1, isindexed2, T, V>(pe, varparameters...)};
					pinputlo[2] = pf;
					pbufferlo[2] = pf;
					auto imf{indirectinput1<indirection1, isindexed2, T, V>(pf, varparameters...)};
					pinputhi[-3] = pg;
					pinputhi -= 4;
					pbufferhi[-3] = pg;
					pbufferhi -= 4;
					auto img{indirectinput1<indirection1, isindexed2, T, V>(pg, varparameters...)};
					pinputlo[3] = ph;
					pinputlo += 4;
					pbufferlo[3] = ph;
					pbufferlo += 4;
					auto imh{indirectinput1<indirection1, isindexed2, T, V>(ph, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
					U curd{indirectinput2<indirection1, indirection2, isindexed2, T>(imd, varparameters...)};
					U cure{indirectinput2<indirection1, indirection2, isindexed2, T>(ime, varparameters...)};
					U curf{indirectinput2<indirection1, indirection2, isindexed2, T>(imf, varparameters...)};
					U curg{indirectinput2<indirection1, indirection2, isindexed2, T>(img, varparameters...)};
					U curh{indirectinput2<indirection1, indirection2, isindexed2, T>(imh, varparameters...)};
					if constexpr(absolute && isfloatingpoint){// one-register filters only
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd, cure, curf, curg, curh);
					}
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
					++offsets[cure];
					++offsets[curf];
					++offsets[curg];
					++offsets[curh];
				}
			}while(pinputlo < pinputhi);
			if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
				V *p{*pinputlo};
				// no write to input, as this is the midpoint
				*pbufferhi = p;
				auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
				U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(cur);
				}
				++offsets[cur];
			}
		}else{// not in reverse order
			ptrdiff_t i{static_cast<ptrdiff_t>(count) - 7};
			if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
				[[likely]]
#endif
				do{
				V *pa{input[i + 7]};
				V *pb{input[i + 6]};
				V *pc{input[i + 5]};
				V *pd{input[i + 4]};
				if constexpr(absolute != isfloatingpoint){// two-register filters only
					buffer[i + 7] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					buffer[i + 6] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					buffer[i + 5] = pc;
					auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
					buffer[i + 4] = pd;
					auto imd{indirectinput1<indirection1, isindexed2, T, V>(pd, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
					U curd{indirectinput2<indirection1, indirection2, isindexed2, T>(imd, varparameters...)};
					// register pressure performance issue on several platforms: first do the high half here
					filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
				}
				V *pe{input[i + 3]};
				V *pf{input[i + 2]};
				V *pg{input[i + 1]};
				V *ph{input[i]};
				if constexpr(absolute != isfloatingpoint){// two-register filters only
					buffer[i + 3] = pe;
					auto ime{indirectinput1<indirection1, isindexed2, T, V>(pe, varparameters...)};
					buffer[i + 2] = pf;
					auto imf{indirectinput1<indirection1, isindexed2, T, V>(pf, varparameters...)};
					buffer[i + 1] = pg;
					auto img{indirectinput1<indirection1, isindexed2, T, V>(pg, varparameters...)};
					buffer[i] = ph;
					auto imh{indirectinput1<indirection1, isindexed2, T, V>(ph, varparameters...)};
					U cure{indirectinput2<indirection1, indirection2, isindexed2, T>(ime, varparameters...)};
					U curf{indirectinput2<indirection1, indirection2, isindexed2, T>(imf, varparameters...)};
					U curg{indirectinput2<indirection1, indirection2, isindexed2, T>(img, varparameters...)};
					U curh{indirectinput2<indirection1, indirection2, isindexed2, T>(imh, varparameters...)};
					// register pressure performance issue on several platforms: do the low half here second
					filterinput<absolute, issigned, isfloatingpoint, T>(cure, curf, curg, curh);
					++offsets[cure];
					++offsets[curf];
					++offsets[curg];
					++offsets[curh];
				}else{
					buffer[i + 7] = pa;
					auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
					buffer[i + 6] = pb;
					auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
					buffer[i + 5] = pc;
					auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
					buffer[i + 4] = pd;
					auto imd{indirectinput1<indirection1, isindexed2, T, V>(pd, varparameters...)};
					buffer[i + 3] = pe;
					auto ime{indirectinput1<indirection1, isindexed2, T, V>(pe, varparameters...)};
					buffer[i + 2] = pf;
					auto imf{indirectinput1<indirection1, isindexed2, T, V>(pf, varparameters...)};
					buffer[i + 1] = pg;
					auto img{indirectinput1<indirection1, isindexed2, T, V>(pg, varparameters...)};
					buffer[i] = ph;
					auto imh{indirectinput1<indirection1, isindexed2, T, V>(ph, varparameters...)};
					U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
					U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
					U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
					U curd{indirectinput2<indirection1, indirection2, isindexed2, T>(imd, varparameters...)};
					U cure{indirectinput2<indirection1, indirection2, isindexed2, T>(ime, varparameters...)};
					U curf{indirectinput2<indirection1, indirection2, isindexed2, T>(imf, varparameters...)};
					U curg{indirectinput2<indirection1, indirection2, isindexed2, T>(img, varparameters...)};
					U curh{indirectinput2<indirection1, indirection2, isindexed2, T>(imh, varparameters...)};
					if constexpr(absolute && isfloatingpoint){// one-register filters only
						filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd, cure, curf, curg, curh);
					}
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
					++offsets[cure];
					++offsets[curf];
					++offsets[curg];
					++offsets[curh];
				}
				i -= 8;
			}while(0 <= i);
			if(4 & i){// possibly finalize 4 entries after the loop above
				V *pa{input[i + 7]};
				V *pb{input[i + 6]};
				V *pc{input[i + 5]};
				V *pd{input[i + 4]};
				buffer[i + 7] = pa;
				auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
				buffer[i + 6] = pb;
				auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
				buffer[i + 5] = pc;
				auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
				buffer[i + 4] = pd;
				i -= 4;// required for the "if(2 & i){" part
				auto imd{indirectinput1<indirection1, isindexed2, T, V>(pd, varparameters...)};
				U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
				U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
				U curc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
				U curd{indirectinput2<indirection1, indirection2, isindexed2, T>(imd, varparameters...)};
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
				}
				++offsets[cura];
				++offsets[curb];
				++offsets[curc];
				++offsets[curd];
			}
			if(2 & i){// possibly finalize 2 entries after the loop above
				V *pa{input[i + 7]};
				V *pb{input[i + 6]};
				buffer[i + 7] = pa;
				auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
				buffer[i + 6] = pb;
				auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
				U cura{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
				U curb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(cura, curb);
				}
				++offsets[cura];
				++offsets[curb];
			}
			if(1 & i){// possibly finalize 1 entry after the loop above
				V *p{input[0]};
				buffer[0] = p;
				auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
				U cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
				if constexpr(absolute || isfloatingpoint){
					filterinput<absolute, issigned, isfloatingpoint, T>(cur);
				}
				++offsets[cur];
			}
		}

		// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
		if(generateoffsetssingle<reversesort, absolute, issigned, isfloatingpoint, T>(count, offsets))
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
			[[likely]]
#endif
		{

			// perform the bidirectional 8-bit sorting sequence
			V **psrclo{buffer}, **psrchi{buffer + count};
			do{// fill the array, two at a time: one low-to-middle, one high-to-middle
				V *plo{*psrclo++};
				V *phi{*psrchi--};
				auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
				auto imhi{indirectinput1<indirection1, isindexed2, T, V>(phi, varparameters...)};
				auto outlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
				auto outhi{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
				auto[curlo, curhi]{filtertop8<absolute, issigned, isfloatingpoint, decltype(outlo), U>(outlo, outhi)};
				size_t offsetlo, offsethi;// this is only allowed for the single-part version, containing just one sorting pass
				if constexpr(reverseorder){
					offsetlo = offsets[curlo + offsetsstride]--;// the next item will be placed one lower
					offsethi = offsets[curhi]++;// the next item will be placed one higher
				}else{
					offsetlo = offsets[curlo]++;// the next item will be placed one higher
					offsethi = offsets[curhi + offsetsstride]--;// the next item will be placed one lower
				}
				input[offsetlo] = plo;
				input[offsethi] = phi;
			}while(psrclo < psrchi);
			if(psrclo == psrchi){// fill in the final item for odd counts
				V *plo{*psrclo};
				auto imlo{indirectinput1<indirection1, isindexed2, T, V>(plo, varparameters...)};
				auto outlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
				size_t curlo{filtertop8<absolute, issigned, isfloatingpoint, decltype(outlo), U>(outlo)};
				size_t offsetlo;// this is only allowed for the single-part version, containing just one sorting pass
				if constexpr(reverseorder){
					offsetlo = offsets[curlo + offsetsstride];
				}else{
					offsetlo = offsets[curlo];
				}
				input[offsetlo] = plo;
			}
		}
	}
}

}// namespace helper

// Definition of the GetOffsetOf template
// Temporary, until a revision of "offsetof" is standardized in C++ with constexpr.
// This part isn't used internally, but serves as a tool to the user for calculating compile-time offsets.
// Section start of all rights reserved for the respective author (Sulley, 2024-06-15):
// https://sulley.cc/2024/06/15/16/18/

template <typename T>
struct ClassMemberTraits;

template <typename C, typename M>
struct ClassMemberTraits<M C::*>
{
	using ClassType = C;
	using MemberType = M;
};

#pragma pack(push, 1)
template<typename M, std::size_t Offset>
struct MemberAt
{
	char padding[Offset];
	M member;
};
#pragma pack(pop)

template<typename M>
struct MemberAt<M, 0>
{
	M member;
};

template<typename B, typename M, std::size_t Offset>
union PaddedUnion
{
	char c;
	B base;
	MemberAt<M, Offset> member;
};

// ~~~~~ Begin core modification ~~~~~
template <
	auto MemberPtr,
	typename B,
	std::size_t Low,
	std::size_t High,
	std::size_t Mid = (Low + High) / 2>
struct OffsetHelper
{
	using M = ClassMemberTraits<decltype(MemberPtr)>::MemberType;

	constexpr static PaddedUnion<B, M, Mid> dummy{};
	constexpr static std::size_t GetOffsetOf()
	{
		if constexpr (&(dummy.base.*MemberPtr) > &dummy.member.member)
		{
			return OffsetHelper<MemberPtr, B, Mid + 1, High>::GetOffsetOf();
		}
		else if constexpr (&(dummy.base.*MemberPtr) < &dummy.member.member)
		{
			return OffsetHelper<MemberPtr, B, Low, Mid>::GetOffsetOf();
		}
		else
		{
			return Mid;
		}
	}
};
// ~~~~~ End core modification ~~~~~

template <auto MemberPtr, typename B>
constexpr std::size_t GetOffsetOf = OffsetHelper<MemberPtr, B, 0, sizeof(B)>::GetOffsetOf();
// Section end

// Generic large array allocation and deallocation functions

template<typename T>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
#if !defined(_WIN32) && defined(_POSIX_C_SOURCE)// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
RSBD8_FUNC_INLINE std::pair<T *, size_t>
#else
RSBD8_FUNC_INLINE T *
#endif
	allocatearray(size_t count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
	, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
	, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
	)noexcept{
	size_t allocsize{count * sizeof(T)};
#ifdef _WIN32
	assert(!(largepagesize - 1 & largepagesize));// a maximum of one bit should be set in the value of largepagesize
	size_t largeallocsize{(largepagesize - 1 & -static_cast<ptrdiff_t>(allocsize)) + allocsize};// round up to the nearest multiple of largepagesize
	DWORD alloctype{MEM_RESERVE | MEM_COMMIT};
	DWORD largealloctype{MEM_LARGE_PAGES | MEM_RESERVE | MEM_COMMIT};
	if(largepagesize) allocsize = largeallocsize, alloctype = largealloctype;
	T *buffer{reinterpret_cast<T *>(VirtualAlloc(nullptr, allocsize, alloctype, PAGE_READWRITE))};
	return{buffer};
#elif defined(_POSIX_C_SOURCE)
	void *pempty{};
#ifdef MAP_HUGETLB
	if(MAP_HUGETLB & mmapflags){// use the 6 bits associated with the huge TLB functionality
		size_t pagesize{static_cast<size_t>(1) << (static_cast<unsigned>(mmapflags) >> MAP_HUGE_SHIFT & ((1 << 6) - 1))};
		allocsize = (pagesize - 1 & -static_cast<ptrdiff_t>(allocsize)) + allocsize};// round up to the nearest multiple of pagesize
#ifdef __ia64__// Only IA64 requires this part for this type of allocation
		pempty = reinterpret_cast<void *>(0x8000000000000000UL);
#endif
	}
#endif
	T *buffer{reinterpret_cast<T *>(mmap(pempty, allocsize, PROT_READ | PROT_WRITE, mmapflags, -1, 0))};
	return{buffer, allocsize};
#else
	T *buffer{new(std::nothrow) T[allocsize]};
	return{buffer};
#endif
}

template<typename T>
RSBD8_FUNC_INLINE void deallocatearray(T *buffer
#if !defined(_WIN32) && defined(_POSIX_C_SOURCE)// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
	, size_t allocsize
#endif
	)noexcept{
#ifdef _WIN32
	BOOL boVirtualFree{VirtualFree(buffer, 0, MEM_RELEASE)};
	static_cast<void>(boVirtualFree);
	assert(boVirtualFree);
#elif defined(_POSIX_C_SOURCE)
	int imunmap{munmap(buffer, allocsize);
	static_cast<void>(imunmap);
	assert(!imunmap);
#else
	delete[] buffer;
#endif
}

// This class is a simple RAII wrapper for the buffer memory allocated with allocatearray().
template<typename T>
struct buffermemorywrapper{
	T *ptr;
#if defined(_POSIX_C_SOURCE)
	size_t size;
#endif
	RSBD8_FUNC_INLINE ~buffermemorywrapper(){
		deallocatearray(ptr
#if defined(_POSIX_C_SOURCE)
			, size
#endif
		);};
	RSBD8_FUNC_INLINE buffermemorywrapper(T *ptrmem
#if defined(_POSIX_C_SOURCE)
		, sizemem
#endif
		)noexcept : ptr(ptrmem)
#if defined(_POSIX_C_SOURCE)
		, size(sizemem)
#endif
	{};
};

// Wrapper template functions for the 4 main sorting functions in this library

// Wrapper for the multi-part radixsortcopynoalloc() function without indirection
template<sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	!std::is_pointer_v<T> &&
	128 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, T const input[], T output[], T buffer[])noexcept{
	static bool constexpr reversesort{static_cast<bool>(1 & direction)};
	static bool constexpr reverseorder{static_cast<bool>(1 << 1 & direction)};
	static bool constexpr absolute{
		(nativeabsmode <= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 & mode))};
	static bool constexpr issigned{
		(nativemode <= mode && nativeabsmode >= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 1 & mode))};
	static bool constexpr isfloatingpoint{
		(nativemode <= mode && std::is_floating_point_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 2 & mode))};
	using U = helper::tounifunsigned<T>;
	helper::radixsortcopynoallocmulti<reversesort, reverseorder, absolute, issigned, isfloatingpoint, U>(count, reinterpret_cast<U const *>(input), reinterpret_cast<U *>(output), reinterpret_cast<U *>(buffer));
}

// Wrapper for the multi-part radixsortnoalloc() function without indirection
template<sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	!std::is_pointer_v<T> &&
	128 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, T input[], T buffer[], bool movetobuffer = false)noexcept{
	static bool constexpr reversesort{static_cast<bool>(1 & direction)};
	static bool constexpr reverseorder{static_cast<bool>(1 << 1 & direction)};
	static bool constexpr absolute{
		(nativeabsmode <= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 & mode))};
	static bool constexpr issigned{
		(nativemode <= mode && nativeabsmode >= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 1 & mode))};
	static bool constexpr isfloatingpoint{
		(nativemode <= mode && std::is_floating_point_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 2 & mode))};
	using U = helper::tounifunsigned<T>;
	helper::radixsortnoallocmulti<reversesort, reverseorder, absolute, issigned, isfloatingpoint, U>(count, reinterpret_cast<U *>(input), reinterpret_cast<U *>(buffer), movetobuffer);
}

// Wrapper for the single-part radixsortcopynoalloc() function without indirection
template<sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	!std::is_pointer_v<T> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, T const input[], T output[])noexcept{
	static bool constexpr reversesort{static_cast<bool>(1 & direction)};
	static bool constexpr reverseorder{static_cast<bool>(1 << 1 & direction)};
	static bool constexpr absolute{
		(nativeabsmode <= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 & mode))};
	static bool constexpr issigned{
		(nativemode <= mode && nativeabsmode >= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 1 & mode))};
	static bool constexpr isfloatingpoint{
		(nativemode <= mode && std::is_floating_point_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 2 & mode))};
	using U = helper::tounifunsigned<T>;
	helper::radixsortcopynoallocsingle<reversesort, reverseorder, absolute, issigned, isfloatingpoint, U>(count, reinterpret_cast<U const *>(input), reinterpret_cast<U *>(output));
}

// Wrapper for the single-part radixsortcopynoalloc() function without indirection with a dummy buffer argument
template<sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	!std::is_pointer_v<T> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, T const input[], T output[], T buffer[])noexcept{
	static_cast<void>(buffer);// the single-part version never needs an extra buffer
	radixsortcopynoalloc<direction, mode, T>(count, input, output);
}

// Wrapper for the single-part radixsortnoalloc() function without indirection
template<sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	!std::is_pointer_v<T> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, T input[], T buffer[])noexcept{
	static bool constexpr reversesort{static_cast<bool>(1 & direction)};
	static bool constexpr reverseorder{static_cast<bool>(1 << 1 & direction)};
	static bool constexpr absolute{
		(nativeabsmode <= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 & mode))};
	static bool constexpr issigned{
		(nativemode <= mode && nativeabsmode >= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 1 & mode))};
	static bool constexpr isfloatingpoint{
		(nativemode <= mode && std::is_floating_point_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 2 & mode))};
	using U = helper::tounifunsigned<T>;
	helper::radixsortnoallocsingle<reversesort, reverseorder, absolute, issigned, isfloatingpoint, U>(count, reinterpret_cast<U *>(input), reinterpret_cast<U *>(buffer));
}

// Wrapper for the single-part radixsortnoalloc() and radixsortcopynoalloc() functions without indirection
// This variant does not set the default "false" for the "movetobuffer" parameter.
template<sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	!std::is_pointer_v<T> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, T input[], T buffer[], bool movetobuffer)noexcept{
	static bool constexpr reversesort{static_cast<bool>(1 & direction)};
	static bool constexpr reverseorder{static_cast<bool>(1 << 1 & direction)};
	static bool constexpr absolute{
		(nativeabsmode <= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 & mode))};
	static bool constexpr issigned{
		(nativemode <= mode && nativeabsmode >= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 1 & mode))};
	static bool constexpr isfloatingpoint{
		(nativemode <= mode && std::is_floating_point_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 2 & mode))};
	using U = helper::tounifunsigned<T>;
	if(!movetobuffer) helper::radixsortnoallocsingle<reversesort, reverseorder, absolute, issigned, isfloatingpoint, U>(count, reinterpret_cast<U *>(input), reinterpret_cast<U *>(buffer));
	else helper::radixsortcopynoallocsingle<reversesort, reverseorder, absolute, issigned, isfloatingpoint, U>(count, reinterpret_cast<U *>(input), reinterpret_cast<U *>(buffer));
}

// Wrapper to implement the radixsort() function without indirection, which only allocates some memory prior to sorting arrays
// This requires no specialisation for handling the single-part types.
template<sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, typename T>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	!std::is_pointer_v<T> &&
	128 >= CHAR_BIT * sizeof(T),
	bool> radixsort(size_t count, T input[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		)noexcept{
	auto
#if defined(_POSIX_C_SOURCE)
		[buffer, allocsize]
#else
		buffer
#endif
		{allocatearray<T>(count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, largepagesize
#elif defined(_POSIX_C_SOURCE)
		, mmapflags
#endif
		)};
	if(buffer){
		radixsortnoalloc<direction, mode, T>(count, input, buffer);// last parameter not filled in on purpose
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

// Wrapper to implement the multi-part radixsortcopy() function without indirection, which only allocates some memory prior to sorting arrays
template<sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, typename T>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	!std::is_pointer_v<T> &&
	128 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	bool> radixsortcopy(size_t count, T const input[], T output[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		)noexcept{
	auto
#if defined(_POSIX_C_SOURCE)
		[buffer, allocsize]
#else
		buffer
#endif
		{allocatearray<T>(count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, largepagesize
#elif defined(_POSIX_C_SOURCE)
		, mmapflags
#endif
		)};
	if(buffer){
		radixsortcopynoalloc<direction, mode, T>(count, input, output, buffer);
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

// Wrapper to implement the single-part radixsortcopy() function without indirection, which only allocates some memory prior to sorting arrays
template<sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, typename T>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	!std::is_pointer_v<T> &&
	8 >= CHAR_BIT * sizeof(T),
	bool> radixsortcopy(size_t count, T const input[], T output[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		)noexcept{
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
	assert(!(largepagesize - 1 & largepagesize));// a maximum of one bit should be set in the value of largepagesize
	static_cast<void>(largepagesize);
#elif defined(_POSIX_C_SOURCE)
	static_cast<void>(mmapflags);
#endif
	// the single-part version never needs an extra buffer
	radixsortcopynoalloc<direction, mode, T>(count, input, output);
	return{true};
}

// Wrapper for the multi-part radixsortcopynoalloc() function with simple first-level indirection
template<sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection1 = 0, typename T, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	!std::is_pointer_v<T> &&
	128 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, T const *const input[], T const *output[], T const *buffer[], vararguments... varparameters)noexcept{
	static bool constexpr reversesort{static_cast<bool>(1 & direction)};
	static bool constexpr reverseorder{static_cast<bool>(1 << 1 & direction)};
	static bool constexpr absolute{
		(nativeabsmode <= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 & mode))};
	static bool constexpr issigned{
		(nativemode <= mode && nativeabsmode >= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 1 & mode))};
	static bool constexpr isfloatingpoint{
		(nativemode <= mode && std::is_floating_point_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 2 & mode))};
	using U = helper::tounifunsigned<T>;
	using V = helper::memberobjectgenerator<U, indirection1>;
	helper::radixsortcopynoallocmulti<&V::object, reversesort, reverseorder, absolute, issigned, isfloatingpoint>(count, reinterpret_cast<V const *const *>(input), reinterpret_cast<V const **>(output), reinterpret_cast<V const **>(buffer), varparameters...);
}

// Wrapper for the multi-part radixsortnoalloc() function with simple first-level indirection
template<sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection1 = 0, typename T, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	!std::is_pointer_v<T> &&
	128 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, T const *input[], T const *buffer[], bool movetobuffer = false, vararguments... varparameters)noexcept{
	static bool constexpr reversesort{static_cast<bool>(1 & direction)};
	static bool constexpr reverseorder{static_cast<bool>(1 << 1 & direction)};
	static bool constexpr absolute{
		(nativeabsmode <= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 & mode))};
	static bool constexpr issigned{
		(nativemode <= mode && nativeabsmode >= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 1 & mode))};
	static bool constexpr isfloatingpoint{
		(nativemode <= mode && std::is_floating_point_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 2 & mode))};
	using U = helper::tounifunsigned<T>;
	using V = helper::memberobjectgenerator<U, indirection1>;
	helper::radixsortnoallocmulti<&V::object, reversesort, reverseorder, absolute, issigned, isfloatingpoint>(count, reinterpret_cast<V const **>(input), reinterpret_cast<V const **>(buffer), movetobuffer, varparameters...);
}

// Wrapper for the single-part radixsortcopynoalloc() function with simple first-level indirection
template<sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection1 = 0, typename T, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	!std::is_pointer_v<T> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, T const *const input[], T const *output[], vararguments... varparameters)noexcept{
	static bool constexpr reversesort{static_cast<bool>(1 & direction)};
	static bool constexpr reverseorder{static_cast<bool>(1 << 1 & direction)};
	static bool constexpr absolute{
		(nativeabsmode <= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 & mode))};
	static bool constexpr issigned{
		(nativemode <= mode && nativeabsmode >= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 1 & mode))};
	static bool constexpr isfloatingpoint{
		(nativemode <= mode && std::is_floating_point_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 2 & mode))};
	using U = helper::tounifunsigned<T>;
	using V = helper::memberobjectgenerator<U, indirection1>;
	helper::radixsortcopynoallocsingle<&V::object, reversesort, reverseorder, absolute, issigned, isfloatingpoint>(count, reinterpret_cast<V const *const *>(input), reinterpret_cast<V const **>(output), varparameters...);
}

// Wrapper for the single-part radixsortcopynoalloc() function with simple first-level indirection with a dummy buffer argument
template<sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection1 = 0, typename T, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	!std::is_pointer_v<T> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, T const *const input[], T const *output[], T const *buffer[], vararguments... varparameters)noexcept{
	static_cast<void>(buffer);// the single-part version never needs an extra buffer
	radixsortcopynoalloc<direction, mode, indirection1, T>(count, input, output, varparameters...);
}

// Wrapper for the single-part radixsortnoalloc() function with simple first-level indirection
template<sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection1 = 0, typename T, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	!std::is_pointer_v<T> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, T const *input[], T const *buffer[], vararguments... varparameters)noexcept{
	static bool constexpr reversesort{static_cast<bool>(1 & direction)};
	static bool constexpr reverseorder{static_cast<bool>(1 << 1 & direction)};
	static bool constexpr absolute{
		(nativeabsmode <= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 & mode))};
	static bool constexpr issigned{
		(nativemode <= mode && nativeabsmode >= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 1 & mode))};
	static bool constexpr isfloatingpoint{
		(nativemode <= mode && std::is_floating_point_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 2 & mode))};
	using U = helper::tounifunsigned<T>;
	using V = helper::memberobjectgenerator<U, indirection1>;
	helper::radixsortnoallocsingle<&V::object, reversesort, reverseorder, absolute, issigned, isfloatingpoint>(count, reinterpret_cast<V const **>(input), reinterpret_cast<V const **>(buffer), varparameters...);
}

// Wrapper for the single-part radixsortnoalloc() and radixsortcopynoalloc() functions with simple first-level indirection
// This variant does not set the default "false" for the "movetobuffer" parameter.
template<sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection1 = 0, typename T, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	!std::is_pointer_v<T> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, T const *input[], T const *buffer[], bool movetobuffer, vararguments... varparameters)noexcept{
	static bool constexpr reversesort{static_cast<bool>(1 & direction)};
	static bool constexpr reverseorder{static_cast<bool>(1 << 1 & direction)};
	static bool constexpr absolute{
		(nativeabsmode <= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 & mode))};
	static bool constexpr issigned{
		(nativemode <= mode && nativeabsmode >= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 1 & mode))};
	static bool constexpr isfloatingpoint{
		(nativemode <= mode && std::is_floating_point_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 2 & mode))};
	using U = helper::tounifunsigned<T>;
	using V = helper::memberobjectgenerator<U, indirection1>;
	if(!movetobuffer) helper::radixsortnoallocsingle<&V::object, reversesort, reverseorder, absolute, issigned, isfloatingpoint>(count, reinterpret_cast<V const **>(input), reinterpret_cast<V const **>(buffer), varparameters...);
	else helper::radixsortcopynoallocsingle<&V::object, reversesort, reverseorder, absolute, issigned, isfloatingpoint>(count, reinterpret_cast<V const **>(input), reinterpret_cast<V const **>(buffer), varparameters...);
}

// Wrapper to implement the radixsort() function with simple first-level indirection, which only allocates some memory prior to sorting arrays
// This requires no specialisation for handling the single-part types.
template<sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection1 = 0, typename T, typename... vararguments>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	!std::is_pointer_v<T> &&
	128 >= CHAR_BIT * sizeof(T),
	bool> radixsort(size_t count, T input[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		, vararguments... varparameters)noexcept{
	auto
#if defined(_POSIX_C_SOURCE)
		[buffer, allocsize]
#else
		buffer
#endif
		{allocatearray<T const *>(count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, largepagesize
#elif defined(_POSIX_C_SOURCE)
		, mmapflags
#endif
		)};
	if(buffer){
		radixsortnoalloc<direction, mode, indirection1, T>(count, input, buffer, false, varparameters...);
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

// Wrapper to implement the multi-part radixsortcopy() function with simple first-level indirection, which only allocates some memory prior to sorting arrays
template<sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection1 = 0, typename T, typename... vararguments>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	!std::is_pointer_v<T> &&
	128 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	bool> radixsortcopy(size_t count, T const *const input[], T const *output[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		, vararguments... varparameters)noexcept{
	auto
#if defined(_POSIX_C_SOURCE)
		[buffer, allocsize]
#else
		buffer
#endif
		{allocatearray<T const *>(count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, largepagesize
#elif defined(_POSIX_C_SOURCE)
		, mmapflags
#endif
		)};
	if(buffer){
		radixsortcopynoalloc<direction, mode, indirection1, T>(count, input, output, buffer, varparameters...);
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

// Wrapper to implement the single-part radixsortcopy() function with simple first-level indirection, which only allocates some memory prior to sorting arrays
template<sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection1 = 0, typename T, typename... vararguments>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	!std::is_pointer_v<T> &&
	8 >= CHAR_BIT * sizeof(T),
	bool> radixsortcopy(size_t count, T const *const input[], T const *output[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		, vararguments... varparameters)noexcept{
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
	assert(!(largepagesize - 1 & largepagesize));// a maximum of one bit should be set in the value of largepagesize
	static_cast<void>(largepagesize);
#elif defined(_POSIX_C_SOURCE)
	static_cast<void>(mmapflags);
#endif
	// the single-part version never needs an extra buffer
	radixsortcopynoalloc<direction, mode, indirection1, T>(count, input, output, varparameters...);
	return{true};
}

// Wrapper for the multi-part radixsortcopynoalloc() function with indirection
template<auto indirection1, sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_pointer_v<decltype(indirection1)> &&
	128 >= CHAR_BIT * sizeof(helper::stripenum<std::remove_pointer_t<std::remove_cvref_t<helper::memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>>) &&
	8 < CHAR_BIT * sizeof(helper::stripenum<std::remove_pointer_t<std::remove_cvref_t<helper::memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[], vararguments... varparameters)noexcept(
	std::is_member_object_pointer_v<decltype(indirection1)> ||
	std::is_nothrow_invocable_v<std::conditional_t<isindexed2, decltype(helper::splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>){
	using W = std::remove_cvref_t<helper::memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>;
	using T = helper::stripenum<std::remove_pointer_t<W>>;
	static_assert(!std::is_pointer_v<T>, "third level indirection is not supported");
	static bool constexpr reversesort{static_cast<bool>(1 & direction)};
	static bool constexpr reverseorder{static_cast<bool>(1 << 1 & direction)};
	static bool constexpr absolute{
		(nativeabsmode <= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 & mode))};
	static bool constexpr issigned{
		(nativemode <= mode && nativeabsmode >= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 1 & mode))};
	static bool constexpr isfloatingpoint{
		(nativemode <= mode && std::is_floating_point_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 2 & mode))};
	if constexpr(std::is_member_function_pointer_v<decltype(indirection1)>){
		helper::radixsortcopynoallocmulti<indirection1, reversesort, reverseorder, absolute, issigned, isfloatingpoint, indirection2, isindexed2, V>(count, input, output, buffer, varparameters...);
	}else{
		using U = helper::tounifunsigned<T>;
		helper::radixsortcopynoallocmulti<reinterpret_cast<std::conditional_t<std::is_pointer_v<W>, U const *(V:: *), U(V:: *)>>(indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, indirection2, isindexed2, V>(count, input, output, buffer, varparameters...);
	}
}

// Wrapper for the multi-part radixsortnoalloc() function with indirection
template<auto indirection1, sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_pointer_v<decltype(indirection1)> &&
	128 >= CHAR_BIT * sizeof(helper::stripenum<std::remove_pointer_t<std::remove_cvref_t<helper::memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>>) &&
	8 < CHAR_BIT * sizeof(helper::stripenum<std::remove_pointer_t<std::remove_cvref_t<helper::memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer = false, vararguments... varparameters)noexcept(
	std::is_member_object_pointer_v<decltype(indirection1)> ||
	std::is_nothrow_invocable_v<std::conditional_t<isindexed2, decltype(helper::splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>){
	using W = std::remove_cvref_t<helper::memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>;
	using T = helper::stripenum<std::remove_pointer_t<W>>;
	static_assert(!std::is_pointer_v<T>, "third level indirection is not supported");
	static bool constexpr reversesort{static_cast<bool>(1 & direction)};
	static bool constexpr reverseorder{static_cast<bool>(1 << 1 & direction)};
	static bool constexpr absolute{
		(nativeabsmode <= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 & mode))};
	static bool constexpr issigned{
		(nativemode <= mode && nativeabsmode >= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 1 & mode))};
	static bool constexpr isfloatingpoint{
		(nativemode <= mode && std::is_floating_point_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 2 & mode))};
	if constexpr(std::is_member_function_pointer_v<decltype(indirection1)>){
		helper::radixsortnoallocmulti<indirection1, reversesort, reverseorder, absolute, issigned, isfloatingpoint, indirection2, isindexed2, V>(count, input, buffer, movetobuffer, varparameters...);
	}else{
		using U = helper::tounifunsigned<T>;
		helper::radixsortnoallocmulti<reinterpret_cast<std::conditional_t<std::is_pointer_v<W>, U const *(V:: *), U(V:: *)>>(indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, indirection2, isindexed2, V>(count, input, buffer, movetobuffer, varparameters...);
	}
}

// Wrapper for the single-part radixsortcopynoalloc() function with indirection
template<auto indirection1, sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<// disable the option for with the V *buffer[] argument here, and do not allow active compile-time template evaluation with it
	!std::is_same_v<V **, std::conditional_t<0 < sizeof...(vararguments),
		std::invoke_result_t<decltype(helper::splitparameter<vararguments...>), vararguments...>, void>> &&
	std::is_member_pointer_v<decltype(indirection1)> &&
	8 >= CHAR_BIT * sizeof(helper::stripenum<std::remove_pointer_t<std::remove_cvref_t<
		typename std::enable_if<!std::is_same_v<V **, std::conditional_t<0 < sizeof...(vararguments),
			std::invoke_result_t<decltype(helper::splitparameter<vararguments...>), vararguments...>, void>>,
			helper::memberpointerdeducebody<indirection1, isindexed2, V, vararguments...>>::type>>>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], vararguments... varparameters)noexcept(
	std::is_member_object_pointer_v<decltype(indirection1)> ||
	std::is_nothrow_invocable_v<std::conditional_t<isindexed2, decltype(helper::splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>){
	using W = std::remove_cvref_t<helper::memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>;
	using T = helper::stripenum<std::remove_pointer_t<W>>;
	static_assert(!std::is_pointer_v<T>, "third level indirection is not supported");
	static bool constexpr reversesort{static_cast<bool>(1 & direction)};
	static bool constexpr reverseorder{static_cast<bool>(1 << 1 & direction)};
	static bool constexpr absolute{
		(nativeabsmode <= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 & mode))};
	static bool constexpr issigned{
		(nativemode <= mode && nativeabsmode >= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 1 & mode))};
	static bool constexpr isfloatingpoint{
		(nativemode <= mode && std::is_floating_point_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 2 & mode))};
	if constexpr(std::is_member_function_pointer_v<decltype(indirection1)>){
		helper::radixsortcopynoallocsingle<indirection1, reversesort, reverseorder, absolute, issigned, isfloatingpoint, indirection2, isindexed2, V>(count, input, output, varparameters...);
	}else{
		using U = helper::tounifunsigned<T>;
		helper::radixsortcopynoallocsingle<reinterpret_cast<std::conditional_t<std::is_pointer_v<W>, U const *(V:: *), U(V:: *)>>(indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, indirection2, isindexed2, V>(count, input, output, varparameters...);
	}
}

// Wrapper for the single-part radixsortcopynoalloc() function with indirection with a dummy buffer argument
template<auto indirection1, sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_pointer_v<decltype(indirection1)> &&
	8 >= CHAR_BIT * sizeof(helper::stripenum<std::remove_pointer_t<std::remove_cvref_t<helper::memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[], vararguments... varparameters)noexcept(
	std::is_member_object_pointer_v<decltype(indirection1)> ||
	std::is_nothrow_invocable_v<std::conditional_t<isindexed2, decltype(helper::splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>){
	static_cast<void>(buffer);// the single-part version never needs an extra buffer
	radixsortcopynoallocsingle<indirection1, direction, mode, indirection2, isindexed2, V>(count, input, output, varparameters);
}

// Wrapper for the single-part radixsortnoalloc() function with indirection
template<auto indirection1, sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<// disable the option for with the bool movetobuffer argument here, and do not allow active compile-time template evaluation with it
	!std::is_same_v<bool, std::conditional_t<0 < sizeof...(vararguments),
		std::invoke_result_t<decltype(helper::splitparameter<vararguments...>), vararguments...>, void>> &&
	std::is_member_pointer_v<decltype(indirection1)> &&
	8 >= CHAR_BIT * sizeof(helper::stripenum<std::remove_pointer_t<std::remove_cvref_t<
		typename std::enable_if<!std::is_same_v<bool, std::conditional_t<0 < sizeof...(vararguments),
			std::invoke_result_t<decltype(helper::splitparameter<vararguments...>), vararguments...>, void>>,
			helper::memberpointerdeducebody<indirection1, isindexed2, V, vararguments...>>::type>>>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], vararguments... varparameters)noexcept(
	std::is_member_object_pointer_v<decltype(indirection1)> ||
	std::is_nothrow_invocable_v<std::conditional_t<isindexed2, decltype(helper::splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>){
	using W = std::remove_cvref_t<helper::memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>;
	using T = helper::stripenum<std::remove_pointer_t<W>>;
	static_assert(!std::is_pointer_v<T>, "third level indirection is not supported");
	static bool constexpr reversesort{static_cast<bool>(1 & direction)};
	static bool constexpr reverseorder{static_cast<bool>(1 << 1 & direction)};
	static bool constexpr absolute{
		(nativeabsmode <= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 & mode))};
	static bool constexpr issigned{
		(nativemode <= mode && nativeabsmode >= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 1 & mode))};
	static bool constexpr isfloatingpoint{
		(nativemode <= mode && std::is_floating_point_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 2 & mode))};
	if constexpr(std::is_member_function_pointer_v<decltype(indirection1)>){
		helper::radixsortnoallocsingle<indirection1, reversesort, reverseorder, absolute, issigned, isfloatingpoint, indirection2, isindexed2, V>(count, input, buffer, varparameters...);
	}else{
		using U = helper::tounifunsigned<T>;
		helper::radixsortnoallocsingle<reinterpret_cast<std::conditional_t<std::is_pointer_v<W>, U const *(V:: *), U(V:: *)>>(indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, indirection2, isindexed2, V>(count, input, buffer, varparameters...);
	}
}

// Wrapper for the single-part radixsortnoalloc() and radixsortcopynoalloc() functions with indirection
// This variant does not set the default "false" for the "movetobuffer" parameter.
template<auto indirection1, sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_pointer_v<decltype(indirection1)> &&
	8 >= CHAR_BIT * sizeof(helper::stripenum<std::remove_pointer_t<std::remove_cvref_t<helper::memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer, vararguments... varparameters)noexcept(
	std::is_member_object_pointer_v<decltype(indirection1)> ||
	std::is_nothrow_invocable_v<std::conditional_t<isindexed2, decltype(helper::splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>){
	using W = std::remove_cvref_t<helper::memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>;
	using T = helper::stripenum<std::remove_pointer_t<W>>;
	static_assert(!std::is_pointer_v<T>, "third level indirection is not supported");
	static bool constexpr reversesort{static_cast<bool>(1 & direction)};
	static bool constexpr reverseorder{static_cast<bool>(1 << 1 & direction)};
	static bool constexpr absolute{
		(nativeabsmode <= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 & mode))};
	static bool constexpr issigned{
		(nativemode <= mode && nativeabsmode >= mode && std::is_signed_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 1 & mode))};
	static bool constexpr isfloatingpoint{
		(nativemode <= mode && std::is_floating_point_v<T>) ||
		(nativemode > mode && static_cast<bool>(1 << 2 & mode))};
	using U = helper::tounifunsigned<T>;
	if(!movetobuffer){
		if constexpr(std::is_member_function_pointer_v<decltype(indirection1)>){
			helper::radixsortnoallocsingle<indirection1, reversesort, reverseorder, absolute, issigned, isfloatingpoint, indirection2, isindexed2, V>(count, input, buffer, varparameters...);
		}else{
			helper::radixsortnoallocsingle<reinterpret_cast<std::conditional_t<std::is_pointer_v<W>, U const *(V:: *), U(V:: *)>>(indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, indirection2, isindexed2, V>(count, input, buffer, varparameters...);
		}
	}else{
		if constexpr(std::is_member_function_pointer_v<decltype(indirection1)>){
			helper::radixsortcopynoallocsingle<indirection1, reversesort, reverseorder, absolute, issigned, isfloatingpoint, indirection2, isindexed2, V>(count, input, buffer, varparameters...);
		}else{
			helper::radixsortcopynoallocsingle<reinterpret_cast<std::conditional_t<std::is_pointer_v<W>, U const *(V:: *), U(V:: *)>>(indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, indirection2, isindexed2, V>(count, input, buffer, varparameters...);
		}
	}
}

// Wrapper to implement the radixsort() function with indirection, which only allocates some memory prior to sorting arrays
// This requires no specialisation for handling the single-part types.
template<auto indirection1, sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_pointer_v<decltype(indirection1)> &&
	128 >= CHAR_BIT * sizeof(helper::stripenum<std::remove_pointer_t<std::remove_cvref_t<helper::memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>>),
	bool> radixsort(size_t count, V *input[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		, vararguments... varparameters)noexcept(
	std::is_member_object_pointer_v<decltype(indirection1)> ||
	std::is_nothrow_invocable_v<std::conditional_t<isindexed2, decltype(helper::splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>){
	auto
#if defined(_POSIX_C_SOURCE)
		[buffer, allocsize]
#else
		buffer
#endif
		{allocatearray<V *>(count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, largepagesize
#elif defined(_POSIX_C_SOURCE)
		, mmapflags
#endif
		)};
	if(buffer){
		buffermemorywrapper<V *> guard{buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
		};// ensure the buffer is deallocated, even if an exception is thrown by the getter function here
		radixsortnoalloc<indirection1, direction, mode, indirection2, isindexed2, V>(count, input, buffer, false, varparameters...);
		return{true};
	}
	return{false};
}

// Wrapper to implement the multi-part radixsortcopy() function with indirection, which only allocates some memory prior to sorting arrays
template<auto indirection1, sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_pointer_v<decltype(indirection1)> &&
	128 >= CHAR_BIT * sizeof(helper::stripenum<std::remove_pointer_t<std::remove_cvref_t<helper::memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>>) &&
	8 < CHAR_BIT * sizeof(helper::stripenum<std::remove_pointer_t<std::remove_cvref_t<helper::memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>>),
	bool> radixsortcopy(size_t count, V *const input[], V output[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		, vararguments... varparameters)noexcept(
	std::is_member_object_pointer_v<decltype(indirection1)> ||
	std::is_nothrow_invocable_v<std::conditional_t<isindexed2, decltype(helper::splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>){
	auto
#if defined(_POSIX_C_SOURCE)
		[buffer, allocsize]
#else
		buffer
#endif
		{allocatearray<V *>(count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, largepagesize
#elif defined(_POSIX_C_SOURCE)
		, mmapflags
#endif
		)};
	if(buffer){
		buffermemorywrapper<V *> guard{buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
		};// ensure the buffer is deallocated, even if an exception is thrown by the getter function here
		radixsortcopynoalloc<indirection1, direction, mode, indirection2, isindexed2, V>(count, input, output, buffer, varparameters...);
		return{true};
	}
	return{false};
}

// Wrapper to implement the single-part radixsortcopy() function with indirection, which only allocates some memory prior to sorting arrays
template<auto indirection1, sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_pointer_v<decltype(indirection1)> &&
	8 >= CHAR_BIT * sizeof(helper::stripenum<std::remove_pointer_t<std::remove_cvref_t<helper::memberpointerdeduce<indirection1, isindexed2, V, vararguments...>>>>),
	bool> radixsortcopy(size_t count, V *const input[], V *output[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		, vararguments... varparameters)noexcept(
	std::is_member_object_pointer_v<decltype(indirection1)> ||
	std::is_nothrow_invocable_v<std::conditional_t<isindexed2, decltype(helper::splitget<indirection1, V, vararguments...>), decltype(indirection1)>, V *, vararguments...>){
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
	assert(!(largepagesize - 1 & largepagesize));// a maximum of one bit should be set in the value of largepagesize
	static_cast<void>(largepagesize);
#elif defined(_POSIX_C_SOURCE)
	static_cast<void>(mmapflags);
#endif
	// the single-part version never needs an extra buffer
	radixsortcopynoalloc<indirection1, direction, mode, indirection2, isindexed2, V>(count, input, output, varparameters...);
	return{true};
}

// Wrapper for the multi-part radixsortcopynoalloc() function with type and offset pointer indirection
template<typename T, ptrdiff_t indirection1 = 0, sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_arithmetic_v<std::remove_pointer_t<T>> &&
	128 >= CHAR_BIT * sizeof(std::remove_pointer_t<T>) &&
	8 < CHAR_BIT * sizeof(std::remove_pointer_t<T>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[], vararguments... varparameters)noexcept{
	using W = helper::stripenum<std::remove_pointer_t<T>>;
	static_assert(!std::is_pointer_v<W>, "third level indirection is not supported");
	static bool constexpr reversesort{static_cast<bool>(1 & direction)};
	static bool constexpr reverseorder{static_cast<bool>(1 << 1 & direction)};
	static bool constexpr absolute{
		(nativeabsmode <= mode && std::is_signed_v<W>) ||
		(nativemode > mode && static_cast<bool>(1 & mode))};
	static bool constexpr issigned{
		(nativemode <= mode && nativeabsmode >= mode && std::is_signed_v<W>) ||
		(nativemode > mode && static_cast<bool>(1 << 1 & mode))};
	static bool constexpr isfloatingpoint{
		(nativemode <= mode && std::is_floating_point_v<W>) ||
		(nativemode > mode && static_cast<bool>(1 << 2 & mode))};
	using U = helper::memberobjectgenerator<std::conditional_t<std::is_pointer_v<T>, helper::tounifunsigned<W> const *, helper::tounifunsigned<W>>, indirection1>;
	// Allow the V type items to gain const by a const_cast.
	helper::radixsortcopynoallocmulti<&U::object, reversesort, reverseorder, absolute, issigned, isfloatingpoint, indirection2, isindexed2, U const>(count, reinterpret_cast<U const *const *>(const_cast<V const *const *>(input)), reinterpret_cast<U const **>(const_cast<V const **>(output)), reinterpret_cast<U const **>(const_cast<V const **>(buffer)), varparameters...);
}

// Wrapper for the multi-part radixsortnoalloc() function with type and offset pointer indirection
template<typename T, ptrdiff_t indirection1 = 0, sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_arithmetic_v<std::remove_pointer_t<T>> &&
	128 >= CHAR_BIT * sizeof(std::remove_pointer_t<T>) &&
	8 < CHAR_BIT * sizeof(std::remove_pointer_t<T>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer = false, vararguments... varparameters)noexcept{
	using W = helper::stripenum<std::remove_pointer_t<T>>;
	static_assert(!std::is_pointer_v<W>, "third level indirection is not supported");
	static bool constexpr reversesort{static_cast<bool>(1 & direction)};
	static bool constexpr reverseorder{static_cast<bool>(1 << 1 & direction)};
	static bool constexpr absolute{
		(nativeabsmode <= mode && std::is_signed_v<W>) ||
		(nativemode > mode && static_cast<bool>(1 & mode))};
	static bool constexpr issigned{
		(nativemode <= mode && nativeabsmode >= mode && std::is_signed_v<W>) ||
		(nativemode > mode && static_cast<bool>(1 << 1 & mode))};
	static bool constexpr isfloatingpoint{
		(nativemode <= mode && std::is_floating_point_v<W>) ||
		(nativemode > mode && static_cast<bool>(1 << 2 & mode))};
	using U = helper::memberobjectgenerator<std::conditional_t<std::is_pointer_v<T>, helper::tounifunsigned<W> const *, helper::tounifunsigned<W>>, indirection1>;
	// Allow the V type items to gain const by a const_cast.
	helper::radixsortnoallocmulti<&U::object, reversesort, reverseorder, absolute, issigned, isfloatingpoint, indirection2, isindexed2, U const>(count, reinterpret_cast<U const **>(const_cast<V const **>(input)), reinterpret_cast<U const **>(const_cast<V const **>(buffer)), movetobuffer, varparameters...);
}

// Wrapper for the single-part radixsortcopynoalloc() function with type and offset pointer indirection
template<typename T, ptrdiff_t indirection1 = 0, sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<// disable the option for with the V *buffer[] argument here, and do not allow active compile-time template evaluation with it
	!std::is_same_v<V **, std::conditional_t<0 < sizeof...(vararguments),
		std::invoke_result_t<decltype(helper::splitparameter<vararguments...>), vararguments...>, void>> &&
	std::is_arithmetic_v<std::remove_pointer_t<T>> &&
	8 >= CHAR_BIT * sizeof(helper::stripenum<std::remove_pointer_t<std::remove_cvref_t<
		typename std::enable_if<!std::is_same_v<V **, std::conditional_t<0 < sizeof...(vararguments),
			std::invoke_result_t<decltype(helper::splitparameter<vararguments...>), vararguments...>, void>>,
			helper::memberpointerdeducebody<indirection1, isindexed2, V, vararguments...>>::type>>>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], vararguments... varparameters)noexcept{
	using W = helper::stripenum<std::remove_pointer_t<T>>;
	static_assert(!std::is_pointer_v<W>, "third level indirection is not supported");
	static bool constexpr reversesort{static_cast<bool>(1 & direction)};
	static bool constexpr reverseorder{static_cast<bool>(1 << 1 & direction)};
	static bool constexpr absolute{
		(nativeabsmode <= mode && std::is_signed_v<W>) ||
		(nativemode > mode && static_cast<bool>(1 & mode))};
	static bool constexpr issigned{
		(nativemode <= mode && nativeabsmode >= mode && std::is_signed_v<W>) ||
		(nativemode > mode && static_cast<bool>(1 << 1 & mode))};
	static bool constexpr isfloatingpoint{
		(nativemode <= mode && std::is_floating_point_v<W>) ||
		(nativemode > mode && static_cast<bool>(1 << 2 & mode))};
	using U = helper::memberobjectgenerator<std::conditional_t<std::is_pointer_v<T>, helper::tounifunsigned<W> const *, helper::tounifunsigned<W>>, indirection1>;
	// Allow the V type items to gain const by a const_cast.
	helper::radixsortcopynoallocsingle<&U::object, reversesort, reverseorder, absolute, issigned, isfloatingpoint, indirection2, isindexed2, U const>(count, reinterpret_cast<U const *const *>(const_cast<V const *const *>(input)), reinterpret_cast<U const **>(const_cast<V const **>(output)), varparameters...);
}

// Wrapper for the single-part radixsortcopynoalloc() function with type and offset pointer indirection with a dummy buffer argument
template<typename T, ptrdiff_t indirection1 = 0, sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_arithmetic_v<std::remove_pointer_t<T>> &&
	8 >= CHAR_BIT * sizeof(std::remove_pointer_t<T>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[], vararguments... varparameters)noexcept{
	static_cast<void>(buffer);// the single-part version never needs an extra buffer
	radixsortcopynoallocsingle<T, indirection1, direction, mode, indirection2, isindexed2, V>(count, input, output, varparameters);
}

// Wrapper for the single-part radixsortnoalloc() function with type and offset pointer indirection
template<typename T, ptrdiff_t indirection1 = 0, sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<// disable the option for with the bool movetobuffer argument here, and do not allow active compile-time template evaluation with it
	!std::is_same_v<bool, std::conditional_t<0 < sizeof...(vararguments),
		std::invoke_result_t<decltype(helper::splitparameter<vararguments...>), vararguments...>, void>> &&
	std::is_arithmetic_v<std::remove_pointer_t<T>> &&
	8 >= CHAR_BIT * sizeof(helper::stripenum<std::remove_pointer_t<std::remove_cvref_t<
		typename std::enable_if<!std::is_same_v<bool, std::conditional_t<0 < sizeof...(vararguments),
			std::invoke_result_t<decltype(helper::splitparameter<vararguments...>), vararguments...>, void>>,
			helper::memberpointerdeducebody<indirection1, isindexed2, V, vararguments...>>::type>>>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], vararguments... varparameters)noexcept{
	using W = helper::stripenum<std::remove_pointer_t<T>>;
	static_assert(!std::is_pointer_v<W>, "third level indirection is not supported");
	static bool constexpr reversesort{static_cast<bool>(1 & direction)};
	static bool constexpr reverseorder{static_cast<bool>(1 << 1 & direction)};
	static bool constexpr absolute{
		(nativeabsmode <= mode && std::is_signed_v<W>) ||
		(nativemode > mode && static_cast<bool>(1 & mode))};
	static bool constexpr issigned{
		(nativemode <= mode && nativeabsmode >= mode && std::is_signed_v<W>) ||
		(nativemode > mode && static_cast<bool>(1 << 1 & mode))};
	static bool constexpr isfloatingpoint{
		(nativemode <= mode && std::is_floating_point_v<W>) ||
		(nativemode > mode && static_cast<bool>(1 << 2 & mode))};
	using U = helper::memberobjectgenerator<std::conditional_t<std::is_pointer_v<T>, helper::tounifunsigned<W> const *, helper::tounifunsigned<W>>, indirection1>;
	// Allow the V type items to gain const by a const_cast.
	helper::radixsortnoallocsingle<&U::object, reversesort, reverseorder, absolute, issigned, isfloatingpoint, indirection2, isindexed2, U const>(count, reinterpret_cast<U const **>(const_cast<V const **>(input)), reinterpret_cast<U const **>(const_cast<V const **>(buffer)), varparameters...);
}

// Wrapper for the single-part radixsortnoalloc() and radixsortcopynoalloc() functions with with type and offset pointer indirection
// This variant does not set the default "false" for the "movetobuffer" parameter.
template<typename T, ptrdiff_t indirection1 = 0, sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_arithmetic_v<std::remove_pointer_t<T>> &&
	8 >= CHAR_BIT * sizeof(std::remove_pointer_t<T>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer, vararguments... varparameters)noexcept{
	using W = helper::stripenum<std::remove_pointer_t<T>>;
	static_assert(!std::is_pointer_v<W>, "third level indirection is not supported");
	static bool constexpr reversesort{static_cast<bool>(1 & direction)};
	static bool constexpr reverseorder{static_cast<bool>(1 << 1 & direction)};
	static bool constexpr absolute{
		(nativeabsmode <= mode && std::is_signed_v<W>) ||
		(nativemode > mode && static_cast<bool>(1 & mode))};
	static bool constexpr issigned{
		(nativemode <= mode && nativeabsmode >= mode && std::is_signed_v<W>) ||
		(nativemode > mode && static_cast<bool>(1 << 1 & mode))};
	static bool constexpr isfloatingpoint{
		(nativemode <= mode && std::is_floating_point_v<W>) ||
		(nativemode > mode && static_cast<bool>(1 << 2 & mode))};
	using U = helper::memberobjectgenerator<std::conditional_t<std::is_pointer_v<T>, helper::tounifunsigned<W> const *, helper::tounifunsigned<W>>, indirection1>;
	// Allow the V type items to gain const by a const_cast.
	if(!movetobuffer){
		helper::radixsortnoallocsingle<&U::object, reversesort, reverseorder, absolute, issigned, isfloatingpoint, indirection2, isindexed2, U const>(count, reinterpret_cast<U const **>(const_cast<V const **>(input)), reinterpret_cast<U const **>(const_cast<V const **>(buffer)), varparameters...);
	}else{
		helper::radixsortcopynoallocsingle<&U::object, reversesort, reverseorder, absolute, issigned, isfloatingpoint, indirection2, isindexed2, U const>(count, reinterpret_cast<U const **>(const_cast<V const **>(input)), reinterpret_cast<U const **>(const_cast<V const **>(buffer)), varparameters...);
	}
}

// Wrapper to implement the radixsort() function with with type and offset pointer indirection, which only allocates some memory prior to sorting arrays with indirection
// This requires no specialisation for handling the single-part types.
template<typename T, ptrdiff_t indirection1 = 0, sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_arithmetic_v<std::remove_pointer_t<T>> &&
	128 >= CHAR_BIT * sizeof(std::remove_pointer_t<T>),
	bool> radixsort(size_t count, V *input[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		, vararguments... varparameters)noexcept{
	auto
#if defined(_POSIX_C_SOURCE)
		[buffer, allocsize]
#else
		buffer
#endif
		{allocatearray<V *>(count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, largepagesize
#elif defined(_POSIX_C_SOURCE)
		, mmapflags
#endif
		)};
	if(buffer){
		radixsortnoalloc<T, indirection1, direction, mode, indirection2, isindexed2, V>(count, input, buffer, false, varparameters...);
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

// Wrapper to implement the multi-part radixsortcopy() function with with type and offset pointer indirection, which only allocates some memory prior to sorting arrays with indirection
template<typename T, ptrdiff_t indirection1 = 0, sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_arithmetic_v<std::remove_pointer_t<T>> &&
	128 >= CHAR_BIT * sizeof(std::remove_pointer_t<T>) &&
	8 < CHAR_BIT * sizeof(std::remove_pointer_t<T>),
	bool> radixsortcopy(size_t count, V *const input[], V output[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		, vararguments... varparameters)noexcept{
	auto
#if defined(_POSIX_C_SOURCE)
		[buffer, allocsize]
#else
		buffer
#endif
		{allocatearray<V *>(count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, largepagesize
#elif defined(_POSIX_C_SOURCE)
		, mmapflags
#endif
		)};
	if(buffer){
		radixsortcopynoalloc<T, indirection1, direction, mode, indirection2, isindexed2, V>(count, input, output, buffer, varparameters...);
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

// Wrapper to implement the single-part radixsortcopy() function with with type and offset pointer indirection, which only allocates some memory prior to sorting arrays with indirection
template<typename T, ptrdiff_t indirection1 = 0, sortingdirection direction = ascendingforwardordered, sortingmode mode = nativemode, ptrdiff_t indirection2 = 0, bool isindexed2 = false, typename V, typename... vararguments>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_arithmetic_v<std::remove_pointer_t<T>> &&
	8 >= CHAR_BIT * sizeof(std::remove_pointer_t<T>),
	bool> radixsortcopy(size_t count, V *const input[], V *output[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		, vararguments... varparameters)noexcept{
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
	assert(!(largepagesize - 1 & largepagesize));// a maximum of one bit should be set in the value of largepagesize
	static_cast<void>(largepagesize);
#elif defined(_POSIX_C_SOURCE)
	static_cast<void>(mmapflags);
#endif
	// the single-part version never needs an extra buffer
	radixsortcopynoalloc<T, indirection1, direction, mode, indirection2, isindexed2, V>(count, input, output, varparameters...);
	return{true};
}

// Library finalisation
// It's a good practice to not propagate private macro definitions when compiling the next files.
// There are no more than these two items from #define statements in this library.
#undef RSBD8_FUNC_INLINE
#undef RSBD8_FUNC_NORMAL
}// namespace rsbd8