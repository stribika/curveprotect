#20121222
#Jan Mojzis
#Public domain.

#help
if [ x"`sigpg -h 2>&1 | grep '^sigpg: usage:'`" != x"sigpg: usage:" ]; then
  echo "sigpg: help error"; exit 111
fi

#nonexistent
e=`sigpg ./nonexistent 2>&1 || :`
if [ x"${e}" != "xsigpg: fatal: unable to run ./nonexistent: file does not exist" ]; then
  echo "${e}"; exit 111
fi
