# Author: David Jacobowitz
#         david.jacobowitz@gmail.com
#
# Date  : Spring 2013
#
# Copyright 2013, David Jacobowitz

# Certain simple scalar data and pointers are based between the
# perl world and the c world, and between threads, through a 
# simple data 'holder' struct called dp_pl_sharelist_t. It has
# routines, both C and perl callable, that let a named scalar be
# added to a simple list of scalars and accessed accordingly to 
# an index.
#
# This file contains wrapper routines for the perl side of things
# to make access a bit more natural. Also, it makes a copy of the 
# variable indices in a perl hash, which should be much faster than
# the cheesy linear search currently found in dp_pl_sharelist.c

package sl_getter;

use strict;
use warnings qw(all);
use dplib;
use Data::Dumper;


sub new {
 my $class = shift;
 my $sl    = shift;

 my $self = {
  sl => $sl, 
  cache => {},
 };
 bless $self, $class;
 return $self;
};

sub add_u32 {
 my $s = shift;
 my $n = shift;
 my $v = shift;
 #print "n $n v $v\n";
 my $idx = dp_pl_sharelist_add_u32($s->{sl},$n,$v);
 $s->{cache}{$n} = $idx;
}

sub add_i32 {
 my $s = shift;
 my $n = shift;
 my $v = shift;
 my $idx = dp_pl_sharelist_add_i32($s->{sl},$n,$v);
 $s->{cache}{$n} = $idx;
}

sub get_u32 {
 my $s = shift;
 my $n  = shift;
 my $idx = -1;
 if (defined($s->{cache}{$n})) {
  $idx = $s->{cache}{$n};
 } else {
  $idx = dp_pl_sharelist_get_idx($s->{sl},$n);
  $s->{cache}{$n} = $idx;
 }
 if ($idx >= 0) {
  return dp_pl_sharelist_get_u32($s->{sl},$idx);
 } else {
  return undef;
 }
};

sub get_vp {
 my $s = shift;
 my $n = shift;
 my $idx = -1;
 if (defined($s->{cache}{$n})) {
  $idx = $s->{cache}{$n};
 } else {
  $idx = dp_pl_sharelist_get_idx($s->{sl},$n);
  $s->{cache}{$n} = $idx;
 }
 if ($idx >= 0) {
  return dp_pl_sharelist_get_vp($s->{sl},$idx);
 } else {
  return 0;
 }
};


sub get_cp {
 my $s = shift;
 my $n = shift;
 my $idx = -1;
 if (defined($s->{cache}{$n})) {
  $idx = $s->{cache}{$n};
 } else {
  $idx = dp_pl_sharelist_get_idx($s->{sl},$n);
  $s->{cache}{$n} = $idx;
 }
 if ($idx >= 0) {
  return dp_pl_sharelist_get_cp($s->{sl},$idx);
 } else {
  return 0;
 }
};

sub get_i32 {
 my $s = shift;
 my $n  = shift;
 my $idx = -1;
 if (defined($s->{cache}{$n})) {
  $idx = $s->{cache}{$n};
 } else {
  $idx = dp_pl_sharelist_get_idx($s->{sl},$n);
  $s->{cache}{$n} = $idx;
 }
 if ($idx >= 0) {
  return dp_pl_sharelist_get_i32($s->{sl},$idx);
 } else {
  return undef;
 }
};

sub get_fpf {
 my $s = shift;
 my $n  = shift;
 my $idx = -1;
 if (defined($s->{cache}{$n})) {
  $idx = $s->{cache}{$n};
 } else {
  $idx = dp_pl_sharelist_get_idx($s->{sl},$n);
  $s->{cache}{$n} = $idx;
 }
 if ($idx >= 0) {
  return dp_pl_sharelist_get_deref_fp($s->{sl},$idx);
 } else {
  return undef;
 }
};

sub set_u32 {
 my $s  = shift;
 my $n  = shift;
 my $v  = shift;
 my $idx = -1;
 if (defined($s->{cache}{$n})) {
  $idx = $s->{cache}{$n};
 } else {
  $idx = dp_pl_sharelist_get_idx($s->{sl},$n);
  $s->{cache}{$n} = $idx;
 }
 if ($idx >= 0) {
  dp_pl_sharelist_set_u32($s->{sl},$idx,$v);
 } else {
  print"-pw- no variable in sharelist named $n\n";
 }
};

sub set_i32 {
 my $s  = shift;
 my $n  = shift;
 my $v  = shift;
 my $idx = -1;
 if (defined($s->{cache}{$n})) {
  $idx = $s->{cache}{$n};
 } else {
  $idx = dp_pl_sharelist_get_idx($s->{sl},$n);
  $s->{cache}{$n} = $idx;
 }
 if ($idx >= 0) {
  dp_pl_sharelist_set_i32($s->{sl},$idx,$v);
 } else {
  print"-pw- no variable in sharelist named $n\n";
 }
};



1;

