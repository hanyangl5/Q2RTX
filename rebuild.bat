cmake --preset androidapprelease
cd .\build\androidapprelease\ && .\gradlew.bat assembleRelease
adb push baseq2\q2rtx.cfg storage/emulated/0/Android/data/com.nvidia.q2rtx/files/q2rtx/baseq2/q2rtx.cfg