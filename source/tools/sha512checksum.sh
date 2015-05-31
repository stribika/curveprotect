#20130113
#Jan Mojzis
#Public domain.

#help
if [ x"`sha512checksum -h 2>&1 | grep '^sha512checksum: usage:'`" != x"sha512checksum: usage:" ]; then
  echo "sha512checksum: help error"; exit 111
fi

#truncation
for i in `awk 'BEGIN{for(i=0;i<256;++i)print i;exit}'`; do
  a=`nacl-sha512 </dev/null | awk 'BEGIN{FS=""}{print substr($0,1,'${i}'*2)}'`
  b=`sha512checksum "${i}" </dev/null | tr -d '\n'`
  if [ x"${a}" != x"${b}" ]; then
    echo "sha512checksum: truncation test failed"; exit 111
  fi
done

for i in `awk 'BEGIN{for(i=0;i<300;++i)print i;exit}'`; do
  export i
  text=`randomtext "${i}"`
  export text
  a=`echo "${text}" | nacl-sha512`
  b=`echo "${text}" | sha512checksum`
  if [ x"${a}" != x"${b}" ]; then
    echo "sha512checksum: failed"; exit 111
  fi
done

for i in `awk 'BEGIN{for(i=4096;i<4106;++i)print i;exit}'`; do
  export i
  text=`randomtext "${i}"`
  export text
  a=`echo "${text}" | nacl-sha512`
  b=`echo "${text}" | sha512checksum`
  if [ x"${a}" != x"${b}" ]; then
    echo "sha512checksum: failed"; exit 111
  fi
done

if [ -w /dev/full ]; then
  e=`sha512checksum < /dev/null 2>&1 > /dev/full || :`
  if [ x"${e}" != x"sha512checksum: fatal: unable to write output: out of disk space" ]; then
    echo "sha512checksum: failed: ${e}"; exit 111
  fi
fi
