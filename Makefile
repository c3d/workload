PRODUCTS=workload.exe
SOURCES=workload.c
MIQ=make-it-quick/
include $(MIQ)rules.mk

$(MIQ)rules.mk:
	git submodule update --init --recursive
