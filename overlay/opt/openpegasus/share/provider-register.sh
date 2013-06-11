#!/bin/sh
# ==================================================================
#
# Copyright (c) 2010 Citrix Systems, Inc.
#
# This is a trimmed down version of a file taken from the xen-cim project:
#
# (C) Copyright IBM Corp. 2006
#
# THIS FILE IS PROVIDED UNDER THE TERMS OF THE COMMON PUBLIC LICENSE
# ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
# CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
#
# You can obtain a current copy of the Common Public License from
# http://www.opensource.org/licenses/cpl1.0.php
#
# Author:       Viktor Mihajlovski <mihajlov@de.ibm.com>
# Contributors:
# Description:  Script to install class definitions (MOFs) and 
#               registration data for a variety of supported CIMOMs
# ==================================================================

thisdir=$(dirname "$0")
if [ -z "$PEGASUS_HOME" ]
then
    export PEGASUS_HOME=$(readlink -f "$thisdir/..")
fi

pegasus_bin="$PEGASUS_HOME/bin"
pegasus_repository="$PEGASUS_HOME/repository"

pegasus_transform()
{
    OUTFILE=$1
    shift
    regfiles=$*
    PROVIDERMODULES=`cat $regfiles 2> /dev/null | grep -v '^[[:space:]]*#.*' | cut -d ' ' -f 4 | sort | uniq`
    if test x"$PROVIDERMODULES" = x
    then
	echo Failed to read registration files >&2
	return 1
    fi
    PROVIDERS=`cat $regfiles 2> /dev/null | grep -v '^[[:space:]]*#.*' | cut -d ' ' -f 3-4 | sort | uniq`
    
# produce ProviderModules
    echo > $OUTFILE
    chatter "Processing provider modules:" $PROVIDERMODULES
    for pm in $PROVIDERMODULES
    do
      cat >> $OUTFILE <<EOFPM
instance of PG_ProviderModule
{
   Name = "$pm";
   Location = "$pm";
   Vendor = "SBLIM";
   Version = "2.0.0";
   InterfaceType = "CMPI";
   InterfaceVersion = "2.0.0";
};

EOFPM
    done
    
# produce Providers
    set -- $PROVIDERS
    while test x$1 != x
    do
      cat >> $OUTFILE <<EOFP
instance of PG_Provider
{
   Name = "$1";
   ProviderModuleName = "$2";
};

EOFP
      shift 2
    done

#produce Capabilities
    let serial=0
    for rf in $regfiles
    do
      cat $rf | grep -v '^[[:space:]]*#.*' | while read CLASSNAME NAMESPACE PROVIDERNAME PROVIDERMODULE CAPS
      do
	let serial=serial+1
	numcap=
	for cap in $CAPS
	do
	  case $cap in
	      instance) 
		  if test x$numcap = x 
		  then numcap=2
		  else numcap="$numcap,2"
		  fi;;
	      association) 
		  if test x$numcap = x 
		  then numcap=3
		  else numcap="$numcap,3"
		  fi;;
	      indication) 
		  if test x$numcap = x
		  then numcap=4
		  else numcap="$numcap,4"
		  fi;;
	      method) 
		  if test x$numcap = x 
		  then numcap=5
		  else numcap="$numcap,5"
		  fi;;
	      **) echo unknown provider type $cap >&2
		  return 1;;
	  esac	  
	done
	cat >> $OUTFILE <<EOFC
instance of PG_ProviderCapabilities
{
   ProviderModuleName = "$PROVIDERMODULE";
   ProviderName = "$PROVIDERNAME";
   ClassName = "$CLASSNAME";
   ProviderType = { $numcap };
   Namespaces = {"$NAMESPACE"};
   SupportedProperties = NULL;
   SupportedMethods = NULL;
   CapabilityID = "$CLASSNAME-$serial";
};

EOFC
      done
    done
}

pegasus_install()
{
    if ps -C cimserver > /dev/null 2>&1 
    then
	CIMMOF="$pegasus_bin/cimmof"
	state=active
    else
	CIMMOF="$pegasus_bin/cimmofl -R $pegasus_repository"
	state=inactive
    fi

    if test x$namespace != x
    then
	CIMMOF="$CIMMOF -n $namespace"
    fi

    mofpath=
    mymofs=
    mregs=
    mofmode=1
    while test x$1 != x
    do 
      if test $1 = ":"
      then 
	  mofmode=0
	  shift
	  continue
      fi
      if test $mofmode = 1
      then
	  if test x$mofpath = x
	  then
	      mofpath=`dirname $1`
	  fi
	  mymofs="$mymofs $1"
      else
	  myregs="$myregs $1"
      fi
      shift
    done
  
    for _TEMPDIR in /var/tmp /tmp
    do
      if test -w $_TEMPDIR
      then
	  _REGFILENAME=$_TEMPDIR/$$.mof
	  break
      fi
    done

    
    trap "rm -f $_REGFILENAME" EXIT

    if pegasus_transform $_REGFILENAME $myregs
    then
	chatter Registering providers with $state cimserver

	$CIMMOF -uc -aE -I $mofpath $mymofs &&
	$CIMMOF -uc -aE -n root/PG_InterOp $_REGFILENAME
    else
	echo "Failed to build pegasus registration MOF." >&2
	return 1
    fi
}

pegasus_uninstall()
{
    mymofs=
    mregs=
    mofmode=1
    while test x$1 != x
    do 
      if test $1 = ":"
      then 
	  mofmode=0
	  shift
	  continue
      fi
      if test $mofmode = 1
      then
	  mymofs="$mymofs $1"
      else
	  myregs="$myregs $1"
      fi
      shift
    done
  
    if ps -C cimserver > /dev/null 2>&1 
    then
	PROVIDERMODULES=`cat $myregs 2> /dev/null | grep -v '^[[:space:]]*#.*' | cut -d ' ' -f 4 | sort | uniq`
	if test x"$PROVIDERMODULES" = x
	    then
	    echo Failed to read registration files >&2
	    return 1
	fi
	CIMPROVIDER="$pegasus_bin/cimprovider"
	for pm in $PROVIDERMODULES
	do
	  chatter "Remove provider module" $pm
	  $CIMPROVIDER -d -m $pm > /dev/null &&
	  $CIMPROVIDER -r -m $pm > /dev/null
	done
	WBEMEXEC="$pegasus_bin/wbemexec"
	CLASSES=`cat $myregs 2> /dev/null | grep -v '^[[:space:]]*#.*' | cut -d ' ' -f 1 | grep -v '^CIM_'`
	for cls in $CLASSES
	do
	  chatter Delete CIM Class $cls
	  $WBEMEXEC > /dev/null <<EOFWX
<?xml version="1.0" encoding="utf-8" ?>
<CIM CIMVERSION="2.0" DTDVERSION="2.0">
 <MESSAGE ID="4711" PROTOCOLVERSION="1.0">
  <SIMPLEREQ>
   <IMETHODCALL NAME="DeleteClass">
    <LOCALNAMESPACEPATH>
     <NAMESPACE NAME="root"></NAMESPACE>
     <NAMESPACE NAME="cimv2"></NAMESPACE>
    </LOCALNAMESPACEPATH>
    <IPARAMVALUE NAME="ClassName">
     <CLASSNAME NAME="$cls"/>
    </IPARAMVALUE>
   </IMETHODCALL>
  </SIMPLEREQ>
 </MESSAGE>
</CIM>
EOFWX
	done
    else
	echo "Sorry, cimserver must be running to deregister the providers." >&2
	return 1
    fi
}

usage() 
{
    echo "usage: $0 [-h] [-v] [-d] [-n <namespace>] -r regfile ... -m mof ..."
}

chatter()
{
    if test x$verbose != x
    then
	echo $*
    fi
}

gb_getopt()
{
    rmode=0
    mmode=0
    options=
    moffiles=
    registrations=
    while [ -n "$1" ]
    do
      case $1 in
	  -r) mmode=0;
	      rmode=1;
	      shift;;
	  -m) mmode=1;
	      rmode=0;
	      shift;;
	  -*) mmode=0;
	      rmode=0;
	      options="$options $1";
	      shift;;
	  **) if [ $mmode = 1 ] 
	      then moffiles="$moffiles $1"       
	      elif [ $rmode = 1 ]
	      then registrations="$registrations -r $1" 
	      else options="$options $1";
	      fi; 
	      shift;;
      esac
    done
    cat <<EOF
$options $registrations $moffiles
EOF
}

prepargs=`gb_getopt $*`
args=`getopt dvhn:r: $prepargs`
rc=$?

if [ $rc = 127 ]
then
    echo "warning: getopt not found ...continue without syntax check"
    args=$prepargs
elif [ $rc != 0 ]
then
    usage $0
    exit 1
fi

set -- $args

while [ -n "$1" ]
do
  case $1 in
      -h) help=1; 
	  shift;
	  break;;
      -v) verbose=1; 
	  shift;;
      -d) deregister=1; 
	  shift;;
      -n) namespace=$2;
	  shift 2;;
      -r) regs="$regs $2";
	  shift 2;;
      --) shift;
	  break;;
      **) break;;
  esac
done

mofs=$*

if [ "$help" = "1" ]
then
    usage
    echo -e "\t-h display help message"
    echo -e "\t-v verbose mode"
    echo -e "\t-d deregister provider and uninstall schema"
    echo -e "\t-n specify namespace"
    echo -e "\t-r specify registration files"
    echo -e "\t-m specify schema mof files"
    echo
    echo Use this command to install schema mofs and register providers.
    echo CIM Server Type is required as well as at least one registration file and one mof.
    exit 0
fi

if test x"$mofs" = x || test x"$regs" = x
then
    usage $0
    exit 1
fi

if test x$deregister = x
then
    pegasus_install $mofs ":" $regs
else
    pegasus_uninstall $mofs ":" $regs
fi
