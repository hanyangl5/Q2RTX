# Q2RTX Android Demo

This Gradle project builds the Android RTX demo launcher and loads the engine through SDL's Android activity.

## Build

1. Install Android Studio / Android SDK 34, NDK, and CMake 3.22+.
2. Ensure a host `glslangValidator` is available on your `PATH`, or pass `-DGLSLANG_COMPILER=/full/path/to/glslangValidator` to CMake.
3. Open the `android/` folder in Android Studio and build the `app` module for `arm64-v8a`.

## Signing

Release builds are signed automatically so they can be installed on-device:

- If `Q2RTX_RELEASE_STORE_FILE`, `Q2RTX_RELEASE_STORE_PASSWORD`, `Q2RTX_RELEASE_KEY_ALIAS`, and `Q2RTX_RELEASE_KEY_PASSWORD` are set as Gradle properties or environment variables, the app uses that keystore.
- Otherwise the build falls back to the standard Android debug keystore at `~/.android/debug.keystore` for local testing.

Without signing, Android emits `INSTALL_PARSE_FAILED_NO_CERTIFICATES` when you try to install `app-release-unsigned.apk`.

## Import Path

Copy the following files into the app-specific external directory shown by the launcher:

- `baseq2/pak*.pak`
- `baseq2/q2rtx_media.pkz`
- `baseq2/blue_noise.pkz`
- `baseq2/shaders.pkz`

The native runtime uses `<externalFilesDir>/q2rtx` as both `basedir` and `homedir`.
