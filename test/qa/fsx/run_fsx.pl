#!/usr/bin/perl -w
#
# Basic fs tests
#

use strict;
our $fslist;

sub usage {
	print STDERR "usage: ${0} [\"optional parameters\"]\n";
}

my $PARMS = shift;
$PARMS = "-N 10000" if ((! defined $PARMS) || ($PARMS eq ""));

my $conf= "kvfs.cfg";
if (! -f $conf) {
	print STDERR "'${conf}' does not exist\n";
	exit 1;
}

require $conf;
if (!defined $fslist) {
	print STDERR "'${conf}' does not conatin \$fslist definition\n";
	exit 1;
}
# Create a target directory for each file system
my $targdirlist = [];
foreach my $f (@{$fslist}) {
	my $targdir =  $f->{NAME};
	if (! -d $targdir) {
		print STDERR "'${targdir} is not a directory\n";
		exit 1;
	} 
	push @$targdirlist , $targdir; 

}

# get current directory
my $cwd = `pwd`;
chomp $cwd;
print "cwd:${cwd}\n";


# fsx
my @worked;
my @failed;
chdir $cwd;
print "make fsx\n";
if (system("make fsx")) {
	print "failed: make fsx in $cwd directory. \n";
	exit 1;
}	
	
foreach my $targdir (@{$targdirlist}) {
	print "---running fsx $PARMS on ${targdir}\n";
	if (! -f "${targdir}/fsx.file") {
		if (system("rm -rf ${targdir}/fsx.file")) {
			push @failed , "rm -rf ${targdir}/fsx.file";
			next;
		}
	}		
	if (system("./fsx $PARMS ${targdir}/fsx.file")) {
				push @failed, "run fsx on ${targdir}";
	} else {
		push @worked, "fsx on ${targdir}";
	}
}


print "worked: @worked\n" if (@worked);
print "failed: @failed\n" if (@failed);

exit (@failed);

