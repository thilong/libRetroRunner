package com.aidoo.test;

import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.aidoo.retrorunner.AspectRatio;
import com.aidoo.retrorunner.RRView;
import com.aidoo.retrorunner.RRParam;

public class RetroRunnerActivity extends AppCompatActivity {
    private RRView retroRunnerView;
    private ViewGroup gameContainer;
    private String testRom;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        //force landscape
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        super.onCreate(savedInstanceState);
        setContentView(R.layout.sample_activity);
        String platform = "nes";

        String testCore = "";
        if (platform.equals("nes")) {
            testRom = getExternalFilesDir(null).getAbsolutePath() + "/test.nes";
            testCore = "libfceumm.so";
            FileUtil.copyFromAsses(this, "demo.nes", testRom);
        } else if (platform.equals("dc")) {
            testRom = getExternalFilesDir(null).getAbsolutePath() + "/bj.cdi";
            //testRom = getFilesDir().getAbsolutePath() + "/lc.chd";
            testCore = "libflycast.so";
            FileUtil.copyFromAsses(this, "bj.cdi", testRom);
        } else if (platform.equals("psp")) {
            testRom = getExternalFilesDir(null).getAbsolutePath() + "/psp.iso";
            testCore = "libppsspp.so";
        }

        RRParam runnerArgument = new RRParam();
        runnerArgument.setRomPath(testRom);
        runnerArgument.setCorePath(testCore);
        runnerArgument.setSystemPath(getFilesDir().getAbsolutePath());
        runnerArgument.setSavePath(getExternalFilesDir(null).getAbsolutePath());

        gameContainer = (ViewGroup) findViewById(R.id.game_container);
        retroRunnerView = new RRView(this, runnerArgument);
        retroRunnerView.setLayoutParams(new ViewGroup.LayoutParams(-1, -1));
        gameContainer.addView(retroRunnerView);

        setupView();
    }

    void setupView() {
        android.graphics.Bitmap.createBitmap(1, 1, Bitmap.Config.ARGB_8888);
        findViewById(R.id.test_save_state).setOnClickListener(v -> onTestSaveState());
        findViewById(R.id.test_load_state).setOnClickListener(v -> onTestLoadState());
        findViewById(R.id.test_save_ram).setOnClickListener(v -> onTestSaveSRAM());
        findViewById(R.id.test_load_ram).setOnClickListener(v -> onTestLoadSRAM());

        ViewGroup testLayouts = ((ViewGroup) findViewById(R.id.test_layouts));
        for (int idx = 0; idx < testLayouts.getChildCount(); idx++) {
            ((Button) testLayouts.getChildAt(idx)).setOnClickListener(this::onTestLayout);
        }
        findViewById(R.id.test_fastward).setOnClickListener(this::onTestFastForward);
        findViewById(R.id.test_resete).setOnClickListener(v -> retroRunnerView.resetEmu());
        findViewById(R.id.test_screen_shot).setOnClickListener(this::onTestScreenshot);

        findViewById(R.id.test_cheat).setOnClickListener(this::onTestCheat);
    }

    @Override
    protected void onPause() {
        retroRunnerView.onPause();
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        retroRunnerView.onResume();
    }

    @Override
    protected void onDestroy() {
        retroRunnerView.onDestroy();
        super.onDestroy();
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (retroRunnerView.updateButtonState(event))
            return true;
        return super.dispatchKeyEvent(event);
    }

    void onTestLoadState() {
        retroRunnerView.loadStateAsync(0, arg -> Toast.makeText(RetroRunnerActivity.this, "load state [0]: " + arg, Toast.LENGTH_SHORT).show());
    }

    void onTestSaveState() {
        retroRunnerView.saveStateAsync(0, arg -> Toast.makeText(RetroRunnerActivity.this, "save state [0]: " + arg, Toast.LENGTH_SHORT).show());
    }

    void onTestSaveSRAM() {
        retroRunnerView.saveRamAsync(arg -> Toast.makeText(RetroRunnerActivity.this, "save ram: " + arg, Toast.LENGTH_SHORT).show());
    }

    void onTestLoadSRAM() {
        retroRunnerView.loadRamAsync(arg -> Toast.makeText(RetroRunnerActivity.this, "load ram: " + arg, Toast.LENGTH_SHORT).show());
    }

    void onTestLayout(View layout) {
        String tag = (String) layout.getTag();
        int ratioValue = Integer.parseInt(tag);
        AspectRatio ratio = AspectRatio.valueOf(ratioValue);
        retroRunnerView.setAspectRatio(ratio);
    }

    void onTestFastForward(View view) {
        if (view.getTag() != null) {
            view.setTag(null);
            retroRunnerView.setFastForward(1.0);
        } else {
            view.setTag(1);
            retroRunnerView.setFastForward(2.0);
        }

    }

    void onTestScreenshot(View view) {
        retroRunnerView.takeScreenshot(testRom + ".png");
    }

    void onTestCheat(View view) {
        retroRunnerView.addCheat("AIOGZPEY","inf hp", true);
    }

    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }
}
