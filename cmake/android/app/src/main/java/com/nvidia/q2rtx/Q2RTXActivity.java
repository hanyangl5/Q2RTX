package com.nvidia.q2rtx;

import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.view.WindowManager;

import org.libsdl.app.SDLActivity;

public final class Q2RTXActivity extends SDLActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "gameaarch64", "main" };
    }
}
