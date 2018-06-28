#! /bin/sh
#
# Extracts all the dynamic libraries required by an executable to another directory.
# The intention is that you can then add just that directory and the executable to
# a docker image to get very small images.
#
# All of the libraries are copied to the output directory so the existing system is
# not modified in any way. The directory structure in the output matches that on the
# original system, so that the original executable's rpath doesn't need to be changed.
#
# Usage is :
#     extractDynamicLibs.sh <executable file> [output directory]
# where
#    <executable file> (required)  - the executable file to get all the dynamic
#                                    libraries for.
#    [output directory] (optional) - the directory to put all the dynamic libraries
#                                    in. Defaults to "libraries".
#
# To add the output to a docker container, you should just ADD the directory to the
# root path, i.e. have this command in your Dockerfile:
#    ADD libraries /
#
# Requires ldd, readlink, mkdir, cp, cd, ln, awk, basename, dirname.
#
# @author Mark Grimes
# @date 18/Feb/2016
# @copyright MIT Licence (https://opensource.org/licenses/MIT)
#


if [ $# -gt 2 ]; then
	echo "Warning: all but the first two parameters were ignored" >&2
	EXECUTABLE_NAME=$1
	OUTPUT_DIRECTORY=$2	
elif [ $# -gt 1 ]; then
	EXECUTABLE_NAME=$1
	OUTPUT_DIRECTORY=$2	
elif [ $# -gt 0 ]; then
	EXECUTABLE_NAME=$1
	OUTPUT_DIRECTORY="libraries"
	echo "Output location not specified so trying \"$OUTPUT_DIRECTORY\""
else
	echo "No executable was specified"
	exit
fi

if [ ! -f $EXECUTABLE_NAME ]; then
	echo "The provided executable name \"$EXECUTABLE_NAME\" does not exist" >&2
	exit
fi

if [ -e "$OUTPUT_DIRECTORY" ]; then
	echo "The requested output directory \"$OUTPUT_DIRECTORY\" already exists, delete it before continuing" >&2
	exit
fi

REQUIRED_LIBS=`ldd "$EXECUTABLE_NAME" 2>&1`
if [ $? -ne 0 ]; then
	echo "ldd encountered the error :" >&2
	echo "    "$REQUIRED_LIBS  >&2 # Used "2>&1" so any error message will be in the variable
	echo "when run on \"$EXECUTABLE_NAME\"" >&2
	exit
fi

mkdir -p "$OUTPUT_DIRECTORY"
if [ $? -ne 0 ]; then
	echo "Unable to create the directory \"$OUTPUT_DIRECTORY\" for the output" >&2
	exit
fi



IFS=$'\n'

START_DIR=$PWD

for ITEM in $REQUIRED_LIBS; do
	ORIGINAL=`echo $ITEM | awk '{print $1}'`
	ACTUAL=`echo $ITEM | awk '{if(NF==4) print "readlink -f "$3}' | sh`

	if [ -n "$ACTUAL" ]; then
		ACTUAL_BASENAME=`basename "$ACTUAL"`
		ACTUAL_DIRNAME=`dirname "$ACTUAL"`
		FULL_OUTPUT_DIRECTORY="$OUTPUT_DIRECTORY/$ACTUAL_DIRNAME"
		#FULL_OUTPUT_DIRECTORY="$OUTPUT_DIRECTORY"
		
		if [ ! -d "$FULL_OUTPUT_DIRECTORY" ]; then
			mkdir -p "$FULL_OUTPUT_DIRECTORY"
		fi
		cp "$ACTUAL" "$FULL_OUTPUT_DIRECTORY"
		if [ "$ORIGINAL" != "$ACTUAL_BASENAME" ]; then
			cd "$FULL_OUTPUT_DIRECTORY"
			ln -s "$ACTUAL_BASENAME" "$ORIGINAL"
			cd "$START_DIR"
		fi
	elif [ -f "$ORIGINAL" ]; then
		ORIGINAL_DIRNAME=`dirname "$ORIGINAL"`
		FULL_OUTPUT_DIRECTORY="$OUTPUT_DIRECTORY/$ORIGINAL_DIRNAME"
		#FULL_OUTPUT_DIRECTORY="$OUTPUT_DIRECTORY"

		if [ ! -d "$FULL_OUTPUT_DIRECTORY" ]; then
			mkdir -p "$FULL_OUTPUT_DIRECTORY"
		fi
		cp "$ORIGINAL" "$FULL_OUTPUT_DIRECTORY"
	fi
done
