package com.aidoo.test;

import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.ViewGroup;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.aidoo.retrorunner.RetroRunnerView;
import com.aidoo.retrorunner.RunnerArgument;

public class RetroRunnerActivity extends AppCompatActivity {
    private RetroRunnerView retroRunnerView;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        //force landscape
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        super.onCreate(savedInstanceState);

        String platform = "psp";
        String testRom = "";
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

        RunnerArgument runnerArgument = new RunnerArgument();
        runnerArgument.setRomPath(testRom);
        runnerArgument.setCorePath(testCore);
        runnerArgument.setSystemPath(getFilesDir().getAbsolutePath());
        runnerArgument.setSavePath(getExternalFilesDir(null).getAbsolutePath());

        retroRunnerView = new RetroRunnerView(this, runnerArgument);
        retroRunnerView.setLayoutParams(new ViewGroup.LayoutParams(-1, -1));
        setContentView(retroRunnerView);
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

        return retroRunnerView.dispatchKeyEvent(event);
    }
}
