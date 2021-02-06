#!/bin/bash

source dkms.conf >/dev/null

set -x
dkms status
sudo ln -sfT $PWD /usr/src/"$PACKAGE_NAME-$PACKAGE_VERSION"
sudo dkms add "$PACKAGE_NAME/$PACKAGE_VERSION"
#sudo dkms install "$PACKAGE_NAME/$PACKAGE_VERSION"
#sudo dkms install sparse-keymap-all/1.0 -k 5.9.0-1-amd64/x86_64 -k 5.9.0-1-686-pae/i686
sudo dkms autoinstall
dkms status
