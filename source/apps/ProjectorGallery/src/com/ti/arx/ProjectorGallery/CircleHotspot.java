package com.ti.arx.ProjectorGallery;

public class CircleHotspot extends Hotspot {
    public CircleHotspot(int id) {
        super(id, Shape.createCircle());
    }

    @Override
    public boolean isInside(float x, float y) {
        return (distance(x, y) < mSize);
    }
}
