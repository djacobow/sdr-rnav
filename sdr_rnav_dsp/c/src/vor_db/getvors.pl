#!/usr/bin/perl -w

use strict;
use warnings qw(all);

use LWP::Simple;
use File::stat;
use Data::Dumper;
use FindBin qw($Bin);
use YAML qw(DumpFile);

my $root_name   = 'vordb';
if (defined($ARGV[0])) { $root_name = $ARGV[0]; };

my $site_name   = "http://www.ourairports.com/data/navaids.csv";
my $local_nfn   = $Bin . "/navaids.csv";
my $max_age     = 7*24*60*60; # seconds
my $oname_cpp   = $root_name . ".c";
my $oname_h     = $root_name . ".h";
my $oname_yaml  = $root_name . ".yaml";

# this restricts to subset of USA and Canada more or less. HI and AK
# should be included.
my $restrictions = {
 min_lat   => 24,
 max_lat   => 72,
 min_long  => -180,
 max_long  => -50,
};

my $selems = {
 id        => 'char[4]',
 latitude  => 'float',
 longitude => 'float',
 magnetic_variation => 'float',
 elevation => 'float',
 frequency => 'uint32_t',
};

my $ncsv = loadData(site_name => $site_name,
                    local_name => $local_nfn,
                    max_age => $max_age,
                    force_fetch => 0,
                    force_local => 0);


my $vordata = pullVORs(csvdata => $ncsv, restrictions => $restrictions);

makeTable(vor_data => $vordata, 
	  output_fname_cpp => $oname_cpp,
	  output_fname_h   => $oname_h,
          struct_elems => $selems,
          restrictions => $restrictions);


makeYaml(vor_data => $vordata,
	 fn => $oname_yaml);


sub makeYaml {
 my %args = @_;
 my $d  = $args{vor_data};
 my $fn = $args{fn};

 my $dh = {};
 foreach my $v (@$d) {
 $dh->{ $v->{id} } = $v;
 $v->{frequency} *= 1000;
 };
 DumpFile($fn,$dh);
};


sub makeTable {
 my %args = @_;
 my $ofnc  = $args{output_fname_cpp};
 my $ofnh  = $args{output_fname_h};
 my $indat = $args{vor_data};
 my $selems= $args{struct_elems};

 print "-info- writing database files for compilation\n";

 my $ofhh = undef;
 my $ofhc = undef;

 open($ofhh,'>',$ofnh) or die "-error could not open output file $ofnh\n";
 open($ofhc,'>',$ofnc) or die "-error could not open output file $ofnc\n";

 my $estr = '';
 foreach my $selem (keys %$selems) {
  if ($selems->{$selem} =~ /char\[(\d+)\]/) {
   my $len = $1; 
   $estr .= "  char\t$selem" . '[' . $len . "];\n";
  } else {
   $estr .= "  $selems->{$selem}\t$selem;\n";
  }
 };

 print $ofhh <<"x_str";
#ifndef __VORDB_H
#define __VORDB_H

#include <inttypes.h>

typedef struct vor_data_t {
 $estr
} vor_data_t;

extern vor_data_t vor_data[];
extern uint32_t vor_count;

vor_data_t *find_vor_matching_id_freq(const char *, uint32_t);

#endif

x_str

my $vcount = scalar (@$indat);

print $ofhc <<"y_str";

#include "vordb.h"
#include <string.h>

uint32_t vor_count = $vcount;

vor_data_t *find_vor_matching_id_freq(const char *id, uint32_t freq) {
 uint32_t i;
 vor_data_t *rval = 0;
 for (i=0;i<vor_count;i++) {
  if ((vor_data[i].frequency == freq) && !strcmp(vor_data[i].id,id)) {
   rval = &(vor_data[i]);
   goto done;
  }
 }
done:
 return rval;
}

vor_data_t vor_data[] = {
y_str


 foreach my $vor (@$indat) {
  if (1) {
   print $ofhc " {\n";
   foreach my $n (keys %$selems) {
    my $q = ($n eq 'id') ? '"' : '';
    print $ofhc $q . $vor->{$n} . $q . ",\n";
   }
   print $ofhc " },\n";
  }
 }
 print $ofhc "};\n";
 close $ofhh;
 close $ofhc;
};


sub pullVORs {
 my %args = @_;
 my $incsv        = $args{csvdata};

 my @indata = split(/\n/,$incsv);
 my @vars   = split(/,/,shift @indata);

 print "-info- filtering out VORs only and area restrictions\n";

 my $vor_types = { VOR => 1, 'VOR-DME' => 2, 'VORTAC' => 3 };

 my $odata = [];
 @vars = map { $_ =~ s/"//g; $_; } @vars;
 my $vh = {};
 for (my $i=0;$i<@vars;$i++) { $vh->{$vars[$i]} = $i; };

 while (@indata) {
  my $il = shift @indata; chomp $il;
  my @parts = map { $_ =~ s/"//g; $_; } split(/,/,$il);

  my $type = $parts[$vh->{type}];
  if (defined($vor_types->{$type})) {
   my $lat  = $parts[$vh->{latitude_deg}];
   my $long = $parts[$vh->{longitude_deg}];

   if (($lat > $args{restrictions}{min_lat}) &&
       ($lat < $args{restrictions}{max_lat}) &&
       ($long > $args{restrictions}{min_long}) &&
       ($long < $args{restrictions}{max_long})) {
   
    my $elev = $parts[$vh->{elevation_ft}];
    my $mvar = $parts[$vh->{magnetic_variation_deg}];
    my $id   = $parts[$vh->{ident}];
    my $freq = $parts[$vh->{frequency_khz}];
    push(@$odata, { latitude => $lat, longitude => $long, 
 		    elevation => $elev, 
		    magnetic_variation => $mvar, id => $id, 
		    frequency => $freq });
   }
  }
 }
 return $odata;
};


sub loadData {

 my %args = @_;

 my $fetch_new = 1;

 if (-e $args{local_name}) {
  my $fdata = stat($args{local_name});
  my $mtime = $fdata->[9];
  my $now   = time;
  if ((($now - $mtime) < $args{max_age}) || ($args{force_local})){
   $fetch_new = 0;
  }
 }

 if ($args{force_fetch}) { $fetch_new = 1;};

 my $navcsv = '';
 if ($fetch_new) {
  print "-info- fetching VOR spreadsheet (missing local copy or too old)\n";
  $navcsv    = get($args{site_name});
  if (defined($navcsv) & length($navcsv)) {
   unlink $args{local_name};
   my $ofh = undef;
   open($ofh,'>',$args{local_name}) or die "-error- could not open local vor file for write\n";
   print $ofh $navcsv;
   close $ofh;
  } else {
   die "-error- could not download VOR data\n";
  }
 } else {
  print "-info- loading VOR spreadsheet\n";
  my $ifh = undef;
  open($ifh,'<',$args{local_name}) or die "-error- could not open local vor file for read\n";
  while (<$ifh>) {
   $navcsv .= $_;
  }
  close $ifh;
 }
 return $navcsv;
}

