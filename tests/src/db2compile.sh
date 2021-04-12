#!/bin/bash

DB2INSTALL=/opt/ibm/db2/default

$DB2INSTALL/bin/db2 connect to hsubsys1 user hpss using oldrat99
$DB2INSTALL/bin/db2 set schema hpss

$DB2INSTALL/bin/db2 precompile db2-fullpath.sqc
$DB2INSTALL/bin/db2 prep db2-fullpath.sqc bindfile
$DB2INSTALL/bin/db2 bind db2-fullpath.bnd

gcc -I${DB2INSTALL}/include -c db2-fullpath.c
gcc -o db2-fullpath db2-fullpath.o -L${DB2INSTALL}/lib64 -ldb2

$DB2INSTALL/bin/db2 connect reset

