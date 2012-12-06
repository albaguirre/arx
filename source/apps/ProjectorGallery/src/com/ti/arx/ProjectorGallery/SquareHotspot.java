package com.ti.arx.ProjectorGallery;

public class SquareHotspot extends Hotspot {

    public SquareHotspot(int id) {
        super(id, Shape.createQuad());
    }

    @Override
    public boolean isInside(float x, float y) {
        return (x > (mX - mSize) && x < (mX + mSize) && y > (mY - mSize) && y < (mY + mSize));
    }
}
