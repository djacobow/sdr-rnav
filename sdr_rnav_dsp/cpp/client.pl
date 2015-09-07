#!/usr/bin/perl -w

use strict;
use warnings qw(all);
use JSON qw(decode_json);
use IO::Socket;
use Data::Dumper;

$| = 1;

my $sock = IO::Socket::INET->new(PeerAddr => "localhost",
	                         PeerPort => "4044",
			         Proto => 'tcp',
                                 Blocking => 1);
my $i = 0;

my $commands = [
  'status',
  'setfft 1',
  'showfft',
  'showfft',
  'setfft 0',
  'status',
  'status',
  'status',
  'status',
  'shutdown',
# 'mixer_lo 0',
];



foreach my $command (@$commands) {
 my $res = sendCommand($sock,$command);
 print Dumper $res;
};


# my $sweep_results = sweep_freqs();
# print Dumper $sweep_results;

$sock->close();


sub sendCommand {
 print "SEND COMMAND =START ================\n";
 my $s  = shift;
 my $is = shift;
 my $l  = length($is);
 my $ostr = sprintf("%3.3d%s",$l,$is);
 print "sending: $ostr\n";
 $s->write($ostr); 
 print "sent\n";
 #sleep 1;
 my $rl;
 my $rc = sysread($s,$rl,3);
 $rl += 0;
 my $b = "";
 if ($rc == 3) {
  $rc = sysread($s,$b,$rl);
 }
 my $x = decode_json($b);
 #print $b;
 print Dumper $x;
 print "SEND COMMAND =END ================\n\n\n\n\n";
};


sub sweep_freqs {
 my $starting_freq = 108000000;
 my $ending_freq   = 118000000;
 my $freq_step     = 50000;
 my $d = {};
 for (my $f=$starting_freq;$f<$ending_freq; $f+=$freq_step) {
  my $res = sendCommand($sock,"tune $f");
  my $r_f = 0;
  my $bcount = 0;
  while ($r_f != $f) {
   $res = sendCommand($sock,"status");
   $r_f = $res->{freq};
   $bcount = $res->{buffer_ct}; 
  }
  my $min_tune_time = $bcount + 10;
  while ($bcount < $min_tune_time) {
   $res = sendCommand($sock,"status");
   $bcount = $res->{buffer_ct}; 
  }
  $d->{$f} = $res;
 }
 return $d;
};

