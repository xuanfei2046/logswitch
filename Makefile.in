SUDOSH=sudosh-1.6.3
OMNITTY=omnitty-0.3.0
OPENSSH=openssh-5.6p1

#DEPENPATH= sudosh-1.6.3 omnitty-0.3.0

subsystem:
	cd $(SUDOSH) && $(MAKE) $(MFLAGS)
	cd $(OMNITTY) && $(MAKE) $(MFLAGS)
	cd $(OPENSSH) && $(MAKE) $(MFLAGS)

install:
	cd $(SUDOSH) && $(MAKE) install
	cd $(OMNITTY) && $(MAKE) install
	cd $(OPENSSH) && $(MAKE) install
uninstall:
	cd $(SUDOSH) && $(MAKE) uninstall
	cd $(OMNITTY) && $(MAKE) uninstall
	cd $(OPENSSH) && $(MAKE) uninstall
clean:
	cd $(SUDOSH) && $(MAKE) clean
	cd $(OMNITTY) && $(MAKE) clean
	cd $(OPENSSH) && $(MAKE) clean
