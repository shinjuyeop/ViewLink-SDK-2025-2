package viewpro.com.viewlink;

import android.view.TextureView;

public class ViewLinkJNI {

    private int outputBufferIndex;

    public native static int Init();

    public native static void UnInit();

    // Network Video
    // 视频流状态枚举，其值与VideoObiNetwork.h中的VideoStreamStatu对应
    public final static int VSS_OpenDemux        = 0;
    public final static int VSS_FindStream       = 1;
    public final static int VSS_ParsStreamParam  = 2;
    public final static int VSS_OpenDecoder      = 3;
    public final static int VSS_Play             = 4;
    public final static int VSS_CloseDecoder     = 5;
    public final static int VSS_CloseDemux       = 6;
    public final static int VSS_OpenMux          = 7;
    public final static int VSS_CloseMux         = 8;
    public final static int VSS_SaveLocalCapture = 9;
    public final static int VSS_SaveLocalRecord  = 10;

    private boolean isFirst=true;

    // 视频流错误码枚举，其值与VideoObiNetwork.h中的VideoStreamError对应
    public final static int         VSE_NoError = 0;
    public final static int         VSE_Waiting = 1;
    public final static int         VSE_Failed  = 2;
    private             TextureView mTextureView;


    // 本地抓拍, 仅支持jpg格式, 参数为全路径, eg: /sdcard/abc.jpg
    public native static boolean LocalCapture(String strFullPath);

    // 开始本地录像，仅支持mp4格式, 参数为全路径, eg: /sdcard/def.mp4
    public native static boolean StartLocalRecord(String strFullPath);


    // 视频暂停，继续播放
    public native static void setPause(boolean isPause);


    // 当前是否正在录像
    public native static boolean IsLocalRecording();


    //当前是否正在录像（硬解）
    public native boolean IsMediaCodecRecording();


    // 结束本地录像
    public native static void StopLocalRecording();

    // Gimbal connection
    public native static int Connect(String strIP, int iPort, String strUdpIP, int iUdpPort,
                                     Object target, String strConnStatusCallback, long lUserParam
            , boolean type);

    public native static boolean IsConnected();

    public native static void Disconnect();

    // Gimbal status notify
    public native static void RegisterDevStatusCB(Object target, String strDevStatusCallback,
                                                  long lUserParam);

    // Gimbal control
    public native static void Move(short sHorizontalSpeed, short sVeritcalSpeed);

    public native static void Stop();

    public native static void ZoomIn(short sSpeed);

    public native static void ZoomOut(short sSpeed);

    public native static void StopZoom();

    public native static void EnableTrackMode(int iTrackTempSize, int iTrackSensor);

    public native static void TrackTargetPosition(int iX, int iY, int iVideoWidth,
                                                  int iVideoHeight);

    public native static void TrackTargetPositionEx(int iTrackTempSize, int iTrackSensor, int iX,
                                                    int iY, int iVideoWidth, int iVideoHeight);

    public native static void DisableTrackMode();

    public native static void FocusIn(short sSpeed);

    public native static void FocusOut(short sSpeed);

    public native static void StopFocus();

    public native static void Home();

    public native static void Pitch90();

    public native static void SwitchMotor(int iOn);

    public native static boolean IsMotorOn();

    public native static void EnableFollowMode(int iEnable);

    //跟随状态
    public native static boolean IsFollowMode();

    public native static void TurnTo(double dYaw, double dPitch);

    public native static void SetGimbalCalibration(int typeValue, double lat, double lng,
                                                   double altValue);

    public native static void SetGimbalCalibrationUp(double stepValue);

    public native static void SetGimbalCalibrationDown(double stepValue);

    public native static void SetGimbalCalibrationLeft(double stepValue);

    public native static void SetGimbalCalibrationRight(double stepValue);


    public native static void SetGimbalCalibrationT1(double lat, double lng, double lat1,
                                                     double lng1);

    public native static void PointCamereHere(double lat, double lng, double alt);


    public native static void StartCarRecognition();
    public native static void StopCarRecognition();
    public native static void StartCarTrack();
    public native static void StartTrack();

    public native static void StopTrack();

    public native static boolean IsTracking();

    public native static void SetTrackTemplateSize(int iTrackTempSize);

    public native static void SetImageColor(int emImgType, int iEnablePIP, int emIRColor);

    public native static void Photograph();

    public native static void SwitchRecord(int iOn);

    //SD卡录像状态
    public native static boolean IsRecording();

    public native static void SwitchDefog(int iOn);

    public native static boolean IsDefogOn();

    public native static void SetRecordMode(int emMode);

    public native static int GetRecordMode();

    public native static void SetFocusMode(int emMode);

    public native static int GetFocusMode();

    public native static void ZoomTo(float fMag);

    public native static void IRDigitalZoomIn(short sSpeed);

    public native static void IRDigitalZoomOut(short sSpeed);

    public native static void SwitchEODigitalZoom(int iOn);

    public native static void QueryDevConfiguration();

    public native static void SetOSD(boolean newOsdVersion,boolean newOsdVersionLeft,String strJsonParam);

    public native static void SetTimeZone(double iTimeZone);

    public native static void SetWirelessCtrlChnlMap(String strJsonParam);

    public native static void SetWirelessCtrlChnlMapV2(String strJsonChannelsMap,
                                                       String strJsonInvertFlag);

    public native static void SendExtentCmd(byte[] byteCmd, int iLength);

    // 激光控制相关接口
    public native static void SwitchLaser(int iOn);

    public native static void LaserZoomIn(short sSpeed);

    public native static void LaserZoomOut(short sSpeed);

    public native static void LaserStopZoom();

    public native static void SetLaserZoomMode(int iMode);

    // 网络编码版控制相关接口
    public native static void QueryEncoderStatus();

    public native static void QueryEncoderFirmwareVersion();

    public native static void QueryEncoderConfiguration();

    public native static void SetEncoderConfiguration(String strJsonParam);

    public native static void SetEncoderNetworkConfig(String strJsonParam);

    public native static void SetEncoderBandwidth(int iBandwith);

    public native static void SetEncoderDHCP(int iOn);


    // test video data callback
    public native static void VideoStreamCallback(int iEncodeType, int iWidth, int iHeight,
                                                  byte[] byteData, long lLen, long lPTS,
                                                  long lUserParam);

    // test device connection status callback
    public native static void DevConnStatusCallback(int iConnStatus, String strMsg,
                                                    long lUserParam);

    // test device status callback
    public native static void DevStatusCallback(int iType, String strJson, long lUserParam);



}
