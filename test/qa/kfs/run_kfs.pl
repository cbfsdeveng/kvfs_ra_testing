#!/usr/bin/perl -w
#
# kfs fs tests
#

use strict;
our $fslist;

sub usage {
	print STDERR "usage: ${0} [\"optional parameters\"]\n";
}

my $PARMS = shift;
$PARMS = "-Vm -t 120 -p 1000" if ((! defined $PARMS) || ($PARMS eq ""));

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


# kfs
my @worked;
my @failed;
chdir $cwd;
print "make kfs\n";
if (system("make kfs")) {
	print "failed: make kfs in $cwd directory. \n";
	exit 1;
}	
	
foreach my $targdir (@{$targdirlist}) {
	print "---running kfs $PARMS on ${targdir}\n";
	if (-d "${targdir}/kfs" ) {
		if (system("rm -rf ${targdir}/kfs")) {
			push @failed, "kfs on ${targdir}";
			next;
		}
	}		
		
	if (system("mkdir ${targdir}/kfs") ||
    		system("./kfs $PARMS ${targdir}/kfs") ||
    		system("rm -rf ${targdir}/kfs")) {
		push @failed, "kfs on ${targdir}";
	} 
	else {
		push @worked, "kfs on ${targdir}";
	}

}  

print "worked: @worked\n" if (@worked);
print "failed: @failed\n" if (@failed);

exit (@failed);

