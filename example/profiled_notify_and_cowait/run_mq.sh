(
    rm -rfd build && mkdir build && \
    make -j$(nproc) && \
    mq.sh run -s odroidc4_1 -f build/loader.img -c "FINISHED"
)
