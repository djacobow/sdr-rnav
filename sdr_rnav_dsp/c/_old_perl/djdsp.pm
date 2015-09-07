#/ Author: David Jacobowitz
#/         david.jacobowitz@gmail.com
#/
#/ Date  : Spring 2013
#/
#/ Copyright 2013, David Jacobowitz

# This package provides a control and read interface to the 
# SDR-RNAV project.  Everything in c++-land is the interface
# to the hardware radio as well as DSP to decode the VOR signals

# This package uses Inline::C which uses lots of deep magic I 
# don't really uderstand, except that I know that linking is 
# crazy complicated.

package djdsp;

use File::Slurp;
use Exporter qw(import);
use Data::Dumper;
use Cwd;

# only export to perl the functions that are of interest to 
# the perl world, which I have all named with the same 
# start
our @EXPORT = findFuncs("radio_", "top_level.c");

print " ===== DJDSP exports the following functions =====\n" .
      join(',',@EXPORT) . "\n" .
      " ===== ===================================== =====\n" .
      "\n\n";

# These files must be compiled...
my @cpp_files = (
  "flags.c",               # macro defines and some Inline:CPP
                           #  bug workarounds
  "../src/my_console.c",   # some helpers for console IO. May
                           #  not really be needed since perl
			   #  will provide our console
  "../src/autofilter.c",   # the filter coefficientes used by 
                           #  dsp_chain
  "../src/my_qs.c",        # implementation of concurrent queues
                           #  of certain data structures
  "../src/string_helper.c", # some string helper routines
  "../src/dsp_chain.c",    # the dsp chain itself. This is not
                           #  included in the djdsp lib, so we 
			   #  need it here.
  "top_level.c",           # wrappers for the functions we want
);

# Inline::C needs absolute paths for its include paths,
# but Inline::C also likes to run in the "BEGIN" clause. So
# in order to allow automated creation of the full paths, 
# this needs to be in a BEGIN, too.
my ($inc, $l_path, $l_f_path, $pwd);
BEGIN {
 $pwd = getcwd;
 chomp $pwd;
 my @idirs     = qw(../dp
                    ../inc
                    ../src
                    ../dp/kiss_fft130
                    .);
 my @ldirs = ("/../dp");
 my @lfiles = ();
                   
 if ($^O eq 'MSWin32') {
  push(@idirs,  "../../rtl_tools");
  push(@ldirs,  "../../rtl_tools/Win32");
  push(@lfiles, "/../dp/dp.dll");
  # push(@lfiles, "/../../rtl_tools/Win32/rtlsdr.dll");
 } else {
  push(@lfiles, "/../dp/libdp.so.0");
#  $l_f_path .= "/usr/local/lib/librtlsdr.so.0 ";
 }
 $inc      = join(" ", map { "-I" . $pwd . '/' . $_ . '/'; } @idirs);
 $l_path   = join(" ", map { "-L" . $pwd . '/' . $_; } @ldirs);
 $l_f_path .= join(" ", map { $pwd . '/' . $_; } @lfiles);

 $l_path   .= " -LC:/strawberry/c/bin ";
 if (1) {
  print "===== djdsp Inline::C build parameters ====\n";
  print "  l_path   : \t$l_path\n";
  print "  l_f_path : \t$l_f_path\n";
  print "  inc      : \t$inc\n";
  print "===== ================================ ====\n\n\n";
 } 
};

# The file "top_level.cpp" is not complete. It contains anchor
# placeholders for routines to read out the plain-old-data 
# structs that contain the radio state. The variable below 
# defines two files that describe those structs as well as their
# names and the name of a variable that is local to where in the 
# top_level.cpp file we will try to read the member items into a 
# perl hash
my $structs = {
 "../inc/receiver_stat.h" => {
  type_name => 'receiver_stat_t',
  var_name  => 'lrstat',
 },
 "../dp/dp_findpeaks.h" => {
  type_name => 'peak_pts_t',
  var_name => 'pts',
 },
};

# 1. load all the cpp files above into one mongo file
#    (inline::c doesn't do multiple files)
# 2. find all the variables of interest in a few typedefs
#    of interest
# 3. insert getter code into certain locations to read 
#    those variables.
makeHashConverters($structs);
$all_cpp = loadCPPdata(@cpp_files);
$all_cpp = insertHashConverters($structs,$all_cpp);

use Inline(
 C => Config => 
 LIBS => "$l_path -ldp -lrtlsdr -lpthread",
 CCFLAGS => "-g -O0 -fno-omit-frame-pointer",
 INC => $inc,
 MYEXTLIB => "$l_f_path",
 #ENABLE => "AUTOWRAP",
 CLEAN_AFTER_BUILD => 0,
 OPTIMIZE => '-g -O0',
 #BUILD_NOISY => 1,
);


Inline->bind(C => $all_cpp);


# --------------------------------------------------------
# -- done --
# -- no user serviceable parts below here --




sub findFuncs {
 my $pat = shift;
 my $fn = shift;
 my $fh = undef;
 open($fh,'<',$fn) or die "-could not open top level file $fn";
 my @list = ();
 while (<$fh>) {
  if (/\*?($pat\w+)/) {
   push(@list,$1);
  }
 }
 return @list;
};

sub loadCPPdata {
 my @files = @_;
 my $all_cpp = '';
 foreach my $file (@files) {
  $all_cpp .= "\n\n" . read_file($file);
 }
 return $all_cpp;
};

# routine to scan a header file, find the typedef requested
# and then create "c-to-perl-getters" to put them in a hash
sub makeHashConverters {
 my $files = shift;
 foreach my $file (keys %{$files}) {
  my $tname = $files->{$file}{type_name};
  my $vname = $files->{$file}{var_name};
  $files->{$file}{repl_string} = '';
  my $fh;
  my $in_struct = 0;
  open($fh,'<',$file) or die "could not open $file\n";;
  while (<$fh>) {
   my $il = $_; chomp $il;
   #print "tname $tname\n";
   if ($il =~ /typedef struct\s$tname/) {
    $in_struct = 1;
   } elsif ($il =~ /\}.*$tname/) {
    $in_struct = 0;
   } elsif ($in_struct) {
    if ($il =~ /(\w+)\s+(\w+);/) {
     my $type = $1;
     my $var  = $2;
     my $makesvstr = '';
     if ($type eq 'float') {
      $makesvstr = "newSVnv($vname.$var)";
     } elsif ($type eq 'double') {
      $makesvstr = "newSVnv($vname.$var)";
     } elsif ($type eq 'dp_bool_t') {
      $makesvstr = "newSViv($vname.$var)";
     } elsif ($type =~ /int/) {
      $makesvstr = "newSViv($vname.$var)";
     }
     if (length($makesvstr)) {
      $files->{$file}{repl_string} .=  << "_end_of_repl_here";
      hv_stores(hash,"$var",$makesvstr);
_end_of_repl_here
     }
    }
   }
  }
  close $fh;
 }
};

# This routine actually puts the getters formed above into
# the c file slurped together above
sub insertHashConverters {
 my $files = shift;
 my $cpp_data = shift;
 foreach my $file (keys %{$files}) {
  my $vn = $files->{$file}{var_name};
  my $d  = $files->{$file}{repl_string};
  my $new_cpp_data = '';
  my @cpp_lines = split(/\n/,$cpp_data);
  foreach my $l (@cpp_lines) {
   if ($l =~ /___PERL_INSERT_HASH_COPYING_$vn/) {
     $new_cpp_data .= $d
   } else {
     $new_cpp_data .= $l . "\n";
   }
  }
  $cpp_data = $new_cpp_data;
 }
 return $cpp_data;
};

1;

