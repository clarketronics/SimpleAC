#!/bin/bash

#Upload variabls
UPLOAD=false
CLEAR=false
SERIALDEVICE=""

#Build variables
EXPORT=false
EXPORTBUILD=""

#Board variable
FQBN="SparkFun:avr:promicro"

#Various other variables
VERBOSE=false
VERBOSEOUTPUT=""
DIR=""

# Functions 
helpMessage(){
  printf "Usage:"
  printf "    build -c                     Clear build data after upload.\n"
  printf "    build -e                     Export compiled binaries to build folder.\n"
  printf "    build -b <FQBN>              Set board type as a fully qualified board name (rather than default).\n"
  printf "    build -h                     Display this help message.\n"
  printf "    build -u <Serial port>       Upload to the specified serial port.\n"
  printf "    build -v                     Verbose output.\n"  
  exit 0
}

compile(){
  arduino-cli compile -b $FQBN $DIR $VERBOSEOUTPUT $EXPORTBUILD 
}

clean(){
    rm -rf $PWD/build/$DIR
}

upload(){
  arduino-cli upload -b $FQBN -p $SERIALDEVICE $VERBOSEOUTPUT
}

getdependancies(){
  arduino-cli lib install LinkedList DFRobotDFPlayerMini MFRC522
  arduino-cli lib install --git-url https://github.com/clarketronics/PN532
}

# Argument parsing
while getopts ":b:cehu:v" opt; do
  case ${opt} in
    c)
      CLEAR=true
      DIR="${FQBN//:/.}"
      ;;
    e)
      EXPORT=true
      EXPORTBUILD="-e"
      ;;
    b)
      FQBN=$OPTARG
      ;;
    h)
      helpMessage
      ;;
    u)
      UPLOAD=true
      SERIALDEVICE=$OPTARG
      ;;
    v)
      VERBOSE=true
      VERBOSEOUTPUT="-v"
      ;;
    :)                                 
      echo "Error: -${OPTARG} requires an argument."
      exit 1
      ;;
    \?)
      echo "Invalid Option: -$OPTARG" 1>&2
      exit 1
      ;;
  esac
done

# Print options to confirm
printf "\n"
printf -- " -------------------Options----------------------\n"
printf -- " Board type: %s \n" $FQBN 
printf -- " Upload to board: %s \n" $UPLOAD 

if [ $UPLOAD == "true" ]; then
  printf -- " Upload port: %s \n" $SERIALDEVICE
fi

printf -- " Verbose: %s \n" $VERBOSE
printf -- " Tidy up: %s \n" $CLEAR

if [ $CLEAR == "true" ]; then
  printf -- " Dir to clean: %s \n" $DIR
fi

# Build excetra
printf "\n\n"
echo "Continue with these inputs?"
select response in Yes No; do
  case $response in
    Yes)
      getdependancies

      compile

      if [ $UPLOAD == "true" ]; then
        upload
        if [ $CLEAR == "true" ]; then
          clean
        fi
      fi
      
      exit 0
      ;;
    No)
      exit 0
      ;;
  esac
done







