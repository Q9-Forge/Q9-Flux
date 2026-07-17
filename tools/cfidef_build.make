-b

SRC = Z:\\Volumes\\SSD1TB\\projects\\Q9\\tools\\cfidef_format.a
DESC = Z:\\Volumes\\SSD1TB\\projects\\MWOS\\OS9\\68030\\PORTS\\Q9\\RBF
OSDEFS = Z:\\Volumes\\SSD1TB\\projects\\MWOS\\OS9\\SRC\\DEFS
MACDIR = Z:\\Volumes\\SSD1TB\\projects\\MWOS\\OS9\\SRC\\MACROS
SYSRELS = Z:\\Volumes\\SSD1TB\\projects\\MWOS\\OS9\\68000\\LIB

cfidef:
	r68 -u=$(DESC) -u=$(OSDEFS) -u=$(MACDIR) -u=. $(SRC) -O=Z:\\Volumes\\SSD1TB\\projects\\Q9\\build\\cfidef.r
	l68 -l=$(SYSRELS)\\sys.l -l=$(SYSRELS)\\drvs1.l -gu=0.0 Z:\\Volumes\\SSD1TB\\projects\\Q9\\build\\cfidef.r -O=Z:\\Volumes\\SSD1TB\\projects\\Q9\\local_images\\cfidef
