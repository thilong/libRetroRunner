package com.aidoo.retrorunner;

import android.view.Surface;

public class NativeRunner {
    static {
        System.loadLibrary("RetroRunner");
    }

    private static NativeRunner instance;

    public static NativeRunner getInstance() {
        if (instance == null)
            instance = new NativeRunner();
        return instance;
    }

    public void initIfNeeded() {

    }

    /*jni methods*/
    /*设置全局的环境变量*/
    public static native void initEnv();

    /*创建一个新的模拟实例，如果已经有在模拟的，则停止*/
    public static native void create(String romPath, String corePath, String systemPath, String savePath);

    /*添加核心变量*/
    public static native void addVariable(String key, String value);

    /*开始模拟*/
    public static native void start();

    /*暂停*/
    public static native void pause();

    /*恢复*/
    public static native void resume();

    /*重置*/
    public static native void reset();

    /*停止，停止后不可恢复*/
    public static native void stop();

    /*设置快进或者慢进， 0.1 - 3.0*/
    public static native void setFastForward(double multiplier);


    /*设置模拟的输出显示目标*/
    public static native void setVideoTarget(Surface surface);

    /*更新显示输出的尺寸*/
    public static native void setVideoTargetSize(int width, int height);


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
