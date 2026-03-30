# Q2RTX Android Demo

This Gradle project builds the Android RTX demo launcher and loads the engine through SDL's Android activity.

## Build

1. Install Android Studio / Android SDK 34, NDK, and CMake 3.22+.
2. Ensure a host `glslangValidator` is available on your `PATH`, or pass `-DGLSLANG_COMPILER=/full/path/to/glslangValidator` to CMake.
3. Open the `android/` folder in Android Studio and build the `app` module for `arm64-v8a`.

## Import Path

Copy the following files into the app-specific external directory shown by the launcher:

- `baseq2/pak*.pak`
- `baseq2/q2rtx_media.pkz`
- `baseq2/blue_noise.pkz`
- `baseq2/shaders.pkz`

The native runtime uses `<externalFilesDir>/q2rtx` as both `basedir` and `homedir`.
