#!/usr/bin/env python

from CheckinTest import *


#
# Read in the commandline arguments
#

usageHelp = r"""checkin-test.py [OPTIONS]

This tool does checkin testing for Trilinos with CMake/CTest and can actually
do the checkin itself using eg/git in a safe way.


Quickstart:
-----------

In order to do a solid checkin, perform the following recommended workflow
(other workflows are described below):

1) Review the changes that you have made to make sure it is safe to
commit/push:

  $ cd $TRILINOS_HOME
  $ eg status                                # Look at state of working dir
  $ eg diff --name-status                    # Look at the files have changed
  $ eg log --oneline --name-status origin..  # [optional] Look at the local commits

  NOTE: If you see any files/directories that are listed as 'unknown' returned
  from 'eg status', then you will need to do an 'eg add' to track them or add
  them to the ignore list *before* you run the checkin-test.py script.  The eg
  script will not allow you to commit if there are new 'unknown' files.

2) Create a commit log file in the main source directory:

  $ cd $TRILINOS_HOME
  $ xemacs -nw checkin_message

  NOTE: Fill out this checkin message listing what you have changed.  Please
  use the Trilinos checkin template for this file and try to list a bug number
  in the commit message.

  NOTE: Alternatively, you can just do the local commit yourself with eg/git
  in any way you would like and avoid letting the checkin-test.py script do
  the commit.  That way, you can do as many local commits as you would like
  and organize them any way you would like.

3) Set up the checkin base build directory (first time only):

  $ cd SOME_BASE_DIR
  $ mkdir CHECKIN
  $ cd CHECKIN

  NOTE: You may need to set up some configuration files if CMake can not find
  the right compilers, MPI, and TPLs by default (see below).

  NOTE: You might want to set up a simple shell driver script.  See some
  examples in the files:

    Trilinos/sampmleScripts/checkin-test-*

4) Do the checkin test, (ptional) commit, and push:

  $ cd SOME_BASE_DIR/CHECKIN
  $ $TRILINOS_HOME/cmake/python/checkin-test.py \
      --make-options="-j4" --ctest-options="-j2" --ctest-time-out=180 \
      --commit-msg-header-file=checkin_message \
      --do-all [--commit] --push

  NOTE: The above will: a) (optionally) commit local changes, b) pull updates
  from the global repo, c) automatically enable the correct packages, d) build
  the code, e) run the tests, f) send you emails about what happened, g) do a
  final pull to from the global repo, h) ammend the last local commit with the
  test results, i) and finally push local commits to the global repo if
  everything passes.

  NOTE: You can do the local commit yourself first with eg/git before running
  this script.  In that case, you must take off the --commit argument or the
  script will fail if there are not uncommitted changes.

  NOTE: If you do not specify the --commit argument, you must not have any
  uncommitted changes or the 'eg pull --rebase' command will fail and
  therefore the whole script will fail.

  NOTE: You need to have SSH public/private keys set up to software.sandia.gov
  for the git commits invoked internally to work without you having to type a
  password.

  NOTE: You can do the final push in a second invocation of the script with a
  follow-up run with --push and removing --do-all (it will remember the
  results from the build/test cases just run).  For more details, see below.

  NOTE: Once you start running the checkin-test.py script, you can go off and
  do something else and just check your email to see if all the builds and
  tests passed and if the commit happened or not.

For more details on using this script, see below.


Detailed Documentation:
-----------------------

There are two basic configurations that are tested: MPI_DEBUG and
SERIAL_RELEASE.  Several configure options are varied in these two builds to
try to catch as much conditional configuration behavior has possible.  If
nothing else, please at least do the MPI_DEBUG build since that will cover the
most code and best supports day-to-day development efforts.  However, if you
are changing code that might break the serial build or break non-debug code,
please allow the SERIAL_RELEASE build to be run as well.  Note that the
MPI_DEBUG build actually uses -DCMAKE_BUILD_TYPE=RELEASE with
-DTrilinos_ENABLE_DEBUG=ON to use optimized compiler options but with runtime
debug checking turned on.  This helps to make the tests run faster but still
builds and runs the runtime debug checking code.  Therefore, you should not
use the MPI_DEBUG configure options when building a debug version for yourself
to do debugging.

The following approximate steps are performed by this script:

1) [Optional] Do the local commit (done iff --commit is set)

2) Do a 'eg pull --rebase' to update the code (done if --pull or --do-all is
set.).

3) Select the list of packages to enable forward based on the package
directories where there are changed files (or from a list of packages passed
in by the user).  NOTE: The automatic behavior can be overridden with the
options --enable-packages, --disable-packages, and --no-enable-fwd-packages.

4) For each build case <BUILD_NAME> (e.g. MPI_DEBUG, SERIAL_RELEASE, etc.)

  4.a) Configure a build directory <BUILD_NAME> in a standard way for all of
  the packages that have changed and all of the packages that depend on these
  packages forward. You can manually select which gets enabled (see the enable
  options above).  (done if --configure or --do-all is set.)
  
  4.b) Build all configured code with 'make' (e.g. with -jN set through
  --make-options).  (done if --build or --do-all is set.)
  
  4.c) Run all tests for enabled packages.  (done if --test or --do-all is
  set.)
  
  4.d) Analyze the results of the update, configure, build, and tests and send
  email about results.  (emails only sent out if --send-emails-to is not set
  to ''.)

5) Amend commit message of the most recent commit with the summary of the
testing performed.  (done if --append-test-results is set, the default.)


6) Push the local commits to the global repo.  (done if --push is set.)

The recommended way to use this script is to create a new base directory apart
from your standard build directories such as:

  $ cd SOME_BASE_DIR
  $ mkdir CHECKIN

The most basic way to do the checkin test is:

  $ cd SOME_BASE_DIR/CHECKIN
  $ $TRILINOS_HOME/cmake/python/checkin-test.py --do-all [other options]

If your MPI installation, other compilers, and standard TPLs (i.e. BLAS and
LAPACK) can be found automatically, then this is all you will need to do.
However, if the setup can not be determined automatically, then you can append
a set of CMake variables that will get read in the files:

  SOME_BASE_DIR/CHECKIN/COMMON.config
  SOME_BASE_DIR/CHECKIN/MPI_DEBUG.config
  SOME_BASE_DIR/CHECKIN/SERIAL_RELEASE.config

Actually, skeletons of these files will automatically be written out with
typical CMake cache variables that you would need to set commented out.  Any
CMake cache variables listed in these files will be read into and passed on
the configure line to 'cmake'.

WARNING: Please do not add any CMake cache variables in the *.config files
that will alter what packages are enabled or what tests are run.  The goal of
these configuration files is to allow you to specify the minimum environment
to find MPI, your compilers, and the basic TPLs.  If you need to fudge what
packages are enabled, please use the script arguments --enable-packages,
--disable-packages, --no-enable-fwd-packages, and/or --enable-all-packages.

NOTE: Before running this script, you should first do a 'git status' and 'git
diff --name-status origin/master...master' and examine what files are changed
to make sure you want to commit what you have in your local working directory.
Also, please look out for unknown files that you may need to add to the git
repository with 'eg add' or add to your ignores list.  Your working directory
needs to be 100% ready to commit before running this script.  Alternatively,
you can just do the local commit(s) yourself before running this script.

NOTE: You don't need to run this script if you have not changed any files that
affect the build or the tests.  For example, if all you have changed are
documentation files, then you don't need to run this script before committing
manually.

Common use cases for using this script are as follows:

(*) Basic full testing without push:

   --do-all [--commit --commit-msg-header-file=<SOME_FILE_NAME>]

   NOTE: This will result in a set of emails getting sent to your email
   address for the different configurations and an overall commit readiness
   status email.

   NOTE: If you have any local uncommitted changes you will need to pass in
   --commit and --commit-msg-header-file.

   NOTE: If everything passed, you can follow this up with a --push (see
   below).

(*) Basic full testing with push:

   --do-all --push [--commit --commit-msg-header-file=<SOME_FILE_NAME>]

   NOTE: If the commit criteria is not satisfied, no commit will occur and you
   will get an email telling you that.

   NOTE: If you have any local uncommitted changes you will need to pass in
   --commit and --commit-msg-header-file.

(*) Push to global repo after a completed set of tests have finished:

   --push

   NOTE: This will pick up the results for the last completed test runs and
   append the results of those tests to the checkin-message of the most recent
   commit.

(*) Test only the packages modified and not the forward dependent packages:

  --do-all --no-enable-fwd-packages

  NOTE: This is a safe thing to do when only tests in the modified packages
  are changed and not library code.  This can speed up the testing process and
  is to be preferred over not running this script at all.  It would be very
  hard to make this script automatically determine if only test code has
  changed because every Trilinos package does not follow a set pattern for
  tests and test code.

(*) MPI DEBUG only (run at least this if nothing else):

  --do-all --without-serial-release

(*) The minimum acceptable testing when code has been changed:

  --do-all --enable-all-packages=off --no-enable-fwd-packages \
    --without-serial-release

  NOTE: This will do only an MPI DEBUG build and will only build and run the
  tests for the packages that have directly been changed and not any forward
  packages.

(*) Test only a specific set of packages and no others:

  --do-all --enable-packages=<PACKAGEA>,<PACKAGEB>,<PACKAGEC> \
    --no-enable-fwd-packages
  
  NOTE: This will override all logic in the script about which packages will
  be enabled and only the given packages will be enabled.

  NOTE: Using this option is greatly preferred to not running this script at
  all!

(*) Test changes locally without pulling updates:

  --local-do-all

  NOTE: This will just configure, build, test, and send and email notification
  without updating or changing the status of the local git repo in any way and
  without any communication with the global repo.

  NOTE: This is not a sufficient level of testing in order to commit and push
  the changes to the global repo.  However, this would be a sufficient level
  of testing in order to do a local commit and then pull to a remote machine
  for further testing and commit/push.

(*) Performing a remote test/push:

On your local development machine <mymachine>, do the local test/commit with:

  --local-do-all --no-enable-fwd-packages [--commit --commit-msg-header-file=<???>]

On your remote test machine's CHECKIN directory, do a full test/commit run:

  --extra-pull-from=<mymachine>:/some/dir/to/your/trilinos/src \
    --do-all --push

NOTE: You can of course do the local commit yourself first and avoid the
--commit argument.

NOTE: You can of course adjust the packages and/or build/test cases that get
enabled on the different machines.

NOTE: Once you invoke the checkin-test.py script on the remote test machine,
you can start changing files again on your local development machine and just
check your email to see what happens.

NOTE: If something goes wrong on the remote test machine, you can either work
on fixing the problem there or you can fix the problem on your local
development machine and then do the process over again.

(*) Check commit readiness status:

   [no arguments]

   NOTE: This will examine results for the last testing process and send out
   an email stating if the a commit is ready to perform or not.

(*) See the default option values without doing anything:

  --show-defaults

  NOTE: This is the easiest way to figure out what all of the default options
  are.

Hopefully the above documentation, the documentation of the command-line
arguments below, and some basic experimentation will be enough to get you
going using this script for all of the pre-checkin testing and global commits.

"""

from optparse import OptionParser

clp = OptionParser(usage=usageHelp)
clp.add_option(
  "--trilinos-src-dir", dest="trilinosSrcDir", type="string",
  default='/'.join(getScriptBaseDir().split("/")[0:-2]),
  help="The Trilinos source base directory for code to be tested." )

clp.add_option(
  "--enable-packages", dest="enablePackages", type="string", default="",
  help="List of comma separated Trilinos packages to test changes for" \
  +" (example, 'Teuchos,Epetra').  If this list of packages is empty, then" \
  +" the list of packages to enable will be determined automatically by examining" \
  +" the set of modified files from the version control update log." )

clp.add_option(
  "--disable-packages", dest="disablePackages", type="string", default="",
  help="List of comma separated Trilinos packages to explicitly disable" \
  +" (example, 'Tpetra,NOX').  This list of disables will be appended after" \
  +" all of the listed enables no mater how they are determined (see" \
  +" --enable-packages option).  NOTE: Only use this option to remove packages" \
  +" that will not build for some reason.  You can disable tests that run" \
  +" by using the CTest option -E passed through the --ctest-options argument" \
  +" in this script." )

addOptionParserChoiceOption(
  "--enable-all-packages", "enableAllPackages", ('default', 'on', 'off'), 0,
  "Determine if all Trilinos packages are enabled 'on', or 'off', or let" \
  +" other logic decide 'default'.  Setting to 'off' is appropriate when" \
  +" the logic in this script determines that a global build file has changed" \
  +" but you know that you don't need to rebuild every Trilinos package for" \
  +" a reasonable test.  Setting --enable-packages effectively disables this" \
  +" option.",
  clp )

clp.add_option(
  "--enable-fwd-packages", dest="enableFwdPackages", action="store_true",
  help="Enable forward Trilinos packages. [default]" )
clp.add_option(
  "--no-enable-fwd-packages", dest="enableFwdPackages", action="store_false",
  help="Do not enable forward Trilinos packages.", default=True )

clp.add_option(
  "--extra-cmake-options", dest="extraCmakeOptions", type="string", default="",
  help="Extra options to pass to 'cmake' after all other options." \
  +" This should be used only as a last resort.  To disable packages, instead use" \
  +" --disable-packages." )

clp.add_option(
  "--make-options", dest="makeOptions", type="string", default="",
  help="The options to pass to make (e.g. -j4)." )

clp.add_option(
  "--ctest-options", dest="ctestOptions", type="string", default="",
  help="Extra options to pass to 'ctest' (e.g. -j2)." )

clp.add_option(
  "--ctest-time-out", dest="ctestTimeOut", type="float", default=None,
  help="Time-out (in seconds) for each single 'ctest' test (e.g. 180)." )

clp.add_option(
  "--show-all-tests", dest="showAllTests", action="store_true",
  help="Show all of the tests in the summary email and in the commit message" \
  +" summary (see --append-test-results)." )
clp.add_option(
  "--no-show-all-tests", dest="showAllTests", action="store_false",
  help="Don't show all of the test results in the summary email. [default]",
  default=False )

clp.add_option(
  "--commit-msg-header-file", dest="commitMsgHeaderFile", type="string", default="",
  help="Custom commit message file if committing with --commit." \
  + "  If an relative path is given, this is expected to be with respect to the" \
  +" base source directory for Trilinos.  The very first line of this file should" \
  +" be the summary line that will be used for the commit." )

clp.add_option(
  "--with-mpi-debug", dest="withMpiDebug", action="store_true",
  help="Do the mpi debug build. [default]" )
clp.add_option(
  "--without-mpi-debug", dest="withMpiDebug", action="store_false",
  help="Skip the mpi debug build.", default=True )

clp.add_option(
  "--with-serial-release", dest="withSerialRelease", action="store_true",
  help="Do the serial release build. [default]" )
clp.add_option(
  "--without-serial-release", dest="withSerialRelease", action="store_false",
  help="Skip the serial release build.", default=True )

clp.add_option(
  "--rebuild", dest="rebuild", action="store_true",
  help="Keep an existing build tree and simply rebuild on top of it. [default]" )
clp.add_option(
  "--from-scratch", dest="rebuild", action="store_false", default=True,
  help="Blow existing build directories rebuild from scratch." )

clp.add_option(
  "--send-email-to", dest="sendEmailTo", type="string",
  default=os.environ["USER"]+"@sandia.gov",
  help="List of comma-separated email addresses to send email notification to." \
  +"  By default, this is your Sandia User ID.  In order to turn off email" \
  +" notification, just set --send-email-to='' and no email will be sent." )

clp.add_option(
  "--commit", dest="doCommit", action="store_true",
  help="Do the local commit of all the staged changes using:\n" \
  +" \n" \
  + "   eg commit -a -F <COMMITMSGHEADERFILE>\n" \
  +" \n" \
  +"before the initial pull." \
  +" The commit message used in specified by the --commit-msg-header-file argument." \
  +"  If you have not already committed your changes, then you will want to" \
  +" use this option.  The commit is performed before the initial pull and" \
  +" before any testing is performed in order to allow for rebasing and for" \
  +" allowing the pull to be backed out.  If the build/test fails and --no-force-commit" \
  +" is specified, then the commit will be backed out." \
  +"  Note: By default, unknown files will result in the commit to fail." \
  +"  In this case, you will need to deal with the unknown files in some way" \
  +" or just commit manually and then not pass in this option when running" \
  +" the script again.  WARNING: Committing alone does *not* push changes to" \
  +" the global repo 'origin', you have to use a --push for that." )
clp.add_option(
  "--skip-commit", dest="doCommit", action="store_false", default=False,
  help="Skip the commit at the end. [default]" )

clp.add_option(
  "--force-commit", dest="forceCommit", action="store_true",
  help="Force the local commit to stay even if there are build/test errors." \
  +" WARNING: Only do this when you are 100% certain that the errors are not" \
  +" caused by your code changes.  This only applies when --commit is specified" \
  +" and this script is doing the commit.  When you commit yourself and don't" \
  +" specify --commit (i.e. --no-commit), then the commit will not be backed out" \
  +" and it is up to you to back-out the commit or deal with it in some other way.")
clp.add_option(
  "--no-force-commit", dest="forceCommit", action="store_false", default=False,
  help="Do not force a local commit. [default]" )

clp.add_option(
  "--pull", dest="doPull", action="store_true",
  help="Do the pull from the default (origin) repository and optionally also" \
    +" merge in changes from the repo pointed to by --extra-pull-from.",
  default=False )

clp.add_option(
  "--extra-pull-from", dest="extraPullFrom", type="string", default="",
  help="Optional extra git repository to pull and merge in changes from after" \
    +" pulling in changes from 'origin'." )

clp.add_option(
  "--allow-no-pull", dest="allowNoPull", action="store_true", default=False,
  help="Skipping the pull and not requiring any pull at all to have been performed." \
    +"  This option is useful for testing against local changes without having to" \
    +" get the updates from the global repo.  However, if you don't pull, you can't" \
    +" push your changes to the global repo." )

clp.add_option(
  "--configure", dest="doConfigure", action="store_true",
  help="Do the configure of Trilinos.", default=False )

clp.add_option(
  "--build", dest="doBuild", action="store_true",
  help="Do the build of Trilinos.", default=False )

clp.add_option(
  "--test", dest="doTest", action="store_true",
  help="Do the running of the enabled Trilinos tests.", default=False )

clp.add_option(
  "--do-all", dest="doAll", action="store_true",
  help="Do update, configure, build, and test (same as --pull --configure" \
  +" --build --test)", default=False )

clp.add_option(
  "--local-do-all", dest="localDoAll", action="store_true",
  help="Do configure, build, and test with no pull (same as --allow-no-pull" \
  +" --configure --build --test)", default=False )

clp.add_option(
  "--do-commit-readiness-check", dest="doCommitReadinessCheck", action="store_true",
  help="Check the commit status at the end and send email if not actually" \
  +" committing. [default]" )
clp.add_option(
  "--skip-commit-readiness-check", dest="doCommitReadinessCheck", action="store_false",
  default=True,
  help="Skip commit status check." )

clp.add_option(
  "--append-test-results", dest="appendTestResults", action="store_true",
  help="After the testing is finished, amend the most recent local commit" \
  +" by appending a summary of the test results.  This provides a record of what builds" \
  +" and tests were performed in order to test the local changes.  NOTE: If the same" \
  +" local commit is amended more than once, the prior test summary sections will be" \
  +" overwritten with the most recent test results from the current run. [default]" )
clp.add_option(
  "--no-append-test-results", dest="appendTestResults", action="store_false",
  default=True,
  help="Do not amend the last local commit with test results.  NOTE: If you have" \
  +" uncommitted local changes that you do not want this script to commit then you" \
  +" must select this option to avoid this last amending commit." )

clp.add_option(
  "--push", dest="doPush", action="store_true",
  help="Push the committed changes in the local repo into to global repo" \
    +" 'origin'.  Note: If you have uncommitted changes this command will fail." \
    +"  You would usually use the --commit option as well to do these together." \
    +"  Note: You must have SSH public/private keys set up with" \
    +" the origin machine (e.g. software.sandia.gov) for the push to happen without" \
    +" having to type your password." )
clp.add_option(
  "--skip-push", dest="doPush", action="store_false", default=False,
  help="Skip the push at the end. [default]" )

clp.add_option(
  "--show-defaults", dest="showDefaults", action="store_true",
  help="Show the default option values and do nothing at all.",
  default=False )

(options, args) = clp.parse_args()

# NOTE: Above, in the pairs of boolean options, the *last* add_option(...) 
# takes effect!  That is why the commands are ordered the way they are!

if options.doCommit and not options.commitMsgHeaderFile:
  print "\nError, if you specify --commit you must also set --commit-msg-header-file!\n"

#
# Echo the commandline
#

print ""
print "**************************************************************************"
print "Script: checkin-test.py \\"
print "  --trilinos-src-dir='"+options.trilinosSrcDir+"' \\"
print "  --enable-packages='"+options.enablePackages+"' \\"
print "  --disable-packages='"+options.disablePackages+"' \\"
print "  --enable-all-packages='"+options.enableAllPackages+"'\\"
if options.enableFwdPackages:
  print "  --enable-fwd-packages \\"
else:
  print "  --no-enable-fwd-packages \\"
print "  --extra-cmake-options='"+options.extraCmakeOptions+"' \\"
print "  --make-options='"+options.makeOptions+"' \\"
print "  --ctest-options='"+options.ctestOptions+"' \\"
if options.ctestTimeOut:
  print "  --ctest-time-out="+str(options.ctestTimeOut)+" \\"
if options.showAllTests:
  print "  --show-all-tests \\"
else:
  print "  --no-show-all-tests \\"
print "  --commit-msg-header-file='"+options.commitMsgHeaderFile+"' \\"
if options.withMpiDebug:
  print "  --with-mpi-debug \\"
else:
  print "  --without-mpi-debug \\" 
if options.withSerialRelease:
  print "  --with-serial-release \\"
else:
  print "  --without-serial-release \\" 
if options.rebuild:
  print "  --rebuild \\"
else:
  print "  --from-scratch \\"
print "  --send-email-to='"+options.sendEmailTo+"' \\"
if options.doCommit:
  print "  --commit \\"
else:
  print "  --skip-commit \\"
if options.forceCommit:
  print "  --force-commit \\"
else:
  print "  --no-force-commit \\"
if options.doPull:
  print "  --pull \\"
if options.extraPullFrom:
  print "  --extra-pull-from='"+options.extraPullFrom+"' \\"
if options.allowNoPull:
  print "  --allow-no-pull \\"
if options.doConfigure:
  print "  --configure \\"
if options.doBuild:
  print "  --build \\"
if options.doTest:
  print "  --test \\"
if options.doAll:
  print "  --do-all \\"
if options.localDoAll:
  print "  --local-do-all \\"
if options.doCommitReadinessCheck:
  print "  --do-commit-readiness-check \\"
else:
  print "  --skip-commit-readiness-check \\"
if options.appendTestResults:
  print "  --append-test-results \\"
else:
  print "  --no-append-test-results \\"
if options.doPush:
  print "  --push \\"
else:
  print "  --skip-push \\"


#
# Execute
#

import time

if not options.showDefaults:

  print "\nStarting time:", getCmndOutput("date",True)
  
  t1 = time.time()
  success = checkinTest(options)
  t2 = time.time()
  print "\nTotal time for checkin-test.py =", (t2-t1)/60.0, "minutes"
  
  print "\nFinal time:", getCmndOutput("date",True)
  
  if success:
    print "\nOVERALL: PASSED\n"
    sys.exit(0)
  else:
    print "\nOVERALL: FAILED\n"
    sys.exit(1)
