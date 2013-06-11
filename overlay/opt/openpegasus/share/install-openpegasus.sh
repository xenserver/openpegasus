#!/bin/sh

set -eu

export PEGASUS_HOME=/opt/openpegasus
export PATH=$PATH:/opt/openpegasus/bin

thisdir=$(dirname "$0")

if ! pidof /opt/xensource/bin/xapi >/dev/null
then
  echo "Saving install of OpenPegasus until next boot."
  cp "$thisdir/66-install-openpegasus" /etc/firstboot.d
  chmod a+x /etc/firstboot.d/66-install-openpegasus
  exit 0
fi

chkconfig --add openpegasus

if [ $1 = 1 ]
then
  service openpegasus start
  $thisdir/install-base-schema.sh
else
  service openpegasus restart
fi
