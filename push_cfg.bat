adb -s 0182200148488540 push .\baseq2\q2rtx.cfg  /storage/emulated/0/Android/data/com.nvidia.q2rtx/files/q2rtx/baseq2/q2rtx.cfg
adb -s 0182200148488540 shell rm -rf /storage/emulated/0/Android/data/com.nvidia.q2rtx/files/q2rtx/baseq2/save
adb -s 0182200148488540 push .\baseq2\shader_vkpt\  /storage/emulated/0/Android/data/com.nvidia.q2rtx/files/q2rtx/baseq2/