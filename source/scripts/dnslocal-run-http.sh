#!/bin/sh -e

PATH="_CURVEPROTECT_/bin:${PATH}"
export PATH

if [ "x${TMPDIR}" = x ]; then
  echo $TMPDIR not set
  exit 111
fi

if [ "x${ZONE}" = x ]; then
  echo $ZONE not set
  exit 111
fi

CONFIG=`cat config`
for line in `shuffleargs $CONFIG`; do
  uri=`echo "${line}"  | sed 's,${ZONE},'"${ZONE}"','`
  if [ x"${uri}" = x ]; then
    continue;
  fi

  PROXYHOST="_DNSCACHEIP_"
  export PROXYHOST
  PROXYPORT="3128"
  export PROXYPORT

  nettcpclient -v "${PROXYHOST}" "${PROXYPORT}" http-get "${uri}" "${TMPDIR}/file.txt" "${TMPDIR}/file.tmp" || continue

  if [ -f ed25519dir ]; then
    ed25519signopen "`cat ed25519dir`" < "${TMPDIR}/file.txt" > "${TMPDIR}/data"
  else
    cat < "${TMPDIR}/file.txt" > "${TMPDIR}/data"
    fsyncfile "${TMPDIR}/data"
  fi

  gunzip < "${TMPDIR}/data" 2>/dev/null && exit 0
  cat < "${TMPDIR}/data"
  exit 0
done
exit 111
