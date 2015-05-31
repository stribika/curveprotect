#20130106
#Jan Mojzis
#Public domain.

#help
if [ x"`dnscryptmakekeys -h 2>&1 | grep '^dnscryptmakekeys: usage:'`" != x"dnscryptmakekeys: usage:" ]; then
  echo "dnscryptmakekeys test failed: help error"; exit 111
fi

#options
if [ "`dnscryptmakekeys -c | wc -l`" -ne 3 ]; then
  echo "dnscryptmakekeys test failed: option '-c' error"; exit 111
fi
if [ "`dnscryptmakekeys -e | wc -l`" -ne 2 ]; then
  echo "dnscryptmakekeys test failed: option '-e' error"; exit 111
fi
if [ "`dnscryptmakekeys -b | wc -l`" -ne 5 ]; then
  echo "dnscryptmakekeys test failed: option '-b' error"; exit 111
fi
if [ "`dnscryptmakekeys | wc -l`" -ne 5 ]; then
  echo "dnscryptmakekeys test failed: 'no option' error"; exit 111
fi

#gen test
ref="`dnscryptmakekeys -c | nacl-sha256`"
for i in `awk 'BEGIN{for(i=0;i<50;++i)print i;exit}'`; do
  x="`dnscryptmakekeys -c | nacl-sha256`"
  if [ x"${ref}" = x"${x}" ]; then
    echo "dnscryptmakekeys test failed: Curve25519 keypair generation disaster"; exit 111
  fi
done
ref="`dnscryptmakekeys -e | nacl-sha256`"
for i in `awk 'BEGIN{for(i=0;i<50;++i)print i;exit}'`; do
  x="`dnscryptmakekeys -e | nacl-sha256`"
  if [ x"${ref}" = x"${x}" ]; then
    echo "dnscryptmakekeys test failed: Ed25519 keypair generation disaster"; exit 111
  fi
done

#disk full
if [ -w /dev/full ]; then
  e=`dnscryptmakekeys 2>&1 >/dev/full || :`
  if [ x"${e}" != x"dnscryptmakekeys: fatal: unable to write output: out of disk space" ]; then
    echo "dnscryptmakekeys test failed: ${e}"; exit 111
  fi
fi
