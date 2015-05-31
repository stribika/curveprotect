#!/bin/sh -e

version=`head -1 conf-version`
curveprotectdir=`head -1 conf-home` #XXX must be absolute path
dnscacheip=`head -1 conf-ip`
dnscryptcacheip=`head -2 conf-ip | tail -1`
tinydnsip=`head -6 conf-ip | tail -1`
usr=`head -1 conf-users`
cfg=`head -2 conf-users | tail -1`
testzone=`head -1 conf-dnstestzone`
testips=`cat conf-openresolvers`
openresolvers=`echo $testips`
ips="`cat conf-ip`"
ips=`echo $ips` #XXX

build="`pwd`/build"
bin="${build}/bin"
sbin="${build}/sbin"
include="$build/include"
lib="$build/lib"
servicedir="$build/servicedir"
servicelinkdir="$build/service"
work="$build/work"
helpers="$build/bin/helpers"
log="$build/log"
etc="$build/etc"
var="$build/var"
tmp="$build/tmp"
www="$build/www"
html="$build/html"
share="$build/share"

if [ -f debug ]; then
  compilerparams="-pedantic -Wno-long-long -Wno-overlength-strings -Wall -Wno-parentheses"
fi

PATH="${bin}:${helpers}:${PATH}"
export PATH

LANG=C
export LANG
LC_ALL=C
export LC_ALL

#cleanup
if [ x"$1" = xclean ]; then
  rm -rf "${build}"
  exit 0
fi


if [ x"$1" = xremove ]; then
  if [ -x "${curveprotectdir}/sbin/_prerm" ]; then
    "${curveprotectdir}/sbin/_prerm"
  fi
  #XXX macosx
  pkgutil -f --unlink com.mojzis.curveprotect 1>/dev/null 2>/dev/null || :
  pkgutil -f --forget com.mojzis.curveprotect 1>/dev/null 2>/dev/null || :
  if [ -x "${curveprotectdir}/sbin/_postrm" ]; then
    "${curveprotectdir}/sbin/_postrm"
  fi
  exit 0
fi

if [ x"$1" = xinstall ]; then
  [ -d "${build}" ] || ./do
  ./do installbin "$2"
  ./do installrest "$2"
  exit 0
fi

if [ x"$1" = xinstallbin ]; then
  [ -x "${bin}/envuidgid" ] || (echo "curveprotect not compiled, compile first!"; exit 111;)

  d=bin
  mkdir -p "$2/${curveprotectdir}/${d}"
  cp -pr "${build}/${d}"/* "$2/${curveprotectdir}/${d}"
  envuidgid root uidgidchown -R "$2/${curveprotectdir}/${d}" || :
  exit 0
fi

if [ x"$1" = xinstallrest ]; then
  [ -x "${bin}/envuidgid" ] || (echo "curveprotect not compiled, compile first!"; exit 111;)

  #bin etc html log servicedir share var www 
  for d in etc html log servicedir share var www; do
    (
      mkdir -p "$2/${curveprotectdir}/${d}"
      cp -pr "${build}/${d}"/* "$2/${curveprotectdir}/${d}"
      envuidgid root uidgidchown -R "$2/${curveprotectdir}/${d}" || :
    )
  done

  #tmp sbin
  for d in tmp sbin service; do
    mkdir -p "$2/${curveprotectdir}/${d}"
    envuidgid root uidgidchown "$2/${curveprotectdir}/${d}" || :
  done

  #lib 
  d=lib
  mkdir -p "$2/${curveprotectdir}/${d}/slownacl" #XXX
  cp -pr "${build}/${d}"/*.py "$2/${curveprotectdir}/${d}"
  cp -pr "${build}/${d}"/slownacl/*.py "$2/${curveprotectdir}/${d}/slownacl" #XXX
  envuidgid root uidgidchown -R "$2/${curveprotectdir}/${d}" || :
  exit 0
fi

#nacl
echo "=== `date` === starting nacl"
if [ ! -x "${bin}/curvecpmessage" ]; then
  ( 
    cd contrib/nacl-20110221; sh do; 
    cd "${build}"
    gzip < log > nacl-20110221-log.gz; rm -f log 
    gzip < data > nacl-20110221-data.gz; rm -f data
    mkdir -p "$share"
    mv nacl-20110221-data.gz nacl-20110221-log.gz "$share"
    mkdir -p "$helpers"
    mv "${bin}/ok"* "${helpers}" 
  )
fi
echo "=== `date` === finishing"

#mkdir -p "$bin"
#mkdir -p "$lib"
#mkdir -p "$tests"
#mkdir -p "$include"
mkdir -p "$servicedir"
mkdir -p "$servicelinkdir"
mkdir -p "$log"
mkdir -p "$etc"
mkdir -p "$var"
mkdir -p "$tmp"
mkdir -p "$www"
mkdir -p "$html"
mkdir -p "$share"
mkdir -p "$sbin"


#dq
echo "=== `date` === starting dq"

okabi \
| awk '
  { if ($1=="amd64" || $1=="ia64" || $1=="ppc64" || $1=="sparcv9" || $1=="mips64") print 1,$1
    else if ($1 == "mips32") print 2,$1
    else print 3,$1
  }
' \
| sort \
| while read okabipriority abi
do
  [ -x "$bin/dq" ] && break

  okc-$abi \
  | while read compiler
  do
    [ -x "$bin/dq" ] && break

    echo "=== `date` === dq $abi $compiler"
    rm -rf "$work"
    mkdir -p "$work/compile"
    cp -pr contrib/dq-20150318/* "$work/compile"

    (
      cd "$work/compile"
      CC=compiler; export CC
      LIBS='-lnacl'; export LIBS
      LDFLAGS="-L${lib}/${abi}"; export LDFLAGS
      CFLAGS="-I${include}/${abi}"; export CFLAGS
      sh -e make-dq.sh 1>dq-20150318.log 2>&1 && cp -p build/bin/dq* "$bin"
      gzip -9 < dq-20150318.log > dq-20150318.log.gz
      mv dq-20150318.log.gz "$share"
    )
  done
done

echo "=== `date` === finishing"


#gnupg
echo "=== `date` === starting gnupg"

[ -x "$bin/gpg" ] || \
  (
    rm -rf "$work"
    mkdir -p "$work/compile"
    cp -pr contrib/gnupg-1.4.19/* "$work/compile"

    cd "$work/compile"
    ./configure --prefix=/opt/curveprotect --enable-minimal --disable-dev-random --disable-card-support --disable-agent-support --disable-exec --disable-photo-viewers --disable-keyserver-helpers --disable-ldap --disable-hkp --disable-finger --disable-generic --disable-keyserver-path --disable-dns-srv --disable-dns-pka --disable-dns-cert --disable-nls --disable-threads --disable-rpath --disable-regex --with-included-zlib 1>/dev/null 2>/dev/null
    make 1>gnupg-1.4.19.log 2>&1 && cp -p g10/gpg "$bin"
    gzip -9 < gnupg-1.4.19.log > gnupg-1.4.19.log.gz
    mv gnupg-1.4.19.log.gz "$share"
  )
echo "=== `date` === finishing"


#djbdns
echo "=== `date` === starting djbdns"

okabi \
| awk '
  { if ($1=="amd64" || $1=="ia64" || $1=="ppc64" || $1=="sparcv9" || $1=="mips64") print 1,$1
    else if ($1 == "mips32") print 2,$1
    else print 3,$1
  }
' \
| sort \
| while read okabipriority abi
do
  [ -x "$bin/dnscache" ] && break

  okc-$abi \
  | while read compiler
  do
    [ -x "$bin/dnscache" ] && break

    echo "=== `date` === djbdns $abi $compiler"
    rm -rf "$work"
    mkdir -p "$work/compile"
    cp contrib/djbdns-1.05/* "$work/compile"
    cp contrib/djbdns.TARGETS "$work/compile"
    (
      cd "$work/compile"
      echo "$compiler" > conf-cc
      echo "$compiler" > conf-ld
      echo "${include}/${abi}" > nacl.inc
      (
        echo "$lib/$abi/randombytes.o"
        echo "$lib/$abi/libnacl.a"
      ) > nacl.lib
      make 1>djbdns-1.05.log 2>&1 && cp -p `cat djbdns.TARGETS` "$bin"
      gzip -9 < djbdns-1.05.log > djbdns-1.05.log.gz
      mv djbdns-1.05.log.gz "$share"
    )
  done
done

echo "=== `date` === finishing"

#ucspi-tcp
echo "=== `date` === starting ucspi-tcp"

okabi \
| awk '
  { if ($1=="amd64" || $1=="ia64" || $1=="ppc64" || $1=="sparcv9" || $1=="mips64") print 1,$1
    else if ($1 == "mips32") print 2,$1
    else print 3,$1
  }
' \
| sort \
| while read okabipriority abi
do
  [ -x "$bin/tcpserver" ] && break

  okc-$abi \
  | while read compiler
  do
    [ -x "$bin/tcpserver" ] && break

    echo "=== `date` === ucspi-tcp $abi $compiler"
    rm -rf "$work"
    mkdir -p "$work/compile"
    cp contrib/ucspi-tcp-0.88/* "$work/compile"
    cp contrib/ucspi-tcp.TARGETS "$work/compile"
    (
      cd "$work/compile"
      echo "$compiler -I$include/$abi" > conf-cc
      echo "$compiler" > conf-ld
      make 1>ucspi-tcp-0.88.log 2>&1 && cp -p `cat ucspi-tcp.TARGETS` "$bin"
      gzip -9 < ucspi-tcp-0.88.log > ucspi-tcp-0.88.log.gz
      mv ucspi-tcp-0.88.log.gz "$share"
    )
  done

done

echo "=== `date` === finishing"

#daemontools
echo "=== `date` === starting daemontools"

okabi \
| awk '
  { if ($1=="amd64" || $1=="ia64" || $1=="ppc64" || $1=="sparcv9" || $1=="mips64") print 1,$1
    else if ($1 == "mips32") print 2,$1
    else print 3,$1
  }
' \
| sort \
| while read okabipriority abi
do
  [ -x "$bin/svc" ] && break

  okc-$abi \
  | while read compiler
  do
    [ -x "$bin/svc" ] && break

    echo "=== `date` === daemontools $abi $compiler"
    rm -rf "$work"
    mkdir -p "$work/compile"
    cp contrib/daemontools-0.76/* "$work/compile"
    cp contrib/daemontools.TARGETS "$work/compile"
    (
      cd "$work/compile"
      echo "$compiler -I$include/$abi" > conf-cc
      echo "$compiler" > conf-ld
      echo "$build" > home
      make 1>daemontools-0.76.log 2>&1 && cp -p `cat daemontools.TARGETS` "$bin"
      gzip -9 < daemontools-0.76.log > daemontools-0.76.log.gz
      mv daemontools-0.76.log.gz "$share"
    )
  done

done

echo "=== `date` === finishing"


echo "=== `date` === building portable headers"
rm -rf "$work"
mkdir -p "$work"
cp -pr source/portable/* "$work"
( cd "$work" && sh do )
cp -pr "$work"/include/* "$include/"
echo "=== `date` === finishing"

echo "=== `date` === starting libs"
okabi \
| while read abi
do
  [ -f "${lib}/${abi}/libs.a" ] && break
  okc-${abi} \
  | while read compiler
  do
    [ -f "${lib}/${abi}/libs.a" ] && break
    compiler="${compiler} ${compilerparams}"
    echo "=== `date` === libs $abi $compiler"
    rm -rf "${work}"
    mkdir -p "${work}/compile"
    cp source/libs/* "${work}/compile"
    (
      cd "${work}/compile"
      ls *.c | sort\
      | while read x
      do
        ${compiler} -I"${include}" -I"${include}/${abi}" -c "${x}"
      done
      if okar-${abi} cr libs.a *.o
      then
        cp -p libs.a "${lib}/${abi}"
        cp -p *.h "${include}/${abi}"
      fi
    )
    rm -rf "${work}"
    mkdir -p "${work}/compile"
    cp source/libs/* "${work}/compile"
    (
      cd "${work}/compile"
      ls *.c | sort\
      | while read x
      do
        ${compiler} -DTEST -I"${include}" -I"${include}/${abi}" -c "${x}"
      done
      if okar-${abi} cr testlibs.a *.o
      then
        cp -p testlibs.a "${lib}/${abi}"
      fi
    )
  done
done
echo "=== `date` === finishing"


echo "=== `date` === starting tests"
okabi \
| awk '
  { if ($1=="amd64" || $1=="ia64" || $1=="ppc64" || $1=="sparcv9" || $1=="mips64") print 1,$1
    else if ($1 == "mips32") print 2,$1
    else print 3,$1
  }
' \
| sort \
| while read okabipriority abi
do
  [ -x "$work/compile/alloctest" ] && break
  libs=""
  libs="$libs $lib/$abi/testlibs.a"
  libs="$libs $lib/$abi/randombytes.o"
  libs="$libs $lib/$abi/libnacl.a"
  libs="$libs `oklibs-$abi`"
  okc-$abi \
  | while read compiler
  do
    [ -x "$work/compile/alloctest" ] && break
    compiler="${compiler} ${compilerparams}"
    echo "=== `date` === tests $abi $compiler"
    rm -rf "${work}"
    mkdir -p "${work}/compile"
    cp source/tests/* "$work/compile"
    (
      cd "${work}/compile"
      ls *.c | sort\
      | while read xx
      do
        x=`echo "${xx}" | sed 's/\.c$//'`
        ${compiler} -DTEST -I"${include}" -I"${include}/${abi}" -c "${x}.c"
        ${compiler} -DTEST -I"${include}" -I"${include}/${abi}" -o "${x}" "${x}.o" $libs
        ./${x}
      done
    )
  done
done
echo "=== `date` === finishing"


echo "=== `date` === starting helpers"
okabi \
| awk '
  { if ($1=="amd64" || $1=="ia64" || $1=="ppc64" || $1=="sparcv9" || $1=="mips64") print 1,$1
    else if ($1 == "mips32") print 2,$1
    else print 3,$1
  }
' \
| sort \
| while read okabipriority abi
do
  [ -x "$helpers/crxmake" ] && break
  libs=""
  libs="$libs $lib/$abi/libs.a"
  libs="$libs $lib/$abi/randombytes.o"
  libs="$libs $lib/$abi/libnacl.a"
  libs="$libs `oklibs-$abi`"
  okc-$abi \
  | while read compiler
  do
    [ -x "$helpers/crxmake" ] && break
    compiler="${compiler} ${compilerparams}"
    echo "=== `date` === helpers $abi $compiler"
    rm -rf "$work"
    mkdir -p "$work/compile"
    cp source/helpers/* "$work/compile"
    cp source/rts.sh "$work/compile"
    (
      cd "$work/compile"
      cat SOURCES \
      | while read x
      do
        $compiler -I"$include" -I"$include/$abi" -c "$x.c"
      done

      if okar-$abi cr helperslibs.a `cat LIBS`
      then
        cat TARGETS \
        | while read x
        do
          $compiler -I"$include" -I"$include/$abi" \
          -o "$x" "$x.o" \
          helperslibs.a $libs \
          && sh rts.sh "$x" && cp -p "$x" "$helpers/$x"
        done
      fi
    )
  done
done
echo "=== `date` === finishing"

echo "=== `date` === starting ed25519tools"
okabi \
| awk '
  { if ($1=="amd64" || $1=="ia64" || $1=="ppc64" || $1=="sparcv9" || $1=="mips64") print 1,$1
    else if ($1 == "mips32") print 2,$1
    else print 3,$1
  }
' \
| sort \
| while read okabipriority abi
do
  [ -x "$bin/ed25519signopen" ] && break
  libs=""
  libs="$libs $lib/$abi/libs.a"
  libs="$libs $lib/$abi/randombytes.o"
  libs="$libs $lib/$abi/libnacl.a"
  libs="$libs `oklibs-$abi`"
  okc-$abi \
  | while read compiler
  do
    [ -x "$bin/ed25519signopen" ] && break
    compiler="${compiler} ${compilerparams}"
    echo "=== `date` === ed25519tools $abi $compiler"
    rm -rf "$work"
    mkdir -p "$work/compile"
    cp source/ed25519tools/* "$work/compile"
    (
      cd "$work/compile"
      cat SOURCES \
      | while read x
      do
        $compiler -I"$include" -I"$include/$abi" -c "$x.c"
      done
      if okar-$abi cr ed25519libs.a `cat LIBS`
      then
        cat TARGETS \
        | while read x
        do
          $compiler -I"$include" -I"$include/$abi" \
          -o "$x" "$x.o" \
          ed25519libs.a $libs \
          && cp -p "$x" "$bin/$x"
        done
      fi
    )
  done
done
echo "=== `date` === finishing"

echo "=== `date` === starting tools"
okabi \
| awk '
  { if ($1=="amd64" || $1=="ia64" || $1=="ppc64" || $1=="sparcv9" || $1=="mips64") print 1,$1
    else if ($1 == "mips32") print 2,$1
    else print 3,$1
  }
' \
| sort \
| while read okabipriority abi
do
  [ -x "$bin/decrypt" ] && break
  libs=""
  libs="$libs $lib/$abi/libs.a"
  libs="$libs $lib/$abi/randombytes.o"
  libs="$libs $lib/$abi/libnacl.a"
  libs="$libs `oklibs-$abi`"
  okc-$abi \
  | while read compiler
  do
    [ -x "$bin/decrypt" ] && break
    compiler="${compiler} ${compilerparams}"
    echo "=== `date` === tools $abi $compiler"
    rm -rf "$work"
    mkdir -p "$work/compile"
    cp source/tools/* "$work/compile"
    cp source/rts.sh "$work/compile"
    (
      cd "$work/compile"
      cat SOURCES \
      | while read x
      do
        $compiler -I"$include" -I"$include/$abi" -c "$x.c"
      done
      if okar-$abi cr toolslibs.a `cat LIBS`
      then
        cat TARGETS \
        | while read x
        do
          $compiler -I"$include" -I"$include/$abi" \
          -o "$x" "$x.o" \
          toolslibs.a $libs \
          && sh rts.sh "$x" && cp -p "$x" "$bin/$x"
        done
      fi
    )
  done
done
echo "=== `date` === finishing"

echo "=== `date` === starting dnscrypttools"
okabi \
| awk '
  { if ($1=="amd64" || $1=="ia64" || $1=="ppc64" || $1=="sparcv9" || $1=="mips64") print 1,$1
    else if ($1 == "mips32") print 2,$1
    else print 3,$1
  }
' \
| sort \
| while read okabipriority abi
do
  [ -x "$bin/dnscryptserver" ] && break
  libs=""
  libs="$libs $lib/$abi/libs.a"
  libs="$libs $lib/$abi/randombytes.o"
  libs="$libs $lib/$abi/libnacl.a"
  libs="$libs `oklibs-$abi`"
  okc-$abi \
  | while read compiler
  do
    [ -x "$bin/dnscryptserver" ] && break
    compiler="${compiler} ${compilerparams}"
    echo "=== `date` === dnscrypttools $abi $compiler"
    rm -rf "$work"
    mkdir -p "$work/compile"
    cp source/dnscrypttools/* "$work/compile"
    cp source/rts.sh "$work/compile"
    (
      cd "$work/compile"
      cat SOURCES \
      | while read x
      do
        $compiler -I"$include" -I"$include/$abi" -c "$x.c"
      done
      if okar-$abi cr dnscrypttoolslibs.a `cat LIBS`
      then
        cat TARGETS \
        | while read x
        do
          $compiler -I"$include" -I"$include/$abi" \
          -o "$x" "$x.o" \
          dnscrypttoolslibs.a $libs \
          && sh rts.sh "$x" && cp -p "$x" "$bin/$x"
        done
      fi
    )
  done
done
echo "=== `date` === finishing"

echo "=== `date` === starting dnszone"
okabi \
| awk '
  { if ($1=="amd64" || $1=="ia64" || $1=="ppc64" || $1=="sparcv9" || $1=="mips64") print 1,$1
    else if ($1 == "mips32") print 2,$1
    else print 3,$1
  }
' \
| sort \
| while read okabipriority abi
do
  [ -x "$bin/dnszonedownload" ] && break
  libs=""
  libs="$libs $lib/$abi/libs.a"
  libs="$libs $lib/$abi/randombytes.o"
  libs="$libs $lib/$abi/libnacl.a"
  libs="$libs `oklibs-$abi`"
  okc-$abi \
  | while read compiler
  do
    [ -x "$bin/dnszonedownload" ] && break
    compiler="${compiler} ${compilerparams}"
    echo "=== `date` === dnszone $abi $compiler"
    rm -rf "$work"
    mkdir -p "$work/compile"
    cp source/dnszone/* "$work/compile"
    cp source/rts.sh "$work/compile"
    (
      cd "$work/compile"
      cat SOURCES \
      | while read x
      do
        $compiler -I"$include" -I"$include/$abi" -c "$x.c"
      done
      if okar-$abi cr dnszonelibs.a `cat LIBS`
      then
        cat TARGETS \
        | while read x
        do
          $compiler -I"$include" -I"$include/$abi" \
          -o "$x" "$x.o" \
          dnszonelibs.a $libs \
          && sh rts.sh "$x" && cp -p "$x" "$bin/$x"
        done
      fi
    )
  done
done
echo "=== `date` === finishing"

echo "=== `date` === building tun.c"
rm -rf "$work"
mkdir -p "$work"
cp -pr source/tun/* "$work"
( cd "$work" && sh do )
echo "=== `date` === finishing"

echo "=== `date` === starting vpn"
okabi \
| awk '
  { if ($1=="amd64" || $1=="ia64" || $1=="ppc64" || $1=="sparcv9" || $1=="mips64") print 1,$1
    else if ($1 == "mips32") print 2,$1
    else print 3,$1
  }
' \
| sort \
| while read okabipriority abi
do
  [ -x "$bin/vpn" ] && break
  libs=`"oklibs-$abi"`
  okc-$abi \
  | while read compiler
  do
    [ -x "$bin/vpn" ] && break
    echo "$compiler" | grep "\-O3" >/dev/null && continue
    compiler="${compiler} ${compilerparams}"
    echo "=== `date` === vpn $abi $compiler"
    rm -rf "$work/compile"
    mkdir -p "$work/compile"
    cp source/vpn/* "$work/compile"
    cp "$include/$abi/crypto_uint"* "$work/compile"
    cp "$work/tun/$abi/tun.c" "$work/compile"
    (
      cd "$work/compile"
      cat SOURCES \
      | while read x
      do
        $compiler -c "$x.c"
      done
      if okar-$abi cr vpnlibs.a `cat LIBS`
      then
        cat TARGETS \
        | while read x
        do
          $compiler -I"$include" -I"$include/$abi" \
          -o "$x" "$x.o" \
          vpnlibs.a $libs \
          && cp -p "$x" "$bin/$x"
        done
      fi
    )
  done
done
echo "=== `date` === finishing"

echo "=== `date` === starting http"
okabi \
| awk '
  { if ($1=="amd64" || $1=="ia64" || $1=="ppc64" || $1=="sparcv9" || $1=="mips64") print 1,$1
    else if ($1 == "mips32") print 2,$1
    else print 3,$1
  }
' \
| sort \
| while read okabipriority abi
do
  [ -x "$bin/httpproxy" ] && break
  libs=`"oklibs-$abi"`
  okc-$abi \
  | while read compiler
  do
    [ -x "$bin/httpproxy" ] && break
    echo "$compiler" | grep "\-O3" >/dev/null && continue
    compiler="${compiler} ${compilerparams}"
    echo "=== `date` === http $abi $compiler"
    rm -rf "$work/compile"
    mkdir -p "$work/compile"
    cp source/http/* "$work/compile"
    cp "$include/$abi/crypto_uint"* "$work/compile"
    (
      cd "$work/compile"
      cat SOURCES \
      | while read x
      do
        $compiler -c "$x.c"
      done
      if okar-$abi cr httplibs.a `cat LIBS`
      then
        cat TARGETS \
        | while read x
        do
          $compiler -I"$include" -I"$include/$abi" \
          -o "$x" "$x.o" \
          httplibs.a $libs \
          && cp -p "$x" "$bin/$x"
        done
      fi
    )
  done
done
echo "=== `date` === finishing"


echo "=== `date` === starting nettools"

okabi \
| awk '
  { if ($1=="amd64" || $1=="ia64" || $1=="ppc64" || $1=="sparcv9" || $1=="mips64") print 1,$1
    else if ($1 == "mips32") print 2,$1
    else print 3,$1
  }
' \
| sort \
| while read okabipriority abi
do
  [ -x "$bin/nettcpclient" ] && break
  libs=""
  libs="$libs $lib/$abi/libs.a"
  libs="$libs $lib/$abi/randombytes.o"
  libs="$libs $lib/$abi/libnacl.a"
  libs="$libs `oklibs-$abi`"

  okc-$abi \
  | while read compiler
  do
    [ -x "$bin/nettcpclient" ] && break
    compiler="${compiler} ${compilerparams}"
    echo "=== `date` === nettools $abi $compiler"
    rm -rf "$work"
    mkdir -p "$work/compile"
    cp source/nettools/* "$work/compile"
    (
      cd "$work/compile"
      cat SOURCES \
      | while read x
      do
        $compiler -I"$include" -I"$include/$abi" -c "$x.c"
      done
      if okar-$abi cr nettoolslibs.a `cat LIBS`
      then
        cat TARGETS \
        | while read x
        do
          $compiler -I"$include" -I"$include/$abi" \
          -o "$x" "$x.o" \
          nettoolslibs.a $libs \
          && cp -p "$x" "$bin/$x"
        done
      fi
    )
  done
done
echo "=== `date` === finishing"

echo "=== `date` === starting bindparser"
okabi \
| awk '
  { if ($1=="amd64" || $1=="ia64" || $1=="ppc64" || $1=="sparcv9" || $1=="mips64") print 1,$1
    else if ($1 == "mips32") print 2,$1
    else print 3,$1
  }
' \
| sort \
| while read okabipriority abi
do
  [ -x "$bin/bindparser" ] && break
  libs=""
  libs="$libs $lib/$abi/libs.a"
  libs="$libs $lib/$abi/randombytes.o"
  libs="$libs $lib/$abi/libnacl.a"
  libs="$libs `oklibs-$abi`"
  okc-$abi \
  | while read compiler
  do
    [ -x "$bin/bindparser" ] && break
    #compiler="${compiler} ${compilerparams}"
    echo "=== `date` === bindparser $abi $compiler"
    rm -rf "$work"
    mkdir -p "$work/compile"
    cp -pr contrib/knot-1.3.3/src/* "$work/compile"
    cp -p source/bindparser/* "$work/compile"
    cp source/rts.sh "$work/compile"
    (
      cd "$work/compile"
      cat SOURCES \
      | while read x
      do
        $compiler -I. -Ilibknot -I"$include" -I"$include/$abi" -c "$x.c"
      done
      if okar-$abi cr bindparserlibs.a `cat LIBS`
      then
        cat TARGETS \
        | while read x
        do
          $compiler -I"$include" -I"$include/$abi" \
          -o "$x" "$x.o" \
          bindparserlibs.a $libs \
          && sh rts.sh "$x" && cp -p "$x" "$bin/$x"
        done
      fi
    )
  done
done
echo "=== `date` === finishing"


echo "=== `date` === starting scripts"
rm -rf "$work"
mkdir -p "$work/scripts"
cp source/scripts/* "$work/scripts"
(
  cd "$work/scripts"
  cat TARGETS \
  | while read x
  do
    sed "s,_CURVEPROTECT_,${curveprotectdir},g" < "$x.sh" | sed "s,_CPCFG_,${cfg},g" | sed "s,_CPUSR_,${usr},g" | sed "s,_DNSCACHEIP_,${dnscacheip},g" | sed "s/_DNSCRYPTCACHEIP_/${dnscryptcacheip}/g" | sed "s/_CURVEPROTECTIPS_/${ips}/g" > "$x.tmp"
    chmod 755 "$x.tmp" && mv -f "$x.tmp" "$x" && cp -p "$x" "$bin/$x"
  done
)
echo "=== `date` === finishing"

echo "=== `date` === starting program check"
(
  cat `find source -name TARGETS | grep -v '/tests/'`
  cat contrib/*.TARGETS
) | while read x
do
  [ -x "${bin}/${x}" ] && continue
  [ -x "${helpers}/${x}" ] && continue
  echo "program ${x} failed !!!!"
  exit 111
done || exit 111
echo "=== `date` === finishing"


echo "=== `date` === starting servicedir"
rm -rf "$work"
mkdir -p "$work/servicedir"
cp -pr dirs/servicedir/* "$work/servicedir"
(
  cd "$work"
  ls servicedir \
  | sort \
  | while read d
  do
    mkdir -p "$servicedir/$d/log"
    sed "s,_CURVEPROTECT_,${curveprotectdir},g" < "servicedir/$d/run" | sed "s,_CPCFG_,${cfg},g" | sed "s,_CPUSR_,${usr},g" > "servicedir/$d/run.tmp"
    chmod 755 "servicedir/$d/run.tmp"
    cp -p "servicedir/$d/run.tmp" "$servicedir/$d/run"
    (
      num=2
      size=99999
      if [ "x$d" = xhttpproxy ]; then
        num=5
        size=9999999
      fi
      if [ "x$d" = xdnscache ]; then
        num=5
        size=99999999
      fi
      echo "#!/bin/sh"
      echo "PATH=\"${curveprotectdir}/bin:\${PATH}\""
      echo "export PATH"
      echo ""
      echo "exec extremeenvuidgid sh -c '"
      echo "  uidgidchown -R ${curveprotectdir}/log/${d}"
      echo "  exec extremesetuidgid multilog t n${num} s${size} ${curveprotectdir}/log/${d}"
      echo "'"
    ) > "servicedir/$d/run.log"
    chmod 755 "servicedir/$d/run.log"
    cp -p "servicedir/$d/run.log" "$servicedir/$d/log/run"
    mkdir -p "${log}/${d}"
  done
)
echo "=== `date` === finishing"

echo "=== `date` === starting etc, lib, www, html, share"

(
  echo "dirs/etc ${etc}"
  echo "dirs/lib ${lib}"
  echo "dirs/www ${www}"
  echo "dirs/html ${html}"
  echo "dirs/var ${var}"
  echo "dirs/share ${share}"
  exit 0
) | (
  while read s d
  do
    find "${s}" \
    | sort \
    | while read x
    do
      y=`echo "${x}" | sed s}"${s}"}"${d}"}g`
      z=`echo "${x}" | sed 's}.*\.}}'`
      [ -d "$x" ] && mkdir -p "$y"
      [ -d "$x" ] && continue
      [ x"${z}" = "xpng" ] && cp "$x" "$y"
      [ x"${z}" = "xpng" ] && continue
      [ -f "$x" ] && sed "s,_CURVEPROTECT_,${curveprotectdir},g" < "$x" | sed "s/_DNSCACHEIP_/${dnscacheip}/g" | sed "s/_DNSCRYPTCACHEIP_/${dnscryptcacheip}/g" | sed "s/_TINYDNSIP_/${tinydnsip}/g" | sed "s/_OPENRESOVERS_/${openresolvers}/g" > "$y"
      [ -x "$x" ] && chmod 755 "$y"
      [ -f "$x" ] && continue
    done 
  done
)

echo "=== `date` === starting lib/config.py"

(
  echo "#autogenerated, do not edit!!!"
  (
    echo "forwarder_ips = [\\"
    cat conf-ip \
    | grep -v "${dnscacheip}" \
    | while read ip
    do
      echo "\"${ip}\",\\"
    done
    echo "]"
  )
  echo "basedir = \"${curveprotectdir}\""
  echo "tinydnsip = \"${tinydnsip}\""
  echo "dnscacheip = \"${dnscacheip}\""
  echo "dnscryptcacheip = \"${dnscryptcacheip}\""
  echo "version = \"${version}\""
  echo "servicedir = \"servicedir\""
  #XXX
  echo "testzone = \"${testzone}\""
  echo "testips = [\\"
  for ip in $testips; do
    echo "\"$ip\",\\"
  done
  echo "]"
) > "${lib}/config.py"

echo "=== `date` === finishing"

echo "=== `date` === starting contrib"
cp -p contrib/*.asc "${share}"
cp -p contrib/*.pk "${share}"
mkdir -p "${lib}/slownacl"
cp -pr contrib/mdempsky-dnscurve/slownacl/* "${lib}/slownacl"
echo "=== `date` === finishing"

echo "=== `date` === starting chrome extension"

v1=`cut -b 1-4 < conf-version`
v2=`cut -b 5-6 < conf-version | sed 's/^0//'`
v3=`cut -b 7-8 < conf-version | sed 's/^0//'`
chromeversion="$v1.$v2.$v3"
chromebuild="${work}/chromebuild"

rm -rf "${chromebuild}"
mkdir -p "${chromebuild}"

#add chrome version 
(
  cd "${build}/share/chrome/"
  sed "s/_VERSION_/${chromeversion}/" < manifest.json > manifest.json.tmp
  mv -f manifest.json.tmp manifest.json
)

cp -pr "${build}/share/chrome/"* "${chromebuild}"

#key="content.key"
#sig="content.sig"
#pub="content.pub"
#tmp="content.tmp"
#zip="${build}/share/curveprotect-${version}.zip"
#crx="${build}/share/curveprotect-${version}.crx"
#
#(
#  exec 1>/dev/null
#  exec 2>/dev/null
#  cd "${chromebuild}"
#
#  #add version
#  #sed "s/_VERSION_/${chromeversion}/" < manifest.json > manifest.json.tmp
#  #mv -f manifest.json.tmp manifest.json
#
#  #zip content
#  zip -q "${zip}" *
#
#  if [ -f conf-rsa ]; then
#    cp `head -1 conf-rsa` "${key}" || :
#  fi
#
#  #create RSA 
#  [ -f "${key}" ] || openssl genrsa -out "${key}" 1024 2>/dev/null
#
#  #sign 
#  openssl sha1 -sha1 -binary -sign "${key}" < "${zip}" > "${sig}"
#
#  #export public key
#  openssl rsa -pubout -outform DER < "${key}" > "${pub}" 2>/dev/null
#
#  #create package
#  crxmake "${crx}" "${tmp}" "${pub}" "${sig}" "${zip}"
#) || :
echo "=== `date` === finishing"

rm -rf "$work"

if [ x"$1" = xsetup ]; then

  #check
  #gpg --help 1>/dev/null 2>/dev/null || (echo "gpg not instaled"; exit 111;)

  ./do install "$2"

  #copy scripts to sbin
  cp -p "${curveprotectdir}/bin/_"* "${curveprotectdir}/sbin"

  "${curveprotectdir}/sbin/_postinst"
fi
