#!/usr/bin/perl -w

# this program was just to help me learn the protocol for rtl_tcp
use strict;
use warnings qw(all);

use IO::Socket::INET;
use Audio::Wav;

$| = 1;


sub bs {
 my $i = shift;

 return (($i & 0xff) << 24) | (($i & 0xff00) << 8) |
        (($i & 0xff0000) >> 8) | (($i & 0xff000000) >> 24);
};

my $socket = new IO::Socket::INET(
 PeerHost => '127.0.0.1',
 PeerPort => '1234',
 Proto    => 'tcp') or die "-err- could not open socket connection $!\n";


my $aw = new Audio::Wav;
my $writer = $aw->write("foo.wav", {  bits_sample => 16, sample_rate => 256000, channels => 2 });


binmode($socket);


my $message = pack("WV",0x1,bs(88500000));
$socket->send($message);
$message = pack("WV",0x2,bs(256000));
$socket->send($message);

my $ct = 0;
my $max_buffers = 100;
my $data = '';

my $start = time;
while (time - $start < 10) {
	
 my $recvd = $socket->read($data,1024*32);
 print "buffer $ct, received $recvd\n";
 $writer->write_raw($data,$recvd);
 $ct++;
}

