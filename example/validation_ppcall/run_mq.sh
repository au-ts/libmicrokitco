
rm -rfd build &&
make &&
mq.sh run -s odroidc4_1 -f build/loader.img -c "FINISHED"

