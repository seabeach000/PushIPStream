#pragma once
#include <memory>
struct AVPacket;

class packetProducer
{
public:
	virtual bool receive_v(std::shared_ptr<AVPacket>& packet) = 0;
	virtual bool receive_a(std::shared_ptr<AVPacket>& packet, int& stream_index) = 0;
	virtual bool receive_s(std::shared_ptr<AVPacket>& packet, int& stream_index) = 0;
};
