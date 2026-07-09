package com.android.viewproplayer;

import android.content.Context;
import android.support.annotation.NonNull;
import android.util.Log;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import java.io.InputStream;
import java.util.HashMap;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;


public class DeviceCapabilities {

    private final String TAG = "DeviceCapabilities";

    private HashMap<String, Integer> hashMap = new HashMap<String, Integer>();
    private Context mContext;
    private String m_strModelName = "";


    public DeviceCapabilities(@NonNull Context context) {
        mContext = context;
        Log.d("","");
    }

    public boolean LoadCapByDeviceType(int iDeviceType) {
        hashMap.clear();

        InputStream inputStream = mContext.getResources().openRawResource(R.raw.device_capabilities);

        DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
        try {
            DocumentBuilder builder = factory.newDocumentBuilder();
            Document dom = builder.parse(inputStream);

            Element root = dom.getDocumentElement();
            NodeList itemsModel = root.getChildNodes();
            for (int i = 0; i < itemsModel.getLength(); ++i) {
                if (itemsModel.item(i).getNodeType() != Node.ELEMENT_NODE) {
                    continue;
                }

                Element NodeModel = (Element) itemsModel.item(i);
                // Log.e(TAG,"NodeModel ###### " + NodeModel.getNodeName());
                String strTypeID = NodeModel.getAttribute("id").replace("0x", "");
                int iTypeID = Integer.parseInt(strTypeID, 16);

                if (iTypeID == iDeviceType) {
                    m_strModelName = NodeModel.getAttribute("name");
                    NodeList itemsCapabilities = NodeModel.getChildNodes();
                    for (int j = 0; j < itemsCapabilities.getLength(); ++j) {
                        if (itemsCapabilities.item(j).getNodeType() != Node.ELEMENT_NODE) {
                            continue;
                        }

                        Element NodeCapability = (Element) itemsCapabilities.item(j);
                        // Log.e(TAG, "########## " + NodeCapability.getAttribute("name") + "=====>" + NodeCapability.getFirstChild().getNodeValue());
                        if (Integer.parseInt(NodeCapability.getFirstChild().getNodeValue()) != 0) {
                            hashMap.put(NodeCapability.getAttribute("name"), Integer.parseInt(NodeCapability.getFirstChild().getNodeValue()));
                        }
                    }
                    break;
                }
            }

            inputStream.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
//  calibration
        return true;
    }

    public String GetModelName() {
        return m_strModelName;
    }

    public boolean HasCapability(String strCapabilityName) {
        return hashMap.get(strCapabilityName) != null &&  hashMap.get(strCapabilityName) != 0;
    }

    public int GetSensorNum() {
        int iSensorNum = 0;
        if (hashMap.get("VisibleZoomLens") != null && hashMap.get("VisibleZoomLens") != 0) {
            ++iSensorNum;
        }

        if (hashMap.get("VisiblePrimeLens") != null && hashMap.get("VisiblePrimeLens") != 0) {
            ++iSensorNum;
        }

        if (hashMap.get("IRZoomLens") != null && hashMap.get("IRZoomLens") != 0) {
            ++iSensorNum;
        }

        if (hashMap.get("IRPrimeLens") != null && hashMap.get("IRPrimeLens") != 0) {
            ++iSensorNum;
        }

        return iSensorNum;
    }



    public boolean HasIRSensor() {
        return (hashMap.get("IRZoomLens") != null && hashMap.get("IRZoomLens") != 0) ||
                (hashMap.get("IRPrimeLens") != null && hashMap.get("IRPrimeLens") != 0);
    }

    public float GetVisibleZoomMin() {
        return (float) 0.0;
    }

    public float GetVisibleZoomMax() {
        return (float) 36.0;
    }


}
