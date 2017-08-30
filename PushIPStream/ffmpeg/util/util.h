/*
* Copyright (c) 2017 Zqvideo wxg
*/

#pragma once

#include <common/memory.h>

#include <boost/rational.hpp>

#include <array>
#include <vector>
#include <utility>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C"
{
#include <libavutil/avutil.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

struct AVFormatContext;
struct AVPacket;
struct AVRational;
struct AVCodecContext;

namespace caspar {
	namespace ffmpeg {

		typedef std::vector<std::pair<std::string, std::string>> ffmpeg_options;

		// Utils
		double read_fps(AVFormatContext& context, double fail_value);
		boost::rational<int> read_framerate(AVFormatContext& context, const boost::rational<int>& fail_value);
		std::wstring probe_stem(const std::wstring& stem, bool only_video);
		spl::shared_ptr<AVPacket> create_packet();
	}
}