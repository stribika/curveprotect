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
USER=`dnstxt "${b32}" | sed 's/ /_/' | tr -cd '[a-z][A-Z][0-9]_'`

if [ "x${USER}" = x ]; then
  echo "dnsaccess: fatal: ${REMOTEPK} not desired" >&2
  exit 111
fi

HOME=`grep "^${USER}:" /etc/passwd | cut -d ":" -f6`
if [ "x${USER}" = x ]; then
  echo "dnsaccess: fatal: ${USER} not exist" >&2
  exit 111
fi
cd "${HOME}"
export HOME
export USER

echo "dnsaccess: access: ${USER}: ${REMOTEPK}" >&2
exec setuidgid "${USER}" ${1+"$@"}
