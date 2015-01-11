#!/usr/bin/perl 
# wgrep.pl - windowed grep utility

use strict;
use IO::File;
use IO::Handle;

my ($before,$after,$show_stars,$show_nums,$sep,$show_fname);
my ($show_sep,$arg,$file,$regexp,$lnum,$fhandle,$nbef,$naft);
my ($matched,$matched2,@line_buf,@temp,$fh,$ret);

$before = 3; $after = 3;         # default window size 
$show_stars = 0; 
$show_nums = 0; 
$sep = "**********\n"; 
$show_fname = 1; 
$show_sep = 1;

# loop until an argument doesn't begin with a "-" 
while ($ARGV[0] =~ /^-(\w)(.*)/) { 
   $arg = $1;                    # $arg holds the option letter
if ($arg eq "s") { $show_stars = 1; } 
elsif ($arg eq "n") { $show_nums = 1; } 
elsif ($arg eq "m") { $show_fname = 0; } 
elsif ($arg eq "d") { $show_sep = 0; } 
elsif ($arg eq "w") {
   # parse 2nd matched section at colon
   @temp=split(/:/,$2);	
   $before = $temp[0] if $temp[0] ne ''; 
   $after = $temp[1] if $temp[1] ne ''; 
   } 
elsif ($arg eq "p") { 
   $before = 0; 
   $after = 0; 
   $show_sep = 0; } 
elsif ($arg eq "W") { 
   $before = 0; 
   $after = 0; 
   }
elsif ($arg eq "h") { &usage(""); } 
else { &usage("wgrep: invalid option: $ARGV[0]"); 
   }                             # end of if command 
shift;                           # go on to next argument 
}                                # end of foreach loop
&usage("wgrep: missing regular expression") if ! $ARGV[0]; 
$regexp = $ARGV[0]; 
shift; 
$regexp =~ s,/,\\/,g;            # "/" --> "\/"


# if no files are specified, use standard input 
if (! @ARGV[0]) { @ARGV[0] = "STDIN"; }

LOOP: 
foreach $file (@ARGV) {          # loop over file list 
   if ($file eq "STDIN") {
      $fh=new IO::Handle;
      $ret=$fh->fdopen(fileno(STDIN),"r");
      die "Can't open STDIN." unless $ret;
      }
   else {
      $fh = new IO::File "$file", "r";
      if (! defined $fh) {
         print STDERR "Can't open file $file; skipping it.\n"; 
         next LOOP;              # jump to LOOP label
         }
      } 
$lnum = 0; 
$nbef = 0; $naft = 0; 
$matched = 0; $matched2 = 0; 
&clear_buf(0) if $before > 0;
while (<$fh>) {                  # loop over the lines in the file 
   ++$lnum;                      # increment line number 
   if ($matched) {               # we're printing the match window
      if ($_ =~ /$regexp/) {     # if current line matches pattern: 
         $naft = 0;              #   reset the after window count,
         &print_info(1);         #   print preliminary stuff,
         print $_;               #   and print the line
         }
      else {                     # current line does not match
         if ($after > 0 && ++$naft <= $after) {
            # print line anyway if still in the after window
            &print_info(0); print $_;	
            }
         else {                  # after window is done
            $matched = 0;        # no longer in a match
            $naft = 0;           # reset the after window count
            # save line in before buffer for future matches
            push(@line_buf, $_); $nbef++;
            }                    # end else not in after window
         }                       # end else curr. line not a match
      }                          # end if we're in a match

   else {                        # we're still looking for a match 
      if ($_ =~ /$regexp/) {     # we found one
         $matched = 1;           # so set match flag
         # print file and/or section separator(s)
         print $sep if $matched2 && $nbef > $before && $show_sep && $show_fname;
         print "********** $file **********\n" if ! $matched2++ && $show_fname;
         # print and clear out before buffer and reset before counter
         &clear_buf(1) if $before > 0; $nbef = 0;
         &print_info(1);
         print $_;               # print current line
         } 
      elsif ($before > 0) {
         # pop off oldest line in before buffer & add current line
         shift(@line_buf) if $nbef >= $before;
         push(@line_buf,$_); $nbef++;
         }                       # end elseif before window is nonzero 
      }                          # end else not in a match 

   }                             # end while loop over lines in this file 
   $fh->close;
}                                # end foreach loop over list of files
exit;                            # end of script proper

# subroutines #
sub print_info { 
   print $_[0] ? "* " : "  " if $show_stars; 
   printf "%4d ", $lnum if $show_nums;
}                                # end subroutine print_info

sub clear_buf {
   # argument says whether to print before window or not 
   my ($i,$j,$print_flag);
   $print_flag = $_[0]; 
   $i = 0; $j = 0; 
   if ($print_flag) {
      # if we're printing line numbers, fiddle with the 
      # counter to account for the before window 
      if ($show_nums) {
         $lnum -= ($#line_buf + 1);
         }
      while ($j <= $#line_buf) { # print before window
         &print_info(0);
         print $line_buf[$j++];
         $lnum++ if $show_nums;
         }                       # end while 
      }                          # end if print_flag 
   @line_buf = ();               # clear line_buf array 
}                                # end subroutine clear_buf

sub usage {
   # optional argument is an additional message line
   print STDERR $_[0],"\n" if $_[0];
   print STDERR "Usage: wgrep [-n] [-w[B][:A] | -W] ";
   print STDERR "[-d] [-p] [-s] [-m] regexp file(s)\n";
   print STDERR "       -n = number lines\n";
   print STDERR "       -s = mark matched lines with asterisks\n";
   print STDERR "       -wB:A = display B lines before and A lines after\n";
   print STDERR "               each matched line [both default to 3]\n";
   print STDERR "       -W = suppress window; equivalent to -w0:0\n";
   print STDERR "       -d = suppress separation lines between sections\n";
   print STDERR "       -m = suppress file name header lines\n";
   print STDERR "       -p = plain mode: equivalent to -W -d\n";
   print STDERR "       -h = print this help message and exit\n";
   print STDERR "Note: If present, -h prevails; otherwise, the rightmost\n";
   print STDERR "      option wins in the case of contradictions.\n";
   exit;
   }                             # end subroutine usage
