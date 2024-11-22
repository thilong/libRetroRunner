package com.aidoo.retrorunner;

import android.view.Surface;

public class RRNative {
    static {
        System.loadLibrary("RetroRunner");
    }

    private static RRNative instance;

    public static RRNative getInstance() {
        if (instance == null)
            instance = new RRNative();
        return instance;
    }

    public void initIfNeeded() {

    }


    /*jni methods--------------------------*/

    /*init global env*/
    public static native void initEnv();

    /*create a new emulate instance, if one is exists, stop it.*/
    public static native void create(String romPath, String corePath, String systemPath, String savePath);

    /**
     * add variable to core
     *
     * @param key       variable key
     * @param value     variable value
     * @param notifyCore    if true, notify core to update the variable
     */
    public static native void addVariable(String key, String value, boolean notifyCore);

    /* start emulate*/
    public static native void start();

    /*pause*/
    public static native void pause();

    /*resume*/
    public static native void resume();

    /*reset*/
    public static native void reset();

    /*stop emu, can't resume anymore*/
    public static native void stop();

    /*set the video surface for drawing*/
    public static native void setVideoSurface(Surface surface);

    /*update the video surface size*/
    public static native void setVideoSurfaceSize(int width, int height);



    /*设置快进或者慢进， 0.1 - 3.0*/
    public static native void setFastForward(double multiplier);

    /*玩家按钮输入*/
    public static native boolean updateButtonState(int player, int key, boolean down);

    /*玩家摇杆输入*/
    public static native void updateAxisState(int player, int axis, float value);

    public static native double getAspectRatio();

    public static native int getGameWidth();

    public static native int getGameHeight();

    public static native void takeScreenshot(String path);


    public static native long addCheat(String code, String desc, boolean enable);

    public static native void removeCheat(long id);

    public static native void clearCheat();

    /**
     * 保存游戏存档
     * 这个方法会在模拟线程中调用，所以需要注意线程安全
     *
     * @return 0:成功 其他：失败错误码
     */
    public static native int saveRam();

    /**
     * 加载游戏存档
     * 这个方法会在模拟线程中调用，所以需要注意线程安全
     *
     * @return 0:成功 其他：失败错误码
     */
    public static native int loadRam();

    /**
     * 保存状态
     *
     * @param idx 状态编号 1-9为用户选定的，0为自动存档
     * @return 0:成功 其他：失败错误码
     */
    public static native int saveState(int idx);

    /**
     * 加载状态
     *
     * @param idx 状态编号 1-9为用户选定的，0为自动存档
     * @return 0:成功 其他：失败错误码
     */
    public static native int loadState(int idx);
}
