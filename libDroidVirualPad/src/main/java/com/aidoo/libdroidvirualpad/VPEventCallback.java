package com.aidoo.libdroidvirualpad;

public interface VPEventCallback {

    void onButtonEvent(int action, int key);

    void onAxisEvent(int deviceId, float axisX, float axisY);

}
