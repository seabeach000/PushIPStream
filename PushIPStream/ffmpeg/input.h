#pragma once

#include "util/util.h"
#include <common/memory.h>

#include <string>

#include <boost/noncopyable.hpp>
namespace caspar {
	namespace ffmpeg {

		class input :boost::noncopyable
		{
		public:
			explicit input(const std::wstring& url_or_file, bool loop, uint32_t in, uint32_t out, const ffmpeg_options& vid_params);

			bool				try_pop(std::shared_ptr<AVPacket>& packet);
			bool				eof() const;
			void				in(uint32_t value);
			uint32_t			in() const;
			void                out(uint32_t value);
			uint32_t            out() const;
			void                loop(bool value);
			bool                loop() const;

			int                 num_audio_streams() const;

			std::shared_ptr<AVFormatContext>	context();

		private:
			struct impl;
			std::shared_ptr<impl> impl_;
		};
	}
}
