#!/bin/sh -e

if [ x"`dirname $0`" != x_CURVEPROTECT_/sbin ]; then
  echo "$0: unable to run script from _CURVEPROTECT_/bin directory" >&2
  exit 111
fi

#create users and groups
usr=_CPUSR_
cfg=_CPCFG_
_CURVEPROTECT_/sbin/_creategroup "${usr}" || :
_CURVEPROTECT_/sbin/_creategroup "${cfg}" || :
_CURVEPROTECT_/sbin/_createuser "${usr}" "${usr}" || :
_CURVEPROTECT_/sbin/_createuser "${cfg}" "${cfg}" || :

#create directories
mkdir -p "_CURVEPROTECT_/.keys/ed25519" || :
mkdir -p "_CURVEPROTECT_/.keys/curvecp" || :
mkdir -p "_CURVEPROTECT_/.keys/dnscurve" || :
mkdir -p "_CURVEPROTECT_/.keys/gnupg" || :
_CURVEPROTECT_/bin/envuidgid "${usr}" _CURVEPROTECT_/bin/uidgidchown -R "_CURVEPROTECT_/.keys"
_CURVEPROTECT_/bin/envuidgid "${cfg}" _CURVEPROTECT_/bin/uidgidchown -R "_CURVEPROTECT_/etc/tmp"

#import gnupg keys
_CURVEPROTECT_/bin/setuidgid "${usr}" _CURVEPROTECT_/bin/gpg --homedir _CURVEPROTECT_/.keys/gnupg --no-permission-warning --no-secmem-warning --batch --import _CURVEPROTECT_/share/gnupgkey-4150E298.asc
_CURVEPROTECT_/bin/setuidgid "${usr}" _CURVEPROTECT_/bin/gpg --homedir _CURVEPROTECT_/.keys/gnupg --no-permission-warning --no-secmem-warning --batch --import _CURVEPROTECT_/share/gnupgkey-0BD07395.asc
chmod 644 _CURVEPROTECT_/.keys/gnupg/pubring.gpg _CURVEPROTECT_/.keys/gnupg/trustdb.gpg

#import OpenDNS key
if [ ! -d "_CURVEPROTECT_/.keys/ed25519/opendns" ]; then
  mkdir "_CURVEPROTECT_/.keys/ed25519/opendns"
  cp _CURVEPROTECT_/share/ed25519key-opendns.pk _CURVEPROTECT_/.keys/ed25519/opendns/publickey.tmp
  _CURVEPROTECT_/bin/fsyncfile _CURVEPROTECT_/.keys/ed25519/opendns/publickey.tmp
  mv -f _CURVEPROTECT_/.keys/ed25519/opendns/publickey.tmp _CURVEPROTECT_/.keys/ed25519/opendns/publickey
  _CURVEPROTECT_/bin/envuidgid "${usr}" _CURVEPROTECT_/bin/uidgidchown -R "_CURVEPROTECT_/.keys/ed25519/opendns"
fi

#start daemontools + services
_CURVEPROTECT_/sbin/_startdaemontools
(
  ls "_CURVEPROTECT_/servicedir"  |\
  sort  |\
  while read d
  do
    _CURVEPROTECT_/bin/fixservice "${d}"
    [ -h "_CURVEPROTECT_/service/${d}" ] || ln -s "_CURVEPROTECT_/servicedir/${d}" "_CURVEPROTECT_/service/${d}"
  done
)

#upgrade
_CURVEPROTECT_/sbin/_upgrade
