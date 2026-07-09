package com.android.viewproplayer;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.NonNull;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.RadioButton;
import android.widget.TextView;
import android.widget.Toast;

import org.json.JSONException;
import org.json.JSONObject;

import java.lang.ref.WeakReference;
import java.util.ArrayList;

import viewpro.com.viewlink.ViewLinkJNI;


public class MainActivity extends Activity {


    private     RTVideoActBroadcastReceiver m_Receiver;
    private Handler                     m_Handler = new DeviceStatusHandler(this);
    public final static  int                HANDLER_MSG_VIDEO_STREAM_STATUS             = 0;
    public final static  int                HANDLER_MSG_DEV_MODEL                       = 1;
    public final static  int                HANDLER_MSG_DEV_CONFIG                      = 2;
    public final static  int                HANDLER_MSG_DEV_TELEMETRY                   = 3;
    public final static  int                HANDLER_MSG_DEV_IP_ENCODER_GET_VIDEO_IO_RES = 4;
    public final static  int                HANDLER_MSG_DEV_IP_ENCODER_GET_CFG_RES      = 5;
    public final static  int                HANDLER_MSG_DEV_IP_ENCODER_SET_CFG_RES      = 6;
    public final static  int                HANDLER_MSG_DEV_IP_ENCODER_GET_VERSION_RES  = 7;
    public final static  int                HANDLER_MSG_DEV_IP_ENCODER_SET_NETWORK_RES  = 8;
    public final static  int                HANDLER_MSG_DEV_IP_ENCODER_SET_BANDWITH_RES = 9;
    public final static  int                HANDLER_MSG_DEV_IP_ENCODER_SET_DHCP_RES     = 10;
    private static double         dYaw   = 0.0;
    private static double         dPitch = 0.0;
    public               DeviceCapabilities mDeviceCapabilities                         =
            new DeviceCapabilities(this);
    private String strDeviceID ="VQ10T";
    private String VersionNO;
    private     int      iSensorType;
    private TextView tv_zoom,tv_pitch,tv_yaw,tv_deviceID,tv_versionNO;
    private     IntentFilter         m_intentFilter;
    private RecyclerView         mRecycler;
    private     RecyclerView.Adapter mAdapter;
    private ArrayList<OsdBean> osdBeans = new ArrayList<>();
    private RadioButton        m_osd_fov;
    private       RadioButton          m_osd_mgrs;
    private       RadioButton          m_osd_enlarge;
    private       RadioButton          m_osd_gps;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);


        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);

        m_intentFilter = new IntentFilter();
        m_intentFilter.addAction(VLKApplication.BROADCAST_CONNECTION_STATUS);
        m_intentFilter.addAction(VLKApplication.BROADCAST_DEVICE_STATUS);
        m_Receiver = new RTVideoActBroadcastReceiver();
        registerReceiver(m_Receiver, m_intentFilter);
        tv_zoom = findViewById(R.id.tv_zoom);
        tv_pitch = findViewById(R.id.tv_pitch);
        tv_yaw = findViewById(R.id.tv_yaw);
        tv_deviceID = findViewById(R.id.tv_deviceID);
        tv_versionNO = findViewById(R.id.tv_versionNO);

        m_osd_fov = findViewById(R.id.osd_fov);
        m_osd_mgrs = findViewById(R.id.osd_mgrs);
        m_osd_enlarge = findViewById(R.id.osd_enlarge);
        m_osd_gps = findViewById(R.id.osd_gps);

        mRecycler =findViewById(R.id.osd_rv);
        mRecycler.setLayoutManager(new GridLayoutManager(this, 2));
        mRecycler.addItemDecoration(new SpacesItemDecoration(5));


        if (strDeviceID.equals("VA40T") || strDeviceID.equals("VA30TR")|| strDeviceID.equals("VH30T")) {
            osdBeans.add(new OsdBean(getResources().getString(R.string.Enable_OSD)));
            osdBeans.add(new OsdBean(getResources().getString(R.string.Cross)));
            osdBeans.add(new OsdBean(getResources().getString(R.string.Pitch_Yaw)));
            osdBeans.add(new OsdBean(getResources().getString(R.string.XYshift)));
            osdBeans.add(new OsdBean(getResources().getString(R.string.GPS)));
            osdBeans.add(new OsdBean(getResources().getString(R.string.Time)));
            osdBeans.add(new OsdBean(getResources().getString(R.string.VL_MAG)));
            osdBeans.add(new OsdBean(getResources().getString(R.string.Ai_Frame)));

            osdBeans.add(new OsdBean(getResources().getString(R.string.IR_Dzoom)));
            osdBeans.add(new OsdBean(getResources().getString(R.string.LRF)));
            osdBeans.add(new OsdBean(getResources().getString(R.string.TF)));
            osdBeans.add(new OsdBean(getResources().getString(R.string.GPS_TAG)));


            m_osd_enlarge.setText(R.string.green_font);

            m_osd_fov.setText(R.string.white_font);
        } else {
            osdBeans.add(new OsdBean(getResources().getString(R.string.Enable_OSD)));
            osdBeans.add(new OsdBean(getResources().getString(R.string.Cross)));
            osdBeans.add(new OsdBean(getResources().getString(R.string.Pitch_Yaw)));
            osdBeans.add(new OsdBean(getResources().getString(R.string.XYshift)));
            osdBeans.add(new OsdBean(getResources().getString(R.string.GPS)));
            osdBeans.add(new OsdBean(getResources().getString(R.string.Time)));
            osdBeans.add(new OsdBean(getResources().getString(R.string.VL_MAG)));
            osdBeans.add(new OsdBean(getResources().getString(R.string.Use_Big_Font)));

            osdBeans.add(new OsdBean(getResources().getString(R.string.Time_Input)));
            osdBeans.add(new OsdBean(getResources().getString(R.string.GPS_Input)));
            osdBeans.add(new OsdBean(getResources().getString(R.string.Pitch_Yaw_Input)));
            osdBeans.add(new OsdBean(getResources().getString(R.string.VL_MAG_Inpu)));

        }


        mAdapter = new RecyclerView.Adapter<OsdHolder>() {
            @NonNull
            @Override
            public OsdHolder onCreateViewHolder(@NonNull ViewGroup viewGroup, int i) {
                return new OsdHolder(View.inflate(viewGroup.getContext(), R.layout.item_osd, null));
            }

            @Override
            public void onBindViewHolder(@NonNull OsdHolder osdHolder, final int i) {
                osdHolder.mTitle.setText(osdBeans.get(i).title);
                osdHolder.mSwitch.setChecked(osdBeans.get(i).isChecked);
                osdHolder.mSwitch.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        osdBeans.get(i).isChecked = !osdBeans.get(i).isChecked;
                        String strJson = getFromUI(true);

                        if (strDeviceID.equals("VA40T") || strDeviceID.equals("VA30TR") || strDeviceID.equals("A40T")|| strDeviceID.equals("VH30T")) {
                            if (osdBeans.get(i).title.equals(getString(R.string.Enable_OSD)) || osdBeans.get(i).title.equals(getString(R.string.Cross)) || osdBeans.get(i).title.equals(getString(R.string.Pitch_Yaw))
                                    || osdBeans.get(i).title.equals(getString(R.string.XYshift)) || osdBeans.get(i).title.equals(getString(R.string.GPS)) || osdBeans.get(i).title.equals(getString(R.string.Time))
                                    || osdBeans.get(i).title.equals(getString(R.string.VL_MAG)) || osdBeans.get(i).title.equals(getString(R.string.Ai_Frame))) {
                                ViewLinkJNI.SetOSD(true, true, strJson);
                            } else {
                                ViewLinkJNI.SetOSD(true, false, strJson);
                            }

                        } else {
                            ViewLinkJNI.SetOSD(false, false, strJson);
                        }


                    }
                });
            }

            @Override
            public int getItemCount() {

                return osdBeans.size();
            }
        };
        mRecycler.setAdapter(mAdapter);
    }

    public void QueryDevConfiguration(View view) {
        ViewLinkJNI.QueryDevConfiguration();
    }

    public void PointCameraHere(View view) {
        ViewLinkJNI.PointCamereHere(10.0,10.0,1);
    }


    public class RTVideoActBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context arg0, Intent arg1) {

            // TODO Auto-generated method stub
            if (VLKApplication.BROADCAST_CONNECTION_STATUS == arg1.getAction()) {

                int iConnStatus = arg1.getIntExtra("ConnStatus", 0);
                if (iConnStatus == 2) // TCP connected
                {
                    Toast.makeText(MainActivity.this, R.string.tcp_connected,
                            Toast.LENGTH_SHORT).show();


                } else if (iConnStatus == 3) // TCP disconnected
                {
                    Toast.makeText(MainActivity.this, R.string.tcp_disconnected,
                            Toast.LENGTH_SHORT).show();

                } else if (iConnStatus == 4) // serial port connected
                {
                    Toast.makeText(MainActivity.this, R.string.serial_port_connected,
                            Toast.LENGTH_SHORT).show();
                } else if (iConnStatus == 5) // serial port disconnected
                {
                    Toast.makeText(MainActivity.this, R.string.serial_port_disconnected,
                            Toast.LENGTH_SHORT).show();
                } else {
                    Toast.makeText(MainActivity.this,
                            R.string.unknown_conn_status + iConnStatus, Toast.LENGTH_SHORT).show();
                }
            } else if (VLKApplication.BROADCAST_DEVICE_STATUS == arg1.getAction()) {
                int iType = arg1.getIntExtra("Type", 0);
                int iError = arg1.getIntExtra("Error", 0);
                String strJson = arg1.getStringExtra("Json");

                if (iType == 0) // VLK_DEV_STATUS_TYPE_MODEL
                {

                    Message msg = Message.obtain();
                    msg.what = HANDLER_MSG_DEV_MODEL;
                    msg.obj = strJson;
                    m_Handler.sendMessage(msg);
                } else if (iType == 1) // VLK_DEV_STATUS_TYPE_CONFIG
                {
                    // Log.d(TAG, "DevStatusCallback iType: VLK_DEV_STATUS_TYPE_CONFIG");
                    Message msg = Message.obtain();
                    msg.what = HANDLER_MSG_DEV_CONFIG;
                    msg.obj = strJson;
                    m_Handler.sendMessage(msg);
                } else if (iType == 2) // VLK_DEV_STATUS_TYPE_TELEMETRY
                {
                    // Log.d(TAG, "DevStatusCallback iType: VLK_DEV_STATUS_TYPE_TELEMETRY");
                    Message msg = Message.obtain();
                    msg.what = HANDLER_MSG_DEV_TELEMETRY;
                    msg.obj = strJson;
                    m_Handler.sendMessage(msg);
                } else if (iType == 3) // VLK_DEV_STATUS_TYPE_ENCODER_VIDEO_IO_RES
                {
                    Message msg = Message.obtain();
                    msg.what = HANDLER_MSG_DEV_IP_ENCODER_GET_VIDEO_IO_RES;
                    msg.arg1 = iError;
                    msg.obj = strJson;
                    m_Handler.sendMessage(msg);
                } else if (iType == 4) // VLK_DEV_STATUS_TYPE_ENCODER_GET_CFG_RES
                {
                    Message msg = Message.obtain();
                    msg.what = HANDLER_MSG_DEV_IP_ENCODER_GET_CFG_RES;
                    msg.arg1 = iError;
                    msg.obj = strJson;
                    m_Handler.sendMessage(msg);
                } else if (iType == 5) // VLK_DEV_STATUS_TYPE_ENCODER_SET_CFG
                {
                    Message msg = Message.obtain();
                    msg.what = HANDLER_MSG_DEV_IP_ENCODER_SET_CFG_RES;
                    msg.arg1 = iError;
                    msg.obj = strJson;

                    m_Handler.sendMessage(msg);
                } else if (iType == 6) // VLK_DEV_STATUS_TYPE_ENCODER_GET_VERSION_RES
                {
                    Message msg = Message.obtain();
                    msg.what = HANDLER_MSG_DEV_IP_ENCODER_GET_VERSION_RES;
                    msg.arg1 = iError;
                    msg.obj = strJson;
                    m_Handler.sendMessage(msg);
                } else if (iType == 7) // VLK_DEV_STATUS_TYPE_ENCODER_SET_NETWORK_RES
                {
                    Message msg = Message.obtain();
                    msg.what = HANDLER_MSG_DEV_IP_ENCODER_SET_NETWORK_RES;
                    msg.arg1 = iError;
                    msg.obj = strJson;

                    m_Handler.sendMessage(msg);
                } else if (iType == 8) // VLK_DEV_STATUS_TYPE_ENCODER_SET_BANDWITH_RES
                {
                    Message msg = Message.obtain();
                    msg.what = HANDLER_MSG_DEV_IP_ENCODER_SET_BANDWITH_RES;
                    msg.obj = strJson;
                    m_Handler.sendMessage(msg);
                } else if (iType == 9) // VLK_DEV_STATUS_TYPE_ENCODER_SET_DHCP_RES
                {
                    Message msg = Message.obtain();
                    msg.what = HANDLER_MSG_DEV_IP_ENCODER_SET_DHCP_RES;
                    msg.obj = strJson;
                    m_Handler.sendMessage(msg);
                }

            }
        }
    }


    public void Home(View view) {
        ViewLinkJNI.Home();
    }


    @Override
    protected void onDestroy() {
        super.onDestroy();

        unregisterReceiver(m_Receiver);
        ViewLinkJNI.Disconnect();
        m_Handler.removeCallbacksAndMessages(null);


        this.finish();


    }


    private static class DeviceStatusHandler extends Handler {
        private final WeakReference<MainActivity> m_Target;
        private       boolean                        ISFIRSTLOADING = true;
        private       boolean                        ISFIRST        = true;
        private       boolean                        isModelCode    = true;



        public DeviceStatusHandler(MainActivity target) {

            m_Target = new WeakReference<MainActivity>(target);
        }

        @Override
        public void handleMessage(android.os.Message msg) {
            MainActivity targetAct = m_Target.get();
            if (null == targetAct) {
                return;
            }
            switch (msg.what) {

                case HANDLER_MSG_DEV_MODEL: {
                    String strJson = (String) msg.obj;
                    try {

                        JSONObject jsonObj = new JSONObject(strJson);
                        int iModelCode = jsonObj.getInt("ModelCode");
                        String strModelName = jsonObj.getString("ModelName");
                        if (!targetAct.mDeviceCapabilities.LoadCapByDeviceType(iModelCode)) {
                            Log.d("DreamSky", "load device capabilities failed !");
                        }


                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                }
                break;
                case HANDLER_MSG_DEV_CONFIG: {
                    String strJson = (String) msg.obj;

                    try {
                        JSONObject jsonObj = new JSONObject(strJson);
                        targetAct.strDeviceID = jsonObj.getString("DeviceID");
                        targetAct.VersionNO = jsonObj.getString("VersionNO");
                        JSONObject jsonOSDConfig = jsonObj.getJSONObject("OSDConfig");
                        String strTimeZone = jsonObj.getString("TimeZone");




                        JSONObject jsonChnlsMap = jsonObj.getJSONObject("ChnlsMap");
                        JSONObject jsonChnlsInvertFlag = jsonObj.getJSONObject("ChnlsInvertFlag");
                        targetAct.tv_deviceID.setText("DeviceID:"+targetAct.strDeviceID);
                        targetAct.tv_versionNO.setText("VersionNO:"+targetAct.VersionNO);



                        try {
                            JSONObject jsonObjOSD = new JSONObject(jsonOSDConfig.toString());

                            boolean bEnableOSD = jsonObjOSD.getInt("EnableOSD") != 0;
                            targetAct.osdBeans.get(0).isChecked = bEnableOSD;

                            boolean bCross = jsonObjOSD.getInt("Cross") != 0;
                            targetAct.osdBeans.get(1).isChecked = bCross;

                            boolean bPitchYaw = jsonObjOSD.getInt("PitchYaw") != 0;
                            targetAct.osdBeans.get(2).isChecked = bPitchYaw;

                            boolean bXYShift = jsonObjOSD.getInt("XYShift") != 0;
                            targetAct.osdBeans.get(3).isChecked = bXYShift;

                            boolean bGPS = jsonObjOSD.getInt("GPS") != 0;
                            targetAct.osdBeans.get(4).isChecked = bGPS;

                            boolean bTime = jsonObjOSD.getInt("Time") != 0;
                            targetAct.osdBeans.get(5).isChecked = bTime;

                            boolean bVLMAG = jsonObjOSD.getInt("VL-MAG") != 0;


                            Log.d("VLMAG", "刷新状态--VLMAG:" + bVLMAG);

                            boolean bBigFont = jsonObjOSD.getInt("BigFont") != 0;
                            targetAct.osdBeans.get(7).isChecked = bBigFont;


                            // boolean bInputSave = jsonObj.getBoolean("InputSave");

                            boolean bInputTime = jsonObjOSD.getInt("InputTime") != 0;

                            targetAct.osdBeans.get(8).isChecked = bInputTime;

                            boolean bInputGPS = jsonObjOSD.getInt("InputGPS") != 0;
                            targetAct.osdBeans.get(9).isChecked = bInputGPS;

                            boolean bInputPitchYaw = jsonObjOSD.getInt("InputPitchYaw") != 0;
                            targetAct.osdBeans.get(10).isChecked = bInputPitchYaw;

                            boolean bInputVLMAG = jsonObjOSD.getInt("InputVL-MAG") != 0;
                            targetAct.osdBeans.get(11).isChecked = bInputVLMAG;

                            boolean bInputZoomTimesOrFOV = jsonObjOSD.getInt("InputZoomTimesOrFOV") != 0;
                            if (bInputZoomTimesOrFOV)
                                targetAct. m_osd_fov.setChecked(true);
                            else
                                targetAct.m_osd_enlarge.setChecked(true);


                            boolean bInputMGRS = jsonObjOSD.getInt("InputMGRS") != 0;
                            if (bInputMGRS)
                                targetAct.m_osd_mgrs.setChecked(true);
                            else
                                targetAct.m_osd_gps.setChecked(true);

                            // boolean bInputCharBlackBorder = jsonObj.getBoolean("InputCharBlackBorder");

                            targetAct.mAdapter.notifyDataSetChanged();
                        } catch (JSONException e) {
                            e.printStackTrace();
                        }



                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                }
                break;
                case HANDLER_MSG_DEV_TELEMETRY: {
                    String strJson = (String) msg.obj;
                    try {
                        JSONObject jsonObj = new JSONObject(strJson);
                        dYaw = jsonObj.getDouble("Yaw");


                        dPitch = jsonObj.getDouble("Pitch");

                        targetAct.tv_yaw.setText("Yaw:"+String.format("%1$.2f", dYaw));
                        targetAct.tv_pitch.setText("Pitch:"+String.format("%1$.2f", dPitch));
                        int iModelCode = jsonObj.getInt("ModelCode");
                        if (isModelCode) {
                            isModelCode = false;
                            if (!targetAct.mDeviceCapabilities.LoadCapByDeviceType(iModelCode)) {
                                Log.d("DreamSky", "load device capabilities failed !");
                            }
                        }

                        targetAct.iSensorType = jsonObj.getInt("SensorType");
                        int tracking = jsonObj.getInt("TrackerStatus");


                        double iZoomMagTimes = jsonObj.getDouble("ZoomMagTimes");

                        targetAct.tv_zoom.setText("Zoom:"+String.format("%1$.2f", iZoomMagTimes));



                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                }
                break;

                case HANDLER_MSG_DEV_IP_ENCODER_GET_CFG_RES: {
                    if (msg.arg1 != 0 || msg.obj == null) {
                        //                        Toast.makeText(targetAct, R.string.code_setting, Toast.LENGTH_SHORT).show();
                    } else {
                        String strJson = (String) msg.obj;
                        try {
                            JSONObject jsonObj = new JSONObject(strJson);
                            JSONObject jsonEncoderConfig = jsonObj.getJSONObject("EncoderConfig");

                        } catch (JSONException e) {
                            e.printStackTrace();
                        }
                    }
                }
                break;

                case HANDLER_MSG_DEV_IP_ENCODER_GET_VERSION_RES: {
                    if (msg.arg1 != 0 || msg.obj == null) {
                        //                        Toast.makeText(targetAct, R.string.code_setting_msg, Toast.LENGTH_SHORT).show();
                    } else {
                        String strJson = (String) msg.obj;
                        try {
                            JSONObject jsonObj = new JSONObject(strJson);
                            JSONObject jsonEncoderVersion = jsonObj.getJSONObject("EncoderVersion");

                        } catch (JSONException e) {
                            e.printStackTrace();
                        }
                    }
                }
                break;
            }
        }
    }


    public String getFromUI(boolean bSave2DevCfg) {
        String strJson = "";
        try {
            JSONObject jsonObj = new JSONObject();
            Log.d("VLMAG", "put--getFromUI:" + osdBeans.get(6).isChecked);
            jsonObj.put("EnableOSD", osdBeans.get(0).isChecked ? 1 : 0);
            jsonObj.put("Cross", osdBeans.get(1).isChecked ? 1 : 0);
            jsonObj.put("PitchYaw", osdBeans.get(2).isChecked ? 1 : 0);
            jsonObj.put("XYShift", osdBeans.get(3).isChecked ? 1 : 0);
            jsonObj.put("GPS", osdBeans.get(4).isChecked ? 1 : 0);
            jsonObj.put("Time", osdBeans.get(5).isChecked ? 1 : 0);
            jsonObj.put("VL-MAG", osdBeans.get(6).isChecked ? 1 : 0);
            jsonObj.put("BigFont", osdBeans.get(7).isChecked ? 1 : 0);

            jsonObj.put("InputSave", bSave2DevCfg ? 1 : 0);
            jsonObj.put("InputTime", osdBeans.get(8).isChecked ? 1 : 0);
            jsonObj.put("InputGPS", osdBeans.get(9).isChecked ? 1 : 0);
            jsonObj.put("InputPitchYaw", osdBeans.get(10).isChecked ? 1 : 0);
            jsonObj.put("InputVL-MAG", osdBeans.get(11).isChecked ? 1 : 0);
            jsonObj.put("InputZoomTimesOrFOV", m_osd_fov.isChecked() ? 1 : 0);
            jsonObj.put("InputMGRS", m_osd_mgrs.isChecked() ? 1 : 0);
            jsonObj.put("InputCharBlackBorder", 1);

            strJson = jsonObj.toString();
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return strJson;

    }
}
