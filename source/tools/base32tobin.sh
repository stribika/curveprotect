#20121220
#Jan Mojzis
#Public domain.

#help
if [ x"`base32tobin -h 2>&1 | grep '^base32tobin: usage:'`" != x"base32tobin: usage:" ]; then
  echo "base32tobin: help error"; exit 111
fi

# zero bytes
base32tobin < /dev/null

#test "13\n" - > "a"
e=`echo "13
" | base32tobin 2>&1`
if [ x"${e}" != "xa" ]; then
  echo "base32tobin: character 'a' test failed"; exit 111
fi

#case sensitivity test
for i in `awk 'BEGIN{for(i=100;i<130;++i)print i;exit}'`; do
  a=`randombytes "$i" | bintobase32`
  b=`echo "${a}" | sed 'y/bcdfghjklmnpqrstuvwxyz/BCDFGHJKLMNPQRSTUVWXYZ/'`
  if [ "`echo ${a} | base32tobin 2>&1 | nacl-sha256`" != "`echo ${b} | base32tobin | nacl-sha256`" ]; then
    echo "base32tobin: case sensitivity test failed"; exit 111
  fi
done

#invalid input test
for i in a e i o; do
  export i
  e=`echo "echo 1${i}20" | base32tobin 2>&1 || :`
  if [ x"${e}" != "xbase32tobin: fatal: unable to decode from base32" ]; then
    echo "${e}"; exit 111
  fi
done

#bintobase32/base32tobin test
empty=`nacl-sha256 </dev/null`
for i in `awk 'BEGIN{for(i=385;i<415;++i)print i;exit}'`; do
  export i
  a=`randombytes "${i}" | bintohex | tr -d '\n'`
  b=`echo "${a}" | nacl-sha256`
  c=`echo "${a}" | hextobin | bintobase32 | base32tobin | bintohex | nacl-sha256`
  if [ x"${b}" = x"${empty}" ]; then
    exit 111
  fi
  if [ x"${c}" = x"${empty}" ]; then
    exit 111
  fi
  if [ "${b}" != "${c}" ]; then
    echo "base32tobin: bintobase32/base32tobin test failed"; exit 111
  fi
done

#disk full
if [ -w /dev/full ]; then
  e=`echo 13 | base32tobin 2>&1 > /dev/full || :`
  if [ x"${e}" != x"base32tobin: fatal: unable to write output: out of disk space" ]; then
    echo "${e}"; exit 111
  fi
fi
