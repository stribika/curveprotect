#!/bin/sh -e

base=_CURVEPROTECT_
x=0

for i in `awk 'BEGIN{for(i=99;i>=0;--i)printf("%02d\n",i);exit}'`; do

  forwarderdir="forwarder${i}"
  etcdir="${base}/etc/${forwarderdir}"
  servicelink="${base}/service/${forwarderdir}"

  if [ ! -h "${servicelink}" ]; then
    continue;
  fi

  if [ -f "${etcdir}/env/SERVER" ]; then
    continue
  fi

  x=`expr "${x}" + 1`
done

echo "${x}"
exit 0
