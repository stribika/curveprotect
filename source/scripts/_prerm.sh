#!/bin/sh

if [ x"`dirname $0`" != x_CURVEPROTECT_/sbin ]; then
  echo "$0: unable to run script from _CURVEPROTECT_/bin directory" >&2
  exit 111
fi

#stop services + daemontools
for i in `ls _CURVEPROTECT_/servicedir 2>/dev/null`; do
    cd "_CURVEPROTECT_/service/${i}"
    rm "_CURVEPROTECT_/service/${i}"
    _CURVEPROTECT_/bin/svc -dx . log
    echo "stopping: _CURVEPROTECT_/service/${i}"
done
_CURVEPROTECT_/sbin/_stopdaemontools
