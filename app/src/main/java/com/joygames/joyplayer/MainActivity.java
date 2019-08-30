package com.joygames.joyplayer;

import android.Manifest;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.View;

import java.util.List;

import pub.devrel.easypermissions.AppSettingsDialog;
import pub.devrel.easypermissions.EasyPermissions;

public class MainActivity extends AppCompatActivity implements EasyPermissions.PermissionCallbacks{

    public static final String ACCESS_COARSE_LOCATION = Manifest.permission.ACCESS_COARSE_LOCATION;
    public static final String ACCESS_FINE_LOCATION   = Manifest.permission.ACCESS_FINE_LOCATION;
    public static final String WRITE_EXTERNAL_STORAGE = Manifest.permission.WRITE_EXTERNAL_STORAGE;
    public static final String READ_EXTERNAL_STORAGE  = Manifest.permission.READ_EXTERNAL_STORAGE;
    public static final String READ_PHONE_STATE       = Manifest.permission.READ_PHONE_STATE;
    public static final String ACCESS_WIFI            = Manifest.permission.ACCESS_WIFI_STATE;
    public static final int IMPORTANT_PERMS_CODE = 0;
    public static final String[] IMPORTANT_PERMS = {ACCESS_WIFI, READ_EXTERNAL_STORAGE, WRITE_EXTERNAL_STORAGE, ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION, READ_PHONE_STATE};

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("avcodec");
        System.loadLibrary("avdevice");
        System.loadLibrary("avfilter");
        System.loadLibrary("avformat");
        System.loadLibrary("avutil");
        System.loadLibrary("swresample");
        System.loadLibrary("swscale");
        System.loadLibrary("native-lib");
    }

    private VideoView mVideoView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        hideActionBar(this);
        hideStatusBar(this);

        if (!EasyPermissions.hasPermissions(MainActivity.this, IMPORTANT_PERMS)) {
            // 申请权限
            EasyPermissions.requestPermissions(MainActivity.this, null, IMPORTANT_PERMS_CODE, IMPORTANT_PERMS);
            Log.e("EasyPermissions","have not permissions");
        }else{
            Log.e("EasyPermissions","have permissions");
        }

        mVideoView = findViewById(R.id.video_view);
        mVideoView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
//                String filePath = new File(Environment.getExternalStorageDirectory(), "yy.264").getAbsolutePath();
                String filePath = "/mnt/sdcard/test.h264";
                mVideoView.play(filePath);
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {

            }
        });
    }

    private void hideActionBar(AppCompatActivity activity) {
        activity.getSupportActionBar().hide();
    }

    private void hideStatusBar(AppCompatActivity activity) {
        int uiOptions = activity.getWindow().getDecorView().getSystemUiVisibility();
        uiOptions |= View.SYSTEM_UI_FLAG_FULLSCREEN;
        activity.getWindow().getDecorView().setSystemUiVisibility(uiOptions);
    }

    @Override
    public void onPermissionsGranted(int requestCode, @NonNull List<String> perms) {

    }

    @Override
    public void onPermissionsDenied(int requestCode, @NonNull List<String> perms) {
        if (EasyPermissions.somePermissionPermanentlyDenied(this, perms)) {
            new AppSettingsDialog.Builder(this).build().show();
        }
    }

}
