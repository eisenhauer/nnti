#!/usr/bin/perl -w
# test-harness.plx

################################################################################
# The Trilinos Project - Test Harness
#
# Jim Willenbring, Mike Phenow, Ken Stanley
#
# About...
#
# Usage...
################################################################################

use strict;

use Getopt::Std;    # for command line options parsing
use lib "lib";      # ended up needing to use a relative address - don't need?

# Variable Declarations ========================================================

# Options variables
my %flags;
my %options;

my $homedir;
my $frequency;
my $date; 
my $cvscmd;
my $basehostfile; 
my $hostfile; 

# Host variables
my $hostos;
my $machinename;
my $ARCH; 

# Error variables
my $errorcount; 
my $warningcount; 
my $updateerror; 
my $trilinosCompileError; 
my $testCompileError; 

# File variables        
my $ERRORS;
my $WARNINGS;
my $BODY;

# Compile variables   
my $comm;
my $compileFail;
my $configscript;

# Test variables
my @lines;
my $line;
my @alltestdirs;
my $testdirectory;
my @potentialscripts;
my $potentialscript;

my @subdirs;
my $numsubdirs;
my $mpi;
my $basesubdirfile;
my $subdirfile;

################################################################################
# Execution ####################################################################
################################################################################

# Preparation ==================================================================

# Get command line options
getopts("gsh", \%options);
if ($options{h}) { printHelp(); }
if ($options{g}) { genConfigTemp($options{s}); }

# Parse test-harness-config
# %options = parseConfig();

# Initialize options 
initOptions();

# Determine host OS
getHostOs();

# Prepare output files
prepFiles();

# Update Trilinos from CVS Repository
cvsUpdate();

# Determine which build directories to use for tests
parseSubdirList();

# Boot LAM if any tests are parallel
#   (If mpich is used, comment out this section and the lamhalt section)
lamboot();

# Find all 'test' directories that may contain scripts that need to be executed
findTestDirs();

# Main Execution ===============================================================

# Build and test from bottom up, recording and reporting results as necessary

# Compile the required Trilinos builds for tests determined above
compile();

# Run tests
# test();   # currently called by compile()

# Clean Up =====================================================================

# Close filehandles and delete temp files
cleanupFiles();

# Halt LAM if any tests are parallel
#   (If mpich is used, comment out this section and the lamboot section)
lamhalt();

################################################################################
# Subroutines ##################################################################
################################################################################

    ############################################################################
    # initOptions()
    #
    # Initialize basic test-harness options
    #   - global variables used: yes
    #   - sends mail: no
    #   - args: 
    #   - returns: 

    sub initOptions {    
        $homedir="$ENV{'HOME'}/Trilinos";
        $frequency="daily"; #default value
        chomp($date=`date`);
        if ($date =~/Sun/) {
          $frequency="weekly";
        }
        
        # Set proper location of cvscmd if necessary, otherwise leave it at "cvs"
        $cvscmd="cvs";
        
        # (LAM only) name of the file containing the names of the machines to be used for parallel jobs
        # If this file doesn't exist, parallel jobs will be run on the local machine only
        $basehostfile='hostfile'; # change filename here if necessary
        # Location of $basehostfile
        $hostfile="$homedir/$basehostfile"; # change path here if necessary
        
        # permissions needed?
        #my $permissions=0775;
    } # initOptions()

    ############################################################################
    # getHostOs()
    #
    # Discovers the host's operating system.
    #   - global variables used: yes
    #   - sends mail: no
    #   - args: 
    #   - returns: 

    sub getHostOs {
        chomp ($hostos=`uname -a`);
        chomp ($machinename=`uname -n`);
        chomp ($ARCH=`uname`); 
            # $ARCH must match the middle portion of the name of the log file 
            # written to by the testAll file - for example on a Linux platform, 
            # the log file is called logLinux.txt, so $ARCH=Linux
            
        SWITCH: {
            # List of current supported platforms: AIX, CPLANT, DEC, 
            # IBMSP, LINUX, SGI32, SGI64, SMOS, SOLARIS, TFLOP, sun
            # If MIME::Lite is used in the end, include the file somewhere in Trilinos
            if ($hostos=~/^Linux/) {
                use lib "lib"; #ended up needing to use a relative address
                last SWITCH; };
            if ($hostos=~/^SunOS/) {
                use lib "/local/homes/jmwille/lib";
                last SWITCH; };
            if ($hostos=~/^CYGWIN/) {
                MIME::Lite->send('smtp',"mailgate.sandia.gov"); #doesn't have sendmail
                use lib "/home/jmwille/lib";
                last SWITCH; };
            # Fix the rest of the switch statement, currently functional for LINUX only
            # if ($hostos=~/^Linux.+cluster/||$hostos=~/^Linux.+node/) {$TRILINOS_ARCH='linux-lam-cluster'; last SWITCH; };
            # if ($hostos=~/^Linux.+cluster/) {$TRILINOS_ARCH='linux-lam-cluster'; last SWITCH; };
            die "hostos does not match any of the OS choices.\n";
        }
    } # getHostOs()

    ############################################################################
    # prepFiles()
    #
    # Prepares the output files.
    #   - global variables used: yes
    #   - sends mail: no
    #   - args: 
    #   - returns: 

    sub prepFiles {
        chdir"$homedir";
    
        # Doesn't count the actual number of errors, but rather
        # the number of times ERRORS is written to.
        $errorcount=0; 
        
        # Analogous to errorcount
        $warningcount=0; 
        
        # If an error occurs during the cvs update, this 
        # indicates that updatelog.txt should be attached
        $updateerror=0; 
        
        # If an error occurs during the compilation of 
        # Trilinos, this indicates that trilinosCompileLog.txt
        # should be attached        
        $trilinosCompileError=0; 
        
        # If an error occurs during the compilation of one of
        # the tests, this indicates that testCompileLog.txt
        # should be attached        
        $testCompileError=0; 
        
        open (ERRORS, ">>errors.txt") or die "$! error trying to open file";        
        open (WARNINGS, ">>warnings.txt") or die "$! error trying to open file";        
        # EmailBody file serves as the body of the email, 
        open (BODY, ">>EmailBody.txt") or die "$! error trying to open file";
    } # prepFiles()

    ############################################################################
    # cvsUpdate()
    #
    # Updates Trilinos from the CVS repository.
    #   - global variables used: yes
    #   - sends mail: on error
    #   - args: 
    #   - returns: 

    sub cvsUpdate {    
        chdir"$homedir";
        
        my $result;
        print "$cvscmd update -dP > $homedir/updatelog.txt 2>&1\n";
        $result=system "$cvscmd update -dP > $homedir/updatelog.txt 2>&1";
        if ($result) {
            ++$errorcount;
            ++$updateerror;
            print ERRORS "*** Error updating Trilinos ***\n";
            print ERRORS "Aborting tests.\n";
            print ERRORS "See updatelog.txt for further information";
            ################################# &sendemail; # COMMENTING OUT EMAIL
            die " *** ERROR updating TRILINOS! ***\n";
        }
    } # cvsUpdate()

    ############################################################################
    # parseSubdirList()
    #
    # Determines which build directories are to be used in the test process
    # based on the subdir-list file.
    #   - global variables used: yes
    #   - sends mail: no
    #   - args: 
    #   - returns: 

    sub parseSubdirList {   
        chdir"$homedir";
         
        $numsubdirs=0;
        $mpi=0;
        $basesubdirfile='subdir-list'; 
        $subdirfile="$homedir/testharness/$basesubdirfile";
        
        if (! -f $subdirfile) {
            ++$warningcount;
            print WARNINGS "WARNING: $subdirfile does not exist.\n";
            &useDefaultParameters;
        } else {
            open SUBDIRECTORIES, "$subdirfile";
            chomp(@lines=<SUBDIRECTORIES>);
            close SUBDIRECTORIES;
            foreach $line (@lines) {
                # ignore comment lines (comment lines start with a pound sign)
                unless ($line=~/^\s*#/ || $line=~/^\s+$/ || $line=~/^$/) {
            	    print "$line\n";
            	    $subdirs[$numsubdirs]=$line;
            	    ++$numsubdirs;
            	    chdir "$homedir/$line";
            	    print "$homedir/$line\n";
            	    open (SCRIPT, "invoke-configure") or die "$! error trying to open file";
            	    my $commcheck=<SCRIPT>;
            	    close SCRIPT;
            	    # Check if the configure script indicates a parallel or serial build
            	    if ($commcheck=~/--enable-mpi/i) {
            	        ++$mpi;
            	    }
            	} # unless
            } # foreach
            if (! $numsubdirs) {
                ++$warningcount;
                print WARNINGS "WARNING: No valid parameter sets found in $subdirfile\n";
                &useDefaultParameters;
            }
        } # else
    } # parseSubdirList()

    ############################################################################
    # lamboot()
    #
    # Check if any tests are parallel tests--if so, do a lamboot.
    #   - global variables used: yes
    #   - sends mail: no
    #   - args: 
    #   - returns: 

    sub lamboot {    
        chdir"$homedir";
        
        if ($mpi) {
            unless (! -f $hostfile) {
                system "lamboot $hostfile -v";
            } else {
                system "lamboot"; ## Could need to be another MPI implementation
            }
        }
    } # lamboot()

    ############################################################################
    # findTestDirs()
    #
    # Find all 'test' directories that may contain scripts that need to be executed
    #   - global variables used: yes
    #   - sends mail: no
    #   - args: 
    #   - returns: 

    sub findTestDirs {    
        chdir"$homedir";
        
        system "find packages/ -name test -print > list_of_test_dirs";
        ### May need to append to list_of_test_dirs to account for test
        ### directories in contrib or other directories not in packages/
    } # findTestDirs()

    ############################################################################
    # compile()
    #
    # Compile the required Trilinos builds for tests determined above
    #   - global variables used: yes
    #   - sends mail: no
    #   - args: 
    #   - returns: 

    sub compile {    
        $comm="serial";
        $compileFail=0;
        for (my $i=0; $i<$numsubdirs; $i++) {
            chdir"$homedir/$subdirs[$i]";
            open (SCRIPT, "invoke-configure") or die "$! error trying to open file\n$subdirs[$i]/invoke-configure\n";
            $configscript=<SCRIPT>;
            close SCRIPT;
            if ($configscript =~/--enable-mpi/i) {
        	    $comm="mpi";
        	} else {
        	  $comm="serial";
        	}
        	system "make clean";
            #Note that the 'invoke-configure' script must be executable
        	$compileFail+=system "./invoke-configure >> $homedir/trilinosCompileLog.txt 2>&1";
            if ($compileFail) {
                # The configure process failed, skip associated tests, report the
        	    # failure
                ++$errorcount;
                ++$trilinosCompileError;
                print ERRORS "Trilinos configure process failed for the";
                print ERRORS "$subdirs[$i] test.\nWill skip associated tests.\n";
                # &sendemail; # COMMENTING OUT EMAIL
                $compileFail=0; # reset so the positive value doesn't trigger
        					    # an error again
            } else {
        	    $compileFail+=system "make >> $homedir/trilinosCompileLog.txt 2>&1";
        		if ($compileFail) {
        	  	    # The build process failed, skip associated tests, report the failure
        	  		++$errorcount;
        	  		++$trilinosCompileError;
        	  		print ERRORS "Trilinos compilation process failed for the $subdirs[$i] test.\nWill skip associated tests.\n";
        	  		# &sendemail; # COMMENTING OUT EMAIL
        	  		$compileFail=0; # reset so the positive value doesn't trigger an error again
        		} else {
        	        test($i);
        	    } # else
            } # else
        } # for (numsubdirs)
    } # compile()
    
    ############################################################################
    # test()
    #
    # Run tests
    #   - global variables used: yes
    #   - sends mail: no
    #   - args: 
    #   - returns: 

    sub test {    
        my $i = $_[0];
        
	    open TESTDIRECTORIES, "list_of_test_dirs";
	    chomp(@alltestdirs=<TESTDIRECTORIES>);
	    close TESTDIRECTORIES;
	  
	    foreach $testdirectory (@alltestdirs) {
	        # for packages that have not been ported to a particular platform,
	        # the tests associated with such a package could be skipped below
	        # use unless (uname =.. && $testdirectory=..) to skip
	        # Packages excluded below (jpetra, tpetra, etc) do not yet build
	        # with autotools
	        unless ($testdirectory=~/^\s+$/ || $testdirectory=~/^$/ || $testdirectory=~/tpetra/ || $testdirectory=~/jpetra/ ) {
	            chdir "$homedir/$subdirs[$i]/$testdirectory";

	            $compileFail=0;
	            system "find $homedir/$subdirs[$i]/$testdirectory/scripts/$frequency/$comm -type f > list_of_files";
	            open FILELIST, "list_of_files";
	            chomp(@potentialscripts=<FILELIST>);
	            close FILELIST;
	            foreach $potentialscript (@potentialscripts) {
                    # Ken Stanley added the following to the checkin-test-harness.  JW copied it here also.  We shouldn't really need
                    # it, but if a script fails to return to the directory it started in, this will fix the problem.
	                chdir "$homedir/$subdirs[$i]/$testdirectory";
		            if (-x $potentialscript ) {
		                $compileFail+=system "$potentialscript $subdirs[$i] True >> $homedir/testCompileLog.txt 2>&1";
		                if ($compileFail) {
		                    ++$errorcount;
		                    ++$testCompileError;
		                    print ERRORS "Trilinos test suite failed for $subdirs[$i] tests.\n\n";
		                }
		                #
		                # Create and send an email
		                #
		                system "rm -f list_of_files";
		                # &sendemail; # COMMENTING OUT EMAIL
		                $compileFail=0; # reset so the positive value doesn't trigger an error again
		            } # if
	            } # foreach
	        } # unless
	    } #foreach
    } # test()

    ############################################################################
    # cleanupFiles()
    #
    # Close filehandles and delete temp files
    #   - global variables used: yes
    #   - sends mail: no
    #   - args: 
    #   - returns: 

    sub cleanupFiles {    
        chdir"$homedir";
        
        close BODY;
        close ERRORS;
        close WARNINGS;
        system "rm -f list_of_test_dirs";
    } # cleanupFiles()
    
    ############################################################################
    # lamhalt()
    #
    # Check if any tests were parallel tests--if so, do a lamhalt.
    #   - global variables used: yes
    #   - sends mail: no
    #   - args: 
    #   - returns: 

    sub lamhalt {    
        chdir"$homedir";
        
        if ($mpi) {
	        system "lamhalt"; # Could need to be another MPI implementation 
        }
    } # lamhalt()
    
    ############################################################################
    # sendMail()
    #
    # Takes an array of addresses, a scalar body, and an array of attachments
    # and sends mail to all of the addresses 
    #   - global variables used: no
    #   - sends mail: on success
    #   - args: Array addresses, String body, Array attachments
    #   - returns: 

    sub sendMail {
        chdir"$homedir";
        
        #my @addresses = $_[0];
        #my $body = $_[1];
        #my @attachments = $_[2];
        
        ##### use MIME::Lite;
        
        # This subroutine is called when it is time to send the email - either
        # when the tests are complete or when an error occurs from which the
        # script cannot recover
        
        print "I'M SENDING EMAIL TO SOMEBODY!"; # Warn me that not all calls to SENDEMAIL were commented out

        # The following line constructs the name of the log that is produced 
        # during tests whether errors occurred or not.  (-v not used.)
        my $testfile2=join("",'log',$ARCH,'.txt');
        my $scriptowner="";
        if ( -f $testfile2) {
            system "mv $homedir/$testfile2 EmailBody.txt";
            open (OWNER, "EmailBody.txt") or die "$! error trying to open file";
            $scriptowner=<OWNER>;
            close OWNER;
        }
        # List addresses for all emails to be sent to below
        chomp $scriptowner;
        my $mail_to;
        # A valid email address must contain a '@'
        if ($scriptowner =~/@/i) {
            $mail_to=join ", ", 'jmwille@sandia.gov',$scriptowner or die "$! error forming To: field";
        } else {
            # List where emails without a script owner should be sent
            # By sending them to the trilinos-regresstion list, we assure that
            # results from tests without script owners and cvs and compilation 
            # errors are recorded in Mailman

            $mail_to=join ", ", 'Trilinos-regression@software.sandia.gov';
        }

        print "\n**$mail_to**\n";

        my $testfile="$homedir/logErrors.txt";
        my $testfile1="$homedir/logMpiErrors.txt";
        
        #construct and send message using MIME-Lite
        open (SUMM, ">>summ.txt") or die "$! error trying to open file";
        if ($errorcount || -f $testfile ||  -f $testfile1 || $warningcount || $updateerror || $testCompileError || $trilinosCompileError) {

            # At least one problem occurred throughout the course of testing

            my $subject_line=join " - ", $ARCH, $machinename,'At least one error occurred';
            if ( $updateerror ) {
                $subject_line=join " - ", $ARCH, $machinename,'CVS update error';
            } elsif ( $trilinosCompileError ) {
                $subject_line=join " - ", $ARCH, $machinename, $comm,'Trilinos compile error';
            } else {
                $subject_line=join " - ", $ARCH, $machinename, $comm,'At least one test failed';
            }
    
            my $msg=MIME::Lite->new(
    			From =>'trilinos-regression@software.sandia.gov',
     			# To =>'jmwille@sandia.gov',
    			To => $mail_to,
    			Subject => $subject_line,
    			Type =>'multipart/mixed'
    			);
            $msg->attach(Type=>'TEXT',
    	        Path=>'summ.txt',
    	        Disposition=>'inline'
    	        );
    	        
            if ($potentialscript) {
                print SUMM "Test script: ";
                print SUMM $potentialscript;
                print SUMM "\n";
            }
        
            if ($frequency =~/weekly/) {
                print SUMM "Results of weekly test:\n";
            } elsif ($frequency =~/daily/) {
                print SUMM "Results of daily test:\n";
            } else {
                print SUMM "Warning: Unknown test frequency, results below:\n";
            }
  
            if ( -f $testfile1) {
                print SUMM "--> Parallel testing did not complete successfully.\n";

                if ($testCompileError) {
                    print SUMM "This failure is probably due to the compile time errors listed in the attachment \"testCompileLog.txt\".\n";
                    print SUMM "If that does not appear to be the case, ";
                }

                print SUMM "See attachment \"logMpiErrors.txt\".\n\n";
                $msg->attach(Type=>'TEXT',
    		        Path=>'logMpiErrors.txt',
    		        Disposition=>'attachment'
    		        );
            }

            if ( -f $testfile) {
                print SUMM "--> Serial testing did not complete successfully.\n";

                if ($testCompileError) {
                    print SUMM "This failure is probably due to the compile time errors listed in the attachment \"testCompileLog.txt\".\n";
                    print SUMM "If that does not appear to be the case, ";
                }
                print SUMM "See attachment \"logErrors.txt\".\n\n";
                $msg->attach(Type=>'TEXT',
    		        Path=>'logErrors.txt',
    		        Disposition =>'attachment'
    		        );
            }

            if ($testCompileError && -f "testCompileLog.txt") {
                $msg->attach(Type=>'TEXT',
                    Path=>'testCompileLog.txt',
                    Disposition=>'attachment'
                    );
            }

            if ($errorcount && -f "errors.txt") {
                print SUMM "--> For additional information about errors that occured,\n";
                print SUMM "See attachment \"errors.txt\".\n\n";
                $msg->attach(Type=>'TEXT',
    		        Path=>'errors.txt',
    		        Disposition =>'attachment'
    		        );
            }
        
            if ($warningcount && -f "warnings.txt") {
                print SUMM "--> At least one non-testing warning occurred (i.e. script/paramter related).\n";
                print SUMM "See attachment \"warnings.txt\".\n\n";
                $msg->attach(Type=>'TEXT',
    		        Path=>'warnings.txt',
    		        Disposition =>'attachment'
    		        );
            }
  
            if ($updateerror && -f "updatelog.txt") {
                print SUMM "--> At least one error occurred during CVS update.\n";
                print SUMM "See attachment \"updatelog.txt\".\n\n";
                $msg->attach(Type=>'TEXT',
    		        Path=>'updatelog.txt',
    		        Disposition=>'attachment'
    		        );
            }
  
            if ($trilinosCompileError && -f "trilinosCompileLog.txt") {
                print SUMM "--> At least one error occurred while compiling Trilinos.\n";
                print SUMM "See attachment \"trilinosCompileLog.txt\".\n\n";
                $msg->attach(Type=>'TEXT',
    		        Path=>'trilinosCompileLog.txt',
    		        Disposition=>'attachment'
    		        );
            }
        
            print SUMM "************************************************\n";
            print SUMM "The following is the output from the test script listed above (or failed compile attempt).\n";
            print SUMM "Please note that the -v option was not selected for this log.\n";
            print SUMM "NOTE:Depending on your Mail User Agent (MUA), the test summary will either appear below, or it will be attached as a file called \"EmailBody.txt\".\n";
            print SUMM "See any attachments listed above for more details.\n";
            print SUMM "*************************************************\n";
        
            if (-f "EmailBody.txt") {
                $msg->attach(Type =>'TEXT',
		            Path=>'EmailBody.txt',
		            Disposition=>'inline'
		            );
            }
        
            $msg->send;
            system "rm -f logErrors.txt logMpiErrors.txt";

        } else {
            # No problems occurred of any kind
            my $subject_line=join " - ", $ARCH, $machinename, $comm,'All tests passed';
            my $msg=MIME::Lite->new(
			    From =>'trilinos-regression@software.sandia.gov',
			    To => $mail_to,
			    Subject => $subject_line,
			    Type =>'multipart/mixed'
			    );
            $msg->attach(Type=>'TEXT',
	            Path=>'summ.txt',
	            Disposition=>'inline'
	            );
	     
            if ($potentialscript) {
                print SUMM "Test script: ";
                print SUMM $potentialscript;
                print SUMM "\n";
            }
    
            if ($frequency =~/weekly/) {
                print SUMM "\nResults of weekly tests:\n";
            } elsif ($frequency =~/daily/) {
                print SUMM "\nResults of daily tests:\n";
            } else {
                print SUMM "\nWarning: Unknown test frequency, results below:\n";
            }

            print SUMM "*****************************************************\n";
            print SUMM "The following is the output from the test script listed above.\n";
            print SUMM "Please note that the -v option was not selected for this log.\n";
            print SUMM "While no errors occurred during this test, this log can still be examined to see which tests were run.\n";
            print SUMM "NOTE:Depending on your Mail User Agent (MUA), the test summary will either appear below, or it will be attached as a file called 'EmailBody.txt'\n";
            print SUMM "******************************************************";

            $msg->attach(Type =>'TEXT',
	            Path=>'EmailBody.txt',
		        Disposition=>'inline'
	            );
            $msg->send;
        }

        close SUMM;    
        
        # Reset error/warning indicators so that errors are not triggered 
        # for any remaining tests
        $errorcount=0;
        $warningcount=0;
        $updateerror=0;
        $trilinosCompileError=0;
        $testCompileError=0;
        
        system "rm -f errors.txt warnings.txt updatelog.txt EmailBody.txt";
        system "rm -f trilinosCompileLog.txt testCompileLog.txt summ.txt";    
        
    } # sendMail()
    
    ############################################################################
    # useDefaultParameters()
    #
    # This subroutine is called when script is forced to try default parameters
    #   - global variables used: no
    #   - sends mail: on success
    #   - args: Array addresses, String body, Array attachments
    #   - returns: 

    sub useDefaultParameters {
        chdir"$homedir";
        
        ++$warningcount;
          print WARNINGS "WARNING: trying default parameters.\n";
          ## maybe should have defaults for each platform some day
          $subdirs[0]='SERIAL';
          $subdirs[1]='MPI';
          ++$mpi;
          $numsubdirs=2;
          print WARNINGS "Default subdirectories $subdirs[0] $subdirs[1]\n";
        
    } # useDefaultParameters()

    ############################################################################
    # printUsage()
    #
    # Prints Test-Harness usage to standart output and exits.
    #   - global variables used: no
    #   - sends mail: no
    #   - args: 
    #   - returns: 

    sub printHelp {
        print "Test-Harness\n";
        print "\n";
        print "options:\n";
        print "  -g : generate template configuration file and exit\n";
        print "  -s : omit comments from generated configuration file\n";
        print "  -h : print this help page and exit\n";
        exit;
    } # printHelp()

    ############################################################################
    # parseConfig()
    #
    # Parses test-harness-config and returns hash of arrays of the form
    # ({VARIABLE_A, [valueA1, valueA2, ...]}, {VARIABLE_B, [valueB1, ...]})
    #   - global variables used: no
    #   - sends mail: no
    #   - args: 
    #   - returns: Hash of Arrays options

    sub parseConfig {
        my %options; # Hash of Arrays
        my $inFile;
        my $line;
        my $name;
        my $value;
        
        open (inFile, "< test-harness-config")
            or die "can't open test-harness-config";
            
        while ($line = <inFile>) {
            chomp($line);
            
            if ($line =~ m/^[# ]/) {
                # skip comments and blank lines
            } else {
                $name =~ m/^\w+/;
                # parse values and push them into $name's array
                # must allow for += for appending
                # must allow for " " for values with spaces
                # must allow for \ for line continuations
            }
        }
        
        return %options;        
    } # parseConfig()
    
    ############################################################################
    # genConfigTemp()
    #
    # Generates Test-Harness template config file named in current directory
    # named "test-harness-config" and exits.
    #   - global variables used: no
    #   - sends mail: no
    #   - args: boolean isShort;
    #   - returns: 

    sub genConfigTemp {
        my $short = $_[0];
        my $outFile;
                
        open (outFile, "> test-harness-config-template")
            or die "can't open test-harness-config-template";
        
        print outFile "# Test-Harness\n";        
        print outFile "\n";

        if (!$short) {
            print outFile "# This file describes the settings to be used by the test harness\n";
            print outFile "#\n";
            print outFile "# All text after a hash (#) is considered a comment and will be ignored\n";
            print outFile "# The format is:\n";
            print outFile "#       TAG = value [value, ...]\n";
            print outFile "# For lists items can also be appended using:\n";
            print outFile "#       TAG += value [value, ...]\n";
            print outFile "# Values that contain spaces should be placed between quotes (\" \")\n";
            print outFile "# Line continuation characters \\ are allowed\n";
            print outFile "\n";
        }
        
        print outFile "#-------------------------------------------------------------------------------\n";
        print outFile "# General configuration options\n";
        print outFile "#-------------------------------------------------------------------------------\n";
        print outFile "\n";
        
        if (!$short) {        
            print outFile "# List the names of all of the build directories that should be configured,\n";
            print outFile "# compiled and tested for the test harness.  Each build directory must be a\n"; 
            print outFile "# subdirectory of 'Trilinos/'.  One subdirectory per line.  Each directory\n"; 
            print outFile "# listed must contain an \"invoke-configure\" file that contains the options\n";
            print outFile "# that should be passed to configure.\n";
            print outFile "\n";
        }
        
        print outFile "SUBDIR_LIST            = \n";
        
        if (!$short) {        
            print outFile "\n";
            print outFile "# List the email addresses to which summaries and error messages will be sent\n";
            print outFile "# by default (i.e. when no script owner exists for a particular test).\n";
            print outFile "\n";
        }
        
        print outFile "DEFAULT_EMAILS         = \n";

        print outFile "\n";        
        print outFile "# etc...\n";
        print outFile "\n";
        
        close outFile;
        exit;
    } # genConfigTemp()