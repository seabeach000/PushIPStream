#pragma once
#include "packetProducer.h"

#include <string>
#include <vector>

class ffmpeg_producer
{
public:
	ffmpeg_producer();
	virtual ~ffmpeg_producer();        
public:
	std::shared_ptr<packetProducer> createProducer(const std::vector<std::wstring>& params);
};


