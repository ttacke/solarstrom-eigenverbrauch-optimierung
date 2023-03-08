#!/usr/bin/perl
use strict;
use warnings;

if(system("curl --version 1>/dev/null 2>&1") != 0) {
    print "Bitte das Tool 'curl' installieren\n";
    exit(1);
}
if(!$ARGV[0]) {
    $ARGV[0] = '192.168.0.30';
    print "Benutzte IP des ESP8266-12E-Controllers(zentrale): $ARGV[0]\n";
}

my $content = `curl -i -XPOST -F file=\@../sd-karteninhalt/index.html 'http://$ARGV[0]/upload_file?name=index.html' 2>&1`;
if($content =~ m#HTTP/1.1 201 Created#) {
    print "Erfolg\n";
    exit(0);
}
print "Fehler beim Upload\n";
exit(1);
