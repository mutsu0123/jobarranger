SUBDIRS = \
    agent \
    common \
    simple \
    linux \
    unknown

DIST_SUBDIRS = $(SUBDIRS)

# Create combined convenience archives after sub-libs are built.
# jobarg executables reference both the wrappers and the sub-libs directly;
# the wrappers aggregate common + simple + platform objects for convenience.
all-local: libzbxagentsysinfo.a libzbxserversysinfo.a

ARCH_LIBSPECSYSINFO = $(ARCH)/libspecsysinfo.a

libzbxagentsysinfo.a: agent/libagentsysinfo.a common/libcommonsysinfo.a simple/libsimplesysinfo.a $(ARCH_LIBSPECSYSINFO)
	rm -f $@
	$(AR) cru $@ \
	    $$($(AR) t agent/libagentsysinfo.a | sed 's|^|agent/|') \
	    $$($(AR) t common/libcommonsysinfo.a | sed 's|^|common/|') \
	    $$($(AR) t simple/libsimplesysinfo.a | sed 's|^|simple/|') \
	    $$($(AR) t $(ARCH_LIBSPECSYSINFO) | sed "s|^|$(ARCH)/|")
	$(RANLIB) $@

libzbxserversysinfo.a: common/libcommonsysinfo.a simple/libsimplesysinfo.a $(ARCH_LIBSPECSYSINFO)
	rm -f $@
	$(AR) cru $@ \
	    $$($(AR) t common/libcommonsysinfo.a | sed 's|^|common/|') \
	    $$($(AR) t simple/libsimplesysinfo.a | sed 's|^|simple/|') \
	    $$($(AR) t $(ARCH_LIBSPECSYSINFO) | sed "s|^|$(ARCH)/|")
	$(RANLIB) $@

clean-local:
	rm -f libzbxagentsysinfo.a libzbxserversysinfo.a
