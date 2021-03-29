FROM ubi8
RUN dnf install -y gcc make git
RUN git clone https://github.com/c3d/workload && cd workload && make
ENTRYPOINT workload/workload
CMD ["50", "200"]
