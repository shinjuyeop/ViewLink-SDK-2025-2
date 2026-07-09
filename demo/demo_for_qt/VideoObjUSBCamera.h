#pragma once
#include <QCamera>
#include <QObject>
#include "QtCameraCapture.h"
#include "FFVideoFormatConvert.h"

class VLKVideoWidget;
class CVideoObjUSBCamera : public QObject
{
	Q_OBJECT
public:
    CVideoObjUSBCamera();
	virtual ~CVideoObjUSBCamera();

    virtual bool Open(const QString& strCameraDescription, VLKVideoWidget* pVideoWidget);
    virtual bool IsOpen();
	virtual void Clear();
	virtual void Close();
	virtual bool StartLocalRecord();
	virtual void StopLocalRecord();
	virtual void Capture();
private slots:
	void grabImage(QImage image);
private:
	void DumpCameraCapability(const QCamera* pCamera);
private:
	QString m_strCameraDescription;
    VLKVideoWidget* m_pWidget;

	QtCameraCapture* m_cameraCapture;
	QCamera* m_camera;

	CFFVideoFormatConvert m_Converter;
};

