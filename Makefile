PRODUCTS=workload.exe
SOURCES=workload.c
MIQ=make-it-quick/
include $(MIQ)rules.mk

$(MIQ)rules.mk:
	git submodule update --init --recursive

podbuild:
	podman build --no-cache -t workload .
	podman tag workload c3d/workload
	podman push c3d/workload docker://quay.io/c3d/workload
