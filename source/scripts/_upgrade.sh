#!/bin/sh -e

if [ x"`dirname $0`" != x_CURVEPROTECT_/sbin ]; then
  echo "$0: unable to run script from _CURVEPROTECT_/bin directory" >&2
  exit 111
fi

#20131207 - upgrade dnscache servers
(
  cd _CURVEPROTECT_/etc/dnscache/root/servers/
  ls | sort |\
  while read file
  do
    cat "${file}" | awk '
      BEGIN {
        FS=":"
        OFS="|"
      }
      {
        if (NF == 2) {
          print $1, $2
        }
        else {
          print $0
        }
      }
    ' > ".${file}.tmp"
    _CURVEPROTECT_/bin/fsyncfile ".${file}.tmp"
    mv -f ".${file}.tmp" "${file}"
  done
)
_CURVEPROTECT_/bin/envuidgid _CPCFG_ _CURVEPROTECT_/bin/uidgidchown -R "_CURVEPROTECT_/etc/dnscache/root/servers/"


#restart everything
_CURVEPROTECT_/bin/svc -t _CURVEPROTECT_/service/* 1>/dev/null 2>/dev/null
