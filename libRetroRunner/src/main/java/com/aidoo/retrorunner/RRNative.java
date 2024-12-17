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
     * @param key        variable key
     * @param value      variable value
     * @param notifyCore if true, notify core to update the variable
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

    /*set emu speed multiplier， > 0.1, 1.0 = 60fps */
    public static native void setFastForward(float multiplier);

    /**
     * update button state
     *
     * @param player player port
     * @param key    button key
     * @param down   true: down, false: up
     * @return
     */
    public static native boolean updateButtonState(int player, int key, boolean down);

    /**
     * update axis state
     *
     * @param player     player port
     * @param analog     analog index [0-3]
     * @param axisButton axis button[ x: 0, y: 1]
     * @param value      axis value, [-1, 1]
     */
    public static native void updateAxisState(int player, int analog, int axisButton, float value);

    /**
     * get the aspect ratio of the game
     *
     * @return aspect ratio
     */
    public static native double getAspectRatio();

    /**
     * get the game width
     *
     * @return game width
     */
    public static native int getGameWidth();

    /**
     * get the game height
     *
     * @return game height
     */
    public static native int getGameHeight();


    public static native long addCheat(String code, String desc, boolean enable);

    public static native void removeCheat(long id);

    public static native void clearCheat();

    //-----------------------------------------------------------------------------------------------------------//
    // The following functions are running in the emu thread, so we should call them in background               //
    //-----------------------------------------------------------------------------------------------------------//

    /**
     * take a screenshot, screenshot is taken in the emu thread. so we should call this method at background.
     * if waitForResult is true, this method will block until the screen shot is saved
     *
     * @param path          the path to save the screen shot
     * @param waitForResult if true, wait for the screen shot to be saved
     * @return if 'waitForResult' is ture, return value means screenshot is is taken or not, otherwise it means command is added or not.
     */
    public static native int takeScreenshot(String path, boolean waitForResult);

    /**
     * save the game ram to file,
     *
     * @param path the path to save the ram, if path is empty, save to default path {game path}.ram
     * @return 0: success , other: failed error code
     */
    public static native int saveRam(String path, boolean waitForResult);

    /**
     * load the game ram from file
     *
     * @param path the path to load the ram, if path is empty, load from default path {game path}.ram
     * @return 0: success , other: failed error code
     */
    public static native int loadRam(String path, boolean waitForResult);

    /**
     * save game state
     *
     * @param idx slot of the state, 1-100 are user defined, 0 is auto save
     * @return 0: success , other: failed error code
     */
    public static native int saveState(int idx, boolean waitForResult);

    public static native int saveStateWithPath(String path, boolean waitForResult);

    /**
     * load game state
     *
     * @param idx slot of the state, 1-100 are user defined, 0 is auto save
     * @return 0: success , other: failed error code
     */
    public static native int loadState(int idx, boolean waitForResult);

    public static native int loadStateWithPath(String path, boolean waitForResult);
}
