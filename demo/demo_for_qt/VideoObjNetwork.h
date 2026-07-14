#pragma once
#include <string>
#include <mutex>
#include <thread>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
};

typedef void (*VideoDataCallback)(int iEncode, int iWidth, int iHeight, const char* pData, long lLen, long lPTS, void* pUserParam);

class VLKVideoWidget;
class CVideoObjNetwork
{
public:
	CVideoObjNetwork();
	virtual ~CVideoObjNetwork();

    virtual bool Open(const std::string& strURL, VLKVideoWidget* pVideoWidget);
    virtual bool IsOpen();
	void SetDataCallback(VideoDataCallback pVideoDataCB, long lUserParam);
	virtual void Clear();
	virtual void Close();
	virtual bool StartLocalRecord(const std::string& filename);
	virtual void StopLocalRecord();
	virtual void Capture();
private:
	static void ThreadFunc(CVideoObjNetwork* pThis);
    virtual void OnThreadFunc();

	bool OpenDemux(const std::string& strURL);
	void CloseDemux();
	void WriteLocalRecord(const AVPacket* pkt);
	static int  interrupt_callback(void* para);
	void ReadPacketLoop();
    
	bool OpenDecoder(const AVCodecParameters *para);
	void Send2Decode(const AVPacket* pkt);
	void Send2Display(const AVFrame* frame);
	void CloseDecoder();
private:
	static bool m_bInit;
    std::mutex m_mutex;
	std::string m_strURL;
    VLKVideoWidget* m_pVideoWidget;

	AVFormatContext* m_pAVFmtContext;
	int m_iVideoStreamIndex;
	int m_iAudioStreamIndex;
    int m_iWidth;
    int m_iHeight;

	std::thread* m_pThread;
    bool m_bExit;

	VideoDataCallback m_cbFunc;
	long m_lUserParam;

	AVCodecContext* m_pCodecContext;

	// Recording variables
	bool m_bRecordEnabled;
	AVFormatContext* m_pOutFmtCtx;
	AVStream* m_pOutStream;
	std::mutex m_recordMutex;
	int64_t m_recordFrameCount;
	int64_t m_firstRecordDts;
	bool m_waitingForRecordKeyframe;
};
