#include "packetsQueue.h"

#include "util/util.h"

using namespace caspar;

struct packetsQueue::implementation :boost::noncopyable
{
	int													index_;
	std::queue<spl::shared_ptr<AVPacket>>				packets_;
public:
	explicit implementation(int stream_index)
	:index_(stream_index)
	{

	}

	void push(const std::shared_ptr<AVPacket>& packet)
	{
		if (!packet)
			return;
		if (packet->stream_index == index_)
			packets_.push(spl::make_shared_ptr(packet));
	}

	std::shared_ptr<AVPacket> poll()
	{
		if (packets_.empty())
			return nullptr;
		auto packet = packets_.front();
		packets_.pop();
		return packet;
	}

	bool ready() const
	{
		return packets_.size() > 10;
	}

	int getIndex()
	{
		return index_;
	}

	int getSize()
	{
		return packets_.size();
	}
};


packetsQueue::packetsQueue(int stream_index)
	:impl_(new implementation(stream_index))
{
}

bool packetsQueue::ready() const
{
	return impl_->ready();
}

void packetsQueue::push(const std::shared_ptr<AVPacket>& packet)
{
	impl_->push(packet);
}

std::shared_ptr<AVPacket> packetsQueue::poll()
{
	return impl_->poll();
}

int packetsQueue::getIndex()
{
	return impl_->getIndex();
}

int packetsQueue::getSize()
{
	return impl_->getSize();
}

