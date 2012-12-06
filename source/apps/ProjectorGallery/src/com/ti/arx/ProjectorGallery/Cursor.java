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

import javax.microedition.khronos.opengles.GL11;

public class Cursor implements GLDrawable {

    public Cursor() {
        mCross = Shape.createCross();
        mSize = 0.05f;
        mLineWidth = 2.0f;
    }

    public void draw(GL11 gl) {
        gl.glLineWidth(mLineWidth);
        gl.glPushMatrix();
        gl.glScalef(mSize, mSize, 1.0f);
        mCross.draw(gl);
        gl.glPopMatrix();
    }

    public void setSize(float size, float lineWidth) {
        mSize = size;
        mLineWidth = lineWidth;
    }

    public void setup(GL11 gl, Properties props) {
        String value = props.getProperty("-markerSize");
        if (value != null) {
            mSize = Float.parseFloat(value);
        }

        value = props.getProperty("-markerThickness");
        if (value != null) {
            mLineWidth = Float.parseFloat(value);
        }

        mCross.setup(gl, props);
    }

    private Shape mCross;
    private float mLineWidth;
    private float mSize;

}


