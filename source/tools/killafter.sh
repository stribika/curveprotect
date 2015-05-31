#20121222
#Jan Mojzis
#Public domain.

#help
if [ x"`killafter -h 2>&1 | grep '^killafter: usage:'`" != x"killafter: usage:" ]; then
  echo "killafter: help error"; exit 111
fi

#bad/negative input
for i in a '' -1 -01 100000000000000000000000000000000000000; do
  e=`killafter "${i}" true 2>&1 | sed 's/number .*: invalid argument$/number: invalid argument/'`
  if [ x"${e}" != x"killafter: fatal: unable to parse number: invalid argument" ]; then
    echo "${e}"; exit 111
  fi
done

#zero length
for i in 0 +0 -0 000; do
  (
    exec 2>/dev/null
    killafter "${i}" cat </dev/zero && exit 111 || :
  ) || exit $?
done

killafter 1 true || exit 111
killafter 1 false && exit 111

#nonexistent
e=`killafter 1 ./nonexistent 2>&1 || :`
if [ x"${e}" != "xkillafter: fatal: unable to run ./nonexistent: file does not exist" ]; then
  echo "${e}"; exit 111
fi
