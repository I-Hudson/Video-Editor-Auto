#include "VideoCut.h"
#include "ConsoleCommands.h"

#include <stdio.h>
#include <vector>

#include <direct.h>
#define GetCurrentDir _getcwd

VideoCut::VideoCut()
{
}

VideoCut::~VideoCut()
{
}

/*
 Execute all functions needed
*/
void VideoCut::run()
{
	bool success = false;

	if (!openInputFile())
	{
		clean();
		return;
	}

	if (!openOutputFile())
	{
		clean();
		return;
	}
	if (!decEncVideo())
	{
		clean();
		return;
	}

	clean();
}

/*
 All command functions
*/
void VideoCut::helpCommand()
{
	std::string helpMessage = R"(Video Cut Help:
	--input (val) - Change the input file.
	--output (val) - Change the output file.
	--run - Run and export the new video.
	--frameCompare (val) - Change the value of which frames are discarded (Clamped between 0 - 1).
	--frameRate (val) - Change the frame rate of the outputted file.
	--openOutput - Open the output directory.
								)";

	std::cout << helpMessage.c_str() << "\n";
}

void VideoCut::inputCommand()
{
	std::cout << "Please enter the input file:";
	std::string s = ConsoleCommands::Get()->getInput();
	m_inFileName = s.c_str();
}

void VideoCut::outputCommand()
{
	std::cout << "Please enter the output file:";
	std::string s = ConsoleCommands::Get()->getInput();
	m_outFileName = s.c_str();
}

void VideoCut::openOutputFolderCommand()
{
	char path[FILENAME_MAX];
	_getcwd(path, sizeof(path));

	std::string command("explorer ");
	command += std::string(path);

	system(command.c_str());
}

void VideoCut::frameCompareCommand()
{
	std::string helpMessage = R"(Video Cut Help:
	--FrameCompare (val) Change the value of which frames are discarded (Clamped between 0 - 1).
	--InputFile (val) Change the input file.
	--OutputFile (val) Change the output file.
								)";

	std::cout << helpMessage.c_str() << "\n";
}

void VideoCut::frameRateCommand()
{
	std::string helpMessage = "Enter new frame rate: ";
	std::cout << helpMessage.c_str();

	helpMessage += ConsoleCommands::Get()->getInput();

	helpMessage = "Frame rate has been changed to: ";
	std::cout << helpMessage.c_str() << "\n";
}

/*
 Open Input file
*/
bool VideoCut::openInputFile()
{
	// open input file
	if (avformat_open_input(&m_ifmtCtx, m_inFileName.c_str(), nullptr, nullptr) != 0)
	{
		//error message
		return false;
	}

	// find file info
	if (avformat_find_stream_info(m_ifmtCtx, nullptr) < 0)
	{
		//error message
		return false;
	}

	for (unsigned int i = 0; i < m_ifmtCtx->nb_streams; i++)
	{
		if (m_ifmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			m_videoStreamIndex = i;
			break;
		}
	}

	// alloc codec context
	m_decCtx = avcodec_alloc_context3(nullptr);
	if (!m_decCtx)
	{
		//error message
		return false;
	}

	// get a pointer to the codec context for the video stream
	if (avcodec_parameters_to_context(m_decCtx, m_ifmtCtx->streams[m_videoStreamIndex]->codecpar) != 0)
	{
		//error message
		return false;
	}

	// find the decoder for the video stream
	m_decCodec = nullptr;
	m_decCodec = avcodec_find_decoder(m_decCtx->codec_id);
	if (!m_decCodec)
	{
		//error message
		return false;
	}

	// open the decodec
	if (avcodec_open2(m_decCtx, m_decCodec, nullptr) < 0)
	{
		//error message
		return false;
	}

	return true;
}

/*
 Open Output file
*/
bool VideoCut::openOutputFile()
{
	avformat_alloc_output_context2(&m_ofmtCtx, nullptr, nullptr, m_outFileName.c_str());
	if (!m_ofmtCtx)
	{
		//error message
		return false;
	}
	// assign format context
	m_outFmt = m_ofmtCtx->oformat;

	// find and open encoder codec
	AVCodecID id = (AVCodecID)m_outFmt->video_codec;
	m_encCodec = avcodec_find_encoder(id);
	if (!m_encCodec)
	{
		//error message
		return false;
	}

	// add a video stream to output format context
	m_videoStream = avformat_new_stream(m_ofmtCtx, nullptr);
	if (!m_videoStream)
	{
		//error message
		return false;
	}
	m_videoStream->time_base.den = m_frameRate;
	m_videoStream->time_base.num = 1;

	//Set Codec parameters values
	m_codecPar = m_videoStream->codecpar;
	m_codecPar->codec_id = (AVCodecID)m_outFmt->video_codec;
	m_codecPar->codec_type = AVMEDIA_TYPE_VIDEO;
	m_codecPar->bit_rate = 20000 * 1000;
	m_codecPar->width = 1920;
	m_codecPar->height = 1080;
	m_codecPar->format = AV_PIX_FMT_YUV420P;

	// Allocate and set Codec context
	m_encCtx = avcodec_alloc_context3(m_encCodec);
	m_encCtx->time_base.den = m_frameRate;
	m_encCtx->time_base.num = 1;
	if (m_codecPar->codec_id == AV_CODEC_ID_H264)
	{
		av_opt_set(m_encCtx, "preset", "ultrafast", 0);
	}
	if (m_ofmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
	{
		m_ofmtCtx->oformat->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	if (avcodec_parameters_to_context(m_encCtx, m_codecPar) < 0)
	{
		return false;
	}

	// "create" an empty dictionary
	AVDictionary *d = nullptr;          
	/*
	* Set some options for VPX encoding
	*/
	/* Timing $ time ./test
	* cpu realtime  : 6.25s
	* speed 8       : 1.12s
	* both          : 0.52s
	*/
	av_dict_set(&d, "deadline", "realtime", 0); // smaller size
	// av_dict_set(&d, "deadline", "best", 0); // smaller size
	//av_dict_set(&d, "cpu-used", "8", 0); // no effect
	//av_dict_set(&d, "speed", "8", 0); // fast
	//av_dict_set(&d, "speed", "0", 0); // slow

	// open codec
	int err = avcodec_open2(m_encCtx, m_encCodec, &d);
	if (err < 0)
	{
		char buf[256];
		av_strerror(err, buf, 256);
		printf("%s", buf);
		//error messsage
		return false;
	}

	//open output file
	if (avio_open(&m_ofmtCtx->pb, m_outFileName.c_str(), AVIO_FLAG_WRITE) < 0)
	{
		//error message
		return false;
	}

	// write the file header
	if (avformat_write_header(m_ofmtCtx, nullptr) < 0)
	{
		//error message
		return false;
	}

	// Alloc both image and image temp
	m_imageFrame = av_frame_alloc();
	m_imageFrame->format = m_encCtx->pix_fmt;
	m_imageFrame->width = m_encCtx->width;
	m_imageFrame->height = m_encCtx->height;
	av_image_alloc(m_imageFrame->data, m_imageFrame->linesize, m_encCtx->width, m_encCtx->height, m_encCtx->pix_fmt, 32);

	m_imageTempFrame = av_frame_alloc();
	m_imageTempFrame->format = m_encCtx->pix_fmt;
	m_imageTempFrame->width = m_encCtx->width;
	m_imageTempFrame->height = m_encCtx->height;
	av_image_alloc(m_imageTempFrame->data, m_imageTempFrame->linesize, m_encCtx->width, m_encCtx->height, m_encCtx->pix_fmt, 32);

	m_swsCtx = sws_getContext(m_decCtx->width, m_decCtx->height, m_decCtx->pix_fmt,
								m_encCtx->width, m_encCtx->height, m_encCtx->pix_fmt,
								SWS_BILINEAR, nullptr, nullptr, nullptr);
	return true;
}

/*
 Return next frame from input stream
*/
int VideoCut::getVideoFrame()
{
	AVPacket pkt;
	//read the next frame in the input stream
	if (av_read_frame(m_ifmtCtx, &pkt) < 0)
	{
		return -1;
	}

	if (pkt.stream_index == m_videoStreamIndex)
	{
		//send packet to decoder
		if (avcodec_send_packet(m_decCtx, &pkt) < 0)
		{
			return 0;
		}
		//receive frame from decoder
		if (avcodec_receive_frame(m_decCtx, m_imageFrame) < 0)
		{
			return 0;
		}

		return 1;
	}
	return -2;
}

/*
 Decode input video stream, encode output video
*/
bool VideoCut::decEncVideo()
{
	AVPacket packet;

	int i = 0;
	int frameCounter = 0;
	int frameIndex = -1;
	int totalEncodedFrames = 0;
	int totalSkipedFrames = 0;
	while (1)
	{
		frameIndex++;
		av_init_packet(&packet);

		int getFrame = getVideoFrame();
		if (getFrame == -2)
		{
			continue;
		}
		else if (getFrame == -1)
		{
			break;
		}
		else if (getFrame == 0)
		{
			continue;
		}

		if (compareFrames((i > 0) ? true : false, i) == true)
		{
			std::cout << "Frame missed: " << frameIndex << "\n";
			totalSkipedFrames++;
			continue;
		}

		sws_scale(m_swsCtx, m_imageFrame->data, m_imageFrame->linesize,
				0, m_decCtx->height, m_imageTempFrame->data, m_imageTempFrame->linesize);

		m_imageTempFrame->pts = frameCounter++;
		// Encoder input: Supply a raw (uncompressed) frame to the encoder 
		if (0 == avcodec_send_frame(m_encCtx, m_imageTempFrame))
		{
			// Encoder output: Receive the packet from the encoder 
			if (0 == avcodec_receive_packet(m_encCtx, &packet))
			{
				/**
				  * Time stamps! Tricky little bastards. Still figuring it out myself.
				  * DTS - Decoding Time Stamp
				  * PTS - Presentation Time Stamp
				  * DTS and PTS are usually the same unless we have B frames. This is
				  * because B-frames depend on information of frames before and after
				  * them and maybe be decoded in a sequence different from PTS.
				  *
				  * av_rescale_q_rnd(a,bq,cq,AVRounding) supposedly gives a*(bq/cq)
				  *
				  */

				packet.flags |= AV_PKT_FLAG_KEY;

				packet.dts = av_rescale_q_rnd(packet.dts,
					m_encCtx->time_base,
					m_ofmtCtx->streams[m_videoStreamIndex]->time_base,
					(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				packet.pts = av_rescale_q_rnd(packet.pts,
					m_encCtx->time_base,
					m_ofmtCtx->streams[m_videoStreamIndex]->time_base,
					(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				packet.duration = av_rescale_q(1,
					m_encCtx->time_base,
					m_ofmtCtx->streams[m_videoStreamIndex]->time_base);

				// If all goes well, we write the packet to output
				if (0 == av_interleaved_write_frame(m_ofmtCtx, &packet))
				{
					//std::cout << "Frame: " << frameIndex << " encoded\n";
					totalEncodedFrames++;
					av_packet_unref(&packet);
				}
			}
		}
		i++;
	}

	int err = 0;
	while (1)
	{
		av_init_packet(&packet);
		err = avcodec_send_frame(m_encCtx, nullptr);
		if (err == 0) // Packet available
		{
			if (0 == avcodec_receive_packet(m_encCtx, &packet))
			{
				packet.dts = av_rescale_q_rnd(packet.dts,
					m_encCtx->time_base,
					m_ofmtCtx->streams[0]->time_base,
					(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				packet.pts = av_rescale_q_rnd(packet.pts,
					m_encCtx->time_base,
					m_ofmtCtx->streams[0]->time_base,
					(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				packet.duration = av_rescale_q(1,
					m_encCtx->time_base,
					m_ofmtCtx->streams[0]->time_base);

				if (0 == av_interleaved_write_frame(m_ofmtCtx, &packet))
				{
					totalEncodedFrames++;
					av_packet_unref(&packet);
				}
			}
			i++;

		}
		else if (AVERROR_EOF == err) // No more packets
		{
			std::cout << "End of file reached\n";
			break;
		}
		else // How did it even get here?
		{
			std::cout << "Error\n";
		}

		av_packet_unref(&packet);
	}

	m_avgDiff /= i;

	std::cout << "Total frames encoded: " << totalEncodedFrames << "\n";
	std::cout << "Total frames skipped: " << totalSkipedFrames << "\n";
	std::cout << "Min diff: " << m_minDiff << "\n";	
	std::cout << "Avg diff: " << m_avgDiff << "\n";	
	std::cout << "Max diff: " << m_maxDiff << "\n";

	// Remember to clean up
	av_write_trailer(m_ofmtCtx);

	return true;
}

/*
 Compare two frames
*/
bool VideoCut::compareFrames(bool a_compare, int a_frameIndex)
{
	bool result = false;

	AVFrame* tempFrame = av_frame_alloc();
	tempFrame->format = AV_PIX_FMT_RGB24;
	tempFrame->width = m_decCtx->width;
	tempFrame->height = m_decCtx->height;
	av_image_alloc(tempFrame->data, tempFrame->linesize, m_decCtx->width, m_decCtx->height, AV_PIX_FMT_RGB24, 32);
	SwsContext* swsRGB = sws_getContext(1920, 1080, AV_PIX_FMT_YUV420P,
										1920, 1080, AV_PIX_FMT_RGB24, 
										0, nullptr, nullptr, nullptr);

	sws_scale(swsRGB, m_imageFrame->data, m_imageFrame->linesize, 0, m_decCtx->height, tempFrame->data, tempFrame->linesize);

	int rgbWidth = m_decCtx->width * 3;
	int prec = 1;
	int precWidth = prec * 3;
	int numSamples = m_decCtx->width / prec * m_decCtx->height / prec;
	double rD = 0.0, gD = 0.0, bD = 0.0;
	for (int y = 0; y < m_decCtx->height; y += prec)
	{
		for (int x = 0; x < rgbWidth; x += precWidth)
		{
			uint8_t r = tempFrame->data[0][y * tempFrame->linesize[0] + x];
			uint8_t g = tempFrame->data[0][y * tempFrame->linesize[0] + x + 1];
			uint8_t b = tempFrame->data[0][y * tempFrame->linesize[0] + x + 2];

			rD += r / (double)255;
			gD += g / (double)255;
			bD += b / (double)255;
		}
	}

	rD /= numSamples;
	gD /= numSamples;
	bD /= numSamples;

	//std::cout << "Red Channel: " << rD << "\n";
	//std::cout << "Green Channel: " << gD << "\n";
	//std::cout << "Blue Channel: " << bD << "\n";

	if (a_compare)
	{
		double rDiff = abs(rD - m_oldR);
		double gDiff = abs(gD - m_oldG);
		double bDiff = abs(bD - m_oldB);
		double diff = (rDiff + gDiff + bDiff) / 3;

		if (diff < m_minDiff)
		{
			m_minDiff = diff;
		}
		else if (diff > m_maxDiff)
		{
			m_maxDiff = diff;
		}

		m_avgDiff += diff;

		std::cout << "Frame[" << a_frameIndex << "]" << diff << "\n";

		if (diff > m_frameCompare)
		{
			result = true;
		}
	}

	m_oldR = rD;
	m_oldG = gD;
	m_oldB = bD;


	//convert frame to rgb
	//compare data
	//return 

	av_freep(&tempFrame->data[0]);
	av_frame_free(&tempFrame);
	sws_freeContext(swsRGB);

	return result;
}

/*
 Clean up
*/
void VideoCut::clean()
{
	avcodec_close(m_decCtx);
	avcodec_close(m_encCtx);

	if (m_ifmtCtx != nullptr)
	{
		avio_close(m_ifmtCtx->pb);
		avformat_free_context(m_ifmtCtx);
		m_ifmtCtx = nullptr;
	}

	if (m_ofmtCtx != nullptr)
	{
		avio_close(m_ofmtCtx->pb);
		avformat_free_context(m_ofmtCtx);
		m_ofmtCtx = nullptr;
	}

	av_free(m_decCodec);
	av_free(m_encCodec);

	if (m_imageFrame != nullptr)
	{
		av_freep(&m_imageFrame->data[0]);
		av_freep(&m_imageFrame->data[1]);
		av_freep(&m_imageFrame->data[2]);
	}
	if (m_imageTempFrame != nullptr)
	{
		av_freep(&m_imageTempFrame->data[0]);
		//av_frame_free(&m_imageFrame);
		av_frame_free(&m_imageTempFrame);
	}

	if (m_swsCtx != nullptr)
	{
		sws_freeContext(m_swsCtx);
	}
}
