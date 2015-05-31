#!/bin/sh

#20121202
#Jan Mojzis
#Public domain.

usage="_removegroup: usage: _removegroup groupname"
fatal="_removegroup: fatal"
if [ x"`dirname $0`" != x_CURVEPROTECT_/sbin ]; then
  echo "${fatal}: run script only from _CURVEPROTECT_/sbin directory" >&2
  exit 111
fi

#group
if [ x"$1" = x ]; then
  echo "${usage}"
  exit 100
fi
grp="$1"

#GROUPADD VERSION --------------------------
if which groupdel 1>/dev/null 2>/dev/null; then
  #linux, netbsd, openbsd, solaris
  r=`groupdel "${grp}" 2>&1`; e="$?"
  if [ "${e}" -gt 0 ]; then
    echo "${fatal}: `echo ${r} | sed 's/\n/_/' | sed 's/ /_/g' | tr -cd '[a-z][A-Z][0-9]_()<>/'`" >&2
  fi
  exit "$e"
fi

#RMGROUP VERSION --------------------------
if which rmgroup 1>/dev/null 2>/dev/null; then
  #aix
  r=`rmgroup "${grp}" 2>&1`; e="$?"
  if [ "${e}" -gt 0 ]; then
    echo "${fatal}: `echo ${r} | sed 's/\n/_/' | sed 's/ /_/g' | tr -cd '[a-z][A-Z][0-9]_()<>/'`" >&2
  fi
  exit "$e"
fi

#PW VERSION --------------------------
if pw groupshow wheel 1>/dev/null 2>/dev/null; then
  #freebsd
  r=`pw groupdel "${grp}" 2>&1`; e="$?"
  if [ "${e}" -gt 0 ]; then
    echo "${fatal}: `echo ${r} | sed 's/\n/_/' | sed 's/ /_/g' | tr -cd '[a-z][A-Z][0-9]_()<>/'`" >&2
  fi
  exit "$e"
fi

#DSCL VERSION --------------------------
if which dscl 1>/dev/null 2>/dev/null; then
  #macosx
  r=`dscl . -delete "/Groups/${grp}" 2>&1`; e="$?"
  if [ "${e}" -gt 0 ]; then
    echo "${fatal}: `echo ${r} | sed 's/\n/_/' | sed 's/ /_/g' | tr -cd '[a-z][A-Z][0-9]_()<>/'`" >&2
  fi
  exit "$e"
fi

echo "${fatal}: unable to remove group ${grp} on Your system." >&2
exit 111
