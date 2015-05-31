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

if [ -f timeout ]; then
  timeout=`cat timeout`
else
  timeout=60
fi

CONFIG=`cat config`
for line in `shuffleargs $CONFIG`; do
  host=`echo "${line}" | sed 's/${ZONE}/'"${ZONE}"'/' | cut -d':' -f1`
  port=`echo "${line}" | sed 's/${ZONE}/'"${ZONE}"'/' | cut -d':' -f2`
  netclient -uv "-T${timeout}" "${host}" "${port}" axfr-get "${ZONE}" "${TMPDIR}/file.txt" "${TMPDIR}/file.tmp" || continue
  cat "${TMPDIR}/file.txt"
  exit 0
done
exit 111
