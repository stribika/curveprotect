#20121219
#Jan Mojzis
#Public domain.

#help
if [ x"`decrypt -h 2>&1 | grep '^decrypt: usage:'`" != x"decrypt: usage:" ]; then
  echo "decrypt test failed: help error"; exit 111
fi

#PASSWORD not set
e=`env - PATH=${PATH} decrypt </dev/null 2>&1 | tail -1 || :`
if [ x"${e}" != "xdecrypt: fatal: \$PASSWORD not set" ]; then
  echo "decrypt test failed: ${e}"; exit 111
fi

#zero bytes using empty password
PASSWORD=""; export PASSWORD
e=`encrypt < /dev/null | decrypt 2>&1`
if [ x"${e}" != "x" ]; then
  echo "decrypt test failed: ${e}"; exit 111
fi

#encrypt/decrypt test
for i in `awk 'BEGIN{for(i=0;i<20;++i)print i;exit}'`; do
  export i
  PASSWORD=`randomtext "${i}"`; export PASSWORD
  randombytes "${i}" | encrypt | decrypt 1>/dev/null
done
#for i in `awk 'BEGIN{for(i=4090;i<4100;++i)print i;exit}'`; do
for i in 4095 4096 4097; do
  export i
  PASSWORD=`randomtext "${i}"`; export PASSWORD
  randombytes "${i}" | encrypt | decrypt 1>/dev/null
done

#data forgery
#for i in `awk 'BEGIN{for(i=1024;i<1034;++i)print i;exit}'`; do
for i in 1027 1028 1029; do
  export i
  PASSWORD=`randomtext "${i}"`; export PASSWORD
  e=`randombytes "${i}" | encrypt | randomreplace | decrypt 2>&1 1>/dev/null || :`
  if [ x"${e}" != "xdecrypt: fatal: unable to decrypt: bad checksum" ]; then
    if [ x"${e}" != "xdecrypt: fatal: unable to read input: bad magic" ]; then
      echo "decrypt test failed: ${e}"; exit 111
    fi
  fi
done


#bad password
e=`env PASSWORD=a encrypt < /dev/null | env PASSWORD=b decrypt 2>&1 || :`
if [ x"${e}" != "xdecrypt: fatal: unable to decrypt: bad checksum" ]; then
  echo "decrypt test failed: ${e}"; exit 111
fi


#magic
PASSWORD=ahoj; export PASSWORD
e=`(echo "5sA7"; randombytes 100) | decrypt 2>&1 1>/dev/null || :`
if [ x"${e}" != "xdecrypt: fatal: unable to read input: bad magic" ]; then
  echo "decrypt test failed: ${e}"; exit 111
fi
e=`(echo "5sA2"; randombytes 100) | decrypt 2>&1 1>/dev/null || :`
if [ x"${e}" != "xdecrypt: fatal: unable to decrypt: bad checksum" ]; then
  echo "decrypt test failed: ${e}"; exit 111
fi
e=`(echo "5sA2"; randombytes 100) | decrypt 2>&1 1>/dev/null || :`
if [ x"${e}" != "xdecrypt: fatal: unable to decrypt: bad checksum" ]; then
  echo "decrypt test failed: ${e}"; exit 111
fi
e=`(echo "5sA2"; randombytes 10) | decrypt 2>&1 1>/dev/null || :`
if [ x"${e}" != "xdecrypt: fatal: unable to read input: truncated file" ]; then
  echo "decrypt test failed: ${e}"; exit 111
fi

#encrypted 'ahoj' using aes256+salsa20
PASSWORD=ahoj; export PASSWORD
ciphertext=35734132885cc2911665ba5233907531f703d8e4dff37d5599e1f9379ef3b493122424427cb80588df60b2a93885c8a88ef20ba40b
export ciphertext
e=`echo "${ciphertext}" | hextobin | decrypt 2>&1`
if [ x"${e}" != "xahoj" ]; then
  echo "decrypt test failed: ${e}"; exit 111
fi

#disk full
if [ -w /dev/full ]; then
  e=`echo "${ciphertext}" | hextobin | decrypt 2>&1 > /dev/full || :`
  if [ x"${e}" != x"decrypt: fatal: unable to write output: out of disk space" ]; then
    echo "decrypt test failed: ${e}"; exit 111
  fi
fi
