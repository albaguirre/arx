package com.ti.arx.ProjectorGallery;

import java.util.Properties;

import javax.microedition.khronos.opengles.GL11;

import android.util.FloatMath;
import android.util.Log;

public abstract class Hotspot implements GLDrawable {

    public interface Listener {
        public void onSelection(Hotspot s);
    }

    public Hotspot(int id, Shape s) {
        mId = id;
        mShape = s;
        mCounter = 0;
        mColorR = 1.0f;
        mColorG = 1.0f;
        mColorB = 1.0f;
        mAlpha = mMinAlpha = 0.1f;
        mX = 0.0f;
        mY = 0.0f;
        mSize = 1.0f;
        mAlphaSize = 3.0f * mSize;
        mDelay = 30;
        mRetrigger = false;
    }

    public abstract boolean isInside(float x, float y);

    @Override
    public void draw(GL11 gl) {
        gl.glPushMatrix();
        gl.glTranslatef(mX, mY, 0.0f);
        gl.glScalef(mSize, mSize, 1.0f);
        if (mHovering) {
            gl.glColor4f(mColorR, mColorG, mColorB, mAlpha);
        } else {
            gl.glColor4f(1.0f, 1.0f, 1.0f, mAlpha);
        }
        mShape.draw(gl);
        gl.glPopMatrix();
    }

    @Override
    public void setup(GL11 gl, Properties props) {
        mShape.setup(gl, props);
    }

    public void configure(float x, float y, float size, int delay) {
        mX = x;
        mY = y;
        mSize = size;
        mAlphaSize = 3.0f*size;
        mDelay = delay;
        Log.d("Hotspot", String.format("id:%d [x:%1.2f y:%1.2f] size:%1.2f delay:%d", mId, x, y, size, delay));
    }

    public void setListener(Listener l) {
        mListener = l;
    }

    public int getID() {
        return mId;
    }

    public void onPointerEvent(float x, float y) {
        float d = distance(x, y);
        mAlpha = (d-mAlphaSize)*((1.0f-mMinAlpha)/-mAlphaSize) + mMinAlpha;
        if (mAlpha < mMinAlpha) {
            mAlpha = mMinAlpha;
        }
        if (isInside(x, y)) {
            mHovering = true;
            mCounter++;
            if (mCounter <= mDelay) {
                double value = (double)mCounter/(double)mDelay;
                mColorR = mColorB = (float)Math.log10(-9.0*value+10.001);
            } else {
                mColorR = mColorB = mColorG = 1.0f;
            }
        } else {
            mHovering = false;
            reset();
        }
        if (mCounter == mDelay && mListener != null) {
            mListener.onSelection(this);
            if (mRetrigger) reset();
        }
    }

    public float distance(float x, float y) {
        float xd = x-mX;
        float yd = y-mY;
        return FloatMath.sqrt(xd*xd + yd*yd);
    }

    private void reset() {
        mCounter = 0;
        mColorR = mColorB = mColorG = 1.0f;
    }

    private int mId;
    protected float mX;
    protected float mY;
    protected float mSize;
    private float mColorR;
    private float mColorG;
    private float mColorB;
    private float mAlpha;
    private float mMinAlpha;
    private float mAlphaSize;
    private Shape mShape;
    private Listener mListener;
    private boolean mHovering;
    private int mCounter;
    private int mDelay;
    private boolean mRetrigger;

    public static final int PREV = 0;
    public static final int ZOOMIN = 1;
    public static final int NEXT = 2;
    public static final int ZOOMOUT = 3;

}
