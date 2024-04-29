FROM ubuntu:20.04
LABEL Description="NVIDIA Device Monitoring and Eventing Development Environment"
LABEL Version="0.1"
LABEL Maintainer="Kun Zhao(kuzhao@nvidia.com)"
LABEL ImageName="bldenv:module"
ARG execdir=/usr/bin
ARG scrpt=setup_bldenv
ADD scripts/$scrpt $execdir
SHELL ["/bin/bash", "-c"]
RUN chmod +x $execdir/$scrpt
RUN $execdir/$scrpt
