#!/bin/sh -e

base=_CURVEPROTECT_
dnscacheip=_DNSCACHEIP_._DNSCRYPTCACHEIP_

PATH="${base}/bin:${PATH}"
export PATH

for i in `awk 'BEGIN{for(i=0;i<100;++i)printf("%02d\n",i);exit}'`; do

  vpndir="vpn${i}"
  etcdir="${base}/etc/${vpndir}"
  servicedir="${base}/servicedir/${vpndir}"
  servicelink="${base}/service/${vpndir}"
  logdir="${base}/log/${vpndir}"
  lockfile="${base}/var/lock/${vpndir}.lock"

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
    echo "  if [ x\"\${PORT}\" = x ]; then"
    echo "    exec sleep 86400"
    echo "  fi"
    echo "  if [ x\"\${SERVER}\" = x ]; then"
    echo "    exec sleep 86400"
    echo "  fi"
    echo "  if [ x\"\${KEYDIR}\" = x ]; then"
    echo "    exec sleep 86400"
    echo "  fi"
    echo "  exec softlimit -a1000000000 -o20 -d10000000 setlock ${lockfile} netclient -vvUck \"\${KEYDIR}\" \"\${SERVER}\" \"\${PORT}\" extremeenvuidgid vpn"
    echo "'" 
  ) > "${servicedir}/run"
  chmod 755 "${servicedir}/run"

  (
    echo "#!/bin/sh"
    echo "PATH=\"${base}/bin:\${PATH}\""
    echo "export PATH"
    echo ""
    echo "exec extremeenvuidgid sh -c '"
    echo "  uidgidchown -R ${logdir}"
    echo "  exec extremesetuidgid multilog t n2 s99999 ${logdir}"
    echo "'"
  ) > "${servicedir}/log/run"
  chmod 755 "${servicedir}/log/run"

  touch "${servicedir}/down"

  (
    mkdir -p "${etcdir}/env"
    mkdir -p "${etcdir}/root"
    echo "${etcdir}/root" > "${etcdir}/env/ROOT"
    echo "${dnscacheip}" > "${etcdir}/env/DNSCACHEIP"
  ) 

  (
    mkdir -p "${logdir}"
  )

  fixservice "${vpndir}"

  ln -s "${servicedir}" "${servicelink}"
  exit 0
done
exit 111
