package com.aidoo.blu;

import android.Manifest;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothHidDevice;
import android.bluetooth.BluetoothHidDeviceAppSdpSettings;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.util.Log;

import androidx.annotation.RequiresApi;
import androidx.core.app.ActivityCompat;

import java.util.concurrent.Executors;

@SuppressLint("MissingPermission")
@RequiresApi(api = Build.VERSION_CODES.P)
public class GamepadHidService {
    private static final int REPORT_ID_GAMEPAD = 1;
    private static final int BUTTON_UP = 0x01;
    private static final int BUTTON_DOWN = 0x02;
    private static final int BUTTON_LEFT = 0x04;
    private static final int BUTTON_RIGHT = 0x08;
    private static final int BUTTON_A = 0x10;
    private static final int BUTTON_B = 0x20;

    private BluetoothHidDevice mBluetoothHidDevice;
    private BluetoothDevice mConnectedDevice;
    private Context mContext;
    private byte mGamepadReport = 0;

    private static final byte[] GAMEPAD_HID_DESCRIPTOR = {
            // 通用桌面页面
            0x05, 0x01,        // Usage Page (Generic Desktop)
            0x09, 0x05,        // Usage (Game Pad)
            (byte)0xA1, 0x01,        // Collection (Application)

            // 方向控制 (4个按钮)
            0x05, 0x09,        //   Usage Page (Button)
            0x19, 0x01,        //   Usage Minimum (Button 1)
            0x29, 0x04,        //   Usage Maximum (Button 4)
            0x15, 0x00,        //   Logical Minimum (0)
            0x25, 0x01,        //   Logical Maximum (1)
            0x75, 0x01,        //   Report Size (1)
            (byte) 0x95, 0x04,        //   Report Count (4)
            (byte)0x81, 0x02,        //   Input (Data,Var,Abs)

            // 填充4位
            0x75, 0x01,        //   Report Size (1)
            (byte)0x95, 0x04,        //   Report Count (4)
            (byte)0x81, 0x03,        //   Input (Const,Var,Abs)

            // A/B按钮 (Button 5/6)
            0x05, 0x09,        //   Usage Page (Button)
            0x19, 0x05,        //   Usage Minimum (Button 5)
            0x29, 0x06,        //   Usage Maximum (Button 6)
            0x15, 0x00,        //   Logical Minimum (0)
            0x25, 0x01,        //   Logical Maximum (1)
            0x75, 0x01,        //   Report Size (1)
            (byte)0x95, 0x02,        //   Report Count (2)
            (byte)0x81, 0x02,        //   Input (Data,Var,Abs)

            // 填充6位
            0x75, 0x01,        //   Report Size (1)
            (byte)0x95, 0x06,        //   Report Count (6)
            (byte)0x81, 0x03,        //   Input (Const,Var,Abs)

            (byte)0xC0               // End Collection
    };

    public GamepadHidService(Context context) {
        mContext = context.getApplicationContext();
        initBluetoothHid();
    }

    private void initBluetoothHid() {

        BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (bluetoothAdapter == null) {
            Log.e("GamepadHid", "Bluetooth not supported");
            return;
        }

        bluetoothAdapter.getProfileProxy(mContext, new BluetoothProfile.ServiceListener() {
            @Override
            public void onServiceConnected(int profile, BluetoothProfile proxy) {
                if (profile == BluetoothProfile.HID_DEVICE) {
                    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P) return;
                    mBluetoothHidDevice = (BluetoothHidDevice) proxy;
                    registerGamepad();
                }
            }

            @Override
            public void onServiceDisconnected(int profile) {
                if (profile == BluetoothProfile.HID_DEVICE) {
                    mBluetoothHidDevice = null;
                }
            }
        }, BluetoothProfile.HID_DEVICE);
    }

    public static void tryGetPermissions(Context ctx) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ActivityCompat.checkSelfPermission(ctx, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED ||
                    ActivityCompat.checkSelfPermission(ctx, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
                Log.e("GamepadHid", "Bluetooth permissions not granted");
            }
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            if (ActivityCompat.checkSelfPermission(ctx, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                Log.e("GamepadHid", "Location permission not granted for Bluetooth");
            }
        }
    }

    @SuppressLint("MissingPermission")
    private void registerGamepad() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P) return;
        if (mBluetoothHidDevice == null) return;
        BluetoothHidDeviceAppSdpSettings sdp = new BluetoothHidDeviceAppSdpSettings(
                "Android Gamepad",
                "Simple Bluetooth Gamepad",
                "Android",
                BluetoothHidDevice.SUBCLASS1_NONE,
                GAMEPAD_HID_DESCRIPTOR
        );


        mBluetoothHidDevice.registerApp(
                sdp,
                null,
                null,
                Executors.newSingleThreadExecutor(),
                new BluetoothHidDevice.Callback() {
                    @Override
                    public void onAppStatusChanged(BluetoothDevice pluggedDevice, boolean registered) {
                        Log.d("GamepadHid", "App " + (registered ? "registered" : "unregistered"));
                    }

                    @Override
                    public void onConnectionStateChanged(BluetoothDevice device, int state) {
                        Log.d("GamepadHid", "Connection state changed: " + state);
                        mConnectedDevice = (state == BluetoothProfile.STATE_CONNECTED) ? device : null;
                    }

                    @Override
                    public void onGetReport(BluetoothDevice device, byte type, byte id, int bufferSize) {
                        // 简单实现，返回当前状态
                        if (type == BluetoothHidDevice.REPORT_TYPE_INPUT && id == REPORT_ID_GAMEPAD) {
                            mBluetoothHidDevice.replyReport(device, type, id, new byte[]{mGamepadReport});
                        }
                    }

                    @Override
                    public void onSetReport(BluetoothDevice device, byte type, byte id, byte[] data) {
                        // 游戏手柄通常不需要处理SetReport
                    }

                    @Override
                    public void onInterruptData(BluetoothDevice device, byte reportId, byte[] data) {
                        // 处理中断数据
                    }
                }
        );
    }

    // 方向控制方法
    public void pressUp() {
        setButtonState(BUTTON_UP, true);
    }

    public void releaseUp() {
        setButtonState(BUTTON_UP, false);
    }

    public void pressDown() {
        setButtonState(BUTTON_DOWN, true);
    }

    public void releaseDown() {
        setButtonState(BUTTON_DOWN, false);
    }

    public void pressLeft() {
        setButtonState(BUTTON_LEFT, true);
    }

    public void releaseLeft() {
        setButtonState(BUTTON_LEFT, false);
    }

    public void pressRight() {
        setButtonState(BUTTON_RIGHT, true);
    }

    public void releaseRight() {
        setButtonState(BUTTON_RIGHT, false);
    }

    // 按钮控制方法
    public void pressA() {
        setButtonState(BUTTON_A, true);
    }

    public void releaseA() {
        setButtonState(BUTTON_A, false);
    }

    public void pressB() {
        setButtonState(BUTTON_B, true);
    }

    public void releaseB() {
        setButtonState(BUTTON_B, false);
    }


    private void setButtonState(int buttonMask, boolean pressed) {
        if (pressed) {
            mGamepadReport |= buttonMask;
        } else {
            mGamepadReport &= ~buttonMask;
        }
        sendGamepadReport();
    }


    private void sendGamepadReport() {
        if (mBluetoothHidDevice != null && mConnectedDevice != null) {
            mBluetoothHidDevice.sendReport(mConnectedDevice, REPORT_ID_GAMEPAD, new byte[]{mGamepadReport});
        }
    }

    public void disconnect() {
        if (mBluetoothHidDevice != null) {
            mBluetoothHidDevice.unregisterApp();
        }
    }
}