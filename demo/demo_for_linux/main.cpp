#include <iostream>
#include <string>
#include <mutex>
#include <chrono>
#include <thread>
using namespace std;
#include "../../inc/ViewLink.h"
#include "cmdline.h"

bool g_bConnected = false;
int VLK_ConnStatusCallback(int iConnStatus, const char* szMessage, int iMsgLen, void* pUserParam)
{
    if (VLK_CONN_STATUS_TCP_CONNECTED == iConnStatus)
    {
        cout << "TCP Gimbal connected !!!" << endl;
        g_bConnected = true;
    }
    else if (VLK_CONN_STATUS_TCP_DISCONNECTED == iConnStatus)
    {
        cout << "TCP Gimbal disconnected !!!" << endl;
        g_bConnected = false;
    }
    else if (VLK_CONN_STATUS_SERIAL_PORT_CONNECTED == iConnStatus)
    {
        cout << "serial port connected !!!" << endl;
        g_bConnected = true;
    }
    else if (VLK_CONN_STATUS_SERIAL_PORT_DISCONNECTED == iConnStatus)
    {
        cout << "serial port disconnected !!!" << endl;
        g_bConnected = false;
    }
    else
    {
        cout << "unknown connection stauts: " << iConnStatus << endl;
        g_bConnected = false;
    }

    return 0;
}

int VLK_DevStatusCallback(int iType, const char* szBuffer, int iBufLen, void* pUserParam)
{
    if (VLK_DEV_STATUS_TYPE_MODEL == iType)
    {
        VLK_DEV_MODEL* pModel = (VLK_DEV_MODEL*)szBuffer;
        cout << "model code: " << pModel->cModelCode << ", model name: " << pModel->szModelName << endl;
    }
    else if (VLK_DEV_STATUS_TYPE_CONFIG == iType)
    {
        VLK_DEV_CONFIG* pDevConfig = (VLK_DEV_CONFIG*)szBuffer;
        cout << "VersionNO: " << pDevConfig->cVersionNO << ", DeviceID: " << pDevConfig->cDeviceID << ", SerialNO: " << pDevConfig->cSerialNO << endl;
    }
    else if (VLK_DEV_STATUS_TYPE_TELEMETRY == iType)
    {
        /*
         * once device is connected, telemetry information will keep updating,
         * in order to avoid disturbing user input, comment out printing telemetry information
         */
        // VLK_DEV_TELEMETRY* pTelemetry = (VLK_DEV_TELEMETRY*)szBuffer;
        // cout << "Yaw: " << pTelemetry->dYaw << ", Pitch: " << pTelemetry->dPitch << ", sensor type: " << pTelemetry->emSensorType << ", Zoom mag times: " << pTelemetry->sZoomMagTimes << endl;
    }
    else
    {
        cout << "error: unknown status type: " << iType << endl;
    }


    return 0;
}

int main(int argc, char *argv[])
{
   // parse cmd line
    cmdline::parser a;
    a.add<string>("type", 't', "connection type", true, "tcp", cmdline::oneof<string>("serial", "tcp"));
    a.add<string>("ip", 'i', "gimbal tcp ip", false, "192.168.2.119");
    a.add<int>("port", 'p', "gimbal tcp port", false, 2000);
    a.add<string>("serial", 's', "serial port name", false, "/dev/ttyS0");
    a.add<int>("baudrate", 'b', "serial port baudrate", false, 115200);
    a.parse_check(argc, argv);

    // print sdk version
    cout << "ViewLink SDK version: " << GetSDKVersion() << endl;

    // initialize sdk
    int iRet = VLK_Init();
    if (VLK_ERROR_NO_ERROR != iRet)
    {
       cout << "VLK_Init failed, error: " << iRet << endl;
       return -1;
    }

    // register device status callback
    VLK_RegisterDevStatusCB(VLK_DevStatusCallback, NULL);

    // connect device
    if (0 == a.get<string>("type").compare("tcp"))
    {
        VLK_CONN_PARAM param;
        memset(&param, 0, sizeof(param));
        param.emType = VLK_CONN_TYPE_TCP;
        strncpy(param.ConnParam.IPAddr.szIPV4, a.get<string>("ip").c_str(), sizeof(param.ConnParam.IPAddr.szIPV4) - 1);
        param.ConnParam.IPAddr.iPort = a.get<int>("port");

        cout << "connecting gimbal ip: " << a.get<string>("ip") << ", port: " << a.get<int>("port") << "..." << endl;
        iRet = VLK_Connect(&param, VLK_ConnStatusCallback, NULL);
        if (VLK_ERROR_NO_ERROR != iRet)
        {
           cout << "VLK_Connect failed, error: " << iRet << endl;
           goto quit;
        }
    }
    else if (0 == a.get<string>("type").compare("serial"))
    {
        VLK_CONN_PARAM param;
        memset(&param, 0, sizeof(param));
        param.emType = VLK_CONN_TYPE_SERIAL_PORT;
        strncpy(param.ConnParam.SerialPort.szSerialPortName, a.get<string>("serial").c_str(), sizeof(param.ConnParam.SerialPort.szSerialPortName) - 1);
        param.ConnParam.SerialPort.iBaudRate = a.get<int>("baudrate");

        cout << "connecting gimbal serial: " << a.get<string>("serial") << ", baudrate: " << a.get<int>("baudrate") << "..." << endl;
        iRet = VLK_Connect(&param, VLK_ConnStatusCallback, NULL);
        if (VLK_ERROR_NO_ERROR != iRet)
        {
           cout << "VLK_Connect failed, error: " << iRet << endl;
           goto quit;
        }
    }
    else
    {
        cout << "unknown conntion type !!!" << endl;
        goto quit;
    }


    cout << "wait device connected..." << endl;
    while (1)
    {
        if (g_bConnected)
        {
           break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }


    while (1)
    {
        cout << "press \'w\' move up \n";
        cout << "press \'s\' move down \n";
        cout << "press \'a\' move left \n";
        cout << "press \'d\' move right \n";
        cout << "press \'h\' move to home posiion \n";
        cout << "press \'1\' zoom in, \'2\' zoom out\n";
        cout << "press \'3\' begin track, \'4\' stop track\n";
        cout << "press \'c\' exit"<< endl;
        char input;
        cin >> input;
        if (input == 'w' || input == 'W')
        {
            VLK_Move(0, 1000);
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            VLK_Stop();
        }
        else if (input == 's' || input == 'S')
        {
            VLK_Move(0, -1000);
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            VLK_Stop();
        }
        else if (input == 'a' || input == 'A')
        {
            VLK_Move(-1000, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            VLK_Stop();
        }
        else if (input == 'd' || input == 'D')
        {
            VLK_Move(1000, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            VLK_Stop();
        }
        else if (input == 'h' || input == 'H')
        {
            VLK_Home();
        }
        else if (input == '1')
        {
            VLK_ZoomIn(4);
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            VLK_StopZoom();
        }
        else if (input == '2')
        {
            VLK_ZoomOut(4);
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            VLK_StopZoom();
        }
        else if (input == '3')
        {
            VLK_TRACK_MODE_PARAM param;
            memset(&param, 0, sizeof(param));
            param.emTrackSensor = VLK_SENSOR_VISIBLE1;
            param.emTrackTempSize = VLK_TRACK_TEMPLATE_SIZE_AUTO;
            VLK_TrackTargetPositionEx(&param, 100, 100, 1280, 720);
        }
        else if (input == '4')
        {
            VLK_DisableTrackMode();
        }
        else if (input == 'c' || input == 'C')
        {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

quit:
    // uninitial sdk
    VLK_UnInit();

    system("PAUSE");
    return 0;
}
