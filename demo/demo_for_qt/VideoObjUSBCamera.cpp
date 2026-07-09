#include "VideoObjUSBCamera.h"
#include "VLKVideoWidget.h"
#include "VideoObjNetwork.h"

CVideoObjUSBCamera::CVideoObjUSBCamera()
    : m_pWidget(NULL)
	, m_camera(NULL)
{
	m_cameraCapture = new QtCameraCapture;
	connect(m_cameraCapture, SIGNAL(frameAvailable(QImage)), this, SLOT(grabImage(QImage)));
}


CVideoObjUSBCamera::~CVideoObjUSBCamera()
{
	Close();

	if (m_cameraCapture != NULL)
	{
		delete m_cameraCapture;
		m_cameraCapture = NULL;
	}
}

bool CVideoObjUSBCamera::Open(const QString& strCameraDescription, VLKVideoWidget* pVideoWidget)
{
	Close();

    m_strCameraDescription = strCameraDescription;
    m_pWidget = pVideoWidget;

    // find Camera by description
	QByteArray byteCameraName;
	QList<QByteArray> listAvailableCamera = QCamera::availableDevices();
	for (QList<QByteArray>::const_iterator it = listAvailableCamera.begin();
		it != listAvailableCamera.end(); ++it)
	{
		if (0 == m_strCameraDescription.compare(QCamera::deviceDescription(*it)))
		{
			byteCameraName = *it;
			break;
		}
	}

	if (byteCameraName.isEmpty())
	{
		qCritical() << "could not find camera which description is " << m_strCameraDescription;
		return false;
	}

	m_camera = new QCamera(byteCameraName);
	m_camera->setViewfinder(m_cameraCapture);
	m_camera->start();

	DumpCameraCapability(m_camera);
	return true;
}

bool CVideoObjUSBCamera::IsOpen()
{
    return m_camera != NULL;
}

void CVideoObjUSBCamera::Clear()
{

}

void CVideoObjUSBCamera::Close()
{
	StopLocalRecord();

	if (m_camera != NULL)
	{
		m_camera->stop();
		delete m_camera;
		m_camera = NULL;
	}
}

void CVideoObjUSBCamera::grabImage(QImage image)
{
	static ulong ulPTS = 0;
	// static QTime time = QTime::currentTime();
	// qDebug () << "=============== frame duration: " << time.msecsTo(QTime::currentTime());
	// time = QTime::currentTime();
//	qDebug() << " width: " << image.width() << ", height: " << image.height() << ", byteCount: " << image.byteCount()
//		<< ", depth: " << image.depth() << ", format: " << image.format();

    // the default configuration makes image upside down
    // we need to correct it
	int* pRGBData = (int*)image.bits();
	for (int i = 0; i < image.height()>>1; ++i)
	{
		for (int j = 0; j < image.width(); ++j)
		{
			int uVal = pRGBData[i * image.width() + j];
			pRGBData[i * image.width() + j] = pRGBData[(image.height() - 1 - i) * image.width() + j];
			pRGBData[(image.height() - 1 - i) * image.width() + j] = uVal;
		}
	}

	AVFrame *frame = NULL;
	m_Converter.RGB32toYUV420P(&image, &frame);
	frame->pts = ulPTS;
    ulPTS += 40;
	if (m_pWidget != NULL && frame != NULL)
	{
		m_pWidget->Repaint(frame);
	}
}

void CVideoObjUSBCamera::DumpCameraCapability(const QCamera* pCamera)
{
	if (NULL == pCamera)
	{
		return;
	}

	QList<QSize> listResolutions = pCamera->supportedViewfinderResolutions();
	qDebug() << "################ supported resulution: ################";
	for (QList<QSize>::iterator it = listResolutions.begin(); it != listResolutions.end(); ++it)
	{
		qDebug() << " width: " << it->width() << ", height: " << it->height();
	}

	QList<QVideoFrame::PixelFormat> listPixelFormat = pCamera->supportedViewfinderPixelFormats();
	qDebug() << "################ supported pixel format: ################";
	for (QList<QVideoFrame::PixelFormat>::iterator it = listPixelFormat.begin(); it != listPixelFormat.end(); ++it)
	{
		qDebug() << "pixel format: " << *it;
	}

	 QCameraViewfinderSettings settings = pCamera->viewfinderSettings();
	qDebug() << "################ current camera video width:" << settings.resolution().width() << ", height: " << settings.resolution().height();
	qDebug() << "################ current camera video pixelFormat:" << settings.pixelFormat() << ", minimum frame rate: " << settings.minimumFrameRate();
	// m_sFrameDuration = 1000 / m_CameraSettings.minimumFrameRate();
    // m_sFrameRate = 10;
    // m_sFrameDuration = 100;
}

bool CVideoObjUSBCamera::StartLocalRecord()
{
	return true;
}

void CVideoObjUSBCamera::StopLocalRecord()
{
}

void CVideoObjUSBCamera::Capture()
{
}



