package com.aidoo.blu;

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothHidDevice;
import android.bluetooth.BluetoothProfile;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.ActivityCompat;

import com.aidoo.libdroidvirualpad.VPEventCallback;
import com.aidoo.libdroidvirualpad.VPView;
import com.aidoo.test.R;

public class HIDTestActivity extends Activity {


    GamepadHidService hidService;
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main_activity);

        findViewById(R.id.blu_connect).setOnClickListener(this::onConnectClicked);
    }

    @Override
    protected void onResume() {
        super.onResume();

    }

    public void onConnectClicked(View v) {

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P) return;

        ActivityCompat.requestPermissions(
                this,
                new String[]{
                        Manifest.permission.BLUETOOTH,
                        Manifest.permission.BLUETOOTH_ADMIN,
                        Manifest.permission.BLUETOOTH_CONNECT
                },
                110
        );


    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P) return;
        if(requestCode == 110 && hidService == null){
            hidService= new GamepadHidService(this);
        }
    }
}
