#!/bin/sh -e

base=_CURVEPROTECT_
x=0

for i in `awk 'BEGIN{for(i=99;i>=0;--i)printf("%02d\n",i);exit}'`; do

  vpndir="vpn${i}"
  etcdir="${base}/etc/${vpndir}"
  servicelink="${base}/service/${vpndir}"

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
