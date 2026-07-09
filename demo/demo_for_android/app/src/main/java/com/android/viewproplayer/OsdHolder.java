package com.android.viewproplayer;

import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.view.View;
import android.widget.Switch;
import android.widget.TextView;

/**
 * DreamSky  2021/11/12
 */
public class OsdHolder extends RecyclerView.ViewHolder {

    public final TextView mTitle;
    public final Switch mSwitch;

    public OsdHolder(@NonNull View itemView) {
        super(itemView);
        mTitle = itemView.findViewById(R.id.osd_title);
        mSwitch = itemView.findViewById(R.id.osd_switch);
    }
}
