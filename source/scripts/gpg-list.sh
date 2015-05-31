#!/bin/sh

exec _CURVEPROTECT_/bin/gpg --no-permission-warning --no-secmem-warning --batch --homedir _CURVEPROTECT_/.keys/gnupg --lock-never --with-colons --list-public-keys
