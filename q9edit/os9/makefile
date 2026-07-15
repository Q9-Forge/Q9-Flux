# q9edit minimal OS-9/xcc build.
#
# Run through tools/build-os9.sh on macOS.  This makefile intentionally uses
# the official MWOS templates instead of duplicating compiler/linker flags.

MWOS        = Z:/Volumes/SSD1TB/projects/MWOS
TRGTS       = qeprobe
MAKENAME    = makefile

include $(MWOS)/MAKETMPL/make.com

SDIR        = .
IDIR        = RELS/k68k
RDIR        = RELS/k020
ODIR        = CMDS

BRELS       = $(CSTART) $(RDIR)/qeprobe.r
BIRELS      = $(IDIR)/qeprobe.i
DEFS        = $(MWOS_DEFS)
LIBS        = $(TERMLIBS) $(MWOS_CSLLIBS)
OPTMZ       =

COMPILE     = $(CC) $(CPARMS) $(OPTMZ) -efe=$(IDIR) $(DEFS) $(COPTS)
ILINK       = $(CC) $(CPARMS) $(OPTMZ) -k -eas=$(RDIR) $(DEFS) $(LIBS) $(COPTS)
LINK        = $(LINKER) $(LPARMS) $(OPTMZ) -tp=$(CPUC) -k -olS $(LIBS) $(LOPTS) $(COPT_GU0)

qeprobe: $(IDIR) $(RDIR) $(ODIR) $(ODIR)/qeprobe
	$(COMMENT)

$(ODIR)/qeprobe: $(BRELS)
	-$(DEL) $@
	$(LINK) $(BRELS) -fd=$@ -oln=qeprobe
	$(ATTRE0) $@

$(RDIR)/qeprobe.r: $(BIRELS)
	-$(DEL) $@
	$(ILINK) $(BIRELS) -fd=qeprobe.r

$(IDIR)/qeprobe.i: $(SDIR)/qeprobe.c makefile
	-$(DEL) $@
	$(COMPILE) $(SDIR)/qeprobe.c

$(IDIR) $(RDIR) $(ODIR):
	-$(MAKDIR) $@

clean: .
	-$(DEL) $(IDIR)/*.i
	-$(DEL) $(RDIR)/*.r
	-$(DEL) $(ODIR)/qeprobe
