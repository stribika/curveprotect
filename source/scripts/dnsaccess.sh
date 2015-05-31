#!/bin/sh

PATH="_CURVEPROTECT_/bin:${PATH}"
export PATH

zone=$1
if [ "x${zone}" = x ]; then
  echo "dnsaccess: usage: dnsaccess zone child" >&2
  exit 100
fi
shift

if [ "x${REMOTEPK}" = x ]; then
  echo "dnsaccess: fatal: \$REMOTEPK not set" >&2
  exit 111
fi

b32=`echo "${REMOTEPK}" | hextobin | bintobase32| sed "s/0$/.${zone}/"`
x=`dnstxt "${b32}" | sed 's/ /_/g' | tr -cd '[a-z][A-Z][0-9]_'`

if [ "x${x}" = x ]; then
  echo "dnsaccess: fatal: ${REMOTEPK} not desired" >&2
  exit 111
fi

echo "dnsaccess: access: ${x}: ${REMOTEPK}" >&2
exec ${1+"$@"}
