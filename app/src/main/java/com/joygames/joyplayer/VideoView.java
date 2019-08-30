package com.joygames.joyplayer;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class VideoView extends SurfaceView {

    private SurfaceHolder mHolder;

    public VideoView(Context context) {
        super(context);
        init();
    }

    public VideoView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public VideoView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }


    private void init() {
        mHolder = getHolder();
        mHolder.setFormat(PixelFormat.RGBA_8888);
    }

    public void play(final String filePath) {
        new Thread(new Runnable() {
            @Override
            public void run() {
//                render(filePath, mHolder.getSurface());
                play(filePath, mHolder.getSurface());

            }
        }).start();
    }

    private native void render(String filePath, Surface surface);

    private native void play(String filePath, Surface surface);
}
