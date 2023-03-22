#!/usr/bin/perl
use strict;
use warnings;

sub _korrigiere_downloadfehler {# TODO was ist eigentlich die Ursache des Fehlers?
    my ($filename) = @_;
    open(my $fh, '<', $filename) or die $!;
    local $/ = undef;
    my $content = <$fh>;
    close($fh);
    $content =~ s/[\n\r]+[0-9a-f]{2,3}[\n\r]+//gsm;
    open($fh, '>', $filename) or die $!;
    print $fh $content;
    close($fh);
    print "Korrektur...";
}

_korrigiere_downloadfehler($ARGV[0]);
