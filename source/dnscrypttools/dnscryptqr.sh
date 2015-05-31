#20130119
#Jan Mojzis
#Public domain.

#help
if [ x"`dnscryptqr -h 2>&1 | grep '^dnscryptqr: usage:'`" != x"dnscryptqr: usage:" ]; then
  echo "dnscryptqr test failed: help error"; exit 111
fi

#missing type
if [ x"`dnscryptqr 2>&1 | tail -1`" != x"dnscryptqr: fatal: type not set" ]; then
  echo "dnscryptqr test failed: missing type"; exit 111
fi
#bad type
if [ x"`dnscryptqr xxx 2>&1 | tail -1`" != x"dnscryptqr: fatal: unable to parse type" ]; then
  echo "dnscryptqr test failed: bad type"; exit 111
fi
#missing fqdn
if [ x"`dnscryptqr 255 2>&1 | tail -1`" != x"dnscryptqr: fatal: fqdn not set" ]; then
  echo "dnscryptqr test failed: missing fqdn"; exit 111
fi
#bad fqdn
for fqdn in aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.cz aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.cz; do
  export fqdn
  if [ x"`dnscryptqr 255 \"${fqdn}\" 2>&1 | tail -1`" != x"dnscryptqr: fatal: fqdn must be at most 255 bytes, at most 63 bytes between dots" ]; then
    echo "dnscryptqr test failed: bad fqdn"; exit 111
  fi
done
#missing ip
if [ x"`dnscryptqr a mojzis.com 2>&1 | tail -1`" != x"dnscryptqr: fatal: ip not set" ]; then
  echo "dnscryptqr test failed: missing ip"; exit 111
fi

#quiet
sum=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
if [ x"`dnscryptqr -q -h 2>&1 | nacl-sha256`" != x"${sum}" ]; then
  echo "dnscryptqr test failed: quiet parameter problem"; exit 111
fi
