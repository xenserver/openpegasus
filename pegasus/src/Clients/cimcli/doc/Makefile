#//%LICENSE////////////////////////////////////////////////////////////////
#//
#// Licensed to The Open Group (TOG) under one or more contributor license
#// agreements.  Refer to the OpenPegasusNOTICE.txt file distributed with
#// this work for additional information regarding copyright ownership.
#// Each contributor licenses this file to you under the OpenPegasus Open
#// Source License; you may not use this file except in compliance with the
#// License.
#//
#// Permission is hereby granted, free of charge, to any person obtaining a
#// copy of this software and associated documentation files (the "Software"),
#// to deal in the Software without restriction, including without limitation
#// the rights to use, copy, modify, merge, publish, distribute, sublicense,
#// and/or sell copies of the Software, and to permit persons to whom the
#// Software is furnished to do so, subject to the following conditions:
#//
#// The above copyright notice and this permission notice shall be included
#// in all copies or substantial portions of the Software.
#//
#// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#//
#//////////////////////////////////////////////////////////////////////////
# Makefile to create documentation from
# nroff formatted for manpages
# This makefile is not connected to the
# standard makefile.  It is strictly a
# means to create the ascii and html
# manpages from an NROFF source.  Note
# that it requires utilities probably only
# found in Linux/Unix environments including.
# nroff and man2html
#
ROOT = ../../../..

include $(ROOT)/mak/config.mak

GROFFOPTIONS=man
# old was man
DOCUMENT=cimcli
SOURCE=$(DOCUMENT).nroff

html:
	man2html <$(SOURCE) >$(DOCUMENT).html


manpage:
	nroff -$(GROFFOPTIONS) $(SOURCE) >$(DOCUMENT)

testnroff:
	nroff -$(GROFFOPTIONS) $(SOURCE) | less

testgroff:
	groff -$(GROFFOPTIONS) -X $(SOURCE)

text:
	nroff -$(GROFFOPTIONS) $(SOURCE) | col -b > $(DOCUMENT).txt

textgroff:
	groff -$(GROFFOPTIONS) $(SOURCE) | col -b > $(DOCUMENT).txt

all: manpage html text
