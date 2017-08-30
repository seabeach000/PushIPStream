#include "StdAfx.h"

/*

*/
#include "ffmpeg_error.h"

#include <common/utf.h>
#include <common/log.h>

#pragma warning(disable: 4146)

extern "C"
{
#include <libavutil/error.h>
}

namespace caspar { namespace ffmpeg {

std::string av_error_str(int errn)
{
	char buf[256];
	memset(buf, 0, 256);
	if(av_strerror(errn, buf, 256) < 0)
		return "";
	return std::string(buf);
}

void throw_on_ffmpeg_error(int ret, const char* source, const char* func, const char* local_func, const char* file, int line)
{
	if(ret >= 0)
		return;

	switch(ret)
	{
	case AVERROR_BSF_NOT_FOUND:
		::boost::exception_detail::throw_exception_(
			averror_bsf_not_found()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret))
				<< call_stack_info(caspar::get_call_stack())
				<< context_info(get_context()),
			local_func, log::remove_source_prefix(file), line);
	case AVERROR_DECODER_NOT_FOUND:
		::boost::exception_detail::throw_exception_(
			averror_decoder_not_found()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret))
				<< call_stack_info(caspar::get_call_stack())
				<< context_info(get_context()),
			local_func, log::remove_source_prefix(file), line);
	case AVERROR_DEMUXER_NOT_FOUND:
		::boost::exception_detail::throw_exception_(
			averror_demuxer_not_found()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret))
				<< call_stack_info(caspar::get_call_stack())
				<< context_info(get_context()),
			local_func, log::remove_source_prefix(file), line);
	case AVERROR_ENCODER_NOT_FOUND:
		::boost::exception_detail::throw_exception_(
			averror_encoder_not_found()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret))
				<< call_stack_info(caspar::get_call_stack())
				<< context_info(get_context()),
			local_func, log::remove_source_prefix(file), line);
	case AVERROR_EOF:
		::boost::exception_detail::throw_exception_(
			averror_eof()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret))
				<< call_stack_info(caspar::get_call_stack())
				<< context_info(get_context()),
			local_func, log::remove_source_prefix(file), line);
	case AVERROR_EXIT:
		::boost::exception_detail::throw_exception_(
			averror_exit()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret))
				<< call_stack_info(caspar::get_call_stack())
				<< context_info(get_context()),
			local_func, log::remove_source_prefix(file), line);
	case AVERROR_FILTER_NOT_FOUND:
		::boost::exception_detail::throw_exception_(
			averror_filter_not_found()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret))
				<< call_stack_info(caspar::get_call_stack())
				<< context_info(get_context()),
			local_func, log::remove_source_prefix(file), line);
	case AVERROR_MUXER_NOT_FOUND:
		::boost::exception_detail::throw_exception_(
			averror_muxer_not_found()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret))
				<< call_stack_info(caspar::get_call_stack())
				<< context_info(get_context()),
			local_func, log::remove_source_prefix(file), line);
	case AVERROR_OPTION_NOT_FOUND:
		::boost::exception_detail::throw_exception_(
			averror_option_not_found()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret))
				<< call_stack_info(caspar::get_call_stack())
				<< context_info(get_context()),
			local_func, log::remove_source_prefix(file), line);
	case AVERROR_PATCHWELCOME:
		::boost::exception_detail::throw_exception_(
			averror_patchwelcome()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret))
				<< call_stack_info(caspar::get_call_stack())
				<< context_info(get_context()),
			local_func, log::remove_source_prefix(file), line);
	case AVERROR_PROTOCOL_NOT_FOUND:
		::boost::exception_detail::throw_exception_(
			averror_protocol_not_found()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret))
				<< call_stack_info(caspar::get_call_stack())
				<< context_info(get_context()),
			local_func, log::remove_source_prefix(file), line);
	case AVERROR_STREAM_NOT_FOUND:
		::boost::exception_detail::throw_exception_(
			averror_stream_not_found()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret))
				<< call_stack_info(caspar::get_call_stack())
				<< context_info(get_context()),
			local_func, log::remove_source_prefix(file), line);
	case AVUNERROR(EINVAL):
		::boost::exception_detail::throw_exception_(
			averror_invalid_argument()
				<< msg_info("Invalid FFmpeg argument given")
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret))
				<< call_stack_info(caspar::get_call_stack())
				<< context_info(get_context()),
			local_func, log::remove_source_prefix(file), line);
	default:
		::boost::exception_detail::throw_exception_(
			ffmpeg_error()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret))
				<< call_stack_info(caspar::get_call_stack())
				<< context_info(get_context()),
			local_func, log::remove_source_prefix(file), line);
	}
}

void throw_on_ffmpeg_error(int ret, const std::wstring& source, const char* func, const char* local_func, const char* file, int line)
{
	throw_on_ffmpeg_error(ret, u8(source).c_str(), func, local_func, file, line);
}

}}
