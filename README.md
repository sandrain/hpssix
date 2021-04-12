# HPSSIX: HPSS Metadata Indexing

HPSSIX incrementally indexes metadata of files in running HPSS system and
allows users to quickly locate files of interest with file metadata, including
POSIX file attributes (or stat metadata) and POSIX extended attributes. HPSSIX
also features the fulltext search for supported file types, e.g., Microsoft
Office document files.

This framework enables fast file search operations.

```
## cp files to hpss
$ mkdir /var/hpss/mnt/home/hs2/demo
$ cp -r documents /var/hpss/mnt/home/hs2/demo

## setting some extended attributes
$ for f in /var/hpss/mnt/home/hs2/demo/documents/mtg.notes/*; do hpssix-tag set -n projid -v 100 $f; done

## show extended attributes
$ for f in /var/hpss/mnt/home/hs2/demo/documents/mtg.notes/*; do getfattr -d $f; done

## index -- this takes time (around 90sec), automatically runs daily
$ hpssixd.sh action

## search -- stat, name, path
$ hpssix-search --uid=`id -u` --count-only (--file-only) (--directory-only) (--verbose) (--limit)
$ hpssix-search --uid=`id -u` --name=pptx
$ hpssix-search --uid=`id -u` --path=papers (--file-only) (--directory-only)
$ hpssix-search --uid=`id -u` --name=pptx --date='>20190227-15:00:00'

## tagging
$ hpssix-search --tag="projid:" (--tag="projid:100", --tag="projid:0,200", --tag="projid:>=90.56")
$ hpssix-search --tag="projid:<=100"

## keyword
$ hpssix-search --keyword="parallel file system performance titan" --name='pptx'
$ hpssix-search --keyword='checkpoint burst buffers' --name=pptx

## document display
$ hpssix-meta --meta /var/hpss/mnt/home/hs2/tests/documents/projects/project-poster.pptx
$ hpssix-meta --text /var/hpss/mnt/home/hs2/tests/documents/projects/project-poster.pptx
```

The architecture is summarized in the following slides.

* [HPSS Metadata Indexing slides](https://drive.google.com/file/d/13T_HOQbkeJbNtdTiP2Xr5OMhA1d2HKED/view?usp=sharing)
* [Performance test](https://docs.google.com/presentation/d/1a6zdbaiwfsCqKLf3YJ9bEb0zQhlctn3s6l8WdPeVzZQ/edit?usp=sharing)

This work was presented in the following events.

* Hyogi Sim, "Extracting Metadata from the ORNL HPSS Archive to Improve its
Usability," Knowledge Is Power: Unleashing the Potential of Your Archives
through Metadata, BoF in ACM/IEEE International Conferenece for HighPerformance
Computing, Networking, Storage, and Analysis (SC), Denver, CO, November 2019
* Hyogi Sim, "Making a Peta-Scale Archival Storage System Searchable,"
High Performance Storage Systems User Forum2019 (HUF 2019),
Indiana University, Bloomington, IN, October 2019

