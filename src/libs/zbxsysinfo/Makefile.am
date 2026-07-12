SUBDIRS = \
    agent \
    common \
    simple \
    linux \
    unknown

DIST_SUBDIRS = $(SUBDIRS)

# sysinfo.c provides parse_command and get_result_value_by_type,
# referenced by zbxserver and zbxdbcache; compile it directly here.
# Force-include common/common.h so EXECUTE_USER_PARAMETER is declared
# (sysinfo.c uses it but does not include common/common.h itself).
noinst_LIBRARIES = libsysinfo.a
libsysinfo_a_SOURCES = sysinfo.c sysinfo.h
libsysinfo_a_CFLAGS = $(AM_CFLAGS) -include $(srcdir)/common/common.h

# Create combined convenience archives after sub-libs are built.
# jobarg executables reference both the wrappers and the sub-libs directly;
# the wrappers aggregate sysinfo.c + common + simple + platform objects.
all-local: libzbxagentsysinfo.a libzbxserversysinfo.a

ARCH_LIBSPECSYSINFO = $(ARCH)/libspecsysinfo.a

libzbxagentsysinfo.a: libsysinfo.a agent/libagentsysinfo.a common/libcommonsysinfo.a simple/libsimplesysinfo.a $(ARCH_LIBSPECSYSINFO)
	rm -f $@
	$(AR) cru $@ \
	    $$($(AR) t libsysinfo.a) \
	    $$($(AR) t agent/libagentsysinfo.a | sed 's|^|agent/|') \
	    $$($(AR) t common/libcommonsysinfo.a | sed 's|^|common/|') \
	    $$($(AR) t simple/libsimplesysinfo.a | sed 's|^|simple/|') \
	    $$($(AR) t $(ARCH_LIBSPECSYSINFO) | sed "s|^|$(ARCH)/|")
	$(RANLIB) $@

libzbxserversysinfo.a: libsysinfo.a common/libcommonsysinfo.a simple/libsimplesysinfo.a $(ARCH_LIBSPECSYSINFO)
	rm -f $@
	$(AR) cru $@ \
	    $$($(AR) t libsysinfo.a) \
	    $$($(AR) t common/libcommonsysinfo.a | sed 's|^|common/|') \
	    $$($(AR) t simple/libsimplesysinfo.a | sed 's|^|simple/|') \
	    $$($(AR) t $(ARCH_LIBSPECSYSINFO) | sed "s|^|$(ARCH)/|")
	$(RANLIB) $@

clean-local:
	rm -f libzbxagentsysinfo.a libzbxserversysinfo.a
