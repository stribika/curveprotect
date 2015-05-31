#!/bin/sh

if [ x"$2" = x ]; then
  echo "gpg-verify: usage: gpg-verify sigfile file"
  exit 100
fi
exec _CURVEPROTECT_/bin/gpg --no-permission-warning --no-secmem-warning --batch --homedir _CURVEPROTECT_/.keys/gnupg --lock-never --verify "$1" "$2"
