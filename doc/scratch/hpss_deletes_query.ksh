#!/usr/bin/env ksh

db2 connect to hsubsys1
db2 set schema hpss

#get all object_id's that have been deleted
# the export to <file_name> and date or timestamp field need to 
#made as variable input parameters to this script for the query below.

db2 " export to /ccs/hpssdev/reference/MetadataIndexing/raghul_devel/deletes.csv of del 
    SELECT distinct h.object_id from nsobject_history h 
        where h.sys_end > '2018-01-10' and 
            h.object_id NOT IN (SELECT object_id from nsobject)" 

db2 connect reset
