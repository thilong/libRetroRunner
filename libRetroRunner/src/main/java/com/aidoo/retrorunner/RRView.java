package com.aidoo.retrorunner;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import androidx.annotation.NonNull;

import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

@SuppressLint("ViewConstructor")
public class RRView extends SurfaceView implements SurfaceHolder.Callback {

    static {
        RRNative.getInstance().setup();
    }

    private static final String TAG = "RetroRunner";
    private final RRParam params;
    private final Executor backgroundExecutor = Executors.newSingleThreadExecutor();
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private AspectRatio aspectRatio = AspectRatio.FullScreen;
    private Thread emuThread = null;
    private volatile boolean emuShouldRun = true;

    public RRView(Context context, RRParam config) {
        super(context);
        this.params = config;
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        startRunnerIfNeeded();
    }

    private void startRunnerIfNeeded() {
        Log.d(TAG, "startRunnerIfNeeded , thread: " + Thread.currentThread().getId());
        if (emuThread != null)
            return;
        getHolder().addCallback(this);

        RRNative.create(params.getRomPath(), params.getCorePath(), params.getSystemPath(), params.getSavePath());
        RRNative.onEmuNotificationCallback = this::onEmuNotification;
        if (params.haveVariables()) {
            for (String key : params.getVariables().keySet()) {
                RRNative.addVariable(key, params.getVariables().get(key), false);
            }
        }
        emuThread = new Thread(() -> {
            Log.d(TAG, "emu thread running");
            while (emuShouldRun) {
                emuShouldRun = RRNative.step();
            }
            RRNative.stop();
        });
        emuThread.start();
    }

    private void onEmuNotification(int cmd) {
        Log.w(TAG, "[VIEW] on notification: " + cmd);
    }

    public void onPause() {
        Log.w(TAG, "onPause");
        RRNative.pause();
    }

    public void onResume() {
        RRNative.resume();
    }

    public void onDestroy() {
        Log.w(TAG, "[VIEW] onDestroy");
        emuShouldRun = false;
        if (emuThread != null) {
            try {
                emuThread.join();
            } catch (Exception e) {
                Log.e(TAG, "[View] failed to wait emu thread join: " + e.getMessage());
            }
        }
        RRNative.onEmuNotificationCallback = null;
        RRNative.destroy();
    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder holder) {
        Log.d(TAG, "[VIEW] surfaceCreated, thread: " + Thread.currentThread().getId());
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
        Surface surface = holder.getSurface();
        Log.d(TAG, "[VIEW] surface " + surface + "[hash:" + surface.hashCode() + "] Changed " + width + "x" + height + ", thread: " + Thread.currentThread().getId());
        RRNative.OnSurfaceChanged(surface, surface.hashCode(), width, height);
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
        Log.w(TAG, "[VIEW] surfaceDestroyed");
        RRNative.OnSurfaceChanged(null, 0, this.getWidth(), this.getHeight());
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {

        int finalWidth = getDefaultSize(getSuggestedMinimumWidth(), widthMeasureSpec);
        int finalHeight = getDefaultSize(getSuggestedMinimumHeight(), heightMeasureSpec);
        //Log.w("RetroRunner", "onMeasure " + finalWidth + "x" + finalHeight);

        double gameAspectRatio = RRNative.getAspectRatio();
        int gameWidth = RRNative.getGameWidth();
        int gameHeight = RRNative.getGameHeight();

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

    public boolean updateButtonState(KeyEvent event) {
        return RRNative.updateButtonState(0, event.getKeyCode(), event.getAction() == KeyEvent.ACTION_DOWN);
    }

    /*以下函数供外部使用*/

    public void stopEmu() {
        RRNative.stop();
    }

    public void pauseEmu(boolean pause) {
        if (pause) {
            RRNative.pause();
        } else {
            RRNative.resume();
        }
    }

    public void resetEmu() {
        RRNative.reset();
    }

    public long addCheat(String code, String desc, boolean enable) {
        return RRNative.addCheat(code, desc, enable);
    }

    public void setFastForward(float multiplier) {
        RRNative.setFastForward(multiplier);
    }

    public void takeScreenshot(String path, boolean waitForResult) {
        RRNative.takeScreenshot(path, waitForResult);
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
    public void saveStateAsync(int idx, RRFuncs.Fn<Boolean> callback) {
        backgroundExecutor.execute(() -> {
            int ret = RRNative.saveState(idx, true);
            mainHandler.post(() -> {
                if (callback != null) {
                    callback.invoke(ret == 0);
                }
            });
        });

    }

    public void loadStateAsync(int idx, RRFuncs.Fn<Boolean> callback) {
        backgroundExecutor.execute(() -> {
            int ret = RRNative.loadState(idx, true);
            mainHandler.post(() -> {
                if (callback != null) {
                    callback.invoke(ret == 0);
                }
            });
        });

    }

    public void saveRamAsync(RRFuncs.Fn<Boolean> callback) {
        backgroundExecutor.execute(() -> {
            int ret = RRNative.saveRam("", true);
            mainHandler.post(() -> {
                if (callback != null) {
                    callback.invoke(ret == 0);
                }
            });
        });

    }

    public void loadRamAsync(RRFuncs.Fn<Boolean> callback) {
        backgroundExecutor.execute(() -> {
            int ret = RRNative.loadRam("", true);
            mainHandler.post(() -> {
                if (callback != null) {
                    callback.invoke(ret == 0);
                }
            });
        });

    }
}
