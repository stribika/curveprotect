#!/bin/sh

#20121202
#Jan Mojzis
#Public domain.


usage="_removeuser: usage: _removeuser username"
fatal="_removeuser: fatal"
if [ x"`dirname $0`" != x_CURVEPROTECT_/sbin ]; then
  echo "${fatal}: run script only from _CURVEPROTECT_/sbin directory" >&2
  exit 111
fi

#user
if [ x"$1" = x ]; then
  echo "${usage}"
  exit 100
fi
usr="$1"

#USERDEL VERSION --------------------------
if which userdel 1>/dev/null 2>/dev/null; then

  #linux
  userdel -f "${usr}" 1>/dev/null 2>/dev/null && exit 0

  #aix, netbsd, openbsd, solaris
  r=`userdel "${usr}" 2>&1`; e="$?"
  if [ "${e}" -gt 0 ]; then
    echo "${fatal}: `echo ${r} | sed 's/\n/_/' | sed 's/ /_/g' | tr -cd '[a-z][A-Z][0-9]_()<>/'`" >&2
  fi
  exit "$e"
fi

#PW VERSION --------------------------
if pw usershow root 1>/dev/null 2>/dev/null; then
  #freebsd
  r=`pw userdel "${usr}" 2>&1`; e="$?"
  if [ "${e}" -gt 0 ]; then
    echo "${fatal}: `echo ${r} | sed 's/\n/_/' | sed 's/ /_/g' | tr -cd '[a-z][A-Z][0-9]_()<>/'`" >&2
  fi
  exit "$e"
fi

#DSCL VERSION --------------------------
if which dscl 1>/dev/null 2>/dev/null; then
  #macosx
  dscl=`which dscl`
  homedir=`"${dscl}" . -read "/Users/${usr}" NFSHomeDirectory | awk '{print $NF}'`
  [ -d "${homedir}" ] && rm -rf "${homedir}"
  r=`dscl . -delete "/Users/${usr}"  2>&1`; e="$?"
  if [ "${e}" -gt 0 ]; then
    echo "${fatal}: `echo ${r} | sed 's/\n/_/' | sed 's/ /_/g' | tr -cd '[a-z][A-Z][0-9]_()<>/'`" >&2
  fi
  exit "$e"
fi

echo "${fatal}: unable to remove user ${usr} on Your system." >&2
exit 111
