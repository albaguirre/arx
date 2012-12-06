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

import java.io.File;
import java.io.FilenameFilter;
import java.util.ArrayList;
import java.util.Properties;

import javax.microedition.khronos.opengles.GL11;

import android.content.Context;
import android.os.Environment;
import android.util.Log;

public class CircularTextureList implements GLObject {

    public CircularTextureList(Context c, String pathName, int maxNumImagesToLoad) {
        mMaxNumImagesToLoad = maxNumImagesToLoad;
        if (mMaxNumImagesToLoad == 0) {
            mMaxNumImagesToLoad = Integer.MAX_VALUE;
        }
        mPaths = new ArrayList<File>(3);
        mPaths.add(new File(pathName));
        mPaths.add(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES));
        mContext = c;
    }

    public CircularTextureList(Context c) {
        this(c, "/storage/sdcard1/images", Integer.MAX_VALUE);
    }

    public void setup(GL11 gl, Properties props) {
        loadTextures();
        for (Texture t : mTextures) {
            t.setup(gl, props);
        }
    }

    public Texture next() {
        mCurIdx++;
        if (mCurIdx >= mTextures.length) {
            mCurIdx = 0;
        }
        return getCurrentTexture();
    }

    public Texture prev() {
         mCurIdx--;
         if (mCurIdx < 0) {
             mCurIdx = mTextures.length -1;
         }
         return getCurrentTexture();
    }

    public Texture getCurrentTexture() {
        return mTextures[mCurIdx];
    }

    public void loadTextures() {
        File[] files = null;
        for (File path : mPaths) {
            Log.d(TAG, "Looking for images in: "+path.toString());
            files = listFiles(path);
            if (files != null) break;
        }

        if (files == null || files.length == 0) {
            Log.d(TAG, "No files found, loading built-in image");
            mTextures = new Texture[1];
            mTextures[0] = new Texture(mContext, R.drawable.dlp);
        } else {
            int size = Math.min(files.length, mMaxNumImagesToLoad);
            mTextures = new Texture[size];
            for (int i = 0; i < size; i++) {
                Log.d(TAG, "found: "+files[i].toString());
                mTextures[i] = new Texture(files[i].getAbsolutePath());
            }
        }
        mCurIdx = 0;
    }

    private File[] listFiles(File path) {
        if (path == null || !path.exists()) {
            Log.e(TAG, "Error opening path: "+path.toString());
            return null;
        }

        FilenameFilter filter = new ImageNameFilter();
        File[] files = path.listFiles(filter);
        return files;
    }

    private ArrayList<File> mPaths;
    private int mMaxNumImagesToLoad;
    private Context mContext;

    private Texture[] mTextures;
    private int mCurIdx;

    private static final String TAG = "CircularTextureList";
}
