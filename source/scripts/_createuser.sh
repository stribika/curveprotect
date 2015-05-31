#!/bin/sh

#20121202
#Jan Mojzis
#Public domain.


usage="_createuser: usage: _createuser username groupname"
fatal="_createuser: fatal"
if [ x"`dirname $0`" != x_CURVEPROTECT_/sbin ]; then
  echo "${fatal}: run script only from _CURVEPROTECT_/sbin directory" >&2
  exit 111
fi

#user
if [ x"$2" = x ]; then
  echo "${usage}"
  exit 100
fi
usr="$1"
grp="$2"

#home directory
homedir="/nonexistent"

if grep "^${usr}:" /etc/passwd >/dev/null; then
  echo "${fatal}: user \"${usr}\"  exists"
  exit 111
fi

#USERADD VERSION --------------------------
if which useradd 1>/dev/null 2>/dev/null; then
  #linux, aix, netbsd, openbsd, solaris
  r=`useradd -d "${homedir}" -g "${grp}" "${usr}" 2>&1`; e="$?"
  if [ "${e}" -gt 0 ]; then
    echo "${fatal}: `echo ${r} | sed 's/\n/_/' | sed 's/ /_/g' | tr -cd '[a-z][A-Z][0-9]_()<>/'`" >&2
  fi
  exit "$e"
fi

#PW VERSION --------------------------
if pw usershow root 1>/dev/null 2>/dev/null; then
  #freebsd
  r=`pw useradd "${usr}" -d "${homedir}" -g "${grp}" 2>&1`; e="$?"
  if [ "${e}" -gt 0 ]; then
    echo "${fatal}: `echo ${r} | sed 's/\n/_/' | sed 's/ /_/g' | tr -cd '[a-z][A-Z][0-9]_()<>/'`" >&2
  fi
  exit "$e"
fi


#DSCL VERSION --------------------------
if which dscl 1>/dev/null 2>/dev/null; then
  #macosx
  dscl=`which dscl`
  grpid=`"${dscl}" . -read "/Groups/${grp}" gid | awk '{print $NF}'`

  #check if user exist
  "${dscl}" . -read "/Users/${usr}" >/dev/null 2>/dev/null
  if [ $? -eq 0 ]; then
    echo "_createuser: fatal: user \"${usr}\" exists"
    exit 111
  fi

  #get first available usrid
  for usrid in `awk 'BEGIN{for(i=501;i<1000;++i)print i;exit}'`; do
    r=`"${dscl}" . -search /Users uid "${usrid}"`
    if [ x"${r}" = x ]; then
      break
    fi
  done

  #create new user
  "${dscl}" . -create "/Users/${usr}"
  "${dscl}" . -append "/Users/${usr}" RealName "${usr}"
  "${dscl}" . -append "/Users/${usr}" NFSHomeDirectory "${homedir}"
  "${dscl}" . -append "/Users/${usr}" UserShell "/usr/bin/false"
  "${dscl}" . -append "/Users/${usr}" PrimaryGroupID "${grpid}"
  "${dscl}" . -append "/Users/${usr}" UniqueID "${usrid}"
  exit $?
fi

echo "${fatal}: unable to create user ${usr} on Your system." >&2
exit 111
