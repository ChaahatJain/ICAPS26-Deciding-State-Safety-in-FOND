#!/bin/sh
curdir=$pwd
mydir="${0%/*}"

echo $mydir
cd $mydir

if [ ! -d $mydir/.venv ]
then
export PIPENV_VENV_IN_PROJECT=1 # store env locally
export SYSTEM_VERSION_COMPAT=1 # for MacOS
rm -f Pipfile.lock
pipenv install
fi
