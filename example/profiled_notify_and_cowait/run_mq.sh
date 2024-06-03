(
    rm -rfd build && \
    make >report.txt && \
    mq.sh run -s odroidc4_1 -f build/loader.img -c "FINISHED" >>report.txt && \
    mq.sh run -s odroidc4_1 -f build/loader.img -c "FINISHED" >>report.txt && \
    mq.sh run -s odroidc4_1 -f build/loader.img -c "FINISHED" >>report.txt
) && grep -E '^===> [0-9]+' <report.txt >result.txt && sed 's/===> //' <result.txt >result_sanitised.txt
