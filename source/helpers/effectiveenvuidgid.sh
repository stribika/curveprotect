#20121219
#Jan Mojzis
#Public domain.

#help
if [ x"`effectiveenvuidgid -h 2>&1 | grep '^effectiveenvuidgid: usage:'`" != x"effectiveenvuidgid: usage:" ]; then
  echo "effectiveenvuidgid: help error"; exit 111
fi

#env test
a=`env - PATH="$PATH" effectiveenvuidgid printenv | sed 's/=.*//' | sort | nacl-sha256`
b=1fbb128b25729643ca4b4345b89c94fd5b1b5d11e79c2ceeeb1d471c76f643fe
if [ x"${a}" != x"${b}" ]; then
  echo "effectiveenvuidgid test failed"; exit 111
fi

#exec test
effectiveenvuidgid true || { echo "effectiveenvuidgid: true test failed"; exit 111; }
effectiveenvuidgid false && { echo "effectiveenvuidgid: false test failed"; exit 111; }

#nonexistent
e=`effectiveenvuidgid ./nonexistent 2>&1 || :`
if [ x"${e}" != "xeffectiveenvuidgid: fatal: unable to run ./nonexistent: file does not exist" ]; then
  echo "${e}"; exit 111
fi
