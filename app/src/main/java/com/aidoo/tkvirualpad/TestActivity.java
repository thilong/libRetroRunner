package com.aidoo.tkvirualpad;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import androidx.annotation.Nullable;

import com.aidoo.libdroidvirualpad.VPEventCallback;
import com.aidoo.libdroidvirualpad.VPView;
import com.aidoo.test.R;

public class TestActivity extends Activity {
    private VPView VPView;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main_activity);
        FrameLayout frameLayout = findViewById(R.id.root);
        VPView = new VPView(this);
        VPView.setLayoutParams(new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        frameLayout.addView(VPView, 0);

    }

    @Override
    protected void onResume() {
        super.onResume();
        VPView.setupIfNeeded(new VPEventCallback() {
            @Override
            public void onButtonEvent(int action, int key) {
                Log.d("TestActivity", "onButtonEvent: " + action + " - " + key);
            }

            @Override
            public void onAxisEvent(int deviceId, float axisX, float axisY) {
                Log.d("TestActivity", "onAxisEvent: " + deviceId + " - " + axisX + " - " + axisY);
            }
        });
    }


}
