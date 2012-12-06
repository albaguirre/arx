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

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.View;
import android.view.KeyEvent;
import android.view.View.OnClickListener;


/**
 * @brief
 */
public class ProjectorGalleryActivity extends Activity {
    /**
     * @brief Overridden to create and display an instance of our custom view.
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mRenderer = new ProjectorPoseRenderer(this);
        mView = new GLSurfaceView(this);
        mView.setEGLConfigChooser(8, 8, 8, 8, 16, 0);
        mRenderer.setGLView(mView);
        mView.setRenderer(mRenderer);

        TrackerResetOnClick trackerReset = new TrackerResetOnClick();
        mView.setOnClickListener(trackerReset);
        mView.setClickable(true);

        mView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
        setContentView(mView);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_DPAD_UP) {
           mRenderer.zoomIn();
           return true;
        }
        if (keyCode == KeyEvent.KEYCODE_DPAD_DOWN) {
            mRenderer.zoomOut();
            return true;
        }
        if (keyCode == KeyEvent.KEYCODE_DPAD_LEFT) {
            mRenderer.prev();
            return true;
        }
        if (keyCode == KeyEvent.KEYCODE_DPAD_RIGHT) {
            mRenderer.next();
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    private class TrackerResetOnClick implements OnClickListener {
        @Override
        public void onClick(View v) {
            ProjectorPoseRenderer r = ProjectorGalleryActivity.this.mRenderer;
            if (r != null) {
                r.reset();
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        mRenderer.start();
    }

    @Override
    protected void onPause() {
        super.onPause();
        mRenderer.stop();
    }

    private GLSurfaceView mView;
    private ProjectorPoseRenderer mRenderer;
}
