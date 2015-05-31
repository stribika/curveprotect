# Don't edit Makefile! Use conf-* for configuration.

SHELL=/bin/sh

default: it

addcr: \
load addcr.o unix.a byte.a
	./load addcr unix.a byte.a 

addcr.o: \
compile addcr.c buffer.h exit.h
	./compile addcr.c

alloc.o: \
compile alloc.c alloc.h error.h
	./compile alloc.c

alloc_re.o: \
compile alloc_re.c alloc.h byte.h
	./compile alloc_re.c

argv0: \
load argv0.o unix.a byte.a
	./load argv0 unix.a byte.a 

argv0.o: \
compile argv0.c pathexec.h strerr.h
	./compile argv0.c

auto-str: \
load auto-str.o unix.a byte.a
	./load auto-str unix.a byte.a 

auto-str.o: \
compile auto-str.c buffer.h readwrite.h exit.h
	./compile auto-str.c

auto_home.c: \
auto-str conf-home
	./auto-str auto_home `head -1 conf-home` > auto_home.c

auto_home.o: \
compile auto_home.c
	./compile auto_home.c

buffer.o: \
compile buffer.c buffer.h
	./compile buffer.c

buffer_0.o: \
compile buffer_0.c readwrite.h buffer.h
	./compile buffer_0.c

buffer_1.o: \
compile buffer_1.c readwrite.h buffer.h
	./compile buffer_1.c

buffer_2.o: \
compile buffer_2.c readwrite.h buffer.h
	./compile buffer_2.c

buffer_copy.o: \
compile buffer_copy.c buffer.h
	./compile buffer_copy.c

buffer_get.o: \
compile buffer_get.c buffer.h byte.h error.h
	./compile buffer_get.c

buffer_put.o: \
compile buffer_put.c buffer.h str.h byte.h error.h
	./compile buffer_put.c

byte.a: \
makelib byte_chr.o byte_copy.o byte_cr.o byte_diff.o byte_rchr.o \
byte_zero.o case_diffb.o case_diffs.o fmt_ulong.o ip4_fmt.o \
ip4_scan.o scan_ulong.o str_chr.o str_diff.o str_len.o str_start.o \
uint16_pack.o uint16_unpack.o uint32_pack.o uint32_unpack.o
	./makelib byte.a byte_chr.o byte_copy.o byte_cr.o \
	byte_diff.o byte_rchr.o byte_zero.o case_diffb.o \
	case_diffs.o fmt_ulong.o ip4_fmt.o ip4_scan.o scan_ulong.o \
	str_chr.o str_diff.o str_len.o str_start.o uint16_pack.o \
	uint16_unpack.o uint32_pack.o uint32_unpack.o

byte_chr.o: \
compile byte_chr.c byte.h
	./compile byte_chr.c

byte_copy.o: \
compile byte_copy.c byte.h
	./compile byte_copy.c

byte_cr.o: \
compile byte_cr.c byte.h
	./compile byte_cr.c

byte_diff.o: \
compile byte_diff.c byte.h
	./compile byte_diff.c

byte_rchr.o: \
compile byte_rchr.c byte.h
	./compile byte_rchr.c

byte_zero.o: \
compile byte_zero.c byte.h
	./compile byte_zero.c

case_diffb.o: \
compile case_diffb.c case.h
	./compile case_diffb.c

case_diffs.o: \
compile case_diffs.c case.h
	./compile case_diffs.c

cdb.a: \
makelib cdb.o cdb_hash.o cdb_make.o
	./makelib cdb.a cdb.o cdb_hash.o cdb_make.o

cdb.o: \
compile cdb.c readwrite.h error.h seek.h byte.h cdb.h uint32.h
	./compile cdb.c

cdb_hash.o: \
compile cdb_hash.c cdb.h uint32.h
	./compile cdb_hash.c

cdb_make.o: \
compile cdb_make.c readwrite.h seek.h error.h alloc.h cdb.h uint32.h \
cdb_make.h buffer.h uint32.h
	./compile cdb_make.c

check: \
it instcheck
	./instcheck

chkshsgr: \
load chkshsgr.o
	./load chkshsgr 

chkshsgr.o: \
compile chkshsgr.c exit.h
	./compile chkshsgr.c

choose: \
warn-auto.sh choose.sh conf-home
	cat warn-auto.sh choose.sh \
	| sed s}HOME}"`head -1 conf-home`"}g \
	> choose
	chmod 755 choose

commands.o: \
compile commands.c buffer.h stralloc.h gen_alloc.h str.h case.h \
commands.h
	./compile commands.c

compile: \
warn-auto.sh conf-cc
	( cat warn-auto.sh; \
	echo exec "`head -1 conf-cc`" '-c $${1+"$$@"}' \
	) > compile
	chmod 755 compile

date@: \
warn-auto.sh date@.sh conf-home
	cat warn-auto.sh date@.sh \
	| sed s}HOME}"`head -1 conf-home`"}g \
	> date@
	chmod 755 date@

delcr: \
load delcr.o unix.a byte.a
	./load delcr unix.a byte.a 

delcr.o: \
compile delcr.c buffer.h exit.h
	./compile delcr.c

dns.a: \
makelib dns_dfd.o dns_domain.o dns_dtda.o dns_ip.o dns_ipq.o \
dns_name.o dns_nd.o dns_packet.o dns_random.o dns_rcip.o dns_rcrw.o \
dns_resolve.o dns_sortip.o dns_transmit.o dns_txt.o
	./makelib dns.a dns_dfd.o dns_domain.o dns_dtda.o dns_ip.o \
	dns_ipq.o dns_name.o dns_nd.o dns_packet.o dns_random.o \
	dns_rcip.o dns_rcrw.o dns_resolve.o dns_sortip.o \
	dns_transmit.o dns_txt.o

dns_dfd.o: \
compile dns_dfd.c error.h alloc.h byte.h dns.h stralloc.h gen_alloc.h \
iopause.h taia.h tai.h uint64.h taia.h
	./compile dns_dfd.c

dns_domain.o: \
compile dns_domain.c error.h alloc.h case.h byte.h dns.h stralloc.h \
gen_alloc.h iopause.h taia.h tai.h uint64.h taia.h
	./compile dns_domain.c

dns_dtda.o: \
compile dns_dtda.c stralloc.h gen_alloc.h dns.h stralloc.h iopause.h \
taia.h tai.h uint64.h taia.h
	./compile dns_dtda.c

dns_ip.o: \
compile dns_ip.c stralloc.h gen_alloc.h uint16.h byte.h dns.h \
stralloc.h iopause.h taia.h tai.h uint64.h taia.h
	./compile dns_ip.c

dns_ipq.o: \
compile dns_ipq.c stralloc.h gen_alloc.h case.h byte.h str.h dns.h \
stralloc.h iopause.h taia.h tai.h uint64.h taia.h
	./compile dns_ipq.c

dns_name.o: \
compile dns_name.c stralloc.h gen_alloc.h uint16.h byte.h dns.h \
stralloc.h iopause.h taia.h tai.h uint64.h taia.h
	./compile dns_name.c

dns_nd.o: \
compile dns_nd.c byte.h fmt.h dns.h stralloc.h gen_alloc.h iopause.h \
taia.h tai.h uint64.h taia.h
	./compile dns_nd.c

dns_packet.o: \
compile dns_packet.c error.h dns.h stralloc.h gen_alloc.h iopause.h \
taia.h tai.h uint64.h taia.h
	./compile dns_packet.c

dns_random.o: \
compile dns_random.c dns.h stralloc.h gen_alloc.h iopause.h taia.h \
tai.h uint64.h taia.h taia.h uint32.h
	./compile dns_random.c

dns_rcip.o: \
compile dns_rcip.c taia.h tai.h uint64.h openreadclose.h stralloc.h \
gen_alloc.h byte.h ip4.h env.h dns.h stralloc.h iopause.h taia.h \
taia.h
	./compile dns_rcip.c

dns_rcrw.o: \
compile dns_rcrw.c taia.h tai.h uint64.h env.h byte.h str.h \
openreadclose.h stralloc.h gen_alloc.h dns.h stralloc.h iopause.h \
taia.h taia.h
	./compile dns_rcrw.c

dns_resolve.o: \
compile dns_resolve.c iopause.h taia.h tai.h uint64.h taia.h byte.h \
dns.h stralloc.h gen_alloc.h iopause.h taia.h
	./compile dns_resolve.c

dns_sortip.o: \
compile dns_sortip.c byte.h dns.h stralloc.h gen_alloc.h iopause.h \
taia.h tai.h uint64.h taia.h
	./compile dns_sortip.c

dns_transmit.o: \
compile dns_transmit.c socket.h uint16.h alloc.h error.h byte.h \
readwrite.h uint16.h dns.h stralloc.h gen_alloc.h iopause.h taia.h \
tai.h uint64.h taia.h
	./compile dns_transmit.c

dns_txt.o: \
compile dns_txt.c stralloc.h gen_alloc.h uint16.h byte.h dns.h \
stralloc.h iopause.h taia.h tai.h uint64.h taia.h
	./compile dns_txt.c

env.o: \
compile env.c str.h env.h
	./compile env.c

error.o: \
compile error.c error.h
	./compile error.c

error_str.o: \
compile error_str.c error.h
	./compile error_str.c

fd_copy.o: \
compile fd_copy.c fd.h
	./compile fd_copy.c

fd_move.o: \
compile fd_move.c fd.h
	./compile fd_move.c

finger@: \
warn-auto.sh finger@.sh conf-home
	cat warn-auto.sh finger@.sh \
	| sed s}HOME}"`head -1 conf-home`"}g \
	> finger@
	chmod 755 finger@

fixcrio: \
load fixcrio.o time.a unix.a byte.a
	./load fixcrio time.a unix.a byte.a 

fixcrio.o: \
compile fixcrio.c sig.h buffer.h strerr.h byte.h readwrite.h exit.h \
iopause.h taia.h tai.h uint64.h pathexec.h
	./compile fixcrio.c

fmt_ulong.o: \
compile fmt_ulong.c fmt.h
	./compile fmt_ulong.c

fork.h: \
choose compile load tryvfork.c fork.h1 fork.h2
	./choose cl tryvfork fork.h1 fork.h2 > fork.h

getln.o: \
compile getln.c byte.h getln.h buffer.h stralloc.h gen_alloc.h
	./compile getln.c

getln2.o: \
compile getln2.c byte.h getln.h buffer.h stralloc.h gen_alloc.h
	./compile getln2.c

hassgact.h: \
choose compile load trysgact.c hassgact.h1 hassgact.h2
	./choose cl trysgact hassgact.h1 hassgact.h2 > hassgact.h

hassgprm.h: \
choose compile load trysgprm.c hassgprm.h1 hassgprm.h2
	./choose cl trysgprm hassgprm.h1 hassgprm.h2 > hassgprm.h

hasshsgr.h: \
choose compile load tryshsgr.c hasshsgr.h1 hasshsgr.h2 chkshsgr \
warn-shsgr
	./chkshsgr || ( cat warn-shsgr; exit 1 )
	./choose clr tryshsgr hasshsgr.h1 hasshsgr.h2 > hasshsgr.h

haswaitp.h: \
choose compile load trywaitp.c haswaitp.h1 haswaitp.h2
	./choose cl trywaitp haswaitp.h1 haswaitp.h2 > haswaitp.h

hier.o: \
compile hier.c auto_home.h
	./compile hier.c

http@: \
warn-auto.sh http@.sh conf-home
	cat warn-auto.sh http@.sh \
	| sed s}HOME}"`head -1 conf-home`"}g \
	> http@
	chmod 755 http@

install: \
load install.o hier.o auto_home.o unix.a byte.a
	./load install hier.o auto_home.o unix.a byte.a 

install.o: \
compile install.c buffer.h strerr.h error.h open.h readwrite.h exit.h
	./compile install.c

instcheck: \
load instcheck.o hier.o auto_home.o unix.a byte.a
	./load instcheck hier.o auto_home.o unix.a byte.a 

instcheck.o: \
compile instcheck.c strerr.h error.h readwrite.h exit.h
	./compile instcheck.c

iopause.h: \
choose compile trypoll.c iopause.h1 iopause.h2
	./choose clr trypoll iopause.h1 iopause.h2 > iopause.h

iopause.o: \
compile iopause.c taia.h tai.h uint64.h select.h iopause.h taia.h
	./compile iopause.c

ip4_fmt.o: \
compile ip4_fmt.c fmt.h ip4.h
	./compile ip4_fmt.c

ip4_scan.o: \
compile ip4_scan.c scan.h ip4.h
	./compile ip4_scan.c

it: \
prog install instcheck

load: \
warn-auto.sh conf-ld
	( cat warn-auto.sh; \
	echo 'main="$$1"; shift'; \
	echo exec "`head -1 conf-ld`" \
	'-o "$$main" "$$main".o $${1+"$$@"}' \
	) > load
	chmod 755 load

makelib: \
warn-auto.sh systype
	( cat warn-auto.sh; \
	echo 'main="$$1"; shift'; \
	echo 'rm -f "$$main"'; \
	echo 'ar cr "$$main" $${1+"$$@"}'; \
	case "`cat systype`" in \
	sunos-5.*) ;; \
	unix_sv*) ;; \
	irix64-*) ;; \
	irix-*) ;; \
	dgux-*) ;; \
	hp-ux-*) ;; \
	sco*) ;; \
	*) echo 'ranlib "$$main"' ;; \
	esac \
	) > makelib
	chmod 755 makelib

mconnect: \
warn-auto.sh mconnect.sh conf-home
	cat warn-auto.sh mconnect.sh \
	| sed s}HOME}"`head -1 conf-home`"}g \
	> mconnect
	chmod 755 mconnect

mconnect-io: \
load mconnect-io.o unix.a byte.a
	./load mconnect-io unix.a byte.a 

mconnect-io.o: \
compile mconnect-io.c sig.h wait.h fork.h buffer.h strerr.h \
readwrite.h exit.h
	./compile mconnect-io.c

ndelay_off.o: \
compile ndelay_off.c ndelay.h
	./compile ndelay_off.c

ndelay_on.o: \
compile ndelay_on.c ndelay.h
	./compile ndelay_on.c

open_read.o: \
compile open_read.c open.h
	./compile open_read.c

open_trunc.o: \
compile open_trunc.c open.h
	./compile open_trunc.c

open_write.o: \
compile open_write.c open.h
	./compile open_write.c

openreadclose.o: \
compile openreadclose.c error.h open.h readclose.h stralloc.h \
gen_alloc.h openreadclose.h stralloc.h
	./compile openreadclose.c

pathexec_env.o: \
compile pathexec_env.c stralloc.h gen_alloc.h alloc.h str.h byte.h \
env.h pathexec.h
	./compile pathexec_env.c

pathexec_run.o: \
compile pathexec_run.c error.h stralloc.h gen_alloc.h str.h env.h \
pathexec.h
	./compile pathexec_run.c

prog: \
tcpserver tcprules tcprulescheck argv0 recordio tcpclient who@ date@ \
finger@ http@ tcpcat mconnect mconnect-io addcr delcr fixcrio \
rblsmtpd rts

prot.o: \
compile prot.c hasshsgr.h prot.h
	./compile prot.c

rblsmtpd: \
load rblsmtpd.o commands.o dns.a time.a unix.a byte.a socket.lib
	./load rblsmtpd commands.o dns.a time.a unix.a byte.a  \
	`cat socket.lib`

rblsmtpd.o: \
compile rblsmtpd.c byte.h str.h scan.h fmt.h env.h exit.h sig.h \
buffer.h readwrite.h sgetopt.h subgetopt.h strerr.h stralloc.h \
gen_alloc.h commands.h pathexec.h dns.h stralloc.h iopause.h taia.h \
tai.h uint64.h taia.h
	./compile rblsmtpd.c

readclose.o: \
compile readclose.c readwrite.h error.h readclose.h stralloc.h \
gen_alloc.h
	./compile readclose.c

recordio: \
load recordio.o time.a unix.a byte.a
	./load recordio time.a unix.a byte.a 

recordio.o: \
compile recordio.c sig.h buffer.h strerr.h str.h byte.h readwrite.h \
exit.h fmt.h iopause.h taia.h tai.h uint64.h pathexec.h
	./compile recordio.c

remoteinfo.o: \
compile remoteinfo.c fmt.h buffer.h socket.h uint16.h error.h \
iopause.h taia.h tai.h uint64.h timeoutconn.h uint16.h remoteinfo.h \
stralloc.h gen_alloc.h uint16.h
	./compile remoteinfo.c

rts: \
warn-auto.sh rts.sh conf-home
	cat warn-auto.sh rts.sh \
	| sed s}HOME}"`head -1 conf-home`"}g \
	> rts
	chmod 755 rts

rules.o: \
compile rules.c alloc.h stralloc.h gen_alloc.h open.h cdb.h uint32.h \
rules.h stralloc.h
	./compile rules.c

scan_ulong.o: \
compile scan_ulong.c scan.h
	./compile scan_ulong.c

seek_set.o: \
compile seek_set.c seek.h
	./compile seek_set.c

select.h: \
choose compile trysysel.c select.h1 select.h2
	./choose c trysysel select.h1 select.h2 > select.h

setup: \
it install
	./install

sgetopt.o: \
compile sgetopt.c buffer.h sgetopt.h subgetopt.h subgetopt.h
	./compile sgetopt.c

sig.o: \
compile sig.c sig.h
	./compile sig.c

sig_block.o: \
compile sig_block.c sig.h hassgprm.h
	./compile sig_block.c

sig_catch.o: \
compile sig_catch.c sig.h hassgact.h
	./compile sig_catch.c

sig_pause.o: \
compile sig_pause.c sig.h hassgprm.h
	./compile sig_pause.c

socket.lib: \
trylsock.c compile load
	( ( ./compile trylsock.c && \
	./load trylsock -lsocket -lnsl ) >/dev/null 2>&1 \
	&& echo -lsocket -lnsl || exit 0 ) > socket.lib
	rm -f trylsock.o trylsock

socket_accept.o: \
compile socket_accept.c byte.h socket.h uint16.h
	./compile socket_accept.c

socket_bind.o: \
compile socket_bind.c byte.h socket.h uint16.h
	./compile socket_bind.c

socket_conn.o: \
compile socket_conn.c readwrite.h byte.h socket.h uint16.h
	./compile socket_conn.c

socket_delay.o: \
compile socket_delay.c socket.h uint16.h
	./compile socket_delay.c

socket_listen.o: \
compile socket_listen.c socket.h uint16.h
	./compile socket_listen.c

socket_local.o: \
compile socket_local.c byte.h socket.h uint16.h
	./compile socket_local.c

socket_opts.o: \
compile socket_opts.c socket.h uint16.h
	./compile socket_opts.c

socket_remote.o: \
compile socket_remote.c byte.h socket.h uint16.h
	./compile socket_remote.c

socket_tcp.o: \
compile socket_tcp.c ndelay.h socket.h uint16.h
	./compile socket_tcp.c

socket_udp.o: \
compile socket_udp.c ndelay.h socket.h uint16.h
	./compile socket_udp.c

str_chr.o: \
compile str_chr.c str.h
	./compile str_chr.c

str_diff.o: \
compile str_diff.c str.h
	./compile str_diff.c

str_len.o: \
compile str_len.c str.h
	./compile str_len.c

str_start.o: \
compile str_start.c str.h
	./compile str_start.c

stralloc_cat.o: \
compile stralloc_cat.c byte.h stralloc.h gen_alloc.h
	./compile stralloc_cat.c

stralloc_catb.o: \
compile stralloc_catb.c stralloc.h gen_alloc.h byte.h
	./compile stralloc_catb.c

stralloc_cats.o: \
compile stralloc_cats.c byte.h str.h stralloc.h gen_alloc.h
	./compile stralloc_cats.c

stralloc_copy.o: \
compile stralloc_copy.c byte.h stralloc.h gen_alloc.h
	./compile stralloc_copy.c

stralloc_eady.o: \
compile stralloc_eady.c alloc.h stralloc.h gen_alloc.h \
gen_allocdefs.h
	./compile stralloc_eady.c

stralloc_opyb.o: \
compile stralloc_opyb.c stralloc.h gen_alloc.h byte.h
	./compile stralloc_opyb.c

stralloc_opys.o: \
compile stralloc_opys.c byte.h str.h stralloc.h gen_alloc.h
	./compile stralloc_opys.c

stralloc_pend.o: \
compile stralloc_pend.c alloc.h stralloc.h gen_alloc.h \
gen_allocdefs.h
	./compile stralloc_pend.c

strerr_die.o: \
compile strerr_die.c buffer.h exit.h strerr.h
	./compile strerr_die.c

strerr_sys.o: \
compile strerr_sys.c error.h strerr.h
	./compile strerr_sys.c

subgetopt.o: \
compile subgetopt.c subgetopt.h
	./compile subgetopt.c

systype: \
find-systype.sh conf-cc conf-ld trycpp.c x86cpuid.c
	( cat warn-auto.sh; \
	echo CC=\'`head -1 conf-cc`\'; \
	echo LD=\'`head -1 conf-ld`\'; \
	cat find-systype.sh; \
	) | sh > systype

tai_pack.o: \
compile tai_pack.c tai.h uint64.h
	./compile tai_pack.c

taia_add.o: \
compile taia_add.c taia.h tai.h uint64.h
	./compile taia_add.c

taia_approx.o: \
compile taia_approx.c taia.h tai.h uint64.h
	./compile taia_approx.c

taia_frac.o: \
compile taia_frac.c taia.h tai.h uint64.h
	./compile taia_frac.c

taia_less.o: \
compile taia_less.c taia.h tai.h uint64.h
	./compile taia_less.c

taia_now.o: \
compile taia_now.c taia.h tai.h uint64.h
	./compile taia_now.c

taia_pack.o: \
compile taia_pack.c taia.h tai.h uint64.h
	./compile taia_pack.c

taia_sub.o: \
compile taia_sub.c taia.h tai.h uint64.h
	./compile taia_sub.c

taia_uint.o: \
compile taia_uint.c taia.h tai.h uint64.h
	./compile taia_uint.c

tcpcat: \
warn-auto.sh tcpcat.sh conf-home
	cat warn-auto.sh tcpcat.sh \
	| sed s}HOME}"`head -1 conf-home`"}g \
	> tcpcat
	chmod 755 tcpcat

tcpclient: \
load tcpclient.o remoteinfo.o timeoutconn.o dns.a time.a unix.a \
byte.a socket.lib
	./load tcpclient remoteinfo.o timeoutconn.o dns.a time.a \
	unix.a byte.a  `cat socket.lib`

tcpclient.o: \
compile tcpclient.c sig.h exit.h sgetopt.h subgetopt.h uint16.h fmt.h \
scan.h str.h ip4.h uint16.h socket.h uint16.h fd.h stralloc.h \
gen_alloc.h buffer.h error.h strerr.h pathexec.h timeoutconn.h \
uint16.h remoteinfo.h stralloc.h uint16.h dns.h stralloc.h iopause.h \
taia.h tai.h uint64.h taia.h
	./compile tcpclient.c

tcprules: \
load tcprules.o cdb.a unix.a byte.a
	./load tcprules cdb.a unix.a byte.a 

tcprules.o: \
compile tcprules.c strerr.h stralloc.h gen_alloc.h getln.h buffer.h \
stralloc.h buffer.h exit.h fmt.h byte.h cdb_make.h buffer.h uint32.h
	./compile tcprules.c

tcprulescheck: \
load tcprulescheck.o rules.o cdb.a unix.a byte.a
	./load tcprulescheck rules.o cdb.a unix.a byte.a 

tcprulescheck.o: \
compile tcprulescheck.c byte.h buffer.h strerr.h env.h rules.h \
stralloc.h gen_alloc.h
	./compile tcprulescheck.c

tcpserver: \
load tcpserver.o rules.o remoteinfo.o timeoutconn.o cdb.a dns.a \
time.a unix.a byte.a socket.lib
	./load tcpserver rules.o remoteinfo.o timeoutconn.o cdb.a \
	dns.a time.a unix.a byte.a  `cat socket.lib`

tcpserver.o: \
compile tcpserver.c uint16.h str.h byte.h fmt.h scan.h ip4.h fd.h \
exit.h env.h prot.h open.h wait.h readwrite.h stralloc.h gen_alloc.h \
alloc.h buffer.h error.h strerr.h sgetopt.h subgetopt.h pathexec.h \
socket.h uint16.h ndelay.h remoteinfo.h stralloc.h uint16.h rules.h \
stralloc.h sig.h dns.h stralloc.h iopause.h taia.h tai.h uint64.h \
taia.h
	./compile tcpserver.c

time.a: \
makelib iopause.o tai_pack.o taia_add.o taia_approx.o taia_frac.o \
taia_less.o taia_now.o taia_pack.o taia_sub.o taia_uint.o
	./makelib time.a iopause.o tai_pack.o taia_add.o \
	taia_approx.o taia_frac.o taia_less.o taia_now.o \
	taia_pack.o taia_sub.o taia_uint.o

timeoutconn.o: \
compile timeoutconn.c ndelay.h socket.h uint16.h iopause.h taia.h \
tai.h uint64.h error.h timeoutconn.h uint16.h
	./compile timeoutconn.c

uint16_pack.o: \
compile uint16_pack.c uint16.h
	./compile uint16_pack.c

uint16_unpack.o: \
compile uint16_unpack.c uint16.h
	./compile uint16_unpack.c

uint32_pack.o: \
compile uint32_pack.c uint32.h
	./compile uint32_pack.c

uint32_unpack.o: \
compile uint32_unpack.c uint32.h
	./compile uint32_unpack.c

unix.a: \
makelib alloc.o alloc_re.o buffer.o buffer_0.o buffer_1.o buffer_2.o \
buffer_copy.o buffer_get.o buffer_put.o env.o error.o error_str.o \
fd_copy.o fd_move.o getln.o getln2.o ndelay_off.o ndelay_on.o \
open_read.o open_trunc.o open_write.o openreadclose.o pathexec_env.o \
pathexec_run.o prot.o readclose.o seek_set.o sgetopt.o sig.o \
sig_block.o sig_catch.o sig_pause.o socket_accept.o socket_bind.o \
socket_conn.o socket_delay.o socket_listen.o socket_local.o \
socket_opts.o socket_remote.o socket_tcp.o socket_udp.o \
stralloc_cat.o stralloc_catb.o stralloc_cats.o stralloc_copy.o \
stralloc_eady.o stralloc_opyb.o stralloc_opys.o stralloc_pend.o \
strerr_die.o strerr_sys.o subgetopt.o wait_nohang.o wait_pid.o
	./makelib unix.a alloc.o alloc_re.o buffer.o buffer_0.o \
	buffer_1.o buffer_2.o buffer_copy.o buffer_get.o \
	buffer_put.o env.o error.o error_str.o fd_copy.o fd_move.o \
	getln.o getln2.o ndelay_off.o ndelay_on.o open_read.o \
	open_trunc.o open_write.o openreadclose.o pathexec_env.o \
	pathexec_run.o prot.o readclose.o seek_set.o sgetopt.o \
	sig.o sig_block.o sig_catch.o sig_pause.o socket_accept.o \
	socket_bind.o socket_conn.o socket_delay.o socket_listen.o \
	socket_local.o socket_opts.o socket_remote.o socket_tcp.o \
	socket_udp.o stralloc_cat.o stralloc_catb.o stralloc_cats.o \
	stralloc_copy.o stralloc_eady.o stralloc_opyb.o \
	stralloc_opys.o stralloc_pend.o strerr_die.o strerr_sys.o \
	subgetopt.o wait_nohang.o wait_pid.o

wait_nohang.o: \
compile wait_nohang.c haswaitp.h
	./compile wait_nohang.c

wait_pid.o: \
compile wait_pid.c error.h haswaitp.h
	./compile wait_pid.c

who@: \
warn-auto.sh who@.sh conf-home
	cat warn-auto.sh who@.sh \
	| sed s}HOME}"`head -1 conf-home`"}g \
	> who@
	chmod 755 who@
