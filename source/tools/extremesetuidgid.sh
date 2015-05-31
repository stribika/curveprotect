#20121219
#Jan Mojzis
#Public domain.

#help
if [ x"`extremesetuidgid -h 2>&1 | grep '^extremesetuidgid: usage:'`" != x"extremesetuidgid: usage:" ]; then
  echo "extremesetuidgid: help error"; exit 111
fi

#root privileges
if [ "`effectiveenvuidgid printenv UID`" -eq 0 ]; then
  extremesetuidgid true || exit 111
  extremesetuidgid false && exit 111
  e=`extremesetuidgid ./nonexistent 2>&1 || :`
  if [ x"${e}" != "xextremesetuidgid: fatal: unable to run ./nonexistent: file does not exist" ]; then
    echo "${e}"; exit 111
  fi
fi
