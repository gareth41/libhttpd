CROSS_COMPILE=
SYS_ROOT?=

#--------#

SRCS=$(wildcard *.c)

LIBNAME=libhttpd
SYS_LIBDIR=$(SYS_ROOT)/usr/lib
SYS_INCDIR=$(SYS_ROOT)/usr/include

VER_MAJ=0
VER_MIN=0
VER_REV=1

LIBS=pthread

DEBUG=-g
CFLAGS=-Wall -c -fPIC $(DEBUG) $(addprefix -D,$(OPTIONS)) -fvisibility=hidden -Wstrict-prototypes -Wno-variadic-macros
CLINKS=-fPIC $(addprefix -l,$(LIBS)) $(DEBUG)

#--------#

LIBDIR=lib
BUILDDIR=.build

LIB_VER=$(VER_MAJ).$(VER_MIN).$(VER_REV)
OBJS=$(addprefix $(BUILDDIR)/,$(patsubst %.c,%.o,$(SRCS)))

GCC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar

#--------#

all: $(LIBDIR)/$(LIBNAME).so $(LIBDIR)/$(LIBNAME).a

new: clean
	$(MAKE) --no-print-directory all

clean:
	rm -rf $(OBJS) $(LIBDIR)/$(LIBNAME).so $(LIBDIR)/$(LIBNAME).so.$(LIB_VER)

mrproper:
	@for i in .*.dir; do \
		CMD="rm -rf $${i} $$(echo $${i} | sed -re 's#\.([^\.]+)\.dir#\1#')"; \
		echo $${CMD}; \
		$${CMD}; \
	done

install: $(SYS_LIBDIR)/$(LIBNAME).so $(SYS_LIBDIR)/$(LIBNAME).a $(SYS_INCDIR)/httpd.h

#--------#

$(SYS_LIBDIR)/$(LIBNAME).%: $(SYS_LIBDIR)/$(LIBNAME).%.$(LIB_VER)
	ln -fs $(shell basename $^) $@

.PRECIOUS: $(SYS_LIBDIR)/$(LIBNAME).%.$(LIB_VER)
$(SYS_LIBDIR)/$(LIBNAME).%.$(LIB_VER): $(LIBDIR)/$(LIBNAME).%.$(LIB_VER)
	install -g root -o root -DT -m 755 $^ $@

$(SYS_INCDIR)/httpd.h: httpd.h
	install -g root -o root -DT -m 644 $^ $@

#--------#

$(LIBDIR)/$(LIBNAME).so: .$(LIBDIR).dir $(LIBDIR)/$(LIBNAME).so.$(LIB_VER)
	@if [ ! -e $@ ]; then CMD="ln -sf `basename $(filter %.so.$(LIB_VER),$^)` $@"; echo $${CMD}; $${CMD}; fi

$(LIBDIR)/$(LIBNAME).so.$(LIB_VER): .$(LIBDIR).dir $(OBJS) makefile
	$(GCC) --shared -Wl,-soname,$(LIBNAME).so.$(LIB_VER) $(CLINKS) $(filter %.o,$^) -o $@

#--------#

$(LIBDIR)/$(LIBNAME).a: .$(LIBDIR).dir $(LIBDIR)/$(LIBNAME).a.$(LIB_VER)
	@if [ ! -e $@ ]; then CMD="ln -sf `basename $(filter %.a.$(LIB_VER),$^)` $@"; echo $${CMD}; $${CMD}; fi

$(LIBDIR)/$(LIBNAME).a.$(LIB_VER): .$(LIBDIR).dir $(OBJS) makefile
	$(AR) rcs $@ $(filter %.o,$^)

#--------#

$(OBJS): $(BUILDDIR)/%.o: .$(BUILDDIR).dir %.c $(BUILDDIR)/%.d makefile
	$(GCC) $(CFLAGS) $*.c -c -o $@

$(patsubst %.o,%.d,$(OBJS)): $(BUILDDIR)/%.d: .$(BUILDDIR).dir %.c makefile
	$(GCC) -MM -MT $(@:.d=.o) $(filter %.c,$^) -o $@

.%.dir:
	@if [ ! -e "$*" ]; then \
		echo "mkdir '$*'"; \
		mkdir "$*"; \
		touch ".$*.dir"; \
	elif [ ! -d "$*" ]; then \
		echo "Error: '$*' already exists, but is not a directory..."; \
		false; \
	fi;

#--------#

-include $(wildcard $(BUILDDIR)/*.d)
