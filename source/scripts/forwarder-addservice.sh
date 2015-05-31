#!/bin/sh -e

base=_CURVEPROTECT_
dnscacheip=_DNSCACHEIP_._DNSCRYPTCACHEIP_

PATH="${base}/bin:${PATH}"
export PATH

for i in `awk 'BEGIN{for(i=0;i<100;++i)printf("%02d\n",i);exit}'`; do

  forwarderdir="forwarder${i}"
  etcdir="${base}/etc/${forwarderdir}"
  servicedir="${base}/servicedir/${forwarderdir}"
  servicelink="${base}/service/${forwarderdir}"
  logdir="${base}/log/${forwarderdir}"
  lockfile="${base}/var/lock/${forwarderdir}.lock"

  if [ -h "${servicelink}" ]; then
    continue
  fi
  
  #XXX
  if [ -d "${etcdir}" ]; then
    continue;
  fi

  #XXX
  if [ -d "${servicedir}" ]; then
    continue;
  fi

  mkdir -p "${servicedir}/log"
  (
    echo "#!/bin/sh"
    echo "exec 2>&1"
    echo "PATH=\"${base}/bin:\${PATH}\""
    echo "export PATH"
    echo ""
    echo "exec envdir ${etcdir}/env sh -c '"
    echo "  if [ x\"\${IP}\" = x ]; then"
    echo "    exec sleep 86400"
    echo "  fi"
    echo "  if [ x\"\${PORT}\" = x ]; then"
    echo "    exec sleep 86400"
    echo "  fi"
    echo "  if [ x\"\${SERVER}\" = x ]; then"
    echo "    exec sleep 86400"
    echo "  fi"
    echo "  if [ x\"\${CLIENTFIRST}\" != x ]; then"
    echo "    FIRST=\"-C\""
    echo "  else"
    echo "    FIRST=\"-c\""
    echo "  fi"
    echo "  if [ x\"\${KEYDIR}\" != x ]; then"
    echo "    KEY=\"-k \${KEYDIR}\""
    echo "  fi"
    echo "  if [ x\"\${DEBUG}\" != x ]; then"
    echo "    RECORDIO=recordio"
    echo "  fi"
    echo "  exec envuidgid _CPUSR_ softlimit -a1000000000 -o20 -d10000000 nettcpserver -U \"\${IP}\" \"\${PORT}\" \${RECORDIO} netclient -vv \${FIRST} \${KEY} \"\${SERVER}\" \"\${PORT}\" fdcopy"
    echo "'" 
  ) > "${servicedir}/run"
  chmod 755 "${servicedir}/run"

  (
    echo "#!/bin/sh"
    echo "PATH=\"${base}/bin:\${PATH}\""
    echo "export PATH"
    echo ""
    echo ""
    echo "exec extremeenvuidgid sh -c '"
    echo "  uidgidchown -R ${logdir}"
    echo "  exec extremesetuidgid multilog t n2 s99999 ${logdir}"
    echo "'"
  ) > "${servicedir}/log/run"
  chmod 755 "${servicedir}/log/run"

  (
    mkdir -p "${etcdir}/env"
    echo "${base}/var/global" > "${etcdir}/env/ROOT"
    echo "${dnscacheip}" > "${etcdir}/env/DNSCACHEIP"
  ) 

  (
    mkdir -p "${logdir}"
  )

  fixservice "${forwarderdir}"

  ln -s "${servicedir}" "${servicelink}"
  exit 0
done
exit 111
