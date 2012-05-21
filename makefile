CROSS_TOOL=

SRCS=$(wildcard *.c)

LIBNAME=libhttpd

VER_MAJ=0
VER_MIN=0
VER_REV=1

CFLAGS=

#--------#

LIBDIR=lib
BUILDDIR=.build

LIB_VER=$(VER_MAJ).$(VER_MIN).$(VER_REV)
OBJS=$(addprefix $(BUILDDIR)/,$(patsubst %.c,%.o,$(SRCS)))

GCC=$(CROSS_TOOL)gcc

#--------#

all: $(LIBDIR)/$(LIBNAME).so

$(LIBDIR)/$(LIBNAME).so: .$(LIBDIR).dir $(LIBDIR)/$(LIBNAME).so.$(LIB_VER)
	ln -s `basename $(filter %.so.$(LIB_VER),$^)` $@

$(LIBDIR)/$(LIBNAME).so.$(LIB_VER): .$(LIBDIR).dir $(OBJS)
	$(GCC) --shared -Wl,-soname,$(LIBNAME).so.$(LIB_VER) $(CLINKS) $(filter %.o,$^) -o $@

#--------#

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

#--------#

$(OBJS): $(BUILDDIR)/%.o: .$(BUILDDIR).dir %.c $(BUILDDIR)/%.d
	$(GCC) $(CFLAGS) $*.c -c -o $@

$(patsubst %.o,%.d,$(OBJS)): $(BUILDDIR)/%.d: .$(BUILDDIR).dir %.c
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
