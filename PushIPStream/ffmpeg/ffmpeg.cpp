/*
*/

#include "StdAfx.h"

#include "ffmpeg.h"


#include <common/log.h>
#include <common/os/general_protection_fault.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/thread/tss.hpp>
#include <boost/bind.hpp>

#include <tbb/recursive_mutex.h>

#if defined(_MSC_VER)
#pragma warning (disable : 4244)
#pragma warning (disable : 4603)
#pragma warning (disable : 4996)
#endif

extern "C"
{
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavfilter/avfilter.h>
#include <libavdevice/avdevice.h>
}

namespace caspar {
	namespace ffmpeg {
		int ffmpeg_lock_callback(void **mutex, enum AVLockOp op)
		{
			if (!mutex)
				return 0;

			auto my_mutex = reinterpret_cast<tbb::recursive_mutex*>(*mutex);

			switch (op)
			{
			case AV_LOCK_CREATE:
			{
				*mutex = new tbb::recursive_mutex();
				break;
			}
			case AV_LOCK_OBTAIN:
			{
				if (my_mutex)
					my_mutex->lock();
				break;
			}
			case AV_LOCK_RELEASE:
			{
				if (my_mutex)
					my_mutex->unlock();
				break;
			}
			case AV_LOCK_DESTROY:
			{
				delete my_mutex;
				*mutex = nullptr;
				break;
			}
			}
			return 0;
		}

		static void sanitize(uint8_t *line)
		{
			while (*line)
			{
				if (*line < 0x08 || (*line > 0x0D && *line < 0x20))
					*line = '?';
				line++;
			}
		}

		void log_callback(void* ptr, int level, const char* fmt, va_list vl)
		{
			static int print_prefix = 1;
			static char prev[1024];
			char line[8192];
			AVClass* avc = ptr ? *(AVClass**)ptr : NULL;
			if (level > AV_LOG_DEBUG)
				return;
			line[0] = 0;

#undef fprintf
			if (print_prefix && avc)
			{
				if (avc->parent_log_context_offset)
				{
					AVClass** parent = *(AVClass***)(((uint8_t*)ptr) + avc->parent_log_context_offset);
					if (parent && *parent)
						std::sprintf(line, "[%s @ %p] ", (*parent)->item_name(parent), parent);
				}
				std::sprintf(line + strlen(line), "[%s @ %p] ", avc->item_name(ptr), ptr);
			}

			std::vsprintf(line + strlen(line), fmt, vl);

			print_prefix = strlen(line) && line[strlen(line) - 1] == '\n';

			//strcpy(prev, line);
			sanitize((uint8_t*)line);

			auto len = strlen(line);
			if (len > 0)
				line[len - 1] = 0;

			try
			{
				if (level == AV_LOG_VERBOSE)
					CASPAR_LOG(debug) << L"[ffmpeg] " << line;
				else if (level == AV_LOG_INFO)
					CASPAR_LOG(info) << L"[ffmpeg] " << line;
				else if (level == AV_LOG_WARNING)
					CASPAR_LOG(warning) << L"[ffmpeg] " << line;
				else if (level == AV_LOG_ERROR)
					CASPAR_LOG(error) << L"[ffmpeg] " << line;
				else if (level == AV_LOG_FATAL)
					CASPAR_LOG(fatal) << L"[ffmpeg] " << line;
				else
					CASPAR_LOG(trace) << L"[ffmpeg] " << line;
			}
			catch (...)
			{
			}
		}

		std::wstring make_version(unsigned int ver)
		{
			std::wstringstream str;
			str << ((ver >> 16) & 0xFF) << L"." << ((ver >> 8) & 0xFF) << L"." << ((ver >> 0) & 0xFF);
			return str.str();
		}

		std::wstring avcodec_version()
		{
			return make_version(::avcodec_version());
		}

		std::wstring avformat_version()
		{
			return make_version(::avformat_version());
		}

		std::wstring avutil_version()
		{
			return make_version(::avutil_version());
		}

		std::wstring avfilter_version()
		{
			return make_version(::avfilter_version());
		}

		std::wstring swscale_version()
		{
			return make_version(::swscale_version());
		}
		bool& get_quiet_logging_for_thread()
		{
			static boost::thread_specific_ptr<bool> quiet_logging_for_thread;

			auto local = quiet_logging_for_thread.get();

			if (!local)
			{
				local = new bool(false);
				quiet_logging_for_thread.reset(local);
			}

			return *local;
		}

		void enable_quiet_logging_for_thread()
		{
			get_quiet_logging_for_thread() = true;
		}

		bool is_logging_quiet_for_thread()
		{
			return get_quiet_logging_for_thread();
		}

		std::shared_ptr<void> temporary_enable_quiet_logging_for_thread(bool enable)
		{
			if (!enable || is_logging_quiet_for_thread())
				return std::shared_ptr<void>();

			get_quiet_logging_for_thread() = true;

			return std::shared_ptr<void>(nullptr, [](void*)
			{
				get_quiet_logging_for_thread() = false; // Only works correctly if destructed in same thread as original caller.
			});
		}

		void log_for_thread(void* ptr, int level, const char* fmt, va_list vl)
		{
			ensure_gpf_handler_installed_for_thread("ffmpeg-thread");

			int min_level = is_logging_quiet_for_thread() ? AV_LOG_DEBUG : AV_LOG_FATAL;

			log_callback(ptr, std::max(level, min_level), fmt, vl);
		}

		void init()
		{
			av_lockmgr_register(ffmpeg_lock_callback);
			av_log_set_callback(log_for_thread);

			avfilter_register_all();
			//fix_yadif_filter_format_query();
			av_register_all();
			avformat_network_init();
			avcodec_register_all();
			avdevice_register_all();
		}

		void uninit()
		{
			avfilter_uninit();
			avformat_network_deinit();
			av_lockmgr_register(nullptr);
		}
	}
}
