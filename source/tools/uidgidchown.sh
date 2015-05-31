#20121219
#Jan Mojzis
#Public domain.

#help
if [ x"`uidgidchown -h 2>&1 | grep '^uidgidchown: usage:'`" != x"uidgidchown: usage:" ]; then
  echo "uidgidchown: help error"; exit 111
fi


#UID/GID
e=`env - PATH="${PATH}" uidgidchown uidgidchown.c 2>&1 | tail -1 || :`
if [ x"${e}" != "xuidgidchown: fatal: \$UID not set" ]; then
  echo "${e}"; exit 111
fi

#UID not set
e=`env - PATH="${PATH}" UID='' uidgidchown uidgidchown.c 2>&1 | tail -1 || :`
if [ x"${e}" != "xuidgidchown: fatal: invalid \$UID: : invalid argument" ]; then
  echo "${e}"; exit 111
fi

#GID not set
e=`env - PATH="${PATH}" UID='10000' uidgidchown uidgidchown.c 2>&1 | tail -1 || :`
if [ x"${e}" != "xuidgidchown: fatal: \$GID not set" ]; then
  echo "${e}"; exit 111
fi

#nonexistent
e=`effectiveenvuidgid uidgidchown ./nonexistent 2>&1 || :`
if [ x"${e}" != "xuidgidchown: warning: unable to change ownership of './nonexistent': file does not exist" ]; then
  echo "${e}"; exit 111
fi

#non-recursive 
#touch uidgidchown.c
effectiveenvuidgid uidgidchown uidgidchown.c

#root privileges
if [ "`effectiveenvuidgid printenv UID`" -eq 0 ]; then
  mkdir -p testdir/testfile1 testdir/testfile2
  env - PATH="${PATH}" UID=0 GID=0 uidgidchown -R testdir
  extremeenvuidgid uidgidchown -R testdir
  rm -rf testdir
fi
