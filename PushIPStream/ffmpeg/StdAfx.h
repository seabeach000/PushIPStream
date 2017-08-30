/*
*/

#pragma once

#if defined _DEBUG && defined _MSC_VER
#include <crtdbg.h>
#endif

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <algorithm>
#include <array>
#include <assert.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/regex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/once.hpp>
#include <boost/thread/thread.hpp>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <deque>
#include <functional>
#include <iostream>
#include <limits>
#include <math.h>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <tbb/atomic.h>
#include <tbb/cache_aligned_allocator.h>
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_invoke.h>
#include <tbb/recursive_mutex.h>
#include <tbb/tbb_thread.h>
#include <unordered_map>
#include <vector>
#include <common/timer.h>

#pragma warning(push, 1)

extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavcodec/avcodec.h>
//2.0   #include <libavfilter/avcodec.h>
//3.0 ��û�����ͷ�ļ�
	#include <libavfilter/avfilter.h>
	#include <libavfilter/avfiltergraph.h>
	#include <libavfilter/buffersink.h>
	#include <libavformat/avformat.h>
	#include <libavutil/avutil.h>
	#include <libavutil/common.h>
	#include <libavutil/cpu.h>
	#include <libavutil/error.h>
	#include <libavutil/imgutils.h>
	#include <libavutil/opt.h>
	#include <libavutil/pixdesc.h>
	#include <libavutil/samplefmt.h>
	#include <libswscale/swscale.h>
}

#pragma warning(pop)
