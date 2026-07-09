#include "FFVideoFormatConvert.h"
#include "VideoObjNetwork.h"

CFFVideoFormatConvert::CFFVideoFormatConvert(void)
	: m_img_convert_ctx(NULL)
	, m_pFrame(NULL)
	, m_pBuffer(NULL), m_uBufferSize(0)
	, m_iWidth(0), m_iHeight(0)
	, m_pImage(NULL)
{
}


CFFVideoFormatConvert::~CFFVideoFormatConvert(void)
{
	Close();
}

void CFFVideoFormatConvert::Close()
{
	if (m_img_convert_ctx)
	{
		sws_freeContext(m_img_convert_ctx);
		m_img_convert_ctx = NULL;
	}

	if (m_pFrame)
	{
		av_frame_free(&m_pFrame);
		m_pFrame = NULL;
	}

	if (m_pBuffer)
	{
		av_free(m_pBuffer);
		m_pBuffer = NULL;
		m_uBufferSize = 0;
	}

	if (m_pImage != NULL)
	{
		delete m_pImage;
		m_pImage = NULL;
	}

	m_iWidth = 0;
	m_iHeight = 0;
}

bool CFFVideoFormatConvert::RGB32toYUV420P(const QImage* pIn, AVFrame** pOut)
{
    // reinitialize object if image width or height changed
	if (pIn->width() != m_iWidth || pIn->height() != m_iHeight)
	{
		Close();
	}

	if (m_pBuffer == NULL)
	{
		m_iWidth = pIn->width();
		m_iHeight = pIn->height();
		m_uBufferSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pIn->width(), pIn->height(), 1);
		m_pBuffer = (uint8_t *) av_malloc(m_uBufferSize);
	}

	if (m_pFrame == NULL)
	{
		m_pFrame = av_frame_alloc();
		m_pFrame->width = pIn->width();
		m_pFrame->height = pIn->height();
		av_image_fill_arrays(m_pFrame->data, m_pFrame->linesize,
			m_pBuffer, AV_PIX_FMT_YUV420P, pIn->width(), pIn->height(), 1);
	}

	if (m_img_convert_ctx == NULL)
	{
		m_img_convert_ctx = sws_getContext(pIn->width(), pIn->height(), 
			AV_PIX_FMT_RGB32,
			pIn->width(), pIn->height(),
			AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	}

	const uint8_t *const srcSlice[] = { pIn->bits() };
	const int srcStride[] = { pIn->bytesPerLine()};
	sws_scale(m_img_convert_ctx,
		srcSlice,
		srcStride, 0, pIn->height(),
		m_pFrame->data,
		m_pFrame->linesize);

	*pOut = m_pFrame;
	return true;
}

bool CFFVideoFormatConvert::YUV420P2RGB32(const AVFrame* pIn, QImage** pOut)
{
    // reinitialize object if image width or height changed
	if (pIn->width != m_iWidth || pIn->height != m_iHeight)
	{
		Close();
	}

	if (m_pBuffer == NULL)
	{
		m_iWidth = pIn->width;
		m_iHeight = pIn->height;
		m_uBufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB32, pIn->width, pIn->height, 1);
		m_pBuffer = (uint8_t *)av_malloc(m_uBufferSize);
	}

	if (m_pFrame == NULL)
	{
		m_pFrame = av_frame_alloc();
		av_image_fill_arrays(m_pFrame->data, m_pFrame->linesize,
			m_pBuffer, AV_PIX_FMT_RGB32, pIn->width, pIn->height, 1);
	}

	if (m_img_convert_ctx == NULL)
	{
		m_img_convert_ctx = sws_getContext(pIn->width, pIn->height,
			AV_PIX_FMT_YUV420P,
			pIn->width, pIn->height,
			AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);
	}

	sws_scale(m_img_convert_ctx,
		(uint8_t const * const *)pIn->data,
		pIn->linesize, 0, pIn->height,
		m_pFrame->data,
		m_pFrame->linesize);

	*pOut = new QImage((uchar *)m_pFrame->data[0], m_iWidth, m_iHeight, QImage::Format_RGB32);

	return true;
}
