#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QTimer>
#include "ViewLink.h"
#include "VideoObjNetwork.h"
#include "VideoObjUSBCamera.h"

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

private slots:
    void on_btnConnectTCP_clicked();

    void on_btnConnctSerialPort_clicked();

private:
    static int VLK_ConnStatusCallback(int iConnStatus, const char* szMessage, int iMsgLen, void* pUserParam);
    static int VLK_DevStatusCallback(int iType, const char* szBuffer, int iBufLen, void* pUserParam);
signals:
    void SignalConnectionStatus(int iConnStatus, const QString& strMessage);
    void SignalDeviceModel(VLK_DEV_MODEL model);
    void SignalDeviceConfig(VLK_DEV_CONFIG config);
    void SignalDeviceTelemetry(VLK_DEV_TELEMETRY telemetry);
private slots:
    void onSlotConnectionStatus(int iConnStatus, const QString& strMessage);
    void onSlotDeviceModel(VLK_DEV_MODEL model);
    void onSlotDeviceConfig(VLK_DEV_CONFIG config);
    void onSlotDeviceTelemetry(VLK_DEV_TELEMETRY telemetry);
    void writeSrtTelemetry();
    void on_btnUp_pressed();

    void on_btnUp_released();

    void on_btnLeft_pressed();

    void on_btnLeft_released();

    void on_btnHome_clicked();

    void on_btnRight_pressed();

    void on_btnRight_released();

    void on_btnDown_pressed();

    void on_btnDown_released();

    void on_cmbImageSensor_activated(int index);

    void on_cmbIRColor_activated(int index);

    void on_checkBoxPIP_clicked(bool checked);
    void on_btnOpenNetworkVideo_clicked();
    void on_btnOpenNetworkVideoIR_clicked();

    void on_btnOpenUSBVideo_clicked();

    void on_btnZoomIn_pressed();

    void on_btnZoomIn_released();

    void on_btnZoomOut_pressed();

    void on_btnZoomOut_released();

    void on_btnGimbalTakePhoto_clicked();

    void on_btnStartRecord_clicked();

    void on_btnStopRecord_clicked();

    void on_btnTurnTo_clicked();

    void on_btnMotorOn_clicked();

    void on_btnMotorOff_clicked();

    void onTurnToPresetClicked();

    void onTurnToMonitorTimeout();

private:
    // initialize UI control
    void InitUI();
	void appendSrtTelemetry(bool finalizePartialFrame = false);
	void stopRecording(const QString& reasonCode, const QString& warningMessage = QString());
    void writeRecordMetadata();
    bool hasFreshTelemetry() const;
    void updateTurnToDeviceStatus();
    void appendTurnToStatus(const QString& message);
    void finishTurnToTest(const QString& result);
    static double yawAngularError(double first, double second);

private:
    Ui::Widget *ui;

    CVideoObjNetwork m_VideoObjNetworkEO;
    CVideoObjNetwork m_VideoObjNetworkIR;
    CVideoObjUSBCamera m_VideoObjUSBCamera;

    // Recording and SRT variables
    bool m_bIsRecording;
    bool m_deviceConnected;
    bool m_hasTelemetry;
    VLK_DEV_TELEMETRY m_latestTelemetry;
    QElapsedTimer m_lastTelemetryTimer;
    QElapsedTimer m_recordElapsedTimer;
    qint64 m_previousSrtMs;
    QDateTime m_recordSessionUtc;
    QString m_recordMetadataPath;
	QString m_recordStopReason;
    QTimer m_srtTimer;
    QFile m_srtFileEO;
    QFile m_srtFileIR;
    int m_srtIndex;

    // VLK_TurnTo diagnostic state
    QTimer m_turnToTimer;
    QElapsedTimer m_turnToElapsedTimer;
    bool m_turnToActive;
    bool m_turnToMotorOn;
    bool m_turnToBeforeTelemetryAvailable;
    bool m_turnToTelemetryReceived;
    bool m_turnToYawMoved;
    bool m_turnToPitchMoved;
    bool m_turnToMovedAwayAfterReach;
    double m_turnToTargetYaw;
    double m_turnToTargetPitch;
    double m_turnToBeforeYaw;
    double m_turnToBeforePitch;
    double m_turnToAfterYaw;
    double m_turnToAfterPitch;
    double m_turnToBestCombinedError;
    qint64 m_turnToFirstWithinToleranceMs;
    qint64 m_turnToLastDeviceStatusPollMs;
};

#endif // WIDGET_H
