#!/bin/sh

#20121202
#Jan Mojzis
#Public domain.

usage="_creategroup: usage: _creategroup group"
fatal="_creategroup: fatal"
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

if grep "^${grp}:" /etc/group >/dev/null; then
  echo "${fatal}: group \"${grp}\" exists"
  exit 111
fi

#GROUPADD VERSION --------------------------
if which groupadd 1>/dev/null 2>/dev/null; then
  #linux, netbsd, openbsd, solaris
  r=`groupadd "${grp}" 2>&1`; e="$?"
  if [ "${e}" -gt 0 ]; then
    echo "${fatal}: `echo ${r} | sed 's/\n/_/' | sed 's/ /_/g' | tr -cd '[a-z][A-Z][0-9]_()<>/'`" >&2
  fi
  exit "$e"
fi

#MKGROUP VERSION --------------------------
if which mkgroup 1>/dev/null 2>/dev/null; then
  #aix
  r=`mkgroup "${grp}" 2>&1`; e="$?"
  if [ "${e}" -gt 0 ]; then
    echo "${fatal}: `echo ${r} | sed 's/\n/_/' | sed 's/ /_/g' | tr -cd '[a-z][A-Z][0-9]_()<>/'`" >&2
  fi
  exit "$e"
fi

#PW VERSION --------------------------
if pw groupshow wheel 1>/dev/null 2>/dev/null; then
  #freebsd
  r=`pw groupadd "${grp}" 2>&1`; e="$?"
  if [ "${e}" -gt 0 ]; then
    echo "${fatal}: `echo ${r} | sed 's/\n/_/' | sed 's/ /_/g' | tr -cd '[a-z][A-Z][0-9]_()<>/'`" >&2
  fi
  exit "$e"
fi


#DSCL VERSION --------------------------
if which dscl 1>/dev/null 2>/dev/null; then

  #macosx
  dscl=`which dscl`

  #check if group exist
  "${dscl}" . -read "/Groups/${grp}" 1>/dev/null 2>/dev/null
  if [ $? -eq 0 ]; then
    echo "${fatal}: group \"${grp}\" exists"
    exit 111
  fi

  #get first available grpid
  for grpid in `awk 'BEGIN{for(i=501;i<1000;++i)print i;exit}'`; do
    r=`"${dscl}" . -search /Groups gid "${grpid}"`
    if [ x"${r}" = x ]; then
      break
    fi
  done

  #create new group
  "${dscl}" . -create "/Groups/${grp}"
  "${dscl}" . -append "/Groups/${grp}" PrimaryGroupID "${grpid}"
  exit $?
fi

echo "${fatal}: unable to create group ${grp} on Your system." >&2
exit 111
