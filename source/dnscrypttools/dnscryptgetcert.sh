#20121230
#Jan Mojzis
#Public domain.

#help
if [ x"`dnscryptgetcert -h 2>&1 | grep '^dnscryptgetcert: usage:'`" != x"dnscryptgetcert: usage:" ]; then
  echo "dnscryptgetcert test failed: help error"; exit 111
fi

#missing provider
e=`dnscryptgetcert 2>&1 | tail -1`
if [ x"${e}" != x"dnscryptgetcert: fatal: provider must be at most 255 bytes, at most 63 bytes between dots" ]; then
  echo "dnscryptgetcert test failed: ${e}"; exit 111
fi
#bad provider
for fqdn in aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.cz aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.cz; do
  export fqdn
  e=`dnscryptgetcert \"${fqdn}\" 2>&1 | tail -1`
  if [ x"${e}" != x"dnscryptgetcert: fatal: provider must be at most 255 bytes, at most 63 bytes between dots" ]; then
    echo "dnscryptgetcert test failed: ${e}"; exit 111
  fi
done

#missing pk
e=`dnscryptgetcert mojzis.com 2>&1 | tail -1`
if [ x"${e}" != x"dnscryptgetcert: fatal: pk must be exactly 64 hex characters" ]; then
  echo "dnscryptgetcert test failed: ${e}"; exit 111
fi
#bad pk
e=`dnscryptgetcert mojzis.com a 2>&1 | tail -1`
if [ x"${e}" != x"dnscryptgetcert: fatal: pk must be exactly 64 hex characters" ]; then
  echo "dnscryptgetcert test failed: ${e}"; exit 111
fi
#missing ip
e=`dnscryptgetcert mojzis.com B7351140206F225D3E2BD822D7FD691EA1C33CC8D6668D0CBE04BFABCA43FB79 2>&1 | tail -1`
if [ x"${e}" != x"dnscryptgetcert: fatal: ip must be a comma-separated series of IPv4 addresses" ]; then
  echo "dnscryptgetcert test failed: ${e}"; exit 111
fi

#quiet
sum=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
if [ x"`dnscryptgetcert -q -h 2>&1 | nacl-sha256`" != x"${sum}" ]; then
  echo "dnscryptgetcert test failed: not quiet"; exit 111
fi
