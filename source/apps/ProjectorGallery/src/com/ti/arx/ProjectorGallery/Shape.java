package com.ti.arx.ProjectorGallery;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.Properties;

import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

import android.util.FloatMath;

public class Shape implements GLDrawable {

    public Shape(int mode, int numCoordsPerVertex) {
        mTexture = null;
        mMode = mode;
        mCoordsPerVertex = numCoordsPerVertex;
    }

    public Shape(float[] vertices, float[] texCoords, byte[] indices, int mode, int numCoordsPerVertex) {
        this(mode, numCoordsPerVertex);

        ByteBuffer byteBuf = ByteBuffer.allocateDirect(vertices.length * FLOAT_SIZE);
        byteBuf.order(ByteOrder.nativeOrder());
        mVertices = byteBuf.asFloatBuffer();
        mVertices.put(vertices);
        mVertices.position(0);

        if (texCoords != null) {
            byteBuf = ByteBuffer.allocateDirect(texCoords.length * FLOAT_SIZE);
            byteBuf.order(ByteOrder.nativeOrder());
            mTexCoords = byteBuf.asFloatBuffer();
            mTexCoords.put(texCoords);
            mTexCoords.position(0);
        } else {
            mTexCoords = null;
        }

        if (indices != null) {
            mIndices = ByteBuffer.allocateDirect(indices.length);
            mIndices.put(indices);
            mIndices.position(0);
        } else {
            mIndices = null;
        }
    }

    public Shape(float[] vertices, int mode, int numCoordsPerVertex) {
        this(vertices, null, null, mode, numCoordsPerVertex);
    }

    public Shape(float[] vertices, byte[] indices, int mode, int numCoordsPerVertex) {
        this(vertices, null, indices, mode, numCoordsPerVertex);
    }

    public Shape(FloatBuffer vertices, int mode, int numCoordsPerVertex) {
        this(mode, numCoordsPerVertex);
        mVertices = vertices;
        mVertices.position(0);
    }

    public void setTexture(Texture tex) {
        mTexture = tex;
    }

    public void draw(GL11 gl) {
        gl.glFrontFace(GL10.GL_CCW);

        if (mTexture != null) {
            gl.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
            gl.glBindBuffer(GL11.GL_ARRAY_BUFFER, mTexCoordsVboID);
            gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, 0);
            mTexture.bind(gl);
        }

        gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
        gl.glBindBuffer(GL11.GL_ARRAY_BUFFER, mVtxVboID);
        gl.glVertexPointer(2, GL10.GL_FLOAT, 0, 0);

        if (mIndices != null) {
            gl.glBindBuffer(GL11.GL_ELEMENT_ARRAY_BUFFER, mIndVboID);
            gl.glDrawElements(mMode, mIndices.capacity(), GL10.GL_UNSIGNED_BYTE, 0);
            gl.glBindBuffer(GL11.GL_ELEMENT_ARRAY_BUFFER, 0);
        } else {
            gl.glDrawArrays(mMode, 0, mVertices.capacity()/mCoordsPerVertex);
        }

        if (mTexture != null) {
            gl.glDisableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
            mTexture.unbind(gl);
        }

        gl.glBindBuffer(GL11.GL_ARRAY_BUFFER, 0);
        gl.glDisableClientState(GL10.GL_VERTEX_ARRAY);
    }

    public void setup(GL11 gl, Properties props) {
        int size = 1;
        if (mIndices != null) size++;
        if (mTexCoords != null) size++;

        int[] ids = new int[size];
        gl.glGenBuffers(size, ids, 0);

        int idx = 0;
        mVtxVboID = ids[idx++];
        if (mIndices != null) mIndVboID = ids[idx++];
        if (mTexCoords != null) mTexCoordsVboID = ids[idx];

        gl.glBindBuffer(GL11.GL_ARRAY_BUFFER, mVtxVboID);
        gl.glBufferData(GL11.GL_ARRAY_BUFFER, mVertices.capacity()*FLOAT_SIZE, mVertices, GL11.GL_STATIC_DRAW);
        gl.glBindBuffer(GL11.GL_ARRAY_BUFFER, 0);

        if (mTexCoords != null) {
            gl.glBindBuffer(GL11.GL_ARRAY_BUFFER, mTexCoordsVboID);
            gl.glBufferData(GL11.GL_ARRAY_BUFFER, mTexCoords.capacity()*FLOAT_SIZE, mTexCoords, GL11.GL_STATIC_DRAW);
            gl.glBindBuffer(GL11.GL_ARRAY_BUFFER, 0);
        }

        if (mIndices != null) {
            gl.glBindBuffer(GL11.GL_ELEMENT_ARRAY_BUFFER, mIndVboID);
            gl.glBufferData(GL11.GL_ELEMENT_ARRAY_BUFFER, mIndices.capacity(), mIndices, GL11.GL_STATIC_DRAW);
            gl.glBindBuffer(GL11.GL_ELEMENT_ARRAY_BUFFER, 0);
        }
    }

    public static Shape createQuad() {
        float vertices[] = {
            -1.0f, -1.0f,
            1.0f, -1.0f,
            -1.0f, 1.0f,
            1.0f, 1.0f,
        };

        float texCoords[] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f,
        };

        byte indices[] = {
            0,1,3, 0,3,2
        };
        return new Shape(vertices, texCoords, indices, GL10.GL_TRIANGLES, 2);
    }

    public static Shape createCross() {
        float vertices[] = {
            -1.0f, 0.0f,
            1.0f, 0.0f,
            0.0f, -1.0f,
            0.0f, 1.0f,
        };
        return new Shape(vertices, GL10.GL_LINES, 2);
    }

    public static Shape createCircle() {
        int numSegments = 360;
        ByteBuffer byteBuf = ByteBuffer.allocateDirect(2 * numSegments * FLOAT_SIZE);
        byteBuf.order(ByteOrder.nativeOrder());
        FloatBuffer vertices = byteBuf.asFloatBuffer();

        int inc = 360/(numSegments);
        for (int i = 0; i < numSegments; i += inc)
        {
            float rad = (float)(i*Math.PI/180.0);
            float x = FloatMath.cos(rad);
            float y = FloatMath.sin(rad);
            vertices.put(x);
            vertices.put(y);
        }
        return new Shape(vertices, GL10.GL_TRIANGLE_FAN, 2);
    }

    private FloatBuffer mVertices;
    private FloatBuffer mTexCoords;
    private ByteBuffer mIndices;
    private int mVtxVboID;
    private int mTexCoordsVboID;
    private int mIndVboID;
    private Texture mTexture;
    private int mCoordsPerVertex;
    private int mMode;
    private static final int FLOAT_SIZE = Float.SIZE/8;
}
