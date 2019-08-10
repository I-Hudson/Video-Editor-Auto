#pragma once
#include <algorithm>
#include <iostream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavcodec/avfft.h>

#include <libavdevice/avdevice.h>

#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

#include <libavformat/avformat.h>
#include <libavformat/avio.h>

// libav resample

#include <libavutil/opt.h>
#include <libavutil/common.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/file.h>

// lib swresample

#include <libswscale/swscale.h>
}

class VideoCut
{
public:
	VideoCut();
	~VideoCut();

	void run();


private:
	/*
	 Open Input and Output file
	*/
	bool openInputFile();
	bool openOutputFile();

	/*
	 Return next frame from input stream
	*/
	int getVideoFrame();
	/*
	 Decode input video stream, encode output video
	*/
	bool decEncVideo();

	/*
	 Compare two frames
	*/
	bool compareFrames(bool a_compare, int a_frameIndex);

	/*
	 Clean up
	*/
	void clean();

/*
 All command functions
*/
public:
	void helpCommand();
	void inputCommand();
	void outputCommand();

	void openOutputFolderCommand();

	void frameCompareCommand();
	void frameRateCommand();

private:
	/*
	 Input and output files 
	*/
	std::string m_inFileName;
	std::string m_outFileName;

	// compare frames
	double m_oldR = 0;
	double m_oldG = 0;
	double m_oldB = 0;
	double m_minDiff = 1;
	double m_avgDiff = 0;
	double m_maxDiff = 0;


	//Decode 
	AVFormatContext* m_ifmtCtx;
	AVCodec* m_decCodec;
	AVCodecContext* m_decCtx;
	int m_videoStreamIndex;

	//Encode
	AVFormatContext* m_ofmtCtx;
	AVOutputFormat* m_outFmt;
	
	AVCodecParameters* m_codecPar;
	AVCodecContext* m_encCtx;
	AVCodec* m_encCodec;

	AVStream* m_videoStream;
	AVFrame* m_imageFrame;
	AVFrame* m_imageTempFrame;

	SwsContext* m_swsCtx;

	/*
	 Output frame rate and frame compare threshold 
	*/
	int m_frameRate = 30;
	float m_frameCompare = 0.03f;

};