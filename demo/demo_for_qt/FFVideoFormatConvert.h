#pragma once
#include <QImage>

struct AVFrame;
struct SwsContext;

class CFFVideoFormatConvert
{
public:
	CFFVideoFormatConvert(void);
	~CFFVideoFormatConvert(void);

	bool RGB32toYUV420P(const QImage* pIn, AVFrame** pOut);
	bool YUV420P2RGB32(const AVFrame* pIn, QImage** pOut);
private:
	void Close();
private:
	SwsContext* m_img_convert_ctx;
	AVFrame* m_pFrame;
	
	uint8_t* m_pBuffer;
	uint m_uBufferSize;
	int m_iWidth;
	int m_iHeight;

	QImage* m_pImage;
};

