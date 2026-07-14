#include "VideoObjNetwork.h"
#include "VLKVideoWidget.h"

bool CVideoObjNetwork::m_bInit = false;

static double r2d(AVRational r)
{
	return r.den == 0 ? 0 : (double)r.num / (double)r.den;
}

CVideoObjNetwork::CVideoObjNetwork()
	: m_pAVFmtContext(NULL)
	, m_iVideoStreamIndex(-1)
	, m_iAudioStreamIndex(-1)
    , m_iWidth(0), m_iHeight(0)
    , m_pThread(NULL)
    , m_cbFunc(NULL)
    , m_lUserParam(0)
    , m_bExit(false)
	, m_pCodecContext(NULL)
    , m_pVideoWidget(NULL)
	,m_bRecordEnabled(false)
	, m_pOutFmtCtx(NULL)
	, m_pOutStream(NULL)
	, m_recordFrameCount(0)
	, m_firstRecordDts(AV_NOPTS_VALUE)
	, m_waitingForRecordKeyframe(true)
{
}


CVideoObjNetwork::~CVideoObjNetwork()
{
	Close();
}

void ffmpeg_log_callback(void* ptr, int level, const char* fmt, va_list vl)
{
    // va_start(vl, fmt);
    QString strResult = QString::vasprintf(fmt, vl);
    // va_end(vl);

    qDebug() << strResult;
}

bool CVideoObjNetwork::Open(const std::string& strURL, VLKVideoWidget* pVideoWidget)
{
    if (!m_bInit)
    {
        // av_log_set_callback(ffmpeg_log_callback);

        // initialize ffmpeg
        av_register_all();
        avformat_network_init();

        m_bInit = true;
    }

	Close();

    m_strURL = strURL;
    m_pVideoWidget = pVideoWidget;

	m_bExit = false;
	m_pThread = new std::thread(ThreadFunc, this);

	return true;
}

bool CVideoObjNetwork::IsOpen()
{
   return m_pThread != NULL;
}

void CVideoObjNetwork::SetDataCallback(VideoDataCallback pVideoDataCB, long lUserParam)
{
	m_cbFunc = pVideoDataCB;
    m_lUserParam = lUserParam;
}

void CVideoObjNetwork::Close()
{
	if (m_pThread != NULL)
	{
		m_bExit = true;
		m_pThread->join();
		delete m_pThread;
		m_pThread = NULL;
	}

	CloseDemux();

	CloseDecoder();
}

bool CVideoObjNetwork::OpenDemux(const std::string& strURL)
{
	AVDictionary *opts = NULL;
	av_dict_set(&opts, "rtsp_transport", "tcp", 0);
	av_dict_set(&opts, "max_delay", "500000", 0);
	av_dict_set(&opts, "stimeout", "5000000", 0);

	 m_pAVFmtContext = avformat_alloc_context();
	 AVIOInterruptCB cb = { interrupt_callback, this };
	 m_pAVFmtContext->interrupt_callback = cb;
	 m_pAVFmtContext->flags |= AVFMT_FLAG_NONBLOCK;
	AVInputFormat* inputformat = /*av_find_input_format("rtsp")*/NULL;
	int re = avformat_open_input(
		&m_pAVFmtContext,
        strURL.c_str(),
        inputformat,
        &opts);
	if (re != 0)
	{
		av_dict_free(&opts);
		char buf[1024] = { 0 };
		av_strerror(re, buf, sizeof(buf) - 1);
        qCritical("open url %s failed! %s\n", strURL.c_str(), buf);

        if (re != AVERROR_EXIT)
		{

		}

		return false;
	}
	av_dict_free(&opts);
    qDebug("open %s success !\n", strURL.c_str());

	re = avformat_find_stream_info(m_pAVFmtContext, 0);

	// std::string stdstrURL = strURL.toStdString();
	// av_dump_format(m_pAVFmtContext, 0, stdstrURL.c_str(), 0);

    // find video stream
	m_iVideoStreamIndex = av_find_best_stream(m_pAVFmtContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (m_iVideoStreamIndex < 0)
    {
        qCritical("could not find video stream !!!!!!!!!\n");
        return false;
    }
	AVStream *as = m_pAVFmtContext->streams[m_iVideoStreamIndex];
    m_iWidth = as->codecpar->width;
    m_iHeight = as->codecpar->height;
    qDebug("video info codec_id: %d, pixel format: %d, width: %d, height: %d, video fps: %lf\n",
		as->codecpar->codec_id, as->codecpar->format,
		as->codecpar->width, as->codecpar->height,
		r2d(as->avg_frame_rate));

    // find audio stream
	/*m_iAudioStreamIndex = av_find_best_stream(m_pAVFmtContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	as = m_pAVFmtContext->streams[m_iAudioStreamIndex];
	m_iSampleRate = as->codecpar->sample_rate;
	m_iChannels = as->codecpar->channels;
	qDebug("audio info codec_id: %d, format: %d, sample_rate: %d, channels: %d, frame_size: %d",
	as->codecpar->codec_id, as->codecpar->format,
	as->codecpar->sample_rate, as->codecpar->channels,
	as->codecpar->frame_size);*/

	return true;
}

void CVideoObjNetwork::CloseDemux()
{
	StopLocalRecord();

	if (m_pAVFmtContext != NULL)
	{
		avformat_close_input(&m_pAVFmtContext);
	}
	m_iVideoStreamIndex = -1;
	m_iAudioStreamIndex = -1;
}

int CVideoObjNetwork::interrupt_callback(void* para)
{
	// qDebug() << __FUNCTION__;
	CVideoObjNetwork* pThis = (CVideoObjNetwork*)para;
	if (NULL == pThis)
	{
		return 0;
	}

    if (pThis->m_bExit)
    {
        return 1;
    }
    
	return 0;
}

void CVideoObjNetwork::ThreadFunc(CVideoObjNetwork* pThis)
{
	if (pThis != NULL)
	{
		pThis->OnThreadFunc();
	}
}

void CVideoObjNetwork::OnThreadFunc()
{
    while (!m_bExit)
    {
        if (!OpenDemux(m_strURL))
        {
            CloseDemux();

			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

		AVStream *as = m_pAVFmtContext->streams[m_iVideoStreamIndex];
		if (!OpenDecoder(as->codecpar))
		{
			CloseDecoder();
			CloseDemux();
			continue;
		}

        ReadPacketLoop();

		CloseDecoder();
        CloseDemux();
    }
}

void CVideoObjNetwork::ReadPacketLoop()
{
    while (!m_bExit)
	{
		AVPacket *pkt = av_packet_alloc();
		int re = av_read_frame(m_pAVFmtContext, pkt);
		if (re != 0)
		{
			av_packet_free(&pkt);
			char buf[1024] = { 0 };
			av_strerror(re, buf, sizeof(buf) - 1);
            qCritical("av_read_frame error: %s\n", buf);
			break;
		}

		// send video package to decoder
        if (pkt->stream_index == m_iVideoStreamIndex)
		{
			WriteLocalRecord(pkt);
			Send2Decode(pkt);
		}
		else
		{
		}

		av_packet_free(&pkt);
	}
}

void CVideoObjNetwork::WriteLocalRecord(const AVPacket* pkt)
{
    std::lock_guard<std::mutex> lock(m_recordMutex);
    if (!m_bRecordEnabled || m_pOutFmtCtx == NULL) {
        return;
    }

    AVPacket* out_pkt = av_packet_clone(pkt);
    if (!out_pkt) {
        return;
    }

	AVStream* inStream = m_pAVFmtContext->streams[m_iVideoStreamIndex];
	AVStream* outStream = m_pOutStream;

	if (m_waitingForRecordKeyframe) {
		if (!(out_pkt->flags & AV_PKT_FLAG_KEY)) {
			av_packet_free(&out_pkt);
			return;
		}
		m_waitingForRecordKeyframe = false;
	}

	int64_t pts = out_pkt->pts;
	int64_t dts = out_pkt->dts;

	if (pts == AV_NOPTS_VALUE && dts != AV_NOPTS_VALUE) {
		pts = dts;
	} else if (pts == AV_NOPTS_VALUE) {
		AVRational frameRate = av_guess_frame_rate(m_pAVFmtContext, inStream, NULL);
		if (frameRate.num <= 0 || frameRate.den <= 0) {
			frameRate = AVRational{30, 1};
		}
		pts = av_rescale_q(m_recordFrameCount, av_inv_q(frameRate), inStream->time_base);
	}
	if (dts == AV_NOPTS_VALUE) {
		dts = pts;
	}
	++m_recordFrameCount;

    if (m_firstRecordDts == AV_NOPTS_VALUE) {
        m_firstRecordDts = dts;
    }

    pts -= m_firstRecordDts;
    dts -= m_firstRecordDts;

	out_pkt->pts = pts;
	out_pkt->dts = dts;
	av_packet_rescale_ts(out_pkt, inStream->time_base, outStream->time_base);
	out_pkt->stream_index = outStream->index;
	out_pkt->pos = -1;

    int ret = av_interleaved_write_frame(m_pOutFmtCtx, out_pkt);
    if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        qCritical("Error writing video frame: %s", errbuf);
    }
    av_packet_free(&out_pkt);
}

void CVideoObjNetwork::Clear()
{
    m_mutex.lock();
    if (m_pAVFmtContext != NULL)
    {
        avformat_flush(m_pAVFmtContext);
    }
    m_mutex.unlock();
}

bool CVideoObjNetwork::StartLocalRecord(const std::string& filename)
{
    std::lock_guard<std::mutex> lock(m_recordMutex);
    if (m_bRecordEnabled) {
        return false;
    }

    if (m_pAVFmtContext == NULL || m_iVideoStreamIndex < 0) {
        qCritical("Cannot start record: input stream is not open.");
        return false;
    }

    int ret = avformat_alloc_output_context2(&m_pOutFmtCtx, NULL, NULL, filename.c_str());
    if (ret < 0 || !m_pOutFmtCtx) {
        qCritical("avformat_alloc_output_context2 failed for %s", filename.c_str());
        return false;
    }

    AVStream* inStream = m_pAVFmtContext->streams[m_iVideoStreamIndex];
    m_pOutStream = avformat_new_stream(m_pOutFmtCtx, NULL);
    if (!m_pOutStream) {
        qCritical("Failed to allocate output stream");
        avformat_free_context(m_pOutFmtCtx);
        m_pOutFmtCtx = NULL;
        return false;
    }

    ret = avcodec_parameters_copy(m_pOutStream->codecpar, inStream->codecpar);
    if (ret < 0) {
        qCritical("Failed to copy codec parameters");
        avformat_free_context(m_pOutFmtCtx);
        m_pOutFmtCtx = NULL;
        m_pOutStream = NULL;
        return false;
    }
    m_pOutStream->codecpar->codec_tag = 0;

    if (!(m_pOutFmtCtx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&m_pOutFmtCtx->pb, filename.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            qCritical("Failed to open output file %s", filename.c_str());
            avformat_free_context(m_pOutFmtCtx);
            m_pOutFmtCtx = NULL;
            m_pOutStream = NULL;
            return false;
        }
    }

    ret = avformat_write_header(m_pOutFmtCtx, NULL);
    if (ret < 0) {
        qCritical("Failed to write output header");
        if (!(m_pOutFmtCtx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&m_pOutFmtCtx->pb);
        }
        avformat_free_context(m_pOutFmtCtx);
        m_pOutFmtCtx = NULL;
        m_pOutStream = NULL;
        return false;
    }

	m_recordFrameCount = 0;
	m_firstRecordDts = AV_NOPTS_VALUE;
	m_waitingForRecordKeyframe = true;
    m_bRecordEnabled = true;
    qDebug("Started recording to file: %s", filename.c_str());
    return true;
}

void CVideoObjNetwork::StopLocalRecord()
{
    std::lock_guard<std::mutex> lock(m_recordMutex);
    if (!m_bRecordEnabled) {
        return;
    }

    m_bRecordEnabled = false;
    if (m_pOutFmtCtx) {
        av_write_trailer(m_pOutFmtCtx);
        if (!(m_pOutFmtCtx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&m_pOutFmtCtx->pb);
        }
        avformat_free_context(m_pOutFmtCtx);
        m_pOutFmtCtx = NULL;
        m_pOutStream = NULL;
    }
    qDebug("Stopped recording.");
}

void CVideoObjNetwork::Capture()
{

}

bool CVideoObjNetwork::OpenDecoder(const AVCodecParameters *para)
{
	AVCodec *vcodec = avcodec_find_decoder(para->codec_id);
	if (!vcodec)
	{
        qCritical("can't find the codec id = %d\n", para->codec_id);
		return false;
	}

	m_pCodecContext = avcodec_alloc_context3(vcodec);
	avcodec_parameters_to_context(m_pCodecContext, para);
	m_pCodecContext->thread_count = 1/*QThread::idealThreadCount()*/;

	int re = avcodec_open2(m_pCodecContext, 0, 0);
	if (re != 0)
	{
		avcodec_free_context(&m_pCodecContext);
		char buf[1024] = { 0 };
		av_strerror(re, buf, sizeof(buf) - 1);
        qCritical("avcodec_open2  failed! : %s\n", buf);
		return false;
	}

	return true;
}

void CVideoObjNetwork::CloseDecoder()
{
	if (m_pCodecContext)
	{
		avcodec_close(m_pCodecContext);
		avcodec_free_context(&m_pCodecContext);
	}
}

void CVideoObjNetwork::Send2Decode(const AVPacket *pkt)
{
	if (!pkt || pkt->size <= 0 || !pkt->data || !m_pCodecContext)
	{
		return;
	}

	int re = avcodec_send_packet(m_pCodecContext, pkt);
	if (re != 0)
	{
		char buf[1024] = { 0 };
		av_strerror(re, buf, sizeof(buf) - 1);
        qCritical("avcodec_send_packet  failed! : %s\n", buf);
	}

	while (!m_bExit)
	{
		AVFrame *frame = av_frame_alloc();
		int re = avcodec_receive_frame(m_pCodecContext, frame);
		if (re != 0)
		{
			av_frame_free(&frame);
			break;
		}

		// display video
		Send2Display(frame);

		av_frame_free(&frame);
	}
}

void CVideoObjNetwork::Send2Display(const AVFrame* frame)
{
	if (!frame || frame->data[0] == NULL || frame->linesize[0] == 0)
	{
		return;
	}

    if (m_pVideoWidget != NULL)
	{
        m_pVideoWidget->Repaint(frame);
	}
}

