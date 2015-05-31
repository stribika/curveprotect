#20121218
#Jan Mojzis
#Public domain.

#help
if [ x"`sinceepoch -h 2>&1 | grep '^sinceepoch: usage:'`" != x"sinceepoch: usage:" ]; then
  echo "sinceepoch: help error"; exit 111
fi

#disk full
if [ -w /dev/full ]; then
  e=`sinceepoch 2>&1 > /dev/full|| :`
  if [ x"${e}" != x"sinceepoch: fatal: unable to write output: out of disk space" ]; then
    echo "${e}"; exit 111
  fi
fi
