#!/usr/bin/perl
use strict;
use warnings;

if(!$ARGV[0]) {
    print "Bitte die IP des ESP8266-12E-Controllers als Parameter angeben\n";
    exit(1);
}
if(system("wget --version 1>/dev/null 2>&1") != 0) {
    print "Bitte das Tool 'wget' installieren\n";
    exit(1);
}
if(system("wget http://192.168.0.15/zeige_log -O anlagen.log") == 0) {
    print "Erfolg\n";
    exit(0);
}
print "Fehler beim Download\n";
exit(1);
