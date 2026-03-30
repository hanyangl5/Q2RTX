adb install .\build\androidapprelease\app\build\outputs\apk\release\app-release.apk
adb shell ls /storage/emulated/0/Android/data/com.nvidia.q2rtx/files/q2rtx/baseq2
adb shell mkdir -p /storage/emulated/0/Android/data/com.nvidia.q2rtx/files/q2rtx/baseq2
adb push baseq2 /storage/emulated/0/Android/data/com.nvidia.q2rtx/files/q2rtx/
adb shell chmod -R 777 /storage/emulated/0/Android/data/com.nvidia.q2rtx/files/q2rtx
adb shell ls /storage/emulated/0/Android/data/com.nvidia.q2rtx/files/q2rtx/baseq2
adb shell cat /storage/emulated/0/Android/data/com.nvidia.q2rtx/files/q2rtx/baseq2/logs/console.log