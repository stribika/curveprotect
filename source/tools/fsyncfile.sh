#20130529
#Jan Mojzis
#Public domain.

if [ x"`fsyncfile -h 2>&1 | grep '^fsyncfile: usage:'`" != x"fsyncfile: usage:" ]; then
  echo "fsyncfile: help error"; exit 111
fi

#ok test
c1="`nacl-sha256 < fsyncfile.c`"
fsyncfile fsyncfile.c
c2="`nacl-sha256 < fsyncfile.c`"

if [ x"${c1}" != x"${c2}" ]; then
  echo "fsyncfile: checksum error"; exit 111
fi

#directory test
mkdir -p testdir
e=`fsyncfile testdir 2>&1 || :`
if [ x"${e}" != x"fsyncfile: fatal: unable to open file testdir: is a directory" ]; then
  echo "${e}"; exit 111
fi
