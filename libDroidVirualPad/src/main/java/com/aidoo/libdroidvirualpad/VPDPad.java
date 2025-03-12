package com.aidoo.libdroidvirualpad;

import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.view.KeyEvent;

import java.util.HashMap;

public class VPDPad extends VPComponentBase {

    public static final int STATE_DIR_LEFT = 1 << 1;
    public static final int STATE_DIR_TOP = 1 << 2;
    public static final int STATE_DIR_RIGHT = 1 << 3;
    public static final int STATE_DIR_BOTTOM = 1 << 4;

    private int state = STATE_UP;
    private boolean is8Direction = false;
    private final HashMap<Integer, Drawable> drawables;
    private final HashMap<Integer, Integer> dirValues;

    public VPDPad() {
        drawables = new HashMap<>();
        dirValues = new HashMap<>();
        dirValues.put(STATE_DIR_LEFT, KeyEvent.KEYCODE_DPAD_LEFT);
        dirValues.put(STATE_DIR_TOP, KeyEvent.KEYCODE_DPAD_UP);
        dirValues.put(STATE_DIR_RIGHT, KeyEvent.KEYCODE_DPAD_RIGHT);
        dirValues.put(STATE_DIR_BOTTOM, KeyEvent.KEYCODE_DPAD_DOWN);
    }

    public void setDrawableForState(int state, Drawable drawable) {
        drawables.put(state, drawable);
    }

    public void setDirectionValue(int dir, int value) {
        dirValues.put(dir, value);
    }

    public void setIs8Direction(boolean is8Direction) {
        this.is8Direction = is8Direction;
    }

    @Override
    public void setState(int state) {
        isDirty = false;
        boolean anyChange = false;
        for (int idx = 1; idx < 5; idx++) {
            int dir = 1 << idx;
            boolean prevState = (this.state & dir) != 0;
            boolean nextState = (state & dir) != 0;
            if (prevState != nextState) {
                if (eventCallback != null) {
                    Integer dirValue = dirValues.get(dir);
                    if (dirValue != null) {
                        eventCallback.onButtonEvent(nextState ? ACTION_DOWN : ACTION_UP, dirValue);
                        anyChange = true;
                    }
                }
            }
        }
        if (anyChange) {
            this.state = state;
            isDirty = true;
        }
    }

    @Override
    public boolean onPointerEvent(int pointerId, int action, int x, int y) {
        if (action == ACTION_DOWN && !isInBounds(x, y)) return false;
        if (action == ACTION_UP) {
            if (pointerId == this.pointerId) {
                setPointerId(-1);
                setState(STATE_UP);
                return true;
            }
            return false;
        }
        if (action == ACTION_MOVE) {
            if (pointerId != this.pointerId) {
                return false;
            }
        }
        setPointerId(pointerId);

        int centerX = this.x + this.width / 2;
        int centerY = this.y + this.height / 2;
        //根据(x,y), (centerX, centerY)的距离与角度判断方向
        double dx = x - centerX;
        double dy = y - centerY;
        double distance = Math.sqrt(dx * dx + dy * dy);
        if (distance < width / 4.0) {
            setState(STATE_UP);
            return isDirty;
        }
        double angle = Math.atan2(dy, dx);
        if (is8Direction) {
            calc8Direction(angle);
        } else {
            calc4Direction(angle);
        }
        return isDirty;
    }

    private void calc8Direction(double angle) {
        //判断8个方向
        if (angle >= -Math.PI / 8 && angle < Math.PI / 8) {
            setState(STATE_DIR_RIGHT);
        } else if (angle >= Math.PI / 8 && angle < 3 * Math.PI / 8) {
            setState(STATE_DIR_BOTTOM | STATE_DIR_RIGHT);
        } else if (angle >= 3 * Math.PI / 8 && angle < 5 * Math.PI / 8) {
            setState(STATE_DIR_BOTTOM);
        } else if (angle >= 5 * Math.PI / 8 && angle < 7 * Math.PI / 8) {
            setState(STATE_DIR_BOTTOM | STATE_DIR_LEFT);
        } else if (angle >= 7 * Math.PI / 8 || angle < -7 * Math.PI / 8) {
            setState(STATE_DIR_LEFT);
        } else if (angle >= -7 * Math.PI / 8 && angle < -5 * Math.PI / 8) {
            setState(STATE_DIR_TOP | STATE_DIR_LEFT);
        } else if (angle >= -5 * Math.PI / 8 && angle < -3 * Math.PI / 8) {
            setState(STATE_DIR_TOP);
        } else if (angle >= -3 * Math.PI / 8 && angle < -Math.PI / 8) {
            setState(STATE_DIR_TOP | STATE_DIR_RIGHT);
        }

    }

    private void calc4Direction(double angle) {
        if (angle >= -Math.PI / 4 && angle < Math.PI / 4) {
            setState(STATE_DIR_RIGHT);
        } else if (angle >= Math.PI / 4 && angle < 3 * Math.PI / 4) {
            setState(STATE_DIR_BOTTOM);
        } else if (angle >= 3 * Math.PI / 4 || angle < -3 * Math.PI / 4) {
            setState(STATE_DIR_LEFT);
        } else if (angle >= -3 * Math.PI / 4 && angle < -Math.PI / 4) {
            setState(STATE_DIR_TOP);
        }
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
