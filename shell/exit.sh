rmmod shared.ko
cd /dev
ls | grep -P "shared.*" | xargs rm
cd /home/code/pt
