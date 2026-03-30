package com.nvidia.q2rtx;

import android.app.Activity;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.widget.Button;
import android.widget.TextView;

import java.io.File;
import java.io.FilenameFilter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public final class LauncherActivity extends Activity {
    private TextView pathText;
    private TextView statusText;
    private Button startButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_launcher);

        pathText = findViewById(R.id.pathText);
        statusText = findViewById(R.id.statusText);
        startButton = findViewById(R.id.startButton);

        startButton.setOnClickListener(v -> startActivity(new Intent(this, Q2RTXActivity.class)));
    }

    @Override
    protected void onResume() {
        super.onResume();
        refreshState();
    }

    private void refreshState() {
        File baseq2Dir = getImportDirectory();
        boolean deviceReady = isDeviceCompatible();
        List<String> missing = getMissingFiles(baseq2Dir);
        boolean assetsReady = missing.isEmpty();

        pathText.setText(baseq2Dir.getAbsolutePath());

        StringBuilder status = new StringBuilder();
        if (!deviceReady) {
            status.append("This launcher only supports Android 14 arm64 devices. Native startup performs the final Vulkan ray tracing extension check.\n\n");
        } else {
            status.append("Device baseline looks compatible. Native startup will now verify Vulkan 1.2 ray tracing extensions.\n\n");
        }

        if (!assetsReady) {
            status.append("Missing assets:\n");
            for (String item : missing) {
                status.append(" - ").append(item).append('\n');
            }
        } else {
            status.append("All required external assets were found.\n");
        }

        statusText.setText(status.toString().trim());
        startButton.setEnabled(deviceReady && assetsReady);
    }

    private File getImportDirectory() {
        File externalRoot = getExternalFilesDir(null);
        if (externalRoot == null) {
            externalRoot = getFilesDir();
        }
        return new File(externalRoot, "q2rtx/baseq2");
    }

    private boolean isDeviceCompatible() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
            return false;
        }
        return Arrays.asList(Build.SUPPORTED_64_BIT_ABIS).contains("arm64-v8a");
    }

    private List<String> getMissingFiles(File baseq2Dir) {
        List<String> missing = new ArrayList<>();

        if (!baseq2Dir.isDirectory()) {
            missing.add("baseq2 directory");
            missing.add("at least one retail pak*.pak");
            missing.add("q2rtx_media.pkz");
            missing.add("blue_noise.pkz");
            //missing.add("shaders.pkz");
            return missing;
        }

        File[] pakFiles = baseq2Dir.listFiles((FilenameFilter) (dir, name) ->
            name.toLowerCase().startsWith("pak") && name.toLowerCase().endsWith(".pak"));
        if (pakFiles == null || pakFiles.length == 0) {
            missing.add("retail pak*.pak");
        }
        requireFile(baseq2Dir, "q2rtx_media.pkz", missing);
        requireFile(baseq2Dir, "blue_noise.pkz", missing);
        //requireFile(baseq2Dir, "shaders.pkz", missing);
        return missing;
    }

    private static void requireFile(File baseq2Dir, String name, List<String> missing) {
        if (!new File(baseq2Dir, name).isFile()) {
            missing.add(name);
        }
    }
}
