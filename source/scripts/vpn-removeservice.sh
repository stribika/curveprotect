#!/bin/sh -e

base=_CURVEPROTECT_

for i in `awk 'BEGIN{for(i=99;i>=0;--i)printf("%02d\n",i);exit}'`; do

  vpndir="vpn${i}"
  etcdir="${base}/etc/${vpndir}"
  servicedir="${base}/servicedir/${vpndir}"
  servicelink="${base}/service/${vpndir}"
  logdir="${base}/log/${vpndir}"

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
