#!/bin/sh -e

#XXX - script is a dirty HACK

PATH="_CURVEPROTECT_/bin:${PATH}"
export PATH

if [ x"$1" = x ]; then
  echo "gpg-download: usage: gpg-download ID"
  exit 100
fi
URI="http://pgp.surfnet.nl:11371/pks/lookup?op=get&search=0x$1"; export URI
PROXYHOST="_DNSCACHEIP_"; export PROXYHOST
PROXYPORT="3128"; export PROXYPORT
TMPDIR="_CURVEPROTECT_/.keys/gnupg"; export TMPDIR
ID="$1"; export ID

setlock "${TMPDIR}/lock" sh -ce '
  cd "${TMPDIR}"
  rm -f gpg-download-*
  nettcpclient "${PROXYHOST}" "${PROXYPORT}" http-get "${URI}" "gpg-download-${ID}.$$" "gpg-download-${ID}.$$.tmp"
  gpg --no-permission-warning --no-secmem-warning --batch --homedir _CURVEPROTECT_/.keys/gnupg --import "gpg-download-${ID}.$$"
  rm -f gpg-download-*
'
