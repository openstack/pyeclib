#!/bin/bash -xe

# This script is run by OpenStack CI jobs for tox based jobs during
# the pre-run phase.

# Install liberasurecode-devel for CentOS from RDO repository.

function is_rhel7 {
    [ -f /usr/bin/yum ] && \
        cat /etc/*release | grep -q -e "Red Hat" -e "CentOS" -e "CloudLinux" && \
        cat /etc/*release | grep -q 'release 7'
}
function is_rhel8 {
    [ -f /usr/bin/dnf ] && \
        cat /etc/*release | grep -q -e "Red Hat" -e "CentOS" -e "CloudLinux" && \
        cat /etc/*release | grep -q 'release 8'
}


if is_rhel7; then
    # Install CentOS OpenStack repos so that we have access to some extra
    # packages.
    sudo yum install -y centos-release-openstack-train
    # Now that RDO repositories are enabled, install missing
    # packages.
    sudo yum install -y liberasurecode-devel yasm
fi

if is_rhel8; then
    # Install CentOS OpenStack repos so that we have access to some extra
    # packages.
    sudo dnf install -y centos-release-openstack-ussuri
    sudo dnf install -y liberasurecode-devel yasm
fi
