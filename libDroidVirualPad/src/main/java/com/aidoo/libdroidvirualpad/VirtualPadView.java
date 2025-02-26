package com.aidoo.libdroidvirualpad;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.ArrayList;
import java.util.HashSet;

public class VirtualPadView extends View {

    boolean isEditing = false;
    ArrayList<ComponentBase> padNodes = null;
    VPEventCallback callback = null;

    public VirtualPadView(Context context) {
        super(context);
    }

    public VirtualPadView(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    public VirtualPadView(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public void setupIfNeeded(VPEventCallback callback) {
        if (null != padNodes) return;
        this.callback = callback;

        padNodes = new ArrayList<>();
        VPButton button = new VPButton();
        button.setEventCallback(callback);
        button.setDrawableForState(VPButton.STATE_UP, getResources().getDrawable(R.drawable.test_btn));
        Drawable downRes = getResources().getDrawable(R.drawable.test_btn);
        downRes.setTint(0x80FF0000);
        button.setDrawableForState(VPButton.STATE_DOWN, downRes);
        button.setBounds(100, 200, 200, 200);
        button.setValue(KeyEvent.KEYCODE_BUTTON_A);
        padNodes.add(button);


        button = new VPButton();
        button.setEventCallback(callback);
        button.setDrawableForState(VPButton.STATE_UP, getResources().getDrawable(R.drawable.test_btn));
        downRes = getResources().getDrawable(R.drawable.test_btn);
        downRes.setTint(0x80CF0000);
        button.setDrawableForState(VPButton.STATE_DOWN, downRes);
        button.setBounds(200, 400, 200, 200);
        button.setValue(KeyEvent.KEYCODE_BUTTON_B);
        padNodes.add(button);

        VPDPad dpad = new VPDPad();
        dpad.setEventCallback(callback);
        dpad.setBounds(600, 400, 400, 400);
        dpad.setDrawableForState(VPDPad.STATE_DOWN, getResources().getDrawable(R.drawable.analog_0));
        dpad.setDrawableForState(VPDPad.STATE_UP, getResources().getDrawable(R.drawable.analog_0));
        dpad.setDrawableForState(VPDPad.STATE_DIR_LEFT, getResources().getDrawable(R.drawable.analog_l));
        dpad.setDrawableForState(VPDPad.STATE_DIR_LEFT | VPDPad.STATE_DIR_TOP, getResources().getDrawable(R.drawable.analog_ul));
        dpad.setDrawableForState(VPDPad.STATE_DIR_TOP, getResources().getDrawable(R.drawable.analog_u));
        dpad.setDrawableForState(VPDPad.STATE_DIR_RIGHT | VPDPad.STATE_DIR_TOP, getResources().getDrawable(R.drawable.analog_ur));
        dpad.setDrawableForState(VPDPad.STATE_DIR_RIGHT, getResources().getDrawable(R.drawable.analog_r));
        dpad.setDrawableForState(VPDPad.STATE_DIR_RIGHT | VPDPad.STATE_DIR_BOTTOM, getResources().getDrawable(R.drawable.analog_dr));
        dpad.setDrawableForState(VPDPad.STATE_DIR_BOTTOM, getResources().getDrawable(R.drawable.analog_d));
        dpad.setDrawableForState(VPDPad.STATE_DIR_LEFT | VPDPad.STATE_DIR_BOTTOM, getResources().getDrawable(R.drawable.analog_dl));
        dpad.setDirectionValue(VPDPad.STATE_DIR_LEFT, KeyEvent.KEYCODE_DPAD_LEFT);
        dpad.setDirectionValue(VPDPad.STATE_DIR_TOP, KeyEvent.KEYCODE_DPAD_UP);
        dpad.setDirectionValue(VPDPad.STATE_DIR_RIGHT, KeyEvent.KEYCODE_DPAD_RIGHT);
        dpad.setDirectionValue(VPDPad.STATE_DIR_BOTTOM, KeyEvent.KEYCODE_DPAD_DOWN);
        padNodes.add(dpad);
    }

    @Override
    public void draw(@NonNull Canvas canvas) {
        super.draw(canvas);

        for (ComponentBase button : padNodes) {
            button.draw(canvas);
        }

    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        int action = event.getActionMasked();
        if (action == MotionEvent.ACTION_MOVE) {
            onTouchMove(event);
            return super.dispatchTouchEvent(event);
        }

        boolean actionIsDown = action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN;
        int pointerIndex = event.getActionIndex();
        int pointerId = event.getPointerId(pointerIndex);

        int x = (int) event.getX(pointerIndex);
        int y = (int) event.getY(pointerIndex);
        boolean shouldInvalidate = false;
        for (int idx = 0; idx < padNodes.size(); idx++) {
            ComponentBase btn = padNodes.get(idx);
            if (btn.onPointerEvent(pointerId, actionIsDown ? ComponentBase.ACTION_DOWN : ComponentBase.ACTION_UP, x, y)) {
                shouldInvalidate = true;
            }
        }
        if (shouldInvalidate) {
            this.invalidate();
            return true;
        }

        return super.dispatchTouchEvent(event);
    }

    void onTouchMove(MotionEvent event) {
        HashSet<Integer> allPointers = new HashSet<>();
        //get all pointers
        for (int i = 0; i < event.getPointerCount(); i++) {
            allPointers.add(event.getPointerId(i));
        }
        boolean shouldInvalid = false;
        for (ComponentBase node : padNodes) {
            int pointerId = node.getPointerId();
            if (allPointers.contains(pointerId)) {
                if (node.onPointerEvent(pointerId, ComponentBase.ACTION_MOVE, (int) event.getX(pointerId), (int) event.getY(pointerId))) {
                    shouldInvalid = true;
                }
            }
        }
        if (shouldInvalid) {
            this.invalidate();
        }
    }

}
