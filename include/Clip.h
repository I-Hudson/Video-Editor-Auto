#pragma once

#include <string>

#include "ResultCodes.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

class Clip
{
public:
	Clip();
	~Clip();

	ResultCodes OpenFile(const std::string& aInFileName);
	ResultCodes GetNextFrame(AVFormatContext* aInContext, AVCodecContext* aOutCodec, AVFrame* aFrame);

private:
	/*
	Input and output files
	*/
	std::string m_inFileName;
	std::string m_outFileName;

	int m_videoSteamIndex;
};