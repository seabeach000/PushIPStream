/*

*/

#pragma once

#include <common/except.h>

#include <string>

namespace caspar {
	namespace ffmpeg {

		struct ffmpeg_error : virtual caspar_exception {};
		struct ffmpeg_user_error : virtual user_error {};
		struct averror_bsf_not_found : virtual ffmpeg_error {};
		struct averror_decoder_not_found : virtual ffmpeg_user_error {};
		struct averror_demuxer_not_found : virtual ffmpeg_user_error {};
		struct averror_encoder_not_found : virtual ffmpeg_user_error {};
		struct averror_eof : virtual ffmpeg_error {};
		struct averror_exit : virtual ffmpeg_error {};
		struct averror_filter_not_found : virtual ffmpeg_user_error {};
		struct averror_muxer_not_found : virtual ffmpeg_user_error {};
		struct averror_option_not_found : virtual ffmpeg_user_error {};
		struct averror_patchwelcome : virtual ffmpeg_error {};
		struct averror_protocol_not_found : virtual ffmpeg_user_error {};
		struct averror_stream_not_found : virtual ffmpeg_error {};
		struct averror_invalid_argument : virtual ffmpeg_user_error {};

		std::string av_error_str(int errn);

		void throw_on_ffmpeg_error(int ret, const char* source, const char* func, const char* local_func, const char* file, int line);
		void throw_on_ffmpeg_error(int ret, const std::wstring& source, const char* func, const char* local_func, const char* file, int line);


		//#define THROW_ON_ERROR(ret, source, func) throw_on_ffmpeg_error(ret, source, __FUNC__, __FILE__, __LINE__)

#define THROW_ON_ERROR_STR_(call) #call
#define THROW_ON_ERROR_STR(call) THROW_ON_ERROR_STR_(call)

#define THROW_ON_ERROR(ret, func, source) \
		throw_on_ffmpeg_error(ret, source, func, __FUNCTION__, __FILE__, __LINE__);

#define THROW_ON_ERROR2(call, source)										\
	[&]() -> int															\
	{																		\
		int ret = call;														\
		throw_on_ffmpeg_error(ret, source, THROW_ON_ERROR_STR(call), __FUNCTION__, __FILE__, __LINE__);	\
		return ret;															\
	}()

#define LOG_ON_ERROR2(call, source)											\
	[&]() -> int															\
	{					\
		int ret = -1;\
		try{																\
		 ret = call;															\
		throw_on_ffmpeg_error(ret, source, THROW_ON_ERROR_STR(call), __FUNCTION__, __FILE__, __LINE__);	\
		return ret;															\
		}catch(...){CASPAR_LOG_CURRENT_EXCEPTION();}						\
		return ret;															\
	}()

#define FF_RET(ret, func) \
		caspar::ffmpeg::throw_on_ffmpeg_error(ret, L"", func, __FUNCTION__, __FILE__, __LINE__);

#define FF(call)										\
	[&]() -> int														\
	{																		\
		auto ret = call;														\
		caspar::ffmpeg::throw_on_ffmpeg_error(static_cast<int>(ret), L"", THROW_ON_ERROR_STR(call), __FUNCTION__, __FILE__, __LINE__);	\
		return ret;															\
	}()

	}
}
