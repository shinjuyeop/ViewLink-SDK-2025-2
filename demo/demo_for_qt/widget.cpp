#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QCamera>
#include <QDir>
#include <QTimer>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    m_bIsRecording(false),
    m_srtIndex(1)
{
    ui->setupUi(this);
    qRegisterMetaType<VLK_DEV_MODEL>("VLK_DEV_MODEL");
    qRegisterMetaType<VLK_DEV_CONFIG>("VLK_DEV_CONFIG");
    qRegisterMetaType<VLK_DEV_TELEMETRY>("VLK_DEV_TELEMETRY");

    connect(this, SIGNAL(SignalConnectionStatus(int, const QString&)), this, SLOT(onSlotConnectionStatus(int, const QString&)));
    connect(this, SIGNAL(SignalDeviceModel(VLK_DEV_MODEL)), this, SLOT(onSlotDeviceModel(VLK_DEV_MODEL)));
    connect(this, SIGNAL(SignalDeviceConfig(VLK_DEV_CONFIG)), this, SLOT(onSlotDeviceConfig(VLK_DEV_CONFIG)));
    connect(this, SIGNAL(SignalDeviceTelemetry(VLK_DEV_TELEMETRY)), this, SLOT(onSlotDeviceTelemetry(VLK_DEV_TELEMETRY)));

    InitUI();

    VLK_RegisterDevStatusCB(VLK_DevStatusCallback, this);
}

Widget::~Widget()
{
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

    // initialize network url list
    ui->cmbNetworkVideoURL->addItem("rtsp://127.0.0.1:8554/live1");
    ui->cmbNetworkVideoURL->addItem("rtsp://192.168.2.119/live1");

    ui->cmbNetworkVideoURLIR->addItem("rtsp://127.0.0.1:8554/live2");
    ui->cmbNetworkVideoURLIR->addItem("rtsp://192.168.2.119/live2");

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
       qDebug() << "TCP Gimbal connected !!!";
       ui->btnConnectTCP->setText(tr("Disconnect"));
    }
    else if (iConnStatus == VLK_CONN_STATUS_TCP_DISCONNECTED)
    {
        qDebug() << "TCP Gimbal disconnected !!!";
        ui->btnConnectTCP->setText("Connect");
    }
    else if (iConnStatus == VLK_CONN_STATUS_SERIAL_PORT_CONNECTED)
    {
        qDebug() << "Serial port Gimbal connected !!!";
        ui->btnConnctSerialPort->setText(tr("Disconnect"));
    }
    else if (iConnStatus == VLK_CONN_STATUS_SERIAL_PORT_DISCONNECTED)
    {
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
    QString::number(telemetry.dPitch);
    ui->lbYaw->setText(QString::number(telemetry.dYaw));
    ui->lbPitch->setText(QString::number(telemetry.dPitch));
    ui->lbTargetLat->setText(QString::number(telemetry.dTargetLat));
    ui->lbTargetLng->setText(QString::number(telemetry.dTargetLng));
    ui->lbTargetAlt->setText(QString::number(telemetry.dTargetAlt));
    ui->lbZoom->setText(QString::number(telemetry.dZoomMagTimes));
    ui->lbLaserDistance->setText(QString::number(telemetry.sLaserDistance));

    // If recording, write subtitle data to both SRT files
    if (m_bIsRecording) {
        qint64 elapsedMs = m_recordStartTime.msecsTo(QDateTime::currentDateTime());
        
        // Format time stamps for SRT: HH:MM:SS,mmm
        qint64 ms = elapsedMs % 1000;
        qint64 secs = (elapsedMs / 1000) % 60;
        qint64 mins = (elapsedMs / 60000) % 60;
        qint64 hours = (elapsedMs / 3600000);

        QString timeStart = QString("%1:%2:%3,%4")
            .arg(hours, 2, 10, QChar('0'))
            .arg(mins, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'))
            .arg(ms, 3, 10, QChar('0'));

        qint64 elapsedEndMs = elapsedMs + 990;
        qint64 msEnd = elapsedEndMs % 1000;
        qint64 secsEnd = (elapsedEndMs / 1000) % 60;
        qint64 minsEnd = (elapsedEndMs / 60000) % 60;
        qint64 hoursEnd = (elapsedEndMs / 3600000);

        QString timeEnd = QString("%1:%2:%3,%4")
            .arg(hoursEnd, 2, 10, QChar('0'))
            .arg(minsEnd, 2, 10, QChar('0'))
            .arg(secsEnd, 2, 10, QChar('0'))
            .arg(msEnd, 3, 10, QChar('0'));

        QString srtBlock = QString("%1\n%2 --> %3\nGPS: %4, %5 (Alt: %6m)\nYaw: %7, Pitch: %8\nLaser Distance: %9m\n\n")
            .arg(m_srtIndex)
            .arg(timeStart)
            .arg(timeEnd)
            .arg(telemetry.dTargetLat, 0, 'f', 6)
            .arg(telemetry.dTargetLng, 0, 'f', 6)
            .arg(telemetry.dTargetAlt, 0, 'f', 1)
            .arg(telemetry.dYaw, 0, 'f', 1)
            .arg(telemetry.dPitch, 0, 'f', 1)
            .arg(telemetry.sLaserDistance);

        if (m_srtFileEO.isOpen()) {
            m_srtFileEO.write(srtBlock.toUtf8());
            m_srtFileEO.flush();
        }
        if (m_srtFileIR.isOpen()) {
            m_srtFileIR.write(srtBlock.toUtf8());
            m_srtFileIR.flush();
        }
        m_srtIndex++;
    }
}

void Widget::on_btnOpenNetworkVideo_clicked()
{
    m_VideoObjUSBCamera.Close();

    if (m_VideoObjNetworkEO.IsOpen())
    {
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

    QDir().mkdir("records");

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString eoVideoPath = QString("records/eo_%1.mp4").arg(timestamp);
    QString irVideoPath = QString("records/ir_%1.mp4").arg(timestamp);
    QString eoSrtPath = QString("records/eo_%1.srt").arg(timestamp);
    QString irSrtPath = QString("records/ir_%1.srt").arg(timestamp);

    bool eoOk = m_VideoObjNetworkEO.StartLocalRecord(eoVideoPath.toStdString());
    bool irOk = m_VideoObjNetworkIR.StartLocalRecord(irVideoPath.toStdString());

    if (!eoOk && !irOk) {
        qWarning() << "Failed to start local recording for both streams.";
        return;
    }

    if (eoOk) {
        m_srtFileEO.setFileName(eoSrtPath);
        if (!m_srtFileEO.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "Failed to open EO SRT file for writing:" << eoSrtPath;
        }
    }

    if (irOk) {
        m_srtFileIR.setFileName(irSrtPath);
        if (!m_srtFileIR.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "Failed to open IR SRT file for writing:" << irSrtPath;
        }
    }

    m_recordStartTime = QDateTime::currentDateTime();
    m_srtIndex = 1;
    m_bIsRecording = true;

    ui->btnStartRecord->setText(tr("Recording..."));
    ui->btnStartRecord->setStyleSheet("QPushButton { color: red; font-weight: bold; }");
    ui->btnStartRecord->setEnabled(false);
    ui->btnStopRecord->setEnabled(true);

    qDebug() << "Started dual local recording to" << eoVideoPath << "and" << irVideoPath;

    // Trigger gimbal onboard recording as well if desired (optional)
    VLK_SwitchRecord(1);
}

void Widget::on_btnStopRecord_clicked()
{
    if (!m_bIsRecording) {
        return;
    }

    m_bIsRecording = false;

    ui->btnStartRecord->setText(tr("Start record"));
    ui->btnStartRecord->setStyleSheet("");
    ui->btnStartRecord->setEnabled(true);
    ui->btnStopRecord->setEnabled(false);

    m_VideoObjNetworkEO.StopLocalRecord();
    m_VideoObjNetworkIR.StopLocalRecord();

    if (m_srtFileEO.isOpen()) {
        m_srtFileEO.close();
    }
    if (m_srtFileIR.isOpen()) {
        m_srtFileIR.close();
    }

    qDebug() << "Stopped dual local recording.";

    // Trigger gimbal onboard recording stop
    VLK_SwitchRecord(0);
}
