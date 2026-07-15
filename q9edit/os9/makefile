# q9edit minimal OS-9/xcc build.
#
# Run through tools/build-os9.sh on macOS.  This makefile intentionally uses
# the official MWOS templates instead of duplicating compiler/linker flags.

MWOS        = Z:/Volumes/SSD1TB/projects/MWOS
TRGTS       = qeprobe qetermprobe
MAKENAME    = makefile

include $(MWOS)/MAKETMPL/make.com

SDIR        = .
IDIR        = RELS/k68k
RDIR        = RELS/k020
ODIR        = CMDS

BRELS       = $(CSTART) $(RDIR)/qeprobe.r
BIRELS      = $(IDIR)/qeprobe.i
TBRELS      = $(CSTART) $(RDIR)/qetermprobe.r
TBIRELS     = $(IDIR)/qetermprobe.i $(IDIR)/qe_platform_os9.i
DEFS        = $(MWOS_DEFS)
LIBS        = $(TERMLIBS) $(MWOS_CSLLIBS)
OPTMZ       =

COMPILE     = $(CC) $(CPARMS) $(OPTMZ) -efe=$(IDIR) $(DEFS) $(COPTS)
ILINK       = $(CC) $(CPARMS) $(OPTMZ) -k -eas=$(RDIR) $(DEFS) $(LIBS) $(COPTS)
LINK        = $(LINKER) $(LPARMS) $(OPTMZ) -tp=$(CPUC) -k -olS $(LIBS) $(LOPTS) $(COPT_GU0)

qeprobe: $(IDIR) $(RDIR) $(ODIR) $(ODIR)/qeprobe
	$(COMMENT)

qetermprobe: $(IDIR) $(RDIR) $(ODIR) $(ODIR)/qetermprobe
	$(COMMENT)

$(ODIR)/qeprobe: $(BRELS)
	-$(DEL) $@
	$(LINK) $(BRELS) -fd=$@ -oln=qeprobe
	$(ATTRE0) $@

$(RDIR)/qeprobe.r: $(BIRELS)
	-$(DEL) $@
	$(ILINK) $(BIRELS) -fd=qeprobe.r

$(ODIR)/qetermprobe: $(TBRELS)
	-$(DEL) $@
	$(LINK) $(TBRELS) -fd=$@ -oln=qetermprobe
	$(ATTRE0) $@

$(RDIR)/qetermprobe.r: $(TBIRELS)
	-$(DEL) $@
	$(ILINK) $(TBIRELS) -fd=qetermprobe.r

$(IDIR)/qeprobe.i: $(SDIR)/qeprobe.c makefile
	-$(DEL) $@
	$(COMPILE) $(SDIR)/qeprobe.c

$(IDIR)/qetermprobe.i: $(SDIR)/qetermprobe.c makefile
	-$(DEL) $@
	$(COMPILE) -v=../src/platform $(SDIR)/qetermprobe.c

$(IDIR)/qe_platform_os9.i: ../src/platform/qe_platform_os9.c ../src/platform/qe_platform.h makefile
	-$(DEL) $@
	$(COMPILE) -v=../src/platform ../src/platform/qe_platform_os9.c

$(IDIR) $(RDIR) $(ODIR):
	-$(MAKDIR) $@

clean: .
	-$(DEL) $(IDIR)/*.i
	-$(DEL) $(RDIR)/*.r
	-$(DEL) $(ODIR)/qeprobe
	-$(DEL) $(ODIR)/qetermprobe
