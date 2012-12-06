/*
 * Copyright (C) 2011 Texas Instruments Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.ti.arx.ProjectorGallery;

import java.util.Properties;

import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.opengl.GLUtils;
import android.util.Log;

public class Texture implements GLObject {

    public Texture(String fileName) {
        mFileName = fileName;
        mAspectRatio = 1.0f;
    }

    public Texture(Context c, int id) {
        mFileName = null;
        mContext = c;
        mResourceId = id;
    }

    public void bind(GL11 gl) {
        gl.glActiveTexture(GL10.GL_TEXTURE0);
        gl.glEnable(GL10.GL_TEXTURE_2D);
        gl.glBindTexture(GL10.GL_TEXTURE_2D, mTextureID);
        gl.glPushMatrix();
        gl.glScalef(1.0f, 1.0f/mAspectRatio, 1.0f);
    }

    public void unbind(GL11 gl) {
        gl.glPopMatrix();
        gl.glBindTexture(GL10.GL_TEXTURE_2D, 0);
        gl.glDisable(GL10.GL_TEXTURE_2D);
    }

    public void setup(GL11 gl, Properties props) {
        Bitmap b = null;

        if (mFileName != null) {
            b = BitmapFactory.decodeFile(mFileName);
        } else {
            b = BitmapFactory.decodeResource(mContext.getResources(), mResourceId);
        }

        if (b == null) {
            Log.e(TAG, "Failed decoding image!");
            return;
        }

        mAspectRatio = (float)b.getWidth()/(float)b.getHeight();
        int potSize = 1 << (int)Math.ceil(Math.log(b.getWidth())/Math.log(2));
        if (potSize > MAX_TEXTURE_SIZE) {
            potSize = MAX_TEXTURE_SIZE;
        }
        Bitmap b2 = Bitmap.createScaledBitmap(b, potSize, potSize, true);
        if (!b2.equals(b)) {
            b.recycle();
        }
        int[] ids = new int[1];
        gl.glGenTextures(1, ids, 0);
        mTextureID = ids[0];

        gl.glBindTexture(GL10.GL_TEXTURE_2D, mTextureID);

        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_LINEAR);
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
        GLUtils.texImage2D(GL10.GL_TEXTURE_2D, 0, b2, 0);

        b2.recycle();
    }

    public float getAspectRatio() {
        return mAspectRatio;
    }

    private String mFileName;
    private float mAspectRatio;
    private int mTextureID;

    private Context mContext;
    private int mResourceId;

    private static final int MAX_TEXTURE_SIZE = 1024;
    private static final String TAG = "Texture";

}
