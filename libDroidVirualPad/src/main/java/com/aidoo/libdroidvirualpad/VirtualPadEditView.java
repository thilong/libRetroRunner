package com.aidoo.libdroidvirualpad;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.ArrayList;

public class VirtualPadEditView extends View {

    boolean isEditing = false;
    ArrayList<ComponentBase> buttons = null;

    public VirtualPadEditView(Context context) {
        super(context);
    }

    public VirtualPadEditView(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    public VirtualPadEditView(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public void setupIfNeeded() {
        if (null != buttons) return;
    }

    public void setEditing(boolean editing) {
        isEditing = editing;
    }

    public boolean isEditing() {
        return isEditing;
    }

    @Override
    public void draw(@NonNull Canvas canvas) {
        super.draw(canvas);

        for (ComponentBase button : buttons) {
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
        int pointerCount = event.getPointerCount();

        float x = event.getX(pointerIndex);
        float y = event.getY(pointerIndex);

        for (int idx = 0; idx < buttons.size(); idx++) {
            VPButton btn = (VPButton) buttons.get(idx);
            int buttonPID = btn.getPointerId();

            if (actionIsDown) {
                if (buttonPID > 0) continue;
                if (btn.isInBounds((int) x, (int) y)) {
                    btn.setPointerId(pointerId);
                    btn.setState(VPButton.STATE_DOWN);
                    this.invalidate(btn.getBounds());
                    return true;
                }
            } else {
                if (pointerId == buttonPID) {
                    btn.setPointerId(-1);
                    btn.setState(VPButton.STATE_UP);
                    this.invalidate(btn.getBounds());
                    return true;
                }
            }
        }

        return super.dispatchTouchEvent(event);
    }

    void onTouchMove(MotionEvent event) {
        //MOVE时需要循环处理每一个pointer
    }

}
