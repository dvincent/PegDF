### Build flags for all targets
#
CF_ALL          = -g -Wall -I . -I pdfdoc 
LF_ALL          = -lz
LL_ALL          =

ifeq	"$(YYDEBUG)" "y"
	CF_ALL += -DYY_DEBUG
endif
ifeq	"$(DEBUG)" "y"
	CF_ALL +=  -DDEBUG
endif

### Build tools
#
CC              = gcc
COMP            = $(CC) $(CF_ALL) $(CF_TGT) -o $@ -c $<
LINK            = $(CC) $(LF_ALL) $(LF_TGT) -o $@ $^ $(LL_TGT) $(LL_ALL)
COMPLINK        = $(CC) $(CF_ALL) $(CF_TGT) $(LF_ALL) $(LF_TGT) -o $@ $< $(LL_TGT) $(LL_ALL)
ARCHIVE			= $(AR) $(ARFLAGS) $@ $^
#
vpath %.h . pdfdoc
# GLOBALS TARGETS
TGT_LIB	=

APP = readpdf
CLEAN = $(APP) pdf.c pdf_parse.o readpdf.o tst.o dict.o bplustree.o pdfindex.o pdfmem.o substream.o

COMMON_HEADERS := *.h
#######

all :

# sub dirs
# General dir rules
include Rules.mk

.PHONY: all
all : $(APP)

.PHONY: libraries

libraries: $(TGT_LIB)

pdf.c  : pdf.peg
	peg -v -o $(@) $(<)

pdf_parse.c pdf_parse.o:	pdf.c

readpdf : pdf_parse.o readpdf.o tst.o dict.o bplustree.o pdfindex.o pdfmem.o substream.o $(TGT_LIB)

test	:	readpdf
	@./readpdf examples/simpledict.pdf
	@if [ "$$?" -eq 0 ] ;\
	then \
		echo "passed test"; \
	else \
		echo "failed test"; \
	fi
