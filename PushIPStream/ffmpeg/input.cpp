#include "input.h"

#include "ffmpeg_error.h"
#include "ffmpeg.h"
#include "util/flv.h"

#include <common/executor.h>
#include <common/param.h>
#include <common/scope_exit.h>

#include <boost/filesystem/path.hpp>

#include <tbb/concurrent_queue.h>
#include <tbb/atomic.h>
#include <tbb/recursive_mutex.h>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C"
{
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#include <libavformat/avformat.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif
using namespace caspar;

static const size_t MAX_BUFFER_COUNT = 100;
static const size_t MAX_BUFFER_COUNT_RT = 3;
static const size_t MIN_BUFFER_COUNT = 50;
static const size_t MAX_BUFFER_SIZE = 64 * 1000000;
namespace caspar {
	namespace ffmpeg {

		struct input::impl : boost::noncopyable
		{
			const spl::shared_ptr<AVFormatContext>						format_context_;
			const int													default_stream_index_ = av_find_default_stream_index(format_context_.get());
			const std::wstring											filename_;
			tbb::atomic<uint32_t>										in_;
			tbb::atomic<uint32_t>										out_;
			tbb::atomic<bool>											loop_;
			uint32_t													file_frame_number_;

			tbb::concurrent_bounded_queue<std::shared_ptr<AVPacket>>	buffer_;
			tbb::atomic<size_t>											buffer_size_;

			executor													executor_;

			boost::posix_time::ptime                                    last_checktime_;
			int															check_timeout_;

			explicit impl(const std::wstring& url_or_file, bool loop, uint32_t in, uint32_t out, const ffmpeg_options& vid_params)
				:format_context_(open_input(url_or_file, vid_params))
				,filename_(url_or_file)
				,executor_(print())
				,check_timeout_(5)
			{
				in_ = in;
				out_ = out;
				if (out_ != std::numeric_limits<uint32_t>::max())
				{
					double fps = read_fps(*format_context_, 0.0);
					int64_t durationLen = static_cast<int64_t>(format_context_->duration*fps / 1000000 + 1);
					if ((durationLen - out_) > 50)
					{
						out_ = out + 50;
					}
					else
					{
						out_ = static_cast<unsigned int>(durationLen);
					}
				}
				loop_ = loop;
				buffer_size_ = 0;
				if (in_ > 0)
					queued_seek(in_);

			}

			bool try_pop(std::shared_ptr<AVPacket>& packet)
			{
				auto result = buffer_.try_pop(packet);

				if (result)
				{
					if (packet)
						buffer_size_ -= packet->size;
					tick();
				}

				return result;
			}

			void queued_seek(const uint32_t target)
			{
				auto stream = format_context_->streams[default_stream_index_];

				auto fps = read_fps(*format_context_, 0.0);

				THROW_ON_ERROR2(avformat_seek_file(
					format_context_.get(),
					default_stream_index_,
					std::numeric_limits<int64_t>::min(),
					static_cast<int64_t>((target / fps * stream->time_base.den) / stream->time_base.num) + stream->start_time,
					std::numeric_limits<int64_t>::max(),
					0), print());

				file_frame_number_ = target;

				auto flush_packet = create_packet();
				flush_packet->data = nullptr;
				flush_packet->size = 0;
				flush_packet->pos = target;

				buffer_.push(flush_packet);
			}

			bool is_eof(int ret)
			{
				if (ret == AVERROR(EIO))
					CASPAR_LOG(trace) << print() << " Received EIO, assuming EOF. ";
				if (ret == AVERROR_EOF)
					CASPAR_LOG(trace) << print() << " Received EOF. ";

				return ret == AVERROR_EOF || ret == AVERROR(EIO) || file_frame_number_ >= out_; // av_read_frame doesn't always correctly return AVERROR_EOF;
			}

			std::ptrdiff_t get_max_buffer_count() const
			{
				return  MAX_BUFFER_COUNT;
			}

			std::ptrdiff_t get_min_buffer_count() const
			{
				return  MIN_BUFFER_COUNT;
			}

			std::wstring print() const
			{
				return L"ffmpeg_input[" + filename_ + L")]";
			}

			bool full() const
			{
				return (buffer_size_ > MAX_BUFFER_SIZE || buffer_.size() > get_max_buffer_count()) && buffer_.size() > get_min_buffer_count();
			}

			void tick()
			{
				if (!executor_.is_running())
					return;

				executor_.begin_invoke([this]
				{
					if (full())
						return;

					try
					{
						auto packet = create_packet();

						auto ret = av_read_frame(format_context_.get(), packet.get()); // packet is only valid until next call of av_read_frame. Use av_dup_packet to extend its life.

						if (is_eof(ret))
						{
							file_frame_number_ = 0;

							if (loop_)
							{
								queued_seek(in_);
								CASPAR_LOG(trace) << print() << " Looping.";
							}
							else
							{
								// Needed by some decoders to decode remaining frames based on last packet.
								auto flush_packet = create_packet();
								flush_packet->data = nullptr;
								flush_packet->size = 0;
								flush_packet->pos = -1;

								buffer_.push(flush_packet);

								executor_.stop();
							}
						}
						else
						{
							THROW_ON_ERROR(ret, "av_read_frame", print());

							if (packet->stream_index == default_stream_index_)
								++file_frame_number_;

							THROW_ON_ERROR2(av_dup_packet(packet.get()), print());

							// Make sure that the packet is correctly deallocated even if size and data is modified during decoding.
							auto size = packet->size;
							auto data = packet->data;

							packet = spl::shared_ptr<AVPacket>(packet.get(), [packet, size, data](AVPacket*)
							{
								packet->size = size;
								packet->data = data;
							});

							buffer_.try_push(packet);
							buffer_size_ += packet->size;
						}

						tick();
					}
					catch (...)
					{
						executor_.stop();
					}
				});
            }

			spl::shared_ptr<AVFormatContext> open_input(const std::wstring& url_or_file, const ffmpeg_options& vid_params)
			{
				AVDictionary* format_options = nullptr;

				CASPAR_SCOPE_EXIT
				{
					if (format_options)
					av_dict_free(&format_options);
				};

				for (auto& option : vid_params)
					av_dict_set(&format_options, option.first.c_str(), option.second.c_str(), 0);

				auto resource_name = std::wstring();
				auto parts = caspar::protocol_split(url_or_file);
				auto protocol = parts.at(0);
				auto path = parts.at(1);
				AVInputFormat* input_format = nullptr;

				if (protocol.empty())
					resource_name = path;
				else
					resource_name = protocol + L"://" + path;

				AVFormatContext* weak_context = nullptr;
				weak_context = avformat_alloc_context();
				weak_context->probesize = weak_context->probesize * 4;
				weak_context->interrupt_callback.opaque = this;
				weak_context->interrupt_callback.callback = this->check_interrupt;
				setCurrentCheckTime(5);
				try
				{
					THROW_ON_ERROR2(avformat_open_input(&weak_context, u8(resource_name).c_str(), input_format, &format_options), resource_name);
				}
				catch (...)
				{
				}

				spl::shared_ptr<AVFormatContext> context(weak_context, [](AVFormatContext* ptr)
				{
					avformat_close_input(&ptr);
				});

				if (format_options)
				{
					std::string unsupported_tokens = "";
					AVDictionaryEntry *t = NULL;
					while ((t = av_dict_get(format_options, "", t, AV_DICT_IGNORE_SUFFIX)) != nullptr)
					{
						if (!unsupported_tokens.empty())
							unsupported_tokens += ", ";
						unsupported_tokens += t->key;
					}
					CASPAR_THROW_EXCEPTION(user_error() << msg_info(unsupported_tokens));
				}

				setCurrentCheckTime(5);
				THROW_ON_ERROR2(avformat_find_stream_info(context.get(), nullptr), resource_name);
				fix_meta_data(*context);
				return context;
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

			static int check_interrupt(void* ctx)
			{
#ifdef  _DEBUG
				return false;
#endif
				caspar::ffmpeg::input::impl* input0 = (caspar::ffmpeg::input::impl*)ctx;
				boost::posix_time::millisec_posix_time_system_config::time_duration_type time_elapse;
				time_elapse = boost::posix_time::microsec_clock::universal_time() - input0->last_checktime_;
				if (time_elapse.total_seconds() > input0->check_timeout_)
				{
					return true;
				}
				return false;
			}

			void setCurrentCheckTime(int timeout)
			{
				last_checktime_ = boost::posix_time::microsec_clock::universal_time();
				check_timeout_ = timeout;
			}

			int num_audio_streams() const
			{
				return 0; // TODO
			}
		};

		input::input(const std::wstring& url_or_file, bool loop, uint32_t in, uint32_t out, const ffmpeg_options& vid_params)
			:impl_(new impl(url_or_file, loop, in, out, vid_params))
		{
		}

		bool input::eof() const
		{
			return !impl_->executor_.is_running();
		}
		
		bool input::try_pop(std::shared_ptr<AVPacket>& packet)
		{
			return impl_->try_pop(packet);
		}

		void input::in(uint32_t value)
		{
			impl_->in_ = value;
		}

		uint32_t input::in() const
		{
			return impl_->in_;
		}

		void input::out(uint32_t value)
		{
			impl_->out_ = value;
		}

		uint32_t input::out() const
		{
			return impl_->out_;
		}

		void input::loop(bool value)
		{
			impl_->loop_ = value;
		}

		bool input::loop() const
		{
			return impl_->loop_;
		}

		int input::num_audio_streams() const
		{
			return impl_->num_audio_streams();
		}

		std::shared_ptr<AVFormatContext> input::context()
		{
			return impl_->format_context_;
		}

	}
}
