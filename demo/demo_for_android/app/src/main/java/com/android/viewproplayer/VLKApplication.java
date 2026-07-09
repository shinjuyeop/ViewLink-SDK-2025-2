package com.android.viewproplayer;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import viewpro.com.viewlink.ViewLinkJNI;

/**
 * 创建者     DreamSky
 * 创建时间   2023/4/19 16:56
 * 描述
 * 更新者     $
 * 更新时间   $
 * 更新描述
 */
public class VLKApplication extends Application {
    private static VLKApplication instance;

    private final static String                TAG = "VLKApplication";
    public final static  String                BROADCAST_CONNECTION_STATUS = "BroadcastConnectionStatus";
    public final static  String                BROADCAST_DEVICE_STATUS = "BroadcastDeviceStatus";
    public final static  String                BROADCAST_VIDEO_STREAM_STATUS = "BroadcastVideoStreamStatus";
    public static        int                   s_iMoveSpeed = 10;
    private              Context               context;
    static {
        try {
            System.loadLibrary("ViewLink");
            System.loadLibrary("native-lib");
        } catch (UnsatisfiedLinkError e) {
            e.printStackTrace();
            Log.e(TAG, "load library failed!");
        }
    }

    @Override
    public void onCreate() {
        context = getApplicationContext();
        super.onCreate();
        instance = this;

        if (0 != ViewLinkJNI.Init()) {
            Log.e(TAG, "ViewLinkJNI.Init() failed!");
            return;
        }

        ViewLinkJNI.RegisterDevStatusCB(this, "DevStatusCallback", 0);


    }



    @Override
    public void onTerminate() {
        super.onTerminate();

        ViewLinkJNI.UnInit();
    }

    public void DevConnStatusCallback(int iConnStatus, String strMsg, long lUserParam) {
        // Log.e(TAG, "DevConnStatusCallback iConnStatus: " + iConnStatus);
        Intent intent = new Intent(BROADCAST_CONNECTION_STATUS);
        intent.putExtra("ConnStatus", iConnStatus);
        intent.putExtra("Msg", strMsg);
        sendBroadcast(intent);
    }

    public void DevStatusCallback(int iType, int iErrorCode, String strJson, long lUserParam) {
        Intent intent = new Intent(BROADCAST_DEVICE_STATUS);
        intent.putExtra("Type", iType);
        intent.putExtra("Error", iErrorCode);
        intent.putExtra("Json", strJson);
        sendBroadcast(intent);
    }

    public void VideoStreamStatusCallback(int iVideoStreamStatus, int iErrorCode, String strMsg, long lUserParam) {
        // Log.d(TAG, "VideoStreamStatusCallback iVideoStreamStatus: " + iVideoStreamStatus + ", iErrorCode: " + iErrorCode);
        Intent intent = new Intent(BROADCAST_VIDEO_STREAM_STATUS);
        intent.putExtra("VideoStreamStatus", iVideoStreamStatus);
        intent.putExtra("iErrorCode", iErrorCode);
        intent.putExtra("Msg", strMsg);
        sendBroadcast(intent);
    }

    public static VLKApplication getInstance() {
        return instance;
    }

}