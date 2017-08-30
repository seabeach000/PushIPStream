#include "ffmpeg_producer_internal.h"

#include <common/log.h>
static const size_t PKT_BUFFER_COUNT = 50;

namespace caspar
{
	namespace ffmpeg {

		ffmpeg_producer_internal::ffmpeg_producer_internal(const std::wstring& url_or_file, bool loop, uint32_t in, uint32_t out, const ffmpeg_options& vid_params)
			:filename_(url_or_file)
			, input_(url_or_file, loop, in, out, vid_params)
			, current_video_pts_(0)
			, current_audio_pts_(0)
			, current_subti_pts_(0)
			, num_audios_(0)
			, num_subtis_(0)
			, audio_index_(0)
			, subti_index_(0)
		{
			for (unsigned stream_index = 0; stream_index < input_.context()->nb_streams; ++stream_index)
			{
				auto stream = input_.context()->streams[stream_index];
				if (stream->codec->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO)
					video_packets_.reset(new packetsQueue(stream_index));
				else if (stream->codec->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO)
					audio_packets_.push_back(std::unique_ptr<packetsQueue>(new packetsQueue(stream_index)));
				else if ((stream->codec->codec_type == AVMediaType::AVMEDIA_TYPE_SUBTITLE))
					subti_packets_.push_back(std::unique_ptr<packetsQueue>(new packetsQueue(stream_index)));
			}

			num_audios_ = audio_packets_.size();
			num_subtis_ = subti_packets_.size();
			if (num_audios_ < 1)
				CASPAR_LOG(warning) << L"No Audio stream Found!";

			is_running_ = true;
			thread_ = boost::thread([this] {run(); });

		}

		ffmpeg_producer_internal::~ffmpeg_producer_internal()
		{
			if (is_running_)
				is_running_ = false;
			thread_.join();
		}

		void ffmpeg_producer_internal::run()
		{
			while (is_running_)
			{
				if (video_packets_->getSize() > PKT_BUFFER_COUNT) //可以对packets的个数做出限制
				{
					boost::this_thread::sleep_for(boost::chrono::milliseconds(20));
					continue;
				}

				std::shared_ptr<AVPacket> pkt;
				input_.try_pop(pkt);
				//数据拿完后，这个线程实际上是可以退出的
				video_packets_->push(pkt);

				if (audio_packets_.size() > 0)
				{
					for (int i = 0; i < audio_packets_.size(); i++)
					{
						audio_packets_[i]->push(pkt);
					}
				}

				if (subti_packets_.size() > 0)
				{
					for (int i = 0; i < subti_packets_.size(); i++)
					{
						subti_packets_[i]->push(pkt);
					}
				}

				//理论上来说它们在队列中的时间戳应该都是对齐的。
				//如果开头本来不对齐，我们需要丢弃,保证更新后的时间戳单调递增

			}
		}
		bool ffmpeg_producer_internal::receive_v(std::shared_ptr<AVPacket>& packet)
		{

			if (video_packets_->getSize() > 0)
			{
				packet = video_packets_->poll();
				//current_video_pts_ = packet->pts;
			}
			else
			{
				return false;
			}

			return true;
		}

		bool ffmpeg_producer_internal::receive_a(std::shared_ptr<AVPacket>& packet, int& stream_index)
		{
			if (num_audios_ > 0)
			{
				int i = audio_index_%num_audios_;

				if (audio_packets_[i]->getSize() > 0)
				{
					packet = audio_packets_[i]->poll();
					stream_index = i;
					audio_index_++;
				}
				else
				{
					//没有拿到数据，索引不进行更新
					return false;
				}
			}

			return true;
		}

		bool ffmpeg_producer_internal::receive_s(std::shared_ptr<AVPacket>& packet, int& stream_index)
		{
			if (num_subtis_ > 0)
			{
				int i = subti_index_%num_subtis_;

				if (subti_packets_[i]->getSize() > 0)
				{
					packet = subti_packets_[i]->poll();
					stream_index = i;
					subti_index_++;
				}
				else
				{
					//没有拿到数据，索引不进行更新
					return false;
				}
			}

			return true;
		}
	}
}

