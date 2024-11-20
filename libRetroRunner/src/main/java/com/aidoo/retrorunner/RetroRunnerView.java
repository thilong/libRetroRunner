package com.aidoo.retrorunner;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.KeyEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

@SuppressLint("ViewConstructor")
public class RetroRunnerView extends SurfaceView implements SurfaceHolder.Callback {

    static {
        NativeRunner.initEnv();
    }

    private final RunnerArgument config;
    private boolean runnerStarted = false;
    private Executor gameThreadExecutor = Executors.newSingleThreadExecutor();
    private Handler mainHandler = new Handler(Looper.getMainLooper());
    private AspectRatio aspectRatio = AspectRatio.FullScreen;

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
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {

        int finalWidth = getDefaultSize(getSuggestedMinimumWidth(), widthMeasureSpec);
        int finalHeight = getDefaultSize(getSuggestedMinimumHeight(), heightMeasureSpec);
        //Log.w("RetroRunner", "onMeasure " + finalWidth + "x" + finalHeight);

        double gameAspectRatio = NativeRunner.getAspectRatio();
        int gameWidth = NativeRunner.getGameWidth();
        int gameHeight = NativeRunner.getGameHeight();

        switch (aspectRatio) {
            case Square:
                if (finalWidth > finalHeight) finalWidth = finalHeight;
                else finalHeight = finalWidth;
                setMeasuredDimension(finalWidth, finalHeight);
                break;
            case CoreProvide:
                if (gameWidth > 0 && gameHeight > 0) {
                    finalWidth = gameWidth;
                    finalHeight = gameHeight;
                }
                setMeasuredDimension(finalWidth, finalHeight);
                break;
            case Ratio4to3:
                gameAspectRatio = 4.0 / 3;
                setDimensionForRatio(gameAspectRatio, gameWidth, gameHeight, finalWidth, finalHeight);
                break;
            case Ratio16to9:
                gameAspectRatio = 16.0 / 9;
                setDimensionForRatio(gameAspectRatio, gameWidth, gameHeight, finalWidth, finalHeight);
                break;
            case AspectFill:
                setDimensionForRatio(gameAspectRatio, gameWidth, gameHeight, finalWidth, finalHeight);
                break;
            case MaxIntegerRatioScale: {
                double ratio = finalWidth * 1.0 / finalHeight;

                int scale;
                if (ratio > gameAspectRatio) {
                    scale = finalHeight / gameHeight;
                } else {
                    scale = finalWidth / gameWidth;
                }
                //Log.w("RetroRunner", "scale " + scale + ", ratio " + ratio + ", aspect :" + gameAspectRatio + ", w " + gameWidth + ", h " + gameHeight);
                finalHeight = scale * gameHeight;
                finalWidth = scale * gameWidth;
                setMeasuredDimension(finalWidth, finalHeight);
                break;
            }
            case FullScreen:
            default:
                super.onMeasure(widthMeasureSpec, heightMeasureSpec);
                break;
        }
    }

    private void setDimensionForRatio(double ratio, int gameWidth, int gameHeight, int widthSpec, int heightSpec) {
        int finalWidth = widthSpec;
        int finalHeight = heightSpec;
        double ratioSpec = finalWidth * 1.0 / finalHeight;
        if (ratioSpec > ratio) {
            //高度决定
            finalWidth = (int) (gameWidth * ratio * (finalHeight * 1.0 / gameHeight));
        } else {
            //宽度决定
            finalHeight = (int) (gameHeight * ratio * (finalWidth * 1.0 / gameWidth));
        }
        setMeasuredDimension(finalWidth, finalHeight);
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

    public boolean updateButtonState(KeyEvent event) {
        return NativeRunner.updateButtonState(0, event.getKeyCode(), event.getAction() == KeyEvent.ACTION_DOWN);
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


    /*以下函数供外部使用*/

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

    public long addCheat(String code, String desc, boolean enable) {
        return NativeRunner.addCheat(code, desc, enable);
    }


    public void setFastForward(double multiplier) {
        NativeRunner.setFastForward(multiplier);
    }

    public void takeScreenshot(String path) {
        NativeRunner.takeScreenshot(path);
    }

    public AspectRatio getAspectRatio() {
        return aspectRatio;
    }

    public void setAspectRatio(AspectRatio newAspectRatio) {
        if (aspectRatio == newAspectRatio) return;
        aspectRatio = newAspectRatio;
        requestLayout();
    }

    /*存档相关*/
    public void saveStateAsync(int idx, Delegate.Fn<Boolean> callback) {
        gameThreadExecutor.execute(() -> {
            int ret = NativeRunner.saveState(idx);
            mainHandler.post(() -> {
                if (callback != null) {
                    callback.invoke(ret == 0);
                }
            });
        });

    }

    public void loadStateAsync(int idx, Delegate.Fn<Boolean> callback) {
        gameThreadExecutor.execute(() -> {
            int ret = NativeRunner.loadState(idx);
            mainHandler.post(() -> {
                if (callback != null) {
                    callback.invoke(ret == 0);
                }
            });
        });

    }

    public void saveRamAsync(Delegate.Fn<Boolean> callback) {
        gameThreadExecutor.execute(() -> {
            int ret = NativeRunner.saveRam();
            mainHandler.post(() -> {
                if (callback != null) {
                    callback.invoke(ret == 0);
                }
            });
        });

    }

    public void loadRamAsync(Delegate.Fn<Boolean> callback) {
        gameThreadExecutor.execute(() -> {
            int ret = NativeRunner.loadRam();
            mainHandler.post(() -> {
                if (callback != null) {
                    callback.invoke(ret == 0);
                }
            });
        });

    }
}
