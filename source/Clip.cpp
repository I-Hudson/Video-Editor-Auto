#include "Clip.h"

Clip::Clip()
{
}

Clip::~Clip()
{
}

ResultCodes Clip::GetNextFrame(AVFormatContext* aInContext, AVCodecContext* aOutCodec, AVFrame* aFrame)
{
	AVPacket pkt;
	//read the next frame in the input stream
	if (av_read_frame(aInContext, &pkt) < 0)
	{
		return ResultCodes::ERROR_READ_FRAME_UNSUCCESSFUL;
	}

	if (pkt.stream_index == m_videoSteamIndex)
	{
		//send packet to decoder
		if (avcodec_send_packet(aOutCodec, &pkt) < 0)
		{
			return ResultCodes::SEND_PACKET_TO_DECODER;
		}
		//receive frame from decoder
		if (avcodec_receive_frame(aOutCodec, aFrame))
		{
			return ResultCodes::RECEVIE_FRAME_FROM_DECODER;
		}
		return ResultCodes::FRAME_RECEVIED;
	}
	return ResultCodes::FRAME_NOT_READY;
}
