package com.aidoo.libdroidvirualpad;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

import java.util.ArrayList;
import java.util.HashSet;

public class VPView extends View {

    boolean isEditing = false;
    ArrayList<VPComponentBase> padNodes = null;
    VPEventCallback callback = null;

    public VPView(Context context) {
        super(context);
    }

    public VPView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public VPView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public void setupIfNeeded(VPEventCallback callback) {
        if (null != padNodes) return;
        this.callback = callback;
        VPComponentCreator creator = new VPComponentCreator().withContext(this.getContext());
        padNodes = new ArrayList<>();
        VPComponentBase button = creator.createComponent(VPComponentCreator.VPComponentType.BUTTON_A);
        button.setPosition(100, 100);
        padNodes.add(button);

        button = creator.createComponent(VPComponentCreator.VPComponentType.BUTTON_B);
        button.setPosition(300, 100);
        padNodes.add(button);

        button = creator.createComponent(VPComponentCreator.VPComponentType.BUTTON_START);
        button.setPosition(100, 300);
        padNodes.add(button);

        VPComponentBase dpad = creator.createComponent(VPComponentCreator.VPComponentType.DPAD);
        dpad.setPosition(300, 300);
        dpad.setEventCallback(this.callback);
        padNodes.add(dpad);
    }

    @Override
    public void draw(Canvas canvas) {
        super.draw(canvas);

        for (VPComponentBase button : padNodes) {
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
            VPComponentBase btn = padNodes.get(idx);
            if (btn.onPointerEvent(pointerId, actionIsDown ? VPComponentBase.ACTION_DOWN : VPComponentBase.ACTION_UP, x, y)) {
                shouldInvalidate = true;
                break;
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
        for (VPComponentBase node : padNodes) {
            int pointerId = node.getPointerId();
            if (allPointers.contains(pointerId)) {
                if (node.onPointerEvent(pointerId, VPComponentBase.ACTION_MOVE, (int) event.getX(pointerId), (int) event.getY(pointerId))) {
                    shouldInvalid = true;
                }
            }
        }
        if (shouldInvalid) {
            this.invalidate();
        }
    }

}
