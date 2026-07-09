#include "stdafx.h"
#include "VideoObjNetwork.h"
#include "DirectDrawObj.h"

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")
// #pragma comment(lib,"swresample.lib")
// #pragma comment(lib, "swscale.lib")

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
	, m_pDirectDrawObj(NULL)
	, m_hWnd(NULL)
{
}


CVideoObjNetwork::~CVideoObjNetwork()
{
	Close();
}

bool CVideoObjNetwork::Open(const std::string& strURL, HWND hWnd)
{
	Close();

    m_strURL = strURL;
	m_hWnd = hWnd;

	m_bExit = false;
	m_pThread = new std::thread(ThreadFunc, this);

	return true;
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
	// ����rtsp������
	AVDictionary *opts = NULL;
	// av_dict_set(&opts, "rtsp_transport", "tcp", 0);
	// av_dict_set(&opts, "max_delay", "500", 0);
	av_dict_set(&opts, "stimeout", "5000", 0); // ����

	 m_pAVFmtContext = avformat_alloc_context();
	 AVIOInterruptCB cb = { interrupt_callback, this };
	 m_pAVFmtContext->interrupt_callback = cb;
	 m_pAVFmtContext->flags |= AVFMT_FLAG_NONBLOCK;
	AVInputFormat* inputformat = /*av_find_input_format("rtsp")*/NULL;
	int re = avformat_open_input(
		&m_pAVFmtContext,
        strURL.c_str(),
		inputformat,  // 0��ʾ�Զ�ѡ������
		&opts); //�������ã�����rtsp����ʱʱ��
	if (re != 0)
	{
		char buf[1024] = { 0 };
		av_strerror(re, buf, sizeof(buf) - 1);
        TRACE("open url %s failed! %s\n", strURL.c_str(), buf);

		if (re != AVERROR_EXIT) // �����رղ�������Ϣ
		{

		}

		return false;
	}
	TRACE("open %s success !\n", strURL.c_str());

	//��ȡ����Ϣ 
	re = avformat_find_stream_info(m_pAVFmtContext, 0);

	// std::string stdstrURL = strURL.toStdString();
	// av_dump_format(m_pAVFmtContext, 0, stdstrURL.c_str(), 0);

	//��ȡ��Ƶ��
	m_iVideoStreamIndex = av_find_best_stream(m_pAVFmtContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (m_iVideoStreamIndex < 0)
    {
        TRACE("could not find video stream !!!!!!!!!\n");
        return false;
    }
	AVStream *as = m_pAVFmtContext->streams[m_iVideoStreamIndex];
    m_iWidth = as->codecpar->width;
    m_iHeight = as->codecpar->height;
	TRACE("video info codec_id: %d, pixel format: %d, width: %d, height: %d, video fps: %lf\n",
		as->codecpar->codec_id, as->codecpar->format,
		as->codecpar->width, as->codecpar->height,
		r2d(as->avg_frame_rate));

	// ��ȡ��Ƶ��
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

			Sleep(1000);
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
            TRACE("av_read_frame error: %s\n", buf);
			break;
		}

        //ptsת��Ϊ����
        pkt->pts = pkt->pts*(1000 * (r2d(m_pAVFmtContext->streams[pkt->stream_index]->time_base)));
        pkt->dts = pkt->dts*(1000 * (r2d(m_pAVFmtContext->streams[pkt->stream_index]->time_base)));
        if (pkt->pts < 0)
        {
			TRACE("bad pts %ld, throw this packet away\n", pkt->pts);
            av_packet_free(&pkt);
            continue;
        }

		if (pkt->stream_index == m_iVideoStreamIndex)// ��Ƶ
		{
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

}

void CVideoObjNetwork::Clear()
{
/*	m_mutex.lock();
    if (m_pAVFmtContext != NULL)//�����ȡ����
    {
        avformat_flush(m_pAVFmtContext);
    }
    m_mutex.unlock();*/
}

bool CVideoObjNetwork::StartLocalRecord()
{
	return true;
}

void CVideoObjNetwork::StopLocalRecord()
{

}

void CVideoObjNetwork::Capture()
{

}

bool CVideoObjNetwork::OpenDecoder(const AVCodecParameters *para)
{
	AVCodec *vcodec = avcodec_find_decoder(para->codec_id);
	if (!vcodec)
	{
		TRACE("can't find the codec id = %d\n", para->codec_id);
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
		TRACE("avcodec_open2  failed! : %s\n", buf);
		return false;
	}

	return true;
}

void CVideoObjNetwork::CloseDecoder()
{
	if (m_pDirectDrawObj)
	{
		m_pDirectDrawObj->Close();
		delete m_pDirectDrawObj;
		m_pDirectDrawObj = NULL;
	}

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
		TRACE("avcodec_send_packet  failed! : %s\n", buf);
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

	if (NULL == m_pDirectDrawObj)
	{
		m_pDirectDrawObj = new CDirectDrawObj();
		if (0 != m_pDirectDrawObj->Open(NULL, m_hWnd, frame->width, frame->height, (AVPixelFormat)frame->format))
		{
			TRACE("m_pDirectDrawObj->Open  failed! : %s\n");
			m_pDirectDrawObj->Close();
			delete m_pDirectDrawObj;
			m_pDirectDrawObj = NULL;
			return;
		}

		::PostMessage(m_hWnd, WM_VIDEO_RESOLUTION, frame->width, frame->height);
	}

	if (m_pDirectDrawObj != NULL)
	{
		m_pDirectDrawObj->Input(frame);
	}
}

