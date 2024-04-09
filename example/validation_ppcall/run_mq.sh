rm -rfd build && \
make && \
mq.sh run -s odroidc4_pool -f build/loader.img -c "FINISHED"
