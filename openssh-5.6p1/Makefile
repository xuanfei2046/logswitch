# $Id: Makefile.in,v 1.310 2010/05/12 06:51:39 dtucker Exp $

# uncomment if you run a non bourne compatable shell. Ie. csh
#SHELL = /bin/sh

AUTORECONF=autoreconf

prefix=/usr/local
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
sbindir=${exec_prefix}/sbin
libexecdir=${exec_prefix}/libexec
datadir=${datarootdir}
datarootdir=${prefix}/share
mandir=${datarootdir}/man
mansubdir=man
sysconfdir=${prefix}/etc
piddir=/var/run
srcdir=.
top_srcdir=.

DESTDIR=

SSH_PROGRAM=${exec_prefix}/bin/cssh
PRIVSEP_PATH=/var/empty
SSH_PRIVSEP_USER=sshd
STRIP_OPT=-s

PATHS= -DSSHDIR=\"$(sysconfdir)\" \
	-D_PATH_SSH_PROGRAM=\"$(SSH_PROGRAM)\" \
	-D_PATH_PRIVSEP_CHROOT_DIR=\"$(PRIVSEP_PATH)\" 

CC=gcc
LD=gcc
CFLAGS=-g -O2 -Wall -Wpointer-arith -Wuninitialized -Wsign-compare -Wno-pointer-sign -Wformat-security -fno-strict-aliasing -fno-builtin-memset -fstack-protector-all 
CPPFLAGS=-I. -I$(srcdir)  $(PATHS) -DHAVE_CONFIG_H
LIBS=-lresolv -lcrypto -ldl -lutil -lz -lnsl  -lcrypt
SSHDLIBS=
LIBEDIT=
AR=/usr/bin/ar
AWK=mawk
RANLIB=ranlib
INSTALL=/usr/bin/install -c
PERL=/usr/bin/perl
SED=/bin/sed
ENT=
XAUTH_PATH=/usr/bin/xauth
LDFLAGS=-L. -Lopenbsd-compat/  -fstack-protector-all
EXEEXT=

INSTALL_SSH_PRNG_CMDS=
INSTALL_SSH_RAND_HELPER=

TARGETS=cssh$(EXEEXT)

LIBSSH_OBJS=acss.o authfd.o authfile.o bufaux.o bufbn.o buffer.o \
	canohost.o regex.o clog.o channels.o cipher.o cipher-acss.o cipher-aes.o \
	cipher-bf1.o cipher-ctr.o cipher-3des1.o cleanup.o \
	compat.o compress.o crc32.o deattack.o fatal.o hostfile.o \
	log.o match.o md-sha256.o moduli.o nchan.o packet.o \
	readpass.o rsa.o ttymodes.o xmalloc.o addrmatch.o \
	atomicio.o key.o dispatch.o kex.o mac.o uidswap.o uuencode.o misc.o \
	monitor_fdpass.o rijndael.o ssh-dss.o ssh-rsa.o dh.o kexdh.o \
	kexgex.o kexdhc.o kexgexc.o msg.o progressmeter.o dns.o \
	entropy.o gss-genr.o umac.o jpake.o schnorr.o \
	ssh-pkcs11.o

SSHOBJS=cssh.o readconf.o clientloop.o sshtty.o \
	sshconnect.o sshconnect1.o sshconnect2.o mux.o \
	roaming_common.o roaming_client.o


MANPAGES	= moduli.5.out 
MANPAGES_IN	= moduli.5
MANTYPE		= doc

CONFIGFILES=cssh_config.out moduli.out cssh_command_allow.out cssh_command_deny.out
CONFIGFILES_IN=cssh_config cssh_command_deny cssh_command_allow moduli

PATHSUBS	= \
	-e 's|/etc/ssh/cssh_config|$(sysconfdir)/cssh_config|g' \
	-e 's|/usr/bin:/bin:/usr/sbin:/sbin|/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin|g'

FIXPATHSCMD	= $(SED) $(PATHSUBS)

all: $(CONFIGFILES) ssh_prng_cmds.out $(MANPAGES) $(TARGETS)

$(LIBSSH_OBJS): Makefile.in config.h
$(SSHOBJS): Makefile.in config.h

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

LIBCOMPAT=openbsd-compat/libopenbsd-compat.a
$(LIBCOMPAT): always
	(cd openbsd-compat && $(MAKE))
always:

libcssh.a: $(LIBSSH_OBJS)
	$(AR) rv $@ $(LIBSSH_OBJS)
	$(RANLIB) $@

cssh$(EXEEXT): $(LIBCOMPAT) libcssh.a $(SSHOBJS)
	$(LD) -o $@ $(SSHOBJS) $(LDFLAGS) -lcssh -lopenbsd-compat $(LIBS) -lpcre


$(MANPAGES): $(MANPAGES_IN)
	if test "$(MANTYPE)" = "cat"; then \
		manpage=$(srcdir)/`echo $@ | sed 's/\.[1-9]\.out$$/\.0/'`; \
	else \
		manpage=$(srcdir)/`echo $@ | sed 's/\.out$$//'`; \
	fi; \
	if test "$(MANTYPE)" = "man"; then \
		$(FIXPATHSCMD) $${manpage} | $(AWK) -f $(srcdir)/mdoc2man.awk > $@; \
	else \
		$(FIXPATHSCMD) $${manpage} > $@; \
	fi

$(CONFIGFILES): $(CONFIGFILES_IN)
	conffile=`echo $@ | sed 's/.out$$//'`; \
	$(FIXPATHSCMD) $(srcdir)/$${conffile} > $@

ssh_prng_cmds.out:	ssh_prng_cmds
	if test ! -z "$(INSTALL_SSH_PRNG_CMDS)"; then \
		$(PERL) $(srcdir)/fixprogs ssh_prng_cmds $(ENT); \
	fi

# fake rule to stop make trying to compile moduli.o into a binary "moduli.o"
moduli:
	echo

clean:	regressclean
	rm -f *.o *.a $(TARGETS) logintest config.cache config.log
	rm -f *.out core survey
	(cd openbsd-compat && $(MAKE) clean)

distclean:	regressclean
	rm -f *.o *.a $(TARGETS) logintest config.cache config.log
	rm -f Makefile buildpkg.sh config.h config.status ssh_prng_cmds
	rm -f survey.sh openbsd-compat/regress/Makefile *~ 
	rm -rf autom4te.cache
	(cd openbsd-compat && $(MAKE) distclean)
	if test -d pkg ; then \
		rm -fr pkg ; \
	fi

veryclean: distclean
	rm -f configure config.h.in *.0

mrproper: veryclean

realclean: veryclean

catman-do:
	@for f in $(MANPAGES_IN) ; do \
		base=`echo $$f | sed 's/\..*$$//'` ; \
		echo "$$f -> $$base.0" ; \
		nroff -mandoc $$f | cat -v | sed -e 's/.\^H//g' \
			>$$base.0 ; \
	done

distprep: catman-do
	$(AUTORECONF)
	-rm -rf autom4te.cache

install: $(CONFIGFILES) $(MANPAGES) $(TARGETS) install-files install-sysconf 
install-nokeys: $(CONFIGFILES) ssh_prng_cmds.out $(MANPAGES) $(TARGETS) install-files install-sysconf
install-nosysconf: $(CONFIGFILES) ssh_prng_cmds.out $(MANPAGES) $(TARGETS) install-files

install-files:
	$(srcdir)/mkinstalldirs $(DESTDIR)$(bindir)
	$(srcdir)/mkinstalldirs $(DESTDIR)$(sbindir)
	$(srcdir)/mkinstalldirs $(DESTDIR)$(mandir)
	$(srcdir)/mkinstalldirs $(DESTDIR)$(mandir)/$(mansubdir)1
	$(srcdir)/mkinstalldirs $(DESTDIR)$(mandir)/$(mansubdir)5
	$(srcdir)/mkinstalldirs $(DESTDIR)$(mandir)/$(mansubdir)8
	$(srcdir)/mkinstalldirs $(DESTDIR)$(libexecdir)
	(umask 022 ; $(srcdir)/mkinstalldirs $(DESTDIR)$(PRIVSEP_PATH))
	$(INSTALL) -m 0755 $(STRIP_OPT) cssh$(EXEEXT) $(DESTDIR)$(bindir)/cssh$(EXEEXT)

install-sysconf:
	if [ ! -d $(DESTDIR)$(sysconfdir) ]; then \
		$(srcdir)/mkinstalldirs $(DESTDIR)$(sysconfdir); \
	fi
	@if [ ! -f $(DESTDIR)$(sysconfdir)/cssh_config ]; then \
		$(INSTALL) -m 644 cssh_config.out $(DESTDIR)$(sysconfdir)/cssh_config; \
	else \
		echo "$(DESTDIR)$(sysconfdir)/cssh_config already exists, install will not overwrite"; \
	fi
	@if [ ! -f $(DESTDIR)$(sysconfdir)/cssh_command_allow ]; then \
		$(INSTALL) -m 644 cssh_command_allow.out $(DESTDIR)$(sysconfdir)/cssh_command_allow; \
	else \
		echo "$(DESTDIR)$(sysconfdir)/cssh_command_allow already exists, install will not overwrite"; \
	fi
	@if [ ! -f $(DESTDIR)$(sysconfdir)/cssh_command_deny ]; then \
		$(INSTALL) -m 644 cssh_command_deny.out $(DESTDIR)$(sysconfdir)/cssh_command_deny;\
	else \
		echo "$(DESTDIR)$(sysconfdir)/cssh_command_deny already exists, install will not overwrite"; \
	fi

host-key: ssh-keygen$(EXEEXT)
	@if [ -z "$(DESTDIR)" ] ; then \
		if [ -f "$(DESTDIR)$(sysconfdir)/ssh_host_key" ] ; then \
			echo "$(DESTDIR)$(sysconfdir)/ssh_host_key already exists, skipping." ; \
		else \
			./ssh-keygen -t rsa1 -f $(DESTDIR)$(sysconfdir)/ssh_host_key -N "" ; \
		fi ; \
		if [ -f $(DESTDIR)$(sysconfdir)/ssh_host_dsa_key ] ; then \
			echo "$(DESTDIR)$(sysconfdir)/ssh_host_dsa_key already exists, skipping." ; \
		else \
			./ssh-keygen -t dsa -f $(DESTDIR)$(sysconfdir)/ssh_host_dsa_key -N "" ; \
		fi ; \
		if [ -f $(DESTDIR)$(sysconfdir)/ssh_host_rsa_key ] ; then \
			echo "$(DESTDIR)$(sysconfdir)/ssh_host_rsa_key already exists, skipping." ; \
		else \
			./ssh-keygen -t rsa -f $(DESTDIR)$(sysconfdir)/ssh_host_rsa_key -N "" ; \
		fi ; \
	fi ;

host-key-force: ssh-keygen$(EXEEXT)
	./ssh-keygen -t rsa1 -f $(DESTDIR)$(sysconfdir)/ssh_host_key -N ""
	./ssh-keygen -t dsa -f $(DESTDIR)$(sysconfdir)/ssh_host_dsa_key -N ""
	./ssh-keygen -t rsa -f $(DESTDIR)$(sysconfdir)/ssh_host_rsa_key -N ""

uninstallall:	uninstall
	-rm -f $(DESTDIR)$(sysconfdir)/cssh_config
	-rm -f $(DESTDIR)$(sysconfdir)/cssh_command_allow
	-rm -f $(DESTDIR)$(sysconfdir)/cssh_command_deny

uninstall:
	-rm -f $(DESTDIR)$(bindir)/cssh$(EXEEXT)

tests interop-tests:	$(TARGETS)
	BUILDDIR=`pwd`; \
	[ -d `pwd`/regress ]  ||  mkdir -p `pwd`/regress; \
	[ -f `pwd`/regress/Makefile ]  || \
	    ln -s `cd $(srcdir) && pwd`/regress/Makefile `pwd`/regress/Makefile ; \
	TEST_SHELL="sh"; \
	TEST_SSH_SSH="$${BUILDDIR}/ssh"; \
	TEST_SSH_SSHAGENT="$${BUILDDIR}/ssh-agent"; \
	TEST_SSH_SSHADD="$${BUILDDIR}/ssh-add"; \
	TEST_SSH_SSHKEYGEN="$${BUILDDIR}/ssh-keygen"; \
	TEST_SSH_SSHPKCS11HELPER="$${BUILDDIR}/ssh-pkcs11-helper"; \
	TEST_SSH_SSHKEYSCAN="$${BUILDDIR}/ssh-keyscan"; \
	TEST_SSH_SFTP="$${BUILDDIR}/sftp"; \
	TEST_SSH_SFTPSERVER="$${BUILDDIR}/sftp-server"; \
	TEST_SSH_PLINK="plink"; \
	TEST_SSH_PUTTYGEN="puttygen"; \
	TEST_SSH_CONCH="conch"; \
	TEST_SSH_IPV6="yes" ; \
	cd $(srcdir)/regress || exit $$?; \
	$(MAKE) \
		.OBJDIR="$${BUILDDIR}/regress" \
		.CURDIR="`pwd`" \
		BUILDDIR="$${BUILDDIR}" \
		OBJ="$${BUILDDIR}/regress/" \
		PATH="$${BUILDDIR}:$${PATH}" \
		TEST_SHELL="$${TEST_SHELL}" \
		TEST_SSH_SSH="$${TEST_SSH_SSH}" \
		TEST_SSH_SSHAGENT="$${TEST_SSH_SSHAGENT}" \
		TEST_SSH_SSHADD="$${TEST_SSH_SSHADD}" \
		TEST_SSH_SSHKEYGEN="$${TEST_SSH_SSHKEYGEN}" \
		TEST_SSH_SSHPKCS11HELPER="$${TEST_SSH_SSHPKCS11HELPER}" \
		TEST_SSH_SSHKEYSCAN="$${TEST_SSH_SSHKEYSCAN}" \
		TEST_SSH_SFTP="$${TEST_SSH_SFTP}" \
		TEST_SSH_SFTPSERVER="$${TEST_SSH_SFTPSERVER}" \
		TEST_SSH_PLINK="$${TEST_SSH_PLINK}" \
		TEST_SSH_PUTTYGEN="$${TEST_SSH_PUTTYGEN}" \
		TEST_SSH_CONCH="$${TEST_SSH_CONCH}" \
		TEST_SSH_IPV6="yes" \
		EXEEXT="$(EXEEXT)" \
		$@ && echo all tests passed

compat-tests: $(LIBCOMPAT)
	(cd openbsd-compat/regress && $(MAKE))

regressclean:
	if [ -f regress/Makefile ] && [ -r regress/Makefile ]; then \
		(cd regress && $(MAKE) clean) \
	fi

survey: survey.sh ssh
	@$(SHELL) ./survey.sh > survey
	@echo 'The survey results have been placed in the file "survey" in the'
	@echo 'current directory.  Please review the file then send with'
	@echo '"make send-survey".'

send-survey:	survey
	mail portable-survey@mindrot.org <survey

package: $(CONFIGFILES) ssh_prng_cmds.out $(MANPAGES) $(TARGETS)
	if [ "no" = yes ]; then \
		sh buildpkg.sh; \
	fi

