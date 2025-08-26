#pragma once
// MIT License
// Copyright (c) 2025 Jan-Willem Krans (janwillem32 <at> hotmail <dot> com)
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// Radix sort, least significant bit first method
// This library implements an efficient stable sort on arrays using an 8-bit indexed radix sort method.
// Sorting functionality is available for unsigned integer, signed integer, floating-point and enumeration types.
// All these sorting functions can sort forwards and reverse, order forwards and reverse, and optionally filter by absolute value.
// Three special-use filters are available: one for absolute signed integer, one for absolute floating-point, and one for regular floating-point values.
// See "Modes of operation for the template functions" for more details on that.
// Implemented function optimisations include the ability to skip sorting steps, using parallel (bidirectional) indexing and copying while sorting, and usage of platform-specific intrinsic functions.
// Radix sort in general can be used to sort all array sizes, but is more efficient when applied to somewhat larger arrays compared to other efficient (and stable) comparison-based methods, like introsort.
// See "Performance tests" for more details about array sizes, types and achievable improvements in speed when sorting large arrays with this library.

// Examples of using the 4 templates with simple arrays as input (automatically deduced template parameters are omitted here):
// rsdb8::radixsort() and rsdb8::radixsortcopy() are only wrapper functions that (typically) allocate memory prior to using the actual sorting functions.
// No intermediate buffer array is required when any variant of radixsortcopynoalloc() is used for sorting a single-byte type.
//
// bool succeeded{rsdb8::radixsort(count, inputarr, pagesizeoptional)};
// bool succeeded{rsdb8::radixsortcopy(count, inputarr, outputarr, pagesizeoptional)};
// rsdb8::radixsortnoalloc(count, inputarr, bufferarr, false);
// rsdb8::radixsortcopynoalloc(count, inputarr, outputarr, bufferarr);

// Examples of using the 4 templates with input from first-level indirection (automatically deduced template parameters are omitted here):
// The address offset template parameters displace the pointers they act on like a flat "char const *", so not as some sort of array indices.
// Using getter functions here requires the "noexcept" attribute on the function, as all of the template functions in this library are noexcept-qualified.
// When using a legacy code getter function that is known to never throw, but lacks "noexcept", just use something like "reinterpret_cast<char(myclass:: *)()const noexcept>" on the function pointer. (Replace "char" with the correct return type.)
//
// bool succeeded{rsdb8::radixsort<&myclass::getterfunc>(count, inputarr, pagesizeoptional)};
// bool succeeded{rsdb8::radixsort<addressoffset, uint64_t>(count, inputarr, pagesizeoptional)};
// bool succeeded{rsdb8::radixsort<&myclass::member>(count, inputarr, pagesizeoptional)};
//
// bool succeeded{rsdb8::radixsortcopy<&myclass::getterfunc>(count, inputarr, outputarr, pagesizeoptional, getterparameters...)};
// bool succeeded{rsdb8::radixsortcopy<addressoffset, bool>(count, inputarr, outputarr, pagesizeoptional)};
// bool succeeded{rsdb8::radixsortcopy<&myclass::member>(count, inputarr, outputarr, pagesizeoptional)};
//
// rsdb8::radixsortnoalloc<&myclass::getterfunc>(count, inputarr, bufferarr, false, getterparameters...);
// rsdb8::radixsortnoalloc<addressoffset, int16_t>(count, inputarr, bufferarr, false);
// rsdb8::radixsortnoalloc<&myclass::member>(count, inputarr, bufferarr, false);
//
// rsdb8::radixsortcopynoalloc<&myclass::getterfunc>(count, inputarr, outputarr, bufferarr, getterparameters...);
// rsdb8::radixsortcopynoalloc<addressoffset, float>(count, inputarr, outputarr, bufferarr);
// rsdb8::radixsortcopynoalloc<&myclass::member>(count, inputarr, outputarr, bufferarr);
//
// There are only 4 template functions that actually implement sorting with indirection here:
// - rsdb8::radixsortcopynoalloc<&myclass::getterfunc>(count, inputarr, outputarr, bufferarr, getterparameters...);
// - rsdb8::radixsortnoalloc<&myclass::getterfunc>(count, inputarr, bufferarr, false, getterparameters...);
// These both have a single-byte and a multi-byte type template function.
// These 4 are also the only template functions where no attempt has been made to (force) inline them.
// Absolutely all other template functions are (inlined) wrapper template functions for these 4, using the tools:
// - rsdb8::allocatearray()
// - rsdb8::deallocatearray()
// - rsdb8::helper::GetOffsetOf
// - rsdb8::helper::gettercreator
// This library only includes the common use scenarios to deal with input from first- and second-level indirection.
// As per usual in software, the user is welcome to create a custom getter function by writing a new "gettercreator" variant.
// Similarly, editing the usually parallel input filter (inlined) template functions is an option to use custom transforms:
// - rsdb8::helper::filtertopbyte() (2 variants, for 1 and 2 integer inputs)
// - rsdb8::helper::filtershiftbyte() (2 variants, for 1 and 2 integer inputs)
// - rsdb8::helper::filterinput() (many variants, usage is dependent on input data size)

// TODO: add wrappers for the second-level indirection variants (rsdb8::helper::gettercreator is already done)
// Examples of using the 4 templates with input from second-level indirection (automatically deduced template parameters are omitted here):
// As the use case for these almost always involve multi-pass techniques, the user is advised to allocate the (reusable) buffers accordingly and avoid the use of radixsortcopy() and radixsort().
// The rsdb8::allocatearray() and rsdb8::deallocatearray() inline function templates are provided for handling an intermediate buffer.
// Again, no intermediate buffer is required when radixsortcopynoalloc() is used for sorting a single-byte type.
// These will internally first retrieve a pointer to a "T" type array "T *myarray".
// After that it's dereferenced at the origin (first set of examples) or indexed (second set of examples) as "myarray[indirectionindex]" to retrieve the value used for sorting.
// Again, all "addressoffset" variants as template inputs displace like on a flat "char const *", so not as some sort of array indices.
// The "addressoffset1" item here displaces the pointer in the input array to get the secondary pointer.
// The "addressoffset2" item here displaces the secondary pointer before applying the (variable) "indirectionindex" item here as an index.
//
// Examples of using the 2 templates with no indexed second-level indirection:
//
// rsdb8::radixsortcopynoalloc<addressoffset2, &myclass::getterfunc>(count, inputarr, outputarr, bufferarr, getterparameters...);
// rsdb8::radixsortcopynoalloc<addressoffset1, addressoffset2, long>(count, inputarr, outputarr, bufferarr);
// rsdb8::radixsortcopynoalloc<addressoffset2, &myclass::member>(count, inputarr, outputarr, bufferarr);
//
// rsdb8::radixsortnoalloc<addressoffset2, &myclass::getterfunc>(count, inputarr, bufferarr, false, getterparameters...);
// rsdb8::radixsortnoalloc<addressoffset1, addressoffset2, wchar_t>(count, inputarr, bufferarr, false);
// rsdb8::radixsortnoalloc<addressoffset2, &myclass::member>(count, inputarr, bufferarr, false);
//
// Examples of using the 2 templates with indexed second-level indirection:
//
// rsdb8::radixsortcopynoalloc<addressoffset2, &myclass::getterfunc>(indirectionindex, count, inputarr, outputarr, bufferarr, getterparameters...);
// rsdb8::radixsortcopynoalloc<addressoffset1, addressoffset2, double>(indirectionindex, count, inputarr, outputarr, bufferarr);
// rsdb8::radixsortcopynoalloc<addressoffset2, &myclass::member>(indirectionindex, count, inputarr, outputarr, bufferarr);
//
// rsdb8::radixsortnoalloc<addressoffset2, &myclass::getterfunc>(indirectionindex, count, inputarr, bufferarr, false, getterparameters...);
// rsdb8::radixsortnoalloc<addressoffset1, addressoffset2, unsigned long>(indirectionindex, count, inputarr, bufferarr, false);
// rsdb8::radixsortnoalloc<addressoffset2, &myclass::member>(indirectionindex, count, inputarr, bufferarr, false);

// Bonus example of the longest regular item, with complete decoration of the template (and not using any defaults):
// bool succeeded{rsdb8::radixsortcopy<addressoffset, unsigned long long int, true, true, true, true true, myclass>(count, inputarr, outputarr, largepagesize)};
// This example means:
// The function radixsortcopy() will load data from "inputarr" (optionally const-qualified) and write the result to "outputarr".
// There are "count" many items of type "unsigned long long int" at the offset "addressoffset" inside the structures in the pointer array "inputarr".
// Correctly using "pagesizeoptional" allows using larger system pages for the system memory allocator, useful for large memory allocations.
// The return "succeeded" indicates if memory allocation (and subsequent sorting that cannot fail) succeeded.
// The first "true" is to use reverse sorting.
// The second "true" is to use reverse ordering.
// The third "true" is to use the absolute value while sorting.
// The fourth "true" is to force a signed type (reinterpreting as the signed type of the same size).
// The fifth "true" is to force a floating-point type (reinterpreting as the floating-point type of the same size).
// The "myclass" struct/class type is enforced instead of being deduced from the input arrays.

// The 4 main sorting template functions that are implemented here
// - radixsortnoalloc():
//    - counted (first parameter "count", the end of arrays are no inputs to these functions unlike some sorting functions)
//    - sorts an array (second parameter "input")
//    - uses an array as a buffer of the same size and type (third parameter "buffer")
//    - with a toggle to output to either the input array or the buffer array (optional fourth parameter "movetobuffer")
//    - the array that is not selected for output contains garbage afterwards (typically the leftovers from an intermediate sorting stage)
//    - both arrays need to be writable, but when using indirection the class itself can be const-qualified
// - radixsortcopynoalloc():
//    - counted (first parameter "count")
//    - similar to radixsortnoalloc(), but will not write to the input array, which can be const-qualified (second parameter "input")
//    - uses a dedicated output array of the same size (third parameter "output")
//    - uses a memory buffer of the same size, which contains garbage afterwards (fourth parameter)
// - radixsort():
//    - wrapper template for radixsortnoalloc()
//    - only allocates memory for the buffer parameter
//    - (Windows-only) large page size can be used if enabled with the lock memory privilege for the application enabled (optional third parameter)
//    - (POSIX implementing systems-only) flags for enabling pages with huge TLB functionality can be used (optional third parameter)
// - radixsortcopy():
//    - wrapper template for radixsortcopynoalloc()
//    - only allocates memory for the buffer parameter
//    - (Windows-only) large page size can be used if enabled with the lock memory privilege for the application enabled (optional third parameter)
//    - (POSIX implementing systems-only) flags for enabling pages with huge TLB functionality can be used (optional third parameter)

// Modes of operation for the template functions
// All sorting functions here are templates, all of them have "bool reversesort = false, bool reverseorder = false, bool absolute = false" as template parameters.
// All sorting functions here additionally have "bool issigned" in the templates to force processing as signed or unsigned types.
// All sorting functions here additionally have "bool isfloatingpoint" in the templates to force processing as floating-point types.
//
// The five regular modes that can be activated are:
// - regular unsigned integer (default for unsigned input types)
//   and also inside-out signed integer
//   (sorts ascending from 0, maximum value, minimum value, to -1):
//    - absolute = false, issigned = false, isfloatingpoint = false
// - regular signed integer (default for signed input types):
//    - absolute = false, issigned = true, isfloatingpoint = false
// - absolute signed integer:
//    - absolute = true, issigned = true, isfloatingpoint = false
// - regular floating-point (default for floating-point input types):
//    - absolute = false, issigned = true, isfloatingpoint = true
// - absolute floating-point
//   and also unsigned integer without using the top bit:
//    - absolute = true, issigned = true, isfloatingpoint = true
//
// The three special modes that can be activated are:
// - inside-out floating-point
//   (sorts ascending from +0., +infinity, +NaN, -NaN, -infinity, to -0.):
//    - absolute = false, issigned = false, isfloatingpoint = true
// - absolute floating-point, but negative inputs will sort just below their positive counterparts
//   (sorts ascending from -0., +0., -infinity, +infinity, to various -NaN or +NaN values at the end):
//    - absolute = true, issigned = false, isfloatingpoint = true
// - absolute signed integer, but negative inputs will sort just below their positive counterparts
//   (sorts ascending from 0, -1, 1, -2, 2, and so on, will work correctly for minimum values):
//    - absolute = true, issigned = false, isfloatingpoint = false
//
// The two reversing modes are:
// - reversesort (default false): reverse the sorting direction
// - reverseorder (default false): reverse the array direction when sorting items with the same value (only used when dealing with indirection)
// Enabling reversesort costs next to nothing in terms of performance, reverseorder does initially take minor extra processing when handling multi-byte types.
// - reversesort = false, reverseorder = false: stable sort, low to high
// - reversesort = true, reverseorder = true: stable sort, high to low, the complete opposite direction of the default functionality
// - reversesort = true, reverseorder = false: stable sort, high to low, but keeps items with the same value in the same order as in the source
// - reversesort = false, reverseorder = false: stable sort, low to high, but reverses items of the same value compared to the order in the source
// The combination of reversesort = false, reverseorder = false is uncommon, but can be useful in some rare cases.
//
// To give an example of reversesort = true, reverseorder = false, as it's a bit tricky to imagine without a reference:
// myclass collA[]{{1, "first"}, {1, "second"}, {-5, "third"}, {2, "fourth"}};// list construct
// myclass *pcollA[]{collA, collA + 1, collA + 2, collA + 3};// list pointers
// rsbd8::radixsortnoalloc<&myclass::intorder, true, false>(4, pcollA, psomeunusedbuffer);
// Members of "pcollA" will then get sorted according to their value "intorder", in reverse order, while keeping the same array order.
// Pointers will in this case point to: {2, "fourth"}, {1, "first"}, {1, "second"}, {-5, "third"}.
// That is different from fully reversing the order when using this line instead of the above:
// rsbd8::radixsortnoalloc<&myclass::intorder, true, true>(4, pcollA, psomeunusedbuffer);
// Pointers will in this case point to: {2, "fourth"}, {1, "second"}, {1, "first"}, {-5, "third"}.
// Notice the same reverse stable sorting here, but opposite placement when encountering the same value multiple times.

// Miscellaneous notes
// Sorting unsigned values is the fastest, very closely followed up by signed values, followed up by floating-point values in this library.
// Unsigned 128-bit and larger integers can be sorted by sequential sorting from the bottom to the top parts as unsigned (64-bit) elements when using indirection.
// Signed 128-bit and larger integers are sorted the same, with only the topmost (64-bit) element sorted as signed because of the sign bit (assuming unfiltered input).
// Re-use the same intermediate buffer combined with radixsortnoalloc() or radixsortcopynoalloc() when sorting 128-bit and larger integers like this.
// bool inputs are reinterpreted as the unsigned integer type of the same size. Anything but 0 or 1 in the source data will impact sorting.
// char sign is ambiguous, as by the original C standard. Cast to char8_t * or unsigned char * if unambiguously a binary sort of char characters is desired.
// Floating-point -0. (implies not machine-generated) sorts below +0. by these functions. (Several functions in namespace std can do that.)
// Floating-point NaN values are sorted before negative infinity for the typical machine-generated "undefined" QNaN (0xFFF8'0000'0000'0000 on an IEEE double).
// Floating-point NaN positive values (implies not machine-generated) are sorted after positive infinity (0x7FF0'0000'0000'0001 and onward on an IEEE double).
// Floating-point SNaN (signalling) values do not trigger signals inside these functions. (Several functions in namespace std can do that.)

// TODO... types larger than 8 bytes...
// - long double - TODO... (checked by LDBL_MANT_DIG if it's defined as a 64-, 80- or 128-bit type)
// - 128-bit integers - TODO...

// Extended filtering information for each of the 8 main modes
//
// Modes of operation for the template functions
// - regular unsigned integer (default for unsigned input types)
//   and also inside-out signed integer
//   (sorts ascending from 0, maximum value, minimum value, to -1):
//    - absolute = false, issigned = false, isfloatingpoint = false
//    - lowest amount of filtering cost
//    - straightforward process, no filter at all
// - regular signed integer (default for signed input types):
//    - absolute = false, issigned = true, isfloatingpoint = false
//    - lower amount of filtering cost
//    - no filter in the processing phases
//    - virtually flips the most significant bit when calculating offsets
// - absolute signed integer:
//    - absolute = true, issigned = true, isfloatingpoint = false
//    - medium amount of filtering cost
//    - creates a sign bit mask, adds it to the input and uses it with xor on the input as a filter in the processing phases
// - regular floating-point (default for floating-point input types):
//    - absolute = false, issigned = true, isfloatingpoint = true
//    - higher amount of filtering cost
//    - creates a sign bit mask and uses it with xor on the exponent and mantissa bits as a filter in the processing phases
//    - virtually flips the most significant bit when calculating offsets
// - absolute floating-point
//   and also unsigned integer without using the top bit:
//    - absolute = true, issigned = true, isfloatingpoint = true
//    - low amount of filtering cost
//    - masks out the sign bit in the processing phases
//
// The three special modes that can be activated are:
// - inside-out floating-point
//   (sorts ascending from +0., +infinity, +NaN, -NaN, -infinity, to -0.):
//    - absolute = false, issigned = false, isfloatingpoint = true
//    - higher amount of filtering cost
//    - creates a sign bit mask and uses it with xor on the exponent and mantissa bits as a filter in the processing phases
// - absolute floating-point, but negative inputs will sort just below their positive counterparts
//   (sorts ascending from -0., +0., -infinity, +infinity, to various -NaN or +NaN values at the end):
//    - absolute = true, issigned = false, isfloatingpoint = true
//    - medium amount of filtering cost
//    - bit rotates left by one to move the sign bit to the least significant bit in the processing phases
//    - virtually flips the least significant bit when calculating offsets
// - absolute signed integer, but negative inputs will sort just below their positive counterparts
//   (sorts ascending from 0, -1, 1, -2, 2, and so on, will work correctly for minimum values):
//    - absolute = true, issigned = false, isfloatingpoint = false
//    - medium amount of filtering cost
//    - creates a sign bit mask, shifts the input left by one and uses xor on the input with the sign bit mask as a filter in the processing phases
//
// Enabling "reverseorder = true" on these modes will add slightly more to the initial filtering cost.
// For example, the highest amount of filtering cost would be on the floating-point full reverse mode "reversesort = true, reverseorder = true, absolute = false, issigned = true, isfloatingpoint = true".
// Take "highest amount" with a grain of salt though, as way more complicated filtering stages can be designed than what is used for this combination of filters.

// Performance tests
// This library has a performance test suite used for development.
// These performance test results are for sorting a block of 256 MiB, with fully random bits in unsigned integer and floating-point arrays (with no indirection).
// std::stable_sort() or std::sort() (whichever is faster) vs rsbd8::radixsort(), measured in 100 ns units:
//  8-bit: 20333316752 vs   339824778, a factor of 59.8 in speedup
// 16-bit: 21240768686 vs  1031283456, a factor of 20.6 in speedup
// 32-bit: 16430484788 vs  1171932112, a factor of 14.0 in speedup
// 64-bit:  8098602698 vs  1609636440, a factor of  5.0 in speedup
//  float:  6432446792 vs   582900096, a factor of 11.0 in speedup
// double:  8380261008 vs  1575877406, a factor of  5.3 in speedup
//
// The next tests were done on smaller blocks.
// There will be a minimum amount of array entries where rsbd8::radixsort() starts to get the upper hand in speed over std::stable_sort().
// These test results were obtained by performance testing on multiple sizes of blocks between .5 to 8 KB, with again fully random bits in unsigned integer and floating-point arrays (with no indirection):
//  8-bit: 300 array entries
// 16-bit: 375 array entries
// 32-bit: 400 array entries
// 64-bit: 700 array entries
//  float: 875 array entries
// double: 600 array entries
// Interpreting this means that radix sort variants will be faster for somewhat larger arrays when sorting data under the given conditions.
// In this case that's a sequence of just plain numbers in an array.
// When dealing with sorting while using indirection or filtering, test results will vary.
// TODO: add system configuration data
// TODO: add tests that include indirection

// Table of contents (searchable)
//
// - MIT License
// - Radix sort, least significant bit first method
// - Examples of using the 4 templates with simple arrays as input
// - Examples of using the 4 templates with input from indirection
// - Examples of using the 4 templates with input from modified indirection
// - Bonus example of the longest regular item, with complete decoration of the template
// - The 4 main sorting template functions that are implemented here
// - Modes of operation for the template functions
// - Miscellaneous notes
// - Extended filtering information for each of the 8 main modes
// - Performance tests
// - Table of contents
//
// - Include statements and the last checks for compatibility
// - Definition of the GetOffsetOf template
// - Per-compiler function attributes
//
// - Helper constants and functions
// - Helper functions to implement the 8 main modes
// - Helper functions to implement the offset transforms
//
// - Generic large array allocation and deallocation functions
// - Function implementation templates for multi-byte types
// - Function implementation templates for single-byte types
//
// - 3 variants directly wrap the multi-byte radixsortcopynoalloc() function without indirection
// - 3 variants directly wrap the multi-byte radixsortnoalloc() function without indirection
// - 6 variants directly wrap the single-byte radixsortcopynoalloc() function without indirection
// - 3 variants directly wrap the single-byte radixsortnoalloc() function without indirection
// - 3 variants of single-byte radixsortcopy() that directly wrap the single-byte radixsortcopynoalloc() function without indirection
// - 3 variants of radixsort() that only allocate some memory prior to sorting arrays without indirection
// - 3 variants of multi-byte radixsortcopy() that only allocate some memory prior to sorting arrays without indirection
// - 3 variants of single-byte radixsortcopy() that directly wrap the single-byte radixsortcopynoalloc() function without indirection
//
// - 3 variants directly wrap the multi-byte radixsortcopynoalloc() function with getter function indirection
// - 3 variants directly wrap the multi-byte radixsortnoalloc() function with getter function indirection
// - 6 variants directly wrap the single-byte radixsortcopynoalloc() function with getter function indirection
// - 3 variants directly wrap the single-byte radixsortnoalloc() function with getter function indirection
// - 3 variants directly wrap the single-byte radixsortnoalloc() and radixsortcopynoalloc() functions with getter function indirection
// - 3 variants of radixsort() that only allocate some memory prior to sorting arrays with getter function indirection
// - 3 variants of multi-byte radixsortcopy() that only allocate some memory prior to sorting arrays with getter function indirection
// - 3 variants of single-byte radixsortcopy() that directly wrap the single-byte radixsortcopynoalloc() function with getter function indirection
//
// - 3 variants directly wrap the multi-byte radixsortcopynoalloc() function with offset and type pointer indirection
// - 3 variants directly wrap the multi-byte radixsortnoalloc() function with offset and type pointer indirection
// - 6 variants directly wrap the single-byte radixsortcopynoalloc() function with offset and type pointer indirection
// - 3 variants directly wrap the single-byte radixsortnoalloc() function with offset and type pointer indirection
// - 3 variants directly wrap the single-byte radixsortnoalloc() and radixsortcopynoalloc() functions with offset and type pointer indirection
// - 3 variants of radixsort() that only allocate some memory prior to sorting arrays with offset and type pointer indirection
// - 3 variants of multi-byte radixsortcopy() that only allocate some memory prior to sorting arrays with offset and type pointer indirection
// - 3 variants of single-byte radixsortcopy() that directly wrap the single-byte radixsortcopynoalloc() function with offset and type pointer indirection
//
// - Library finalisation

// Include statements and the last checks for compatibility
// Compiler features minimum requirements are evaluated during compile-time if part of C++14 and newer.
// A more difficult test to implement here would be to detect mixed endianness between floating-point and integer data.
// The C++20 "std::endian" parts in the "bit" header currently unfortunately don't indicate more than generally big and little endianness.
//
// C++17 features detection
// Microsoft C/C++-compatible compilers don't set the __cplusplus predefined macro conforming to the standard by default for some very outdated legacy code reasons.
// (/Zc:__cplusplus can correct it, but it's not part of the regular "Standards conformance" /permissive- compiler options.)
// Use its internal macro instead.
#if 201703L > __cplusplus || defined(_MSVC_LANG) && 201703L > _MSVC_LANG
#error Compiler does not conform to C++17 to compile this library.
#endif
#include <utility>
#if CHAR_BIT & 8 - 1
#error This platform has an addressable unit that isn't divisible by 8. For these kinds of platforms it's better to re-write this library and not use an 8-bit indexed radix sort method.
#endif
#include <cassert>
#if 202002L <= __cplusplus || defined(_MSVC_LANG) && 202002L <= _MSVC_LANG
#include <bit>// (C++20)
// Library feature-test macros (C++20)
//
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
#elif (defined(__GNUC__) || defined(__xlC__)) &&                               \
		(defined(__VEC__) || defined(__ALTIVEC__))
/* XLC or GCC-compatible compiler, targeting PowerPC with VMX/VSX */
#include <altivec.h>
#elif defined(__GNUC__) && defined(__SPE__)
/* GCC-compatible compiler, targeting PowerPC with SPE */
#include <spe.h>
#endif
// End of imported section
namespace rsbd8{// Avoid putting any include files into this library's namespace.
namespace helper{// This libary defines a number of helper items, so categorise them as such.

// Definition of the GetOffsetOf template
// Temporary, until a revision of "offsetof" is standardized in C++ with constexpr.
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

// Per-compiler function attributes
// RSBD8_FUNC_INLINE is suitable to attempt force inlining of any function.
// RSBD8_FUNC_NORMAL is specifically for template functions in this header-only library, and doesn't include even a regular "inline" statement to prevent linking issues for non-template functions.
// These are the only two macros defined in this file, and #undef statements are used for them at the end.
#if defined(DEBUG) || defined(_DEBUG)// This part is debug-only. These are non-standard conforming macros, but note that the "NDEBUG" rule for detecting non-debug builds should only ever apply to runtime assert() statments in C++.
#ifdef _MSC_VER
#define RSBD8_FUNC_INLINE __declspec(noalias safebuffers)
#define RSBD8_FUNC_NORMAL __declspec(noalias safebuffers)
#else
#define RSBD8_FUNC_INLINE
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

// Helper class gettercreator
// The basic variant just transforms an offset and type (with an optional index) for the input pointer to an inlined function.
// More advanced uses are in the options for second-level indirection through another pointer or another (inlined) getter function.
// Casting the pointer-to-member functions using this class is against some common code conventions and several guidelines.
// However, it saves a lot of otherwise duplicated lines of code for this library and it doesn't really involve any loss of performance.
// The U template argument is the desired (unsigned integer) return type for these templated functions.
// The primary template argument is either the first-level address offset (integer), or another getter function (member function pointer).
// The type of the primary template argument will enable one of the two sets of template functions here.
// The addressoffset2 template argument is the optional second-level address offset (integer).
//
// Copied from the main library description:
// Using getter functions here requires the "noexcept" attribute on said function. All of the template functions in this library are noexcept-qualified.
// The address offset template parameters displace the pointers they act on like a flat "char const *", so not as some sort of array indices.
// When using a legacy code getter function that is known to never throw, but lacks "noexcept", just use something like "reinterpret_cast<char(myclass:: *)()const noexcept>" on the function pointer. (Replace "char" with the correct return type.)
template<typename U, auto primary, ptrdiff_t addressoffset2 = 0, class... arguments>
struct gettercreator{
	// first level indirection trough a pointer
	RSBD8_FUNC_INLINE std::enable_if_t<
		std::is_integral_v<decltype(primary)>,
		U> indirection1()const noexcept{
		return{*reinterpret_cast<U const *>(reinterpret_cast<char const *>(this) + primary)};
	}
	RSBD8_FUNC_INLINE std::enable_if_t<
		std::is_integral_v<decltype(primary)>,
		U> indirection1indexed(auto index)const noexcept{
		return{reinterpret_cast<U const *>(reinterpret_cast<char const *>(this) + primary)[index]};
	}

	// first level indirection trough another getter function
	RSBD8_FUNC_INLINE std::enable_if_t<
		std::is_member_function_pointer_v<decltype(primary)> &&
		std::is_nothrow_invocable_v<decltype(primary)(arguments...)>,
		U> indirection1funcconst(arguments &&... getterfuncinput)const noexcept{
		using T = std::invoke_result_t<decltype(primary)(arguments...)>;// no further need for the original class type
#ifdef __cpp_lib_bit_cast
		return{std::bit_cast<U>((this->*reinterpret_cast<T(gettercreator:: *)(arguments...)const noexcept>(primary))(getterfuncinput...))};
#else
		T val{(this->*reinterpret_cast<T(gettercreator:: *)(arguments...)const noexcept>(primary))(getterfuncinput...)};
		return{reinterpret_cast<U &>(val)};
#endif
	}
	RSBD8_FUNC_INLINE std::enable_if_t<
		std::is_member_function_pointer_v<decltype(primary)> &&
		std::is_nothrow_invocable_v<decltype(primary)(arguments...)>,
		U> indirection1func(arguments &&... getterfuncinput)noexcept{
		using T = std::invoke_result_t<decltype(primary)(arguments...)>;// no further need for the original class type
#ifdef __cpp_lib_bit_cast
		return{std::bit_cast<U>((this->*reinterpret_cast<T(gettercreator:: *)(arguments...)noexcept>(primary))(getterfuncinput...))};
#else
		T val{(this->*reinterpret_cast<T(gettercreator:: *)(arguments...)noexcept>(primary))(getterfuncinput...)};
		return{reinterpret_cast<U &>(val)};
#endif
	}

	// second level indirection trough a pointer
	RSBD8_FUNC_INLINE std::enable_if_t<
		std::is_integral_v<decltype(primary)>,
		U> indirection2()const noexcept{
		return{*reinterpret_cast<U const *>(*reinterpret_cast<char const *const *>(reinterpret_cast<char const *>(this) + primary) + addressoffset2)};
	}
	RSBD8_FUNC_INLINE std::enable_if_t<
		std::is_integral_v<decltype(primary)>,
		U> indirection2indexed1(auto index)const noexcept{
		return{reinterpret_cast<U const *>(reinterpret_cast<char const *const *>(reinterpret_cast<char const *>(this) + primary)[index] + addressoffset2)};
	}
	RSBD8_FUNC_INLINE std::enable_if_t<
		std::is_integral_v<decltype(primary)>,
		U> indirection2indexed2(auto index)const noexcept{
		return{reinterpret_cast<U const *>(*reinterpret_cast<char const *const *>(reinterpret_cast<char const *>(this) + primary) + addressoffset2)[index]};
	}
	RSBD8_FUNC_INLINE std::enable_if_t<
		std::is_integral_v<decltype(primary)>,
		U> indirection2indexed(auto index1, auto index2)const noexcept{
		return{reinterpret_cast<U const *>(reinterpret_cast<char const *const *>(reinterpret_cast<char const *>(this) + primary)[index1] + addressoffset2)[index2]};
	}

	// second level indirection trough another getter function
	RSBD8_FUNC_INLINE std::enable_if_t<
		std::is_member_function_pointer_v<decltype(primary)> &&
		std::is_nothrow_invocable_v<decltype(primary)(arguments...)>,
		U> indirection2funcconst(arguments &&... getterfuncinput)const noexcept{
		return{*reinterpret_cast<U const *>((this->*reinterpret_cast<char const *(gettercreator:: *)(arguments...)const noexcept>(primary))(getterfuncinput...) + addressoffset2)};
	}
	RSBD8_FUNC_INLINE std::enable_if_t<
		std::is_member_function_pointer_v<decltype(primary)> &&
		std::is_nothrow_invocable_v<decltype(primary)(arguments...)>,
		U> indirection2funcconstindexed(auto index, arguments &&... getterfuncinput)const noexcept{
		return{reinterpret_cast<U const *>((this->*reinterpret_cast<char const *(gettercreator:: *)(arguments...)const noexcept>(primary))(getterfuncinput...) + addressoffset2)[index]};
	}
	RSBD8_FUNC_INLINE std::enable_if_t<
		std::is_member_function_pointer_v<decltype(primary)> &&
		std::is_nothrow_invocable_v<decltype(primary)(arguments...)>,
		U> indirection2func(arguments &&... getterfuncinput)noexcept{
		return{*reinterpret_cast<U const *>((this->*reinterpret_cast<char const *(gettercreator:: *)(arguments...)noexcept>(primary))(getterfuncinput...) + addressoffset2)};
	}
	RSBD8_FUNC_INLINE std::enable_if_t<
		std::is_member_function_pointer_v<decltype(primary)> &&
		std::is_nothrow_invocable_v<decltype(primary)(arguments...)>,
		U> indirection2funcindexed(auto index, arguments &&... getterfuncinput)noexcept{
		return{reinterpret_cast<U const *>((this->*reinterpret_cast<char const *(gettercreator:: *)(arguments...)noexcept>(primary))(getterfuncinput...) + addressoffset2)[index]};
	}
};

RSBD8_FUNC_INLINE void addcarryofless(unsigned &accumulator, size_t minuend, size_t subtrahend)noexcept{
#ifdef _M_X64
	_addcarry_u32(_subborrow_u64(0, minuend, subtrahend, nullptr), accumulator, 0, &accumulator);// cmp r, r followed by adc r, 0
#elif defined(_M_IX86)
	_addcarry_u32(_subborrow_u32(0, minuend, subtrahend, nullptr), accumulator, 0, &accumulator);// cmp r, r followed by adc r, 0
#else
	accumulator += minuend < subtrahend;
#endif
}

template<typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	64 >= CHAR_BIT * sizeof(T) &&
	std::is_integral_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type>,
	unsigned> bitscanforwardportable(T input)noexcept{
	assert(input);// design descision: do not allow 0 as input as neither x86/x64 bsf nor using the de Bruijn sequence supports it
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
		// This is still easy to handle in assembly (2 general options, and some instructions can be interleaved in between actions):
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
			28,  2,
			29, 14, 24,  3,
			30, 22, 20, 15, 25, 17,  4,  8,
			31, 27, 13, 23, 21, 19, 16,  7,
			26, 12, 18,  6,
			11,  5,
			10,
			 9};
		return{MultiplyDeBruijnBitPosition[static_cast<std::make_unsigned_t<T>>(input & -static_cast<std::make_signed_t<T>>(input)) * 0x077CB531u >> 27]};
	}else{
		static unsigned char constexpr MultiplyDeBruijnBitPosition[64]{
			 0,
			 1,
			17,  2,
			18, 50,  3, 57,
			47, 19, 22, 51, 29,  4, 33, 58,
			15, 48, 20, 27, 25, 23, 52, 41, 54, 30, 38,  5, 43, 34, 59,  8,
			63, 16, 49, 56, 46, 21, 28, 32, 14, 26, 24, 40, 53, 37, 42,  7,
			62, 55, 45, 31, 13, 39, 36,  6,
			61, 44, 12, 35,
			60, 11,
			10,
			 9};
		return{MultiplyDeBruijnBitPosition[static_cast<std::make_unsigned_t<T>>(input & -static_cast<std::make_signed_t<T>>(input)) * 0x37E84A99DAE458Full >> 58]};
	}
#endif
}

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
// There are 4 base functions, handling 1, 2, 3, 4 or 8 inputs.
// Each of the functions have two varians that take one or two pointers per input to store each input before its first modification.
// - modes with no filtering here:
//    - regular unsigned integer and also inside-out signed integer
//    - regular signed integer
// - modes with one-pass filtering here:
//    - absolute floating-point and also unsigned integer without using the top bit
//    - absolute floating-point, but negative inputs will sort just below their positive counterparts
// - modes with two-pass filtering here:
//    - regular floating point
//    - inside-out floating-point
//    - absolute signed integer
//    - absolute signed integer, but negative inputs will sort just below their positive counterparts
// The four starting template functions are customized for the sorting phase, and have no need for variants with pointers.
// All other template functions modify their inputs and each has a variant that write their inputs to memory either once or twice.

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T),
	size_t> filtertopbyte(T cur)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curp{static_cast<std::make_signed_t<T>>(cur)};
		if constexpr(isfloatingpoint || !issigned) cur *= 2;
		curp >>= CHAR_BIT * sizeof(T) - 1;
		T curq{static_cast<T>(curp)};
		if constexpr(isfloatingpoint) cur >>= CHAR_BIT * sizeof(T) - 8 + 1;
		else if constexpr(issigned) cur += curq;
		cur ^= curq;
		if constexpr(8 <= CHAR_BIT * sizeof(T)){
			if constexpr(isfloatingpoint) return{static_cast<size_t>(cur) & 0xFFu};
			else{
				cur >>= CHAR_BIT * sizeof(T) - 8;
				return{static_cast<size_t>(cur)};
			}
		}else return{static_cast<size_t>(cur)};
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		if constexpr(8 <= CHAR_BIT * sizeof(T)){
			cur >>= CHAR_BIT * sizeof(T) - 8 - 1;
			return{static_cast<size_t>(cur) & 0xFFu};
		}else if(issigned){
			cur <<= 1;
			return{static_cast<size_t>(cur)};
		}else{
			cur = rotateleftportable<1>(cur);
			return{static_cast<size_t>(cur)};
		}
	}else if constexpr(8 <= CHAR_BIT * sizeof(T)){
		cur >>= CHAR_BIT * sizeof(T) - 8;
		return{static_cast<size_t>(cur)};
	}else return{static_cast<size_t>(cur)};
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T),
	std::pair<T, T>> filtertopbyte(T cura, T curb)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		if constexpr(isfloatingpoint || !issigned) cura *= 2;
		curap >>= CHAR_BIT * sizeof(T) - 1;
		T curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		if constexpr(isfloatingpoint || !issigned) curb *= 2;
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		T curbq{static_cast<T>(curbp)};
		if constexpr(isfloatingpoint){
			cura >>= CHAR_BIT * sizeof(T) - 8 + 1;
			curb >>= CHAR_BIT * sizeof(T) - 8 + 1;
		}else if constexpr(issigned){
			cura += curaq;
			cura += curaq;
		}
		cura ^= curaq;
		curb ^= curbq;
		if constexpr(8 <= CHAR_BIT * sizeof(T)){
			if constexpr(isfloatingpoint){
				return{std::make_pair(static_cast<size_t>(cura) & 0xFFu, static_cast<size_t>(curb) & 0xFFu)};
			}else{
				cura >>= CHAR_BIT * sizeof(T) - 8;
				curb >>= CHAR_BIT * sizeof(T) - 8;
				return{std::make_pair(static_cast<size_t>(cura), static_cast<size_t>(curb))};
			}
		}else return{std::make_pair(static_cast<size_t>(cura), static_cast<size_t>(curb))};
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		if constexpr(8 <= CHAR_BIT * sizeof(T)){
			cura >>= CHAR_BIT * sizeof(T) - 8 - 1;
			curb >>= CHAR_BIT * sizeof(T) - 8 - 1;
			return{std::make_pair(static_cast<size_t>(cura) & 0xFFu, static_cast<size_t>(curb) & 0xFFu)};
		}else if(issigned){
			cura <<= 1;
			curb <<= 1;
			return{std::make_pair(static_cast<size_t>(cura), static_cast<size_t>(curb))};
		}else{
			cura = rotateleftportable<1>(cura);
			curb = rotateleftportable<1>(curb);
			return{std::make_pair(static_cast<size_t>(cura), static_cast<size_t>(curb))};
		}
	}else if constexpr(8 <= CHAR_BIT * sizeof(T)){
		cura >>= CHAR_BIT * sizeof(T) - 8;
		curb >>= CHAR_BIT * sizeof(T) - 8;
		return{std::make_pair(static_cast<size_t>(cura), static_cast<size_t>(curb))};
	}else return{std::make_pair(static_cast<size_t>(cura), static_cast<size_t>(curb))};
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	T> filtershiftbyte(T cur, unsigned shift)noexcept{
	// Filtering is simplified if possible.
	// This should never filter the top byte for non-absolute floating-point inputs.
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curp{static_cast<std::make_signed_t<T>>(cur)};
		if constexpr(absolute && !issigned) cur *= 2;
		curp >>= CHAR_BIT * sizeof(T) - 1;
		T curq{static_cast<T>(curp)};
		if constexpr(!isfloatingpoint && issigned) cur += curq;
		cur ^= curq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		if(issigned) cur <<= 1;
		else cur = rotateleftportable<1>(cur);
	}
	cur >>= shift;
	return{static_cast<size_t>(cur) & 0xFFu};
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	std::pair<size_t, size_t>> filtershiftbyte(T cura, T curb, unsigned shift)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		if constexpr(absolute && !issigned) cura *= 2;
		curap >>= CHAR_BIT * sizeof(T) - 1;
		T curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		if constexpr(absolute && !issigned) curb *= 2;
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		T curbq{static_cast<T>(curbp)};
		if constexpr(!isfloatingpoint && issigned){
			cura += curaq;
			curb += curbq;
		}
		cura ^= curaq;
		curb ^= curbq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		if(issigned){
			cura <<= 1;
			curb <<= 1;
		}else{
			cura = rotateleftportable<1>(cura);
			curb = rotateleftportable<1>(curb);
		}
	}
	cura >>= shift;
	curb >>= shift;
	return{std::make_pair(static_cast<size_t>(cura) & 0xFFu, static_cast<size_t>(curb) & 0xFFu)};
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T),
	void> filterinput(T &cur)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curp{static_cast<std::make_signed_t<T>>(cur)};
		if constexpr(isfloatingpoint || !issigned) cur *= 2;
		curp >>= CHAR_BIT * sizeof(T) - 1;
		T curq{static_cast<T>(curp)};
		if constexpr(isfloatingpoint) cur >>= 1;
		else if constexpr(issigned) cur += curq;
		cur ^= curq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		if(issigned) cur <<= 1;
		else cur = rotateleftportable<1>(cur);
	}
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T),
	void> filterinput(T &cur, T *out)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curp{static_cast<std::make_signed_t<T>>(cur)};
		*out = cur;
		if constexpr(isfloatingpoint || !issigned) cur *= 2;
		curp >>= CHAR_BIT * sizeof(T) - 1;
		T curq{static_cast<T>(curp)};
		if constexpr(isfloatingpoint) cur >>= 1;
		else if constexpr(issigned) cur += curq;
		cur ^= curq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*out = cur;
		if(issigned) cur <<= 1;
		else cur = rotateleftportable<1>(cur);
	}else *out = cur;
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T),
	void> filterinput(T &cur, T *out, T *dst)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curp{static_cast<std::make_signed_t<T>>(cur)};
		*out = cur;
		*dst = cur;
		if constexpr(isfloatingpoint || !issigned) cur *= 2;
		curp >>= CHAR_BIT * sizeof(T) - 1;
		T curq{static_cast<T>(curp)};
		if constexpr(isfloatingpoint) cur >>= 1;
		else if constexpr(issigned) cur += curq;
		cur ^= curq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*out = cur;
		*dst = cur;
		if(issigned) cur <<= 1;
		else cur = rotateleftportable<1>(cur);
	}else{
		*out = cur;
		*dst = cur;
	}
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T),
	void> filterinput(T &cura, T &curb)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		if constexpr(isfloatingpoint || !issigned) cura *= 2;
		curap >>= CHAR_BIT * sizeof(T) - 1;
		T curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		if constexpr(isfloatingpoint || !issigned) curb *= 2;
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		T curbq{static_cast<T>(curbp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
		}else if constexpr(issigned){
			cura += curaq;
			curb += curbq;
		}
		cura ^= curaq;
		curb ^= curbq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		if(issigned){
			cura <<= 1;
			curb <<= 1;
		}else{
			cura = rotateleftportable<1>(cura);
			curb = rotateleftportable<1>(curb);
		}
	}
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T),
	void> filterinput(T &cura, T *outa, T &curb, T *outb)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		*outa = cura;
		if constexpr(isfloatingpoint || !issigned) cura *= 2;
		curap >>= CHAR_BIT * sizeof(T) - 1;
		T curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		*outb = curb;
		if constexpr(isfloatingpoint || !issigned) curb *= 2;
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		T curbq{static_cast<T>(curbp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
		}else if constexpr(issigned){
			cura += curaq;
			curb += curbq;
		}
		cura ^= curaq;
		curb ^= curbq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*outa = cura;
		if(issigned) cura <<= 1;
		else cura = rotateleftportable<1>(cura);
		*outb = curb;
		if(issigned) curb <<= 1;
		else curb = rotateleftportable<1>(curb);
	}else{
		*outa = cura;
		*outb = curb;
	}
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T),
	void> filterinput(T &cura, T *outa, T *dsta, T &curb, T *outb, T *dstb)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		*outa = cura;
		*dsta = cura;
		if constexpr(isfloatingpoint || !issigned) cura *= 2;
		curap >>= CHAR_BIT * sizeof(T) - 1;
		T curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		*outb = curb;
		*dstb = curb;
		if constexpr(isfloatingpoint || !issigned) curb *= 2;
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		T curbq{static_cast<T>(curbp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
		}else if constexpr(issigned){
			cura += curaq;
			curb += curbq;
		}
		cura ^= curaq;
		curb ^= curbq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*outa = cura;
		*dsta = cura;
		if(issigned) cura <<= 1;
		else cura = rotateleftportable<1>(cura);
		*outb = curb;
		*dstb = curb;
		if(issigned) curb <<= 1;
		else curb = rotateleftportable<1>(curb);
	}else{
		*outa = cura;
		*dsta = cura;
		*outb = curb;
		*dstb = curb;
	}
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T),
	void> filterinput(T &cura, T &curb, T &curc)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		if constexpr(isfloatingpoint || !issigned) cura *= 2;
		curap >>= CHAR_BIT * sizeof(T) - 1;
		T curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		if constexpr(isfloatingpoint || !issigned) curb *= 2;
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		T curbq{static_cast<T>(curbp)};
		std::make_signed_t<T> curcp{static_cast<std::make_signed_t<T>>(curc)};
		if constexpr(isfloatingpoint || !issigned) curc *= 2;
		curcp >>= CHAR_BIT * sizeof(T) - 1;
		T curcq{static_cast<T>(curcp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
			curc >>= 1;
		}else if constexpr(issigned){
			cura += curaq;
			curb += curbq;
			curc += curcq;
		}
		cura ^= curaq;
		curb ^= curbq;
		curc ^= curcq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		if(issigned){
			cura <<= 1;
			curb <<= 1;
			curc <<= 1;
		}else{
			cura = rotateleftportable<1>(cura);
			curb = rotateleftportable<1>(curb);
			curc = rotateleftportable<1>(curc);
		}
	}
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T),
	void> filterinput(T &cura, T *outa, T &curb, T *outb, T &curc, T *outc)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		*outa = cura;
		if constexpr(isfloatingpoint || !issigned) cura *= 2;
		curap >>= CHAR_BIT * sizeof(T) - 1;
		T curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		*outb = curb;
		if constexpr(isfloatingpoint || !issigned) curb *= 2;
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		T curbq{static_cast<T>(curbp)};
		std::make_signed_t<T> curcp{static_cast<std::make_signed_t<T>>(curc)};
		*outc = curc;
		if constexpr(isfloatingpoint || !issigned) curc *= 2;
		curcp >>= CHAR_BIT * sizeof(T) - 1;
		T curcq{static_cast<T>(curcp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
			curc >>= 1;
		}else if constexpr(issigned){
			cura += curaq;
			curb += curbq;
			curc += curcq;
		}
		cura ^= curaq;
		curb ^= curbq;
		curc ^= curcq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*outa = cura;
		if(issigned) cura <<= 1;
		else cura = rotateleftportable<1>(cura);
		*outb = curb;
		if(issigned) curb <<= 1;
		else curb = rotateleftportable<1>(curb);
		*outc = curc;
		if(issigned) curc <<= 1;
		else curc = rotateleftportable<1>(curc);
	}else{
		*outa = cura;
		*outb = curb;
		*outc = curc;
	}
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T),
	void> filterinput(T &cura, T *outa, T *dsta, T &curb, T *outb, T *dstb, T &curc, T *outc, T *dstc)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		*outa = cura;
		*dsta = cura;
		if constexpr(isfloatingpoint || !issigned) cura *= 2;
		curap >>= CHAR_BIT * sizeof(T) - 1;
		T curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		*outb = curb;
		*dstb = curb;
		if constexpr(isfloatingpoint || !issigned) curb *= 2;
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		T curbq{static_cast<T>(curbp)};
		std::make_signed_t<T> curcp{static_cast<std::make_signed_t<T>>(curc)};
		*outc = curc;
		*dstc = curc;
		if constexpr(isfloatingpoint || !issigned) curc *= 2;
		curcp >>= CHAR_BIT * sizeof(T) - 1;
		T curcq{static_cast<T>(curcp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
			curc >>= 1;
		}else if constexpr(issigned){
			cura += curaq;
			curb += curbq;
			curc += curcq;
		}
		cura ^= curaq;
		curb ^= curbq;
		curc ^= curcq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*outa = cura;
		*dsta = cura;
		if(issigned) cura <<= 1;
		else cura = rotateleftportable<1>(cura);
		*outb = curb;
		*dstb = curb;
		if(issigned) curb <<= 1;
		else curb = rotateleftportable<1>(curb);
		*outc = curc;
		*dstc = curc;
		if(issigned) curc <<= 1;
		else curc = rotateleftportable<1>(curc);
	}else{
		*outa = cura;
		*dsta = cura;
		*outb = curb;
		*dstb = curb;
		*outc = curc;
		*dstc = curc;
	}
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T),
	void> filterinput(T &cura, T &curb, T &curc, T &curd)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		if constexpr(isfloatingpoint || !issigned) cura *= 2;
		curap >>= CHAR_BIT * sizeof(T) - 1;
		T curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		if constexpr(isfloatingpoint || !issigned) curb *= 2;
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		T curbq{static_cast<T>(curbp)};
		std::make_signed_t<T> curcp{static_cast<std::make_signed_t<T>>(curc)};
		if constexpr(isfloatingpoint || !issigned) curc *= 2;
		curcp >>= CHAR_BIT * sizeof(T) - 1;
		T curcq{static_cast<T>(curcp)};
		std::make_signed_t<T> curdp{static_cast<std::make_signed_t<T>>(curd)};
		if constexpr(isfloatingpoint || !issigned) curd *= 2;
		curdp >>= CHAR_BIT * sizeof(T) - 1;
		T curdq{static_cast<T>(curdp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
			curc >>= 1;
			curd >>= 1;
		}else if constexpr(issigned){
			cura += curaq;
			curb += curbq;
			curc += curcq;
			curd += curdq;
		}
		cura ^= curaq;
		curb ^= curbq;
		curc ^= curcq;
		curd ^= curdq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		if(issigned){
			cura <<= 1;
			curb <<= 1;
			curc <<= 1;
			curd <<= 1;
		}else{
			cura = rotateleftportable<1>(cura);
			curb = rotateleftportable<1>(curb);
			curc = rotateleftportable<1>(curc);
			curd = rotateleftportable<1>(curd);
		}
	}
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T),
	void> filterinput(T &cura, T *outa, T &curb, T *outb, T &curc, T *outc, T &curd, T *outd)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		*outa = cura;
		if constexpr(isfloatingpoint || !issigned) cura *= 2;
		curap >>= CHAR_BIT * sizeof(T) - 1;
		T curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		*outb = curb;
		if constexpr(isfloatingpoint || !issigned) curb *= 2;
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		T curbq{static_cast<T>(curbp)};
		std::make_signed_t<T> curcp{static_cast<std::make_signed_t<T>>(curc)};
		*outc = curc;
		if constexpr(isfloatingpoint || !issigned) curc *= 2;
		curcp >>= CHAR_BIT * sizeof(T) - 1;
		T curcq{static_cast<T>(curcp)};
		std::make_signed_t<T> curdp{static_cast<std::make_signed_t<T>>(curd)};
		*outd = curd;
		if constexpr(isfloatingpoint || !issigned) curd *= 2;
		curdp >>= CHAR_BIT * sizeof(T) - 1;
		T curdq{static_cast<T>(curdp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
			curc >>= 1;
			curd >>= 1;
		}else if constexpr(issigned){
			cura += curaq;
			curb += curbq;
			curc += curcq;
			curd += curdq;
		}
		cura ^= curaq;
		curb ^= curbq;
		curc ^= curcq;
		curd ^= curdq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*outa = cura;
		if(issigned) cura <<= 1;
		else cura = rotateleftportable<1>(cura);
		*outb = curb;
		if(issigned) curb <<= 1;
		else curb = rotateleftportable<1>(curb);
		*outc = curc;
		if(issigned) curc <<= 1;
		else curc = rotateleftportable<1>(curc);
		*outd = curd;
		if(issigned) curd <<= 1;
		else curd = rotateleftportable<1>(curd);
	}else{
		*outa = cura;
		*outb = curb;
		*outc = curc;
		*outd = curd;
	}
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T),
	void> filterinput(T &cura, T *outa, T *dsta, T &curb, T *outb, T *dstb, T &curc, T *outc, T *dstc, T &curd, T *outd, T *dstd)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		*outa = cura;
		*dsta = cura;
		if constexpr(isfloatingpoint || !issigned) cura *= 2;
		curap >>= CHAR_BIT * sizeof(T) - 1;
		T curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		*outb = curb;
		*dstb = curb;
		if constexpr(isfloatingpoint || !issigned) curb *= 2;
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		T curbq{static_cast<T>(curbp)};
		std::make_signed_t<T> curcp{static_cast<std::make_signed_t<T>>(curc)};
		*outc = curc;
		*dstc = curc;
		if constexpr(isfloatingpoint || !issigned) curc *= 2;
		curcp >>= CHAR_BIT * sizeof(T) - 1;
		T curcq{static_cast<T>(curcp)};
		std::make_signed_t<T> curdp{static_cast<std::make_signed_t<T>>(curd)};
		*outd = curd;
		*dstd = curd;
		if constexpr(isfloatingpoint || !issigned) curd *= 2;
		curdp >>= CHAR_BIT * sizeof(T) - 1;
		T curdq{static_cast<T>(curdp)};
		if constexpr(isfloatingpoint){
			cura >>= 1;
			curb >>= 1;
			curc >>= 1;
			curd >>= 1;
		}else if constexpr(issigned){
			cura += curaq;
			curb += curbq;
			curc += curcq;
			curd += curdq;
		}
		cura ^= curaq;
		curb ^= curbq;
		curc ^= curcq;
		curd ^= curdq;
	}else if constexpr(isfloatingpoint && absolute){// one-register filtering
		*outa = cura;
		*dsta = cura;
		if(issigned) cura <<= 1;
		else cura = rotateleftportable<1>(cura);
		*outb = curb;
		*dstb = curb;
		if(issigned) curb <<= 1;
		else curb = rotateleftportable<1>(curb);
		*outc = curc;
		*dstc = curc;
		if(issigned) curc <<= 1;
		else curc = rotateleftportable<1>(curc);
		*outd = curd;
		*dstd = curd;
		if(issigned) curd <<= 1;
		else curd = rotateleftportable<1>(curd);
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

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T),
	void> filterinput(T &cura, T &curb, T &curc, T &curd, T &cure, T &curf, T &curg, T &curh)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		if constexpr(isfloatingpoint || !issigned) cura *= 2;
		curap >>= CHAR_BIT * sizeof(T) - 1;
		T curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		if constexpr(isfloatingpoint || !issigned) curb *= 2;
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		T curbq{static_cast<T>(curbp)};
		std::make_signed_t<T> curcp{static_cast<std::make_signed_t<T>>(curc)};
		if constexpr(isfloatingpoint || !issigned) curc *= 2;
		curcp >>= CHAR_BIT * sizeof(T) - 1;
		T curcq{static_cast<T>(curcp)};
		std::make_signed_t<T> curdp{static_cast<std::make_signed_t<T>>(curd)};
		if constexpr(isfloatingpoint || !issigned) curd *= 2;
		curdp >>= CHAR_BIT * sizeof(T) - 1;
		T curdq{static_cast<T>(curdp)};
		std::make_signed_t<T> curep{static_cast<std::make_signed_t<T>>(cure)};
		if constexpr(isfloatingpoint || !issigned) cure *= 2;
		curep >>= CHAR_BIT * sizeof(T) - 1;
		T cureq{static_cast<T>(curep)};
		std::make_signed_t<T> curfp{static_cast<std::make_signed_t<T>>(curf)};
		if constexpr(isfloatingpoint || !issigned) curf *= 2;
		curfp >>= CHAR_BIT * sizeof(T) - 1;
		T curfq{static_cast<T>(curfp)};
		std::make_signed_t<T> curgp{static_cast<std::make_signed_t<T>>(curg)};
		if constexpr(isfloatingpoint || !issigned) curg *= 2;
		curgp >>= CHAR_BIT * sizeof(T) - 1;
		T curgq{static_cast<T>(curgp)};
		std::make_signed_t<T> curhp{static_cast<std::make_signed_t<T>>(curh)};
		if constexpr(isfloatingpoint || !issigned) curh *= 2;
		curhp >>= CHAR_BIT * sizeof(T) - 1;
		T curhq{static_cast<T>(curhp)};
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
			cura += curaq;
			curb += curbq;
			curc += curcq;
			curd += curdq;
			cure += cureq;
			curf += curfq;
			curg += curgq;
			curh += curhq;
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
		if(issigned){
			cura <<= 1;
			curb <<= 1;
			curc <<= 1;
			curd <<= 1;
			cure <<= 1;
			curf <<= 1;
			curg <<= 1;
			curh <<= 1;
		}else{
			cura = rotateleftportable<1>(cura);
			curb = rotateleftportable<1>(curb);
			curc = rotateleftportable<1>(curc);
			curd = rotateleftportable<1>(curd);
			cure = rotateleftportable<1>(cure);
			curf = rotateleftportable<1>(curf);
			curg = rotateleftportable<1>(curg);
			curh = rotateleftportable<1>(curh);
		}
	}
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T),
	void> filterinput(T &cura, T *outa, T &curb, T *outb, T &curc, T *outc, T &curd, T *outd, T &cure, T *oute, T &curf, T *outf, T &curg, T *outg, T &curh, T *outh)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		*outa = cura;
		if constexpr(isfloatingpoint || !issigned) cura *= 2;
		curap >>= CHAR_BIT * sizeof(T) - 1;
		T curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		*outb = curb;
		if constexpr(isfloatingpoint || !issigned) curb *= 2;
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		T curbq{static_cast<T>(curbp)};
		std::make_signed_t<T> curcp{static_cast<std::make_signed_t<T>>(curc)};
		*outc = curc;
		if constexpr(isfloatingpoint || !issigned) curc *= 2;
		curcp >>= CHAR_BIT * sizeof(T) - 1;
		T curcq{static_cast<T>(curcp)};
		std::make_signed_t<T> curdp{static_cast<std::make_signed_t<T>>(curd)};
		*outd = curd;
		if constexpr(isfloatingpoint || !issigned) curd *= 2;
		curdp >>= CHAR_BIT * sizeof(T) - 1;
		T curdq{static_cast<T>(curdp)};
		std::make_signed_t<T> curep{static_cast<std::make_signed_t<T>>(cure)};
		*oute = cure;
		if constexpr(isfloatingpoint || !issigned) cure *= 2;
		curep >>= CHAR_BIT * sizeof(T) - 1;
		T cureq{static_cast<T>(curep)};
		std::make_signed_t<T> curfp{static_cast<std::make_signed_t<T>>(curf)};
		*outf = curf;
		if constexpr(isfloatingpoint || !issigned) curf *= 2;
		curfp >>= CHAR_BIT * sizeof(T) - 1;
		T curfq{static_cast<T>(curfp)};
		std::make_signed_t<T> curgp{static_cast<std::make_signed_t<T>>(curg)};
		*outg = curg;
		if constexpr(isfloatingpoint || !issigned) curg *= 2;
		curgp >>= CHAR_BIT * sizeof(T) - 1;
		T curgq{static_cast<T>(curgp)};
		std::make_signed_t<T> curhp{static_cast<std::make_signed_t<T>>(curh)};
		*outh = curh;
		if constexpr(isfloatingpoint || !issigned) curh *= 2;
		curhp >>= CHAR_BIT * sizeof(T) - 1;
		T curhq{static_cast<T>(curhp)};
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
			cura += curaq;
			curb += curbq;
			curc += curcq;
			curd += curdq;
			cure += cureq;
			curf += curfq;
			curg += curgq;
			curh += curhq;
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
		if(issigned) cura <<= 1;
		else cura = rotateleftportable<1>(cura);
		*outb = curb;
		if(issigned) curb <<= 1;
		else curb = rotateleftportable<1>(curb);
		*outc = curc;
		if(issigned) curc <<= 1;
		else curc = rotateleftportable<1>(curc);
		*outd = curd;
		if(issigned) curd <<= 1;
		else curd = rotateleftportable<1>(curd);
		*oute = cure;
		if(issigned) cure <<= 1;
		else cure = rotateleftportable<1>(cure);
		*outf = curf;
		if(issigned) curf <<= 1;
		else curf = rotateleftportable<1>(curf);
		*outg = curg;
		if(issigned) curg <<= 1;
		else curg = rotateleftportable<1>(curg);
		*outh = curh;
		if(issigned) curh <<= 1;
		else curh = rotateleftportable<1>(curh);
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

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T),
	void> filterinput(T &cura, T *outa, T *dsta, T &curb, T *outb, T *dstb, T &curc, T *outc, T *dstc, T &curd, T *outd, T *dstd, T &cure, T *oute, T *dste, T &curf, T *outf, T *dstf, T &curg, T *outg, T *dstg, T &curh, T *outh, T *dsth)noexcept{
	if constexpr(isfloatingpoint != absolute){// two-register filtering
		std::make_signed_t<T> curap{static_cast<std::make_signed_t<T>>(cura)};
		*outa = cura;
		*dsta = cura;
		if constexpr(isfloatingpoint || !issigned) cura *= 2;
		curap >>= CHAR_BIT * sizeof(T) - 1;
		T curaq{static_cast<T>(curap)};
		std::make_signed_t<T> curbp{static_cast<std::make_signed_t<T>>(curb)};
		*outb = curb;
		*dstb = curb;
		if constexpr(isfloatingpoint || !issigned) curb *= 2;
		curbp >>= CHAR_BIT * sizeof(T) - 1;
		T curbq{static_cast<T>(curbp)};
		std::make_signed_t<T> curcp{static_cast<std::make_signed_t<T>>(curc)};
		*outc = curc;
		*dstc = curc;
		if constexpr(isfloatingpoint || !issigned) curc *= 2;
		curcp >>= CHAR_BIT * sizeof(T) - 1;
		T curcq{static_cast<T>(curcp)};
		std::make_signed_t<T> curdp{static_cast<std::make_signed_t<T>>(curd)};
		*outd = curd;
		*dstd = curd;
		if constexpr(isfloatingpoint || !issigned) curd *= 2;
		curdp >>= CHAR_BIT * sizeof(T) - 1;
		T curdq{static_cast<T>(curdp)};
		std::make_signed_t<T> curep{static_cast<std::make_signed_t<T>>(cure)};
		*oute = cure;
		*dste = cure;
		if constexpr(isfloatingpoint || !issigned) cure *= 2;
		curep >>= CHAR_BIT * sizeof(T) - 1;
		T cureq{static_cast<T>(curep)};
		std::make_signed_t<T> curfp{static_cast<std::make_signed_t<T>>(curf)};
		*outf = curf;
		*dstf = curf;
		if constexpr(isfloatingpoint || !issigned) curf *= 2;
		curfp >>= CHAR_BIT * sizeof(T) - 1;
		T curfq{static_cast<T>(curfp)};
		std::make_signed_t<T> curgp{static_cast<std::make_signed_t<T>>(curg)};
		*outg = curg;
		*dstg = curg;
		if constexpr(isfloatingpoint || !issigned) curg *= 2;
		curgp >>= CHAR_BIT * sizeof(T) - 1;
		T curgq{static_cast<T>(curgp)};
		std::make_signed_t<T> curhp{static_cast<std::make_signed_t<T>>(curh)};
		*outh = curh;
		*dsth = curh;
		if constexpr(isfloatingpoint || !issigned) curh *= 2;
		curhp >>= CHAR_BIT * sizeof(T) - 1;
		T curhq{static_cast<T>(curhp)};
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
			cura += curaq;
			curb += curbq;
			curc += curcq;
			curd += curdq;
			cure += cureq;
			curf += curfq;
			curg += curgq;
			curh += curhq;
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
		if(issigned) cura <<= 1;
		else cura = rotateleftportable<1>(cura);
		*outb = curb;
		*dstb = curb;
		if(issigned) curb <<= 1;
		else curb = rotateleftportable<1>(curb);
		*outc = curc;
		*dstc = curc;
		if(issigned) curc <<= 1;
		else curc = rotateleftportable<1>(curc);
		*outd = curd;
		*dstd = curd;
		if(issigned) curd <<= 1;
		else curd = rotateleftportable<1>(curd);
		*oute = cure;
		*dste = cure;
		if(issigned) cure <<= 1;
		else cure = rotateleftportable<1>(cure);
		*outf = curf;
		*dstf = curf;
		if(issigned) curf <<= 1;
		else curf = rotateleftportable<1>(curf);
		*outg = curg;
		*dstg = curg;
		if(issigned) curg <<= 1;
		else curg = rotateleftportable<1>(curg);
		*outh = curh;
		*dsth = curh;
		if(issigned) curh <<= 1;
		else curh = rotateleftportable<1>(curh);
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

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	std::pair<unsigned, unsigned>> multibyteoffsets(size_t count, size_t offsets[], unsigned paritybool = 0)noexcept{
	// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
	// reversesort is frequently optimised away in this part, e.g.: reversesort * 2 - 1 generates 1 or -1
	//
	// one of the special mode operates differently: absolute floating-point, but negative inputs will sort just below their positive counterparts
	// Bit rotate left by one is the first filtering step to make this possible.
	// Sort as unsigned, just with the least significant bit flipped to complete the filter.
	// As an example of this, the 8-bit sorting pattern in ascending mode:
	// 0b1000'0000 -0.
	// 0b0000'0000 +0.
	// 0b1000'0001 -pow(2., 1 - mantissabitcount - exponentbias)
	// 0b0000'0001 +pow(2., 1 - mantissabitcount - exponentbias)
	// ...
	// 0b1111'1111 -QNaN (maximum)
	// 0b0111'1111 +QNaN (maximum)
	// Determining the starting point depends of several factors here.
	size_t *t{offsets// either aim to cache low-to-high or high-to-low
		+ reversesort * (CHAR_BIT * sizeof(T) * 256 / 8 - 1 - (issigned && !absolute) * 256 / 2)
		- (isfloatingpoint && !issigned && absolute) * (reversesort * 2 - 1)};
	unsigned runsteps{(1 << CHAR_BIT * sizeof(T) / 8) - 1};
	// handle the sign bit, virtually offset the top byte by half the range here
	if constexpr(issigned && !absolute && reversesort){
		size_t offset{*t};// the starting point was already adjusted to the signed range
		*t-- = 0;// the first offset always starts at zero
		unsigned b{count < offset};// carry-out can only happen once per cycle here, so optimise that
		unsigned j{256 / 2 - 1};
		do{
			size_t difference{*t};
			*t = offset;
			t[CHAR_BIT * sizeof(T) * 256 / 8 + 1] = offset - 1;// high half
			--t;
			offset += difference;
			helper::addcarryofless(b, count, difference);
		}while(--j);
		size_t differencemid{t[256]};
		t[256] = offset;
		t[CHAR_BIT * sizeof(T) * 256 / 8 + 1] = offset - 1;// high half
		t += 256 - 1;// offset to the end of the range
		offset += differencemid;
		helper::addcarryofless(b, count, differencemid);
		j = 256 / 2 - 2;
		do{
			size_t difference{*t};
			*t = offset;
			t[CHAR_BIT * sizeof(T) * 256 / 8 + 1] = offset - 1;// high half
			--t;
			offset -= difference;
			helper::addcarryofless(b, count, difference);
		}while(--j);
		helper::addcarryofless(b, count, *t);
		*t = offset;
		t[CHAR_BIT * sizeof(T) * 256 / 8] = count;// high half, the last offset always starts at the end
		t[CHAR_BIT * sizeof(T) * 256 / 8 + 1] = offset - 1;// high half
		t -= 256 / 2 + 1;// offset to the next byte to process
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
			t[CHAR_BIT * sizeof(T) * 256 / 8 - reversesort * 2 + 1] = offset - 1;// odd, high half
			offset += difference;
			helper::addcarryofless(b, count, difference);
			difference = t[reversesort * -6 + 3];// odd
			t[reversesort * -6 + 3] = offset;
			t[CHAR_BIT * sizeof(T) * 256 / 8] = offset - 1;// even, high half
			t += reversesort * -4 + 2;// step forward twice
			offset += difference;
			helper::addcarryofless(b, count, difference);
		}while(--j);
		helper::addcarryofless(b, count, *t);
		*t = offset;// even
		t[CHAR_BIT * sizeof(T) * 256 / 8] = count;// even, high half, the last offset always starts at the end
		t[CHAR_BIT * sizeof(T) * 256 / 8 - reversesort * 2 + 1] = offset - 1;// odd, high half
		t += reversesort * -4 + 2;// step forward twice
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
			t[CHAR_BIT * sizeof(T) * 256 / 8 + reversesort * 2 - 1] = offset - 1;// high half
			t -= reversesort * 2 - 1;
			offset += difference;
			helper::addcarryofless(b, count, difference);
		}while(--j);
		helper::addcarryofless(b, count, *t);
		*t = offset;
		t[CHAR_BIT * sizeof(T) * 256 / 8] = count;// high half, the last offset always starts at the end
		t[CHAR_BIT * sizeof(T) * 256 / 8 + reversesort * 2 - 1] = offset - 1;// high half
		if constexpr(16 < CHAR_BIT * sizeof(T) || !issigned || absolute || reversesort) t -= reversesort * 2 - 1;
		paritybool ^= b;
		runsteps ^= b << k;
	}while((16 == CHAR_BIT * sizeof(T) && issigned && !absolute)? false :
		reversesort? (0 <= --k) : (static_cast<int>(CHAR_BIT * sizeof(T) / 8 - (issigned && !absolute)) > ++k));
	// handle the sign bit, virtually offset the top byte by half the range here
	if constexpr(issigned && !absolute && !reversesort){
		size_t offset{t[256 / 2]};
		t[256 / 2] = 0;// the first offset always starts at zero
		t += 256 / 2 + 1;
		unsigned b{count < offset};// carry-out can only happen once per cycle here, so optimise that
		unsigned j{256 / 2 - 1};
		do{
			size_t difference{*t};
			*t = offset;
			t[CHAR_BIT * sizeof(T) * 256 / 8 - 1] = offset - 1;// high half
			++t;
			offset += difference;
			helper::addcarryofless(b, count, difference);
		}while(--j);
		size_t differencemid{t[-256]};
		t[-256] = offset;
		t[CHAR_BIT * sizeof(T) * 256 / 8 - 1] = offset - 1;// high half
		t -= 256 - 1;// offset to the start of the range
		offset += differencemid;
		helper::addcarryofless(b, count, differencemid);
		j = 256 / 2 - 2;
		do{
			size_t difference{*t};
			*t = offset;
			t[CHAR_BIT * sizeof(T) * 256 / 8 - 1] = offset - 1;// high half
			++t;
			offset += difference;
			helper::addcarryofless(b, count, difference);
		}while(--j);
		helper::addcarryofless(b, count, *t);
		*t = offset;
		t[CHAR_BIT * sizeof(T) * 256 / 8] = count;// high half, the last offset always starts at the end
		t[CHAR_BIT * sizeof(T) * 256 / 8 - 1] = offset - 1;// high half
		paritybool ^= b;
		runsteps ^= b << (CHAR_BIT * sizeof(T) / 8 - 1);
	}
	return{std::make_pair(runsteps, paritybool)};// paritybool will be 1 for when the swap count is odd
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<T> &&
	8 >= CHAR_BIT * sizeof(T) &&
	!std::is_same_v<bool, T>,
	bool> singlebyteoffsets(size_t count, size_t offsets[])noexcept{
	// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
	// reversesort is frequently optimised away in this part, e.g.: reversesort * 2 - 1 generates 1 or -1
	//
	// one of the special mode operates differently: absolute floating-point, but negative inputs will sort just below their positive counterparts
	// Bit rotate left by one is the first filtering step to make this possible.
	// Sort as unsigned, just with the least significant bit flipped to complete the filter.
	// As an example of this, the 8-bit sorting pattern in ascending mode:
	// 0b1000'0000 -0.
	// 0b0000'0000 +0.
	// 0b1000'0001 -pow(2., 1 - mantissabitcount - exponentbias)
	// 0b0000'0001 +pow(2., 1 - mantissabitcount - exponentbias)
	// ...
	// 0b1111'1111 -QNaN (maximum)
	// 0b0111'1111 +QNaN (maximum)
	// Determining the starting point depends of several factors here.
	size_t *t{offsets// either aim to cache low-to-high or high-to-low
		+ reversesort * (CHAR_BIT * sizeof(T) * 256 / 8 - 1 - (issigned && !absolute) * 256 / 2)
		- (isfloatingpoint && !issigned && absolute) * (reversesort * 2 - 1)};
	size_t offset;
	unsigned b;
	// handle the sign bit, virtually offset the top byte by half the range here
	if constexpr(issigned){
		offset = *t;// the starting point was already adjusted to the signed range
		*t = 0;// the first offset always starts at zero
		t -= reversesort * 2 - 1;
		helper::addcarryofless(b, count, offset);
		unsigned j{256 / 2 - 1};
		do{
			size_t difference{*t};
			*t = offset;
			t[CHAR_BIT * sizeof(T) * 256 / 8 + reversesort * 2 - 1] = offset - 1;// high half
			t -= reversesort * 2 - 1;
			offset += difference;
			helper::addcarryofless(b, count, difference);
		}while(--j);
		size_t differencemid{t[256 * (reversesort * 2 - 1)]};
		t[256 * (reversesort * 2 - 1)] = offset;
		t[CHAR_BIT * sizeof(T) * 256 / 8 + reversesort * 2 - 1] = offset - 1;// high half
		t += (256 - 1) * (reversesort * 2 - 1);// offset to the start/end of the range
		offset += differencemid;
		helper::addcarryofless(b, count, differencemid);
		j = 256 / 2 - 2;
		do{
			size_t difference{*t};
			*t = offset;
			t[CHAR_BIT * sizeof(T) * 256 / 8 + reversesort * 2 - 1] = offset - 1;// high half
			t -= reversesort * 2 - 1;
			offset -= difference * (reversesort * 2 - 1);
			helper::addcarryofless(b, count, difference);
		}while(--j);
	}else{// unsigned
		offset = *t;
		*t = 0;// the first offset always starts at zero
		// custom loop for the special mode: absolute floating-point, but negative inputs will sort just below their positive counterparts
		if constexpr(isfloatingpoint && !issigned && absolute){// starts at one removed from the initial index
			t += reversesort * 2 - 1;// step back
			helper::addcarryofless(b, count, offset);
			unsigned j{(256 - 2) / 2};// double the number of items per loop
			do{
				size_t difference{*t};// even
				*t = offset;
				t[CHAR_BIT * sizeof(T) * 256 / 8 - reversesort * 2 + 1] = offset - 1;// odd, high half
				offset += difference;
				helper::addcarryofless(b, count, difference);
				difference = t[reversesort * -6 + 3];// odd
				t[reversesort * -6 + 3] = offset;
				t[CHAR_BIT * sizeof(T) * 256 / 8] = offset - 1;// even, high half
				t += reversesort * -4 + 2;// step forward twice
				offset += difference;
				helper::addcarryofless(b, count, difference);
			}while(--j);
		}else{// all other modes
			t -= reversesort * 2 - 1;
			helper::addcarryofless(b, count, offset);
			unsigned j{256 - 2};
			do{
				size_t difference{*t};
				*t = offset;
				t[CHAR_BIT * sizeof(T) * 256 / 8 + reversesort * 2 - 1] = offset - 1;// high half
				t -= reversesort * 2 - 1;
				offset += difference;
				helper::addcarryofless(b, count, difference);
			}while(--j);
		}
	}
	helper::addcarryofless(b, count, *t);
	if(!b)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
		[[likely]]
#endif
	{
		*t = offset;
		t[CHAR_BIT * sizeof(T) * 256 / 8] = count;// high half, the last offset always starts at the end
		// again, adjust for the special mode
		t[CHAR_BIT * sizeof(T) * 256 / 8 + ((isfloatingpoint && !issigned && absolute) != reversesort) * 2 - 1] = offset - 1;// high half
		return{true};
	}
	return{false};
}

}// namespace helper

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
	return{std::make_pair(buffer, allocsize)};
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

// Function implementation templates for multi-byte types

// radixsortcopynoalloc() function implementation template for multi-byte types without indirection
template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_NORMAL std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T) &&
	!std::is_same_v<bool, T>,
	void> radixsortcopynoalloc(size_t count, T const input[], T output[], T buffer[])noexcept{
	// do not pass a nullptr here, even though it's safe if count is 0
	assert(input);
	assert(output);
	assert(buffer);
	// All the code in this function is adapted for count to be one below its input value here.
	--count;
	if(0 < static_cast<ptrdiff_t>(count)){// a 0 or 1 count array is legal here
		size_t offsets[CHAR_BIT * sizeof(T) * 256 / 8 * 2];// a sizeable amount of indices, but it's worth it, and this function never calls functions either to further increase stack usage anyway
		std::memset(offsets, 0, sizeof(offsets) / 2);// only the low half

		// count the 256 configurations, all in one go
		ptrdiff_t i{static_cast<ptrdiff_t>(count)};
		if /*constexpr*/(64 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T const *pinput{input};
				do{
					T curhi{pinput[0]};
					T curlo{pinput[1]};
					pinput += 2;
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							curhi, output + i, buffer + i,
							curlo, output + i - 1, buffer + i - 1);
					}
					// register pressure performance issue on several platforms: first do the high half here
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					size_t curhi5{static_cast<size_t>(curhi >> (40 - helper::log2ptrs))};
					size_t curhi6{static_cast<size_t>(curhi >> (48 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i] = curhi;
						buffer[i] = curhi;
					}
					curhi >>= 56;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					curhi6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curhi5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + curhi6);
					// register pressure performance issue on several platforms: do the low half here second
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					size_t curlo5{static_cast<size_t>(curlo >> (40 - helper::log2ptrs))};
					size_t curlo6{static_cast<size_t>(curlo >> (48 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i - 1] = curlo;
						buffer[i - 1] = curlo;
					}
					curlo >>= 56;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					curlo6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curlo5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + curlo6);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					T cur{pinput[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, output, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					size_t cur5{static_cast<size_t>(cur >> (40 - helper::log2ptrs))};
					size_t cur6{static_cast<size_t>(cur >> (48 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[0] = cur;
						buffer[0] = cur;
					}
					cur >>= 56;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					cur6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + cur5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + cur6);
				}
			}else{// 8-byte, not in reverse order
				do{
					T curhi{input[i]};
					T curlo{input[i - 1]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							curhi, buffer + i,
							curlo, buffer + i - 1);
					}
					// register pressure performance issue on several platforms: first do the high half here
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					size_t curhi5{static_cast<size_t>(curhi >> (40 - helper::log2ptrs))};
					size_t curhi6{static_cast<size_t>(curhi >> (48 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i] = curhi;
					curhi >>= 56;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					curhi6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curhi5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + curhi6);
					// register pressure performance issue on several platforms: do the low half here second
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					size_t curlo5{static_cast<size_t>(curlo >> (40 - helper::log2ptrs))};
					size_t curlo6{static_cast<size_t>(curlo >> (48 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i - 1] = curlo;
					curlo >>= 56;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					curlo6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curlo5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + curlo6);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					T cur{input[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					size_t cur5{static_cast<size_t>(cur >> (40 - helper::log2ptrs))};
					size_t cur6{static_cast<size_t>(cur >> (48 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[0] = cur;
					cur >>= 56;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					cur6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + cur5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + cur6);
				}
			}
		}else if /*constexpr*/(56 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T const *pinput{input};
				do{
					T curhi{pinput[0]};
					T curlo{pinput[1]};
					pinput += 2;
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							curhi, output + i, buffer + i,
							curlo, output + i - 1, buffer + i - 1);
					}
					// register pressure performance issue on several platforms: first do the high half here
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					size_t curhi5{static_cast<size_t>(curhi >> (40 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i] = curhi;
						buffer[i] = curhi;
					}
					curhi >>= 48;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curhi5);
					// register pressure performance issue on several platforms: do the low half here second
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					size_t curlo5{static_cast<size_t>(curlo >> (40 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i - 1] = curlo;
						buffer[i - 1] = curlo;
					}
					curlo >>= 48;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curlo5);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					T cur{pinput[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, output, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					size_t cur5{static_cast<size_t>(cur >> (40 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[0] = cur;
						buffer[0] = cur;
					}
					cur >>= 48;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + cur5);
				}
			}else{// 7-byte, not in reverse order
				do{
					T curhi{input[i]};
					T curlo{input[i - 1]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							curhi, buffer + i,
							curlo, buffer + i - 1);
					}
					// register pressure performance issue on several platforms: first do the high half here
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					size_t curhi5{static_cast<size_t>(curhi >> (40 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i] = curhi;
					curhi >>= 48;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curhi5);
					// register pressure performance issue on several platforms: do the low half here second
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					size_t curlo5{static_cast<size_t>(curlo >> (40 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i - 1] = curlo;
					curlo >>= 48;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curlo5);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					T cur{input[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					size_t cur5{static_cast<size_t>(cur >> (40 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[0] = cur;
					cur >>= 48;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + cur5);
				}
			}
		}else if /*constexpr*/(48 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T const *pinput{input};
				do{
					T curhi{pinput[0]};
					T curlo{pinput[1]};
					pinput += 2;
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							curhi, output + i, buffer + i,
							curlo, output + i - 1, buffer + i - 1);
					}
					// register pressure performance issue on several platforms: first do the high half here
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i] = curhi;
						buffer[i] = curhi;
					}
					curhi >>= 40;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					// register pressure performance issue on several platforms: do the low half here second
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i - 1] = curlo;
						buffer[i - 1] = curlo;
					}
					curlo >>= 40;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					T cur{pinput[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, output, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[0] = cur;
						buffer[0] = cur;
					}
					cur >>= 40;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
				}
			}else{// 6-byte, not in reverse order
				do{
					T curhi{input[i]};
					T curlo{input[i - 1]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							curhi, buffer + i,
							curlo, buffer + i - 1);
					}
					// register pressure performance issue on several platforms: first do the high half here
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i] = curhi;
					curhi >>= 40;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					// register pressure performance issue on several platforms: do the low half here second
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i - 1] = curlo;
					curlo >>= 40;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					T cur{input[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[0] = cur;
					cur >>= 40;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
				}
			}
		}else if /*constexpr*/(40 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T const *pinput{input};
				do{
					T curhi{pinput[0]};
					T curlo{pinput[1]};
					pinput += 2;
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							curhi, output + i, buffer + i,
							curlo, output + i - 1, buffer + i - 1);
					}
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i] = curhi;
						buffer[i] = curhi;
					}
					curhi >>= 32;
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i - 1] = curlo;
						buffer[i - 1] = curlo;
					}
					curlo >>= 32;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(curhi)];
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					T cur{pinput[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, output, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[0] = cur;
						buffer[0] = cur;
					}
					cur >>= 32;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
				}
			}else{// 5-byte, not in reverse order
				do{
					T curhi{input[i]};
					T curlo{input[i - 1]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							curhi, buffer + i,
							curlo, buffer + i - 1);
					}
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i] = curhi;
					curhi >>= 32;
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i - 1] = curlo;
					curlo >>= 32;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(curhi)];
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					T cur{input[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[0] = cur;
					cur >>= 32;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
				}
			}
		}else if /*constexpr*/(32 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T const *pinput{input};
				do{
					T cura{pinput[0]};
					T curb{pinput[1]};
					pinput += 2;
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, output + i, buffer + i,
							curb, output + i - 1, buffer + i - 1);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					size_t cur2a{static_cast<size_t>(cura) >> (16 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i] = cura;
						buffer[i] = cura;
					}
					cura >>= 24;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					size_t cur2b{static_cast<size_t>(curb) >> (16 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i - 1] = curb;
						buffer[i - 1] = curb;
					}
					curb >>= 24;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					cur2a &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					cur2b &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2b);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					T cur{pinput[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, output, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur) >> (8 - helper::log2ptrs)};
					size_t cur2{static_cast<size_t>(cur) >> (16 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[0] = cur;
						buffer[0] = cur;
					}
					cur >>= 24;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
				}
			}else{// 4-byte, not in reverse order
				do{
					T cura{input[i]};
					T curb{input[i - 1]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, buffer + i,
							curb, buffer + i - 1);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					size_t cur2a{static_cast<size_t>(cura) >> (16 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i] = cura;
					cura >>= 24;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					size_t cur2b{static_cast<size_t>(curb) >> (16 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i - 1] = curb;
					curb >>= 24;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					cur2a &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					cur2b &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2b);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					T cur{input[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur) >> (8 - helper::log2ptrs)};
					size_t cur2{static_cast<size_t>(cur) >> (16 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[0] = cur;
					cur >>= 24;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
				}
			}
		}else if /*constexpr*/(24 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T const *pinput{input};
				i -= 2;
				if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					T cura{pinput[0]};
					T curb{pinput[1]};
					T curc{pinput[2]};
					pinput += 3;
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, output + i + 2, buffer + i + 2,
							curb, output + i + 1, buffer + i + 1,
							curc, output + i, buffer + i);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i + 2] = cura;
						buffer[i + 2] = cura;
					}
					cura >>= 16;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i + 1] = curb;
						buffer[i + 1] = curb;
					}
					curb >>= 16;
					size_t cur0c{static_cast<size_t>(curc) & 0xFFu};
					size_t cur1c{static_cast<size_t>(curc) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i] = curc;
						buffer[i] = curc;
					}
					curc >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(curb)];
					++offsets[cur0c];
					cur1c &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(curc)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1c);
					i -= 3;
				}while(0 <= i);
				if(2 & i){
					T cura{pinput[0]};
					T curb{pinput[1]};
					pinput += 2;
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, output + i + 2, buffer + i + 2,
							curb, output + i + 1, buffer + i + 1);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i + 2] = cura;
						buffer[i + 2] = cura;
					}
					cura >>= 16;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i + 1] = curb;
						buffer[i + 1] = curb;
					}
					curb >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
				}else if(1 & i){// possibly finalize 1 entry after the loop above
					T cur{pinput[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, output, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[0] = cur;
						buffer[0] = cur;
					}
					cur >>= 16;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
				}
			}else{// 2-byte, not in reverse order
				i -= 3;
				if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					T cura{input[i + 2]};
					T curb{input[i + 1]};
					T curc{input[i]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, buffer + i + 2,
							curb, buffer + i + 1,
							curc, buffer + i);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i + 2] = cura;
					cura >>= 16;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i + 1] = curb;
					curb >>= 16;
					size_t cur0c{static_cast<size_t>(curc) & 0xFFu};
					size_t cur1c{static_cast<size_t>(curc) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i] = curc;
					curc >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(curb)];
					++offsets[cur0c];
					cur1c &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(curc)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1c);
					i -= 3;
				}while(0 <= i);
				if(2 & i){// possibly finalize 2 entries after the loop above
					T cura{input[i + 2]};
					T curb{input[i + 1]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, buffer + i + 2,
							curb, buffer + i + 1);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i + 2] = cura;
					cura >>= 16;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i + 1] = curb;
					curb >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
				}else if(1 & i){// possibly finalize 1 entry after the loop above
					T cur{input[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[0] = cur;
					cur >>= 16;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
				}
			}
		}else if /*constexpr*/(16 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T const *pinput{input};
				i -= 3;
				if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					T cura{pinput[0]};
					T curb{pinput[1]};
					T curc{pinput[2]};
					T curd{pinput[3]};
					pinput += 4;
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, output + i + 3, buffer + i + 3,
							curb, output + i + 2, buffer + i + 2,
							curc, output + i + 1, buffer + i + 1,
							curd, output + i, buffer + i);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i + 3] = cura;
						buffer[i + 3] = cura;
					}
					cura >>= 8;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i + 2] = curb;
						buffer[i + 2] = curb;
					}
					curb >>= 8;
					size_t cur0c{static_cast<size_t>(curc) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i + 1] = curc;
						buffer[i + 1] = curc;
					}
					curc >>= 8;
					size_t cur0d{static_cast<size_t>(curd) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i] = curd;
						buffer[i] = curd;
					}
					curd >>= 8;
					++offsets[cur0a];
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[cur0c];
					++offsets[256 + static_cast<size_t>(curc)];
					++offsets[cur0d];
					++offsets[256 + static_cast<size_t>(curd)];
					i -= 4;
				}while(0 <= i);
				if(2 & i){
					T cura{pinput[0]};
					T curb{pinput[1]};
					pinput += 2;
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, output + i + 3, buffer + i + 3,
							curb, output + i + 2, buffer + i + 2);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i + 3] = cura;
						buffer[i + 3] = cura;
					}
					cura >>= 8;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[i + 2] = curb;
						buffer[i + 2] = curb;
					}
					curb >>= 8;
					++offsets[cur0a];
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					++offsets[256 + static_cast<size_t>(curb)];
				}
				if(1 & i){// possibly finalize 1 entry after the loop above
					T cur{pinput[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, output, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						output[0] = cur;
						buffer[0] = cur;
					}
					cur >>= 8;
					++offsets[cur0];
					++offsets[256 + static_cast<size_t>(cur)];
				}
			}else{// 2-byte, not in reverse order
				i -= 3;
				if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					T cura{input[i + 3]};
					T curb{input[i + 2]};
					T curc{input[i + 1]};
					T curd{input[i]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, buffer + i + 3,
							curb, buffer + i + 2,
							curc, buffer + i + 1,
							curd, buffer + i);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i + 3] = cura;
					cura >>= 8;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i + 2] = curb;
					curb >>= 8;
					size_t cur0c{static_cast<size_t>(curc) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i + 1] = curc;
					curc >>= 8;
					size_t cur0d{static_cast<size_t>(curd) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i] = curd;
					curd >>= 8;
					++offsets[cur0a];
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[cur0c];
					++offsets[256 + static_cast<size_t>(curc)];
					++offsets[cur0d];
					++offsets[256 + static_cast<size_t>(curd)];
					i -= 4;
				}while(0 <= i);
				if(2 & i){// possibly finalize 2 entries after the loop above
					T cura{input[i + 3]};
					T curb{input[i + 2]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, buffer + i + 3,
							curb, buffer + i + 2);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i + 3] = cura;
					cura >>= 8;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i + 2] = curb;
					curb >>= 8;
					++offsets[cur0a];
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					++offsets[256 + static_cast<size_t>(curb)];
				}
				if(1 & i){// possibly finalize 1 entry after the loop above
					T cur{input[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[0] = cur;
					cur >>= 8;
					++offsets[cur0];
					++offsets[256 + static_cast<size_t>(cur)];
				}
			}
		}//else static_assert(false, "Implementing larger types will require additional work and optimisation for this library.");

		// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
		auto[runsteps, paritybool]{helper::multibyteoffsets<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, offsets)};

		// perform the bidirectional 8-bit sorting sequence
		if(runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
			[[likely]]
#endif
		{
			T *pdst{buffer}, *pdstnext{output};// for the next iteration
			unsigned shifter{helper::bitscanforwardportable(runsteps)};// at least 1 bit is set inside runsteps as by previous check
			T const *psrclo, *psrchi;
			if /*constexpr*/(true){// useless when not handling indirection: !reverseorder){
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
			shifter *= 8;
			if /*constexpr*/(false){// useless when not handling indirection: reverseorder){
				psrclo = pdstnext;
				psrchi = pdstnext + count;
			}
			for(;;){
				// handle the top byte for floating-point differently
				if /*constexpr*/(!absolute && isfloatingpoint) if(CHAR_BIT * sizeof(T) - 8 == shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
				{
					do{// fill the array, two at a time: one low-to-middle, one high-to-middle
						T outlo{*psrclo++};
						T outhi{*psrchi--};
						auto[curlo, curhi]{helper::filtertopbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo, outhi)};
						size_t offsetlo{poffset[curlo]++};// the next item will be placed one higher
						size_t offsethi{poffset[curhi + CHAR_BIT * sizeof(T) * 256 / 8]--};// the next item will be placed one lower
						pdst[offsetlo] = outlo;
						pdst[offsethi] = outhi;
					}while(psrclo < psrchi);
					if(psrclo == psrchi){// fill in the final item for odd counts
						T outlo{*psrclo};
						size_t curlo{helper::filtertopbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo)};
						size_t offsetlo{poffset[curlo]};
						pdst[offsetlo] = outlo;
					}
					break;// no further processing beyond the top byte
				}
				do{// fill the array, two at a time: one low-to-middle, one high-to-middle
					T outlo{*psrclo++};
					T outhi{*psrchi--};
					auto[curlo, curhi]{helper::filtershiftbyte(outlo, outhi, shifter)};
					size_t offsetlo{poffset[curlo]++};// the next item will be placed one higher
					size_t offsethi{poffset[curhi + CHAR_BIT * sizeof(T) * 256 / 8]--};// the next item will be placed one lower
					pdst[offsetlo] = outlo;
					pdst[offsethi] = outhi;
				}while(psrclo < psrchi);
				if(psrclo == psrchi){// fill in the final item for odd counts
					T outlo{*psrclo};
					size_t curlo{helper::filtershiftbyte(outlo, shifter)};
					size_t offsetlo{poffset[curlo]};
					pdst[offsetlo] = outlo;
				}
				runsteps >>= 1;
				if(!runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
					break;
#if defined(__has_cpp_attribute) && __has_cpp_attribute(maybe_unused)
				[[maybe_unused]]
#endif
				unsigned index;
				if /*constexpr*/(16 < CHAR_BIT * sizeof(T)) index = helper::bitscanforwardportable(runsteps);// at least 1 bit is set inside runsteps as by previous check
				shifter += 8;
				poffset += 256;
				// swap the pointers for the next round, data is moved on each iteration
				psrclo = pdst;
				psrchi = pdst + count;
				pdst = pdstnext;
				pdstnext = const_cast<T *>(psrclo);// never written past the first iteration
				// skip a step if possible
				if /*constexpr*/(16 < CHAR_BIT * sizeof(T)){
					runsteps >>= index;
					shifter += index * 8;
					poffset += static_cast<size_t>(index) * 256;
				}
			}
		}
	}
}

// radixsortnoalloc() function implementation template for multi-byte types without indirection
template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_NORMAL std::enable_if_t<
	std::is_unsigned_v<T> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T) &&
	!std::is_same_v<bool, T>,
	void> radixsortnoalloc(size_t count, T input[], T buffer[], bool movetobuffer = false)noexcept{
	// do not pass a nullptr here, even though it's safe if count is 0
	assert(input);
	assert(buffer);
	// All the code in this function is adapted for count to be one below its input value here.
	--count;
	if(0 < static_cast<ptrdiff_t>(count)){// a 0 or 1 count array is legal here
		size_t offsets[CHAR_BIT * sizeof(T) * 256 / 8 * 2];// a sizeable amount of indices, but it's worth it, and this function never calls functions either to further increase stack usage anyway
		std::memset(offsets, 0, sizeof(offsets) / 2);// only the low half

		// count the 256 configurations, all in one go
		if /*constexpr*/(64 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T *pinputlo{input}, *pinputhi{input + count};
				T *pbufferlo{buffer}, *pbufferhi{buffer + count};
				do{
					T curlo{*pinputlo};
					T curhi{*pinputhi};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							curlo, pinputhi, pbufferhi,
							curhi, pinputlo, pbufferlo);
						--pinputhi;
						--pbufferhi;
						++pinputlo;
						++pbufferlo;
					}
					// register pressure performance issue on several platforms: first do the low half here
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					size_t curlo5{static_cast<size_t>(curlo >> (40 - helper::log2ptrs))};
					size_t curlo6{static_cast<size_t>(curlo >> (48 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint){
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
					++offsets[7 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curlo5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + curlo6);
					// register pressure performance issue on several platforms: do the high half here second
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					size_t curhi5{static_cast<size_t>(curhi >> (40 - helper::log2ptrs))};
					size_t curhi6{static_cast<size_t>(curhi >> (48 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint){
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
					++offsets[7 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curhi5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + curhi6);
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					T cur{*pinputlo};
					// no write to input, as this is the midpoint
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, pbufferhi);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					size_t cur5{static_cast<size_t>(cur >> (40 - helper::log2ptrs))};
					size_t cur6{static_cast<size_t>(cur >> (48 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) *pbufferhi = cur;
					cur >>= 56;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					cur6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + cur5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + cur6);
				}
			}else{// 8-byte, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count)};
				do{
					T curhi{input[i]};
					T curlo{input[i - 1]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							curhi, buffer + i,
							curlo, buffer + i - 1);
					}
					// register pressure performance issue on several platforms: first do the high half here
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					size_t curhi5{static_cast<size_t>(curhi >> (40 - helper::log2ptrs))};
					size_t curhi6{static_cast<size_t>(curhi >> (48 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i] = curhi;
					curhi >>= 56;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					curhi6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curhi5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + curhi6);
					// register pressure performance issue on several platforms: do the low half here second
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					size_t curlo5{static_cast<size_t>(curlo >> (40 - helper::log2ptrs))};
					size_t curlo6{static_cast<size_t>(curlo >> (48 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i - 1] = curlo;
					curlo >>= 56;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					curlo6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curlo5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + curlo6);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					T cur{input[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					size_t cur5{static_cast<size_t>(cur >> (40 - helper::log2ptrs))};
					size_t cur6{static_cast<size_t>(cur >> (48 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[0] = cur;
					cur >>= 56;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					cur6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + cur5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + cur6);
				}
			}
		}else if /*constexpr*/(56 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T *pinputlo{input}, *pinputhi{input + count};
				T *pbufferlo{buffer}, *pbufferhi{buffer + count};
				do{
					T curlo{*pinputlo};
					T curhi{*pinputhi};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							curlo, pinputhi, pbufferhi,
							curhi, pinputlo, pbufferlo);
						--pinputhi;
						--pbufferhi;
						++pinputlo;
						++pbufferlo;
					}
					// register pressure performance issue on several platforms: first do the low half here
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					size_t curlo5{static_cast<size_t>(curlo >> (40 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint){
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
					++offsets[6 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curlo5);
					// register pressure performance issue on several platforms: do the high half here second
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					size_t curhi5{static_cast<size_t>(curhi >> (40 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint){
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
					++offsets[6 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curhi5);
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					T cur{*pinputlo};
					// no write to input, as this is the midpoint
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, pbufferhi);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					size_t cur5{static_cast<size_t>(cur >> (40 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) *pbufferhi = cur;
					cur >>= 48;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + cur5);
				}
			}else{// 7-byte, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count)};
				do{
					T curhi{input[i]};
					T curlo{input[i - 1]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							curhi, buffer + i,
							curlo, buffer + i - 1);
					}
					// register pressure performance issue on several platforms: first do the high half here
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					size_t curhi5{static_cast<size_t>(curhi >> (40 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i] = curhi;
					curhi >>= 48;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curhi5);
					// register pressure performance issue on several platforms: do the low half here second
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					size_t curlo5{static_cast<size_t>(curlo >> (40 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i - 1] = curlo;
					curlo >>= 48;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curlo5);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					T cur{input[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					size_t cur5{static_cast<size_t>(cur >> (40 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[0] = cur;
					cur >>= 48;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + cur5);
				}
			}
		}else if /*constexpr*/(48 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T *pinputlo{input}, *pinputhi{input + count};
				T *pbufferlo{buffer}, *pbufferhi{buffer + count};
				do{
					T curlo{*pinputlo};
					T curhi{*pinputhi};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							curlo, pinputhi, pbufferhi,
							curhi, pinputlo, pbufferlo);
						--pinputhi;
						--pbufferhi;
						++pinputlo;
						++pbufferlo;
					}
					// register pressure performance issue on several platforms: first do the low half here
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						*pinputhi-- = curlo;
						*pbufferhi-- = curlo;
					}
					curlo >>= 40;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					// register pressure performance issue on several platforms: do the high half here second
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						*pinputlo++ = curhi;
						*pbufferlo++ = curhi;
					}
					curhi >>= 40;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					T cur{*pinputlo};
					// no write to input, as this is the midpoint
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, pbufferhi);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) *pbufferhi = cur;
					cur >>= 40;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
				}
			}else{// 6-byte, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count)};
				do{
					T curhi{input[i]};
					T curlo{input[i - 1]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							curhi, buffer + i,
							curlo, buffer + i - 1);
					}
					// register pressure performance issue on several platforms: first do the high half here
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i] = curhi;
					curhi >>= 40;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					// register pressure performance issue on several platforms: do the low half here second
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i - 1] = curlo;
					curlo >>= 40;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					T cur{input[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[0] = cur;
					cur >>= 40;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
				}
			}
		}else if /*constexpr*/(40 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T *pinputlo{input}, *pinputhi{input + count};
				T *pbufferlo{buffer}, *pbufferhi{buffer + count};
				do{
					T curlo{*pinputlo};
					T curhi{*pinputhi};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							curlo, pinputhi, pbufferhi,
							curhi, pinputlo, pbufferlo);
						--pinputhi;
						--pbufferhi;
						++pinputlo;
						++pbufferlo;
					}
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						*pinputhi-- = curlo;
						*pbufferhi-- = curlo;
					}
					curlo >>= 32;
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						*pinputlo++ = curhi;
						*pbufferlo++ = curhi;
					}
					curhi >>= 32;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(curlo)];
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					T cur{*pinputlo};
					// no write to input, as this is the midpoint
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, pbufferhi);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) *pbufferhi = cur;
					cur >>= 32;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
				}
			}else{// 5-byte, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count)};
				do{
					T curhi{input[i]};
					T curlo{input[i - 1]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							curhi, buffer + i,
							curlo, buffer + i - 1);
					}
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i] = curhi;
					curhi >>= 32;
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i - 1] = curlo;
					curlo >>= 32;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(curhi)];
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					T cur{input[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[0] = cur;
					cur >>= 32;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
				}
			}
		}else if /*constexpr*/(32 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T *pinputlo{input}, *pinputhi{input + count};
				T *pbufferlo{buffer}, *pbufferhi{buffer + count};
				do{
					T cura{*pinputlo};
					T curb{*pinputhi};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, pinputhi, pbufferhi,
							curb, pinputlo, pbufferlo);
						--pinputhi;
						--pbufferhi;
						++pinputlo;
						++pbufferlo;
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					size_t cur2a{static_cast<size_t>(cura) >> (16 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						*pinputhi-- = cura;
						*pbufferhi-- = cura;
					}
					cura >>= 24;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					size_t cur2b{static_cast<size_t>(curb) >> (16 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						*pinputlo++ = curb;
						*pbufferlo++ = curb;
					}
					curb >>= 24;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					cur2a &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					cur2b &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2b);
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					T cur{*pinputlo};
					// no write to input, as this is the midpoint
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, pbufferhi);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur) >> (8 - helper::log2ptrs)};
					size_t cur2{static_cast<size_t>(cur) >> (16 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint) *pbufferhi = cur;
					cur >>= 24;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
				}
			}else{// 4-byte, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count)};
				do{
					T cura{input[i]};
					T curb{input[i - 1]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, buffer + i,
							curb, buffer + i - 1);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					size_t cur2a{static_cast<size_t>(cura) >> (16 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i] = cura;
					cura >>= 24;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					size_t cur2b{static_cast<size_t>(curb) >> (16 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i - 1] = curb;
					curb >>= 24;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					cur2a &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					cur2b &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2b);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					T cur{input[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur) >> (8 - helper::log2ptrs)};
					size_t cur2{static_cast<size_t>(cur) >> (16 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[0] = cur;
					cur >>= 24;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
				}
			}
		}else if /*constexpr*/(24 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T *pinputlo{input}, *pinputhi{input + count};
				T *pbufferlo{buffer}, *pbufferhi{buffer + count};
				size_t initialcount{(count + 1) % 6};
				if(4 & initialcount){// possibly initialize with 4 entries before the loop below
					T cura{pinputlo[0]};
					T curb{pinputhi[0]};
					T curc{pinputlo[1]};
					T curd{pinputhi[-1]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, pinputhi, pbufferhi,
							curb, pinputlo, pbufferlo,
							curc, pinputhi - 1, pbufferhi - 1,
							curd, pinputlo + 1, pbufferlo + 1);
						pinputhi -= 2;
						pbufferhi -= 2;
						pinputlo += 2;
						pbufferlo += 2;
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						pinputhi[0] = cura;
						pbufferhi[0] = cura;
					}
					cura >>= 16;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						pinputlo[0] = curb;
						pbufferlo[0] = curb;
					}
					curb >>= 16;
					size_t cur0c{static_cast<size_t>(curc) & 0xFFu};
					size_t cur1c{static_cast<size_t>(curc) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						pinputhi[-1] = curc;
						pinputhi -= 2;
						pbufferhi[-1] = curc;
						pbufferhi -= 2;
					}
					curc >>= 16;
					size_t cur0d{static_cast<size_t>(curd) & 0xFFu};
					size_t cur1d{static_cast<size_t>(curd) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						pinputlo[1] = curd;
						pinputlo += 2;
						pbufferlo[1] = curd;
						pbufferlo += 2;
					}
					curd >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[cur0c];
					cur1c &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(curc)];
					++offsets[cur0d];
					cur1d &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(curd)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1c);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1d);
				}else if(2 & initialcount){// possibly initialize with 2 entries before the loop below
					T cura{*pinputlo};
					T curb{*pinputhi};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, pinputhi, pbufferhi,
							curb, pinputlo, pbufferlo);
						--pinputhi;
						--pbufferhi;
						++pinputlo;
						++pbufferlo;
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						*pinputhi-- = cura;
						*pbufferhi-- = cura;
					}
					cura >>= 16;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						*pinputlo++ = curb;
						*pbufferlo++ = curb;
					}
					curb >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
				}
				if(5 <= count)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					T cura{pinputlo[0]};
					T curb{pinputhi[0]};
					T curc{pinputlo[1]};
					T curd{pinputhi[-1]};
					// register pressure performance issue on several platforms: first do the high half here
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, pinputhi, pbufferhi,
							curb, pinputlo, pbufferlo,
							curc, pinputhi - 1, pbufferhi - 1);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						pinputhi[0] = cura;
						pbufferhi[0] = cura;
					}
					cura >>= 16;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						pinputlo[0] = curb;
						pbufferlo[0] = curb;
					}
					curb >>= 16;
					size_t cur0c{static_cast<size_t>(curc) & 0xFFu};
					size_t cur1c{static_cast<size_t>(curc) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						pinputhi[-1] = curc;
						pbufferhi[-1] = curc;
					}
					curc >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(curb)];
					++offsets[cur0c];
					cur1c &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(curc)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1c);
					T cure{pinputlo[2]};
					T curf{pinputhi[-2]};
					// register pressure performance issue on several platforms: do the low half here second
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							curd, pinputlo + 1, pbufferlo + 1,
							cure, pinputhi - 2, pbufferhi - 2,
							curf, pinputlo + 2, pbufferlo + 2);
						pinputhi -= 3;
						pbufferhi -= 3;
						pinputlo += 3;
						pbufferlo += 3;
					}
					size_t cur0d{static_cast<size_t>(curd) & 0xFFu};
					size_t cur1d{static_cast<size_t>(curd) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						pinputlo[1] = curd;
						pbufferlo[1] = curd;
					}
					curd >>= 16;
					size_t cur0e{static_cast<size_t>(cure) & 0xFFu};
					size_t cur1e{static_cast<size_t>(cure) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						pinputhi[-2] = cure;
						pinputhi -= 3;
						pbufferhi[-2] = cure;
						pbufferhi -= 3;
					}
					cure >>= 16;
					size_t cur0f{static_cast<size_t>(curf) & 0xFFu};
					size_t cur1f{static_cast<size_t>(curf) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						pinputlo[2] = curf;
						pinputlo += 3;
						pbufferlo[2] = curf;
						pbufferlo += 3;
					}
					curf >>= 16;
					++offsets[cur0d];
					cur1d &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(curd)];
					++offsets[cur0e];
					cur1e &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(cure)];
					++offsets[cur0f];
					cur1f &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(curf)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1d);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1e);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1f);
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					T cur{*pinputlo};
					// no write to input, as this is the midpoint
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, pbufferhi);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint) *pbufferhi = cur;
					cur >>= 16;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
				}
			}else{// 3-byte, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count) - 2};
				if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					T cura{input[i + 2]};
					T curb{input[i + 1]};
					T curc{input[i]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, buffer + i + 2,
							curb, buffer + i + 1,
							curc, buffer + i);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i + 3] = cura;
					cura >>= 16;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i + 2] = curb;
					curb >>= 16;
					size_t cur0c{static_cast<size_t>(curc) & 0xFFu};
					size_t cur1c{static_cast<size_t>(curc) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i + 1] = curc;
					curc >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[cur0c];
					cur1c &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(curc)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1c);
					i -= 3;
				}while(0 <= i);
				if(2 & i){// possibly finalize 2 entries after the loop above
					T cura{input[i + 2]};
					T curb{input[i + 1]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, buffer + i + 2,
							curb, buffer + i + 1);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i + 2] = cura;
					cura >>= 16;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i + 1] = curb;
					curb >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
				}
				if(1 & i){// possibly finalize 1 entry after the loop above
					T cur{input[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur) >> (8 - helper::log2ptrs)};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[0] = cur;
					cur >>= 16;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
				}
			}
		}else if /*constexpr*/(16 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
				T *pinputlo{input}, *pinputhi{input + count};
				T *pbufferlo{buffer}, *pbufferhi{buffer + count};
				if(2 & count + 1){// possibly initialize with 2 entries before the loop below
					T cura{*pinputlo};
					T curb{*pinputhi};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, pinputhi, pbufferhi,
							curb, pinputlo, pbufferlo);
						--pinputhi;
						--pbufferhi;
						++pinputlo;
						++pbufferlo;
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						*pinputhi-- = cura;
						*pbufferhi-- = cura;
					}
					cura >>= 8;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						*pinputlo++ = curb;
						*pbufferlo++ = curb;
					}
					curb >>= 8;
					++offsets[cur0a];
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					++offsets[256 + static_cast<size_t>(curb)];
				}
				if(3 <= count)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					T cura{pinputlo[0]};
					T curb{pinputhi[0]};
					T curc{pinputlo[1]};
					T curd{pinputhi[-1]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, pinputhi, pbufferhi,
							curb, pinputlo, pbufferlo,
							curc, pinputhi - 1, pbufferhi - 1,
							curd, pinputlo + 1, pbufferlo + 1);
						pinputhi -= 2;
						pbufferhi -= 2;
						pinputlo += 2;
						pbufferlo += 2;
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						pinputhi[0] = cura;
						pbufferhi[0] = cura;
					}
					cura >>= 8;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						pinputlo[0] = curb;
						pbufferlo[0] = curb;
					}
					curb >>= 8;
					size_t cur0c{static_cast<size_t>(curc) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						pinputhi[-1] = curc;
						pinputhi -= 2;
						pbufferhi[-1] = curc;
						pbufferhi -= 2;
					}
					curc >>= 8;
					size_t cur0d{static_cast<size_t>(curd) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint){
						pinputlo[1] = curd;
						pinputlo += 2;
						pbufferlo[1] = curd;
						pbufferlo += 2;
					}
					curd >>= 8;
					++offsets[cur0a];
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[cur0c];
					++offsets[256 + static_cast<size_t>(curc)];
					++offsets[cur0d];
					++offsets[256 + static_cast<size_t>(curd)];
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					T cur{*pinputlo};
					// no write to input, as this is the midpoint
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, pbufferhi);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint) *pbufferhi = cur;
					cur >>= 8;
					++offsets[cur0];
					++offsets[256 + static_cast<size_t>(cur)];
				}
			}else{// 2-byte, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count) - 3};
				if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					T cura{input[i + 3]};
					T curb{input[i + 2]};
					T curc{input[i + 1]};
					T curd{input[i]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, buffer + i + 3,
							curb, buffer + i + 2,
							curc, buffer + i + 1,
							curd, buffer + i);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i + 3] = cura;
					cura >>= 8;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i + 2] = curb;
					curb >>= 8;
					size_t cur0c{static_cast<size_t>(curc) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i + 1] = curc;
					curc >>= 8;
					size_t cur0d{static_cast<size_t>(curd) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i] = curd;
					curd >>= 8;
					++offsets[cur0a];
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[cur0c];
					++offsets[256 + static_cast<size_t>(curc)];
					++offsets[cur0d];
					++offsets[256 + static_cast<size_t>(curd)];
					i -= 4;
				}while(0 <= i);
				if(2 & i){// possibly finalize 2 entries after the loop above
					T cura{input[i + 3]};
					T curb{input[i + 2]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
							cura, buffer + i + 3,
							curb, buffer + i + 2);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i + 3] = cura;
					cura >>= 8;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i + 2] = curb;
					curb >>= 8;
					++offsets[cur0a];
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					++offsets[256 + static_cast<size_t>(curb)];
				}
				if(1 & i){// possibly finalize 1 entry after the loop above
					T cur{input[0]};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[0] = cur;
					cur >>= 8;
					++offsets[cur0];
					++offsets[256 + static_cast<size_t>(cur)];
				}
			}
		}//else static_assert(false, "Implementing larger types will require additional work and optimisation for this library.");

		// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
		auto[runsteps, paritybool]{helper::multibyteoffsets<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, offsets, movetobuffer)};

		// perform the bidirectional 8-bit sorting sequence
		if(runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
			[[likely]]
#endif
		{
			T *psrclo{input}, *pdst{buffer};
			unsigned shifter{helper::bitscanforwardportable(runsteps)};// at least 1 bit is set inside runsteps as by previous check
			if(paritybool){// swap if the count of sorting actions to do is odd
				psrclo = buffer;
				pdst = input;
			}
			// skip a step if possible
			runsteps >>= shifter;
			size_t *poffset{offsets + static_cast<size_t>(shifter) * 256};
			shifter *= 8;
			T *psrchi{psrclo + count};
			T *pdstnext{psrclo};// for the next iteration
			for(;;){
				// handle the top byte for floating-point differently
				if /*constexpr*/(absolute || isfloatingpoint) if(CHAR_BIT * sizeof(T) - 8 == shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
					{
					do{// fill the array, two at a time: one low-to-middle, one high-to-middle
						T outlo{*psrclo++};
						T outhi{*psrchi--};
						auto[curlo, curhi]{helper::filtertopbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo, outhi)};
						size_t offsetlo{poffset[curlo]++};// the next item will be placed one higher
						size_t offsethi{poffset[curhi + CHAR_BIT * sizeof(T) * 256 / 8]--};// the next item will be placed one lower
						pdst[offsetlo] = outlo;
						pdst[offsethi] = outhi;
					}while(psrclo < psrchi);
					if(psrclo == psrchi){// fill in the final item for odd counts
						T outlo{*psrclo};
						size_t curlo{helper::filtertopbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo)};
						size_t offsetlo{poffset[curlo]};
						pdst[offsetlo] = outlo;
					}
					break;// no further processing beyond the top byte
				}
				do{// fill the array, two at a time: one low-to-middle, one high-to-middle
					T outlo{*psrclo++};
					T outhi{*psrchi--};
					auto[curlo, curhi]{helper::filtershiftbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo, outhi, shifter)};
					size_t offsetlo{poffset[curlo]++};// the next item will be placed one higher
					size_t offsethi{poffset[curhi + CHAR_BIT * sizeof(T) * 256 / 8]--};// the next item will be placed one lower
					pdst[offsetlo] = outlo;
					pdst[offsethi] = outhi;
				}while(psrclo < psrchi);
				if(psrclo == psrchi){// fill in the final item for odd counts
					T outlo{*psrclo};
					size_t curlo{helper::filtershiftbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo, shifter)};
					size_t offsetlo{poffset[curlo]};
					pdst[offsetlo] = outlo;
				}
				runsteps >>= 1;
				if(!runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
					break;
#if defined(__has_cpp_attribute) && __has_cpp_attribute(maybe_unused)
				[[maybe_unused]]
#endif
				unsigned index;
				if /*constexpr*/(16 < CHAR_BIT * sizeof(T)) index = helper::bitscanforwardportable(runsteps);// at least 1 bit is set inside runsteps as by previous check
				shifter += 8;
				poffset += 256;
				// swap the pointers for the next round, data is moved on each iteration
				psrclo = pdst;
				psrchi = pdst + count;
				pdst = pdstnext;
				pdstnext = psrclo;
				// skip a step if possible
				if /*constexpr*/(16 < CHAR_BIT * sizeof(T)){
					runsteps >>= index;
					shifter += index * 8;
					poffset += static_cast<size_t>(index) * 256;
				}
			}
		}
	}
}

// radixsortcopynoalloc() function implementation template for multi-byte types with getter function indirection
template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename V, class... arguments>
RSBD8_FUNC_NORMAL std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_unsigned_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>> &&
	64 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>) &&
	8 < CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>) &&
	!std::is_same_v<bool, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[], arguments &&... getterfuncinput)noexcept{
	using T = std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>;
	// do not pass a nullptr here, even though it's safe if count is 0
	assert(input);
	assert(output);
	assert(buffer);
	// All the code in this function is adapted for count to be one below its input value here.
	--count;
	if(0 < static_cast<ptrdiff_t>(count)){// a 0 or 1 count array is legal here
		size_t offsets[CHAR_BIT * sizeof(T) * 256 / 8 * 2];// a sizeable amount of indices, but it's worth it, and this function never calls functions either to further increase stack usage anyway
		std::memset(offsets, 0, sizeof(offsets) / 2);// only the low half

		// count the 256 configurations, all in one go
		ptrdiff_t i{static_cast<ptrdiff_t>(count)};
		if /*constexpr*/(64 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(reverseorder){// also reverse the array at the same time
				V *const *pinput{input};
				do{
					V *phi{pinput[0]};
					V *plo{pinput[1]};
					pinput += 2;
					output[i] = phi;
					buffer[i] = phi;
					T curhi{(phi->*pgetterfunc)(getterfuncinput...)};
					output[i - 1] = plo;
					buffer[i - 1] = plo;
					T curlo{(plo->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					// register pressure performance issue on several platforms: first do the high half here
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					size_t curhi5{static_cast<size_t>(curhi >> (40 - helper::log2ptrs))};
					size_t curhi6{static_cast<size_t>(curhi >> (48 - helper::log2ptrs))};
					curhi >>= 56;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					curhi6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curhi5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + curhi6);
					// register pressure performance issue on several platforms: do the low half here second
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					size_t curlo5{static_cast<size_t>(curlo >> (40 - helper::log2ptrs))};
					size_t curlo6{static_cast<size_t>(curlo >> (48 - helper::log2ptrs))};
					curlo >>= 56;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					curlo6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curlo5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + curlo6);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{pinput[0]};
					output[i] = p;
					buffer[i] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					size_t cur5{static_cast<size_t>(cur >> (40 - helper::log2ptrs))};
					size_t cur6{static_cast<size_t>(cur >> (48 - helper::log2ptrs))};
					cur >>= 56;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					cur6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + cur5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + cur6);
				}
			}else{// 8-byte, not in reverse order
				do{
					V *phi{input[i]};
					V *plo{input[i - 1]};
					output[i] = phi;
					buffer[i] = phi;
					T curhi{(phi->*pgetterfunc)(getterfuncinput...)};
					output[i - 1] = plo;
					buffer[i - 1] = plo;
					T curlo{(plo->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					// register pressure performance issue on several platforms: first do the high half here
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					size_t curhi5{static_cast<size_t>(curhi >> (40 - helper::log2ptrs))};
					size_t curhi6{static_cast<size_t>(curhi >> (48 - helper::log2ptrs))};
					curhi >>= 56;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					curhi6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curhi5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + curhi6);
					// register pressure performance issue on several platforms: do the low half here second
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					size_t curlo5{static_cast<size_t>(curlo >> (40 - helper::log2ptrs))};
					size_t curlo6{static_cast<size_t>(curlo >> (48 - helper::log2ptrs))};
					curlo >>= 56;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					curlo6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curlo5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + curlo6);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					size_t cur5{static_cast<size_t>(cur >> (40 - helper::log2ptrs))};
					size_t cur6{static_cast<size_t>(cur >> (48 - helper::log2ptrs))};
					cur >>= 56;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					cur6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + cur5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + cur6);
				}
			}
		}else if /*constexpr*/(56 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(reverseorder){// also reverse the array at the same time
				V *const *pinput{input};
				do{
					V *phi{pinput[0]};
					V *plo{pinput[1]};
					pinput += 2;
					output[i] = phi;
					buffer[i] = phi;
					T curhi{(phi->*pgetterfunc)(getterfuncinput...)};
					output[i - 1] = plo;
					buffer[i - 1] = plo;
					T curlo{(plo->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					// register pressure performance issue on several platforms: first do the high half here
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					size_t curhi5{static_cast<size_t>(curhi >> (40 - helper::log2ptrs))};
					curhi >>= 48;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curhi5);
					// register pressure performance issue on several platforms: do the low half here second
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					size_t curlo5{static_cast<size_t>(curlo >> (40 - helper::log2ptrs))};
					curlo >>= 48;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curlo5);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{pinput[0]};
					output[i] = p;
					buffer[i] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					size_t cur5{static_cast<size_t>(cur >> (40 - helper::log2ptrs))};
					cur >>= 48;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + cur5);
				}
			}else{// 7-byte, not in reverse order
				do{
					V *phi{input[i]};
					V *plo{input[i - 1]};
					output[i] = phi;
					buffer[i] = phi;
					T curhi{(phi->*pgetterfunc)(getterfuncinput...)};
					output[i - 1] = plo;
					buffer[i - 1] = plo;
					T curlo{(plo->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					// register pressure performance issue on several platforms: first do the high half here
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					size_t curhi5{static_cast<size_t>(curhi >> (40 - helper::log2ptrs))};
					curhi >>= 48;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curhi5);
					// register pressure performance issue on several platforms: do the low half here second
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					size_t curlo5{static_cast<size_t>(curlo >> (40 - helper::log2ptrs))};
					curlo >>= 48;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curlo5);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					size_t cur5{static_cast<size_t>(cur >> (40 - helper::log2ptrs))};
					cur >>= 48;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + cur5);
				}
			}
		}else if /*constexpr*/(48 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(reverseorder){// also reverse the array at the same time
				V *const *pinput{input};
				do{
					V *phi{pinput[0]};
					V *plo{pinput[1]};
					pinput += 2;
					output[i] = phi;
					buffer[i] = phi;
					T curhi{(phi->*pgetterfunc)(getterfuncinput...)};
					output[i - 1] = plo;
					buffer[i - 1] = plo;
					T curlo{(plo->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					// register pressure performance issue on several platforms: first do the high half here
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					curhi >>= 40;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					// register pressure performance issue on several platforms: do the low half here second
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					curlo >>= 40;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{pinput[0]};
					output[i] = p;
					buffer[i] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					cur >>= 40;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
				}
			}else{// 6-byte, not in reverse order
				do{
					V *phi{input[i]};
					V *plo{input[i - 1]};
					output[i] = phi;
					buffer[i] = phi;
					T curhi{(phi->*pgetterfunc)(getterfuncinput...)};
					output[i - 1] = plo;
					buffer[i - 1] = plo;
					T curlo{(plo->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					// register pressure performance issue on several platforms: first do the high half here
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					curhi >>= 40;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					// register pressure performance issue on several platforms: do the low half here second
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					curlo >>= 40;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					cur >>= 40;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
				}
			}
		}else if /*constexpr*/(40 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(reverseorder){// also reverse the array at the same time
				V *const *pinput{input};
				do{
					V *phi{pinput[0]};
					V *plo{pinput[1]};
					pinput += 2;
					output[i] = phi;
					buffer[i] = phi;
					T curhi{(phi->*pgetterfunc)(getterfuncinput...)};
					output[i - 1] = plo;
					buffer[i - 1] = plo;
					T curlo{(plo->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					curhi >>= 32;
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					curlo >>= 32;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(curhi)];
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{pinput[0]};
					output[i] = p;
					buffer[i] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					cur >>= 32;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
				}
			}else{// 5-byte, not in reverse order
				do{
					V *phi{input[i]};
					V *plo{input[i - 1]};
					output[i] = phi;
					buffer[i] = phi;
					T curhi{(phi->*pgetterfunc)(getterfuncinput...)};
					output[i - 1] = plo;
					buffer[i - 1] = plo;
					T curlo{(plo->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					curhi >>= 32;
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					curlo >>= 32;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(curhi)];
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					cur >>= 32;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
				}
			}
		}else if /*constexpr*/(32 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(reverseorder){// also reverse the array at the same time
				V *const *pinput{input};
				do{
					V *pa{pinput[0]};
					V *pb{pinput[1]};
					pinput += 2;
					output[i] = pa;
					buffer[i] = pa;
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					output[i - 1] = pb;
					buffer[i - 1] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					size_t cur2a{static_cast<size_t>(cura) >> (16 - helper::log2ptrs)};
					cura >>= 24;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					size_t cur2b{static_cast<size_t>(curb) >> (16 - helper::log2ptrs)};
					curb >>= 24;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					cur2a &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					cur2b &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2b);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{pinput[0]};
					output[0] = p;
					buffer[0] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur) >> (8 - helper::log2ptrs)};
					size_t cur2{static_cast<size_t>(cur) >> (16 - helper::log2ptrs)};
					cur >>= 24;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
				}
			}else{// 4-byte, not in reverse order
				do{
					V *pa{input[i]};
					V *pb{input[i - 1]};
					output[i] = pa;
					buffer[i] = pa;
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					output[i - 1] = pb;
					buffer[i - 1] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					size_t cur2a{static_cast<size_t>(cura) >> (16 - helper::log2ptrs)};
					cura >>= 24;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					size_t cur2b{static_cast<size_t>(curb) >> (16 - helper::log2ptrs)};
					curb >>= 24;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					cur2a &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					cur2b &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2b);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur) >> (8 - helper::log2ptrs)};
					size_t cur2{static_cast<size_t>(cur) >> (16 - helper::log2ptrs)};
					cur >>= 24;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
				}
			}
		}else if /*constexpr*/(24 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(reverseorder){// also reverse the array at the same time
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
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					output[i + 2] = pb;
					buffer[i + 2] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					output[i + 1] = pc;
					buffer[i + 1] = pc;
					T curc{(pc->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					cura >>= 16;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					curb >>= 16;
					size_t cur0c{static_cast<size_t>(curc) & 0xFFu};
					size_t cur1c{static_cast<size_t>(curc) >> (8 - helper::log2ptrs)};
					curc >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[cur0c];
					cur1c &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(curc)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1c);
					i -= 3;
				}while(0 <= i);
				if(2 & i){
					V *pa{pinput[0]};
					V *pb{pinput[1]};
					pinput += 2;
					output[i + 3] = pa;
					buffer[i + 3] = pa;
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					output[i + 2] = pb;
					buffer[i + 2] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					cura >>= 16;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					curb >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
				}
				if(1 & i){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					output[0] = p;
					buffer[0] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur) >> (8 - helper::log2ptrs)};
					cur >>= 16;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
				}
			}else{// 3-byte, not in reverse order
				i -= 2;
				if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
					[[likely]]
#endif
					do{
					V *pa{input[i + 2]};
					V *pb{input[i + 1]};
					V *pc{input[i]};
					buffer[i + 2] = pa;
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 1] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					buffer[i] = pc;
					T curc{(pc->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					cura >>= 16;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					curb >>= 16;
					size_t cur0c{static_cast<size_t>(curc) & 0xFFu};
					size_t cur1c{static_cast<size_t>(curc) >> (8 - helper::log2ptrs)};
					curc >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[cur0c];
					cur1c &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(curc)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1c);
					i -= 3;
				}while(0 <= i);
				if(2 & i){// possibly finalize 2 entries after the loop above
					V *pa{input[i + 3]};
					V *pb{input[i + 2]};
					buffer[i + 3] = pa;
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 2] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					cura >>= 16;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					curb >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
				}
				if(1 & i){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur) >> (8 - helper::log2ptrs)};
					cur >>= 16;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
				}
			}
		}else if /*constexpr*/(16 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(reverseorder){// also reverse the array at the same time
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
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					output[i + 2] = pb;
					buffer[i + 2] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					output[i + 1] = pc;
					buffer[i + 1] = pc;
					T curc{(pc->*pgetterfunc)(getterfuncinput...)};
					output[i] = pd;
					buffer[i] = pd;
					T curd{(pd->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					cura >>= 8;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					curb >>= 8;
					size_t cur0c{static_cast<size_t>(curc) & 0xFFu};
					curc >>= 8;
					size_t cur0d{static_cast<size_t>(curd) & 0xFFu};
					curd >>= 8;
					++offsets[cur0a];
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[cur0c];
					++offsets[256 + static_cast<size_t>(curc)];
					++offsets[cur0d];
					++offsets[256 + static_cast<size_t>(curd)];
					i -= 4;
				}while(0 <= i);
				if(2 & i){
					V *pa{pinput[0]};
					V *pb{pinput[1]};
					pinput += 2;
					output[i + 3] = pa;
					buffer[i + 3] = pa;
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					output[i + 2] = pb;
					buffer[i + 2] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					cura >>= 8;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					curb >>= 8;
					++offsets[cur0a];
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					++offsets[256 + static_cast<size_t>(curb)];
				}
				if(1 & i){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					output[0] = p;
					buffer[0] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					cur >>= 8;
					++offsets[cur0];
					++offsets[256 + static_cast<size_t>(cur)];
				}
			}else{// 2-byte, not in reverse order
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
					buffer[i + 3] = pa;
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 2] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 1] = pc;
					T curc{(pc->*pgetterfunc)(getterfuncinput...)};
					buffer[i] = pd;
					T curd{(pd->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					cura >>= 8;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					curb >>= 8;
					size_t cur0c{static_cast<size_t>(curc) & 0xFFu};
					curc >>= 8;
					size_t cur0d{static_cast<size_t>(curd) & 0xFFu};
					curd >>= 8;
					++offsets[cur0a];
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[cur0c];
					++offsets[256 + static_cast<size_t>(curc)];
					++offsets[cur0d];
					++offsets[256 + static_cast<size_t>(curd)];
					i -= 4;
				}while(0 <= i);
				if(2 & i){// possibly finalize 2 entries after the loop above
					V *pa{input[i + 3]};
					V *pb{input[i + 2]};
					buffer[i + 3] = pa;
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 2] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					cura >>= 8;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					curb >>= 8;
					++offsets[cur0a];
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					++offsets[256 + static_cast<size_t>(curb)];
				}
				if(1 & i){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					cur >>= 8;
					++offsets[cur0];
					++offsets[256 + static_cast<size_t>(cur)];
				}
			}
		}//else static_assert(false, "Implementing larger types will require additional work and optimisation for this library.");

		// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
		auto[runsteps, paritybool]{helper::multibyteoffsets<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, offsets)};

		// perform the bidirectional 8-bit sorting sequence
		if(runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
			[[likely]]
#endif
		{
			V **pdst{buffer}, **pdstnext{output};// for the next iteration
			unsigned shifter{helper::bitscanforwardportable(runsteps)};// at least 1 bit is set inside runsteps as by previous check
			V *const *psrclo, *const *psrchi;
			if /*constexpr*/(!reverseorder){
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
			shifter *= 8;
			if /*constexpr*/(reverseorder){
				psrclo = pdstnext;
				psrchi = pdstnext + count;
			}
			for(;;){
				// handle the top byte for floating-point differently
				if /*constexpr*/(absolute || isfloatingpoint) if(CHAR_BIT * sizeof(T) - 8 == shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
					{
					do{// fill the array, two at a time: one low-to-middle, one high-to-middle
						V *plo{*psrclo++};
						V *phi{*psrchi--};
						T outlo{(plo->*pgetterfunc)(getterfuncinput...)};
						T outhi{(phi->*pgetterfunc)(getterfuncinput...)};
						auto[curlo, curhi]{helper::filtertopbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo, outhi)};
						size_t offsetlo{poffset[curlo]++};// the next item will be placed one higher
						size_t offsethi{poffset[curhi + CHAR_BIT * sizeof(T) * 256 / 8]--};// the next item will be placed one lower
						pdst[offsetlo] = plo;
						pdst[offsethi] = phi;
					}while(psrclo < psrchi);
					if(psrclo == psrchi){// fill in the final item for odd counts
						V *plo{*psrclo};
						T outlo{(plo->*pgetterfunc)(getterfuncinput...)};
						size_t curlo{helper::filtertopbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo)};
						size_t offsetlo{poffset[curlo]};
						pdst[offsetlo] = plo;
					}
					break;// no further processing beyond the top byte
				}
				do{// fill the array, two at a time: one low-to-middle, one high-to-middle
					V *plo{*psrclo++};
					V *phi{*psrchi--};
					T outlo{(plo->*pgetterfunc)(getterfuncinput...)};
					T outhi{(phi->*pgetterfunc)(getterfuncinput...)};
					auto[curlo, curhi]{helper::filtershiftbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo, outhi, shifter)};
					size_t offsetlo{poffset[curlo]++};// the next item will be placed one higher
					size_t offsethi{poffset[curhi + CHAR_BIT * sizeof(T) * 256 / 8]--};// the next item will be placed one lower
					pdst[offsetlo] = plo;
					pdst[offsethi] = phi;
				}while(psrclo < psrchi);
				if(psrclo == psrchi){// fill in the final item for odd counts
					V *plo{*psrclo};
					T outlo{(plo->*pgetterfunc)(getterfuncinput...)};
					size_t curlo{helper::filtershiftbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo, shifter)};
					size_t offsetlo{poffset[curlo]};
					pdst[offsetlo] = plo;
				}
				runsteps >>= 1;
				if(!runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
					break;
#if defined(__has_cpp_attribute) && __has_cpp_attribute(maybe_unused)
				[[maybe_unused]]
#endif
				unsigned index;
				if /*constexpr*/(16 < CHAR_BIT * sizeof(T)) index = helper::bitscanforwardportable(runsteps);// at least 1 bit is set inside runsteps as by previous check
				shifter += 8;
				poffset += 256;
				// swap the pointers for the next round, data is moved on each iteration
				psrclo = pdst;
				psrchi = pdst + count;
				pdst = pdstnext;
				pdstnext = const_cast<V **>(psrclo);// never written past the first iteration
				// skip a step if possible
				if /*constexpr*/(16 < CHAR_BIT * sizeof(T)){
					runsteps >>= index;
					shifter += index * 8;
					poffset += static_cast<size_t>(index) * 256;
				}
			}
		}
	}
}

// radixsortnoalloc() function implementation template for multi-byte types with getter function indirection
template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V, class... arguments>
RSBD8_FUNC_NORMAL std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_unsigned_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>> &&
	64 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>) &&
	8 < CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>) &&
	!std::is_same_v<bool, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer = false, arguments &&... getterfuncinput)noexcept{
	using T = std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>;
	// do not pass a nullptr here, even though it's safe if count is 0
	assert(input);
	assert(buffer);
	// All the code in this function is adapted for count to be one below its input value here.
	--count;
	if(0 < static_cast<ptrdiff_t>(count)){// a 0 or 1 count array is legal here
		size_t offsets[CHAR_BIT * sizeof(T) * 256 / 8 * 2];// a sizeable amount of indices, but it's worth it, and this function never calls functions either to further increase stack usage anyway
		std::memset(offsets, 0, sizeof(offsets) / 2);// only the low half

		// count the 256 configurations, all in one go
		if /*constexpr*/(64 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(reverseorder){// also reverse the array at the same time
				V **pinputlo{input}, **pinputhi{input + count};
				V **pbufferlo{buffer}, **pbufferhi{buffer + count};
				do{
					V *plo{*pinputlo};
					V *phi{*pinputhi};
					*pinputhi-- = plo;
					*pbufferhi-- = plo;
					T curlo{(plo->*pgetterfunc)(getterfuncinput...)};
					*pinputlo++ = phi;
					*pbufferlo++ = phi;
					T curhi{(phi->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(curlo, curhi);
					}
					// register pressure performance issue on several platforms: first do the low half here
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					size_t curlo5{static_cast<size_t>(curlo >> (40 - helper::log2ptrs))};
					size_t curlo6{static_cast<size_t>(curlo >> (48 - helper::log2ptrs))};
					curlo >>= 56;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					curlo6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curlo5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + curlo6);
					// register pressure performance issue on several platforms: do the high half here second
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					size_t curhi5{static_cast<size_t>(curhi >> (40 - helper::log2ptrs))};
					size_t curhi6{static_cast<size_t>(curhi >> (48 - helper::log2ptrs))};
					curhi >>= 56;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					curhi6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curhi5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + curhi6);
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					V *p{*pinputlo};
					// no write to input, as this is the midpoint
					*pbufferhi = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					size_t cur5{static_cast<size_t>(cur >> (40 - helper::log2ptrs))};
					size_t cur6{static_cast<size_t>(cur >> (48 - helper::log2ptrs))};
					cur >>= 56;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					cur6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + cur5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + cur6);
				}
			}else{// 8-byte, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count)};
				do{
					V *phi{input[i]};
					V *plo{input[i - 1]};
					buffer[i] = phi;
					T curhi{(phi->*pgetterfunc)(getterfuncinput...)};
					buffer[i - 1] = plo;
					T curlo{(plo->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					// register pressure performance issue on several platforms: first do the high half here
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					size_t curhi5{static_cast<size_t>(curhi >> (40 - helper::log2ptrs))};
					size_t curhi6{static_cast<size_t>(curhi >> (48 - helper::log2ptrs))};
					curhi >>= 56;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					curhi6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curhi5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + curhi6);
					// register pressure performance issue on several platforms: do the low half here second
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					size_t curlo5{static_cast<size_t>(curlo >> (40 - helper::log2ptrs))};
					size_t curlo6{static_cast<size_t>(curlo >> (48 - helper::log2ptrs))};
					curlo >>= 56;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					curlo6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curlo5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + curlo6);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					size_t cur5{static_cast<size_t>(cur >> (40 - helper::log2ptrs))};
					size_t cur6{static_cast<size_t>(cur >> (48 - helper::log2ptrs))};
					cur >>= 56;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					cur6 &= sizeof(void *) * 0xFFu;
					++offsets[7 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + cur5);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 6 * 256) + cur6);
				}
			}
		}else if /*constexpr*/(56 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(reverseorder){// also reverse the array at the same time
				V **pinputlo{input}, **pinputhi{input + count};
				V **pbufferlo{buffer}, **pbufferhi{buffer + count};
				do{
					V *plo{*pinputlo};
					V *phi{*pinputhi};
					*pinputhi-- = plo;
					*pbufferhi-- = plo;
					T curlo{(plo->*pgetterfunc)(getterfuncinput...)};
					*pinputlo++ = phi;
					*pbufferlo++ = phi;
					T curhi{(phi->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(curlo, curhi);
					}
					// register pressure performance issue on several platforms: first do the low half here
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					size_t curlo5{static_cast<size_t>(curlo >> (40 - helper::log2ptrs))};
					curlo >>= 48;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curlo5);
					// register pressure performance issue on several platforms: do the high half here second
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					size_t curhi5{static_cast<size_t>(curhi >> (40 - helper::log2ptrs))};
					curhi >>= 48;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curhi5);
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					V *p{*pinputlo};
					// no write to input, as this is the midpoint
					*pbufferhi = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					size_t cur5{static_cast<size_t>(cur >> (40 - helper::log2ptrs))};
					cur >>= 48;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + cur5);
				}
			}else{// 7-byte, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count)};
				do{
					V *phi{input[i]};
					V *plo{input[i - 1]};
					buffer[i] = phi;
					T curhi{(phi->*pgetterfunc)(getterfuncinput...)};
					buffer[i - 1] = plo;
					T curlo{(plo->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					// register pressure performance issue on several platforms: first do the high half here
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					size_t curhi5{static_cast<size_t>(curhi >> (40 - helper::log2ptrs))};
					curhi >>= 48;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					curhi5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curhi5);
					// register pressure performance issue on several platforms: do the low half here second
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					size_t curlo5{static_cast<size_t>(curlo >> (40 - helper::log2ptrs))};
					curlo >>= 48;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					curlo5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + curlo5);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, buffer);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					size_t cur5{static_cast<size_t>(cur >> (40 - helper::log2ptrs))};
					cur >>= 48;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					cur5 &= sizeof(void *) * 0xFFu;
					++offsets[6 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 5 * 256) + cur5);
				}
			}
		}else if /*constexpr*/(48 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(reverseorder){// also reverse the array at the same time
				V **pinputlo{input}, **pinputhi{input + count};
				V **pbufferlo{buffer}, **pbufferhi{buffer + count};
				do{
					V *plo{*pinputlo};
					V *phi{*pinputhi};
					*pinputhi-- = plo;
					*pbufferhi-- = plo;
					T curlo{(plo->*pgetterfunc)(getterfuncinput...)};
					*pinputlo++ = phi;
					*pbufferlo++ = phi;
					T curhi{(phi->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(curlo, curhi);
					}
					// register pressure performance issue on several platforms: first do the low half here
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					curlo >>= 40;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					// register pressure performance issue on several platforms: do the high half here second
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					curhi >>= 40;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					T cur{*pinputlo};
					// no write to input, as this is the midpoint
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					cur >>= 40;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
				}
			}else{// 6-byte, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count)};
				do{
					V *phi{input[i]};
					V *plo{input[i - 1]};
					buffer[i] = phi;
					T curhi{(phi->*pgetterfunc)(getterfuncinput...)};
					buffer[i - 1] = plo;
					T curlo{(plo->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					// register pressure performance issue on several platforms: first do the high half here
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					size_t curhi4{static_cast<size_t>(curhi >> (32 - helper::log2ptrs))};
					curhi >>= 40;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					curhi4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curhi4);
					// register pressure performance issue on several platforms: do the low half here second
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					size_t curlo4{static_cast<size_t>(curlo >> (32 - helper::log2ptrs))};
					curlo >>= 40;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					curlo4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + curlo4);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					size_t cur4{static_cast<size_t>(cur >> (32 - helper::log2ptrs))};
					cur >>= 40;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					cur4 &= sizeof(void *) * 0xFFu;
					++offsets[5 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 4 * 256) + cur4);
				}
			}
		}else if /*constexpr*/(40 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(reverseorder){// also reverse the array at the same time
				V **pinputlo{input}, **pinputhi{input + count};
				V **pbufferlo{buffer}, **pbufferhi{buffer + count};
				do{
					V *plo{*pinputlo};
					V *phi{*pinputhi};
					*pinputhi-- = plo;
					*pbufferhi-- = plo;
					T curlo{(plo->*pgetterfunc)(getterfuncinput...)};
					*pinputlo++ = phi;
					*pbufferlo++ = phi;
					T curhi{(phi->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(curlo, curhi);
					}
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					curlo >>= 32;
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					curhi >>= 32;
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(curlo)];
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(curhi)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					V *p{*pinputlo};
					// no write to input, as this is the midpoint
					*pbufferhi = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					cur >>= 32;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
				}
			}else{// 5-byte, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count)};
				do{
					V *phi{input[i]};
					V *plo{input[i - 1]};
					buffer[i] = phi;
					T curhi{(phi->*pgetterfunc)(getterfuncinput...)};
					buffer[i - 1] = plo;
					T curlo{(plo->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(curhi, curlo);
					}
					size_t curhi0{static_cast<size_t>(curhi) & 0xFFu};
					size_t curhi1{static_cast<size_t>(curhi >> (8 - helper::log2ptrs))};
					size_t curhi2{static_cast<size_t>(curhi >> (16 - helper::log2ptrs))};
					size_t curhi3{static_cast<size_t>(curhi >> (24 - helper::log2ptrs))};
					if /*constexpr*/(!absolute && !isfloatingpoint) buffer[i] = curhi;
					curhi >>= 32;
					size_t curlo0{static_cast<size_t>(curlo) & 0xFFu};
					size_t curlo1{static_cast<size_t>(curlo >> (8 - helper::log2ptrs))};
					size_t curlo2{static_cast<size_t>(curlo >> (16 - helper::log2ptrs))};
					size_t curlo3{static_cast<size_t>(curlo >> (24 - helper::log2ptrs))};
					curlo >>= 32;
					++offsets[curhi0];
					curhi1 &= sizeof(void *) * 0xFFu;
					curhi2 &= sizeof(void *) * 0xFFu;
					curhi3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(curhi)];
					++offsets[curlo0];
					curlo1 &= sizeof(void *) * 0xFFu;
					curlo2 &= sizeof(void *) * 0xFFu;
					curlo3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(curlo)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curhi1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curhi2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curhi3);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + curlo1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + curlo2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + curlo3);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur >> (8 - helper::log2ptrs))};
					size_t cur2{static_cast<size_t>(cur >> (16 - helper::log2ptrs))};
					size_t cur3{static_cast<size_t>(cur >> (24 - helper::log2ptrs))};
					cur >>= 32;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					cur3 &= sizeof(void *) * 0xFFu;
					++offsets[4 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 3 * 256) + cur3);
				}
			}
		}else if /*constexpr*/(32 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(reverseorder){// also reverse the array at the same time
				V **pinputlo{input}, **pinputhi{input + count};
				V **pbufferlo{buffer}, **pbufferhi{buffer + count};
				do{
					V *pa{*pinputlo};
					V *pb{*pinputhi};
					*pinputhi-- = pa;
					*pbufferhi-- = pa;
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					*pinputlo++ = pb;
					*pbufferlo++ = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					size_t cur2a{static_cast<size_t>(cura) >> (16 - helper::log2ptrs)};
					cura >>= 24;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					size_t cur2b{static_cast<size_t>(curb) >> (16 - helper::log2ptrs)};
					curb >>= 24;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					cur2a &= sizeof(void *) * 0xFFu;
					cur1b &= sizeof(void *) * 0xFFu;
					cur2b &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2b);
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					V *p{*pinputlo};
					// no write to input, as this is the midpoint
					*pbufferhi = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur) >> (8 - helper::log2ptrs)};
					size_t cur2{static_cast<size_t>(cur) >> (16 - helper::log2ptrs)};
					cur >>= 24;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
				}
			}else{// 4-byte, not in reverse order
				ptrdiff_t i{static_cast<ptrdiff_t>(count)};
				do{
					V *pa{input[i]};
					V *pb{input[i - 1]};
					buffer[i] = pa;
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					buffer[i - 1] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					size_t cur2a{static_cast<size_t>(cura) >> (16 - helper::log2ptrs)};
					cura >>= 24;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					size_t cur2b{static_cast<size_t>(curb) >> (16 - helper::log2ptrs)};
					curb >>= 24;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					cur2a &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					cur2b &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2b);
					i -= 2;
				}while(0 < i);
				if(!(1 & i)){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur) >> (8 - helper::log2ptrs)};
					size_t cur2{static_cast<size_t>(cur) >> (16 - helper::log2ptrs)};
					cur >>= 24;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					cur2 &= sizeof(void *) * 0xFFu;
					++offsets[3 * 256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 2 * 256) + cur2);
				}
			}
		}else if /*constexpr*/(24 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(reverseorder){// also reverse the array at the same time
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
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					pinputlo[0] = pb;
					pbufferlo[0] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					pinputhi[1] = pc;
					pinputhi -= 2;
					pbufferhi[1] = pc;
					pbufferhi -= 2;
					T curc{(pc->*pgetterfunc)(getterfuncinput...)};
					pinputlo[-1] = pd;
					pinputlo += 2;
					pbufferlo[-1] = pd;
					pbufferlo += 2;
					T curd{(pd->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					cura >>= 16;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					curb >>= 16;
					size_t cur0c{static_cast<size_t>(curc) & 0xFFu};
					size_t cur1c{static_cast<size_t>(curc) >> (8 - helper::log2ptrs)};
					curc >>= 16;
					size_t cur0d{static_cast<size_t>(curd) & 0xFFu};
					size_t cur1d{static_cast<size_t>(curd) >> (8 - helper::log2ptrs)};
					curd >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[cur0c];
					cur1c &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(curc)];
					++offsets[cur0d];
					cur1d &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(curd)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1c);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1d);
				}else if(2 & initialcount){// possibly initialize with 2 entries before the loop below
					V *pa{pinputlo[0]};
					V *pb{pinputhi[0]};
					*pinputhi-- = pa;
					*pbufferhi-- = pa;
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					*pinputlo++ = pb;
					*pbufferlo++ = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					cura >>= 16;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					curb >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
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
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					pinputlo[0] = pb;
					pbufferlo[0] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					pinputhi[1] = pc;
					pbufferhi[1] = pc;
					T curc{(pc->*pgetterfunc)(getterfuncinput...)};
					// register pressure performance issue on several platforms: first do the high half here
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					cura >>= 16;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					curb >>= 16;
					size_t cur0c{static_cast<size_t>(curc) & 0xFFu};
					size_t cur1c{static_cast<size_t>(curc) >> (8 - helper::log2ptrs)};
					curc >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(curb)];
					++offsets[cur0c];
					cur1c &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(curc)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1c);
					V *pe{pinputlo[2]};
					V *pf{pinputhi[-2]};
					pinputlo[1] = pd;
					pbufferlo[1] = pd;
					T curd{(pd->*pgetterfunc)(getterfuncinput...)};
					pinputhi[-2] = pe;
					pinputhi -= 3;
					pbufferhi[-2] = pe;
					pbufferhi -= 3;
					T cure{(pe->*pgetterfunc)(getterfuncinput...)};
					pinputlo[2] = pf;
					pinputlo += 3;
					pbufferlo[2] = pf;
					pbufferlo += 3;
					T curf{(pf->*pgetterfunc)(getterfuncinput...)};
					// register pressure performance issue on several platforms: do the low half here second
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(curd, cure, curf);
					}
					size_t cur0d{static_cast<size_t>(curd) & 0xFFu};
					size_t cur1d{static_cast<size_t>(curd) >> (8 - helper::log2ptrs)};
					curd >>= 16;
					size_t cur0e{static_cast<size_t>(cure) & 0xFFu};
					size_t cur1e{static_cast<size_t>(cure) >> (8 - helper::log2ptrs)};
					cure >>= 16;
					size_t cur0f{static_cast<size_t>(curf) & 0xFFu};
					size_t cur1f{static_cast<size_t>(curf) >> (8 - helper::log2ptrs)};
					curf >>= 16;
					++offsets[cur0d];
					cur1d &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(curd)];
					++offsets[cur0e];
					cur1e &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(cure)];
					++offsets[cur0f];
					cur1f &= sizeof(void *) * 0xFFu;
					++offsets[2 * 256 + static_cast<size_t>(curf)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1d);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1e);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1f);
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					V *p{*pinputlo};
					// no write to input, as this is the midpoint
					*pbufferhi = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur) >> (8 - helper::log2ptrs)};
					cur >>= 16;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
				}
			}else{// 3-byte, not in reverse order
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
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 1] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					buffer[i] = pc;
					T curc{(pc->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					cura >>= 16;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					curb >>= 16;
					size_t cur0c{static_cast<size_t>(curc) & 0xFFu};
					size_t cur1c{static_cast<size_t>(curc) >> (8 - helper::log2ptrs)};
					curc >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[cur0c];
					cur1c &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(curc)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1c);
					i -= 3;
				}while(0 <= i);
				if(2 & i){// possibly finalize 2 entries after the loop above
					V *pa{input[i + 2]};
					V *pb{input[i + 1]};
					buffer[i + 2] = pa;
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 1] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					size_t cur1a{static_cast<size_t>(cura) >> (8 - helper::log2ptrs)};
					cura >>= 16;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					size_t cur1b{static_cast<size_t>(curb) >> (8 - helper::log2ptrs)};
					curb >>= 16;
					++offsets[cur0a];
					cur1a &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					cur1b &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(curb)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1a);
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1b);
				}
				if(1 & i){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					size_t cur1{static_cast<size_t>(cur) >> (8 - helper::log2ptrs)};
					cur >>= 16;
					++offsets[cur0];
					cur1 &= sizeof(void *) * 0xFFu;
					++offsets[256 + static_cast<size_t>(cur)];
					++*reinterpret_cast<size_t *>(reinterpret_cast<char *>(offsets + 256) + cur1);
				}
			}
		}else if /*constexpr*/(16 == CHAR_BIT * sizeof(T)){
			if /*constexpr*/(reverseorder){// also reverse the array at the same time
				V **pinputlo{input}, **pinputhi{input + count};
				V **pbufferlo{buffer}, **pbufferhi{buffer + count};
				if(2 & count + 1){// possibly initialize with 2 entries before the loop below
					V *pa{*pinputlo};
					V *pb{*pinputhi};
					*pinputhi-- = pa;
					*pbufferhi-- = pa;
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					*pinputlo++ = pb;
					*pbufferlo++ = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					cura >>= 8;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					curb >>= 8;
					++offsets[cur0a];
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
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
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					pinputlo[0] = pb;
					pbufferlo[0] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					pinputhi[-1] = pc;
					pinputhi -= 2;
					pbufferhi[-1] = pc;
					pbufferhi -= 2;
					T curc{(pc->*pgetterfunc)(getterfuncinput...)};
					pinputlo[1] = pd;
					pinputlo += 2;
					pbufferlo[1] = pd;
					pbufferlo += 2;
					T curd{(pd->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					cura >>= 8;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					curb >>= 8;
					size_t cur0c{static_cast<size_t>(curc) & 0xFFu};
					curc >>= 8;
					size_t cur0d{static_cast<size_t>(curd) & 0xFFu};
					curd >>= 8;
					++offsets[cur0a];
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[cur0c];
					++offsets[256 + static_cast<size_t>(curc)];
					++offsets[cur0d];
					++offsets[256 + static_cast<size_t>(curd)];
				}while(pinputlo < pinputhi);
				if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
					V *p{*pinputlo};
					// no write to input, as this is the midpoint
					*pbufferhi = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					cur >>= 8;
					++offsets[cur0];
					++offsets[256 + static_cast<size_t>(cur)];
				}
			}else{// 2-byte, not in reverse order
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
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 2] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 1] = pc;
					T curc{(pc->*pgetterfunc)(getterfuncinput...)};
					buffer[i] = pd;
					T curd{(pd->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					cura >>= 8;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					curb >>= 8;
					size_t cur0c{static_cast<size_t>(curc) & 0xFFu};
					curc >>= 8;
					size_t cur0d{static_cast<size_t>(curd) & 0xFFu};
					curd >>= 8;
					++offsets[cur0a];
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					++offsets[256 + static_cast<size_t>(curb)];
					++offsets[cur0c];
					++offsets[256 + static_cast<size_t>(curc)];
					++offsets[cur0d];
					++offsets[256 + static_cast<size_t>(curd)];
					i -= 4;
				}while(0 <= i);
				if(2 & i){// possibly finalize 2 entries after the loop above
					V *pa{input[i + 3]};
					V *pb{input[i + 2]};
					buffer[i + 3] = pa;
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 2] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb);
					}
					size_t cur0a{static_cast<size_t>(cura) & 0xFFu};
					cura >>= 8;
					size_t cur0b{static_cast<size_t>(curb) & 0xFFu};
					curb >>= 8;
					++offsets[cur0a];
					++offsets[256 + static_cast<size_t>(cura)];
					++offsets[cur0b];
					++offsets[256 + static_cast<size_t>(curb)];
				}
				if(1 & i){// possibly finalize 1 entry after the loop above
					V *p{input[0]};
					buffer[0] = p;
					T cur{(p->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute || isfloatingpoint){
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
					}
					size_t cur0{static_cast<size_t>(cur) & 0xFFu};
					cur >>= 8;
					++offsets[cur0];
					++offsets[256 + static_cast<size_t>(cur)];
				}
			}
		}//else static_assert(false, "Implementing larger types will require additional work and optimisation for this library.");

		// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
		auto[runsteps, paritybool]{helper::multibyteoffsets<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, offsets, movetobuffer)};

		// perform the bidirectional 8-bit sorting sequence
		if(runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
			[[likely]]
#endif
		{
			V **psrclo{input}, **pdst{buffer};
			unsigned shifter{helper::bitscanforwardportable(runsteps)};// at least 1 bit is set inside runsteps as by previous check
			if(paritybool){// swap if the count of sorting actions to do is odd
				psrclo = buffer;
				pdst = input;
			}
			// skip a step if possible
			runsteps >>= shifter;
			size_t *poffset{offsets + static_cast<size_t>(shifter) * 256};
			shifter *= 8;
			V **psrchi{psrclo + count};
			V **pdstnext{psrclo};// for the next iteration
			for(;;){
				// handle the top byte for floating-point differently
				if /*constexpr*/(absolute || isfloatingpoint) if(CHAR_BIT * sizeof(T) - 8 == shifter)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
				{
					do{// fill the array, two at a time: one low-to-middle, one high-to-middle
						V *plo{*psrclo++};
						V *phi{*psrchi--};
						T outlo{(plo->*pgetterfunc)(getterfuncinput...)};
						T outhi{(phi->*pgetterfunc)(getterfuncinput...)};
						auto[curlo, curhi]{helper::filtertopbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo, outhi)};
						size_t offsetlo{poffset[curlo]++};// the next item will be placed one higher
						size_t offsethi{poffset[curhi + CHAR_BIT * sizeof(T) * 256 / 8]--};// the next item will be placed one lower
						pdst[offsetlo] = plo;
						pdst[offsethi] = phi;
					}while(psrclo < psrchi);
					if(psrclo == psrchi){// fill in the final item for odd counts
						V *plo{*psrclo};
						T outlo{(plo->*pgetterfunc)(getterfuncinput...)};
						size_t curlo{helper::filtertopbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo)};
						size_t offsetlo{poffset[curlo]};
						pdst[offsetlo] = plo;
					}
					break;// no further processing beyond the top byte
				}
				do{// fill the array, two at a time: one low-to-middle, one high-to-middle
					V *plo{*psrclo++};
					V *phi{*psrchi--};
					T outlo{(plo->*pgetterfunc)(getterfuncinput...)};
					T outhi{(phi->*pgetterfunc)(getterfuncinput...)};
					auto[curlo, curhi]{helper::filtershiftbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo, outhi, shifter)};
					size_t offsetlo{poffset[curlo]++};// the next item will be placed one higher
					size_t offsethi{poffset[curhi + CHAR_BIT * sizeof(T) * 256 / 8]--};// the next item will be placed one lower
					pdst[offsetlo] = plo;
					pdst[offsethi] = phi;
				}while(psrclo < psrchi);
				if(psrclo == psrchi){// fill in the final item for odd counts
					V *plo{*psrclo};
					T outlo{(plo->*pgetterfunc)(getterfuncinput...)};
					size_t curlo{helper::filtershiftbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo, shifter)};
					size_t offsetlo{poffset[curlo]};
					pdst[offsetlo] = plo;
				}
				runsteps >>= 1;
				if(!runsteps)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely)
					[[unlikely]]
#endif
					break;
#if defined(__has_cpp_attribute) && __has_cpp_attribute(maybe_unused)
				[[maybe_unused]]
#endif
				unsigned index;
				if /*constexpr*/(16 < CHAR_BIT * sizeof(T)) index = helper::bitscanforwardportable(runsteps);// at least 1 bit is set inside runsteps as by previous check
				shifter += 8;
				poffset += 256;
				// swap the pointers for the next round, data is moved on each iteration
				psrclo = pdst;
				psrchi = pdst + count;
				pdst = pdstnext;
				pdstnext = psrclo;
				// skip a step if possible
				if /*constexpr*/(16 < CHAR_BIT * sizeof(T)){
					runsteps >>= index;
					shifter += index * 8;
					poffset += static_cast<size_t>(index) * 256;
				}
			}
		}
	}
}

// Function implementation templates for single-byte types

// radixsortcopynoalloc() function implementation template for single-byte types without indirection
template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_NORMAL std::enable_if_t<
	std::is_unsigned_v<T> &&
	8 >= CHAR_BIT * sizeof(T) &&
	!std::is_same_v<bool, T>,
	void> radixsortcopynoalloc(size_t count, T const input[], T output[])noexcept{
	// do not pass a nullptr here, even though it's safe if count is 0
	assert(input);
	assert(output);
	// All the code in this function is adapted for count to be one below its input value here.
	--count;
	if(0 < static_cast<ptrdiff_t>(count)){// a 0 or 1 count array is legal here
		size_t offsets[CHAR_BIT * sizeof(T) * 256 / 8 * 2];// a sizeable amount of indices, but it's worth it, and this function never calls functions either to further increase stack usage anyway
		std::memset(offsets, 0, sizeof(offsets) / 2);// only the low half

		// count the 256 configurations, all in one go
		ptrdiff_t i{static_cast<ptrdiff_t>(count)};
		if /*constexpr*/(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
			T const *pinput{input};
			i -= 7;
			if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
				[[likely]]
#endif
				do{
				T cura{pinput[0]};
				T curb{pinput[1]};
				T curc{pinput[2]};
				T curd{pinput[3]};
				if /*constexpr*/(absolute != isfloatingpoint){// two-register filters only
					// register pressure performance issue on several platforms: first do the high half here
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
						cura, output + i + 7,
						curb, output + i + 6,
						curc, output + i + 5,
						curd, output + i + 4);
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
				}
				T cure{pinput[4]};
				T curf{pinput[5]};
				T curg{pinput[6]};
				T curh{pinput[7]};
				pinput += 8;
				if /*constexpr*/(absolute != isfloatingpoint){// two-register filters only
					// register pressure performance issue on several platforms: do the low half here second
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
						cure, output + i + 3,
						curf, output + i + 2,
						curg, output + i + 1,
						curh, output + i);
					++offsets[cure];
					++offsets[curf];
					++offsets[curg];
				}else if /*constexpr*/(absolute && isfloatingpoint){// one-register filters only
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
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
					output[i + 7] = cura;
					++offsets[cura];
					output[i + 6] = curb;
					++offsets[curb];
					output[i + 5] = curc;
					++offsets[curc];
					output[i + 4] = curd;
					++offsets[curd];
					output[i + 3] = cure;
					++offsets[cure];
					output[i + 2] = curf;
					++offsets[curf];
					output[i + 1] = curg;
					++offsets[curg];
					output[i] = curh;
				}
				++offsets[curh];
				i -= 8;
			}while(0 <= i);
			if(4 & i){
				T cura{pinput[0]};
				T curb{pinput[1]};
				T curc{pinput[2]};
				T curd{pinput[3]};
				pinput += 4;
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
						cura, output + i + 7,
						curb, output + i + 6,
						curc, output + i + 5,
						curd, output + i + 4);
					i -= 4;// required for the "if(2 & i){" part
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
				}else{
					output[i + 7] = cura;
					++offsets[cura];
					output[i + 6] = curb;
					++offsets[curb];
					output[i + 5] = curc;
					++offsets[curc];
					output[i + 4] = curd;
					i -= 4;// required for the "if(2 & i){" part
				}
				++offsets[curd];
			}
			if(2 & i){
				T cura{pinput[0]};
				T curb{pinput[1]};
				pinput += 2;
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
						cura, output + i + 7,
						curb, output + i + 6);
					++offsets[cura];
				}else{
					output[i + 7] = cura;
					++offsets[cura];
					output[i + 6] = curb;
				}
				++offsets[curb];
			}
			if(1 & i){// possibly finalize 1 entry after the loop above
				T cur{pinput[0]};
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, output);
				}else output[0] = cur;
				++offsets[cur];
			}
		}else{// not in reverse order
			i -= 7;
			if(0 <= i)
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
				[[likely]]
#endif
				do{
				T cura{input[i + 7]};
				T curb{input[i + 6]};
				T curc{input[i + 5]};
				T curd{input[i + 4]};
				if /*constexpr*/(absolute != isfloatingpoint){// two-register filters only
					// register pressure performance issue on several platforms: first do the high half here
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
				}
				T cure{input[i + 3]};
				T curf{input[i + 2]};
				T curg{input[i + 1]};
				T curh{input[i]};
				if /*constexpr*/(absolute != isfloatingpoint){// two-register filters only
					// register pressure performance issue on several platforms: do the low half here second
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cure, curf, curg, curh);
				}else{
					if /*constexpr*/(absolute && isfloatingpoint){// one-register filters only
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd, cure, curf, curg, curh);
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
				T cura{input[i + 7]};
				T curb{input[i + 6]};
				T curc{input[i + 5]};
				T curd{input[i + 4]};
				i -= 4;// required for the "if(2 & i){" part
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
				}
				++offsets[cura];
				++offsets[curb];
				++offsets[curc];
				++offsets[curd];
			}
			if(2 & i){// possibly finalize 2 entries after the loop above
				T cura{input[i + 7]};
				T curb{input[i + 6]};
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb);
				}
				++offsets[cura];
				++offsets[curb];
			}
			if(1 & i){// possibly finalize 1 entry after the loop above
				T cur{input[0]};
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
				}
				++offsets[cur];
			}
		}

		// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
		if(helper::singlebyteoffsets<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, offsets))
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
			[[likely]]
#endif
		{

			// perform the bidirectional 8-bit sorting sequence
			T const *psrclo{input}, *psrchi{input + count};
			do{// fill the array, two at a time: one low-to-middle, one high-to-middle
				T outlo{*psrclo++};
				T outhi{*psrchi--};
				auto[curlo, curhi]{helper::filtertopbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo, outhi)};
				size_t offsetlo, offsethi;// this is only allowed for the single-byte version, containing just one sorting pass
				if /*constexpr*/(false){// useless when not handling indirection: reverseorder){
					offsetlo = offsets[curlo + CHAR_BIT * sizeof(T) * 256 / 8]--;// the next item will be placed one lower
					offsethi = offsets[curhi]++;// the next item will be placed one higher
				}else{
					offsetlo = offsets[curlo]++;// the next item will be placed one higher
					offsethi = offsets[curhi + CHAR_BIT * sizeof(T) * 256 / 8]--;// the next item will be placed one lower
				}
				output[offsetlo] = outlo;
				output[offsethi] = outhi;
			}while(psrclo < psrchi);
			if(psrclo == psrchi){// fill in the final item for odd counts
				T outlo{*psrclo};
				size_t curlo{helper::filtertopbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo)};
				size_t offsetlo;// this is only allowed for the single-byte version, containing just one sorting pass
				if /*constexpr*/(false){// useless when not handling indirection: reverseorder){
					offsetlo = offsets[curlo + CHAR_BIT * sizeof(T) * 256 / 8];
				}else{
					offsetlo = offsets[curlo];
				}
				output[offsetlo] = outlo;
			}
		}
	}
}

// radixsortnoalloc() function implementation template for single-byte types without indirection
template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_NORMAL std::enable_if_t<
	std::is_unsigned_v<T> &&
	8 >= CHAR_BIT * sizeof(T) &&
	!std::is_same_v<bool, T>,
	void> radixsortnoalloc(size_t count, T input[], T buffer[])noexcept{
	// do not pass a nullptr here, even though it's safe if count is 0
	assert(input);
	assert(buffer);
	// All the code in this function is adapted for count to be one below its input value here.
	--count;
	if(0 < static_cast<ptrdiff_t>(count)){// a 0 or 1 count array is legal here
		size_t offsets[CHAR_BIT * sizeof(T) * 256 / 8 * 2];// a sizeable amount of indices, but it's worth it, and this function never calls functions either to further increase stack usage anyway
		std::memset(offsets, 0, sizeof(offsets) / 2);// only the low half

		// count the 256 configurations, all in one go
		if /*constexpr*/(false){// useless when not handling indirection: reverseorder){// also reverse the array at the same time
			T *pinputlo{input}, *pinputhi{input + count};
			T *pbufferlo{buffer}, *pbufferhi{buffer + count};
			if(4 & count + 1){// possibly initialize with 4 entries before the loop below
				T cura{pinputlo[0]};
				T curb{pinputhi[0]};
				T curc{pinputlo[1]};
				T curd{pinputhi[-1]};
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
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
					pinputhi[0] = cura;
					pbufferhi[0] = cura;
					++offsets[cura];
					pinputlo[0] = curb;
					pbufferlo[0] = curb;
					++offsets[curb];
					pinputhi[-1] = curc;
					pinputhi -= 2;
					pbufferhi[-1] = curc;
					pbufferhi -= 2;
					++offsets[curc];
					pinputlo[1] = curd;
					pinputlo += 2;
					pbufferlo[1] = curd;
					pbufferlo += 2;
				}
				++offsets[curd];
			}
			if(2 & count + 1){// possibly initialize with 2 entries before the loop below
				T cura{*pinputlo};
				T curb{*pinputhi};
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
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
				T cura{pinputlo[0]};
				T curb{pinputhi[0]};
				T curc{pinputlo[1]};
				T curd{pinputhi[-1]};
				if /*constexpr*/(absolute != isfloatingpoint){// two-register filters only
					// register pressure performance issue on several platforms: first do the high half here
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
						cura, pinputhi, pbufferhi,
						curb, pinputlo, pbufferlo,
						curc, pinputhi - 1, pbufferhi - 1,
						curd, pinputlo + 1, pbufferlo + 1);
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
				}
				T cure{pinputlo[2]};
				T curf{pinputhi[-2]};
				T curg{pinputlo[3]};
				T curh{pinputhi[-3]};
				if /*constexpr*/(absolute != isfloatingpoint){// two-register filters only
					// register pressure performance issue on several platforms: do the low half here second
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
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
				}else if /*constexpr*/(absolute && isfloatingpoint){// one-register filters only
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
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
					pinputhi[0] = cura;
					pbufferhi[0] = cura;
					++offsets[cura];
					pinputlo[0] = curb;
					pbufferlo[0] = curb;
					++offsets[curb];
					pinputhi[-1] = curc;
					pbufferhi[-1] = curc;
					++offsets[curc];
					pinputlo[1] = curd;
					pbufferlo[1] = curd;
					++offsets[curd];
					pinputhi[-2] = cure;
					pbufferhi[-2] = cure;
					++offsets[cure];
					pinputlo[2] = curf;
					pbufferlo[2] = curf;
					++offsets[curf];
					pinputhi[-3] = curg;
					pinputhi -= 4;
					pbufferhi[-3] = curg;
					pbufferhi -= 4;
					++offsets[curg];
					pinputlo[3] = curh;
					pinputlo += 4;
					pbufferlo[3] = curh;
					pbufferlo += 4;
				}
				++offsets[curh];
			}while(pinputlo < pinputhi);
			if(pinputlo == pinputhi){// possibly finalize 1 entry after the loop above
				T cur{*pinputlo};
				// no write to input, as this is the midpoint
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, pbufferhi);
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
				T cura{input[i + 7]};
				T curb{input[i + 6]};
				T curc{input[i + 5]};
				T curd{input[i + 4]};
				if /*constexpr*/(absolute != isfloatingpoint){// two-register filters only
					// register pressure performance issue on several platforms: first do the high half here
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
						cura, buffer + i + 7,
						curb, buffer + i + 6,
						curc, buffer + i + 5,
						curd, buffer + i + 4);
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
				}
				T cure{input[i + 3]};
				T curf{input[i + 2]};
				T curg{input[i + 1]};
				T curh{input[i]};
				if /*constexpr*/(absolute != isfloatingpoint){// two-register filters only
					// register pressure performance issue on several platforms: do the low half here second
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
						cure, buffer + i + 3,
						curf, buffer + i + 2,
						curg, buffer + i + 1,
						curh, buffer + i);
					++offsets[cure];
					++offsets[curf];
					++offsets[curg];
				}else if /*constexpr*/(absolute && isfloatingpoint){// one-register filters only
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
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
					buffer[i + 7] = cura;
					++offsets[cura];
					buffer[i + 6] = curb;
					++offsets[curb];
					buffer[i + 5] = curc;
					++offsets[curc];
					buffer[i + 4] = curd;
					++offsets[curd];
					buffer[i + 3] = cure;
					++offsets[cure];
					buffer[i + 2] = curf;
					++offsets[curf];
					buffer[i + 1] = curg;
					++offsets[curg];
					buffer[i] = curh;
				}
				++offsets[curh];
				i -= 8;
			}while(0 <= i);
			if(4 & i){// possibly finalize 4 entries after the loop above
				T cura{input[i + 7]};
				T curb{input[i + 6]};
				T curc{input[i + 5]};
				T curd{input[i + 4]};
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
						cura, buffer + i + 7,
						curb, buffer + i + 6,
						curc, buffer + i + 5,
						curd, buffer + i + 4);
					i -= 4;// required for the "if(2 & i){" part
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
				}else{
					buffer[i + 7] = cura;
					++offsets[cura];
					buffer[i + 6] = curb;
					++offsets[curb];
					buffer[i + 5] = curc;
					++offsets[curc];
					buffer[i + 4] = curd;
					i -= 4;// required for the "if(2 & i){" part
				}
				++offsets[curd];
			}
			if(2 & i){// possibly finalize 2 entries after the loop above
				T cura{input[i + 7]};
				T curb{input[i + 6]};
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(
						cura, buffer + i + 7,
						curb, buffer + i + 6);
					++offsets[cura];
				}else{
					buffer[i + 7] = cura;
					++offsets[cura];
					buffer[i + 6] = curb;
				}
				++offsets[curb];
			}
			if(1 & i){// possibly finalize 1 entry after the loop above
				T cur{input[0]};
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur, buffer);
				}else buffer[0] = cur;
				++offsets[cur];
			}
		}

		// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
		if(helper::singlebyteoffsets<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, offsets))
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
			[[likely]]
#endif
		{

			// perform the bidirectional 8-bit sorting sequence
			T *psrclo{buffer}, *psrchi{buffer + count};
			do{// fill the array, two at a time: one low-to-middle, one high-to-middle
				T outlo{*psrclo++};
				T outhi{*psrchi--};
				auto[curlo, curhi]{helper::filtertopbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo, outhi)};
				size_t offsetlo, offsethi;// this is only allowed for the single-byte version, containing just one sorting pass
				if /*constexpr*/(false){// useless when not handling indirection: reverseorder){
					offsetlo = offsets[curlo + CHAR_BIT * sizeof(T) * 256 / 8]--;// the next item will be placed one lower
					offsethi = offsets[curhi]++;// the next item will be placed one higher
				}else{
					offsetlo = offsets[curlo]++;// the next item will be placed one higher
					offsethi = offsets[curhi + CHAR_BIT * sizeof(T) * 256 / 8]--;// the next item will be placed one lower
				}
				input[offsetlo] = outlo;
				input[offsethi] = outhi;
			}while(psrclo < psrchi);
			if(psrclo == psrchi){// fill in the final item for odd counts
				T outlo{*psrclo};
				size_t curlo{helper::filtertopbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo)};
				size_t offsetlo;// this is only allowed for the single-byte version, containing just one sorting pass
				if /*constexpr*/(false){// useless when not handling indirection: reverseorder){
					offsetlo = offsets[curlo + CHAR_BIT * sizeof(T) * 256 / 8];
				}else{
					offsetlo = offsets[curlo];
				}
				input[offsetlo] = outlo;
			}
		}
	}
}

// radixsortcopynoalloc() function implementation template for single-byte types with getter function indirection
template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V, class... arguments>
RSBD8_FUNC_NORMAL std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_unsigned_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>> &&
	8 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>) &&
	!std::is_same_v<bool, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], arguments &&... getterfuncinput)noexcept{
	using T = std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>;
	// do not pass a nullptr here, even though it's safe if count is 0
	assert(input);
	assert(output);
	// All the code in this function is adapted for count to be one below its input value here.
	--count;
	if(0 < static_cast<ptrdiff_t>(count)){// a 0 or 1 count array is legal here
		size_t offsets[CHAR_BIT * sizeof(T) * 256 / 8 * 2];// a sizeable amount of indices, but it's worth it, and this function never calls functions either to further increase stack usage anyway
		std::memset(offsets, 0, sizeof(offsets) / 2);// only the low half

		// count the 256 configurations, all in one go
		ptrdiff_t i{static_cast<ptrdiff_t>(count)};
		if /*constexpr*/(reverseorder){// also reverse the array at the same time
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
				if /*constexpr*/(absolute != isfloatingpoint){// two-register filters only
					output[i + 7] = pa;
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					output[i + 6] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					output[i + 5] = pc;
					T curc{(pc->*pgetterfunc)(getterfuncinput...)};
					output[i + 4] = pd;
					T curd{(pd->*pgetterfunc)(getterfuncinput...)};
					// register pressure performance issue on several platforms: first do the high half here
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
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
				if /*constexpr*/(absolute != isfloatingpoint){// two-register filters only
					output[i + 3] = pe;
					T cure{(pe->*pgetterfunc)(getterfuncinput...)};
					output[i + 2] = pf;
					T curf{(pf->*pgetterfunc)(getterfuncinput...)};
					output[i + 1] = pg;
					T curg{(pg->*pgetterfunc)(getterfuncinput...)};
					output[i] = ph;
					T curh{(ph->*pgetterfunc)(getterfuncinput...)};
					// register pressure performance issue on several platforms: do the low half here second
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cure, curf, curg, curh);
					++offsets[cure];
					++offsets[curf];
					++offsets[curg];
					++offsets[curh];
				}else{
					output[i + 7] = pa;
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					output[i + 6] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					output[i + 5] = pc;
					T curc{(pc->*pgetterfunc)(getterfuncinput...)};
					output[i + 4] = pd;
					T curd{(pd->*pgetterfunc)(getterfuncinput...)};
					output[i + 3] = pe;
					T cure{(pe->*pgetterfunc)(getterfuncinput...)};
					output[i + 2] = pf;
					T curf{(pf->*pgetterfunc)(getterfuncinput...)};
					output[i + 1] = pg;
					T curg{(pg->*pgetterfunc)(getterfuncinput...)};
					output[i] = ph;
					T curh{(ph->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute && isfloatingpoint){// one-register filters only
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd, cure, curf, curg, curh);
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
				T cura{(pa->*pgetterfunc)(getterfuncinput...)};
				output[i + 6] = pb;
				T curb{(pb->*pgetterfunc)(getterfuncinput...)};
				output[i + 5] = pc;
				T curc{(pc->*pgetterfunc)(getterfuncinput...)};
				output[i + 4] = pd;
				i -= 4;// required for the "if(2 & i){" part
				T curd{(pd->*pgetterfunc)(getterfuncinput...)};
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
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
				T cura{(pa->*pgetterfunc)(getterfuncinput...)};
				output[i + 6] = pb;
				T curb{(pb->*pgetterfunc)(getterfuncinput...)};
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb);
				}
				++offsets[cura];
				++offsets[curb];
			}
			if(1 & i){// possibly finalize 1 entry after the loop above
				V *p{pinput[0]};
				output[0] = p;
				T cur{(p->*pgetterfunc)(getterfuncinput...)};
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
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
				if /*constexpr*/(absolute != isfloatingpoint){// two-register filters only
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					T curc{(pc->*pgetterfunc)(getterfuncinput...)};
					T curd{(pd->*pgetterfunc)(getterfuncinput...)};
					// register pressure performance issue on several platforms: first do the high half here
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
				}
				V *pe{input[i + 3]};
				V *pf{input[i + 2]};
				V *pg{input[i + 1]};
				V *ph{input[i]};
				if /*constexpr*/(absolute != isfloatingpoint){// two-register filters only
					T cure{(pe->*pgetterfunc)(getterfuncinput...)};
					T curf{(pf->*pgetterfunc)(getterfuncinput...)};
					T curg{(pg->*pgetterfunc)(getterfuncinput...)};
					T curh{(ph->*pgetterfunc)(getterfuncinput...)};
					// register pressure performance issue on several platforms: do the low half here second
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cure, curf, curg, curh);
					++offsets[cure];
					++offsets[curf];
					++offsets[curg];
					++offsets[curh];
				}else{
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					T curc{(pc->*pgetterfunc)(getterfuncinput...)};
					T curd{(pd->*pgetterfunc)(getterfuncinput...)};
					T cure{(pe->*pgetterfunc)(getterfuncinput...)};
					T curf{(pf->*pgetterfunc)(getterfuncinput...)};
					T curg{(pg->*pgetterfunc)(getterfuncinput...)};
					T curh{(ph->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute && isfloatingpoint){// one-register filters only
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd, cure, curf, curg, curh);
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
				T cura{(pa->*pgetterfunc)(getterfuncinput...)};
				T curb{(pb->*pgetterfunc)(getterfuncinput...)};
				T curc{(pc->*pgetterfunc)(getterfuncinput...)};
				T curd{(pd->*pgetterfunc)(getterfuncinput...)};
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
				}
				++offsets[cura];
				++offsets[curb];
				++offsets[curc];
				++offsets[curd];
			}
			if(2 & i){// possibly finalize 2 entries after the loop above
				V *pa{input[i + 7]};
				V *pb{input[i + 6]};
				T cura{(pa->*pgetterfunc)(getterfuncinput...)};
				T curb{(pb->*pgetterfunc)(getterfuncinput...)};
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb);
				}
				++offsets[cura];
				++offsets[curb];
			}
			if(1 & i){// possibly finalize 1 entry after the loop above
				V *p{input[0]};
				T cur{(p->*pgetterfunc)(getterfuncinput...)};
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
				}
				++offsets[cur];
			}
		}

		// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
		if(helper::singlebyteoffsets<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, offsets))
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
			[[likely]]
#endif
		{

			// perform the bidirectional 8-bit sorting sequence
			V *const *psrclo{input}, *const *psrchi{input + count};
			do{// fill the array, two at a time: one low-to-middle, one high-to-middle
				V *plo{*psrclo++};
				V *phi{*psrchi--};
				T outlo{(plo->*pgetterfunc)(getterfuncinput...)};
				T outhi{(phi->*pgetterfunc)(getterfuncinput...)};
				auto[curlo, curhi]{helper::filtertopbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo, outhi)};
				size_t offsetlo, offsethi;// this is only allowed for the single-byte version, containing just one sorting pass
				if /*constexpr*/(reverseorder){
					offsetlo = offsets[curlo + CHAR_BIT * sizeof(T) * 256 / 8]--;// the next item will be placed one lower
					offsethi = offsets[curhi]++;// the next item will be placed one higher
				}else{
					offsetlo = offsets[curlo]++;// the next item will be placed one higher
					offsethi = offsets[curhi + CHAR_BIT * sizeof(T) * 256 / 8]--;// the next item will be placed one lower
				}
				output[offsetlo] = plo;
				output[offsethi] = phi;
			}while(psrclo < psrchi);
			if(psrclo == psrchi){// fill in the final item for odd counts
				V *plo{*psrclo};
				T outlo{(plo->*pgetterfunc)(getterfuncinput...)};
				size_t curlo{helper::filtertopbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo)};
				size_t offsetlo;// this is only allowed for the single-byte version, containing just one sorting pass
				if /*constexpr*/(reverseorder){
					offsetlo = offsets[curlo + CHAR_BIT * sizeof(T) * 256 / 8];
				}else{
					offsetlo = offsets[curlo];
				}
				output[offsetlo] = plo;
			}
		}
	}
}

// radixsortnoalloc() function implementation template for single-byte types with getter function indirection
template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V, class... arguments>
RSBD8_FUNC_NORMAL std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_unsigned_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>> &&
	8 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>) &&
	!std::is_same_v<bool, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], arguments &&... getterfuncinput)noexcept{
	using T = std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>;
	// do not pass a nullptr here, even though it's safe if count is 0
	assert(input);
	assert(buffer);
	// All the code in this function is adapted for count to be one below its input value here.
	--count;
	if(0 < static_cast<ptrdiff_t>(count)){// a 0 or 1 count array is legal here
		size_t offsets[CHAR_BIT * sizeof(T) * 256 / 8 * 2];// a sizeable amount of indices, but it's worth it, and this function never calls functions either to further increase stack usage anyway
		std::memset(offsets, 0, sizeof(offsets) / 2);// only the low half

		// count the 256 configurations, all in one go
		if /*constexpr*/(reverseorder){// also reverse the array at the same time
			V **pinputlo{input}, **pinputhi{input + count};
			V **pbufferlo{buffer}, **pbufferhi{buffer + count};
			if(4 & count + 1){// possibly initialize with 4 entries before the loop below
				V *pa{pinputlo[0]};
				V *pb{pinputhi[0]};
				V *pc{pinputlo[1]};
				V *pd{pinputhi[-1]};
				pinputhi[0] = pa;
				pbufferhi[0] = pa;
				T cura{(pa->*pgetterfunc)(getterfuncinput...)};
				pinputlo[0] = pb;
				pbufferlo[0] = pb;
				T curb{(pb->*pgetterfunc)(getterfuncinput...)};
				pinputhi[-1] = pc;
				pinputhi -= 2;
				pbufferhi[-1] = pc;
				pbufferhi -= 2;
				T curc{(pc->*pgetterfunc)(getterfuncinput...)};
				pinputlo[1] = pd;
				pinputlo += 2;
				pbufferlo[1] = pd;
				pbufferlo += 2;
				T curd{(pd->*pgetterfunc)(getterfuncinput...)};
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
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
				T cura{(pa->*pgetterfunc)(getterfuncinput...)};
				*pinputlo++ = pb;
				*pbufferlo++ = pb;
				T curb{(pb->*pgetterfunc)(getterfuncinput...)};
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb);
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
				if /*constexpr*/(absolute != isfloatingpoint){// two-register filters only
					pinputhi[0] = pa;
					pbufferhi[0] = pa;
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					pinputlo[0] = pb;
					pbufferlo[0] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					pinputhi[-1] = pc;
					pbufferhi[-1] = pc;
					T curc{(pc->*pgetterfunc)(getterfuncinput...)};
					pinputlo[1] = pd;
					pbufferlo[1] = pd;
					T curd{(pd->*pgetterfunc)(getterfuncinput...)};
					// register pressure performance issue on several platforms: first do the high half here
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
				}
				V *pe{pinputlo[2]};
				V *pf{pinputhi[-2]};
				V *pg{pinputlo[3]};
				V *ph{pinputhi[-3]};
				if /*constexpr*/(absolute != isfloatingpoint){// two-register filters only
					pinputhi[-2] = pe;
					pbufferhi[-2] = pe;
					T cure{(pe->*pgetterfunc)(getterfuncinput...)};
					pinputlo[2] = pf;
					pbufferlo[2] = pf;
					T curf{(pf->*pgetterfunc)(getterfuncinput...)};
					pinputhi[-3] = pg;
					pinputhi -= 4;
					pbufferhi[-3] = pg;
					pbufferhi -= 4;
					T curg{(pg->*pgetterfunc)(getterfuncinput...)};
					pinputlo[3] = ph;
					pinputlo += 4;
					pbufferlo[3] = ph;
					pbufferlo += 4;
					T curh{(ph->*pgetterfunc)(getterfuncinput...)};
					// register pressure performance issue on several platforms: do the low half here second
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cure, curf, curg, curh);
					++offsets[cure];
					++offsets[curf];
					++offsets[curg];
					++offsets[curh];
				}else{
					pinputhi[0] = pa;
					pbufferhi[0] = pa;
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					pinputlo[0] = pb;
					pbufferlo[0] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					pinputhi[-1] = pc;
					pbufferhi[-1] = pc;
					T curc{(pc->*pgetterfunc)(getterfuncinput...)};
					pinputlo[1] = pd;
					pbufferlo[1] = pd;
					T curd{(pd->*pgetterfunc)(getterfuncinput...)};
					pinputhi[-2] = pe;
					pbufferhi[-2] = pe;
					T cure{(pe->*pgetterfunc)(getterfuncinput...)};
					pinputlo[2] = pf;
					pbufferlo[2] = pf;
					T curf{(pf->*pgetterfunc)(getterfuncinput...)};
					pinputhi[-3] = pg;
					pinputhi -= 4;
					pbufferhi[-3] = pg;
					pbufferhi -= 4;
					T curg{(pg->*pgetterfunc)(getterfuncinput...)};
					pinputlo[3] = ph;
					pinputlo += 4;
					pbufferlo[3] = ph;
					pbufferlo += 4;
					T curh{(ph->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute && isfloatingpoint){// one-register filters only
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd, cure, curf, curg, curh);
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
				T cur{(p->*pgetterfunc)(getterfuncinput...)};
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
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
				if /*constexpr*/(absolute != isfloatingpoint){// two-register filters only
					buffer[i + 7] = pa;
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 6] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 5] = pc;
					T curc{(pc->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 4] = pd;
					T curd{(pd->*pgetterfunc)(getterfuncinput...)};
					// register pressure performance issue on several platforms: first do the high half here
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
					++offsets[cura];
					++offsets[curb];
					++offsets[curc];
					++offsets[curd];
				}
				V *pe{input[i + 3]};
				V *pf{input[i + 2]};
				V *pg{input[i + 1]};
				V *ph{input[i]};
				if /*constexpr*/(absolute != isfloatingpoint){// two-register filters only
					buffer[i + 3] = pe;
					T cure{(pe->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 2] = pf;
					T curf{(pf->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 1] = pg;
					T curg{(pg->*pgetterfunc)(getterfuncinput...)};
					buffer[i] = ph;
					T curh{(ph->*pgetterfunc)(getterfuncinput...)};
					// register pressure performance issue on several platforms: do the low half here second
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cure, curf, curg, curh);
					++offsets[cure];
					++offsets[curf];
					++offsets[curg];
					++offsets[curh];
				}else{
					buffer[i + 7] = pa;
					T cura{(pa->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 6] = pb;
					T curb{(pb->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 5] = pc;
					T curc{(pc->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 4] = pd;
					T curd{(pd->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 3] = pe;
					T cure{(pe->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 2] = pf;
					T curf{(pf->*pgetterfunc)(getterfuncinput...)};
					buffer[i + 1] = pg;
					T curg{(pg->*pgetterfunc)(getterfuncinput...)};
					buffer[i] = ph;
					T curh{(ph->*pgetterfunc)(getterfuncinput...)};
					if /*constexpr*/(absolute && isfloatingpoint){// one-register filters only
						helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd, cure, curf, curg, curh);
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
				T cura{(pa->*pgetterfunc)(getterfuncinput...)};
				buffer[i + 6] = pb;
				T curb{(pb->*pgetterfunc)(getterfuncinput...)};
				buffer[i + 5] = pc;
				T curc{(pc->*pgetterfunc)(getterfuncinput...)};
				buffer[i + 4] = pd;
				i -= 4;// required for the "if(2 & i){" part
				T curd{(pd->*pgetterfunc)(getterfuncinput...)};
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb, curc, curd);
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
				T cura{(pa->*pgetterfunc)(getterfuncinput...)};
				buffer[i + 6] = pb;
				T curb{(pb->*pgetterfunc)(getterfuncinput...)};
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cura, curb);
				}
				++offsets[cura];
				++offsets[curb];
			}
			if(1 & i){// possibly finalize 1 entry after the loop above
				V *p{input[0]};
				buffer[0] = p;
				T cur{(p->*pgetterfunc)(getterfuncinput...)};
				if /*constexpr*/(absolute || isfloatingpoint){
					helper::filterinput<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(cur);
				}
				++offsets[cur];
			}
		}

		// transform counts into base offsets for each set of 256 items, both for the low and high half of offsets here
		if(helper::singlebyteoffsets<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, offsets))
#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
			[[likely]]
#endif
		{

			// perform the bidirectional 8-bit sorting sequence
			V **psrclo{buffer}, **psrchi{buffer + count};
			do{// fill the array, two at a time: one low-to-middle, one high-to-middle
				V *plo{*psrclo++};
				V *phi{*psrchi--};
				T outlo{(plo->*pgetterfunc)(getterfuncinput...)};
				T outhi{(phi->*pgetterfunc)(getterfuncinput...)};
				auto[curlo, curhi]{helper::filtertopbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo, outhi)};
				size_t offsetlo, offsethi;// this is only allowed for the single-byte version, containing just one sorting pass
				if /*constexpr*/(reverseorder){
					offsetlo = offsets[curlo + CHAR_BIT * sizeof(T) * 256 / 8]--;// the next item will be placed one lower
					offsethi = offsets[curhi]++;// the next item will be placed one higher
				}else{
					offsetlo = offsets[curlo]++;// the next item will be placed one higher
					offsethi = offsets[curhi + CHAR_BIT * sizeof(T) * 256 / 8]--;// the next item will be placed one lower
				}
				input[offsetlo] = plo;
				input[offsethi] = phi;
			}while(psrclo < psrchi);
			if(psrclo == psrchi){// fill in the final item for odd counts
				V *plo{*psrclo};
				T outlo{(plo->*pgetterfunc)(getterfuncinput...)};
				size_t curlo{helper::filtertopbyte<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(outlo)};
				size_t offsetlo;// this is only allowed for the single-byte version, containing just one sorting pass
				if /*constexpr*/(reverseorder){
					offsetlo = offsets[curlo + CHAR_BIT * sizeof(T) * 256 / 8];
				}else{
					offsetlo = offsets[curlo];
				}
				input[offsetlo] = plo;
			}
		}
	}
}

// 3 variants directly wrap the multi-byte radixsortcopynoalloc() function without indirection

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<bool, T> ||
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, void>>::type>) &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, T const input[], T output[], T buffer[])noexcept{
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		std::make_unsigned<T>>>>>>;
	radixsortcopynoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, U>(count, reinterpret_cast<U const *>(input), reinterpret_cast<U *>(output), reinterpret_cast<U *>(buffer));
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, T const input[], T output[], T buffer[])noexcept{
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		std::make_unsigned<T>>>>>>;
	radixsortcopynoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, U>(count, reinterpret_cast<U const *>(input), reinterpret_cast<U *>(output), reinterpret_cast<U *>(buffer));
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, T const input[], T output[], T buffer[])noexcept{
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		typename std::conditional_t<std::is_scalar_v<T>,// make sure this is evaluated last
			std::enable_if<true, std::make_unsigned<T>>,// last fallback
			std::enable_if<true, void>>::type>>>>>;// keep it the same size or break compilation
	radixsortcopynoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, U>(count, reinterpret_cast<U const *>(input), reinterpret_cast<U *>(output), reinterpret_cast<U *>(buffer));
}

// 3 variants directly wrap the multi-byte radixsortnoalloc() function without indirection

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<bool, T> ||
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, void>>::type>) &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, T input[], T buffer[], bool movetobuffer = false)noexcept{
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		std::make_unsigned<T>>>>>>;
	radixsortnoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, U>(count, reinterpret_cast<U *>(input), reinterpret_cast<U *>(buffer), movetobuffer);
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, T input[], T buffer[], bool movetobuffer = false)noexcept{
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		std::make_unsigned<T>>>>>>;
	radixsortnoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, U>(count, reinterpret_cast<U *>(input), reinterpret_cast<U *>(buffer), movetobuffer);
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, T input[], T buffer[], bool movetobuffer = false)noexcept{
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		typename std::conditional_t<std::is_scalar_v<T>,// make sure this is evaluated last
			std::enable_if<true, std::make_unsigned<T>>,// last fallback
			std::enable_if<true, void>>::type>>>>>;// keep it the same size or break compilation
	radixsortnoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, U>(count, reinterpret_cast<U *>(input), reinterpret_cast<U *>(buffer), movetobuffer);
}

// 6 variants directly wrap the single-byte radixsortcopynoalloc() function without indirection

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<bool, T> ||
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, void>>::type>) &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, T const input[], T output[])noexcept{
	radixsortcopynoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, unsigned char>(count, reinterpret_cast<unsigned char const *>(input), reinterpret_cast<unsigned char *>(output));
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, T const input[], T output[])noexcept{
	radixsortcopynoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, unsigned char>(count, reinterpret_cast<unsigned char const *>(input), reinterpret_cast<unsigned char *>(output));
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, T const input[], T output[])noexcept{
	radixsortcopynoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, unsigned char>(count, reinterpret_cast<unsigned char const *>(input), reinterpret_cast<unsigned char *>(output));
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<bool, T> ||
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, void>>::type>) &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, T const input[], T output[], T buffer[])noexcept{
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, unsigned char>(count, reinterpret_cast<unsigned char const *>(input), reinterpret_cast<unsigned char *>(output));
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, T const input[], T output[], T buffer[])noexcept{
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, unsigned char>(count, reinterpret_cast<unsigned char const *>(input), reinterpret_cast<unsigned char *>(output));
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, T const input[], T output[], T buffer[])noexcept{
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, unsigned char>(count, reinterpret_cast<unsigned char const *>(input), reinterpret_cast<unsigned char *>(output));
}

// 3 variants directly wrap the single-byte radixsortnoalloc() function without indirection

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<bool, T> ||
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, void>>::type>) &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, T input[], T buffer[])noexcept{
	radixsortnoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, unsigned char>(count, reinterpret_cast<unsigned char *>(input), reinterpret_cast<unsigned char *>(buffer));
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, T input[], T buffer[])noexcept{
	radixsortnoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, unsigned char>(count, reinterpret_cast<unsigned char *>(input), reinterpret_cast<unsigned char *>(buffer));
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, T input[], T buffer[])noexcept{
	radixsortnoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, unsigned char>(count, reinterpret_cast<unsigned char *>(input), reinterpret_cast<unsigned char *>(buffer));
}

// 3 variants directly wrap the single-byte radixsortnoalloc() and radixsortcopynoalloc() functions without indirection
// These do not set the default "false" for the "movetobuffer" parameter.

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	(std::is_same_v<bool, T> ||
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, void>>::type>) &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, T input[], T buffer[], bool movetobuffer)noexcept{
	if(!movetobuffer) radixsortnoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, input, buffer);
	else radixsortcopynoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, input, buffer);
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, T input[], T buffer[], bool movetobuffer)noexcept{
	if(!movetobuffer) radixsortnoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, input, buffer);
	else radixsortcopynoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, input, buffer);
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, typename T>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, T input[], T buffer[], bool movetobuffer)noexcept{
	if(!movetobuffer) radixsortnoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, input, buffer);
	else radixsortcopynoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, input, buffer);
}

// 3 variants of radixsort() that only allocate some memory prior to sorting arrays without indirection
// These require no separate specialisations for handling the single-byte types.

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T),
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
		radixsortnoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, input, buffer);// last parameter not filled in on purpose
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, typename T>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T),
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
		radixsortnoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, input, buffer);// last parameter not filled in on purpose
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, typename T>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T),
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
		radixsortnoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, input, buffer);// last parameter not filled in on purpose
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

// 3 variants of multi-byte radixsortcopy() that only allocate some memory prior to sorting arrays without indirection

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T) &&
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
		radixsortcopynoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, input, output, buffer);
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, typename T>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T) &&
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
		radixsortcopynoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, input, output, buffer);
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, typename T>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T) &&
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
		radixsortcopynoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, input, output, buffer);
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

// 3 variants of single-byte radixsortcopy() that directly wrap the single-byte radixsortcopynoalloc() function without indirection

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, typename T>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
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
	// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, input, output);
	return{true};
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, typename T>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
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
	// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, input, output);
	return{true};
}

template<bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, typename T>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
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
	// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<reversesort, reverseorder, absolute, issigned, isfloatingpoint, T>(count, input, output);
	return{true};
}

// 3 variants directly wrap the multi-byte radixsortcopynoalloc() function with getter function indirection

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V, class... arguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	(std::is_same_v<bool, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>> ||
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, void>>::type>) &&
	64 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>) &&
	8 < CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[], arguments &&... getterfuncinput)noexcept{
	using T = std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>;
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		std::make_unsigned<T>>>>>>;
	if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)const noexcept>){
		radixsortcopynoalloc<reinterpret_cast<U(V:: *)(arguments...)const noexcept>(&helper::gettercreator<U, pgetterfunc>::indirection1funcconst), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer, getterfuncinput...);
	}else if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)noexcept>){
		radixsortcopynoalloc<reinterpret_cast<U(V:: *)(arguments...)noexcept>(&helper::gettercreator<U, pgetterfunc>::indirection1func), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer, getterfuncinput...);
	}else{
		static_assert(false, "Implementing calling through member functions other than those decorated with const noexcept or noexcept will require additional work for this library.");
	}
}

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V, class... arguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>) &&
	8 < CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[], arguments &&... getterfuncinput)noexcept{
	using T = std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>;
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		std::make_unsigned<T>>>>>>;
	if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)const noexcept>){
		radixsortcopynoalloc<reinterpret_cast<U(V:: *)(arguments...)const noexcept>(&helper::gettercreator<U, pgetterfunc>::indirection1funcconst), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer, getterfuncinput...);
	}else if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)noexcept>){
		radixsortcopynoalloc<reinterpret_cast<U(V:: *)(arguments...)noexcept>(&helper::gettercreator<U, pgetterfunc>::indirection1func), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer, getterfuncinput...);
	}else{
		static_assert(false, "Implementing calling through member functions other than those decorated with const noexcept or noexcept will require additional work for this library.");
	}
}

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V, class... arguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>) &&
	8 < CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[], arguments &&... getterfuncinput)noexcept{
	using T = std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>;
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		typename std::conditional_t<std::is_scalar_v<T>,// make sure this is evaluated last
			std::enable_if<true, std::make_unsigned<T>>,// last fallback
			std::enable_if<true, void>>::type>>>>>;// keep it the same size or break compilation
	if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)const noexcept>){
		radixsortcopynoalloc<reinterpret_cast<U(V:: *)(arguments...)const noexcept>(&helper::gettercreator<U, pgetterfunc>::indirection1funcconst), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer, getterfuncinput...);
	}else if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)noexcept>){
		radixsortcopynoalloc<reinterpret_cast<U(V:: *)(arguments...)noexcept>(&helper::gettercreator<U, pgetterfunc>::indirection1func), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer, getterfuncinput...);
	}else{
		static_assert(false, "Implementing calling through member functions other than those decorated with const noexcept or noexcept will require additional work for this library.");
	}
}

// 3 variants directly wrap the multi-byte radixsortnoalloc() function with getter function indirection

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V, class... arguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	(std::is_same_v<bool, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>> ||
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, void>>::type>) &&
	64 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>) &&
	8 < CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer = false, arguments &&... getterfuncinput)noexcept{
	using T = std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>;
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		std::make_unsigned<T>>>>>>;
	if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)const noexcept>){
		radixsortnoalloc<reinterpret_cast<U(V:: *)(arguments...)const noexcept>(&helper::gettercreator<U, pgetterfunc>::indirection1funcconst), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, movetobuffer, getterfuncinput...);
	}else if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)noexcept>){
		radixsortnoalloc<reinterpret_cast<U(V:: *)(arguments...)noexcept>(&helper::gettercreator<U, pgetterfunc>::indirection1func), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, movetobuffer, getterfuncinput...);
	}else{
		static_assert(false, "Implementing calling through member functions other than those decorated with const noexcept or noexcept will require additional work for this library.");
	}
}

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V, class... arguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>) &&
	8 < CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer = false, arguments &&... getterfuncinput)noexcept{
	using T = std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>;
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		std::make_unsigned<T>>>>>>;
	if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)const noexcept>){
		radixsortnoalloc<reinterpret_cast<U(V:: *)(arguments...)const noexcept>(&helper::gettercreator<U, pgetterfunc>::indirection1funcconst), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, movetobuffer, getterfuncinput...);
	}else if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)noexcept>){
		radixsortnoalloc<reinterpret_cast<U(V:: *)(arguments...)noexcept>(&helper::gettercreator<U, pgetterfunc>::indirection1func), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, movetobuffer, getterfuncinput...);
	}else{
		static_assert(false, "Implementing calling through member functions other than those decorated with const noexcept or noexcept will require additional work for this library.");
	}
}

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V, class... arguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>) &&
	8 < CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer = false, arguments &&... getterfuncinput)noexcept{
	using T = std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>;
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		typename std::conditional_t<std::is_scalar_v<T>,// make sure this is evaluated last
			std::enable_if<true, std::make_unsigned<T>>,// last fallback
			std::enable_if<true, void>>::type>>>>>;// keep it the same size or break compilation
	if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)const noexcept>){
		radixsortnoalloc<reinterpret_cast<U(V:: *)(arguments...)const noexcept>(&helper::gettercreator<U, pgetterfunc>::indirection1funcconst), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, movetobuffer, getterfuncinput...);
	}else if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)noexcept>){
		radixsortnoalloc<reinterpret_cast<U(V:: *)(arguments...)noexcept>(&helper::gettercreator<U, pgetterfunc>::indirection1func), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, movetobuffer, getterfuncinput...);
	}else{
		static_assert(false, "Implementing calling through member functions other than those decorated with const noexcept or noexcept will require additional work for this library.");
	}
}

// 6 variants directly wrap the single-byte radixsortcopynoalloc() function with getter function indirection

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V, class... arguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	(std::is_same_v<bool, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>> ||
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, void>>::type>) &&
	8 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], arguments &&... getterfuncinput)noexcept{
	using T = std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>;
	if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)const noexcept>){
		radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, pgetterfunc>::indirection1funcconst), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, getterfuncinput...);
	}else if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)noexcept>){
		radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()noexcept>(&helper::gettercreator<unsigned char, pgetterfunc>::indirection1func), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, getterfuncinput...);
	}else{
		static_assert(false, "Implementing calling through member functions other than those decorated with const noexcept or noexcept will require additional work for this library.");
	}
}

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V, class... arguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], arguments &&... getterfuncinput)noexcept{
	using T = std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>;
	if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)const noexcept>){
		radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, pgetterfunc>::indirection1funcconst), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, getterfuncinput...);
	}else if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)noexcept>){
		radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()noexcept>(&helper::gettercreator<unsigned char, pgetterfunc>::indirection1func), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, getterfuncinput...);
	}else{
		static_assert(false, "Implementing calling through member functions other than those decorated with const noexcept or noexcept will require additional work for this library.");
	}
}

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V, class... arguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], arguments &&... getterfuncinput)noexcept{
	using T = std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>;
	if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)const noexcept>){
		radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, pgetterfunc>::indirection1funcconst), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, getterfuncinput...);
	}else if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)noexcept>){
		radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()noexcept>(&helper::gettercreator<unsigned char, pgetterfunc>::indirection1func), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, getterfuncinput...);
	}else{
		static_assert(false, "Implementing calling through member functions other than those decorated with const noexcept or noexcept will require additional work for this library.");
	}
}

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V, class... arguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	(std::is_same_v<bool, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>> ||
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, void>>::type>) &&
	8 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[], arguments &&... getterfuncinput)noexcept{
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	using T = std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>;
	if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)const noexcept>){
		radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, pgetterfunc>::indirection1funcconst), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, getterfuncinput...);
	}else if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)noexcept>){
		radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()noexcept>(&helper::gettercreator<unsigned char, pgetterfunc>::indirection1func), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, getterfuncinput...);
	}else{
		static_assert(false, "Implementing calling through member functions other than those decorated with const noexcept or noexcept will require additional work for this library.");
	}
}

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V, class... arguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[], arguments &&... getterfuncinput)noexcept{
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	using T = std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>;
	if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)const noexcept>){
		radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, pgetterfunc>::indirection1funcconst), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, getterfuncinput...);
	}else if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)noexcept>){
		radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()noexcept>(&helper::gettercreator<unsigned char, pgetterfunc>::indirection1func), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, getterfuncinput...);
	}else{
		static_assert(false, "Implementing calling through member functions other than those decorated with const noexcept or noexcept will require additional work for this library.");
	}
}

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V, class... arguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[], arguments &&... getterfuncinput)noexcept{
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	using T = std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>;
	if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)const noexcept>){
		radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, pgetterfunc>::indirection1funcconst), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, getterfuncinput...);
	}else if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)noexcept>){
		radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()noexcept>(&helper::gettercreator<unsigned char, pgetterfunc>::indirection1func), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, getterfuncinput...);
	}else{
		static_assert(false, "Implementing calling through member functions other than those decorated with const noexcept or noexcept will require additional work for this library.");
	}
}

// 3 variants directly wrap the single-byte radixsortnoalloc() function with getter function indirection

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V, class... arguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	(std::is_same_v<bool, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>> ||
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, void>>::type>) &&
	8 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], arguments &&... getterfuncinput)noexcept{
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	using T = std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>;
	if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)const noexcept>){
		radixsortnoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, pgetterfunc>::indirection1funcconst), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, getterfuncinput...);
	}else if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)noexcept>){
		radixsortnoalloc<reinterpret_cast<unsigned char(V:: *)()noexcept>(&helper::gettercreator<unsigned char, pgetterfunc>::indirection1func), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, getterfuncinput...);
	}else{
		static_assert(false, "Implementing calling through member functions other than those decorated with const noexcept or noexcept will require additional work for this library.");
	}
}

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V, class... arguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], arguments &&... getterfuncinput)noexcept{
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	using T = std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>;
	if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)const noexcept>){
		radixsortnoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, pgetterfunc>::indirection1funcconst), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, getterfuncinput...);
	}else if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)noexcept>){
		radixsortnoalloc<reinterpret_cast<unsigned char(V:: *)()noexcept>(&helper::gettercreator<unsigned char, pgetterfunc>::indirection1func), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, getterfuncinput...);
	}else{
		static_assert(false, "Implementing calling through member functions other than those decorated with const noexcept or noexcept will require additional work for this library.");
	}
}

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V, class... arguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], arguments &&... getterfuncinput)noexcept{
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	using T = std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>;
	if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)const noexcept>){
		radixsortnoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, pgetterfunc>::indirection1funcconst), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, getterfuncinput...);
	}else if constexpr(std::is_same_v<decltype(pgetterfunc), T(V:: *)(arguments...)noexcept>){
		radixsortnoalloc<reinterpret_cast<unsigned char(V:: *)()noexcept>(&helper::gettercreator<unsigned char, pgetterfunc>::indirection1func), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, getterfuncinput...);
	}else{
		static_assert(false, "Implementing calling through member functions other than those decorated with const noexcept or noexcept will require additional work for this library.");
	}
}

// 3 variants directly wrap the single-byte radixsortnoalloc() and radixsortcopynoalloc() functions with getter function indirection
// These do not set the default "false" for the "movetobuffer" parameter.

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V, class... arguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	(std::is_same_v<bool, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>> ||
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, void>>::type>) &&
	8 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer, arguments &&... getterfuncinput)noexcept{
	if(!movetobuffer) radixsortnoalloc<pgetterfunc, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, getterfuncinput...);
	else radixsortcopynoalloc<pgetterfunc, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, getterfuncinput...);
}

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V, class... arguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer, arguments &&... getterfuncinput)noexcept{
	if(!movetobuffer) radixsortnoalloc<pgetterfunc, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, getterfuncinput...);
	else radixsortcopynoalloc<pgetterfunc, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, getterfuncinput...);
}

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V, class... arguments>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer, arguments &&... getterfuncinput)noexcept{
	if(!movetobuffer) radixsortnoalloc<pgetterfunc, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, getterfuncinput...);
	else radixsortcopynoalloc<pgetterfunc, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, getterfuncinput...);
}

// 3 variants of radixsort() that only allocate some memory prior to sorting arrays with getter function indirection
// These require no separate specialisations for handling the single-byte types.

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V, class... arguments>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	bool> radixsort(size_t count, V *input[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		, arguments &&... getterfuncinput)noexcept{
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
		radixsortnoalloc<pgetterfunc, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, getterfuncinput...);// last parameter not filled in on purpose
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V, class... arguments>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	bool> radixsort(size_t count, V *input[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		, arguments &&... getterfuncinput)noexcept{
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
		radixsortnoalloc<pgetterfunc, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, getterfuncinput...);// last parameter not filled in on purpose
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V, class... arguments>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	bool> radixsort(size_t count, V *input[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		, arguments &&... getterfuncinput)noexcept{
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
		radixsortnoalloc<pgetterfunc, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, getterfuncinput...);// last parameter not filled in on purpose
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

// 3 variants of multi-byte radixsortcopy() that only allocate some memory prior to sorting arrays with getter function indirection

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V, class... arguments>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>) &&
	8 < CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	bool> radixsortcopy(size_t count, V *const input[], V output[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		, arguments &&... getterfuncinput)noexcept{
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
		radixsortcopynoalloc<pgetterfunc, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer, getterfuncinput...);
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V, class... arguments>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>) &&
	8 < CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	bool> radixsortcopy(size_t count, V *const input[], V *output[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		, arguments &&... getterfuncinput)noexcept{
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
		radixsortcopynoalloc<pgetterfunc, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer, getterfuncinput...);
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V, class... arguments>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>) &&
	8 < CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	bool> radixsortcopy(size_t count, V *const input[], V *output[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		, arguments &&... getterfuncinput)noexcept{
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
		radixsortcopynoalloc<pgetterfunc, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer, getterfuncinput...);
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

// 3 variants of single-byte radixsortcopy() that directly wrap the single-byte radixsortcopynoalloc() function with getter function indirection

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V, class... arguments>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	bool> radixsortcopy(size_t count, V *const input[], V *output[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		, arguments &&... getterfuncinput)noexcept{
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
	assert(!(largepagesize - 1 & largepagesize));// a maximum of one bit should be set in the value of largepagesize
	static_cast<void>(largepagesize);
#elif defined(_POSIX_C_SOURCE)
	static_cast<void>(mmapflags);
#endif
	// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<pgetterfunc, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, getterfuncinput...);
	return{true};
}

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V, class... arguments>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	bool> radixsortcopy(size_t count, V *const input[], V *output[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		, arguments &&... getterfuncinput)noexcept{
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
	assert(!(largepagesize - 1 & largepagesize));// a maximum of one bit should be set in the value of largepagesize
	static_cast<void>(largepagesize);
#elif defined(_POSIX_C_SOURCE)
	static_cast<void>(mmapflags);
#endif
	// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<pgetterfunc, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, getterfuncinput...);
	return{true};
}

template<auto pgetterfunc, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V, class... arguments>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_function_pointer_v<decltype(pgetterfunc)> &&
	std::is_nothrow_invocable_v<decltype(pgetterfunc), V *, arguments...> &&
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::underlying_type<std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>,
		std::enable_if<true, std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::invoke_result_t<decltype(pgetterfunc), V *, arguments...>),
	bool> radixsortcopy(size_t count, V *const input[], V *output[]
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, size_t largepagesize = 0
#elif defined(_POSIX_C_SOURCE)
		, int mmapflags = MAP_ANONYMOUS | MAP_PRIVATE
#endif
		, arguments &&... getterfuncinput)noexcept{
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
	assert(!(largepagesize - 1 & largepagesize));// a maximum of one bit should be set in the value of largepagesize
	static_cast<void>(largepagesize);
#elif defined(_POSIX_C_SOURCE)
	static_cast<void>(mmapflags);
#endif
	// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<pgetterfunc, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, getterfuncinput...);
	return{true};
}

// 3 variants directly wrap the multi-byte radixsortcopynoalloc() function with offset and type pointer indirection

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[])noexcept{
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		std::make_unsigned<T>>>>>>;
	radixsortcopynoalloc<reinterpret_cast<U(V:: *)()const noexcept>(&helper::gettercreator<U, offset>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer);
}

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[])noexcept{
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		std::make_unsigned<T>>>>>>;
	radixsortcopynoalloc<reinterpret_cast<U(V:: *)()const noexcept>(&helper::gettercreator<U, offset>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer);
}

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[])noexcept{
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		typename std::conditional_t<std::is_scalar_v<T>,// make sure this is evaluated last
			std::enable_if<true, std::make_unsigned<T>>,// last fallback
			std::enable_if<true, void>>::type>>>>>;// keep it the same size or break compilation
	radixsortcopynoalloc<reinterpret_cast<U(V:: *)()const noexcept>(&helper::gettercreator<U, offset>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer);
}

// 3 variants directly wrap the multi-byte radixsortnoalloc() function with offset and type pointer indirection

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer = false)noexcept{
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		std::make_unsigned<T>>>>>>;
	radixsortnoalloc<reinterpret_cast<U(V:: *)()const noexcept>(&helper::gettercreator<U, offset>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, movetobuffer);
}

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer = false)noexcept{
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		std::make_unsigned<T>>>>>>;
	radixsortnoalloc<reinterpret_cast<U(V:: *)()const noexcept>(&helper::gettercreator<U, offset>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, movetobuffer);
}

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer = false)noexcept{
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		typename std::conditional_t<std::is_scalar_v<T>,// make sure this is evaluated last
			std::enable_if<true, std::make_unsigned<T>>,// last fallback
			std::enable_if<true, void>>::type>>>>>;// keep it the same size or break compilation
	radixsortnoalloc<reinterpret_cast<U(V:: *)()const noexcept>(&helper::gettercreator<U, offset>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, movetobuffer);
}

// 6 variants directly wrap the single-byte radixsortcopynoalloc() function with offset and type pointer indirection

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[])noexcept{
	// Make sure to cast "bool *" to "unsigned char *".
	radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, offset>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output);
}

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[])noexcept{
	radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, offset>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output);
}

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[])noexcept{
	radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, offset>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output);
}

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[])noexcept{
	// Make sure to cast "bool *" to "unsigned char *".
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, offset>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output);
}

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[])noexcept{
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, offset>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, reinterpret_cast<unsigned char const *>(input), reinterpret_cast<unsigned char *>(output));
}

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[])noexcept{
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, offset>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output);
}

// 3 variants directly wrap the single-byte radixsortnoalloc() function with offset and type pointer indirection

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[])noexcept{
	// Make sure to cast "bool *" to "unsigned char *".
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	radixsortnoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, offset>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);
}

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[])noexcept{
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	radixsortnoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, offset>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);
}

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[])noexcept{
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	radixsortnoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, offset>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);
}

// 3 variants directly wrap the single-byte radixsortnoalloc() and radixsortcopynoalloc() functions with offset and type pointer indirection
// These do not set the default "false" for the "movetobuffer" parameter.

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer)noexcept{
	if(!movetobuffer) radixsortnoalloc<offset, T, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);
	else radixsortcopynoalloc<offset, T, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);
}

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer)noexcept{
	if(!movetobuffer) radixsortnoalloc<offset, T, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);
	else radixsortcopynoalloc<offset, T, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);
}

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer)noexcept{
	if(!movetobuffer) radixsortnoalloc<offset, T, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);
	else radixsortcopynoalloc<offset, T, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);
}

// 3 variants of radixsort() that only allocate some memory prior to sorting arrays with offset and type pointer indirection
// These require no separate specialisations for handling the single-byte types.

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T),
	bool> radixsort(size_t count, V *input[]
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
		{allocatearray<V *>(count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, largepagesize
#elif defined(_POSIX_C_SOURCE)
		, mmapflags
#endif
		)};
	if(buffer){
		radixsortnoalloc<offset, T, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);// last parameter not filled in on purpose
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T),
	bool> radixsort(size_t count, V *input[]
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
		{allocatearray<V *>(count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, largepagesize
#elif defined(_POSIX_C_SOURCE)
		, mmapflags
#endif
		)};
	if(buffer){
		radixsortnoalloc<offset, T, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);// last parameter not filled in on purpose
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T),
	bool> radixsort(size_t count, V *input[]
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
		{allocatearray<V *>(count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, largepagesize
#elif defined(_POSIX_C_SOURCE)
		, mmapflags
#endif
		)};
	if(buffer){
		radixsortnoalloc<offset, T, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);// last parameter not filled in on purpose
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

// 3 variants of multi-byte radixsortcopy() that only allocate some memory prior to sorting arrays with offset and type pointer indirection

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	bool> radixsortcopy(size_t count, V *const input[], V output[]
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
		{allocatearray<V *>(count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, largepagesize
#elif defined(_POSIX_C_SOURCE)
		, mmapflags
#endif
		)};
	if(buffer){
		radixsortcopynoalloc<offset, T, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer);
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	bool> radixsortcopy(size_t count, V *const input[], V *output[]
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
		{allocatearray<V *>(count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, largepagesize
#elif defined(_POSIX_C_SOURCE)
		, mmapflags
#endif
		)};
	if(buffer){
		radixsortcopynoalloc<offset, T, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer);
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	64 >= CHAR_BIT * sizeof(T) &&
	8 < CHAR_BIT * sizeof(T),
	bool> radixsortcopy(size_t count, V *const input[], V *output[]
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
		{allocatearray<V *>(count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, largepagesize
#elif defined(_POSIX_C_SOURCE)
		, mmapflags
#endif
		)};
	if(buffer){
		radixsortcopynoalloc<offset, T, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer);
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

// 3 variants of single-byte radixsortcopy() that directly wrap the single-byte radixsortcopynoalloc() function with offset and type pointer indirection

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	bool> radixsortcopy(size_t count, V *const input[], V *output[]
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
	// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<offset, T, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output);
	return{true};
}

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	bool> radixsortcopy(size_t count, V *const input[], V *output[]
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
	// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<offset, T, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output);
	return{true};
}

template<ptrdiff_t offset, typename T, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<T>,
		std::underlying_type<T>,
		std::enable_if<true, T>>::type> &&
	8 >= CHAR_BIT * sizeof(T),
	bool> radixsortcopy(size_t count, V *const input[], V *output[]
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
	// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<offset, T, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output);
	return{true};
}

// 3 variants directly wrap the multi-byte radixsortcopynoalloc() function with class member pointer indirection

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>) &&
	8 < CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[])noexcept{
	using T = std::remove_reference_t<decltype(std::declval<V>().*pmember)>;
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		std::make_unsigned<T>>>>>>;
	radixsortcopynoalloc<reinterpret_cast<U(V:: *)()const noexcept>(&helper::gettercreator<U, helper::GetOffsetOf<pmember, V>>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer);
}

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>) &&
	8 < CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[])noexcept{
	using T = std::remove_reference_t<decltype(std::declval<V>().*pmember)>;
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		std::make_unsigned<T>>>>>>;
	radixsortcopynoalloc<reinterpret_cast<U(V:: *)()const noexcept>(&helper::gettercreator<U, helper::GetOffsetOf<pmember, V>>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer);
}

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>) &&
	8 < CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[])noexcept{
	using T = std::remove_reference_t<decltype(std::declval<V>().*pmember)>;
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		typename std::conditional_t<std::is_scalar_v<T>,// make sure this is evaluated last
			std::enable_if<true, std::make_unsigned<T>>,// last fallback
			std::enable_if<true, void>>::type>>>>>;// keep it the same size or break compilation
	radixsortcopynoalloc<reinterpret_cast<U(V:: *)()const noexcept>(&helper::gettercreator<U, helper::GetOffsetOf<pmember, V>>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer);
}

// 3 variants directly wrap the multi-byte radixsortnoalloc() function with class member pointer indirection

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>) &&
	8 < CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer = false)noexcept{
	using T = std::remove_reference_t<decltype(std::declval<V>().*pmember)>;
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		std::make_unsigned<T>>>>>>;
	radixsortnoalloc<reinterpret_cast<U(V:: *)()const noexcept>(&helper::gettercreator<U, helper::GetOffsetOf<pmember, V>>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, movetobuffer);
}

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>) &&
	8 < CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer = false)noexcept{
	using T = std::remove_reference_t<decltype(std::declval<V>().*pmember)>;
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		std::make_unsigned<T>>>>>>;
	radixsortnoalloc<reinterpret_cast<U(V:: *)()const noexcept>(&helper::gettercreator<U, helper::GetOffsetOf<pmember, V>>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, movetobuffer);
}

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>) &&
	8 < CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer = false)noexcept{
	using T = std::remove_reference_t<decltype(std::declval<V>().*pmember)>;
	using U = std::conditional_t<1          == sizeof(T), unsigned char,
		std::conditional_t<sizeof(short)     == sizeof(T), unsigned short,
		std::conditional_t<sizeof(int)       == sizeof(T), unsigned,
		std::conditional_t<sizeof(long)      == sizeof(T), unsigned long,
		std::conditional_t<sizeof(long long) == sizeof(T), unsigned long long,
		typename std::conditional_t<std::is_scalar_v<T>,// make sure this is evaluated last
			std::enable_if<true, std::make_unsigned<T>>,// last fallback
			std::enable_if<true, void>>::type>>>>>;// keep it the same size or break compilation
	radixsortnoalloc<reinterpret_cast<U(V:: *)()const noexcept>(&helper::gettercreator<U, helper::GetOffsetOf<pmember, V>>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer, movetobuffer);
}

// 6 variants directly wrap the single-byte radixsortcopynoalloc() function with class member pointer indirection

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[])noexcept{
	// Make sure to cast "bool *" to "unsigned char *".
	radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, helper::GetOffsetOf<pmember, V>>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output);
}

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[])noexcept{
	radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, helper::GetOffsetOf<pmember, V>>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output);
}

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[])noexcept{
	radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, helper::GetOffsetOf<pmember, V>>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output);
}

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[])noexcept{
	// Make sure to cast "bool *" to "unsigned char *".
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, helper::GetOffsetOf<pmember, V>>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output);
}

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[])noexcept{
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, helper::GetOffsetOf<pmember, V>>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, reinterpret_cast<unsigned char const *>(input), reinterpret_cast<unsigned char *>(output));
}

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	void> radixsortcopynoalloc(size_t count, V *const input[], V *output[], V *buffer[])noexcept{
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, helper::GetOffsetOf<pmember, V>>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output);
}

// 3 variants directly wrap the single-byte radixsortnoalloc() function with class member pointer indirection

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[])noexcept{
	// Make sure to cast "bool *" to "unsigned char *".
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	radixsortnoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, helper::GetOffsetOf<pmember, V>>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);
}

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[])noexcept{
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	radixsortnoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, helper::GetOffsetOf<pmember, V>>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);
}

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[])noexcept{
	static_cast<void>(buffer);// the single-byte version never needs an extra buffer
	radixsortnoalloc<reinterpret_cast<unsigned char(V:: *)()const noexcept>(&helper::gettercreator<unsigned char, helper::GetOffsetOf<pmember, V>>::indirection1), reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);
}

// 3 variants directly wrap the single-byte radixsortnoalloc() and radixsortcopynoalloc() functions with class member pointer indirection
// These do not set the default "false" for the "movetobuffer" parameter.

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer)noexcept{
	if(!movetobuffer) radixsortnoalloc<pmember, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);
	else radixsortcopynoalloc<pmember, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);
}

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer)noexcept{
	if(!movetobuffer) radixsortnoalloc<pmember, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);
	else radixsortcopynoalloc<pmember, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);
}

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V>
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	void> radixsortnoalloc(size_t count, V *input[], V *buffer[], bool movetobuffer)noexcept{
	if(!movetobuffer) radixsortnoalloc<pmember, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);
	else radixsortcopynoalloc<pmember, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);
}

// 3 variants of radixsort() that only allocate some memory prior to sorting arrays with class member pointer indirection
// These require no separate specialisations for handling the single-byte types.

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>) &&
	8 < CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	bool> radixsort(size_t count, V *input[]
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
		{allocatearray<V *>(count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, largepagesize
#elif defined(_POSIX_C_SOURCE)
		, mmapflags
#endif
		)};
	if(buffer){
		radixsortnoalloc<pmember, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);// last parameter not filled in on purpose
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>) &&
	8 < CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	bool> radixsort(size_t count, V *input[]
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
		{allocatearray<V *>(count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, largepagesize
#elif defined(_POSIX_C_SOURCE)
		, mmapflags
#endif
		)};
	if(buffer){
		radixsortnoalloc<pmember, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);// last parameter not filled in on purpose
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>) &&
	8 < CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	bool> radixsort(size_t count, V *input[]
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
		{allocatearray<V *>(count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, largepagesize
#elif defined(_POSIX_C_SOURCE)
		, mmapflags
#endif
		)};
	if(buffer){
		radixsortnoalloc<pmember, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, buffer);// last parameter not filled in on purpose
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

// 3 variants of multi-byte radixsortcopy() that only allocate some memory prior to sorting arrays with class member pointer indirection

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>) &&
	8 < CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	bool> radixsortcopy(size_t count, V *const input[], V output[]
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
		{allocatearray<V *>(count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, largepagesize
#elif defined(_POSIX_C_SOURCE)
		, mmapflags
#endif
		)};
	if(buffer){
		radixsortcopynoalloc<pmember, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer);
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>) &&
	8 < CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	bool> radixsortcopy(size_t count, V *const input[], V *output[]
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
		{allocatearray<V *>(count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, largepagesize
#elif defined(_POSIX_C_SOURCE)
		, mmapflags
#endif
		)};
	if(buffer){
		radixsortcopynoalloc<pmember, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer);
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	64 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>) &&
	8 < CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	bool> radixsortcopy(size_t count, V *const input[], V *output[]
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
		{allocatearray<V *>(count
#ifdef _WIN32// _WIN32 will remain defined for Windows versions past the legacy 32-bit original.
		, largepagesize
#elif defined(_POSIX_C_SOURCE)
		, mmapflags
#endif
		)};
	if(buffer){
		radixsortcopynoalloc<pmember, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output, buffer);
		deallocatearray(buffer
#if defined(_POSIX_C_SOURCE)
			, allocsize
#endif
			);
		return{true};
	}
	return{false};
}

// 3 variants of single-byte radixsortcopy() that directly wrap the single-byte radixsortcopynoalloc() function with class member pointer indirection

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = false, bool isfloatingpoint = false, class V>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_unsigned_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	bool> radixsortcopy(size_t count, V *const input[], V *output[]
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
	// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<pmember, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output);
	return{true};
}

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = false, class V>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_signed_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	!std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	bool> radixsortcopy(size_t count, V *const input[], V *output[]
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
	// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<pmember, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output);
	return{true};
}

template<auto pmember, bool reversesort = false, bool reverseorder = false, bool absolute = false, bool issigned = true, bool isfloatingpoint = true, class V>
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
[[nodiscard]]
#endif
RSBD8_FUNC_INLINE std::enable_if_t<
	std::is_member_object_pointer_v<decltype(pmember)> &&
	std::is_floating_point_v<typename std::conditional_t<std::is_enum_v<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::underlying_type<std::remove_reference_t<decltype(std::declval<V>().*pmember)>>,
		std::enable_if<true, std::remove_reference_t<decltype(std::declval<V>().*pmember)>>>::type> &&
	8 >= CHAR_BIT * sizeof(std::remove_reference_t<decltype(std::declval<V>().*pmember)>),
	bool> radixsortcopy(size_t count, V *const input[], V *output[]
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
	// the single-byte version never needs an extra buffer
	radixsortcopynoalloc<pmember, reversesort, reverseorder, absolute, issigned, isfloatingpoint, V>(count, input, output);
	return{true};
}

// Library finalisation
// It's a good practice to not propagate private macro definitions when compiling the next files.
// There are no more than these two items from #define statements in this library.
#undef RSBD8_FUNC_INLINE
#undef RSBD8_FUNC_NORMAL
}// namespace rsbd8