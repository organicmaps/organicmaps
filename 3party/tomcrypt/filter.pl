#!/usr/bin/perl

# we want to filter every between START_INS and END_INS out and then insert crap from another file (this is fun)

$dst = shift;
$ins = shift;

open(SRC,"<$dst");
open(INS,"<$ins");
open(TMP,">tmp.delme");

$l = 0;
while (<SRC>) {
   if ($_ =~ /START_INS/) {
      print TMP $_;
      $l = 1;
      while (<INS>) {
         print TMP $_;
      }
      close INS;
   } elsif ($_ =~ /END_INS/) {
      print TMP $_;
      $l = 0;
   } elsif ($l == 0) {
      print TMP $_;
   }
}

close TMP;
close SRC;
