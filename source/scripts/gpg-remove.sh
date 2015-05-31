#!/bin/sh

if [ x"$1" = x ]; then
  echo "gpg-remove: usage: gpg-remove ID"
  exit 100
fi
exec _CURVEPROTECT_/bin/gpg --no-permission-warning --no-secmem-warning --batch --homedir _CURVEPROTECT_/.keys/gnupg --yes --delete-key "$1"
