#!/bin/bash -xe

# This script is run by OpenStack CI jobs for tox based jobs during
# the pre-run phase.

# Install liberasurecode-devel for CentOS from RDO repository.

function is_rhel9 {
    [ -f /usr/bin/dnf ] && \
        cat /etc/*release | grep -q -e "Red Hat" -e "CentOS" -e "CloudLinux" && \
        cat /etc/*release | grep -q 'release 9'
}


if is_rhel9; then
    # Install CentOS OpenStack repos so that we have access to some extra
    # packages.
    sudo dnf install -y centos-release-openstack-zed
    sudo dnf install -y liberasurecode-devel nasm
fi
