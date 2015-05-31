#20130119
#Jan Mojzis
#Public domain.

sig=content.sig; export sig
pub=content.pub; export pub
zip=content.zip; export zip
tmp=content.tmp; export tmp
crx=content.crx; export crx

#untar 
tar -xf crxmake.tar

#disk full
if [ -w /dev/full ]; then
  e=`crxmake "${crx}" /dev/full "${pub}" "${sig}" "${zip}" 2>&1 || :`
  if [ x"${e}" != x"crxmake: fatal: unable to write to file /dev/full: out of disk space" ]; then
    echo "crxmake test failed: ${e}"; exit 111
  fi
fi

crxmake "${crx}" "${tmp}" "${pub}" "${sig}" "${zip}"
a=`nacl-sha256 < "${crx}"`
b=8ddec86319be69d5ab9467b37c2baf71aa4d355d4fc11354da6698e7b7907580
if [ x"${a}" != x"${b}" ]; then
  echo "crxmake test failed"; exit 111
fi
