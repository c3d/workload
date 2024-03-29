FROM ubi9
RUN dnf install -y gcc make git procps-ng
RUN git clone https://github.com/c3d/workload && cd workload && make
ENV CPU=50 MEMORY=200 INCREMENT=128
CMD ["bash", "-c", "workload/workload $CPU $MEMORY $INCREMENT" ]
