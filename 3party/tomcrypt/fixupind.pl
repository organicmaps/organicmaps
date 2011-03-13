open(IN,"<crypt.ind");
open(OUT,">crypt.ind.tmp");
$a = <IN>;
print OUT  "$a\n\\addcontentsline{toc}{chapter}{Index}\n";
while (<IN>) {
   print OUT $_;
}
close OUT;
close IN;
system("mv -f crypt.ind.tmp crypt.ind");

