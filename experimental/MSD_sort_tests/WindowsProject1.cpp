// MIT License
// Copyright (c) 2025-2026 Jan-Willem Krans (janwillem32 <at> hotmail <dot> com)
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// WindowsProject1.cpp : Defines the entry point for the application.
// the test batch size for the performance tests (8 GiB default)
// suggestions, all prime numbers to make sure all sorts of count leftover paths are touched inside the functions:
// 251uz, the largest prime that will fit into an 8-bit unsigned integer (allocates 2 MiB with regular large pages enabled)
// 65521uz, the largest prime that will fit into a 16-bit unsigned integer (allocates 2 MiB with regular large pages enabled)
// 1073741789uz, the largest prime that will just allocate 1 GiB
// 4294967291uz, the largest prime that will fit into a 32-bit unsigned integer (allocates 4 GiB)
// 4294967311uz, the first prime that breaks the 32-bit unsigned integer limit, (allocates 4 GiB + 2 MiB with regular large pages enabled)
#define RSBD8_TEST_BATCH_SIZE 8uz * 1024 * 1024 * 1024
// the entire benchmarks for the external std::sort() and std::stable_sort() functions can be disabled
#ifdef _DEBUG// skip in debug builds by default to save a lot of time on these slow functions, and don't waste resources on unnecessary tests
#define RSBD8_DISABLE_BENCHMARK_EXTERNAL
#endif
// the maximum and minimum number of threads to use for the performance tests can optionally be set here
//#define RSBD8_THREAD_MAXIMUM 99
//#define RSBD8_THREAD_MINIMUM 1

// disable warning messages for this file only:
// C4559: 'x': redefinition; the function gains __declspec(noalias)
// C4711: function 'x' selected for automatic inline expansion
#pragma warning(disable: 4559 4711)

// utility function for insertion sort, used for small partitions in the MSD radix sort
template<typename T>
__declspec(noalias safebuffers) __forceinline void insertion_sort(std::size_t n, T array[]){
	if(n > 1u){
		std::size_t i{1u};
		do{
			T val{array[i]};
			std::size_t j{i};
			++i;
			assert(j);// the first i items are already sorted, so j will never underflow here
			T cur{array[j - 1u]};
			while(cur > val){
				array[j] = cur;
				--j;
				if(!j) break;
				cur = array[j - 1u];
			}
			array[j] = val;
		}while(n > i);
	}
}

// new implementation of the MSD radix sort

template<unsigned radixbits, typename T>// the std::shared_ptr is just for releasing the resource when the function is called by std::async, and is not used in the function itself
__declspec(noalias safebuffers) void experimentalMSD_sort_bottomlevel(std::size_t n, T array[], std::size_t count[], unsigned reportedcores, std::atomic_uint &coresused, std::shared_ptr<size_t[]>){
	// the top bit of reportedcores indicates if this call is made by std::async, and a release on coresused at the end is required
	static unsigned constexpr mask{(1u << radixbits) - 1u};
	// Create holes
	std::size_t loc[mask + 1u];
	T unsorted[mask + 1u];
	std::size_t live{};
	std::size_t index{};
	std::size_t i{};
	do{
		loc[i] = index;
		unsorted[live] = array[index];
		std::size_t oldindex{index};
		index += count[i];
		count[i] = index;
		++i;
		live += oldindex < index;
	}while(mask >= i);
	--live;
	// Perform sort
	std::size_t j{n};
	do{
		T val{unsorted[live]};
		T d{val & mask};
		std::size_t cur{loc[d]};
		array[cur] = val;
		++cur;
		loc[d] = cur;
		unsorted[live] = array[cur];
		live -= cur == count[d];
		assert((j == 1u)? SIZE_MAX == live : mask >= live);
	}while(--j);
	coresused -= reportedcores >> (CHAR_BIT * sizeof(reportedcores) - 1u);// make space for another thread if high bit of reportedcores is set
}

template<std::size_t bit, unsigned radixbits, typename T>// the std::shared_ptr is just for releasing the resource when the function is called by std::async, and is not used in the function itself
__declspec(noalias safebuffers) decltype(auto) experimentalMSD_sort_midlevel(std::size_t n, T array[], std::size_t count[], unsigned reportedcores, std::atomic_uint &coresused, std::shared_ptr<size_t[]>){
	// the top bit of reportedcores indicates if this call is made by std::async, and a release on coresused at the end is required
	static unsigned constexpr mask{(1u << radixbits) - 1u};
	// Create holes
	std::size_t loc[mask + 1u];
	T unsorted[mask + 1u];
	std::size_t live{};
	std::size_t index{};
	std::size_t i{};
	do{
		loc[i] = index;
		unsorted[live] = array[index];
		std::size_t oldindex{index};
		index += count[i];
		count[i] = index;
		++i;
		live += oldindex < index;
	}while(mask >= i);
	--live;
	// Perform sort and make new counts for the next level
	// raw pointer here, as the soring part can't throw anyway, later on a std::shared_ptr is used to pass on the resource to the children
	std::size_t *nextcounts{new std::size_t[(mask + 1u) * (mask + 1u)]{}};
	std::size_t j{n};
	do{
		T val{unsorted[live]};
		T d{val >> bit & mask};
		T dchild{val >> (bit - radixbits) & ((1u << 2u * radixbits) - 1u)};// double the width
		std::size_t cur{loc[d]};
		++nextcounts[dchild];
		array[cur] = val;
		++cur;
		loc[d] = cur;
		unsorted[live] = array[cur];
		live -= cur == count[d];
		assert((j == 1u)? SIZE_MAX == live : mask >= live);
	}while(--j);
	// Pass on to the next level, using the new counts and async calls for the children
	std::size_t *pcounts{nextcounts};
	n = 0u;
	std::size_t k{};
	if constexpr(radixbits < bit){
		// careful with this line, it's basically a matroshka of nested std::vector<std::future<internaltype>> (std::vector<std::future<void>> at the bottom) and took well over an hour to figure out how to write correctly, and the error messages for getting it wrong are very confusing, so don't change it without fully understanding it
		using internaltype = typename std::conditional_t<radixbits < bit, std::invoke_result<decltype(experimentalMSD_sort_midlevel<bit - radixbits, radixbits, T>), std::size_t, T[], std::size_t[], unsigned, std::atomic_uint &, std::shared_ptr<size_t[]>>, std::enable_if<true, void>>::type;
		std::vector<std::future<internaltype>> vecasync;
		{// limit the scope of the shared pointer to less than that of vecasync
			std::shared_ptr<std::size_t[]> spchildrencounts(nextcounts);
			do{// mid-level
				std::size_t currentindex{count[k]};
				++k;
				std::size_t currentcount{currentindex - n};
				if(20u < currentcount){// If replaced by 1u < n, insertion_sort is not needed.
					if((reportedcores & INT_MAX) > coresused){// mask out the possible high bit of reportedcores
						++coresused;// it doesn't matter if this is incremented by more than one thread at a time, as the check for whether to push into the vector is only an optimization to avoid pushing too many tasks into the vector, and pushing a few extra tasks in the vector won't cause any problems
						vecasync.push_back(std::move(std::async(std::launch::async, experimentalMSD_sort_midlevel<bit - radixbits, radixbits, T>, currentcount, array, pcounts, reportedcores | INT_MIN, std::ref(coresused), spchildrencounts)));
					}else experimentalMSD_sort_midlevel<bit - radixbits, radixbits, T>(currentcount, array, pcounts, reportedcores & INT_MAX, coresused, std::shared_ptr<std::size_t[]>{});// do the work here, mask out the possible high bit of reportedcores
				}else insertion_sort<T>(currentcount, array);
				n = currentindex;
				array += currentcount;
				pcounts += mask + 1u;// move to the next set of counts for the next child
			}while(mask >= k);
		}
		coresused -= reportedcores >> (CHAR_BIT * sizeof(reportedcores) - 1u);// make space for another thread if high bit of reportedcores is set
		return vecasync;
	}else{
		std::vector<std::future<void>> vecasync;
		{// limit the scope of the shared pointer to less than that of vecasync
			std::shared_ptr<std::size_t[]> spchildrencounts(nextcounts);
			do{// bottom-level
				std::size_t currentindex{count[k]};
				++k;
				std::size_t currentcount{currentindex - n};
				if(20u < currentcount){// If replaced by 1u < n, insertion_sort is not needed.
					if((reportedcores & INT_MAX) > coresused){// mask out the possible high bit of reportedcores
						++coresused;// it doesn't matter if this is incremented by more than one thread at a time, as the check for whether to push into the vector is only an optimization to avoid pushing too many tasks into the vector, and pushing a few extra tasks in the vector won't cause any problems
						vecasync.push_back(std::move(std::async(std::launch::async, experimentalMSD_sort_bottomlevel<radixbits, T>, currentcount, array, pcounts, reportedcores | INT_MIN, std::ref(coresused), spchildrencounts)));
					}else experimentalMSD_sort_bottomlevel<radixbits, T>(currentcount, array, pcounts, reportedcores & INT_MAX, coresused, std::shared_ptr<std::size_t[]>{});// do the work here, mask out the possible high bit of reportedcores
				}else insertion_sort<T>(currentcount, array);
				n = currentindex;
				array += currentcount;
				pcounts += mask + 1u;// move to the next set of counts for the next child
			}while(mask >= k);
		}
		coresused -= reportedcores >> (CHAR_BIT * sizeof(reportedcores) - 1u);// make space for another thread if high bit of reportedcores is set
		return vecasync;
	}
}

template<unsigned radixbits, typename T>
__declspec(noalias safebuffers) decltype(auto) experimentalMSD_sort_toplevelinit(std::size_t n, T array[]){
	static unsigned constexpr mask{(1u << radixbits) - 1u}, bit{CHAR_BIT * sizeof(T) - radixbits};
	// no mask, as the top bits are all that matter for the first level of sorting
	std::array<std::size_t, mask + 1u> localcount{};
	std::ptrdiff_t i{static_cast<std::ptrdiff_t>(n >> 3u)};
	while(0 <= --i){// TODO: optimise the count based on architecture, right now it's 8 items at a time, but any number above 0 will do
		T val0{array[0]};
		T val1{array[1]};
		T val2{array[2]};
		T val3{array[3]};
		T val4{array[4]};
		T val5{array[5]};
		T val6{array[6]};
		T val7{array[7]};
		array += 8;
		val0 >>= bit;
		val1 >>= bit;
		val2 >>= bit;
		val3 >>= bit;
		val4 >>= bit;
		val5 >>= bit;
		val6 >>= bit;
		val7 >>= bit;
		++localcount[val0];
		++localcount[val1];
		++localcount[val2];
		++localcount[val3];
		++localcount[val4];
		++localcount[val5];
		++localcount[val6];
		++localcount[val7];
	}
	if(4u & n){// finalise the last 4 to 7 counts
		T val0{array[0]};
		T val1{array[1]};
		T val2{array[2]};
		T val3{array[3]};
		array += 4;
		val0 >>= bit;
		val1 >>= bit;
		val2 >>= bit;
		val3 >>= bit;
		++localcount[val0];
		++localcount[val1];
		++localcount[val2];
		++localcount[val3];
	}
	if(2u & n){// finalise the last 2 or 3 counts
		T val0{array[0]};
		T val1{array[1]};
		array += 2;
		val0 >>= bit;
		val1 >>= bit;
		++localcount[val0];
		++localcount[val1];
	}
	if(1u & n) ++localcount[array[0] >> bit];// finalise the last count
	return localcount;
}

template<unsigned radixbits = 8u, typename T>
__declspec(noalias safebuffers) __forceinline void experimentalMSD_sort(std::size_t n, T array[]){
	if(2u <= n){
		if(2u != n){// filter out 0, 1 and 2 in two simple chained conditional jumps
			static unsigned constexpr mask{(1u << radixbits) - 1u}, bit{CHAR_BIT * sizeof(T) - radixbits};
			unsigned reportedcores{std::thread::hardware_concurrency()};// when this is 0, assume single-core
			reportedcores += static_cast<unsigned>(!reportedcores);// make it at least one
			using inittype = std::invoke_result_t<decltype(experimentalMSD_sort_toplevelinit<radixbits, T>), std::size_t, T[]>;
			inittype count;
			{// Count bucket sizes, only done in the top-level version
				if(unsigned i{reportedcores - 1u}){// allow multitreading
					std::size_t const partitionsize{n / reportedcores};
					std::size_t const partitionremainder{n % reportedcores};
					T *parray{array};
					if(!partitionsize) goto singlethreadinit;// not enough items to distribute, so just do it single-threaded
					std::unique_ptr<std::future<inittype>[]> counterhandles{new std::future<inittype>[i]};
					std::size_t partitiondist{static_cast<std::size_t>(-static_cast<std::ptrdiff_t>(partitionremainder))};// just to be able to use the sign bit
					std::future<inittype> *pcounterhandle{counterhandles.get()};
					do{
						std::size_t partitionblocksize{partitionsize + (partitiondist >> (CHAR_BIT * sizeof(std::size_t) - 1u))};// add on the sign bit to distribute the remainder
						++partitiondist;
						*pcounterhandle++ = std::move(std::async(std::launch::async, experimentalMSD_sort_toplevelinit<radixbits, T>, partitionblocksize, parray));
						parray += partitionblocksize;
					}while(--i);
					// the final partition can just be processed by this thread
					count = std::move(experimentalMSD_sort_toplevelinit<radixbits, T>(partitionsize, parray));// the final partition never takes part in distributing the remainder
					// sum up the local counts into the global count
					pcounterhandle = counterhandles.get();
					i = reportedcores - 1u;
					do{
						std::transform(count.begin(), count.end(), pcounterhandle->get().begin(), count.begin(), std::plus{});
						++pcounterhandle;
						//std::size_t localcount[]{pcounterhandle->get().data()};
						//++pcounterhandle;
						//std::ptrdiff_t j{static_cast<std::ptrdiff_t>(mask)};
						//do{
							//count[j] += localcount[j];
						//}while(0 <= --j);
					}while(--i);
				}else{
singlethreadinit:
					count = experimentalMSD_sort_toplevelinit<radixbits, T>(n, array);
				}
				assert(n == std::accumulate(count.begin(), count.end(), 0u));
			}
			std::size_t *nextcounts{new std::size_t[(mask + 1u) * (mask + 1u)]{}};
			// Create holes
			// these two sequences are unfortunately not bidirectionally parallelizable by the not-stable, in-place MSD radix sort, unlike the stable, out-of-place LSD radix sort version
			std::size_t loc[mask + 1u];
			T unsorted[mask + 1u];
			std::size_t live{};
			std::size_t index{};
			std::size_t i{};
			do{
				loc[i] = index;
				unsorted[live] = array[index];
				std::size_t oldindex{index};
				index += count[i];
				count[i] = index;
				++i;
				live += oldindex < index;
			}while(mask >= i);
			--live;
			// Perform sort and make new counts for the next level in bottom-to-top order
			// raw pointer here, as the soring part can't throw anyway, later on a std::shared_ptr is used to pass on the resource to the children
			std::size_t j{n};
			do{
				T val{unsorted[live]};
				T d{val >> bit};// no mask, as the top bits are all that matter for the first level of sorting
				T dchild{val >> (bit - radixbits)};// same as above, just with double the width
				std::size_t cur{loc[d]};
				++nextcounts[dchild];
				array[cur] = val;
				++cur;
				loc[d] = cur;
				unsorted[live] = array[cur];
				live -= cur == count[d];
				assert((j == 1u)? SIZE_MAX == live || mask >= live : mask >= live);
			}while(--j);
			// Pass on to the next level, using the new counts and async calls for the children
			std::atomic_uint coresused{1u};// do count the current thread, as this can disable multithreading if needed
			std::size_t *pcounts{nextcounts};
			n = 0u;
			std::size_t k{};
			reportedcores += (reportedcores >> 1u) + ((reportedcores + 1u) >> 2u);// allow over-subscription of threads by 75%, as the tasks are not perfectly balanced and some overhead is involved in pushing tasks into the vector and creating threads, so this should allow for better thread utilization without pushing too many tasks into the vector, which would cause too much overhead and reduce performance
			assert(!(reportedcores & INT_MIN));// the top level never has the top bit set
			if constexpr(radixbits < bit){
				// careful with this line, it's basically a matroshka of nested std::vector<std::future<internaltype>> (std::vector<std::future<void>> at the bottom) and took well over an hour to figure out how to write correctly, and the error messages for getting it wrong are very confusing, so don't change it without fully understanding it
				using internaltype = typename std::conditional_t<radixbits < bit, std::invoke_result<decltype(experimentalMSD_sort_midlevel<bit - radixbits, radixbits, T>), std::size_t, T[], std::size_t[], unsigned, std::atomic_uint &, std::shared_ptr<size_t[]>>, std::enable_if<true, void>>::type;
				std::vector<std::future<internaltype>> vecasync;
				vecasync.reserve(reportedcores - 1u);// pre-allocate space here, given the most likely scenario of the current thread pushing out more than enough std::async tasks
				{// limit the scope of the shared pointer to less than that of vecasync
					std::shared_ptr<std::size_t[]> spchildrencounts(nextcounts);
					do{// mid-level
						std::size_t currentindex{count[k]};
						++k;
						std::size_t currentcount{currentindex - n};
						if(20u < currentcount){// If replaced by 1u < n, insertion_sort is not needed.
							if(reportedcores > coresused){
								++coresused;// it doesn't matter if this is incremented by more than one thread at a time, as the check for whether to push into the vector is only an optimization to avoid pushing too many tasks into the vector, and pushing a few extra tasks in the vector won't cause any problems
								vecasync.push_back(std::move(std::async(std::launch::async, experimentalMSD_sort_midlevel<bit - radixbits, radixbits, T>, currentcount, array, pcounts, reportedcores | INT_MIN, std::ref(coresused), spchildrencounts)));
							}else experimentalMSD_sort_midlevel<bit - radixbits, radixbits, T>(currentcount, array, pcounts, reportedcores, coresused, std::shared_ptr<std::size_t[]>{});// do the work here
						}else insertion_sort<T>(currentcount, array);
						n = currentindex;
						array += currentcount;
						pcounts += mask + 1u;// move to the next set of counts for the next child
					}while(mask >= k);
				}
				--coresused;// release it at the end, before the destructor of vecasync
			}else if constexpr(bit){// disable this part if T is sorted in one pass, as the mid-level is not needed in that case
				std::vector<std::future<void>> vecasync;
				vecasync.reserve(reportedcores - 1u);// pre-allocate space here, given the most likely scenario of the current thread pushing out more than enough std::async tasks
				{// limit the scope of the shared pointer to less than that of vecasync
					std::shared_ptr<std::size_t[]> spchildrencounts(nextcounts);
					do{// bottom-level
						std::size_t currentindex{count[k]};
						++k;
						std::size_t currentcount{currentindex - n};
						if(20u < currentcount){// If replaced by 1u < n, insertion_sort is not needed.
							if(reportedcores > coresused){
								++coresused;// it doesn't matter if this is incremented by more than one thread at a time, as the check for whether to push into the vector is only an optimization to avoid pushing too many tasks into the vector, and pushing a few extra tasks in the vector won't cause any problems
								vecasync.push_back(std::move(std::async(std::launch::async, experimentalMSD_sort_bottomlevel<radixbits, T>, currentcount, array, pcounts, reportedcores | INT_MIN, std::ref(coresused), spchildrencounts)));
							}else experimentalMSD_sort_bottomlevel<radixbits, T>(currentcount, array, pcounts, reportedcores, coresused, std::shared_ptr<std::size_t[]>{});// do the work here
						}else insertion_sort<T>(currentcount, array);
						n = currentindex;
						array += currentcount;
						pcounts += mask + 1u;// move to the next set of counts for the next child
					}while(mask >= k);
				}
				--coresused;// release it at the end, before the destructor of vecasync
			}
			assert(!coresused);
		}else{// a little extra as an early out
			T lo{array[0]}, hi{array[1]};
			array[0] = std::min(lo, hi);
			array[1] = std::max(lo, hi);
		}
	}
}

// original trial MSD sort type...
// copied from: https://stackoverflow.com/questions/28884387/how-is-the-most-significant-bit-radix-sort-more-efficient-than-the-least-signifi

void msd_sort(std::size_t n, std::uint64_t array[], std::uint64_t bit = 64 - 8) {
	static unsigned constexpr radixbits{8u}, mask{(1u << radixbits) - 1u};
	// Count bucket sizes
	std::size_t count[mask + 2]{};
	{
		std::ptrdiff_t i{static_cast<std::ptrdiff_t>(n) - 1};
		do{
			++count[(array[i] >> bit & mask) + std::size_t{1u}];
		}while(0 <= --i);
	}
	// Create holes
	std::size_t loc[mask + 1];
	std::uint64_t unsorted[mask + 1];
	std::size_t live{};
	{
		std::size_t index{};// count[0] is zero
		std::size_t i{};
		do{
			loc[i] = index;
			++i;
			unsorted[live] = array[index];
			std::size_t oldindex{index};
			index += count[i];// index up
			count[i] = index;// index up
			live += oldindex < index;
		}while(mask + 1 > i);
		--live;
	}
	// Perform sort
	std::size_t j{n};
	do{
		std::uint64_t val{unsorted[live]};
		std::uint64_t d{val >> bit & mask};
		std::size_t cur{loc[d]};
		array[cur] = val;
		++cur;
		loc[d] = cur;
		unsorted[live] = array[cur];
		live -= cur == count[d + 1];
		assert((j == 1)? SIZE_MAX == live : mask + 1 > live);
	}while(--j);
	if (bit>0) {
		for (size_t i=0; i<mask + 1; i++) {
			n = count[i+1] - count[i];
			if (n > 20) { // If replaced by n > 1, insertion_sort is not needed.
				msd_sort(n, array + count[i], bit-radixbits);
			} else {
				insertion_sort(n, array + count[i]);
			}
		}
	}
}

/* copy from original code for reference
void lsd_sort(std::size_t n, std::uint64_t array[]) {
  const int64_t mask = INT64_C(7);
  std::vector<int64_t> buffer(n);
  for (int64_t bit=0; bit<63; bit+=3) {
    // Copy and count
    int count[9]={};
    for (int i=0; i<n; i++) {
      buffer[i] = array[i];
      count[((array[i]>>bit) & mask) + 1]++;
    }
    // Init writer positions
    for (int i=0; i<8; i++) {
      count[i+1]+=count[i];
    }
    // Perform sort
    for (int i=0; i<n; i++) {
      int64_t val = buffer[i];
      int64_t d = (val>>bit) & mask;
      array[count[d]] = val;
      count[d]++;
    }
  }
}
*/

#ifdef _WIN64
__declspec(noalias safebuffers) wchar_t *WritePaddedu64(wchar_t *pwcOut, std::uint64_t n)noexcept{
	// 18446744073709551615 is the maximum output by this function, the output is padded on the left with spaces if required to get to 20 characters
	static std::uint64_t constexpr fourspaces{static_cast<std::uint64_t>(L' ') << 48 | static_cast<std::uint64_t>(L' ') << 32 | static_cast<std::uint64_t>(L' ') << 16 | static_cast<std::uint64_t>(L' ')};
	reinterpret_cast<std::uint64_t UNALIGNED *>(pwcOut)[0] = fourspaces;
	reinterpret_cast<std::uint64_t UNALIGNED *>(pwcOut)[1] = fourspaces;
	reinterpret_cast<std::uint64_t UNALIGNED *>(pwcOut)[2] = fourspaces;
	reinterpret_cast<std::uint64_t UNALIGNED *>(pwcOut)[3] = fourspaces;
	reinterpret_cast<std::uint64_t UNALIGNED *>(pwcOut)[4] = fourspaces;// writes one wchar_t more than required, but it gets overwritten by the first output in the loop anyway
	pwcOut += 19;// make it point at the least significand digit first
	// original code:
	//do{
	//*pwcOut-- = static_cast<wchar_t>(static_cast<std::uint32_t>(n % 10ui64) + L'0'); the last digit is never substituted with a space
	//n /= 10ui64;
	//}while(n);
	// assembly checked: the x64 compiler eliminates the need for the div instruction here, the x86-32 compiler isn't able to optimize this routine.
	// 64-bit builds do use the 64-bit unsigned division by invariant integers using multiplication method, but the compiler is only capable of selecting the most basic variant of its methods, and not the simplified versions.
	// 32-bit builds use the 64-bit division support library (__aulldvrm(), __aulldiv(), __alldvrm() and __alldiv()) leaving the modulo and division operations completely unoptimized.
#ifdef _WIN64
	if(n > 9ui64){
		std::uint64_t q;
		do{
			// 64-bit unsigned division by invariant integers using multiplication
			// n / 10
			static std::uint64_t constexpr mprime{0xCCCCCCCCCCCCCCCDui64}, d{0x000000000000000Aui64};
			static std::uint8_t constexpr sh_post{3ui8};
			q = __umulh(n, mprime) >> sh_post;
			// n % 10
			// there is no trivially computable remainder for this method
			// given that we actually only need the bottom 4 bits here (the remainder is 4 bits: ceil(log2(divisor - 1 + 1))), a little optimization is allowed
			//std::uint32_t r{static_cast<std::uint32_t>(n) - static_cast<std::uint32_t>(q) * static_cast<std::uint32_t>(d)};
			//*pwcOut-- = static_cast<wchar_t>(r + L'0');
			std::uint32_t rm{static_cast<std::uint32_t>(q) * 4ui32 + static_cast<std::uint32_t>(q)};// lea, q * 5
			rm = rm * 2ui32 - L'0';// lea, q * 10 - L'0'
			*pwcOut-- = static_cast<wchar_t>(static_cast<std::uint32_t>(n) - rm);// sub, n - (q * 10 - L'0')
			n = q;
		}while(q > 9ui64);
	}
	*pwcOut = static_cast<wchar_t>(static_cast<std::uint32_t>(n) + L'0');// the last digit is never substituted with a space
#else
	std::uint32_t nlo{static_cast<std::uint32_t>(n)}, nhi{static_cast<std::uint32_t>(n >> 32)};
	if(nhi){// at most ten iterations of division by ten are needed to reduce the size of the dividend from a 64-bit unsigned integer to a 32-bit unsigned integer
		std::uint32_t qhi;
		do{
			// 64-bit unsigned division by invariant integers using multiplication
			// n / 10
			static std::uint8_t constexpr sh_post{3ui8};
			static std::uint64_t constexpr mprime{0xCCCCCCCCCCCCCCCDui64}, d{0x000000000000000Aui64};
			//std::uint64_t q{__umulh(n, mprime) >> sh_post};
			// given that the top and bottom half of mprime only differ by one, a little optimization is allowed
			std::uint32_t constexpr mprimehi{static_cast<std::uint32_t>(mprime >> 32)};
			static_assert(static_cast<std::uint32_t>(mprime & 0x00000000FFFFFFFFui64) == mprimehi + 1ui32);
			// unsigned 64-by-64-to-128-bit multiply
			std::uint64_t j2{__emulu(nlo, mprimehi)};
			std::uint32_t n1a{static_cast<std::uint32_t>(j2)}, n2{static_cast<std::uint32_t>(j2 >> 32)};
			// calculate __emulu(nlo, mprimehi + 1ui32) and __emulu(nhi, mprimehi + 1ui32) as __emulu(nlo, mprimehi) + nlo and __emulu(nhi, mprimehi) + nhi
			// sum table, from least to most significant word
			// nlo nhi
			// n1a n2
			//     n1a n2
			//     n2a n3
			//         n2a n3
			assert(n2 < 0xFFFFFFFFui32);// even __emulu(0xFFFFFFFFui32, 0xFFFFFFFFui32) will only output 0xFFFFFFFE00000001ui64, so adding the carry to just n2 is not a problem here
			std::uint32_t n0, n1;
			unsigned char EmptyCarry0{_addcarry_u32(_addcarry_u32(_addcarry_u32(0, nlo, n1a, &n0), nhi, n2, &n1), n2, 0ui32, &n2)};// add nlo and nhi to the earlier computed parts
			assert(!EmptyCarry0);// the last add-with-carry cannot carry out
			static_cast<void>(EmptyCarry0);
			static_cast<void>(n0);// the lowest part is discarded, as it cannot carry out when it has no further addend
			std::uint64_t j3{__emulu(nhi, mprimehi)};
			std::uint32_t n2a{static_cast<std::uint32_t>(j3)}, n3{static_cast<std::uint32_t>(j3 >> 32)};
			unsigned char EmptyCarry1{_addcarry_u32(_addcarry_u32(_addcarry_u32(0, n1, n1a, &n1), n2, n3, &n2), n3, 0ui32, &n3)};
			assert(!EmptyCarry1);// the last add-with-carry cannot carry out
			static_cast<void>(EmptyCarry1);
			unsigned char EmptyCarry2{_addcarry_u32(_addcarry_u32(_addcarry_u32(0, n1, n2a, &n1), n2, n2a, &n2), n3, 0ui32, &n3)};
			assert(!EmptyCarry2);// the last add-with-carry cannot carry out
			static_cast<void>(EmptyCarry2);
			// n0 and n1 are discarded
			std::uint32_t at_1[2]{n2, n3}; std::uint64_t t_1{*reinterpret_cast<std::uint64_t UNALIGNED *>(at_1)};// recompose
			std::uint64_t q{t_1 >> sh_post};
			std::uint32_t qlo{static_cast<std::uint32_t>(q)};
			qhi = static_cast<std::uint32_t>(q >> 32);
			// n % 10
			// there is no trivially computable remainder for this method
			// given that we actually only need the bottom 4 bits here (the remainder is 4 bits: ceil(log2(divisor - 1 + 1))), a little optimization is allowed
			//std::uint32_t r{nlo - qlo * static_cast<std::uint32_t>(d)};
			//*pwcOut-- = static_cast<wchar_t>(r + L'0');
			std::uint32_t rm{qlo * 4ui32 + qlo};// lea, q * 5
			nhi = qhi;
			rm = rm * 2ui32 - L'0';// lea, q * 10 - L'0'
			*pwcOut-- = static_cast<wchar_t>(nlo - rm);// sub, n - (q * 10 - L'0')
			nlo = qlo;
		}while(qhi);
		assert(nlo > 9ui32);
		goto HandleLowHalf;// skip the first check, as nlo will guaranteed be larger than 9
	}
	if(nlo > 9ui32){// use a simpler loop when the high half of the dividend is zero
HandleLowHalf:
		std::uint32_t q;
		do{
			// 32-bit unsigned division by invariant integers using multiplication
			// n / 10
			static std::uint8_t constexpr sh_post{3ui8};
			static std::uint32_t constexpr mprime{0xCCCCCCCDui32}, d{0x0000000Aui32};
			q = static_cast<std::uint32_t>(__emulu(nlo, mprime) >> 32) >> sh_post;
			// n % 10
			// there is no trivially computable remainder for this method
			//std::uint32_t r{nlo - q * d};
			//*pwcOut-- = static_cast<wchar_t>(r + L'0');
			std::uint32_t rm{q * 4ui32 + q};// lea, q * 5
			rm = rm * 2ui32 - L'0';// lea, q * 10 - L'0'
			*pwcOut-- = static_cast<wchar_t>(nlo - rm);// sub, n - (q * 10 - L'0')
			nlo = q;
		}while(q > 9ui32);
	}
	*pwcOut = static_cast<wchar_t>(nlo + L'0');// the last digit is never substituted with a space
#endif
	return{pwcOut};
}
#else
extern "C" __declspec(noalias safebuffers) wchar_t *__vectorcall WritePaddedu64ra(wchar_t *pwcOut, std::uint32_t nhi, __m128i nlo)noexcept;// .asm file
__declspec(noalias safebuffers) __forceinline wchar_t *WritePaddedu64(wchar_t *pwcOut, std::uint64_t n)noexcept{return{WritePaddedu64ra(pwcOut, static_cast<std::uint32_t>(n >> 32), _mm_cvtsi32_si128(static_cast<std::int32_t>(n)))};}
#endif

// message handler for about box
__declspec(noalias safebuffers) INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)noexcept{
	static_cast<void>(lParam);
	switch(message){
		case WM_INITDIALOG:
			return{static_cast<INT_PTR>(TRUE)};
		case WM_COMMAND:
			if(LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL){
				BOOL bos{EndDialog(hDlg, LOWORD(wParam))};
				static_cast<void>(bos);
				assert(bos);
				return{static_cast<INT_PTR>(TRUE)};
			}
			break;
	}
	return{static_cast<INT_PTR>(FALSE)};
}

//
// FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
// PURPOSE: Processes messages for the main window.
//
// WM_COMMAND - process the application menu
// WM_PAINT - Paint the main window
// WM_DESTROY - post a quit message and return
//
//
__declspec(noalias safebuffers) LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)noexcept{
	switch(message){
	case WM_COMMAND:
		// Parse the menu selections:
		switch(LOWORD(wParam)){
		case IDM_ABOUT:
			{
				INT_PTR ips{DialogBoxParamW(reinterpret_cast<HINSTANCE>(&__ImageBase), MAKEINTRESOURCEW(IDD_ABOUTBOX), hWnd, About, 0)};
				static_cast<void>(ips);
				assert(ips);
				break;
			}
		case IDM_EXIT:
			{
				BOOL bos{DestroyWindow(hWnd)};
				static_cast<void>(bos);
				assert(bos);
				break;
			}
		default:
			return{DefWindowProcW(hWnd, message, wParam, lParam)};
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc{BeginPaint(hWnd, &ps)};
			static_cast<void>(hdc);// Add any drawing code that uses hdc here...
			BOOL bos{EndPaint(hWnd, &ps)};
			static_cast<void>(bos);
			assert(bos);
			break;
		}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return{DefWindowProcW(hWnd, message, wParam, lParam)};
	}
	return{static_cast<LRESULT>(0)};
}

#if defined(_DEBUG) && defined(_M_IX86)
// verify the initial x87 status and control word values
// the compiler's floating point environment control functions often fail to just report anything properly (#pragma STDC FENV_ACCESS and the items in <cfenv> or <float.h>)
// just issue the instructions in assembly directly
__declspec(noalias safebuffers naked) std::uint16_t __vectorcall Getx87StatusAndControlWords(std::uint16_t &ControlWord)noexcept{
	static_cast<void>(ControlWord);// ControlWord in ecx, return value in ax, disable a false compiler warning for ControlWord
	__asm{
		xor eax, eax; do not stall the pipeline with a partial register update (ax in eax)
		fnstcw [ecx]; store the x87 control word
		fnstsw ax; move the x87 status word to the return register
		ret
	}
}
#endif

// Visual C++ allows global function overrides that are executed by the initialization function before wWinMain()
#include <vcruntime_startup.h>
// unused, the project settings for file open mode control are fine how they are
//extern "C" int __CRTDECL _get_startup_file_mode();// the default version of this function only sets the file open mode to text
// unused, the project settings for floating point control are fine how they are
//extern "C" void __CRTDECL _initialize_default_precision();// the default version of this function only sets the precision to 53 bits (double) instead of the x87 default 64 bits (80-bit x86 extended precision format)
//extern "C" void __CRTDECL _initialize_denormal_control();// the default version of this function does nothing
// noenv - this disables the allocation of the environment strings once passed to wmain()
extern "C" wchar_t *__CRTDECL __dcrt_get_wide_environment_from_os(){return{nullptr};}
extern "C" char *__CRTDECL __dcrt_get_narrow_environment_from_os(){assert(false); return{nullptr};}// Unicode program, so this function should not even get linked in
extern "C" bool __CRTDECL _should_initialize_environment(){return{false};}
// noarg - this disables the allocation of the environment strings once passed to wmain() and the lpCmdLine parameter to wWinMain()
extern "C" _crt_argv_mode __CRTDECL _get_startup_argv_mode(){return{_crt_argv_no_arguments};}
extern "C" char *__CRTDECL _get_narrow_winmain_command_line(){assert(false); return{nullptr};}// Unicode program, so this function should not even get linked in
extern "C" wchar_t *__CRTDECL _get_wide_winmain_command_line(){return{nullptr};}
// There are a few more calls used in the initialization function that can be overridden. Most of these are used to help mapping the crt to the system calls. Keep a lookout for changes in vcruntime_startup.h and the files in its installation folder for these in future compiler versions.

__declspec(noalias safebuffers) int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow){
	static_cast<void>(hInstance);// There is no use for storing this, as it's equal to &__ImageBase by definition.
	assert(&__ImageBase == reinterpret_cast<IMAGE_DOS_HEADER *>(hInstance));
	static_cast<void>(hPrevInstance);// hPrevInstance has no meaning. It was used in 16-bit Windows, but is now always zero.
	assert(!hPrevInstance);
	static_cast<void>(lpCmdLine);// The overridden _get_wide_winmain_command_line() will not be returning a string, as it's never allocated.
	assert(!lpCmdLine);
#ifdef _DEBUG
	{
		// lpCmdLine isn't useful, as GetCommandLineW() and GetCommandLineA() are still available for the programs that need to parse the command line.
		// The internal initialization functions that lead to calling wWinMain() parse the output of GetCommandLineW() anyway, and then store the resulting strings by dynamically allocating memory.
		LPWSTR pszCmdLine{GetCommandLineW()};
		assert(pszCmdLine);
		// GetCommandLineW() internally just reads the process environment block:
#ifdef _WIN64
		std::uintptr_t pProcessEnvironmentBlock{__readgsqword(0x60)};
#else
		std::uintptr_t pProcessEnvironmentBlock{__readfsdword(0x30)};
#endif
		RTL_USER_PROCESS_PARAMETERS *pUPP{reinterpret_cast<PEB *>(pProcessEnvironmentBlock)->ProcessParameters};
		// these conveniently also include a length parameter (in bytes)
		assert(pUPP->ImagePathName.Buffer);
		assert(pUPP->ImagePathName.Length);
		assert(pUPP->ImagePathName.MaximumLength);
		assert(pUPP->CommandLine.Buffer);
		assert(pUPP->CommandLine.Length);
		assert(pUPP->CommandLine.MaximumLength);
		assert(pUPP->CommandLine.Buffer == pszCmdLine);// GetCommandLineW() simply outputs a pointer to the string in the UNICODE_STRING item that is always allocated for the process
#ifdef _WIN64
		UNICODE_STRING *pDesktopInfo{reinterpret_cast<UNICODE_STRING *>(reinterpret_cast<std::uintptr_t>(pUPP) + 0xC0)};
#else
		UNICODE_STRING *pDesktopInfo{reinterpret_cast<UNICODE_STRING *>(reinterpret_cast<std::uintptr_t>(pUPP) + 0x78)};
#endif
		assert(pDesktopInfo->Buffer);// can point to an empty string if simply unnamed
		//assert(pDesktopInfo->Length); empty string is legal
		//assert(pDesktopInfo->MaximumLength); empty string is legal

		// floating point environment intitialization verification
#ifdef _M_IX86
		// verify the initial x87 status and control word values
		std::uint16_t ControlWord;
		std::uint16_t StatusWord{Getx87StatusAndControlWords(ControlWord)};
		assert(!(StatusWord & ((1 << 7) | (1 << 6) | (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | 1)));// The Interrupt Request, Stack Fault, Precision, Underflow, Overflow, Zero divide, Denormalized and Invalid operation exception flags should not be set.
		ControlWord &= ~((1 << 15) | (1 << 14) | (1 << 13) | (1 << 12) | (1 << 7) | (1 << 6));// mask out the unused bits
		assert(ControlWord == ((1 << 9) | (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | 1));// The Rounding Control should be set to nound to nearest or to even if equidistant, the Precision Control should be set to 53 bits (IEEE 754 double equivalent), and the flags for all 6 exception masks should be set.
#endif
		// verify the initial mxcsr value
		std::uint32_t mxcsr{_mm_getcsr()};
		assert(mxcsr == (_MM_MASK_MASK | _MM_ROUND_NEAREST | _MM_FLUSH_ZERO_OFF | _MM_DENORMALS_ZERO_OFF));// no exceptions should be set, all exceptions masked, rounding to nearest or to even if equidistant, subnormal values are not flushed to zero nor interpreted as zero
	}
#endif

	// Verify that the RDTSCP CPU feature which is required for the performance tests is available.
	if(!gk_FBCPUId.RDTSCP){
		MessageBoxW(nullptr, L"RDTSCP CPU feature not available", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
		return{0};// failure status
	}

	// Enable the heap terminate-on-corruption security option.
	BOOL boHeapSetInformation{HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0)};
	static_cast<void>(boHeapSetInformation);
	assert(boHeapSetInformation);

	wchar_t szTicksRu64Text[24];// the debug output strings are filled in here
	// wWinMain entry time
	WritePaddedu64(szTicksRu64Text, PerfCounter100ns());
	*reinterpret_cast<std::uint64_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint64_t>(L'\n') << 32 | static_cast<std::uint64_t>(L'w') << 16 | static_cast<std::uint64_t>(L' ');// the last wchar_t is correctly set to zero here
	// output debug strings to the system
	OutputDebugStringW(szTicksRu64Text);

	{
		// Set time critical process and thread priority and single-processor operating mode.
		// Set the security descriptor to allow changing the process information.
		// Note: NtCurrentProcess()/ZwCurrentProcess(), NtCurrentThread()/ZwCurrentThread() and NtCurrentSession()/ZwCurrentSession() resolve to macros defined to HANDLE (void *) values of (sign-extended) -1, -2 and -3 respectively in Wdm.h. Due to being hard-coded in user- and kernel-mode drivers like this, these values are pretty certain to never change on this platform.
		DWORD dr{SetSecurityInfo(reinterpret_cast<HANDLE>(static_cast<std::intptr_t>(-1)), SE_KERNEL_OBJECT, PROCESS_SET_INFORMATION, nullptr, nullptr, nullptr, nullptr)};
		if(ERROR_SUCCESS != dr){
			MessageBoxW(nullptr, L"SetSecurityInfo() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
			return{0};// failure status
		}
		BOOL boSetPriorityClass{SetPriorityClass(reinterpret_cast<HANDLE>(static_cast<std::intptr_t>(-1)), REALTIME_PRIORITY_CLASS)};
		if(!boSetPriorityClass){
			MessageBoxW(nullptr, L"SetPriorityClass() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
			return{0};// failure status
		}

		// Enable the permissions to use large pages for VirtualAlloc().
		HANDLE hToken;
		BOOL boOpenProcessToken{OpenProcessToken(reinterpret_cast<HANDLE>(static_cast<std::intptr_t>(-1)), TOKEN_ADJUST_PRIVILEGES, &hToken)};
		if(!boOpenProcessToken){
			MessageBoxW(nullptr, L"OpenProcessToken() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
			return{0};// failure status
		}
		// Fill in the struct for AdjustTokenPrivileges().
		struct TOKEN_PRIVILEGES_1UNIT{DWORD PrivilegeCount; LUID_AND_ATTRIBUTES Privilege[1];}Info;
		Info.PrivilegeCount = 1;
		Info.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
		// Get the LUID.
		BOOL boLookupPrivilegeValueW{LookupPrivilegeValueW(nullptr, SE_LOCK_MEMORY_NAME, &Info.Privilege[0].Luid)};
		if(!boLookupPrivilegeValueW){
			MessageBoxW(nullptr, L"LookupPrivilegeValueW() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
			return{0};// failure status
		}
		// Adjust the lock memory privilege.
		BOOL boAdjustTokenPrivileges{AdjustTokenPrivileges(hToken, FALSE, reinterpret_cast<PTOKEN_PRIVILEGES>(&Info), 0, nullptr, nullptr)};
		BOOL boCloseHandle{CloseHandle(hToken)};// cleanup
		static_cast<void>(boCloseHandle);
		assert(boCloseHandle);
		if(!boAdjustTokenPrivileges){
			MessageBoxW(nullptr, L"AdjustTokenPrivileges() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
			return{0};// failure status
		}
	}

	{// unit tests with the 3 simulated 80-bit long double types
		// direct sorting tests with the 80-bit long double types

		rsbd8::helper::longdoubletest80<false, true, true> aji80[7]{
			{0, 0xFFFFu},// -inf
			{0, 0x7FFFu},// +inf
			{0x8000000000000000u, 0xFFFFu},// QNaN, machine indeterminate
			{0xFFFFFFFFFFFFFFFFu, 0x7FFEu},// max normal
			{0, 1u},// min normal
			{0xFFFFFFFFFFFFFFFFu, 0},// max subnormal
			{1u, 0}};// min subnormal
		rsbd8::helper::longdoubletest80<false, true, true> ajo80[_countof(aji80)], ajb80[_countof(aji80)];
		rsbd8::radixsortcopynoalloc(_countof(aji80), aji80, ajo80, ajb80);
		assert(ajo80[0].mantissa == 0x8000000000000000u && ajo80[0].signexponent == 0xFFFFu);// QNaN, machine indeterminate
		assert(ajo80[1].mantissa == 0 && ajo80[1].signexponent == 0xFFFFu);// -inf
		assert(ajo80[2].mantissa == 1u && ajo80[2].signexponent == 0);// min subnormal
		assert(ajo80[3].mantissa == 0xFFFFFFFFFFFFFFFFu && ajo80[3].signexponent == 0);// max subnormal
		assert(ajo80[4].mantissa == 0 && ajo80[4].signexponent == 1u);// min normal
		assert(ajo80[5].mantissa == 0xFFFFFFFFFFFFFFFFu && ajo80[5].signexponent == 0x7FFEu);// max normal
		assert(ajo80[6].mantissa == 0 && ajo80[6].signexponent == 0x7FFFu);// +inf

		std::memset(ajb80, 0, sizeof(ajb80));
		rsbd8::radixsortnoalloc<rsbd8::sortingdirection::dscrevorder>(_countof(aji80), ajo80, ajb80, true);
		assert(ajb80[0].mantissa == 0 && ajb80[0].signexponent == 0x7FFFu);// +inf
		assert(ajb80[1].mantissa == 0xFFFFFFFFFFFFFFFFu && ajb80[1].signexponent == 0x7FFEu);// max normal
		assert(ajb80[2].mantissa == 0 && ajb80[2].signexponent == 1u);// min normal
		assert(ajb80[3].mantissa == 0xFFFFFFFFFFFFFFFFu && ajb80[3].signexponent == 0);// max subnormal
		assert(ajb80[4].mantissa == 1u && ajb80[4].signexponent == 0);// min subnormal
		assert(ajb80[5].mantissa == 0 && ajb80[5].signexponent == 0xFFFFu);// -inf
		assert(ajb80[6].mantissa == 0x8000000000000000u && ajb80[6].signexponent == 0xFFFFu);// QNaN, machine indeterminate

		rsbd8::helper::longdoubletest96<false, true, true> aji96[7]{
			{0, 0xABABFFFFu},// -inf
			{0, 0xD0017FFFu},// +inf
			{0x8000000000000000u, 0xEEEEFFFFu},// QNaN, machine indeterminate
			{0xFFFFFFFFFFFFFFFFu, 0x01017FFEu},// max normal
			{0, 0xFFF80001u},// min normal
			{0xFFFFFFFFFFFFFFFFu, 0xC7C80000u},// max subnormal
			{1u, 0xB3710000u}};// min subnormal
		rsbd8::helper::longdoubletest96<false, true, true> ajo96[_countof(aji96)], ajb96[_countof(aji96)];
		rsbd8::radixsortcopynoalloc(_countof(aji96), aji96, ajo96, ajb96);
		assert(ajo96[0].mantissa == 0x8000000000000000u && ajo96[0].signexponent == 0xEEEEFFFFu);// QNaN, machine indeterminate
		assert(ajo96[1].mantissa == 0 && ajo96[1].signexponent == 0xABABFFFFu);// -inf
		assert(ajo96[2].mantissa == 1u && ajo96[2].signexponent == 0xB3710000u);// min subnormal
		assert(ajo96[3].mantissa == 0xFFFFFFFFFFFFFFFFu && ajo96[3].signexponent == 0xC7C80000u);// max subnormal
		assert(ajo96[4].mantissa == 0 && ajo96[4].signexponent == 0xFFF80001u);// min normal
		assert(ajo96[5].mantissa == 0xFFFFFFFFFFFFFFFFu && ajo96[5].signexponent == 0x01017FFEu);// max normal
		assert(ajo96[6].mantissa == 0 && ajo96[6].signexponent == 0xD0017FFFu);// +inf

		std::memset(ajb96, 0, sizeof(ajb96));
		rsbd8::radixsortnoalloc<rsbd8::sortingdirection::dscrevorder>(_countof(aji96), ajo96, ajb96, true);
		assert(ajb96[0].mantissa == 0 && ajb96[0].signexponent == 0xD0017FFFu);// +inf
		assert(ajb96[1].mantissa == 0xFFFFFFFFFFFFFFFFu && ajb96[1].signexponent == 0x01017FFEu);// max normal
		assert(ajb96[2].mantissa == 0 && ajb96[2].signexponent == 0xFFF80001u);// min normal
		assert(ajb96[3].mantissa == 0xFFFFFFFFFFFFFFFFu && ajb96[3].signexponent == 0xC7C80000u);// max subnormal
		assert(ajb96[4].mantissa == 1u && ajb96[4].signexponent == 0xB3710000u);// min subnormal
		assert(ajb96[5].mantissa == 0 && ajb96[5].signexponent == 0xABABFFFFu);// -inf
		assert(ajb96[6].mantissa == 0x8000000000000000u && ajb96[6].signexponent == 0xEEEEFFFFu);// QNaN, machine indeterminate

		rsbd8::helper::longdoubletest128<false, true, true> aji128[7]{
			{0, 0xBBBBAAAAABABFFFFu},// -inf
			{0, 0x22221111D0017FFFu},// +inf
			{0x8000000000000000u, 0x44443333EEEEFFFFu},// QNaN, machine indeterminate
			{0xFFFFFFFFFFFFFFFFu, 0x6666555501017FFEu},// max normal
			{0, 0x88887777FFF80001u},// min normal
			{0xFFFFFFFFFFFFFFFFu, 0xCCCC9999C7C80000u},// max subnormal
			{1u, 0xFFFFDDDDB3710000u}};// min subnormal
		rsbd8::helper::longdoubletest128<false, true, true> ajo128[_countof(aji128)], ajb128[_countof(aji128)];
		rsbd8::radixsortcopynoalloc(_countof(aji128), aji128, ajo128, ajb128);
		assert(ajo128[0].mantissa == 0x8000000000000000u && ajo128[0].signexponent == 0x44443333EEEEFFFFu);// QNaN, machine indeterminate
		assert(ajo128[1].mantissa == 0 && ajo128[1].signexponent == 0xBBBBAAAAABABFFFFu);// -inf
		assert(ajo128[2].mantissa == 1u && ajo128[2].signexponent == 0xFFFFDDDDB3710000u);// min subnormal
		assert(ajo128[3].mantissa == 0xFFFFFFFFFFFFFFFFu && ajo128[3].signexponent == 0xCCCC9999C7C80000u);// max subnormal
		assert(ajo128[4].mantissa == 0 && ajo128[4].signexponent == 0x88887777FFF80001u);// min normal
		assert(ajo128[5].mantissa == 0xFFFFFFFFFFFFFFFFu && ajo128[5].signexponent == 0x6666555501017FFEu);// max normal
		assert(ajo128[6].mantissa == 0 && ajo128[6].signexponent == 0x22221111D0017FFFu);// +inf

		std::memset(ajb128, 0, sizeof(ajb128));
		rsbd8::radixsortnoalloc<rsbd8::sortingdirection::dscrevorder>(_countof(aji128), ajo128, ajb128, true);
		assert(ajb128[0].mantissa == 0 && ajb128[0].signexponent == 0x22221111D0017FFFu);// +inf
		assert(ajb128[1].mantissa == 0xFFFFFFFFFFFFFFFFu && ajb128[1].signexponent == 0x6666555501017FFEu);// max normal
		assert(ajb128[2].mantissa == 0 && ajb128[2].signexponent == 0x88887777FFF80001u);// min normal
		assert(ajb128[3].mantissa == 0xFFFFFFFFFFFFFFFFu && ajb128[3].signexponent == 0xCCCC9999C7C80000u);// max subnormal
		assert(ajb128[4].mantissa == 1u && ajb128[4].signexponent == 0xFFFFDDDDB3710000u);// min subnormal
		assert(ajb128[5].mantissa == 0 && ajb128[5].signexponent == 0xBBBBAAAAABABFFFFu);// -inf
		assert(ajb128[6].mantissa == 0x8000000000000000u && ajb128[6].signexponent == 0x44443333EEEEFFFFu);// QNaN, machine indeterminate

		// basic indirect sorting tests with the 80-bit long double types

		rsbd8::helper::longdoubletest80<false, true, true> *ako80[_countof(aji80)], *akb80[_countof(aji80)], *aki80[_countof(aji80)]{
			aji80, aji80 + 1, aji80 + 2, aji80 + 3, aji80 + 4, aji80 + 5, aji80 + 6};// indirect input
		rsbd8::radixsortcopynoalloc(_countof(aki80), aki80, ako80, akb80);
		assert(ako80[0]->mantissa == 0x8000000000000000u && ako80[0]->signexponent == 0xFFFFu);// QNaN, machine indeterminate
		assert(ako80[1]->mantissa == 0 && ako80[1]->signexponent == 0xFFFFu);// -inf
		assert(ako80[2]->mantissa == 1u && ako80[2]->signexponent == 0);// min subnormal
		assert(ako80[3]->mantissa == 0xFFFFFFFFFFFFFFFFu && ako80[3]->signexponent == 0);// max subnormal
		assert(ako80[4]->mantissa == 0 && ako80[4]->signexponent == 1u);// min normal
		assert(ako80[5]->mantissa == 0xFFFFFFFFFFFFFFFFu && ako80[5]->signexponent == 0x7FFEu);// max normal
		assert(ako80[6]->mantissa == 0 && ako80[6]->signexponent == 0x7FFFu);// +inf

		std::memset(akb80, 0, sizeof(akb80));
		rsbd8::radixsortnoalloc<rsbd8::sortingdirection::dscrevorder>(_countof(aki80), ako80, akb80, true);
		assert(akb80[0]->mantissa == 0 && akb80[0]->signexponent == 0x7FFFu);// +inf
		assert(akb80[1]->mantissa == 0xFFFFFFFFFFFFFFFFu && akb80[1]->signexponent == 0x7FFEu);// max normal
		assert(akb80[2]->mantissa == 0 && akb80[2]->signexponent == 1u);// min normal
		assert(akb80[3]->mantissa == 0xFFFFFFFFFFFFFFFFu && akb80[3]->signexponent == 0);// max subnormal
		assert(akb80[4]->mantissa == 1u && akb80[4]->signexponent == 0);// min subnormal
		assert(akb80[5]->mantissa == 0 && akb80[5]->signexponent == 0xFFFFu);// -inf
		assert(akb80[6]->mantissa == 0x8000000000000000u && akb80[6]->signexponent == 0xFFFFu);// QNaN, machine indeterminate

		rsbd8::helper::longdoubletest96<false, true, true> *ako96[_countof(aji96)], *akb96[_countof(aji96)], *aki96[_countof(aji96)]{
			aji96, aji96 + 1, aji96 + 2, aji96 + 3, aji96 + 4, aji96 + 5, aji96 + 6};// indirect input
		rsbd8::radixsortcopynoalloc(_countof(aki96), aki96, ako96, akb96);
		assert(ako96[0]->mantissa == 0x8000000000000000u && ako96[0]->signexponent == 0xEEEEFFFFu);// QNaN, machine indeterminate
		assert(ako96[1]->mantissa == 0 && ako96[1]->signexponent == 0xABABFFFFu);// -inf
		assert(ako96[2]->mantissa == 1u && ako96[2]->signexponent == 0xB3710000u);// min subnormal
		assert(ako96[3]->mantissa == 0xFFFFFFFFFFFFFFFFu && ako96[3]->signexponent == 0xC7C80000u);// max subnormal
		assert(ako96[4]->mantissa == 0 && ako96[4]->signexponent == 0xFFF80001u);// min normal
		assert(ako96[5]->mantissa == 0xFFFFFFFFFFFFFFFFu && ako96[5]->signexponent == 0x01017FFEu);// max normal
		assert(ako96[6]->mantissa == 0 && ako96[6]->signexponent == 0xD0017FFFu);// +inf

		std::memset(akb96, 0, sizeof(akb96));
		rsbd8::radixsortnoalloc<rsbd8::sortingdirection::dscrevorder>(_countof(aki96), ako96, akb96, true);
		assert(akb96[0]->mantissa == 0 && akb96[0]->signexponent == 0xD0017FFFu);// +inf
		assert(akb96[1]->mantissa == 0xFFFFFFFFFFFFFFFFu && akb96[1]->signexponent == 0x01017FFEu);// max normal
		assert(akb96[2]->mantissa == 0 && akb96[2]->signexponent == 0xFFF80001u);// min normal
		assert(akb96[3]->mantissa == 0xFFFFFFFFFFFFFFFFu && akb96[3]->signexponent == 0xC7C80000u);// max subnormal
		assert(akb96[4]->mantissa == 1u && akb96[4]->signexponent == 0xB3710000u);// min subnormal
		assert(akb96[5]->mantissa == 0 && akb96[5]->signexponent == 0xABABFFFFu);// -inf
		assert(akb96[6]->mantissa == 0x8000000000000000u && akb96[6]->signexponent == 0xEEEEFFFFu);// QNaN, machine indeterminate

		rsbd8::helper::longdoubletest128<false, true, true> *ako128[_countof(aji128)], *akb128[_countof(aji128)], *aki128[_countof(aji128)]{
			aji128, aji128 + 1, aji128 + 2, aji128 + 3, aji128 + 4, aji128 + 5, aji128 + 6};// indirect input
		rsbd8::radixsortcopynoalloc(_countof(aki128), aki128, ako128, akb128);
		assert(ako128[0]->mantissa == 0x8000000000000000u && ako128[0]->signexponent == 0x44443333EEEEFFFFu);// QNaN, machine indeterminate
		assert(ako128[1]->mantissa == 0 && ako128[1]->signexponent == 0xBBBBAAAAABABFFFFu);// -inf
		assert(ako128[2]->mantissa == 1u && ako128[2]->signexponent == 0xFFFFDDDDB3710000u);// min subnormal
		assert(ako128[3]->mantissa == 0xFFFFFFFFFFFFFFFFu && ako128[3]->signexponent == 0xCCCC9999C7C80000u);// max subnormal
		assert(ako128[4]->mantissa == 0 && ako128[4]->signexponent == 0x88887777FFF80001u);// min normal
		assert(ako128[5]->mantissa == 0xFFFFFFFFFFFFFFFFu && ako128[5]->signexponent == 0x6666555501017FFEu);// max normal
		assert(ako128[6]->mantissa == 0 && ako128[6]->signexponent == 0x22221111D0017FFFu);// +inf

		std::memset(akb128, 0, sizeof(akb128));
		rsbd8::radixsortnoalloc<rsbd8::sortingdirection::dscrevorder>(_countof(aki128), ako128, akb128, true);
		assert(akb128[0]->mantissa == 0 && akb128[0]->signexponent == 0x22221111D0017FFFu);// +inf
		assert(akb128[1]->mantissa == 0xFFFFFFFFFFFFFFFFu && akb128[1]->signexponent == 0x6666555501017FFEu);// max normal
		assert(akb128[2]->mantissa == 0 && akb128[2]->signexponent == 0x88887777FFF80001u);// min normal
		assert(akb128[3]->mantissa == 0xFFFFFFFFFFFFFFFFu && akb128[3]->signexponent == 0xCCCC9999C7C80000u);// max subnormal
		assert(akb128[4]->mantissa == 1u && akb128[4]->signexponent == 0xFFFFDDDDB3710000u);// min subnormal
		assert(akb128[5]->mantissa == 0 && akb128[5]->signexponent == 0xBBBBAAAAABABFFFFu);// -inf
		assert(akb128[6]->mantissa == 0x8000000000000000u && akb128[6]->signexponent == 0x44443333EEEEFFFFu);// QNaN, machine indeterminate
	}

	{// simple unit tests, mostly to track template compile-time issues
		// 2 unit tests: radixsortcopynoalloc(), single-byte enum, no indirection, (explicit template statement) descending and ascending
		enum cert_v_binencoding64 : std::uint8_t{// in groups of ten
			$0, $1, $2, $3, $4, $5, $6, $7, $8, $9,// 0
			$a, $b, $c, $d, $e, $f, $g, $h, $i, $j,// 10
			$k, $l, $m, $n, $o, $p, $q, $r, $s, $t,// 20
			$u, $v, $w, $x, $y, $z, $A, $B, $C, $D,// 30
			$E, $F, $G, $H, $I, $J, $K, $L, $M, $N,// 40
			$O, $P, $Q, $R, $S, $T, $U, $V, $W, $X,// 50
			$Y, $Z, $$, $_};// 60
		// test sequence 0B_iqUE (oblique), with one item from each row; 0:0, 10:i, 20:q, 30:B, 40:E, 50:U, 60:_
		static cert_v_binencoding64 constexpr tein[7]{$0, $B, $_, $i, $q, $U, $E};
		cert_v_binencoding64 teout[_countof(tein)];
		cert_v_binencoding64 tebuf[_countof(tein)];// dummy, as it's an 8-bit type
		rsbd8::radixsortcopynoalloc<rsbd8::sortingdirection::dscrevorder>(_countof(tein), tein, teout, tebuf);
		assert(teout[0] == $_ && teout[1] == $U && teout[2] == $E && teout[3] == $B && teout[4] == $q && teout[5] == $i && teout[6] == $0);
		rsbd8::radixsortcopynoalloc<rsbd8::sortingdirection::ascfwdorder>(_countof(tein), tein, teout);
		assert(teout[0] == $0 && teout[1] == $i && teout[2] == $q && teout[3] == $B && teout[4] == $E && teout[5] == $U && teout[6] == $_);

		// 1 unit test: radixsortnoalloc(), write to buffer, float (multi-byte), no indirection, (implicit template statement) ascending
		std::uint32_t inm[7]{8, 0, 3, 1u << 31 | 2, 3, 1u << 31 | 18, 1u << 31 | 2};
		std::uint32_t outm[_countof(inm)];
		rsbd8::radixsortnoalloc(_countof(inm), reinterpret_cast<float *>(inm), reinterpret_cast<float *>(outm), true);
		assert(outm[0] == (1u << 31 | 18) && outm[1] == (1u << 31 | 2) && outm[2] == (1u << 31 | 2) && outm[3] == 0 && outm[4] == 3 && outm[5] == 3 && outm[6] == 8);

		// 2 unit test, the same as above, but indirect
		std::uint32_t const *inim[7]{outm + 6, outm + 3, outm + 4, outm + 1, outm + 5, outm, outm + 2};
		std::uint32_t const *outim[_countof(inim)];
		std::uint32_t const *bufim[_countof(inim)];
		rsbd8::radixsortcopynoalloc<rsbd8::sortingdirection::dscrevorder>(_countof(inim), reinterpret_cast<float const *const *>(inim), reinterpret_cast<float const **>(outim), reinterpret_cast<float const **>(bufim));
		assert(outim[0] == inim[0] && outim[1] == inim[4] && outim[2] == inim[2] && outim[3] == inim[1] && outim[4] == inim[6] && outim[5] == inim[3] && outim[6] == inim[5]);
		rsbd8::radixsortnoalloc(_countof(inim), reinterpret_cast<float const **>(inim), reinterpret_cast<float const **>(bufim), false);
		assert(*inim[0] == (1u << 31 | 18) && *inim[1] == (1u << 31 | 2) && *inim[2] == (1u << 31 | 2) && *inim[3] == 0 && *inim[4] == 3 && *inim[5] == 3 && *inim[6] == 8);

		// 6 groups of short unit tests: radixsortcopynoalloc() (and one directly to its implementation), 8-byte with first level getter indirection, (implicit template statement) ascending
		// Part of this test is firing up the debugger in "release mode" to see how well the inlining parallel processing fares, or just read the asm output functions directly. (This generates quite a few similar functions for the various cases though.)
#pragma pack(push, 1)
		class Testmeclass{
			std::uint64_t wasted{};// unused, default to zero for this test class
			char misalignoffset{};// unused, default to zero for this test class
		public:
			std::uint64_t co;
			std::int64_t sco;
			constexpr __forceinline std::uint64_t get()const noexcept{return{co};};
			constexpr __forceinline std::uint64_t getwparam(int)const noexcept{return{co};};
			constexpr __forceinline std::uint64_t bget()noexcept{return{co};};
			constexpr __forceinline std::int64_t sget()const noexcept{return{sco};};
			constexpr __forceinline std::int64_t zget()noexcept{return{sco};};
			constexpr __forceinline Testmeclass(std::uint64_t input)noexcept : co{input}, sco{static_cast<std::int64_t>(input) - 1} {};
		};
#pragma pack(pop)
		static std::size_t constexpr sizecontainer{sizeof(Testmeclass)};
		static_assert(25 == sizecontainer);
		static std::size_t constexpr offsetco{rsbd8::getoffsetof<&Testmeclass::co>};
		static_assert(9 == offsetco);
		static std::size_t constexpr offsetsco{rsbd8::getoffsetof<&Testmeclass::sco>};
		static_assert(17 == offsetsco);

		Testmeclass cin[]{8, 0, 6, 4, 0, 2, 6};
		Testmeclass const *fin[_countof(cin)]{cin, cin + 1, cin + 2, cin + 3, cin + 4, cin + 5, cin + 6};
		Testmeclass const *fout[_countof(cin)];
		Testmeclass const *fbuf[_countof(cin)];

		rsbd8::radixsortcopynoalloc<&Testmeclass::get>(_countof(fin), fin, fout, fbuf);
		rsbd8::radixsortcopynoalloc<&Testmeclass::getwparam>(_countof(fin), fin, fout, fbuf, 8);
		rsbd8::radixsortcopynoalloc<&Testmeclass::co>(_countof(fin), fin, fout, fbuf);
		rsbd8::radixsortcopynoalloc<std::uint64_t, 9>(_countof(fin), fin, fout, fbuf);

		rsbd8::radixsortcopynoalloc<&Testmeclass::sget>(_countof(fin), fin, fout, fbuf);
		rsbd8::radixsortcopynoalloc<&Testmeclass::sco>(_countof(fin), fin, fout, fbuf);
		rsbd8::radixsortcopynoalloc<std::int64_t, 17>(_countof(fin), fin, fout, fbuf);

		// correctly fails to compile (not const-correct):
		//rsbd8::radixsortcopynoalloc<&Testmeclass::bget>(_countof(fin), fin, fout, fbuf);
		// correctly fails to compile (not const-correct):
		//rsbd8::radixsortcopynoalloc<&Testmeclass::zget>(_countof(fin), fin, fout, fbuf);

		Testmeclass *yin[_countof(cin)]{cin, cin + 1, cin + 2, cin + 3, cin + 4, cin + 5, cin + 6};
		Testmeclass *yout[_countof(cin)];
		Testmeclass *ybuf[_countof(cin)];

		rsbd8::radixsortcopynoalloc<&Testmeclass::get>(_countof(yin), yin, yout, ybuf);
		rsbd8::radixsortcopynoalloc<&Testmeclass::getwparam>(_countof(yin), yin, yout, ybuf, 8);
		rsbd8::radixsortcopynoalloc<&Testmeclass::co>(_countof(yin), yin, yout, ybuf);
		rsbd8::radixsortcopynoalloc<std::uint64_t, 9>(_countof(yin), yin, yout, ybuf);

		rsbd8::radixsortcopynoalloc<&Testmeclass::sget>(_countof(yin), yin, yout, ybuf);
		rsbd8::radixsortcopynoalloc<&Testmeclass::sco>(_countof(yin), yin, yout, ybuf);
		rsbd8::radixsortcopynoalloc<std::int64_t, 17>(_countof(yin), yin, yout, ybuf);

		// unlike the commented calls above, these work as intended:
		rsbd8::radixsortcopynoalloc<&Testmeclass::bget>(_countof(yin), yin, yout, ybuf);
		rsbd8::radixsortcopynoalloc<&Testmeclass::zget>(_countof(yin), yin, yout, ybuf);

		// TODO: add more unit tests
	}

	// allocate RSBD8_TEST_BATCH_SIZE for the in- and outputs
	assert(sizeof(void *) <= static_cast<std::size_t>(RSBD8_TEST_BATCH_SIZE));// RSBD8_TEST_BATCH_SIZE must be greater than or equal to sizeof(void *)
	assert(static_cast<std::size_t>(PTRDIFF_MAX) >= static_cast<std::size_t>(RSBD8_TEST_BATCH_SIZE));// RSBD8_TEST_BATCH_SIZE must be less than or equal to PTRDIFF_MAX
	SIZE_T upLargePageSize{GetLargePageMinimum()};
	upLargePageSize = !upLargePageSize? 4096 : upLargePageSize;// just set it to 4096 if the system doesn't support large pages
	assert(!(upLargePageSize - 1 & upLargePageSize));// only one bit should be set in the value of upLargePageSize
	std::size_t upLargePageSizem1{upLargePageSize - 1};
	std::size_t upSizeIn{(upLargePageSizem1 & -static_cast<std::ptrdiff_t>(RSBD8_TEST_BATCH_SIZE)) + (RSBD8_TEST_BATCH_SIZE)};// round up to the nearest multiple of upLargePageSize
	void *in{VirtualAlloc(nullptr, upSizeIn, MEM_LARGE_PAGES | MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)};
	if(!in){
		MessageBoxW(nullptr, L"out of memory failure", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
		return{0};// failure status
	}
	std::size_t upSizeOut{(upLargePageSizem1 & -static_cast<std::ptrdiff_t>((RSBD8_TEST_BATCH_SIZE) + (upLargePageSize >> 1))) + ((RSBD8_TEST_BATCH_SIZE) + (upLargePageSize >> 1))};// round up to the nearest multiple of upLargePageSize
	void *oriout{VirtualAlloc(nullptr, upSizeOut, MEM_LARGE_PAGES | MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)};// add half a page
	if(!oriout){
		BOOL boVirtualFree{VirtualFree(in, 0, MEM_RELEASE)};
		static_cast<void>(boVirtualFree);
		assert(boVirtualFree);
		MessageBoxW(nullptr, L"out of memory failure", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
		return{0};// failure status
	}
	void *out{reinterpret_cast<std::int8_t *>(oriout) + (upLargePageSize >> 1)};// offset by half a page, this is an optimization using the processor's addressing methods, and this is used in many memory copy routines

	// measure the TSC execution base time to subtract from the results (method taken from an Intel manual)
	SwitchToThread();// prevent context switching during the benchmark
	std::uint64_t u64init;
	{
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		u64init = u64stop - u64start;

		srand(static_cast<unsigned>(u64start));// prepare a seed for rand()
	}
	{
		// filled initialization of the input part (only done once)
		static_assert(RAND_MAX == 0x7FFF, L"RAND_MAX changed from 0x7FFF (15 bits of data), update this part of the code");
		std::uint64_t *pFIin{reinterpret_cast<std::uint64_t *>(in)};
		std::size_t j{static_cast<std::size_t>(RSBD8_TEST_BATCH_SIZE) / 8};
		do{
			*pFIin++ = static_cast<std::uint64_t>(static_cast<unsigned>(rand())) << 60
				| static_cast<std::uint64_t>(static_cast<unsigned>(rand())) << 45 | static_cast<std::uint64_t>(static_cast<unsigned>(rand())) << 30
				| static_cast<std::uint64_t>(static_cast<unsigned>(rand()) << 15) | static_cast<std::uint64_t>(static_cast<unsigned>(rand()));
		}while(--j);
		if(7 & (RSBD8_TEST_BATCH_SIZE)){
			// this will not set the top 4 bits, but only 7 bytes are needed at most, so this is not an issue
			std::uint64_t fill{static_cast<std::uint64_t>(static_cast<unsigned>(rand())) << 45 | static_cast<std::uint64_t>(static_cast<unsigned>(rand())) << 30
				| static_cast<std::uint64_t>(static_cast<unsigned>(rand()) << 15) | static_cast<std::uint64_t>(static_cast<unsigned>(rand()))};
			if(4 & (RSBD8_TEST_BATCH_SIZE)){
				*reinterpret_cast<std::uint32_t *&>(pFIin)++ = static_cast<std::uint32_t>(fill);
				fill >>= 32;
			}
			if(2 & (RSBD8_TEST_BATCH_SIZE)){
				*reinterpret_cast<std::uint16_t *&>(pFIin)++ = static_cast<std::uint16_t>(fill);
				fill >>= 16;
			}
			if(1 & (RSBD8_TEST_BATCH_SIZE)){
				*reinterpret_cast<std::uint8_t *>(pFIin) = static_cast<std::uint8_t>(fill);
			}
		}
	}

	bool succeeded;// used to check for successful sorting, or repeat runs if needed

	// one experimental part of the MSD radix sort
	do{/* enable this if you have tme to waste, as even the optimised version is just over 21 times slower on 4294967311 B (just over 4 GiB) of data than the LSD radix sort, and the original version is over 100 times slower, so this is not really useful for benchmarking, but it can be used for testing and debugging
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = true;// TODO: not implemented yet...
		experimentalMSD_sort((RSBD8_TEST_BATCH_SIZE) / 8, reinterpret_cast<std::uint64_t *>(out));
		//msd_sort((RSBD8_TEST_BATCH_SIZE) / 8, reinterpret_cast<std::uint64_t *>(out));// the original version

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint64_t MSD experimental radix sort test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 8 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint64_t const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);*/
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort((RSBD8_TEST_BATCH_SIZE) / 16, reinterpret_cast<rsbd8::helper::longdoubletest128<false, true, true> *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"encapsulated long double rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 16 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<rsbd8::helper::longdoubletest128<false, true, true> const *>(out)};
		auto lo{*piter++};
		auto curlo{rsbd8::helper::convertinput<false, true, true, rsbd8::helper::longdoubletest128<false, true, true>>(lo)};
		do{
			auto hi{*piter++};
			auto curhi{rsbd8::helper::convertinput<false, true, true, rsbd8::helper::longdoubletest128<false, true, true>>(hi)};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsortcopy((RSBD8_TEST_BATCH_SIZE) / 16, reinterpret_cast<rsbd8::helper::longdoubletest128<false, true, true> const *>(in), reinterpret_cast<rsbd8::helper::longdoubletest128<false, true, true> *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"encapsulated long double rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 16 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<rsbd8::helper::longdoubletest128<false, true, true> const *>(out)};
		auto lo{*piter++};
		auto curlo{rsbd8::helper::convertinput<false, true, true, rsbd8::helper::longdoubletest128<false, true, true>>(lo)};
		do{
			auto hi{*piter++};
			auto curhi{rsbd8::helper::convertinput<false, true, true, rsbd8::helper::longdoubletest128<false, true, true>>(hi)};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX// 128-bit tests are only available on 64-bit and larger systems
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort((RSBD8_TEST_BATCH_SIZE) / 16, reinterpret_cast<rsbd8::helper::test128<false, false, false> *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint128_t rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 16 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<rsbd8::helper::test128<false, false, false> const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsortcopy((RSBD8_TEST_BATCH_SIZE) / 16, reinterpret_cast<rsbd8::helper::test128<false, false, false> const *>(in), reinterpret_cast<rsbd8::helper::test128<false, false, false> *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint128_t rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 16 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<rsbd8::helper::test128<false, false, false> const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort((RSBD8_TEST_BATCH_SIZE) / 16, reinterpret_cast<rsbd8::helper::test128<false, true, false> *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int128_t rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 16 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<rsbd8::helper::test128<false, true, false> const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsortcopy((RSBD8_TEST_BATCH_SIZE) / 16, reinterpret_cast<rsbd8::helper::test128<false, true, false> const *>(in), reinterpret_cast<rsbd8::helper::test128<false, true, false> *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int128_t rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 16 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<rsbd8::helper::test128<false, true, false> const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort((RSBD8_TEST_BATCH_SIZE) / 16, reinterpret_cast<rsbd8::helper::test128<false, true, true> *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"quad rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 16 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<rsbd8::helper::test128<false, true, true> const *>(out)};
		auto lo{*piter++};
		auto curlo{rsbd8::helper::convertinput<false, true, true, rsbd8::helper::test128<false, true, true>>(lo)};
		do{
			auto hi{*piter++};
			auto curhi{rsbd8::helper::convertinput<false, true, true, rsbd8::helper::test128<false, true, true>>(hi)};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsortcopy((RSBD8_TEST_BATCH_SIZE) / 16, reinterpret_cast<rsbd8::helper::test128<false, true, true> const *>(in), reinterpret_cast<rsbd8::helper::test128<false, true, true> *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"quad rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 16 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<rsbd8::helper::test128<false, true, true> const *>(out)};
		auto lo{*piter++};
		auto curlo{rsbd8::helper::convertinput<false, true, true, rsbd8::helper::test128<false, true, true>>(lo)};
		do{
			auto hi{*piter++};
			auto curhi{rsbd8::helper::convertinput<false, true, true, rsbd8::helper::test128<false, true, true>>(hi)};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
#endif
#ifndef RSBD8_DISABLE_BENCHMARK_EXTERNAL
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		std::sort(std::execution::par_unseq, reinterpret_cast<std::uint64_t *>(out), reinterpret_cast<std::uint64_t *>(out) + (RSBD8_TEST_BATCH_SIZE) / 8);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint64_t std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		std::stable_sort(std::execution::par_unseq, reinterpret_cast<std::uint64_t *>(out), reinterpret_cast<std::uint64_t *>(out) + (RSBD8_TEST_BATCH_SIZE) / 8);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint64_t std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
#endif// RSBD8_DISABLE_BENCHMARK_EXTERNAL
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort((RSBD8_TEST_BATCH_SIZE) / 8, reinterpret_cast<std::uint64_t *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint64_t rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 8 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint64_t const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsortcopy((RSBD8_TEST_BATCH_SIZE) / 8, reinterpret_cast<std::uint64_t const *>(in), reinterpret_cast<std::uint64_t *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint64_t rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 8 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint64_t const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
#ifndef RSBD8_DISABLE_BENCHMARK_EXTERNAL
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		std::sort(std::execution::par_unseq, reinterpret_cast<std::int64_t *>(out), reinterpret_cast<std::int64_t *>(out) + (RSBD8_TEST_BATCH_SIZE) / 8);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int64_t std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		std::stable_sort(std::execution::par_unseq, reinterpret_cast<std::int64_t *>(out), reinterpret_cast<std::int64_t *>(out) + (RSBD8_TEST_BATCH_SIZE) / 8);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int64_t std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
#endif// RSBD8_DISABLE_BENCHMARK_EXTERNAL
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort((RSBD8_TEST_BATCH_SIZE) / 8, reinterpret_cast<std::int64_t *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int64_t rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 8 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::int64_t const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsortcopy((RSBD8_TEST_BATCH_SIZE) / 8, reinterpret_cast<std::int64_t const *>(in), reinterpret_cast<std::int64_t *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int64_t rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 8 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::int64_t const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
#ifndef RSBD8_DISABLE_BENCHMARK_EXTERNAL
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		std::sort(std::execution::par_unseq, reinterpret_cast<double *>(out), reinterpret_cast<double *>(out) + (RSBD8_TEST_BATCH_SIZE) / 8);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"double std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		std::stable_sort(std::execution::par_unseq, reinterpret_cast<double *>(out), reinterpret_cast<double *>(out) + (RSBD8_TEST_BATCH_SIZE) / 8);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"double std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
#endif// RSBD8_DISABLE_BENCHMARK_EXTERNAL
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort((RSBD8_TEST_BATCH_SIZE) / 8, reinterpret_cast<double *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"double rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 8 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint64_t const *>(out)};
		auto lo{*piter++};
		auto curlo{rsbd8::helper::convertinput<false, true, true, std::uint64_t>(lo)};
		do{
			auto hi{*piter++};
			auto curhi{rsbd8::helper::convertinput<false, true, true, std::uint64_t>(hi)};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsortcopy((RSBD8_TEST_BATCH_SIZE) / 8, reinterpret_cast<double const *>(in), reinterpret_cast<double *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"double rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 8 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint64_t const *>(out)};
		auto lo{*piter++};
		auto curlo{rsbd8::helper::convertinput<false, true, true, std::uint64_t>(lo)};
		do{
			auto hi{*piter++};
			auto curhi{rsbd8::helper::convertinput<false, true, true, std::uint64_t>(hi)};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
#ifndef RSBD8_DISABLE_BENCHMARK_EXTERNAL
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		std::sort(std::execution::par_unseq, reinterpret_cast<std::uint32_t *>(out), reinterpret_cast<std::uint32_t *>(out) + (RSBD8_TEST_BATCH_SIZE) / 4);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint32_t std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		std::stable_sort(std::execution::par_unseq, reinterpret_cast<std::uint32_t *>(out), reinterpret_cast<std::uint32_t *>(out) + (RSBD8_TEST_BATCH_SIZE) / 4);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint32_t std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
#endif// RSBD8_DISABLE_BENCHMARK_EXTERNAL
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort((RSBD8_TEST_BATCH_SIZE) / 4, reinterpret_cast<std::uint32_t *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint32_t rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 4 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint32_t const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsortcopy((RSBD8_TEST_BATCH_SIZE) / 4, reinterpret_cast<std::uint32_t const *>(in), reinterpret_cast<std::uint32_t *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint32_t rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 4 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint32_t const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
#ifndef RSBD8_DISABLE_BENCHMARK_EXTERNAL
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		std::sort(std::execution::par_unseq, reinterpret_cast<std::int32_t *>(out), reinterpret_cast<std::int32_t *>(out) + (RSBD8_TEST_BATCH_SIZE) / 4);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int32_t std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		std::stable_sort(std::execution::par_unseq, reinterpret_cast<std::int32_t *>(out), reinterpret_cast<std::int32_t *>(out) + (RSBD8_TEST_BATCH_SIZE) / 4);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int32_t std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
#endif// RSBD8_DISABLE_BENCHMARK_EXTERNAL
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort((RSBD8_TEST_BATCH_SIZE) / 4, reinterpret_cast<std::int32_t *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int32_t rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 4 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::int32_t const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsortcopy((RSBD8_TEST_BATCH_SIZE) / 4, reinterpret_cast<std::int32_t const *>(in), reinterpret_cast<std::int32_t *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int32_t rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 4 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::int32_t const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
#ifndef RSBD8_DISABLE_BENCHMARK_EXTERNAL
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		std::sort(std::execution::par_unseq, reinterpret_cast<float *>(out), reinterpret_cast<float *>(out) + (RSBD8_TEST_BATCH_SIZE) / 4);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"float std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		std::stable_sort(std::execution::par_unseq, reinterpret_cast<float *>(out), reinterpret_cast<float *>(out) + (RSBD8_TEST_BATCH_SIZE) / 4);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"float std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
#endif// RSBD8_DISABLE_BENCHMARK_EXTERNAL
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort((RSBD8_TEST_BATCH_SIZE) / 4, reinterpret_cast<float *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"float rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 4 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint32_t const *>(out)};
		auto lo{*piter++};
		auto curlo{rsbd8::helper::convertinput<false, true, true, std::uint32_t>(lo)};
		do{
			auto hi{*piter++};
			auto curhi{rsbd8::helper::convertinput<false, true, true, std::uint32_t>(hi)};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsortcopy((RSBD8_TEST_BATCH_SIZE) / 4, reinterpret_cast<float const *>(in), reinterpret_cast<float *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"float rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 4 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint32_t const *>(out)};
		auto lo{*piter++};
		auto curlo{rsbd8::helper::convertinput<false, true, true, std::uint32_t>(lo)};
		do{
			auto hi{*piter++};
			auto curhi{rsbd8::helper::convertinput<false, true, true, std::uint32_t>(hi)};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
#ifndef RSBD8_DISABLE_BENCHMARK_EXTERNAL
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		std::sort(std::execution::par_unseq, reinterpret_cast<std::uint16_t *>(out), reinterpret_cast<std::uint16_t *>(out) + (RSBD8_TEST_BATCH_SIZE) / 2);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint16_t std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		std::stable_sort(std::execution::par_unseq, reinterpret_cast<std::uint16_t *>(out), reinterpret_cast<std::uint16_t *>(out) + (RSBD8_TEST_BATCH_SIZE) / 2);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint16_t std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
#endif// RSBD8_DISABLE_BENCHMARK_EXTERNAL
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort((RSBD8_TEST_BATCH_SIZE) / 2, reinterpret_cast<std::uint16_t *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint16_t rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 2 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint16_t const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsortcopy((RSBD8_TEST_BATCH_SIZE) / 2, reinterpret_cast<std::uint16_t const *>(in), reinterpret_cast<std::uint16_t *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint16_t rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 2 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint16_t const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
#ifndef RSBD8_DISABLE_BENCHMARK_EXTERNAL
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		std::sort(std::execution::par_unseq, reinterpret_cast<std::int16_t *>(out), reinterpret_cast<std::int16_t *>(out) + (RSBD8_TEST_BATCH_SIZE) / 2);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int16_t std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		std::stable_sort(std::execution::par_unseq, reinterpret_cast<std::int16_t *>(out), reinterpret_cast<std::int16_t *>(out) + (RSBD8_TEST_BATCH_SIZE) / 2);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int16_t std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
#endif// RSBD8_DISABLE_BENCHMARK_EXTERNAL
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort((RSBD8_TEST_BATCH_SIZE) / 2, reinterpret_cast<std::int16_t *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int16_t rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 2 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::int16_t const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsortcopy((RSBD8_TEST_BATCH_SIZE) / 2, reinterpret_cast<std::int16_t const *>(in), reinterpret_cast<std::int16_t *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int16_t rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 2 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::int16_t const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort<rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::forcefloatingp>((RSBD8_TEST_BATCH_SIZE) / 2, reinterpret_cast<std::uint16_t *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"half rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 2 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint16_t const *>(out)};
		auto lo{*piter++};
		auto curlo{rsbd8::helper::convertinput<false, true, true, std::uint16_t>(lo)};
		do{
			auto hi{*piter++};
			auto curhi{rsbd8::helper::convertinput<false, true, true, std::uint16_t>(hi)};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsortcopy<rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::forcefloatingp>((RSBD8_TEST_BATCH_SIZE) / 2, reinterpret_cast<std::uint16_t const *>(in), reinterpret_cast<std::uint16_t *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"half rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) / 2 - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint16_t const *>(out)};
		auto lo{*piter++};
		auto curlo{rsbd8::helper::convertinput<false, true, true, std::uint16_t>(lo)};
		do{
			auto hi{*piter++};
			auto curhi{rsbd8::helper::convertinput<false, true, true, std::uint16_t>(hi)};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
#ifndef RSBD8_DISABLE_BENCHMARK_EXTERNAL
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		std::sort(std::execution::par_unseq, reinterpret_cast<std::uint8_t *>(out), reinterpret_cast<std::uint8_t *>(out) + (RSBD8_TEST_BATCH_SIZE));

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint8_t std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		std::stable_sort(std::execution::par_unseq, reinterpret_cast<std::uint8_t *>(out), reinterpret_cast<std::uint8_t *>(out) + (RSBD8_TEST_BATCH_SIZE));

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint8_t std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
#endif// RSBD8_DISABLE_BENCHMARK_EXTERNAL
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort((RSBD8_TEST_BATCH_SIZE), reinterpret_cast<std::uint8_t *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint8_t rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint8_t const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsortcopy((RSBD8_TEST_BATCH_SIZE), reinterpret_cast<std::uint8_t const *>(in), reinterpret_cast<std::uint8_t *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint8_t rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint8_t const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
#ifndef RSBD8_DISABLE_BENCHMARK_EXTERNAL
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		std::sort(std::execution::par_unseq, reinterpret_cast<std::int8_t *>(out), reinterpret_cast<std::int8_t *>(out) + (RSBD8_TEST_BATCH_SIZE));

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int8_t std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		std::stable_sort(std::execution::par_unseq, reinterpret_cast<std::int8_t *>(out), reinterpret_cast<std::int8_t *>(out) + (RSBD8_TEST_BATCH_SIZE));

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int8_t std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
#endif// RSBD8_DISABLE_BENCHMARK_EXTERNAL
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort((RSBD8_TEST_BATCH_SIZE), reinterpret_cast<std::int8_t *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int8_t rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::int8_t const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsortcopy((RSBD8_TEST_BATCH_SIZE), reinterpret_cast<std::int8_t const *>(in), reinterpret_cast<std::int8_t *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int8_t rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::int8_t const *>(out)};
		auto curlo{*piter++};
		do{
			auto curhi{*piter++};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort<rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::forcefloatingp>((RSBD8_TEST_BATCH_SIZE), reinterpret_cast<std::uint8_t *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"mini rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint8_t const *>(out)};
		auto lo{*piter++};
		auto curlo{rsbd8::helper::convertinput<false, true, true, std::uint8_t>(lo)};
		do{
			auto hi{*piter++};
			auto curhi{rsbd8::helper::convertinput<false, true, true, std::uint8_t>(hi)};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::memcpy(out, in, (RSBD8_TEST_BATCH_SIZE));// copy, and warm up some of the caches

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsortcopy<rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::forcefloatingp>((RSBD8_TEST_BATCH_SIZE), reinterpret_cast<std::uint8_t const *>(in), reinterpret_cast<std::uint8_t *>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"mini rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{(RSBD8_TEST_BATCH_SIZE) - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint8_t const *>(out)};
		auto lo{*piter++};
		auto curlo{rsbd8::helper::convertinput<false, true, true, std::uint8_t>(lo)};
		do{
			auto hi{*piter++};
			auto curhi{rsbd8::helper::convertinput<false, true, true, std::uint8_t>(hi)};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		rsbd8::helper::longdoubletest128<false, true, true> const *pSource{reinterpret_cast<rsbd8::helper::longdoubletest128<false, true, true> const *>(in)};
		rsbd8::helper::longdoubletest128<false, true, true> const **pDest{reinterpret_cast<rsbd8::helper::longdoubletest128<false, true, true> const **>(out)};
		std::size_t testsize{(RSBD8_TEST_BATCH_SIZE) / std::max(sizeof(rsbd8::helper::longdoubletest128<false, true, true>), sizeof(void *))};
		std::size_t i{testsize};
		do{
			*pDest++ = pSource++;
		}while(--i);

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort(testsize, reinterpret_cast<rsbd8::helper::longdoubletest128<false, true, true> **>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"encapsulated long double, indirect rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{testsize - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<rsbd8::helper::longdoubletest128<false, true, true> const *const *>(out)};
		auto lo{*piter++};
		auto curlo{rsbd8::helper::convertinput<false, true, true, rsbd8::helper::longdoubletest128<false, true, true>>(*lo)};
		do{
			auto hi{*piter++};
			auto curhi{rsbd8::helper::convertinput<false, true, true, rsbd8::helper::longdoubletest128<false, true, true>>(*hi)};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
#if 0xFFFFFFFFFFFFFFFFu <= UINTPTR_MAX// 128-bit tests are only available on 64-bit and larger systems
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		rsbd8::helper::test128<false, true, true> const *pSource{reinterpret_cast<rsbd8::helper::test128<false, true, true> const *>(in)};
		rsbd8::helper::test128<false, true, true> const **pDest{reinterpret_cast<rsbd8::helper::test128<false, true, true> const **>(out)};
		std::size_t testsize{(RSBD8_TEST_BATCH_SIZE) / std::max(sizeof(rsbd8::helper::test128<false, true, true>), sizeof(void *))};
		std::size_t i{testsize};
		do{
			*pDest++ = pSource++;
		}while(--i);

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort(testsize, reinterpret_cast<rsbd8::helper::test128<false, false, false> **>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint128_t, indirect rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{testsize - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<rsbd8::helper::test128<false, false, false> const *const *>(out)};
		auto lo{*piter++};
		auto curlo{*lo};
		do{
			auto hi{*piter++};
			auto curhi{*hi};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		rsbd8::helper::test128<false, true, true> const *pSource{reinterpret_cast<rsbd8::helper::test128<false, true, true> const *>(in)};
		rsbd8::helper::test128<false, true, true> const **pDest{reinterpret_cast<rsbd8::helper::test128<false, true, true> const **>(out)};
		std::size_t testsize{(RSBD8_TEST_BATCH_SIZE) / std::max(sizeof(rsbd8::helper::test128<false, true, true>), sizeof(void *))};
		std::size_t i{testsize};
		do{
			*pDest++ = pSource++;
		}while(--i);

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort(testsize, reinterpret_cast<rsbd8::helper::test128<false, true, true> **>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"quad, indirect rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{testsize - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<rsbd8::helper::test128<false, true, true> const *const *>(out)};
		auto lo{*piter++};
		auto curlo{rsbd8::helper::convertinput<false, true, true, rsbd8::helper::test128<false, true, true>>(*lo)};
		do{
			auto hi{*piter++};
			auto curhi{rsbd8::helper::convertinput<false, true, true, rsbd8::helper::test128<false, true, true>>(*hi)};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
#endif
#ifndef RSBD8_DISABLE_BENCHMARK_EXTERNAL
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::uint64_t const *pSource{reinterpret_cast<std::uint64_t const *>(in)};
		std::uint64_t const **pDest{reinterpret_cast<std::uint64_t const **>(out)};
		std::size_t testsize{(RSBD8_TEST_BATCH_SIZE) / std::max(sizeof(std::uint64_t), sizeof(void *))};
		std::size_t i{testsize};
		do{
			*pDest++ = pSource++;
		}while(--i);

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64wstart{__rdtsc()};

		auto indlesslambda{[]<typename T>(T a, T b){return *a < *b;}};
		std::stable_sort(std::execution::par_unseq, reinterpret_cast<std::uint64_t **>(out), reinterpret_cast<std::uint64_t **>(out) + testsize, indlesslambda);

		// stop measuring
		std::uint64_t u64wstop;
		{
			unsigned int uAux;// unused
			u64wstop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64wstop - u64wstart - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint64_t, indirect std::stable_sort() test\n"	);
		OutputDebugStringW(szTicksRu64Text);
	}
#endif// RSBD8_DISABLE_BENCHMARK_EXTERNAL
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::uint64_t const *pSource{reinterpret_cast<std::uint64_t const *>(in)};
		std::uint64_t const **pDest{reinterpret_cast<std::uint64_t const **>(out)};
		std::size_t testsize{(RSBD8_TEST_BATCH_SIZE) / std::max(sizeof(std::uint64_t), sizeof(void *))};
		std::size_t i{testsize};
		do{
			*pDest++ = pSource++;
		}while(--i);

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64wstart{__rdtsc()};

		succeeded = rsbd8::radixsort(testsize, reinterpret_cast<std::uint64_t **>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64wstop;
		{
			unsigned int uAux;// unused
			u64wstop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64wstop - u64wstart - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint64_t, indirect rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{testsize - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint64_t const *const *>(out)};
		auto lo{*piter++};
		auto curlo{*lo};
		do{
			auto hi{*piter++};
			auto curhi{*hi};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
#ifndef RSBD8_DISABLE_BENCHMARK_EXTERNAL
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::uint64_t const *pSource{reinterpret_cast<std::uint64_t const *>(in)};
		std::uint64_t const **pDest{reinterpret_cast<std::uint64_t const **>(out)};
		std::size_t testsize{(RSBD8_TEST_BATCH_SIZE) / std::max(sizeof(std::uint64_t), sizeof(void *))};
		std::size_t i{testsize};
		do{
			*pDest++ = pSource++;
		}while(--i);

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64wstart{__rdtsc()};

		auto indlesslambda{[]<typename T>(T a, T b){return *a < *b;}};
		std::stable_sort(std::execution::par_unseq, reinterpret_cast<double **>(out), reinterpret_cast<double **>(out) + testsize, indlesslambda);

		// stop measuring
		std::uint64_t u64wstop;
		{
			unsigned int uAux;// unused
			u64wstop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64wstop - u64wstart - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"double, indirect std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
#endif// RSBD8_DISABLE_BENCHMARK_EXTERNAL
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::uint64_t const *pSource{reinterpret_cast<std::uint64_t const *>(in)};
		std::uint64_t const **pDest{reinterpret_cast<std::uint64_t const **>(out)};
		std::size_t testsize{(RSBD8_TEST_BATCH_SIZE) / std::max(sizeof(std::uint64_t), sizeof(void *))};
		std::size_t i{testsize};
		do{
			*pDest++ = pSource++;
		}while(--i);

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort(testsize, reinterpret_cast<double **>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"double, indirect rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{testsize - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint64_t const *const *>(out)};
		auto lo{*piter++};
		auto curlo{rsbd8::helper::convertinput<false, true, true, std::uint64_t>(*lo)};
		do{
			auto hi{*piter++};
			auto curhi{rsbd8::helper::convertinput<false, true, true, std::uint64_t>(*hi)};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
#ifndef RSBD8_DISABLE_BENCHMARK_EXTERNAL
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::uint32_t const *pSource{reinterpret_cast<std::uint32_t const *>(in)};
		std::uint32_t const **pDest{reinterpret_cast<std::uint32_t const **>(out)};
		std::size_t testsize{(RSBD8_TEST_BATCH_SIZE) / std::max(sizeof(std::uint32_t), sizeof(void *))};
		std::size_t i{testsize};
		do{
			*pDest++ = pSource++;
		}while(--i);

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64wstart{__rdtsc()};

		auto indlesslambda{[]<typename T>(T a, T b){return *a < *b;}};
		std::stable_sort(std::execution::par_unseq, reinterpret_cast<std::uint32_t **>(out), reinterpret_cast<std::uint32_t **>(out) + testsize, indlesslambda);

		// stop measuring
		std::uint64_t u64wstop;
		{
			unsigned int uAux;// unused
			u64wstop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64wstop - u64wstart - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint32_t, indirect std::stable_sort() test\n"	);
		OutputDebugStringW(szTicksRu64Text);
	}
#endif// RSBD8_DISABLE_BENCHMARK_EXTERNAL
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::uint32_t const *pSource{reinterpret_cast<std::uint32_t const *>(in)};
		std::uint32_t const **pDest{reinterpret_cast<std::uint32_t const **>(out)};
		std::size_t testsize{(RSBD8_TEST_BATCH_SIZE) / std::max(sizeof(std::uint32_t), sizeof(void *))};
		std::size_t i{testsize};
		do{
			*pDest++ = pSource++;
		}while(--i);

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64wstart{__rdtsc()};

		succeeded = rsbd8::radixsort(testsize, reinterpret_cast<std::uint32_t **>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64wstop;
		{
			unsigned int uAux;// unused
			u64wstop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64wstop - u64wstart - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint32_t, indirect rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{testsize - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint32_t const *const *>(out)};
		auto lo{*piter++};
		auto curlo{*lo};
		do{
			auto hi{*piter++};
			auto curhi{*hi};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
#ifndef RSBD8_DISABLE_BENCHMARK_EXTERNAL
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::uint32_t const *pSource{reinterpret_cast<std::uint32_t const *>(in)};
		std::uint32_t const **pDest{reinterpret_cast<std::uint32_t const **>(out)};
		std::size_t testsize{(RSBD8_TEST_BATCH_SIZE) / std::max(sizeof(std::uint32_t), sizeof(void *))};
		std::size_t i{testsize};
		do{
			*pDest++ = pSource++;
		}while(--i);

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64wstart{__rdtsc()};

		auto indlesslambda{[]<typename T>(T a, T b){return *a < *b;}};
		std::stable_sort(std::execution::par_unseq, reinterpret_cast<float **>(out), reinterpret_cast<float **>(out) + testsize, indlesslambda);

		// stop measuring
		std::uint64_t u64wstop;
		{
			unsigned int uAux;// unused
			u64wstop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64wstop - u64wstart - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"float, indirect std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
#endif// RSBD8_DISABLE_BENCHMARK_EXTERNAL
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::uint32_t const *pSource{reinterpret_cast<std::uint32_t const *>(in)};
		std::uint32_t const **pDest{reinterpret_cast<std::uint32_t const **>(out)};
		std::size_t testsize{(RSBD8_TEST_BATCH_SIZE) / std::max(sizeof(std::uint32_t), sizeof(void *))};
		std::size_t i{testsize};
		do{
			*pDest++ = pSource++;
		}while(--i);

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort(testsize, reinterpret_cast<float **>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"float, indirect rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{testsize - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint32_t const *const *>(out)};
		auto lo{*piter++};
		auto curlo{rsbd8::helper::convertinput<false, true, true, std::uint32_t>(*lo)};
		do{
			auto hi{*piter++};
			auto curhi{rsbd8::helper::convertinput<false, true, true, std::uint32_t>(*hi)};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
#ifndef RSBD8_DISABLE_BENCHMARK_EXTERNAL
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::uint16_t const *pSource{reinterpret_cast<std::uint16_t const *>(in)};
		std::uint16_t const **pDest{reinterpret_cast<std::uint16_t const **>(out)};
		std::size_t testsize{(RSBD8_TEST_BATCH_SIZE) / std::max(sizeof(std::uint16_t), sizeof(void *))};
		std::size_t i{testsize};
		do{
			*pDest++ = pSource++;
		}while(--i);

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64wstart{__rdtsc()};

		auto indlesslambda{[]<typename T>(T a, T b){return *a < *b;}};
		std::stable_sort(std::execution::par_unseq, reinterpret_cast<std::uint16_t **>(out), reinterpret_cast<std::uint16_t **>(out) + testsize, indlesslambda);

		// stop measuring
		std::uint64_t u64wstop;
		{
			unsigned int uAux;// unused
			u64wstop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64wstop - u64wstart - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint16_t, indirect std::stable_sort() test\n"	);
		OutputDebugStringW(szTicksRu64Text);
	}
#endif// RSBD8_DISABLE_BENCHMARK_EXTERNAL
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::uint16_t const *pSource{reinterpret_cast<std::uint16_t const *>(in)};
		std::uint16_t const **pDest{reinterpret_cast<std::uint16_t const **>(out)};
		std::size_t testsize{(RSBD8_TEST_BATCH_SIZE) / std::max(sizeof(std::uint16_t), sizeof(void *))};
		std::size_t i{testsize};
		do{
			*pDest++ = pSource++;
		}while(--i);

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64wstart{__rdtsc()};

		succeeded = rsbd8::radixsort(testsize, reinterpret_cast<std::uint16_t **>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64wstop;
		{
			unsigned int uAux;// unused
			u64wstop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64wstop - u64wstart - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint16_t, indirect rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{testsize - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint16_t const *const *>(out)};
		auto lo{*piter++};
		auto curlo{*lo};
		do{
			auto hi{*piter++};
			auto curhi{*hi};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::uint16_t const *pSource{reinterpret_cast<std::uint16_t const *>(in)};
		std::uint16_t const **pDest{reinterpret_cast<std::uint16_t const **>(out)};
		std::size_t testsize{(RSBD8_TEST_BATCH_SIZE) / std::max(sizeof(std::uint16_t), sizeof(void *))};
		std::size_t i{testsize};
		do{
			*pDest++ = pSource++;
		}while(--i);

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort<rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::forcefloatingp>(testsize, reinterpret_cast<std::uint16_t **>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"half, indirect rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{testsize - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint16_t const *const *>(out)};
		auto lo{*piter++};
		auto curlo{rsbd8::helper::convertinput<false, true, true, std::uint16_t>(*lo)};
		do{
			auto hi{*piter++};
			auto curhi{rsbd8::helper::convertinput<false, true, true, std::uint16_t>(*hi)};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
#ifndef RSBD8_DISABLE_BENCHMARK_EXTERNAL
	{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::uint8_t const *pSource{reinterpret_cast<std::uint8_t const *>(in)};
		std::uint8_t const **pDest{reinterpret_cast<std::uint8_t const **>(out)};
		std::size_t testsize{(RSBD8_TEST_BATCH_SIZE) / std::max(sizeof(std::uint8_t), sizeof(void *))};
		std::size_t i{testsize};
		do{
			*pDest++ = pSource++;
		}while(--i);

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64wstart{__rdtsc()};

		auto indlesslambda{[]<typename T>(T a, T b){return *a < *b;}};
		std::stable_sort(std::execution::par_unseq, reinterpret_cast<std::uint8_t **>(out), reinterpret_cast<std::uint8_t **>(out) + testsize, indlesslambda);

		// stop measuring
		std::uint64_t u64wstop;
		{
			unsigned int uAux;// unused
			u64wstop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64wstop - u64wstart - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint8_t, indirect std::stable_sort() test\n"	);
		OutputDebugStringW(szTicksRu64Text);
	}
#endif// RSBD8_DISABLE_BENCHMARK_EXTERNAL
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::uint8_t const *pSource{reinterpret_cast<std::uint8_t const *>(in)};
		std::uint8_t const **pDest{reinterpret_cast<std::uint8_t const **>(out)};
		std::size_t testsize{(RSBD8_TEST_BATCH_SIZE) / std::max(sizeof(std::uint8_t), sizeof(void *))};
		std::size_t i{testsize};
		do{
			*pDest++ = pSource++;
		}while(--i);

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64start{__rdtsc()};

		succeeded = rsbd8::radixsort<rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::forcefloatingp>(testsize, reinterpret_cast<std::uint8_t **>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"mini, indirect rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{testsize - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint8_t const *const *>(out)};
		auto lo{*piter++};
		auto curlo{rsbd8::helper::convertinput<false, true, true, std::uint8_t>(*lo)};
		do{
			auto hi{*piter++};
			auto curhi{rsbd8::helper::convertinput<false, true, true, std::uint8_t>(*hi)};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);
	do{
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()

		std::uint8_t const *pSource{reinterpret_cast<std::uint8_t const *>(in)};
		std::uint8_t const **pDest{reinterpret_cast<std::uint8_t const **>(out)};
		std::size_t testsize{(RSBD8_TEST_BATCH_SIZE) / std::max(sizeof(std::uint8_t), sizeof(void *))};
		std::size_t i{testsize};
		do{
			*pDest++ = pSource++;
		}while(--i);

		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		std::uint64_t u64wstart{__rdtsc()};

		succeeded = rsbd8::radixsort(testsize, reinterpret_cast<std::uint8_t **>(out), upLargePageSize);

		// stop measuring
		std::uint64_t u64wstop;
		{
			unsigned int uAux;// unused
			u64wstop = __rdtscp(&uAux);
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
		}
		WritePaddedu64(szTicksRu64Text, u64wstop - u64wstart - u64init);
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint8_t, indirect rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifdef _DEBUG
		// verify if sorted
		size_t k{testsize - 1};// -1 for the intial bounds
		auto piter{reinterpret_cast<std::uint8_t const *const *>(out)};
		auto lo{*piter++};
		auto curlo{*lo};
		do{
			auto hi{*piter++};
			auto curhi{*hi};
			if(curhi < curlo) __debugbreak();// break on error, this is more useful than using std::is_sorted(), as the pointer and two current values can be analysed here
			// shift up by one
			lo = hi;
			curlo = curhi;
		}while(--k);
#endif
	}while(!succeeded);

	// benchmark finished time
	WritePaddedu64(szTicksRu64Text, PerfCounter100ns());
	*reinterpret_cast<std::uint64_t UNALIGNED*>(szTicksRu64Text + 20) = static_cast<std::uint64_t>(L'\n') << 32 | static_cast<std::uint64_t>(L'b') << 16 | static_cast<std::uint64_t>(L' ');// the last wchar_t is correctly set to zero here
	// output debug strings to the system
	OutputDebugStringW(szTicksRu64Text);

	BOOL boVirtualFreeIn{VirtualFree(in, 0, MEM_RELEASE)};
	static_cast<void>(boVirtualFreeIn);
	assert(boVirtualFreeIn);
	BOOL boVirtualFreeOut{VirtualFree(oriout, 0, MEM_RELEASE)};
	static_cast<void>(boVirtualFreeOut);
	assert(boVirtualFreeOut);

	// perform application initialization
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof WNDCLASSEXW;
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = reinterpret_cast<HINSTANCE>(&__ImageBase);
	wcex.hIcon = static_cast<HICON>(LoadImageW(reinterpret_cast<HINSTANCE>(&__ImageBase), MAKEINTRESOURCEW(IDI_WINDOWSPROJECT1), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE));
	assert(wcex.hIcon);// internal resource, no allocation of a new item
	wcex.hCursor = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTCOLOR | LR_SHARED | LR_DEFAULTSIZE));
	assert(wcex.hCursor);// global system resource, no allocation of a new item
	wcex.hbrBackground = reinterpret_cast<HBRUSH>(static_cast<std::uintptr_t>(COLOR_WINDOWFRAME));
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WINDOWSPROJECT1);
	wchar_t const *szkFromResource;
	int lenProjName0endincl{LoadStringW(reinterpret_cast<HINSTANCE>(&__ImageBase), IDC_WINDOWSPROJECT1, reinterpret_cast<LPWSTR>(&szkFromResource), 0)};// get const pointer to resource
	static_cast<void>(lenProjName0endincl);
	assert(lenProjName0endincl);
	assert(szkFromResource);
	wcex.lpszClassName = szkFromResource;
	wcex.hIconSm = static_cast<HICON>(LoadImageW(reinterpret_cast<HINSTANCE>(&__ImageBase), MAKEINTRESOURCEW(IDI_SMALL), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE));
	assert(wcex.hIconSm);// internal resource, no allocation of a new item
	if(ATOM aClass{RegisterClassExW(&wcex)}){
		int lenAppTitle0endincl{LoadStringW(reinterpret_cast<HINSTANCE>(&__ImageBase), IDS_APP_TITLE, reinterpret_cast<LPWSTR>(&szkFromResource), 0)};// get const pointer to resource
		static_cast<void>(lenAppTitle0endincl);
		assert(lenAppTitle0endincl);
		assert(szkFromResource);
		if(HWND hWnd{CreateWindowExW(0, MAKEINTRESOURCEW(aClass), szkFromResource, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, reinterpret_cast<HINSTANCE>(&__ImageBase), nullptr)}){
			BOOL boShowWindow{ShowWindow(hWnd, nCmdShow)};
			static_cast<void>(boShowWindow);// ShowWindow() returns a BOOL that indicates if the window was previously visible
			BOOL boUpdateWindow{UpdateWindow(hWnd)};
			static_cast<void>(boUpdateWindow);
			if(HACCEL hAccelTable{LoadAcceleratorsW(reinterpret_cast<HINSTANCE>(&__ImageBase), MAKEINTRESOURCEW(IDC_WINDOWSPROJECT1))}){
				// main message loop
				MSG msg;
				BOOL boGetMessageW;
				goto GetFirstMessage;
				do{
					if(boGetMessageW == -1){
						MessageBoxW(hWnd, L"GetMessageW() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
						msg.wParam = 0;// failure status for the return statement
						break;
					}
					if(!TranslateAcceleratorW(msg.hwnd, hAccelTable, &msg)){
						BOOL boTranslateMessage{TranslateMessage(&msg)};
						static_cast<void>(boTranslateMessage);
						LRESULT lrDispatchMessageW{DispatchMessageW(&msg)};
						static_cast<void>(lrDispatchMessageW);
					}
GetFirstMessage:
					boGetMessageW = GetMessageW(&msg, nullptr, 0, 0);
				}while(boGetMessageW);
				BOOL boDestroyAcceleratorTable{DestroyAcceleratorTable(hAccelTable)};
				static_cast<void>(boDestroyAcceleratorTable);
				assert(boDestroyAcceleratorTable);
				BOOL boDestroyWindow{DestroyWindow(hWnd)};
				static_cast<void>(boDestroyWindow);// DestroyWindow() returns a BOOL that indicates if the window was destroyed here or earlier by the system
				BOOL boUnregisterClassW{UnregisterClassW(MAKEINTRESOURCEW(aClass), reinterpret_cast<HINSTANCE>(&__ImageBase))};
				static_cast<void>(boUnregisterClassW);
				assert(boUnregisterClassW);
				return{static_cast<int>(msg.wParam)};
			}else MessageBoxW(hWnd, L"LoadAcceleratorsW() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
			// cleanup and return on errors
			BOOL boDestroyWindow{DestroyWindow(hWnd)};
			static_cast<void>(boDestroyWindow);
			assert(boDestroyWindow);
		}else MessageBoxW(nullptr, L"CreateWindowExW() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
		// cleanup and return on errors
		BOOL boUnregisterClassW{UnregisterClassW(MAKEINTRESOURCEW(aClass), reinterpret_cast<HINSTANCE>(&__ImageBase))};
		static_cast<void>(boUnregisterClassW);
		assert(boUnregisterClassW);
	}else MessageBoxW(nullptr, L"RegisterClassExW() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
	return{0};// failure status
}