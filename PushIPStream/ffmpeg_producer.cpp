#include "ffmpeg_producer.h"
#include "ffmpeg/util/util.h"
#include "ffmpeg/ffmpeg_producer_internal.h"

#include <common/param.h>
#include <common/env.h>

using namespace caspar;
using namespace ffmpeg;
ffmpeg_producer::ffmpeg_producer()
{
	//ffmpeg::init();不应该在这里，这应该是全局
}
ffmpeg_producer::~ffmpeg_producer()
{
}

std::shared_ptr<packetProducer> ffmpeg_producer::createProducer(const std::vector<std::wstring>& params)
{
	auto file_or_url = params.at(0);

	if (!boost::contains(file_or_url, L"://"))
	{
		// File
		file_or_url = probe_stem(env::media_folder() + L"/" + file_or_url, false);
	}

	if (file_or_url.empty())
		return nullptr;

	constexpr auto uint32_max = std::numeric_limits<uint32_t>::max();

	auto loop = contains_param(L"LOOP", params);

	auto in = get_param(L"SEEK", params, static_cast<uint32_t>(0)); // compatibility
	in = get_param(L"IN", params, in);

	auto out = get_param(L"LENGTH", params, uint32_max);
	if (out < uint32_max - in)
		out += in;
	else
		out = uint32_max;
	out = get_param(L"OUT", params, out);
	ffmpeg_options vid_params;
	auto producer = spl::make_shared<ffmpeg_producer_internal>(
		file_or_url,
		loop,
		in,
		out,
		vid_params
		);

	return producer;
};
