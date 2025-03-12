package com.aidoo.libdroidvirualpad;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.view.KeyEvent;

import java.util.HashMap;

public class VPComponentCreator {

    public enum VPComponentType {
        BUTTON_A,
        BUTTON_B,
        BUTTON_C,
        BUTTON_D,
        BUTTON_X,
        BUTTON_Y,
        BUTTON_Z,

        BUTTON_L,
        BUTTON_L2,
        BUTTON_R,
        BUTTON_R2,

        BUTTON_SELECT,
        BUTTON_START,

        DPAD,
        JOYSTICK
    }

    static class ButtonMeta {
        public int drawableId;
        public int keyCode;

        public ButtonMeta(int resId, int keyValue) {
            this.drawableId = resId;
            this.keyCode = keyValue;
        }
    }


    private static final HashMap<VPComponentType, ButtonMeta> buttonMetas = new HashMap<>();

    static {
        buttonMetas.put(VPComponentType.BUTTON_A, new ButtonMeta(R.drawable.vp_btn_a, KeyEvent.KEYCODE_BUTTON_A));
        buttonMetas.put(VPComponentType.BUTTON_B, new ButtonMeta(R.drawable.vp_btn_b, KeyEvent.KEYCODE_BUTTON_B));
        buttonMetas.put(VPComponentType.BUTTON_C, new ButtonMeta(R.drawable.vp_btn_c, KeyEvent.KEYCODE_BUTTON_C));
        buttonMetas.put(VPComponentType.BUTTON_X, new ButtonMeta(R.drawable.vp_btn_x, KeyEvent.KEYCODE_BUTTON_X));
        buttonMetas.put(VPComponentType.BUTTON_Y, new ButtonMeta(R.drawable.vp_btn_y, KeyEvent.KEYCODE_BUTTON_Y));
        buttonMetas.put(VPComponentType.BUTTON_Z, new ButtonMeta(R.drawable.vp_btn_z, KeyEvent.KEYCODE_BUTTON_Z));

        buttonMetas.put(VPComponentType.BUTTON_L, new ButtonMeta(R.drawable.vp_btn_l, KeyEvent.KEYCODE_BUTTON_L1));
        buttonMetas.put(VPComponentType.BUTTON_L2, new ButtonMeta(R.drawable.vp_btn_zl, KeyEvent.KEYCODE_BUTTON_L2));
        buttonMetas.put(VPComponentType.BUTTON_R, new ButtonMeta(R.drawable.vp_btn_r, KeyEvent.KEYCODE_BUTTON_R1));
        buttonMetas.put(VPComponentType.BUTTON_R2, new ButtonMeta(R.drawable.vp_btn_zr, KeyEvent.KEYCODE_BUTTON_R2));

        buttonMetas.put(VPComponentType.BUTTON_SELECT, new ButtonMeta(R.drawable.vp_btn_select, KeyEvent.KEYCODE_BUTTON_SELECT));
        buttonMetas.put(VPComponentType.BUTTON_START, new ButtonMeta(R.drawable.vp_btn_start, KeyEvent.KEYCODE_BUTTON_START));

    }

    private Context context;

    public VPComponentCreator withContext(Context context) {
        this.context = context;
        return this;
    }

    public VPComponentBase createComponent(VPComponentType type) {
        if (type == VPComponentType.DPAD) return createDPad();
        if (type == VPComponentType.JOYSTICK) return createJoystick();
        return createButton(type);
    }

    private VPComponentBase createButton(VPComponentType type) {
        VPButton ret = new VPButton();
        ButtonMeta meta = buttonMetas.get(type);
        Drawable resDown = context.getResources().getDrawable(meta.drawableId);
        Drawable resUp = context.getResources().getDrawable(meta.drawableId);
        resDown.setTint(0x800000FF);

        ret.setValue(meta.keyCode);
        ret.setDrawableForState(VPComponentBase.STATE_UP, resUp);
        ret.setDrawableForState(VPComponentBase.STATE_DOWN, resDown);
        int dps = (int) (40 * context.getResources().getDisplayMetrics().density);
        ret.setBounds(0, 0, dps, dps);
        return ret;
    }

    private VPComponentBase createDPad() {
        VPDPad dpad = new VPDPad();
        dpad.setDrawableForState(VPDPad.STATE_DOWN, context.getResources().getDrawable(R.drawable.vp_dpad));
        dpad.setDrawableForState(VPDPad.STATE_UP, context.getResources().getDrawable(R.drawable.vp_dpad));
        dpad.setDrawableForState(VPDPad.STATE_DIR_LEFT, context.getResources().getDrawable(R.drawable.vp_dpad_l));
        dpad.setDrawableForState(VPDPad.STATE_DIR_LEFT | VPDPad.STATE_DIR_TOP, context.getResources().getDrawable(R.drawable.vp_dpad_lu));
        dpad.setDrawableForState(VPDPad.STATE_DIR_TOP, context.getResources().getDrawable(R.drawable.vp_dpad_u));
        dpad.setDrawableForState(VPDPad.STATE_DIR_RIGHT | VPDPad.STATE_DIR_TOP, context.getResources().getDrawable(R.drawable.vp_dpad_ru));
        dpad.setDrawableForState(VPDPad.STATE_DIR_RIGHT, context.getResources().getDrawable(R.drawable.vp_dpad_r));
        dpad.setDrawableForState(VPDPad.STATE_DIR_RIGHT | VPDPad.STATE_DIR_BOTTOM, context.getResources().getDrawable(R.drawable.vp_dpad_rd));
        dpad.setDrawableForState(VPDPad.STATE_DIR_BOTTOM, context.getResources().getDrawable(R.drawable.vp_dpad_d));
        dpad.setDrawableForState(VPDPad.STATE_DIR_LEFT | VPDPad.STATE_DIR_BOTTOM, context.getResources().getDrawable(R.drawable.vp_dpad_ld));

        dpad.setDirectionValue(VPDPad.STATE_DIR_LEFT, KeyEvent.KEYCODE_DPAD_LEFT);
        dpad.setDirectionValue(VPDPad.STATE_DIR_TOP, KeyEvent.KEYCODE_DPAD_UP);
        dpad.setDirectionValue(VPDPad.STATE_DIR_RIGHT, KeyEvent.KEYCODE_DPAD_RIGHT);
        dpad.setDirectionValue(VPDPad.STATE_DIR_BOTTOM, KeyEvent.KEYCODE_DPAD_DOWN);
        int dps = (int) (100 * context.getResources().getDisplayMetrics().density);
        dpad.setBounds(0, 0, dps, dps);
        return dpad;
    }

    private VPComponentBase createJoystick() {
        return new VPButton();
    }

}
