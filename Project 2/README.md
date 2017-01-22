# Project 2

Description : http://www.cse.psu.edu/~trj1/cmpsc473-s16/cmpsc473-p2.html

Completed for Operating system class. 


Toy signature scanner
=================

This program scans input files within a single directory to detect
whether signature values are present in each file.  When a signature
match is found, a job is created to modify the signature text (e.g.,
make it upper case), creating a version of the file with the signature
text replaced.

Build
=====
make

Rebuild
=======
make clean; make

Create Submission
=================
(1) Replace the PSUID in the Makefile with your PSU ID.  
(2) From p2 directory (make sure it's the right one): make tar

Usage
=====
project2 <input directory for scanning> <number of writers>

A sample directory, "test" included in tarball for testing. It
contains one large file "2000010.txt" and two other small files. Small
files are to test thread interleaving across multiple files.  The
large file is to test thread interleaving on a single file.

"Number of writers" is the number of writer threads - between 0 and 2
for testing.

Logging (example)
=======
./project2 test 2 > out.log
Please find all diagnois output in file out.log

Signature ==> Fix
=======================
"the" ==> "THE"
"but" ==> "BUT"
"rat" ==> "RAT"
"and" ==> "AND"
"far" ==> "FAR"
