#20121219
#Jan Mojzis
#Public domain.

#help
if [ x"`hextobin -h 2>&1 | grep '^hextobin: usage:'`" != x"hextobin: usage:" ]; then
  echo "hextobin: help error"; exit 111
fi

# zero bytes
hextobin < /dev/null

e=`echo '
' | hextobin 2>&1 || :`
if [ x"${e}" != "x" ]; then
  echo "${e}"; exit 111
  echo "hextobin: empty character test failed"; exit 111
fi

#test "61\n" - > "a"
e=`echo "61
" | hextobin 2>&1`
if [ x"${e}" != "xa" ]; then
  echo "hextobin: character 'a' test failed"; exit 111
fi

#case sensitivity test
for i in `awk 'BEGIN{for(i=10;i<30;++i)print i;exit}'`; do
  a=`randombytes "$i" | bintohex`
  b=`echo "${a}" | sed 'y/abcdef/ABCDEF/'`
  if [ "`echo ${a} | hextobin 2>&1 | nacl-sha256`" != "`echo ${b} | hextobin | nacl-sha256`" ]; then
    echo "hextobin: case sensitivity test failed"; exit 111
  fi
done

#invalid input test
e=`echo "aaa" | hextobin 2>&1 || :`
if [ x"${e}" != "xhextobin: fatal: unable to decode from hex" ]; then
  echo "${e}"; exit 111
fi
e=`echo "abcdefgh" | hextobin 2>&1 || :`
if [ x"${e}" != "xhextobin: fatal: unable to decode from hex" ]; then
  echo "${e}"; exit 111
fi

#bintohex/hextobin test
for i in `awk 'BEGIN{for(i=10240;i<10250;++i)print i;exit}'`; do
  randombytes "${i}" | bintohex | hextobin 1>/dev/null
done

#disk full
if [ -w /dev/full ]; then
  e=`echo aa | hextobin 2>&1 > /dev/full || :`
  if [ x"${e}" != x"hextobin: fatal: unable to write output: out of disk space" ]; then
    echo "${e}"; exit 111
  fi
fi
