#20121230
#Jan Mojzis
#Public domain.

#help
if [ x"`dnscryptmakecert -h 2>&1 | grep '^dnscryptmakecert: usage:'`" != x"dnscryptmakecert: usage:" ]; then
  echo "dnscryptmakecert test failed: help error"; exit 111
fi

#bad provider
SECRETKEY=`randombytes 64 | bintohex`; export SECRETKEY
SERVERPK=`randombytes 32 | bintohex`; export SERVERPK
PERIOD=36000; export PERIOD
for provider in aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.cz aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.cz; do
  export provider
  if [ x"`dnscryptmakecert \"${provider}\" \"${SERVERPK}\" \"${PERIOD}\" 2>&1 | tail -1`" != x"dnscryptmakecert: fatal: provider must be at most 255 bytes, at most 63 bytes between dots" ]; then
    echo "dnscryptmakecert test failed: bad provider name ${provider} accepted"; exit 111
  fi
done

#bad SECRETKEY
SECRETKEY=`randombytes 63 | bintohex`; export SECRETKEY
SERVERPK=`randombytes 32 | bintohex`; export SERVERPK
PERIOD=36000; export PERIOD
if [ x"`dnscryptmakecert testzone \"${SERVERPK}\" \"${PERIOD}\" 2>&1 | tail -1`" != x'dnscryptmakecert: fatal: $SECRETKEY must be exactly 128 hex characters' ]; then
  echo "dnscryptmakecert test failed: bad SECRETKEY checksum error"; exit 111
fi

#bad serverpk
SECRETKEY=`randombytes 64 | bintohex`; export SECRETKEY
SERVERPK=`randombytes 31 | bintohex`; export SERVERPK
PERIOD=36000; export PERIOD
if [ x"`dnscryptmakecert testzone \"${SERVERPK}\" \"${PERIOD}\" 2>&1 | tail -1`" != x'dnscryptmakecert: fatal: serverpk must be exactly 64 hex characters' ]; then
  echo "dnscryptmakecert test failed: bad SERVERPK checksum error"; exit 111
fi

#disk full
if [ -w /dev/full ]; then
  SECRETKEY=`randombytes 64 | bintohex`; export SECRETKEY
  SERVERPK=`randombytes 32 | bintohex`; export SERVERPK
  PERIOD=36000; export PERIOD
  e="`dnscryptmakecert testzone \"${SERVERPK}\" \"${PERIOD}\" 2>&1 >/dev/full || :`"
  if [ x"${e}" != x"dnscryptmakecert: fatal: unable to write output: out of disk space" ]; then
    echo "dnscryptmakecert test failed: ${e}"; exit 111
  fi
fi
