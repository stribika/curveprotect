#!/bin/sh -e

base=_CURVEPROTECT_

PATH="${base}/bin:${PATH}"
export PATH

for i in `awk 'BEGIN{for(i=99;i>=0;--i)printf("%02d\n",i);exit}'`; do

  forwarderdir="forwarder${i}"
  etcdir="${base}/etc/${forwarderdir}"
  servicedir="${base}/servicedir/${forwarderdir}"
  servicelink="${base}/service/${forwarderdir}"
  logdir="${base}/log/${forwarderdir}"

  if [ ! -h "${servicelink}" ]; then
    rm -rf "${etcdir}" "${servicedir}" "${logdir}"
    continue;
  fi

  if [ -f "${etcdir}/env/SERVER" ]; then
    continue
  fi

  (
    cd "${servicelink}"
    rm "${servicelink}"
    svc -dx . log
  )
  rm -rf "${etcdir}" "${servicedir}" "${logdir}"
  exit 0
done
exit 111
