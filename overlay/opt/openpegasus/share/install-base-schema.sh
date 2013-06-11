#!/bin/sh

set -eu

thisdir=$(dirname "$0")

cimver=223
cimdir="$thisdir/Schemas/CIM$cimver"

pgver=20
pgdir="$thisdir/Schemas/Pegasus/Internal/VER$pgver"
pg_managedsystem_dir="$thisdir/Schemas/Pegasus/ManagedSystem/VER$pgver"

pg_interop_ver=20
pg_interop_dir="$thisdir/Schemas/Pegasus/InterOp/VER$pg_interop_ver"


CIMMOF="cimmof -aE"

$CIMMOF -n root/PG_Internal "$cimdir/qualifiers.mof"
#$CIMMOF -n root/PG_Internal "$cimdir/cim_schema_2.23.0.mof"
$CIMMOF -n root/PG_Internal -I "$pgdir" "$pgdir/PG_InternalSchema$pgver.mof"
$CIMMOF -n root/PG_Internal -I "$pgdir" "$pgdir/PG_SLPTemplate.mof"

$CIMMOF -n root/PG_InterOp "$cimdir/qualifiers.mof"
$CIMMOF -n root/PG_InterOp "$cimdir/cim_schema_2.23.0.mof"
$CIMMOF -n root/PG_InterOp "$pg_interop_dir/PG_InterOpSchema$pg_interop_ver.mof"
#$CIMMOF -n root/PG_InterOp -I "$pg_interop_dir" "$pg_interop_dir/PG_CIMXMLCommunicationMechanism$pg_interop_ver.mof"
#$CIMMOF -n root/PG_InterOp -I "$pg_interop_dir" "$pg_interop_dir/PG_Namespace$pg_interop_ver.mof"
#$CIMMOF -n root/PG_InterOp -I "$pg_interop_dir" "$pg_interop_dir/PG_ServerProfile$pg_interop_ver.mof"

#$CIMMOF -n root/PG_InterOp "$pg_managedsystem_dir/PG_ManagedSystemSchema${pgver}R.mof"
#$CIMMOF -n root/PG_InterOp "$pg_managedsystem_dir/PG_SLPProvider${pgver}R.mof"

$CIMMOF -n root/cimv2 -I "$cimdir" "$cimdir/cim_schema_2.23.0.mof"
$CIMMOF -n root/cimv2 -I "$pg_interop_dir" "$pg_interop_dir/PG_Events$pg_interop_ver.mof"
#$CIMMOF -n root/cimv2 -I "$pgdir" "$pgdir/PG_ManagedSystemSchema$pgver.mof"
#$CIMMOF -n root/cimv2 -I "$pgdir" "$pgdir/PG_ManagedSystemSchema${pgver}R.mof"
