package com.aidoo.retrorunner;

import android.annotation.SuppressLint;
import android.content.Context;
import android.util.Log;
import android.view.KeyEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

@SuppressLint("ViewConstructor")
public class RetroRunnerView extends SurfaceView implements SurfaceHolder.Callback {

    static {
        NativeRunner.initEnv();
    }

    private final RunnerArgument config;
    private boolean runnerStarted = false;

    public RetroRunnerView(Context context, RunnerArgument config) {
        super(context);
        this.config = config;
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        getHolder().addCallback(this);
        startRunnerIfNeeded();
    }

    /**
     * 生命周期函数，在Activity,Fragment的onPause中调用
     */
    public void onPause() {
        Log.w("RetroRunner", "onPause");
        NativeRunner.pause();
    }

    /**
     * 生命周期函数，在Activity,Fragment的onResume中调用
     */
    public void onResume() {
        NativeRunner.resume();
    }

    /**
     * 生命周期函数，在Activity,Fragment的onDestroy中调用
     */
    public void onDestroy() {
        //TODO:需要检测是否需要在onStop中做这个操作
        Log.w("RetroRunner", "onDestroy");
        NativeRunner.stop();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.w("RetroRunner", "surfaceCreated");
        NativeRunner.setVideoTarget(holder.getSurface());
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.w("RetroRunner", "surfaceChanged " + width + "x" + height);
        NativeRunner.setVideoTargetSize(width, height);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.w("RetroRunner", "surfaceDestroyed");
        NativeRunner.setVideoTarget(null);
    }

    private void startRunnerIfNeeded() {
        if (runnerStarted) return;
        runnerStarted = true;
        NativeRunner.create(config.getRomPath(), config.getCorePath(), config.getSystemPath(), config.getSavePath());
        if (config.haveVariables()) {
            for (String key : config.getVariables().keySet()) {
                NativeRunner.addVariable(key, config.getVariables().get(key));
            }
        }
        NativeRunner.start();
    }

    public void stopEmu() {
        NativeRunner.stop();
    }

    public void pauseEmu(boolean pause) {
        if (pause) {
            NativeRunner.pause();
        } else {
            NativeRunner.resume();
        }
    }

    public void resetEmu() {
        NativeRunner.reset();
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        NativeRunner.updateButtonState(0, event.getKeyCode(), event.getAction() == KeyEvent.ACTION_DOWN);
        return true;
    }
}
