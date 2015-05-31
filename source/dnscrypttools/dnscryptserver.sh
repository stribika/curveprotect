#20130119
#Jan Mojzis
#Public domain.

#help
if [ x"`dnscryptserver -h 2>&1 | grep '^dnscryptserver: usage:'`" != x"dnscryptserver: usage:" ]; then
  echo "dnscryptserver test failed: help error"; exit 111
fi
