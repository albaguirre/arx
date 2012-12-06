package com.ti.arx.ProjectorGallery;

import java.util.ArrayList;
import java.util.Properties;

import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

public class UILayer implements GLDrawable {

    public UILayer() {
        mAspectRatio = 1.0f;
        mSpots = new ArrayList<Hotspot>(4);
        mSize = 0.2f;
        mDelay = 30;
        mListener = null;
    }

    @Override
    public void draw(GL11 gl) {
        gl.glEnable(GL10.GL_BLEND);
        for (Hotspot s : mSpots) {
            s.draw(gl);
        }
        gl.glDisable(GL10.GL_BLEND);
    }

    @Override
    public void setup(GL11 gl, Properties props) {
        gl.glBlendFunc(GL10.GL_SRC_ALPHA, GL10.GL_ONE_MINUS_SRC_ALPHA);

        String value = props.getProperty("-hotSpotSize");
        if (value != null) {
            mSize = Float.parseFloat(value);
        }

        value = props.getProperty("-hotSpotActionDelay");
        if (value != null) {
            mDelay = Integer.parseInt(value);
        }

        value = props.getProperty("-hotSpots");
        if (value != null) {
            String[] coords = value.split(" ");
            if ((coords.length % 2) == 0) {
                int id = Hotspot.PREV;
                for (int i = 0; i < coords.length; i+= 2) {
                    float x = Float.parseFloat(coords[i]);
                    float y = Float.parseFloat(coords[i+1]);
                    addSpot(id, x, y);
                    id++;
                }
            }
        }

        for (Hotspot s : mSpots) {
            s.setup(gl, props);
        }
    }

    public void onPointerEvent(float x, float y) {
        for (Hotspot s : mSpots) {
            s.onPointerEvent(x, y);
        }
    }

    public void setListener(Hotspot.Listener l) {
        mListener = l;
    }

    public void setAspectRatio(float ar) {
        mAspectRatio = ar;
    }

    private void addSpot(int id, float x, float y) {
        x = (2.0f*x - 1.0f);
        y = (-2.0f*y + 1.0f);
        y /= mAspectRatio;
        Hotspot s = new CircleHotspot(id);
        //Hotspot s = new SquareHotspot(id);
        s.configure(x, y, mSize, mDelay);
        s.setListener(mListener);
        mSpots.add(s);
    }

    private Hotspot.Listener mListener;
    private ArrayList<Hotspot> mSpots;
    private int mDelay;
    private float mSize;
    private float mAspectRatio;
}
