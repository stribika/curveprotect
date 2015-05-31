#!/bin/sh

PATH=_CURVEPROTECT_/bin:/usr/local/bin:/usr/local/sbin:/bin:/sbin:/usr/bin:/usr/sbin:/usr/X11R6/bin
export PATH

#XXX
_CURVEPROTECT_/sbin/_addinterfaces 1>/dev/null 2>/dev/null

exec </dev/null
exec >/dev/null
exec 2>/dev/null

[ -d _CURVEPROTECT_/service ] || mkdir -p _CURVEPROTECT_/service

_CURVEPROTECT_/bin/svc -dx _CURVEPROTECT_/service/* _CURVEPROTECT_/service/*/log

env - PATH="${PATH}" svscan _CURVEPROTECT_/service 2>&1 | \
env - PATH="${PATH}" readproctitle service errors: ................................................................................................................................................................................................................................................................................................................................................................................................................
