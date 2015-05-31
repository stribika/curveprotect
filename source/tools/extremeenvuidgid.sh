#20121219
#Jan Mojzis
#Public domain.

#help
if [ x"`extremeenvuidgid -h 2>&1 | grep '^extremeenvuidgid: usage:'`" != x"extremeenvuidgid: usage:" ]; then
  echo "extremeenvuidgid: help error"; exit 111
fi

#env test
a=`env - PATH="$PATH" extremeenvuidgid printenv | sed 's/=.*//' | sort | nacl-sha256`
b=1fbb128b25729643ca4b4345b89c94fd5b1b5d11e79c2ceeeb1d471c76f643fe
if [ x"${a}" != x"${b}" ]; then
  echo "extremeenvuidgid test failed"; exit 111
fi

#UID test
sh -c '
  P="$$"
  U=`expr "${P}" + 141500000`
  echo "UID=$U"
  exec extremeenvuidgid printenv
' | grep "^UID=" | sort -u | wc -l |\
while read line; do
  if [ x"${line}" != x1 ]; then
    echo "extremeenvuidgid: UID test failed"; exit 111
    exit 111
  fi
done

#GID test
sh -c '
  P="$$"
  U=`expr "${P}" + 141500000`
  echo "GID=$U"
  exec extremeenvuidgid printenv
' | grep "^GID=" | sort -u | wc -l |\
while read line; do
  if [ x"${line}" != x1 ]; then
    echo "extremeenvuidgid: GID test failed"; exit 111
    exit 111
  fi
done

#exec test
extremeenvuidgid true || exit 111
extremeenvuidgid false && exit 111

#nonexistent
e=`extremeenvuidgid ./nonexistent 2>&1 || :`
if [ x"${e}" != "xextremeenvuidgid: fatal: unable to run ./nonexistent: file does not exist" ]; then
  echo "${e}"; exit 111
fi
