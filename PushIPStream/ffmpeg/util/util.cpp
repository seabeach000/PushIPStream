/*

*/



#include "util.h"
#include "flv.h"

#include "../ffmpeg_error.h"

#include <common/os/filesystem.h>


#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C"
{
	#include <libswscale/swscale.h>
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar {
	namespace ffmpeg {
		

		bool is_valid_file(const std::wstring& filename, bool only_video)
		{
			static const auto valid_exts = {
				L".mov",
				L".mp4",
				L".flv",
				L".mpg",
				L".mkv",
				L".ts"
			};

			auto ext = boost::to_lower_copy(boost::filesystem::path(filename).extension().wstring());

			if (std::find(valid_exts.begin(), valid_exts.end(), ext) != valid_exts.end())
				return true;

			return false;
		}

		std::wstring probe_stem(const std::wstring& stem, bool only_video)
		{
			auto stem2 = boost::filesystem::path(stem);
			auto parent = find_case_insensitive(stem2.parent_path().wstring());

			if (!parent)
				return L"";

			auto dir = boost::filesystem::path(*parent);

			for (auto it = boost::filesystem::directory_iterator(dir); it != boost::filesystem::directory_iterator(); ++it)
			{
				if (boost::iequals(it->path().stem().wstring(), stem2.filename().wstring()) && is_valid_file(it->path().wstring(), only_video))
					return it->path().wstring();
			}
			return L"";
		}

		bool is_sane_fps(AVRational time_base)
		{
			double fps = static_cast<double>(time_base.den) / static_cast<double>(time_base.num);
			return fps > 20.0 && fps < 65.0;
		}

		AVRational fix_time_base(AVRational time_base)
		{
			if (time_base.num == 1)
				time_base.num = static_cast<int>(std::pow(10.0, static_cast<int>(std::log10(static_cast<float>(time_base.den))) - 1));

			if (!is_sane_fps(time_base))
			{
				auto tmp = time_base;
				tmp.den /= 2;
				if (is_sane_fps(tmp))
					time_base = tmp;
			}

			return time_base;
		}

		double read_fps(AVFormatContext& context, double fail_value)
		{
			auto framerate = read_framerate(context, boost::rational<int>(static_cast<int>(fail_value * 1000000.0), 1000000));

			return static_cast<double>(framerate.numerator()) / static_cast<double>(framerate.denominator());
		}

		boost::rational<int> read_framerate(AVFormatContext& context, const boost::rational<int>& fail_value)
		{
			auto video_index = av_find_best_stream(&context, AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);
			auto audio_index = av_find_best_stream(&context, AVMEDIA_TYPE_AUDIO, -1, -1, 0, 0);

			if (video_index > -1)
			{
				const auto video_context = context.streams[video_index]->codec;
				const auto video_stream = context.streams[video_index];

				auto frame_rate_time_base = video_stream->avg_frame_rate;
				std::swap(frame_rate_time_base.num, frame_rate_time_base.den);

				if (is_sane_fps(frame_rate_time_base))
				{
					return boost::rational<int>(frame_rate_time_base.den, frame_rate_time_base.num);
				}

				AVRational time_base = video_context->time_base;

				if (boost::filesystem::path(context.filename).extension().string() == ".flv")
				{
					try
					{
						auto meta = read_flv_meta_info(context.filename);
						return boost::rational<int>(static_cast<int>(boost::lexical_cast<double>(meta["framerate"]) * 1000000.0), 1000000);
					}
					catch (...)
					{
						return fail_value;
					}
				}
				else
				{
					time_base.num *= video_context->ticks_per_frame;

					if (!is_sane_fps(time_base))
					{
						time_base = fix_time_base(time_base);

						if (!is_sane_fps(time_base) && audio_index > -1)
						{
							auto& audio_context = *context.streams[audio_index]->codec;
							auto& audio_stream = *context.streams[audio_index];

							double duration_sec = audio_stream.duration / static_cast<double>(audio_context.sample_rate);

							time_base.num = static_cast<int>(duration_sec*100000.0);
							time_base.den = static_cast<int>(video_stream->nb_frames * 100000);
						}
					}
				}

				boost::rational<int> fps(time_base.den, time_base.num);

				return fps;
			}

			return fail_value;
		}

		void fix_meta_data(AVFormatContext& context)
		{
			auto video_index = av_find_best_stream(&context, AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);

			if (video_index > -1)
			{
				auto video_stream = context.streams[video_index];
				auto video_context = context.streams[video_index]->codec;

				if (boost::filesystem::path(context.filename).extension().string() == ".flv")
				{
					try
					{
						auto meta = read_flv_meta_info(context.filename);
						double fps = boost::lexical_cast<double>(meta["framerate"]);
						video_stream->nb_frames = static_cast<int64_t>(boost::lexical_cast<double>(meta["duration"])*fps);
					}
					catch (...) {}
				}
				else
				{
					auto stream_time = video_stream->time_base;
					auto duration = video_stream->duration;
					auto codec_time = video_context->time_base;
					auto ticks = video_context->ticks_per_frame;

					if (video_stream->nb_frames == 0)
						video_stream->nb_frames = (duration*stream_time.num*codec_time.den) / (stream_time.den*codec_time.num*ticks);
				}
			}
		}

		spl::shared_ptr<AVPacket> create_packet()
		{
			spl::shared_ptr<AVPacket> packet(new AVPacket(), [](AVPacket* p)
			{
				av_free_packet(p);
				delete p;
			});

			av_init_packet(packet.get());
			return packet;
		}

		spl::shared_ptr<AVFormatContext> open_input(const std::wstring& filename)
		{
			AVFormatContext* weak_context = nullptr;
			THROW_ON_ERROR2(avformat_open_input(&weak_context, u8(filename).c_str(), nullptr, nullptr), filename);
			spl::shared_ptr<AVFormatContext> context(weak_context, [](AVFormatContext* p)
			{
				avformat_close_input(&p);
			});
			THROW_ON_ERROR2(avformat_find_stream_info(weak_context, nullptr), filename);
			fix_meta_data(*context);
			return context;
		}

	}
}
