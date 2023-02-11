#!/usr/bin/perl
use strict;
use warnings;

my $verbrauch = [];
open(my $fh, '<', './anlagen.log') or die "Bitte erst 'lade_aktuelle_logdatei.pl' ausfuehren\n";
$/ = "\n";
while(my $line = <$fh>) {
    next if($line !~ m/\d+,ev1,[\-\d\.]+,[\-\d\.]+,[\-\d\.]+,[\-\d\.]+,([\-\d\.]+)/);

    push(@$verbrauch, $1) if($1 > 0);
}
close($fh);
print scalar(@$verbrauch) . " Datensaetze, Grundverbrauch: " . (sort(@$verbrauch))[int(scalar(@$verbrauch) / 2)] . " W\n";
