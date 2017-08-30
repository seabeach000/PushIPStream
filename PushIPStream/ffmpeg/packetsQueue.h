#pragma once

#include "./StdAfx.h"

#include <common/memory.h>

#include <queue>

#include <boost/noncopyable.hpp>

struct AVFormatContext;

class packetsQueue
{
public:
	packetsQueue(int stream_index);

	bool ready() const;
	void push(const std::shared_ptr<AVPacket>& packet);
	std::shared_ptr<AVPacket> poll();
	int  getIndex();
	int  getSize();
private:
	struct implementation;
	caspar::spl::shared_ptr<implementation> impl_;
};

