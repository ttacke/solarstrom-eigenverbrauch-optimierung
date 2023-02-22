#!/usr/bin/perl
use strict;
use warnings;

if(!$ARGV[0]) {
    print "Bitte die IP des ESP8266-12E-Controllers als Parameter angeben\n";
    exit(1);
}
if(system("curl --version 1>/dev/null 2>&1") != 0) {
    print "Bitte das Tool 'curl' installieren\n";
    exit(1);
}
if(system("curl -XPOST -F file=\@../sd-karteninhalt/index.html 'http://$ARGV[0]/upload_file?name=index.html' 1>/dev/null 2>&1") == 0) {
    print "Erfolg\n";
    exit(0);
}
print "Fehler beim Upload\n";
exit(1);
