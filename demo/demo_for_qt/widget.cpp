#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QCamera>
#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QSaveFile>
#include <QStringList>
#include <QTimer>
#include <cmath>

namespace {
const qint64 kSrtFrameRate = 30;

QString formatDmsCoordinate(double coordinate, bool latitude)
{
    const QChar positiveHemisphere = latitude ? QChar('N') : QChar('E');
    const QChar negativeHemisphere = latitude ? QChar('S') : QChar('W');
    const QChar hemisphere = coordinate < 0.0
        ? negativeHemisphere
        : positiveHemisphere;

    // Round the complete coordinate so a value such as 59.999 seconds
    // correctly carries into the next minute or degree.
    const qint64 totalHundredths = qRound64(std::fabs(coordinate) * 360000.0);
    const qint64 degrees = totalHundredths / 360000;
    const qint64 minuteRemainder = totalHundredths % 360000;
    const qint64 minutes = minuteRemainder / 6000;
    const double seconds = (minuteRemainder % 6000) / 100.0;

    return QString("%1 %2%3%4%5%6%7")
        .arg(hemisphere)
        .arg(degrees)
        .arg(QChar(0x00b0))
        .arg(minutes)
        .arg(QChar(0x2032))
        .arg(seconds, 0, 'f', 2)
        .arg(QChar(0x2033));
}
}

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    m_bIsRecording(false),
    m_deviceConnected(false),
    m_hasTelemetry(false),
    m_previousSrtMs(0),
    m_srtIndex(1)
{
    memset(&m_latestTelemetry, 0, sizeof(m_latestTelemetry));
    ui->setupUi(this);
    qRegisterMetaType<VLK_DEV_MODEL>("VLK_DEV_MODEL");
    qRegisterMetaType<VLK_DEV_CONFIG>("VLK_DEV_CONFIG");
    qRegisterMetaType<VLK_DEV_TELEMETRY>("VLK_DEV_TELEMETRY");

    connect(this, SIGNAL(SignalConnectionStatus(int, const QString&)), this, SLOT(onSlotConnectionStatus(int, const QString&)));
    connect(this, SIGNAL(SignalDeviceModel(VLK_DEV_MODEL)), this, SLOT(onSlotDeviceModel(VLK_DEV_MODEL)));
    connect(this, SIGNAL(SignalDeviceConfig(VLK_DEV_CONFIG)), this, SLOT(onSlotDeviceConfig(VLK_DEV_CONFIG)));
    connect(this, SIGNAL(SignalDeviceTelemetry(VLK_DEV_TELEMETRY)), this, SLOT(onSlotDeviceTelemetry(VLK_DEV_TELEMETRY)));
    connect(&m_srtTimer, SIGNAL(timeout()), this, SLOT(writeSrtTelemetry()));
    m_srtTimer.setTimerType(Qt::PreciseTimer);

    InitUI();

    VLK_RegisterDevStatusCB(VLK_DevStatusCallback, this);
}

Widget::~Widget()
{
    if (m_bIsRecording) {
		stopRecording("application_shutdown");
    }
    m_VideoObjNetworkEO.Close();
    m_VideoObjNetworkIR.Close();
    m_VideoObjUSBCamera.Close();

    delete ui;
}

void Widget::InitUI()
{
    // initialize serial port list
    ui->cmbSerialPort->clear();
    QList<QSerialPortInfo> listSerialPortInfo = QSerialPortInfo::availablePorts();
    foreach(const QSerialPortInfo &port_info, listSerialPortInfo)
    {
        ui->cmbSerialPort->addItem(port_info.portName());
    }

    ui->lineEditBaudrate->setText("115200");

    // initialize tcp gimbal list
    ui->cmbIPAddr->addItem("192.168.2.119");
    ui->lineEditPort->setText("2000");

    ui->cmbNetworkVideoURL->addItem("rtsp://192.168.2.119/live1");
    ui->cmbNetworkVideoURL->addItem("rtsp://127.0.0.1:8554/live1");

    ui->cmbNetworkVideoURLIR->addItem("rtsp://192.168.2.119/live2");
    ui->cmbNetworkVideoURLIR->addItem("rtsp://127.0.0.1:8554/live2");

    // initialize USB Camera list
    QList<QByteArray> listAvailableCamera = QCamera::availableDevices();
    foreach (const QByteArray &camera, listAvailableCamera)
    {
        ui->cmbUSBCamera->addItem(QCamera::deviceDescription(camera));
    }

    // initialize image sensor combobox items
    ui->cmbImageSensor->addItem(tr("Visible1"), VLK_IMAGE_TYPE_VISIBLE1);
    ui->cmbImageSensor->addItem(tr("IR"), VLK_IMAGE_TYPE_IR1);
    // ui->cmbImageSensor->addItem(tr("Visible2"), VLK_IMAGE_TYPE_VISIBLE2);
    // ui->cmbImageSensor->addItem(tr("IR2"), VLK_IMAGE_TYPE_IR2);
    // ui->cmbImageSensor->addItem(tr("Fusion"), VLK_IMAGE_TYPE_FUSION);

    // initialize IR color combobox items
    ui->cmbIRColor->addItem(tr("White hot"), VLK_IR_COLOR_WHITEHOT);
    ui->cmbIRColor->addItem(tr("Black hot"), VLK_IR_COLOR_BLACKHOT);
    ui->cmbIRColor->addItem(tr("Pseudo hot"), VLK_IR_COLOR_PSEUDOHOT);

    // Set recording button initial states
    ui->btnStartRecord->setEnabled(true);
    ui->btnStopRecord->setEnabled(false);
}

int Widget::VLK_ConnStatusCallback(int iConnStatus, const char* szMessage, int iMsgLen, void* pUserParam)
{
    Widget* pThis = (Widget*)pUserParam;
    if (NULL == pThis)
    {
        return 0;
    }

    QString strMessage;
    if (szMessage != NULL && iMsgLen > 0 )
    {
        strMessage = QString::fromUtf8(szMessage);
    }

    emit pThis->SignalConnectionStatus(iConnStatus, strMessage);

    return 0;
}

int Widget::VLK_DevStatusCallback(int iType, const char* szBuffer, int iBufLen, void* pUserParam)
{
    Widget* pThis = (Widget*)pUserParam;
    if (NULL == pThis)
    {
        return 0;
    }

    if (iType == VLK_DEV_STATUS_TYPE_MODEL)
    {
        if (iBufLen != sizeof(VLK_DEV_MODEL))
        {
            qCritical("################## bad device model info\n");
            return 0;
        }

        VLK_DEV_MODEL* pModel = (VLK_DEV_MODEL*)szBuffer;
        qDebug("model code: %02x, model name: %s\n", pModel->cModelCode, pModel->szModelName);

        emit pThis->SignalDeviceModel(*pModel);
    }
    else if (iType == VLK_DEV_STATUS_TYPE_CONFIG)
    {
        if (iBufLen != sizeof(VLK_DEV_CONFIG))
        {
            qCritical("################## bad device configure\n");
            return 0;
        }

        VLK_DEV_CONFIG* pDevConfig = (VLK_DEV_CONFIG*)szBuffer;
        qDebug("VersionNO: %s, DeviceID: %s, SerialNO: %s\n",
            pDevConfig->cVersionNO, pDevConfig->cDeviceID, pDevConfig->cSerialNO);

        emit pThis->SignalDeviceConfig(*pDevConfig);
    }
    else if (iType == VLK_DEV_STATUS_TYPE_TELEMETRY)
    {
        if (iBufLen != sizeof(VLK_DEV_TELEMETRY))
        {
            qCritical("################## bad telemetry data\n");
            return 0;
        }

        VLK_DEV_TELEMETRY* pTelemetry = (VLK_DEV_TELEMETRY*)szBuffer;
        // qDebug("Yaw: %lf, Pitch: %lf, Sensor type: %02x, Zoom mag times: %d\n",
        //    pTelemetry->dYaw, pTelemetry->dPitch, pTelemetry->emSensorType, pTelemetry->sZoomMagTimes);

        emit pThis->SignalDeviceTelemetry(*pTelemetry);
    }

    return 0;
}

void Widget::on_btnConnectTCP_clicked()
{
    // check if serial port connected
    if (!!VLK_IsSerialPortConnected())
    {
        VLK_DisconnectSerialPort();
        ui->btnConnctSerialPort->setText(tr("Connect"));
    }

    // check if tcp connected
    if (!!VLK_IsTCPConnected())
    {
       VLK_DisconnectTCP();
       ui->btnConnectTCP->setText(tr("Connect"));
    }
    else
    {
        // connect TCP Gimbal
        VLK_CONN_PARAM Param;
        memset(&Param, 0, sizeof(VLK_CONN_PARAM));
        Param.emType = VLK_CONN_TYPE_TCP;

        QString strIPAddr = ui->cmbIPAddr->currentText();
        std::string stdstrIPAddr = strIPAddr.toStdString();
        strncpy(Param.ConnParam.IPAddr.szIPV4, stdstrIPAddr.c_str(), 16);
        Param.ConnParam.IPAddr.szIPV4[15] = '\0';
        Param.ConnParam.IPAddr.iPort = ui->lineEditPort->text().toInt();

        if (VLK_ERROR_NO_ERROR != VLK_Connect(&Param, VLK_ConnStatusCallback, this))
        {
            qCritical() << "create tcp connection failed!";
            ui->btnConnectTCP->setText(tr("Connect"));
        }
        else
        {
            ui->btnConnectTCP->setText(tr("Connecting"));
        }
    }
}

void Widget::on_btnConnctSerialPort_clicked()
{
   // check if tcp connected
  if (!!VLK_IsTCPConnected())
  {
     VLK_DisconnectTCP();
     ui->btnConnectTCP->setText(tr("Connect"));
  }

  // check if serial port connected
  if (!!VLK_IsSerialPortConnected())
  {
      VLK_DisconnectSerialPort();
      ui->btnConnctSerialPort->setText(tr("Connect"));
  }
  else
  {
      // connect serial port
      VLK_CONN_PARAM Param;
      memset(&Param, 0, sizeof(VLK_CONN_PARAM));
      Param.emType = VLK_CONN_TYPE_SERIAL_PORT;

      QString strSerialPort = ui->cmbSerialPort->currentText();
      std::string stdstrSerialPort = strSerialPort.toStdString();
      strncpy(Param.ConnParam.SerialPort.szSerialPortName, stdstrSerialPort.c_str(), 16);
      Param.ConnParam.SerialPort.szSerialPortName[15] = '\0';
      Param.ConnParam.SerialPort.iBaudRate = ui->lineEditBaudrate->text().toInt();

      if (VLK_ERROR_NO_ERROR != VLK_Connect(&Param, VLK_ConnStatusCallback, this))
      {
          qCritical() << "create tcp connection failed!";
          ui->btnConnctSerialPort->setText(tr("Connect"));
      }
      else
      {
          ui->btnConnctSerialPort->setText(tr("Connecting"));
      }
  }
}

void Widget::onSlotConnectionStatus(int iConnStatus, const QString& strMessage)
{
    Q_UNUSED(strMessage);
    if (iConnStatus == VLK_CONN_STATUS_TCP_CONNECTED)
    {
       m_deviceConnected = true;
       qDebug() << "TCP Gimbal connected !!!";
       ui->btnConnectTCP->setText(tr("Disconnect"));
    }
    else if (iConnStatus == VLK_CONN_STATUS_TCP_DISCONNECTED)
    {
        m_deviceConnected = false;
        m_hasTelemetry = false;
        qDebug() << "TCP Gimbal disconnected !!!";
        ui->btnConnectTCP->setText("Connect");
    }
    else if (iConnStatus == VLK_CONN_STATUS_SERIAL_PORT_CONNECTED)
    {
        m_deviceConnected = true;
        qDebug() << "Serial port Gimbal connected !!!";
        ui->btnConnctSerialPort->setText(tr("Disconnect"));
    }
    else if (iConnStatus == VLK_CONN_STATUS_SERIAL_PORT_DISCONNECTED)
    {
        m_deviceConnected = false;
        m_hasTelemetry = false;
        qDebug() << "Serial port Gimbal disconnected !!!";
        ui->btnConnctSerialPort->setText(tr("Connect"));
    }
    else
    {
        qCritical() << "unknown connection status: " << iConnStatus;
    }
}

void Widget::onSlotDeviceModel(VLK_DEV_MODEL model)
{
    Q_UNUSED(model);
}

void Widget::onSlotDeviceConfig(VLK_DEV_CONFIG config)
{
    Q_UNUSED(config);
}

void Widget::onSlotDeviceTelemetry(VLK_DEV_TELEMETRY telemetry)
{
    m_latestTelemetry = telemetry;
    m_hasTelemetry = true;
    m_lastTelemetryTimer.start();

    QString::number(telemetry.dPitch);
    ui->lbYaw->setText(QString::number(telemetry.dYaw));
    ui->lbPitch->setText(QString::number(telemetry.dPitch));
    ui->lbTargetLat->setText(QString::number(telemetry.dTargetLat));
    ui->lbTargetLng->setText(QString::number(telemetry.dTargetLng));
    ui->lbTargetAlt->setText(QString::number(telemetry.dTargetAlt));
    ui->lbZoom->setText(QString::number(telemetry.dZoomMagTimes));
    ui->lbLaserDistance->setText(QString::number(telemetry.sLaserDistance));

}

void Widget::writeSrtTelemetry()
{
    if (!m_bIsRecording || !m_recordElapsedTimer.isValid()) {
        return;
    }

	const bool eoHealthy = m_VideoObjNetworkEO.IsStreamReady()
		&& m_VideoObjNetworkEO.IsLocalRecording();
	const bool irHealthy = m_VideoObjNetworkIR.IsStreamReady()
		&& m_VideoObjNetworkIR.IsLocalRecording();
	if (!eoHealthy || !irHealthy) {
		QStringList interruptedStreams;
		if (!eoHealthy) {
			interruptedStreams.append("EO");
		}
		if (!irHealthy) {
			interruptedStreams.append("IR");
		}
		const QString streamNames = interruptedStreams.join(",");
		stopRecording(
			QString("stream_interrupted:%1").arg(streamNames),
			tr("%1 stream disconnected or stopped. EO and IR recording was stopped together.")
				.arg(streamNames));
		return;
	}

	appendSrtTelemetry();
}

void Widget::appendSrtTelemetry(bool finalizePartialFrame)
{
	if (!m_bIsRecording || !m_recordElapsedTimer.isValid()) {
		return;
	}

    const qint64 recordingElapsedMs = m_recordElapsedTimer.elapsed();
    const auto formatSrtTime = [](qint64 value) {
        return QString("%1:%2:%3,%4")
            .arg(value / 3600000, 2, 10, QChar('0'))
            .arg((value / 60000) % 60, 2, 10, QChar('0'))
            .arg((value / 1000) % 60, 2, 10, QChar('0'))
            .arg(value % 1000, 3, 10, QChar('0'));
    };

    const bool telemetryFresh = m_deviceConnected
        && m_hasTelemetry
        && m_lastTelemetryTimer.isValid()
        && m_lastTelemetryTimer.elapsed() <= 2000;

    QString telemetryText;
    if (telemetryFresh) {
        const double irZoom = m_latestTelemetry.iIRDigitalZoom > 0
            ? static_cast<double>(m_latestTelemetry.iIRDigitalZoom)
            : 1.0;
        telemetryText = QString(
            "ACFT %1 ACFT %2 ACFT ALT  %3m\n"
            "TAG %4 TAG %5 TAG ALT  %6m\n"
            "EO %7x  FOV %8%9  IR %10x  LRF %11m\n"
            "Plat: p  ACFT: pitch  %12  yaw  %13  roll  %14")
            .arg(formatDmsCoordinate(m_latestTelemetry.dDroneLat, true))
            .arg(formatDmsCoordinate(m_latestTelemetry.dDroneLng, false))
            .arg(m_latestTelemetry.dDroneAlt, 0, 'f', 3)
            .arg(formatDmsCoordinate(m_latestTelemetry.dTargetLat, true))
            .arg(formatDmsCoordinate(m_latestTelemetry.dTargetLng, false))
            .arg(m_latestTelemetry.dTargetAlt, 0, 'f', 3)
            .arg(m_latestTelemetry.dZoomMagTimes, 0, 'f', 1)
            .arg(m_latestTelemetry.dFov, 0, 'f', 1)
            .arg(QChar(0x00b0))
            .arg(irZoom, 0, 'f', 1)
            .arg(static_cast<double>(m_latestTelemetry.sLaserDistance), 0, 'f', 1)
            .arg(m_latestTelemetry.dPitch, 0, 'f', 2)
            .arg(m_latestTelemetry.dYaw, 0, 'f', 2)
            .arg(m_latestTelemetry.dRoll, 0, 'f', 2);
    } else {
        telemetryText = QString(
            "ACFT N/A ACFT N/A ACFT ALT  N/A\n"
            "TAG N/A TAG N/A TAG ALT  N/A\n"
            "EO N/A  FOV N/A  IR N/A  LRF N/A\n"
            "Plat: p  ACFT: pitch  N/A  yaw  N/A  roll  N/A");
    }

    const auto writeBlock = [&](qint64 startMs, qint64 endMs) {
        const QString srtBlock = QString("%1\n%2 --> %3\n%4\n\n")
            .arg(m_srtIndex)
            .arg(formatSrtTime(startMs))
            .arg(formatSrtTime(endMs))
            .arg(telemetryText);

        if (m_srtFileEO.isOpen()) {
            m_srtFileEO.write(srtBlock.toUtf8());
        }
        if (m_srtFileIR.isOpen()) {
            m_srtFileIR.write(srtBlock.toUtf8());
        }
        m_previousSrtMs = endMs;
        ++m_srtIndex;
    };

    // Produce the same 33/33/34 ms cadence as a 30 fps recording. If the UI
    // timer is delayed, write every missing interval so the SRT stays contiguous.
    qint64 nextFrameEndMs = (static_cast<qint64>(m_srtIndex) * 1000) / kSrtFrameRate;
    while (nextFrameEndMs <= recordingElapsedMs) {
        writeBlock(m_previousSrtMs, nextFrameEndMs);
        nextFrameEndMs = (static_cast<qint64>(m_srtIndex) * 1000) / kSrtFrameRate;
    }

    if (finalizePartialFrame && recordingElapsedMs > m_previousSrtMs) {
        writeBlock(m_previousSrtMs, recordingElapsedMs);
    }

    if (m_srtFileEO.isOpen()) {
        m_srtFileEO.flush();
    }
    if (m_srtFileIR.isOpen()) {
        m_srtFileIR.flush();
    }
}

void Widget::on_btnOpenNetworkVideo_clicked()
{
    m_VideoObjUSBCamera.Close();

    if (m_VideoObjNetworkEO.IsOpen())
    {
		if (m_bIsRecording) {
			stopRecording("stream_closed:EO");
		}
       m_VideoObjNetworkEO.Close();
       ui->widgetMainVideo->Clear();
       ui->btnOpenNetworkVideo->setText(tr("Open"));
    }
    else
    {
       QString strURL_EO = ui->cmbNetworkVideoURL->currentText();
       m_VideoObjNetworkEO.Open(strURL_EO.toStdString(), ui->widgetMainVideo);
       ui->btnOpenNetworkVideo->setText(tr("Close"));
    }
}

void Widget::on_btnOpenNetworkVideoIR_clicked()
{
    m_VideoObjUSBCamera.Close();

    if (m_VideoObjNetworkIR.IsOpen())
    {
		if (m_bIsRecording) {
			stopRecording("stream_closed:IR");
		}
       m_VideoObjNetworkIR.Close();
       ui->widgetSubVideo->Clear();
       ui->btnOpenNetworkVideoIR->setText(tr("Open"));
    }
    else
    {
       QString strURL_IR = ui->cmbNetworkVideoURLIR->currentText();
       m_VideoObjNetworkIR.Open(strURL_IR.toStdString(), ui->widgetSubVideo);
       ui->btnOpenNetworkVideoIR->setText(tr("Close"));
    }
}

void Widget::on_btnOpenUSBVideo_clicked()
{
	if (m_bIsRecording) {
		stopRecording("video_source_changed");
	}
    m_VideoObjNetworkEO.Close();
    m_VideoObjNetworkIR.Close();

    if (m_VideoObjUSBCamera.IsOpen())
    {
        m_VideoObjUSBCamera.Close();
        ui->widgetMainVideo->Clear();
        ui->btnOpenUSBVideo->setText(tr("Open"));
    }
    else
    {
        QString strUSBCamera = ui->cmbUSBCamera->currentText();
        m_VideoObjUSBCamera.Open(strUSBCamera, ui->widgetMainVideo);
        ui->btnOpenUSBVideo->setText(tr("Close"));
    }
}


void Widget::on_btnUp_pressed()
{
    double dSpeedRate = (double)ui->SliderMoveSpeed->value() / (double)ui->SliderMoveSpeed->maximum();
    double dScaleSpeed = (double)VLK_MAX_PITCH_SPEED * dSpeedRate;
    VLK_Move(0, (short)dScaleSpeed);
}

void Widget::on_btnUp_released()
{
    VLK_Stop();
}

void Widget::on_btnLeft_pressed()
{
    double dSpeedRate = (double)ui->SliderMoveSpeed->value() / (double)ui->SliderMoveSpeed->maximum();
    double dScaleSpeed = (double)VLK_MAX_YAW_SPEED * dSpeedRate * -1;
    VLK_Move(dScaleSpeed, 0);
}

void Widget::on_btnLeft_released()
{
    VLK_Stop();
}

void Widget::on_btnHome_clicked()
{
    VLK_Home();
}

void Widget::on_btnRight_pressed()
{
    double dSpeedRate = (double)ui->SliderMoveSpeed->value() / (double)ui->SliderMoveSpeed->maximum();
    double dScaleSpeed = (double)VLK_MAX_YAW_SPEED * dSpeedRate;
    VLK_Move(dScaleSpeed, 0);
}

void Widget::on_btnRight_released()
{
    VLK_Stop();
}

void Widget::on_btnDown_pressed()
{
    double dSpeedRate = (double)ui->SliderMoveSpeed->value() / (double)ui->SliderMoveSpeed->maximum();
    double dScaleSpeed = (double)VLK_MAX_PITCH_SPEED * dSpeedRate * -1;
    VLK_Move(0, dScaleSpeed);
}

void Widget::on_btnDown_released()
{
    VLK_Stop();
}

void Widget::on_cmbImageSensor_activated(int index)
{
    VLK_IMAGE_TYPE emType = (VLK_IMAGE_TYPE)ui->cmbImageSensor->itemData(index).toInt();
    VLK_IR_COLOR emColor = (VLK_IR_COLOR)ui->cmbIRColor->itemData(ui->cmbIRColor->currentIndex()).toInt();
    VLK_SetImageColor(emType, ui->checkBoxPIP->isChecked() ? 1 : 0, emColor);
}

void Widget::on_cmbIRColor_activated(int index)
{
    VLK_IMAGE_TYPE emType = (VLK_IMAGE_TYPE)ui->cmbImageSensor->itemData(ui->cmbImageSensor->currentIndex()).toInt();
    VLK_IR_COLOR emColor = (VLK_IR_COLOR)ui->cmbIRColor->itemData(index).toInt();
    VLK_SetImageColor(emType, ui->checkBoxPIP->isChecked() ? 1 : 0, emColor);
}

void Widget::on_checkBoxPIP_clicked(bool checked)
{
    VLK_IMAGE_TYPE emType = (VLK_IMAGE_TYPE)ui->cmbImageSensor->itemData(ui->cmbImageSensor->currentIndex()).toInt();
    VLK_IR_COLOR emColor = (VLK_IR_COLOR)ui->cmbIRColor->itemData(ui->cmbIRColor->currentIndex()).toInt();
    VLK_SetImageColor(emType, checked ? 1 : 0, emColor);
}

void Widget::on_btnZoomIn_pressed()
{
    VLK_ZoomIn(4);
}

void Widget::on_btnZoomIn_released()
{
    VLK_StopZoom();
}

void Widget::on_btnZoomOut_pressed()
{
    VLK_ZoomOut(4);
}

void Widget::on_btnZoomOut_released()
{
    VLK_StopZoom();
}

void Widget::on_btnGimbalTakePhoto_clicked()
{
    // please make sure there is SD card in the Gimbal
    VLK_Photograph();
}

void Widget::on_btnStartRecord_clicked()
{
    if (m_bIsRecording) {
        return;
    }

	if (!m_VideoObjNetworkEO.IsStreamReady()
		|| !m_VideoObjNetworkIR.IsStreamReady()) {
		QMessageBox::warning(
			this,
			tr("Recording unavailable"),
			tr("Open both EO and IR streams and wait until both streams are ready before recording."));
		return;
	}

    QDir().mkdir("records");

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
    QString eoVideoPath = QString("records/eo_%1.mp4").arg(timestamp);
    QString irVideoPath = QString("records/ir_%1.mp4").arg(timestamp);
    QString eoSrtPath = QString("records/eo_%1.srt").arg(timestamp);
    QString irSrtPath = QString("records/ir_%1.srt").arg(timestamp);
    m_recordMetadataPath = QString("records/session_%1.json").arg(timestamp);
    m_recordSessionUtc = QDateTime::currentDateTimeUtc();
    m_recordElapsedTimer.start();
    const CVideoObjNetwork::RecordClock::time_point sessionStart =
        CVideoObjNetwork::RecordClock::now();

    bool eoOk = m_VideoObjNetworkEO.StartLocalRecord(eoVideoPath.toStdString(), sessionStart);
    bool irOk = m_VideoObjNetworkIR.StartLocalRecord(irVideoPath.toStdString(), sessionStart);

    if (!eoOk || !irOk) {
        if (eoOk) {
            m_VideoObjNetworkEO.StopLocalRecord();
        }
        if (irOk) {
            m_VideoObjNetworkIR.StopLocalRecord();
        }
        QFile::remove(eoVideoPath);
        QFile::remove(irVideoPath);
        m_recordElapsedTimer.invalidate();
        m_recordMetadataPath.clear();
        qWarning() << "Both EO and IR streams must be ready before recording.";
        return;
    }

    m_srtFileEO.setFileName(eoSrtPath);
    m_srtFileIR.setFileName(irSrtPath);
    const bool eoSrtOk = m_srtFileEO.open(QIODevice::WriteOnly | QIODevice::Text);
    const bool irSrtOk = m_srtFileIR.open(QIODevice::WriteOnly | QIODevice::Text);
    if (!eoSrtOk || !irSrtOk) {
        qWarning() << "Failed to open both SRT files:" << eoSrtPath << irSrtPath;
        m_VideoObjNetworkEO.StopLocalRecord();
        m_VideoObjNetworkIR.StopLocalRecord();
        if (m_srtFileEO.isOpen()) {
            m_srtFileEO.close();
        }
        if (m_srtFileIR.isOpen()) {
            m_srtFileIR.close();
        }
        QFile::remove(eoVideoPath);
        QFile::remove(irVideoPath);
        QFile::remove(eoSrtPath);
        QFile::remove(irSrtPath);
        m_recordElapsedTimer.invalidate();
        m_recordMetadataPath.clear();
        return;
    }

    m_srtIndex = 1;
    m_previousSrtMs = 0;
	m_recordStopReason = "recording";
    m_bIsRecording = true;
    m_srtTimer.start(33);

    ui->btnStartRecord->setText(tr("Recording..."));
    ui->btnStartRecord->setStyleSheet("QPushButton { color: red; font-weight: bold; }");
    ui->btnStartRecord->setEnabled(false);
    ui->btnStopRecord->setEnabled(true);

    qDebug() << "Started dual local recording to" << eoVideoPath << "and" << irVideoPath;

}

void Widget::on_btnStopRecord_clicked()
{
	stopRecording("user");
}

void Widget::stopRecording(const QString& reasonCode, const QString& warningMessage)
{
	if (!m_bIsRecording) {
		return;
	}

	m_srtTimer.stop();
	appendSrtTelemetry(true);
	m_bIsRecording = false;
	m_recordStopReason = reasonCode;

    ui->btnStartRecord->setText(tr("Start record"));
    ui->btnStartRecord->setStyleSheet("");
    ui->btnStartRecord->setEnabled(true);
    ui->btnStopRecord->setEnabled(false);

    m_VideoObjNetworkEO.StopLocalRecord();
    m_VideoObjNetworkIR.StopLocalRecord();
    writeRecordMetadata();

    if (m_srtFileEO.isOpen()) {
        m_srtFileEO.close();
    }
    if (m_srtFileIR.isOpen()) {
        m_srtFileIR.close();
    }

    qDebug() << "Stopped dual local recording.";

    m_recordElapsedTimer.invalidate();
    m_recordMetadataPath.clear();

	if (!warningMessage.isEmpty()) {
		QMessageBox::warning(this, tr("Recording stopped"), warningMessage);
	}
}

void Widget::writeRecordMetadata()
{
    if (m_recordMetadataPath.isEmpty()) {
        return;
    }

    const CVideoObjNetwork::RecordTimingInfo eo =
        m_VideoObjNetworkEO.GetRecordTimingInfo();
    const CVideoObjNetwork::RecordTimingInfo ir =
        m_VideoObjNetworkIR.GetRecordTimingInfo();

    const auto timingToJson = [](const CVideoObjNetwork::RecordTimingInfo& timing) {
        QJsonObject result;
        result["first_keyframe_recorded"] = timing.hasFirstKeyframe;
        if (!timing.hasFirstKeyframe) {
            result["first_keyframe_offset_ms"] = QJsonValue(QJsonValue::Null);
            return result;
        }

        result["first_keyframe_offset_ms"] =
            static_cast<double>(timing.firstKeyframeOffsetMs);
        result["source_pts"] = timing.sourcePts == AV_NOPTS_VALUE
            ? QJsonValue(QJsonValue::Null)
            : QJsonValue(static_cast<double>(timing.sourcePts));
        result["source_dts"] = timing.sourceDts == AV_NOPTS_VALUE
            ? QJsonValue(QJsonValue::Null)
            : QJsonValue(static_cast<double>(timing.sourceDts));
        QJsonObject timeBase;
        timeBase["num"] = timing.timeBaseNum;
        timeBase["den"] = timing.timeBaseDen;
        result["source_time_base"] = timeBase;
        return result;
    };

    QJsonObject root;
    root["session_start_utc"] = m_recordSessionUtc.toString(Qt::ISODateWithMs);
    root["session_end_utc"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    root["session_elapsed_ms"] = m_recordElapsedTimer.isValid()
        ? QJsonValue(static_cast<double>(m_recordElapsedTimer.elapsed()))
        : QJsonValue(QJsonValue::Null);
	root["stop_reason"] = m_recordStopReason;
    root["eo"] = timingToJson(eo);
    root["ir"] = timingToJson(ir);
    root["ir_minus_eo_first_keyframe_ms"] =
        eo.hasFirstKeyframe && ir.hasFirstKeyframe
        ? QJsonValue(static_cast<double>(
            ir.firstKeyframeOffsetMs - eo.firstKeyframeOffsetMs))
        : QJsonValue(QJsonValue::Null);
    root["timing_note"] =
        "Offsets use host packet-arrival time and do not prove sensor-level synchronization.";

    QSaveFile metadataFile(m_recordMetadataPath);
    if (!metadataFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open recording metadata:" << m_recordMetadataPath;
        return;
    }
    metadataFile.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!metadataFile.commit()) {
        qWarning() << "Failed to save recording metadata:" << m_recordMetadataPath;
    }
}
