package com.aidoo.libdroidvirualpad;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;

public class VPComponentBase {
    public static final int STATE_UP = 0;
    public static final int STATE_DOWN = 1;

    public static final int ACTION_UP = 0;
    public static final int ACTION_DOWN = 1;
    public static final int ACTION_MOVE = 2;

    public static final Paint editingPaint = new Paint();

    static {
        editingPaint.setARGB(0x30, 0x00, 0x00, 0xFF);
        editingPaint.setStyle(Paint.Style.FILL_AND_STROKE);
        editingPaint.setStrokeWidth(1);
    }

    protected int id = -1;
    protected int value = 0;

    protected int pointerId = -1;

    protected int x, y;
    protected int width, height;
    protected boolean isEditing;
    protected boolean isDirty;

    protected VPEventCallback eventCallback;

    public void draw(Canvas canvas) {
    }

    public boolean onPointerEvent(int pointerId, int action, int x, int y) {
        return false;
    }

    public boolean isInBounds(int x, int y) {
        return x >= this.x && x <= this.x + width && y >= this.y && y <= this.y + height;
    }

    public Rect getBounds() {
        return new Rect(x, y, x + width, y + height);
    }

    public void setBounds(int x, int y, int width, int height) {
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;
        isDirty = true;
    }

    public int getId() {
        return id;
    }

    public void setId(int id) {
        this.id = id;
    }

    public void setEventCallback(VPEventCallback eventCallback) {
        this.eventCallback = eventCallback;
    }

    public int getValue() {
        return value;
    }

    public void setValue(int value) {
        this.value = value;
    }

    public int getPointerId() {
        return pointerId;
    }

    public void setPointerId(int pointerId) {
        this.pointerId = pointerId;
    }

    public int getX() {
        return x;
    }

    public void setX(int x) {
        this.x = x;
        isDirty = true;
    }

    public int getY() {
        return y;
    }

    public void setY(int y) {
        this.y = y;
        isDirty = true;
    }

    public void setPosition(int x, int y) {
        this.x = x;
        this.y = y;
        isDirty = true;
    }

    public int getWidth() {
        return width;
    }

    public void setWidth(int width) {
        this.width = width;
        isDirty = true;
    }


    public int getHeight() {
        return height;
    }

    public void setHeight(int height) {
        this.height = height;
        isDirty = true;
    }

    public boolean isEditing() {
        return isEditing;
    }

    public void setEditing(boolean editing) {
        isEditing = editing;
        isDirty = true;
    }

    public void setState(int state) {
    }
}
