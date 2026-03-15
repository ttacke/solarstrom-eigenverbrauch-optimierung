#!/usr/bin/perl
use strict;
use warnings;
$| = 1;

sub _hole_daten {
    my $daten = [];
    my @files = ();
    print "Hole Daten...\n";
    opendir(my $dh, '../sd-karteninhalt') or die $!;
    while(my $filename = readdir($dh)) {
        next if($filename !~ m/^anlage_log\-\d{4}\-\d{2}\.csv$/);

        push(@files, $filename);
    }
    closedir($dh);
    if(!scalar(@files)) {
        die "Bitte erst 'download_der_daten_der_zentrale.pl [IP]' ausfuehren\n";
    }

    my $fehler = 0;
    my $last_error_line = '';
    my $i = 0;
    foreach my $filename (sort(@files)) {
        my $fh;
        if(!open($fh, '<', "../sd-karteninhalt/$filename")) {
            die "Fehler: $!\n";
        }
        $/ = "\n";
        while(my $line = <$fh>) {
            $i++;
            printf("\r%3.2f Mio zeilen", $i / 1000000) if($i % 10000 == 0);
            chomp($line);
            if($line =~ /\.\./ || $line !~ /^\d{10}/) {
                # warn $line;
                $last_error_line = $line;
                $fehler++;
                next;
            }
            my @e = $line =~ m/^(\d{10,}),(e[234]),([\-\d]+),([\-\d]+),(\d+),(\d+),(\d+),([\-\d]+),([\-\d]+),([\-\d]+),(\d+),(\d+),(?:(\d+),|)(?:(\d+),(\d+),(\d+),|)/;
            if(!scalar(@e)) {
                # warn $line;
                $last_error_line = $line;
                $fehler++;
                next;
            }

            my @w = $line =~ m/,w[2],(\d+),(\d+)(?:,(\d+)|)/;
            if(!scalar(@w)) {
                # warn $line;
                $last_error_line = $line;
                $fehler++;
                next;
            }

            my @t = $line =~ m/,kb?1([\-\d\.]+),([\d\.]+),kl?1([\-\d\.]+),([\d\.]+)(?:,hs(\d),(\d+)|)/;

            my @auto    = $line =~ m/,va[4],([a-z]+),([a-z]+),([a-z]+),([\d\.]+),([\d\.]+),([\d\.]+),([a-z]+),([a-z]+)/;
            my @roller  = $line =~ m/,vb[8],([a-z]+),([a-z]+),([a-z]+),([\d\.]+),([\d\.]+),([\d\.]+),([a-z]+),([a-z]+),([a-z]+)/;
            my @begleitheiz = $line =~ m/,vc[12],([a-z]+),([\d]+),([a-z]+),([\d]+)(?:,([\d]+)|)/;

            my @d = gmtime($e[0]);
            my $neu = {
                zeitpunkt                             => $e[0],
                datum                                 => sprintf("%04d-%02d-%02d", $d[5] + 1900, $d[4] + 1, $d[3]),
                minute                                => sprintf('%02d', $d[1]),
                stunde                                => sprintf('%02d', $d[2]),
                tag                                   => sprintf('%02d', $d[3]),
                monat                                 => sprintf('%02d', $d[4] + 1),
                jahr                                  => sprintf('%d', $d[5] + 1900),

                netzbezug_in_w                        => $e[2],
                solarakku_zuschuss_in_w               => $e[3],
                solarerzeugung_in_w                   => $e[4],
                stromverbrauch_in_w                   => $e[5],
                solarakku_ladestand_in_promille       => $e[6],
                l1_strom_ma                           => $e[7],
                l2_strom_ma                           => $e[8],
                l3_strom_ma                           => $e[9],
                gib_anteil_pv1_in_prozent             => $e[10],
                ersatzstrom_ist_aktiv                 => $e[11] ? 1 : 0,
                gesamt_energiemenge_in_wh             => $e[1] eq 'e2' ? 0 : $e[12],

                l1_solarstrom_ma                      => ($e[1] eq 'e2' || $e[1] eq 'e3') ? 0 : $e[13],
                l2_solarstrom_ma                      => ($e[1] eq 'e2' || $e[1] eq 'e3') ? 0 : $e[14],
                l3_solarstrom_ma                      => ($e[1] eq 'e2' || $e[1] eq 'e3') ? 0 : $e[15],

                stunden_solarstrahlung                => $w[0],
                tages_solarstrahlung                  => $w[1],
                zeitpunkt_sonnenuntergang             => $w[2],

                waermepumpen_zuluft_temperatur        => $t[0],
                waermepumpen_zuluft_luftfeuchtigkeit  => $t[1],
                waermepumpen_abluft_temperatur        => $t[2],
                waermepumpen_abluft_luftfeuchtigkeit  => $t[3],

                heizungsunterstuetzung_an             => $t[4],
                heizung_temperatur_differenz          => $t[5] ? $t[5] - 540 : undef,
                heizungs_temperatur_differenz_in_grad => $t[5] ? ($t[5] - 540) / 1.875 : undef,

                begleitheizung_an                     => undef,
                begleitheizung_leistung               => undef,
                wasser_wp_leistung                    => undef,

                auto_ladestatus                       => undef,
                auto_laden_ist_an                     => undef,
                auto_lastschutz                       => undef,
                wasser_relay_ist_an                   => undef,
                wasser_lastschutz                     => undef,

                roller_ladestatus                     => undef,
                roller_laden_ist_an                   => undef,
                roller_lastschutz                     => undef,
                roller_benoetigte_ladeleistung_in_w   => undef,
                aktuelle_roller_ladeleistung_in_w     => undef,
                genutzte_roller_ladeleistung_in_w     => undef,
                heizung_relay_ist_an                  => undef,
                heizung_lastschutz                    => undef,
                ladeverhalten_wintermodus             => undef,
            };

            if(scalar(@begleitheiz)) {
                $neu->{'begleitheizung_an'} = ($begleitheiz[2] eq 'on' ? 1 : 0);
                $neu->{'begleitheizung_leistung'} = $begleitheiz[3];
                $neu->{'wasser_wp_leistung'} = (exists($begleitheiz[4]) ? $begleitheiz[4] : 0);
            }

            if(scalar(@auto)) {
                $neu->{'auto_ladestatus'}                   = $auto[0];
                $neu->{'auto_laden_ist_an'}                 = ($auto[1] eq 'on' ? 1 : 0);
                $neu->{'auto_lastschutz'}                   = ($auto[2] eq 'on' ? 1 : 0);
                $neu->{'wasser_relay_ist_an'}               = ($auto[6] eq 'on' ? 1 : 0);
                $neu->{'wasser_lastschutz'}                 = ($auto[7] eq 'on' ? 1 : 0);
            }

            if(scalar(@roller)) {
                $neu->{'roller_ladestatus'}                   = $roller[0];
                $neu->{'roller_laden_ist_an'}                 = ($roller[1] eq 'on' ? 1 : 0);
                $neu->{'roller_lastschutz'}                   = ($roller[2] eq 'on' ? 1 : 0);
                $neu->{'roller_benoetigte_ladeleistung_in_w'} = $roller[3];
                $neu->{'aktuelle_roller_ladeleistung_in_w'}   = $roller[4];
                $neu->{'genutzte_roller_ladeleistung_in_w'}   = $roller[5];
                $neu->{'heizung_relay_ist_an'}                = ($roller[6] eq 'on' ? 1 : 0);
                $neu->{'heizung_lastschutz'}                  = ($roller[7] eq 'on' ? 1 : 0);
                $neu->{'ladeverhalten_wintermodus'}           = ($roller[8] eq 'on' ? 1 : 0);
            }

            push(@$daten, $neu);
        }
        close($fh);
    }
    printf("\r%3.2f Mio zeilen\n", $i / 1000000);
    print "Sortiere...\n";
    $daten = [sort { $a->{zeitpunkt} <=> $b->{zeitpunkt} } @$daten];
    print "...Fertig\n";
    if($fehler) {
        warn "ACHTUNG: $fehler Fehler!\n";
        #warn $last_error_line;

    }
    return $daten;
}
my $daten = _hole_daten();
open(my $log_fh, '>', 'solar.csv') or die $!;
my @header = sort(keys(%{$daten->[0]}));
print $log_fh join(',', @header) . "\n";
foreach my $e (@$daten) {
    my $line = '';
    foreach my $key (@header) {
        $line .= (defined($e->{$key}) ? $e->{$key} : '') . ",";
    }
    print $log_fh $line . "\n";
}
