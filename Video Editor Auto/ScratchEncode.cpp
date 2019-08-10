#include "ScratchEncode.h"

SratchEncode::SratchEncode()
{
}

SratchEncode::~SratchEncode()
{
}

/*
	VIDEO ENCODES BUT NOT SEQ RIGHT NEED TO LOOK AT THAT
*/

int SratchEncode::encodeVideo(AVFormatContext* a_fmtCtx, AVCodecContext* a_codec, AVFrame* a_frame, int a_videoStream, const char * a_fileName, std::vector<int>* a_framesToRemove)
{
	remux(a_fmtCtx, a_codec, a_frame, a_videoStream, a_fileName, a_framesToRemove);
	//mux(a_fmtCtx, a_codec, a_frame, a_videoStream, a_fileName, a_framesToRemove);

	return 0;
}

int SratchEncode::readtEncode()
{
	/* allocate the output media context */
	avformat_alloc_output_context2(&m_ofmtCtx, nullptr, nullptr, "NewFile.mp4");
	if (!m_ofmtCtx)
	{
		printf("Error\n");
		exit(1);
	}

	m_ofmt = m_ofmtCtx->oformat;

	/* Add the audio and video streams using the default format codecs
	 * and initialize the codecs. */
	if (m_ofmt->video_codec != AV_CODEC_ID_NONE)
	{
		addStream(&m_videoSt, m_ofmtCtx, m_videoCodec, m_ofmt->video_codec);
		m_haveVideo = 1;
		m_encodeVideo = 1;
	}

	if (m_haveVideo)
	{
		openVideo(m_ofmtCtx, m_videoCodec, &m_videoSt);
	}

	av_dump_format(m_ofmtCtx, 0, "NewFile.mp4", 1);

	/* open the output file, if needed */
	if (!(m_ofmt->flags & AVFMT_NOFILE)) {
		m_ret = avio_open(&m_ofmtCtx->pb, "NewFile.mp4", AVIO_FLAG_WRITE);
		if (m_ret < 0) {
			fprintf(stderr, "Could not open\n");
			return 1;
		}
	}

	/* Write the stream header, if any. */
	m_ret = avformat_write_header(m_ofmtCtx, nullptr);
	if (m_ret < 0) {
		fprintf(stderr, "Error occurred when opening output file\n");
		return 1;
	}

	return 1;
}

/* Prepare a dummy image. */
static void fill_yuv_image(AVFrame *pict, int frame_index,
	int width, int height)
{
	int x, y, i;

	i = frame_index;

	/* Y */
	for (y = 0; y < height; y++)
		for (x = 0; x < width; x++)
			pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;

	/* Cb and Cr */
	for (y = 0; y < height / 2; y++) {
		for (x = 0; x < width / 2; x++) {
			pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
			pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
		}
	}
}

static void fillImage(AVFrame *pict, AVFrame *a_newImage, int frame_index,
	int width, int height)
{
	int x, y, i;

	i = frame_index;

	/* Y */
	for (y = 0; y < height; y++)
		for (x = 0; x < width; x++)
			int yy = pict->data[0][y * pict->linesize[0] + x];
	int uu = pict->data[1][y * pict->linesize[1] + x];
	int vv = pict->data[2][y * pict->linesize[2] + x];

	pict->data[0][y * pict->linesize[0] + x] = a_newImage->data[0][y * a_newImage->linesize[0] + x];

}

void SratchEncode::encodeFrame(AVFrame* a_frame)
{
	AVCodecContext* c = m_videoSt.enc;
	if (av_compare_ts(m_videoSt.next_pts, c->time_base, 10, AVRational{ 1,1 }) >= 0)
	{
		return;
	}

	m_ret = av_frame_is_writable(m_videoSt.frame);
	if (m_ret < 0)
	{
		char buf[256];
		av_strerror(m_ret, buf, 256);
		fprintf(stderr, "Error encoding video frame: %s\n", buf);
		exit(1);
	}

	if (1)//c->pix_fmt != AV_PIX_FMT_YUV420P)
	{
		if (!m_videoSt.sws_ctx)
		{
			m_videoSt.sws_ctx = sws_getContext(c->width, c->height,
				AV_PIX_FMT_RGB24,
				c->width, c->height,
				AV_PIX_FMT_YUV420P,
				SWS_BICUBIC, 0, 0, 0);

		}
		sws_scale(m_videoSt.sws_ctx, (const uint8_t* const*)a_frame->data, a_frame->linesize, 0, 1080, m_videoSt.frame->data, m_videoSt.frame->linesize);
	}
	else
	{
		//sws_scale(m_videoSt.sws_ctx, (const uint8_t* const*)a_frame->data, a_frame->linesize, 0, c->height, m_videoSt.frame->data, m_videoSt.frame->linesize);
		//fill_yuv_image(m_videoSt.frame, m_videoSt.next_pts, c->width, c->height);
		fillImage(m_videoSt.frame, a_frame, m_videoSt.next_pts, c->width, c->height);
	}

	m_videoSt.frame->pts = m_videoSt.next_pts++;


	m_ret = avcodec_send_frame(c, m_videoSt.frame);
	if (m_ret < 0)
	{
		fprintf(stderr, "Error encoding video frame: %s\n", "");
		exit(1);
	}

	AVPacket* packet = av_packet_alloc();
	packet->data = nullptr;
	packet->size = 0;

	m_ret = avcodec_receive_packet(c, packet);
	packet->flags |= AV_PKT_FLAG_KEY;
	if (m_ret < 0)
	{
		char buf[256];
		av_strerror(m_ret, buf, 256);
		fprintf(stderr, "Error encoding video frame: %s\n", buf);
		return;
	}

	m_ret = writeFrame(m_ofmtCtx, &c->time_base, m_videoSt.st, packet);

	av_packet_free(&packet);
}

int SratchEncode::closeEncode()
{
	//DELAYED FRAMES
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;

	for (;;) {
		avcodec_send_frame(m_videoSt.enc, NULL);
		if (avcodec_receive_packet(m_videoSt.enc, &pkt) == 0) {
			av_interleaved_write_frame(m_ofmtCtx, &pkt);
			av_packet_unref(&pkt);
		}
		else {
			break;
		}
	}

	av_write_trailer(m_ofmtCtx);

	/* Close each codec. */
	if (m_haveVideo)
		closeStream(m_ofmtCtx, &m_videoSt);

	if (!(m_ofmt->flags & AVFMT_NOFILE))
		/* Close the output file. */
		avio_closep(&m_ofmtCtx->pb);
	/* free the stream */
	avformat_free_context(m_ofmtCtx);

	return 1;
}

int SratchEncode::remux(AVFormatContext * a_fmtCtx, AVCodecContext * a_codec, AVFrame * a_frame, int a_videoStream, const char * a_fileName, std::vector<int>* a_framesToRemove)
{
	OutputStream videoSt = { 0 };
	AVOutputFormat* ofmt = nullptr;
	AVFormatContext* ofmtCtx = nullptr;
	AVCodec* videoCodec = nullptr;
	int ret;
	int haveVideo = 0;
	int encodeVideo = 0;
	AVPacket pkt;
	int streamIndex = 0;
	int* streamMapping = nullptr;
	int streamMappingSize = 0;

	/* allocate the output media context */
	avformat_alloc_output_context2(&ofmtCtx, nullptr, nullptr, a_fileName);
	if (!ofmtCtx)
	{
		//print error
		return -1;
	}

	streamMappingSize = a_fmtCtx->nb_streams;
	streamMapping = (int*)av_mallocz_array(streamMappingSize, sizeof(*streamMapping));
	if (!streamMapping)
	{
		printf("Stream mapping failed!!!\n");
		exit(1);
	}

	ofmt = ofmtCtx->oformat;

	for (int i = 0; i < a_fmtCtx->nb_streams; i++)
	{
		AVStream* oStream;
		AVStream* iStream = a_fmtCtx->streams[i];
		AVCodecParameters* iCodecPar = iStream->codecpar;

		if (iCodecPar->codec_type != AVMEDIA_TYPE_VIDEO)
		{
			streamMapping[i] = -1;
			continue;
		}

		oStream = avformat_new_stream(ofmtCtx, nullptr);
		if (!oStream)
		{
			printf("Failed alloc output stream\n");
			exit(1);
		}

		ret = avcodec_parameters_copy(oStream->codecpar, iCodecPar);
		if (ret < 0)
		{
			printf("Failed to copy codecpar\n");
			oStream->codecpar->codec_tag = 0;
			exit(1);
		}

	}

	av_dump_format(ofmtCtx, 0, a_fileName, 1);

	if (!(ofmt->flags & AVFMT_NOFILE))
	{
		ret = avio_open(&ofmtCtx->pb, a_fileName, AVIO_FLAG_WRITE);
		if (ret < 0) {
			fprintf(stderr, "Could not open output file '%s'", a_fileName);
			exit(1);
		}
	}

	ret = avformat_write_header(ofmtCtx, nullptr);
	if (ret < 0) {
		fprintf(stderr, "Error occurred when opening output file\n");
	}

	int i = 0;
	int index = 0;
	while (1)
	{
		AVStream* iStream, *oStream;

		ret = av_read_frame(a_fmtCtx, &pkt);
		if (ret < 0)
		{
			break;
		}

		iStream = a_fmtCtx->streams[pkt.stream_index];
		if (pkt.stream_index >= streamMappingSize ||
			streamMapping[pkt.stream_index] < 0)
		{
			av_packet_unref(&pkt);
			continue;
		}

		pkt.stream_index = streamMapping[pkt.stream_index];
		oStream = ofmtCtx->streams[pkt.stream_index];


		AVRounding round = AV_ROUND_NEAR_INF;
		round = (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);

		if (index % 2 == 0)
		{
			/* copy packet */
			pkt.pts = av_rescale_q_rnd(pkt.pts, iStream->time_base, oStream->time_base, round);
			pkt.dts = av_rescale_q_rnd(pkt.dts, iStream->time_base, oStream->time_base, round);
			pkt.duration = av_rescale_q(pkt.duration, iStream->time_base, oStream->time_base);
			pkt.pos = -1;


			ret = av_interleaved_write_frame(ofmtCtx, &pkt);
			if (ret < 0) {
				fprintf(stderr, "Error muxing packet\n");
				break;
			}
			i++;
		}
		av_packet_unref(&pkt);

		index++;
	}

	av_write_trailer(ofmtCtx);

	/* close output */
	if (ofmtCtx && !(ofmt->flags & AVFMT_NOFILE))
		avio_closep(&ofmtCtx->pb);
	avformat_free_context(ofmtCtx);

	av_freep(&streamMapping);

	if (ret < 0 && ret != AVERROR_EOF) {
		fprintf(stderr, "Error occurred: %s\n");
		return 1;
	}

	return 0;
}

int SratchEncode::mux(AVFormatContext * a_fmtCtx, AVCodecContext * a_codec, AVFrame * a_frame, int a_videoStream, const char * a_fileName, std::vector<int>* a_framesToRemove)
{
	OutputStream videoSt = { 0 };
	AVOutputFormat* ofmt;
	AVFormatContext* ofmtCtx;
	AVCodec* videoCodec = nullptr;
	int ret;
	int haveVideo = 0;
	int encodeVideo = 0;
	int i;

	/* allocate the output media context */
	avformat_alloc_output_context2(&ofmtCtx, nullptr, nullptr, a_fileName);
	if (!ofmtCtx)
	{
		printf("Error\n");
		exit(1);
	}

	ofmt = ofmtCtx->oformat;

	/* Add the audio and video streams using the default format codecs
	 * and initialize the codecs. */
	if (ofmt->video_codec != AV_CODEC_ID_NONE)
	{
		addStream(&videoSt, ofmtCtx, videoCodec, ofmt->video_codec);
		haveVideo = 1;
		encodeVideo = 1;
	}

	if (haveVideo)
	{
		openVideo(ofmtCtx, videoCodec, &videoSt);
	}

	av_dump_format(ofmtCtx, 0, a_fileName, 1);

	/* open the output file, if needed */
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmtCtx->pb, a_fileName, AVIO_FLAG_WRITE);
		if (ret < 0) {
			fprintf(stderr, "Could not open\n");
			return 1;
		}
	}

	/* Write the stream header, if any. */
	ret = avformat_write_header(ofmtCtx, nullptr);
	if (ret < 0) {
		fprintf(stderr, "Error occurred when opening output file\n");
		return 1;
	}

	while (encodeVideo)
	{
		encodeVideo = !writeVideoFrame(ofmtCtx, &videoSt, a_videoStream, a_fmtCtx, a_codec);
	}
	/* Write the trailer, if any. The trailer must be written before you
	 * close the CodecContexts open when you wrote the header; otherwise
	 * av_write_trailer() may try to use memory that was freed on
	 * av_codec_close(). */
	av_write_trailer(ofmtCtx);

	/* Close each codec. */
	if (haveVideo)
		closeStream(ofmtCtx, &videoSt);

	if (!(ofmt->flags & AVFMT_NOFILE))
		/* Close the output file. */
		avio_closep(&ofmtCtx->pb);
	/* free the stream */
	avformat_free_context(ofmtCtx);

	return 0;
}

void SratchEncode::addStream(OutputStream* a_os, AVFormatContext* a_of, AVCodec* a_codec, AVCodecID a_id)
{
	AVCodecContext* c;

	/* Find the encoder */
	a_codec = avcodec_find_encoder(a_id);
	if (!a_codec)
	{
		printf("Could not find encoder!!!\n");
		return;
	}

	a_os->st = avformat_new_stream(a_of, nullptr);
	if (!a_os->st)
	{
		printf("Could not alloc new stream!!!\n");
		return;
	}

	a_os->st->id == a_of->nb_streams - 1;
	c = avcodec_alloc_context3(a_codec);
	if (!c)
	{
		printf("Could not alloc an encoding context!!!\n");
		return;
	}
	a_os->enc = c;

	switch (a_codec->type)
	{
	case AVMEDIA_TYPE_VIDEO:
		c->codec_id = a_id;

		c->bit_rate = 400000;
		/* Resolution must be a multiple of two. */
		c->width = 1920;
		c->height = 1080;
		/* timebase: This is the fundamental unit of time (in seconds) in terms
		 * of which frame timestamps are represented. For fixed-fps content,
		 * timebase should be 1/framerate and timestamp increments should be
		 * identical to 1. */
		a_os->st->time_base = AVRational{ 1, 30 };
		c->time_base = a_os->st->time_base;

		/* emit one intra frame every twelve frames at most */
		c->gop_size = 12;
		c->pix_fmt = AV_PIX_FMT_YUV420P;
		c->max_b_frames = 2;

		if (a_os->st->codecpar->codec_id == AV_CODEC_ID_H264)
		{
			av_opt_set(c, "preset", "ultrafast", 0);
		}
		break;

	default:
		break;
	}

	/* Some formats want stream headers to be separate. */
	if (a_of->oformat->flags & AVFMT_GLOBALHEADER)
	{
		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}
}

void SratchEncode::openVideo(AVFormatContext * a_of, AVCodec * a_codec, OutputStream * a_os)
{
	int ret;
	AVCodecContext* c = a_os->enc;

	ret = avcodec_open2(c, a_codec, nullptr);
	if (ret < 0)
	{
		printf("Could not open video: %s!!!\n");
		return;
	}

	/* allocate and init a re-usable frame */
	a_os->frame = allocFrame(c->pix_fmt, c->width, c->height);
	if (!a_os->frame)
	{
		printf("Could not alloc video frame!!!\n");
		exit(1);
	}

	/* If the output format is not YUV420P, then a temporary YUV420P
	 * picture is needed too. It is then converted to the required
	 * output format. */
	a_os->tmp_frame = nullptr;
	if (c->pix_fmt != AV_PIX_FMT_YUV420P)
	{
		a_os->tmp_frame = allocFrame(AV_PIX_FMT_YUV420P, c->width, c->height);
		if (!a_os->tmp_frame)
		{
			printf("Could not alloc temp video frame!!!\n");
			exit(1);
		}
	}

	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(a_os->st->codecpar, c);
	if (ret < 0)
	{
		fprintf(stderr, "Could not copy the stream parameters\n");
		exit(1);
	}
}

AVFrame* SratchEncode::allocFrame(AVPixelFormat a_format, int a_width, int a_height)
{
	AVFrame* frame;
	int ret;

	frame = av_frame_alloc();
	if (!frame)
	{
		return nullptr;
	}

	frame->format = a_format;
	frame->width = a_width;
	frame->height = a_height;

	ret = av_frame_get_buffer(frame, 32);
	if (ret < 0)
	{
		printf("Could not alloc frame data!!!\n");
		return nullptr;
	}

	return frame;

	return nullptr;
}

int SratchEncode::writeVideoFrame(AVFormatContext * a_of, OutputStream * a_os, int videoStream, AVFormatContext* a_ifmt, AVCodecContext* a_dCodec)
{
	int ret = 0;
	AVCodecContext* c;
	AVFrame* frame;
	int gotPacket = 1, decode = 1;
	AVPacket pkt = { 0 }, encPkt = { 0 };

	c = a_os->enc;

	AVPixelFormat outForm = AV_PIX_FMT_YUV440P;
	AVFrame* cFrame = av_frame_alloc();
	uint8_t* bufferC = nullptr;
	int numBytesC;
	numBytesC = av_image_get_buffer_size(outForm, 1920, 1080, 1);
	bufferC = (uint8_t*)av_malloc(numBytesC * sizeof(uint8_t));
	av_image_fill_arrays(cFrame->data, cFrame->linesize, bufferC, outForm, 1920, 1080, 1);

	//copy frame from read frame to cFrame then get the packet.

	//get frame from 
	while (ret = ret = av_read_frame(a_ifmt, &pkt) >= 0)
	{
		if (pkt.stream_index == videoStream)
		{
			if (ret < 0)
			{
				printf("Could not encode video frame: %s\n", "");
				return 0;
			}

			if (gotPacket)
			{
				ret = writeFrame(a_of, &c->time_base, a_os->st, &pkt);
			}
			else
			{
				ret = 0;
			}

			if (pkt.data == nullptr)
			{
				char buf[256];
				av_strerror(ret, buf, sizeof(buf));
				fprintf(stderr, "Error while writing video frame: %s\n", buf);
			}
			else
			{
				if (ret < 0)
				{
					char buf[256];
					av_strerror(ret, buf, sizeof(buf));
					fprintf(stderr, "Error while writing video frame: %s\n", buf);
					exit(1);
				}
			}
			av_packet_unref(&pkt);
		}
	}

	return gotPacket ? 0 : 1;
}

AVFrame* SratchEncode::getVideoFrame(OutputStream * a_os)
{
	AVCodecContext* c = a_os->enc;

	if (av_frame_make_writable(a_os->frame) < 0)
	{
		exit(1);
	}

	if (c->pix_fmt != AV_PIX_FMT_YUV420P)
	{
		/* as we only generate a YUV420P picture, we must convert it
		 * to the codec pixel format if needed */
		if (a_os->sws_ctx)
		{
			a_os->sws_ctx = sws_getContext(c->width, c->height, AV_PIX_FMT_YUV420P,
				c->width, c->height, c->pix_fmt,
				SWS_BICUBIC, nullptr, nullptr, nullptr);

			if (a_os->sws_ctx)
			{
				printf("Could not init the coversion context!!!\n");
				exit(1);
			}
		}
		//fill image
		sws_scale(a_os->sws_ctx, (const uint8_t* const*)a_os->tmp_frame->data, a_os->tmp_frame->linesize, 0,
			c->height, a_os->frame->data, a_os->frame->linesize);
	}
	else
	{
		//fill image
	}

	a_os->frame->pts = a_os->next_pts++;

	return a_os->frame;
}

int SratchEncode::writeFrame(AVFormatContext * a_of, const AVRational * a_time_base, AVStream * a_st, AVPacket * a_pkt)
{
	/* rescale output packet timestamp values from codec to stream timebase */
	//av_packet_rescale_ts(a_pkt, *a_time_base, a_st->time_base);
	//a_pkt->stream_index = a_st->index;

	/* Write the compressed frame to the media file. */
	//log_pack

	return av_interleaved_write_frame(a_of, a_pkt);
}

void SratchEncode::closeStream(AVFormatContext * a_oc, OutputStream * a_ost)
{
	avcodec_free_context(&a_ost->enc);
	av_frame_free(&a_ost->frame);
	av_frame_free(&a_ost->tmp_frame);
	sws_freeContext(a_ost->sws_ctx);
	swr_free(&a_ost->swr_ctx);
}

void SratchEncode::SaveFrame(AVCodecContext * a_codec, AVFrame* a_frame, AVPacket * a_packet, FILE * a_file)
{
	int ret;

	if (a_frame)
	{
		printf("Send frame %3" PRId64 "\n", a_frame->pts);
	}

	ret = avcodec_send_frame(a_codec, a_frame);
	if (ret < 0)
	{
		fprintf(stderr, "Error sending a frame for encoding\n");
		return;
	}

	while (ret >= 0)
	{
		ret = avcodec_receive_packet(a_codec, a_packet);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			return;
		}
		else if (ret < 0)
		{
			printf("Error during encoding!!!\n");
			return;
		}

		printf("Write packet %3" PRId64 " (size=%5d)\n", a_packet->pts, a_packet->size);
		fwrite(a_packet->data, 1, a_packet->size, a_file);
		av_packet_unref(a_packet);
	}
}
