/*
 *  Copyright (C) 2012 Texas Instruments, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.ti.arx.ProjectorGallery;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Properties;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.opengl.GLU;

import android.util.Log;

public class ProjectorPoseRenderer implements GLSurfaceView.Renderer {

    public ProjectorPoseRenderer(Context context) {
        mProjectorK = new float[9];
        mProjMatrix = new float[16];
        mViewMatrix = new float[16];

        mSetupList = new ArrayList<GLObject>();
        mDrawableList = new ArrayList<GLDrawable>();
        mNoTransformDrawableList = new ArrayList<GLDrawable>();

        mTextureList = new CircularTextureList(context);
        mImageQuad = Shape.createQuad();
        mCursor = new Cursor();
        mUI = new UILayer();
        mUI.setListener(new UIListener());

        mSetupList.add(mTextureList);
        mSetupList.add(mImageQuad);
        mSetupList.add(mCursor);
        mSetupList.add(mUI);

        mDrawableList.add(mImageQuad);
        mDrawableList.add(mUI);

        mNoTransformDrawableList.add(mCursor);

        mProperties = new Properties();
        mDraw = false;
        mDrawUI = false;

        mZoom = 1.0f;
        mSceneX = 0.0f;
        mSceneY = 0.0f;
        mSceneZ = 0.0f;
        mZoomStep = 4.0f;
        mMinZoom = 20.0f;
        mMaxZoom = 50.0f;
    }

    public void setGLView(GLSurfaceView view) {
        mView = view;
    }

    public boolean start() {
        return create(mViewMatrix);
    }

    public void stop() {
        destroy();
    }

    @Override
    public void onDrawFrame(GL10 gl10) {
        if (!(gl10 instanceof GL11)) return;
        GL11 gl = (GL11)gl10;

        gl.glClear(GL10.GL_COLOR_BUFFER_BIT | GL10.GL_DEPTH_BUFFER_BIT);
        gl.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        gl.glDisable(GL10.GL_BLEND);

        if (!mDraw) return;

        gl.glMatrixMode(GL10.GL_PROJECTION);
        gl.glLoadIdentity();
        if (mUsePose) {
            gl.glLoadMatrixf(mProjMatrix, 0);
        } else {
            GLU.gluOrtho2D(gl, -1.0f, 1.0f, -1.0f/mAspectRatio, 1.0f/mAspectRatio);
        }

        gl.glMatrixMode(GL10.GL_MODELVIEW);
        gl.glLoadIdentity();
        if (mUsePose) {
            gl.glLoadMatrixf(mViewMatrix, 0);
        }

        if (mUsePose) {
            gl.glTranslatef(mSceneX, mSceneY, mSceneZ);
            gl.glScalef(mZoom, mZoom, 1.0f);
        }

        for (GLDrawable d : mDrawableList) {
            d.draw(gl);
        }

        gl.glMatrixMode(GL10.GL_PROJECTION);
        gl.glLoadIdentity();
        gl.glMatrixMode(GL10.GL_MODELVIEW);
        gl.glLoadIdentity();
        GLU.gluOrtho2D(gl, -1.0f, 1.0f, -1.0f/mAspectRatio, 1.0f/mAspectRatio);

        for (GLDrawable d : mNoTransformDrawableList) {
            d.draw(gl);
        }
    }

    @Override
    public void onSurfaceChanged(GL10 gl10, int width, int height) {
        if (! (gl10 instanceof GL11)) {
            return;
        }
        loadCalibration();
        loadParams();
        setup(mProperties);

        mProjMatrix[0] = 2*mProjectorK[0]/mProjectorW;
        mProjMatrix[4] = mProjectorK[1];
        mProjMatrix[8] = 2*(mProjectorK[2]/mProjectorW) - 1;
        mProjMatrix[12] = 0;

        mProjMatrix[1] = mProjectorK[3];
        mProjMatrix[5] = 2*mProjectorK[4]/mProjectorH;
        mProjMatrix[9] = 2*(mProjectorK[5]/mProjectorH) - 1;
        mProjMatrix[13] = 0;

        mProjMatrix[2] = mProjectorK[6];
        mProjMatrix[6] = mProjectorK[7];
        mProjMatrix[10] = -(FRUSTUM_NEAR_Z + FRUSTUM_FAR_Z)/(FRUSTUM_FAR_Z - FRUSTUM_NEAR_Z);
        mProjMatrix[14] = -(2*FRUSTUM_NEAR_Z*FRUSTUM_FAR_Z)/(FRUSTUM_FAR_Z - FRUSTUM_NEAR_Z);

        mProjMatrix[3] = 0;
        mProjMatrix[7] = 0;
        mProjMatrix[11] = -1;
        mProjMatrix[15] = 0;

        mAspectRatio = (float)width/(float)height;
        mUI.setAspectRatio(mAspectRatio);


        GL11 gl = (GL11)gl10;
        gl.glViewport(0, 0, width, height);
        //gl.glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        gl.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        gl.glEnable(GL10.GL_CULL_FACE);

        for (GLObject o : mSetupList) {
            o.setup(gl, mProperties);
        }

        mImageQuad.setTexture(mTextureList.getCurrentTexture());
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {}

    private void loadCalibration() {
        File file = new File("/data/misc/Parameters/ProjectorCalibration.txt");
        if (!file.exists()) return;

        try {
            BufferedReader r = new BufferedReader (new FileReader(file));
            String line = r.readLine();
            r.close();
            String[] values = line.split(" ");
            if (values.length == mProjectorK.length) {
                for (int i = 0; i < values.length; i++) {
                    float value = Float.parseFloat(values[i]);
                    Log.d(TAG, "projector_k["+i+"]="+value);
                    mProjectorK[i] = value;
                }
            }
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void loadParams() {
        File file = new File("/data/misc/Parameters/galleryParameters.txt");
        FileInputStream fis;
        try {
            fis = new FileInputStream(file);
            mProperties.load(fis);
            Log.d(TAG, "properties: "+mProperties);
        } catch (FileNotFoundException e1) {
            e1.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void setup(Properties props) {
        String value = props.getProperty("-objX");
        if (value != null) {
            mSceneX = Float.parseFloat(value);
            Log.d(TAG, "mSceneX: "+mSceneX);
        }
        value = props.getProperty("-objY");
        if (value != null) {
            mSceneY = Float.parseFloat(value);
            Log.d(TAG, "mSceneY: "+mSceneY);
        }
        value = props.getProperty("-objZ");
        if (value != null) {
            mSceneZ = Float.parseFloat(value);
            Log.d(TAG, "mSceneZ: "+mSceneZ);
        }
        value = props.getProperty("-objScale");
        if (value != null) {
            mZoom = Float.parseFloat(value);
        }
        value = props.getProperty("-objScaleStep");
        if (value != null) {
            mZoomStep = Float.parseFloat(value);
        }
        value = props.getProperty("-objScaleMax");
        if (value != null) {
            mMaxZoom = Float.parseFloat(value);
        }
        value = props.getProperty("-objScaleMin");
        if (value != null) {
            mMinZoom = Float.parseFloat(value);
        }
        value = props.getProperty("-projW");
        if (value != null) {
            mProjectorW = Integer.parseInt(value);
            Log.d(TAG, "mProjectorW: "+mProjectorW);
        }
        value = props.getProperty("-projH");
        if (value != null) {
            mProjectorH = Integer.parseInt(value);
            Log.d(TAG, "mProjectorH: "+mProjectorH);
        }
    }

    private void onProjPoseUpdate(int status, float x, float y) {
        mDraw = status != STATUS_LOST_TRACKING;
        mUsePose = status != STATUS_NOFEATURES;
        //mUseIdentity = true;
        //mDrawUI = false;
        //Log.d(TAG, String.format("rx [%1.2f %1.2f %1.2f %1.2f]", mViewMatrix[0], mViewMatrix[1], mViewMatrix[2], mViewMatrix[3]));
        //Log.d(TAG, String.format("ry [%1.2f %1.2f %1.2f %1.2f]", mViewMatrix[4], mViewMatrix[5], mViewMatrix[6], mViewMatrix[7]));
        //Log.d(TAG, String.format("rz [%1.2f %1.2f %1.2f %1.2f]", mViewMatrix[8], mViewMatrix[9], mViewMatrix[10], mViewMatrix[11]));
        //Log.d(TAG, String.format("t  [%1.2f %1.2f %1.2f %1.2f]", mViewMatrix[12], mViewMatrix[13], mViewMatrix[14], mViewMatrix[15]));
        //Log.d(TAG, String.format("update - status:%d [x:%1.2f y:%1.2f]", status, x, y));
        mUI.onPointerEvent(x, y);
        mView.requestRender();
    }

    public void zoomIn() {
        mZoom += mZoomStep;
        if (mZoom > mMaxZoom) {
            mZoom = mMaxZoom;
        }
        Log.d(TAG, "zoom in to: "+mZoom);
        zoomChanged(mZoom);
    }

    public void zoomOut() {
        mZoom -= mZoomStep;
        if (mZoom < mMinZoom) {
            mZoom = mMinZoom;
        }
        Log.d(TAG, "zoom out to: "+mZoom);
        zoomChanged(mZoom);
    }

    public void next() {
        Texture tex = mTextureList.next();
        mImageQuad.setTexture(tex);
    }

    public void prev() {
        Texture tex = mTextureList.prev();
        mImageQuad.setTexture(tex);
    }

    private void onARXDeath() {
        stop();
        start();
    }

    private class UIListener implements Hotspot.Listener {

        @Override
        public void onSelection(Hotspot s) {
            switch (s.getID()) {
            case Hotspot.ZOOMIN:
                zoomIn();
                break;
            case Hotspot.ZOOMOUT:
                zoomOut();
                break;
            case Hotspot.PREV:
                prev();
                break;
            case Hotspot.NEXT:
                next();
                break;
            }
        }
    }

    public native void reset();
    private native void zoomChanged(float value);
    private native boolean create(float[] projPose);
    private native void destroy();

    private boolean mDraw;
    private boolean mUsePose;
    private boolean mDrawUI;

    private GLSurfaceView mView;
    private ArrayList<GLObject> mSetupList;
    private ArrayList<GLDrawable> mDrawableList;
    private ArrayList<GLDrawable> mNoTransformDrawableList;
    private Properties mProperties;

    private Shape mImageQuad;
    private CircularTextureList mTextureList;
    private Cursor mCursor;
    private UILayer mUI;
    private float mAspectRatio;

    private float[] mProjMatrix;
    private float[] mProjectorK;
    private int mProjectorW;
    private int mProjectorH;

    private float mZoom;
    private float mZoomStep;
    private float mMinZoom;
    private float mMaxZoom;

    private float mSceneX;
    private float mSceneY;
    private float mSceneZ;

    ///////////////////////////////////////////////////////////
    // These fields are initialized and/or updated by JNI layer
    private float[] mViewMatrix;
    private int context;
    /////////////////////////////////////////////////////////

    private static final float FRUSTUM_NEAR_Z = 0.01f;
    private static final float FRUSTUM_FAR_Z = 500.0f;

    private static final int HOTSPOT_PREV = 0;
    private static final int HOTSPOT_NEXT = 2;
    private static final int HOTSPOT_ZOOMOUT = 3;
    private static final int HOTSPOT_ZOOMIN = 1;

    private static final int STATUS_LOST_TRACKING = 0;
    private static final int STATUS_TRACKING = 1;
    private static final int STATUS_TRACKING_AND_MOVING = 2;
    private static final int STATUS_NOFEATURES = 3;

    private static final String TAG = "ProjectorPoseRenderer";

    static {
        System.loadLibrary("arprojector_jni");
    }
}
