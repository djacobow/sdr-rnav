#!/usr/bin/perl -w

# An example of how to set up a DSP routine using the 
# dplib library in perl, and also how to move some of 
# that dsp in PDL if desired.
#
#

# Author: David Jacobowitz
#         david.jacobowitz@gmail.com
#
# Date  : Spring, 2014
#
# Copyright 2014, David Jacobowitz

use strict;
use warnings qw(all);

use PDL;
use dplib;
use dp2pdl;
use sl_getter;
use rcontrol;
use Data::Dumper;

my $cfg = {
 blens => {
  radio => 1024 * 32,
 },
 max_baselist => 5,
 rcmds => {
  # the RTL2832 radio has various commands for 
  # configuration. These are aliass for the low-level
  # commands. (Probably should wrap these up sometime.
  GT_FREQ => 2,
  GT_SR =>  4,
  ST_AGC => 5,
  ST_FREQ => 1,
  ST_SR => 3,
  ST_TEST => 8,
  ST_TUNER_GAIN => 7,
  ST_TUNER_GAIN_MODE => 6,
 },
 # the dp_baselist chain will only actually run objects whose
 # runmask has bits in common with the baselists curent runmask.
 # this allows a controlling thread or other logic to control which
 # objects in the DSP chain run in any given iteration. This allows,
 # for example, an FFT to be switched on or off when needed/not needed.
 rmasks => {
  NEVER => 0,
  ALWAYS => 1,
 },
 srates => {
  radio => 250000,
  audio => 8000,
 },
};


# this module is hard to use unless you have a way to share
# data between C land and perl land. With clever typemaps and 
# automatic conversions, this could be elegant. But I could not 
# really figured that out, so I created the dp_pl_sharelist...
# instead. This is a single object that can hold a bag of named
# variables. Any function with a handle to the sharelist itself
# can access any variable by name, and the methods can be called
# from perl or C.
my $sl  = dplib::set_sl(dp_pl_sharelist_create(5));
# ths sharelist getter is just some syntactic sugar to make 
# perl access of the sharelist ever so slight more perly. It
# also caches the indices of commonly used variables names,
# to minimize strcmps.
my $slg = sl_getter->new($sl);
$cfg->{sl} = $sl;
$cfg->{slg} = $slg;

$cfg->{blens}{audio} = int($cfg->{blens}{radio} * $cfg->{srates}{audio} / $cfg->{srates}{radio});

$slg->add_u32('dsp_done',0);
$slg->add_u32('runmask',$cfg->{rmasks}{ALWAYS});

# turn on and configure the radio
my ($radio, $cb_thread) = radio_init($cfg,0);
dp_radio2832_dev_cmd_st_only($radio,$cfg->{rcmds}{ST_FREQ},88500000);
dp_radio2832_dev_cmd_st_only($radio,$cfg->{rcmds}{ST_SR},$cfg->{srates}{radio});
#dp_radio2832_dev_cmd_st_only($radio,$cfg->{rcmds}{ST_TEST},1);

# set up the dsp chain
my $olist               = createDSPchain($cfg,$radio);
# start the dsp thread
my $dsp_thread          = threads->create(\&dsp_thread,$cfg);

# allow the program to run for some time
sleep(10);

# signal to the dsp chain that we want it to stop. Note
# that there is nothing thread-safe about the sharelist, but 
# in this case it's ok, since the dsp_chain only reads this 
# particular variable and we only write it here.
$slg->set_u32('dsp_done',1);

# cleanup and shut down
dp_radio2832_close_device($radio);
$cb_thread->join();
$dsp_thread->join();


# __ the end __


# this function comprises the thread that the DSP chain will
# run in. Any variables that you want to be visible from 
# dsp land you will want to declare inside this function so
# that they can be accessed from the thread -- this includes
# variables that you might want to acccess from the perl 
# callback 
sub dsp_thread {
 my $cfg = shift;
 # I want to access pdl, so we create some pdls here. 
 # note that this one merely references a pointer in 
 # the v_rad_c vector
 my $pdls = {
  aud => pdl_from_vec_nocopy($cfg->{buffers}{v_rad_c},0),
 };
 $cfg->{pdls} = $pdls;

 # this hands execution over to c code that will continue
 # the thread's execution. The c function here, provided by
 # the dplib module will
 # -- prerun all the objects in the "olist"
 # -- run all the objects in the olist, repeatedly, until
 #    the shared variable 'dsp_done' is set by someone. 
 #    The olist running method will, on each iteration,
 #    consult a 'runmask' variable in the global shared 
 #    list. Only objects that have a common bit set in 
 #    their runmask and the current runmask will actually
 #    be called. This allows the dsp chain to turn on and
 #    off different elements at different times
 # -- for each iteration of the olist, a callback to 
 #    perl will also be made, so that you can do perly 
 #    things as part of your dsp loop. The idea is that 
 #    perly things are kept to a minimum here, but it is 
 #    there for when you need it
 # -- postrun all the orjects in the olist after the dsp
 #    loop is complete. (mostly closing files)
 dplib::dsp_thread_fn($cfg,$cfg->{olist}{bl});
};


# the "dsp chain" is a simple ordered list of objects that read
# and write data from arrays. Typically, one object will write to
# an array, and anaother will read it. These objects are dplib 
# objects, implemented in C, but we can build up this chain 
# in perl rather than C. It's not pretty, but it works. The basic
# gist here is:
#
# - create something called a baselist. This is the chain array 
#   object. It holds the array of object points and also has 
#   methods for running the work() and other members of each object
#   in the array
# - create all the buffers you will use. dplib provides the dp_vec_t 
#   buffer for this purpose. These are basically int16_t pointers,
#   with some length and valid info tagged on, along with appropriate
#   methods. Most of the dplib objects don't actually use the 
#   accessor methods because they're slow.
# - add all the objects to the baselist in the order you want them
#   to run. You don't have to configure all the objects before adding
#   them (you do have to create them first, duh) but you will have to 
#   configure them efore starting the dsp_thread_fn above
sub createDSPchain {
 my $cfg   = shift;
 my $radio = shift;

 my $buffers = {
  v_rad_c => dp_vec_create($cfg->{blens}{radio}*2),
  #v_aud_c => dp_vec_create($cfg->{blens}{audio}*2),
  v_aud_c => dp_vec_create($cfg->{blens}{radio}*2/8),
 };

 $cfg->{buffers} = $buffers;

 my $olist = {
  named => {},
  bl    => dp_baselist_create($cfg->{max_baselist}),
 };
 $cfg->{olist} = $olist;

 my $o = undef;
 $o = addObj($olist,'radio0',$radio,$cfg->{rmasks}{ALWAYS});
 dp_radio2832_set_buffers($o,$cfg->{blens}{radio},4);
 dp_set_out($o,$buffers->{v_rad_c}); 

 $o = addObj($olist,'decim0',dp_create_decimate(),$cfg->{rmasks}{ALWAYS});
 my $decim = int($cfg->{srates}{radio} / $cfg->{srates}{audio});
 print "decimation is $decim\n";
 dp_decimate_set_decim($o,$decim);
 dp_set_inlt($o,$buffers->{v_rad_c},$cfg->{blens}{radio},1);
 dp_set_out($o,$buffers->{v_aud_c});

 $o = addObj($olist,'writer0',dp_create_wav_w(),$cfg->{rmasks}{ALWAYS});
 dp_wav_w_set_num_channels($o,2);
 dp_wav_w_set_sample_rate($o,$cfg->{srates}{audio}); 
 dp_wav_w_set_bits_per_sample($o,16);
 dp_set_inl($o,$buffers->{v_aud_c},$cfg->{blens}{audio});
 dp_wav_w_set_fname($o,"output.wav");

 return $olist;
};


# helper function for putting together the dsp chain. Just 
# cuts down on a little typing.
sub addObj {
 my $ol = shift;
 my $n  = shift;
 my $o  = shift;
 my $g  = shift;
 $ol->{named}{$n} = { 
  obj => $o,
 };
 dp_baselist_add($ol->{bl},$o);
 dp_set_name($o,$n);
 dp_set_group($o,$g);
 return $o;
};


# every iteration of the DSP chain includes a callback to a function
# of this name. Here is where you have the ability to run perl as 
# part of the DSP loop. You don't want to do too much as the whole 
# point of this setup is to allow the DSP to be in C, but you obviously
# will need to extract info from the DSP at some point and move it 
# to perl land. Typically, what you might do here is extract some 
# results and then push them into some thread-aware container for 
# passing back to the main thread that will further process the results
# outside of the DSP loop.
sub perl_dsp_after_cb {
 my $cfg = shift;
 print $cfg->{pdls}{aud}->slice("0:10") . "\n";;
};

