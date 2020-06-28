echo "module status:"
cat /proc/devices | grep shared
echo "devices status:"
ls -l /dev | grep shared