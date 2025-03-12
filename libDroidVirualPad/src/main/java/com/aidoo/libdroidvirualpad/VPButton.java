package com.aidoo.libdroidvirualpad;

import android.graphics.Canvas;
import android.graphics.drawable.Drawable;

import java.util.HashMap;

public class VPButton extends VPComponentBase {

    private int state = STATE_UP;

    private final HashMap<Integer, Drawable> drawables;

    public VPButton() {
        drawables = new HashMap<>();
    }

    public void setDrawableForState(int state, Drawable drawable) {
        drawables.put(state, drawable);
    }

    @Override
    public void setState(int state) {
        this.state = state;
        if (eventCallback != null) {
            eventCallback.onButtonEvent(state, value);
        }
        isDirty = true;
    }

    @Override
    public boolean onPointerEvent(int pointerId, int action, int x, int y) {
        switch (action) {
            case ACTION_DOWN:
                if (isInBounds(x, y)) {
                    setPointerId(pointerId);
                    setState(STATE_DOWN);
                    return true;
                }
                break;
            case ACTION_UP:
                if (pointerId == this.pointerId) {
                    setState(STATE_UP);
                    return true;
                }
                break;
            case ACTION_MOVE:
                if (pointerId != this.pointerId) return false;
                if (!isInBounds(x, y)) {
                    setState(STATE_UP);
                    setPointerId(-1);
                    return true;
                }
                break;
        }
        return false;
    }

    @Override
    public void draw(Canvas canvas) {
        super.draw(canvas);
        Drawable drawable = isEditing ? drawables.get(STATE_UP) : drawables.get(state);

        if (null != drawable) {
            drawable.setBounds(x, y, x + width, y + height);
            drawable.draw(canvas);
        }

        if (isEditing) {
            canvas.drawRect(x, y, x + width, y + height, editingPaint);
        }

    }

}
