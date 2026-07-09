package com.android.viewproplayer;

import android.Manifest;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.view.View;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import java.lang.ref.WeakReference;

import viewpro.com.viewlink.ViewLinkJNI;


public class ConnectActivity extends Activity implements
        View.OnClickListener {



    private static final int PERMISSION_REQUEST_CAMERA_AUDIOREC = 123;


    private ConnActBroadcastReceiver m_Receiver;

    private IntentFilter             m_intentFilter;
    private Handler m_Handler = new ConnResultHandler(this);
    public  ProgressHUDS mProgressHUDS;
    private TextView tv_port;
    private EditText edGimbalPort;
    private TextView tv_btnConnect;
    private EditText edGimbalIP;

    @Override
    public void onClick(View v) {
        switch (v.getId()) {



            case R.id.tv_btnConnect:
                showMyDialog(true);

                int iPort = Integer.parseInt(edGimbalPort.getText().toString());

                ViewLinkJNI.Disconnect();



                String strGimbalIP = edGimbalIP.getText().toString();



                ViewLinkJNI.Connect(strGimbalIP, iPort, "", 0, getApplication(),
                        "DevConnStatusCallback", 0, true);

                break;



        }
    }

    private class ConnResultHandler extends Handler {
        private final WeakReference<ConnectActivity> m_Target;

        public ConnResultHandler(ConnectActivity target) {
            m_Target = new WeakReference<ConnectActivity>(target);
        }

        @Override
        public void handleMessage(android.os.Message msg) {
            ConnectActivity targetAct = m_Target.get();
            if (null == targetAct) {
                return;
            }
            scheduleDismiss();
            if (msg.what == 2) // TCP connected
            {

                Toast.makeText(targetAct, R.string.tcp_connected, Toast.LENGTH_SHORT).show();

                    Intent intent = new Intent(targetAct, MainActivity.class);
                    targetAct.startActivity(intent);
                    targetAct.finish();

            } else if (msg.what == 3) // TCP disconnected
            {


                Toast.makeText(targetAct, R.string.tcp_disconnected, Toast.LENGTH_SHORT).show();


            } else if (msg.what == 4) // serial port connected
            {

                Toast.makeText(targetAct, R.string.serial_port_connected, Toast.LENGTH_SHORT).show();
            } else if (msg.what == 5) // serial port disconnected
            {
                Toast.makeText(targetAct, R.string.serial_port_disconnected, Toast.LENGTH_SHORT).show();

            } else {

                Toast.makeText(targetAct, R.string.unknown_conn_status + msg.what,
                        Toast.LENGTH_SHORT).show();

            }
        }
    }


    private void scheduleDismiss() {
        if(mProgressHUDS !=null){
            mProgressHUDS.dismiss();
        }

    }

    public void showMyDialog(boolean isCancelable) {
        mProgressHUDS = ProgressHUDS.show(this, R.string.on_connection, isCancelable, null);
    }

    public class ConnActBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context arg0, Intent arg1) {
            // TODO Auto-generated method stub
            int iConnStatus = arg1.getIntExtra("ConnStatus", 0);
            // String strMst = arg1.getStringExtra("Msg");

            m_Handler.obtainMessage(iConnStatus).sendToTarget();
        }
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_connect);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);

        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);

        startCameraPreviewWithPermCheck();


        m_intentFilter = new IntentFilter();
        m_intentFilter.addAction(VLKApplication.BROADCAST_CONNECTION_STATUS);
        m_Receiver = new ConnActBroadcastReceiver();
        registerReceiver(m_Receiver, m_intentFilter);

        tv_btnConnect =findViewById(R.id.tv_btnConnect);
        tv_btnConnect.setOnClickListener(this);

        edGimbalIP = findViewById(R.id.edGimbalIP);

        edGimbalPort = findViewById(R.id.edGimbalPort);
    }



    private void startCameraPreviewWithPermCheck() {


            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && this.checkSelfPermission(
                    Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                String[] permissions = {
                        Manifest.permission.CAMERA,
                        Manifest.permission.ACCESS_FINE_LOCATION,
                        Manifest.permission.ACCESS_LOCATION_EXTRA_COMMANDS,
                        Manifest.permission.READ_EXTERNAL_STORAGE,
                        Manifest.permission.WRITE_EXTERNAL_STORAGE};
                ActivityCompat.requestPermissions(this, permissions,
                        PERMISSION_REQUEST_CAMERA_AUDIOREC);


        }
    }



    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String permissions[],
                                           @NonNull int[] grantResults) {
        switch (requestCode) {
            case PERMISSION_REQUEST_CAMERA_AUDIOREC: {
                if (grantResults.length > 0
                        && grantResults[0] == PackageManager.PERMISSION_GRANTED) {

                } else {
                    Toast.makeText(this, "No CAMERA or AudioRecord permission",
                            Toast.LENGTH_LONG).show();
                }

                break;
            }
        }
    }


    @Override
    protected void onDestroy() {
        // TODO Auto-generated method stub
        super.onDestroy();
        unregisterReceiver(m_Receiver);
        //        m_Handler.removeCallbacksAndMessages(null);
    }

}
