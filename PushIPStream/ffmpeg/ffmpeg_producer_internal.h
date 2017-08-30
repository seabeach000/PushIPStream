#pragma once
#include "../packetProducer.h"
#include "util/util.h"
#include "input.h"
#include "packetsQueue.h"

#include <boost/thread.hpp>

#include <string>
#include <vector>
namespace caspar
{
	namespace ffmpeg {

		class ffmpeg_producer_internal :
			public packetProducer
		{
		public:
			ffmpeg_producer_internal(const std::wstring& url_or_file, bool loop, uint32_t in, uint32_t out, const ffmpeg_options& vid_params);
			virtual ~ffmpeg_producer_internal();

		public:
			const std::wstring									filename_;
			input												input_;
			std::unique_ptr<packetsQueue>                       video_packets_;
			std::vector<std::unique_ptr<packetsQueue>>			audio_packets_;
			std::vector<std::unique_ptr<packetsQueue>>			subti_packets_;

			boost::thread										thread_;
			tbb::atomic<bool>									is_running_;
			int64_t                                             current_video_pts_;
			int64_t												current_audio_pts_;
			int64_t												current_subti_pts_;

			int32_t												num_audios_;
			int32_t                                             num_subtis_;
			int													audio_index_;
			int													subti_index_;
		public:
			bool receive_v(std::shared_ptr<AVPacket>& packet);
			bool receive_a(std::shared_ptr<AVPacket>& packet,int& stream_index);
			bool receive_s(std::shared_ptr<AVPacket>& packet,int& stream_index);
		private:
			void run();
		};
	}
}
