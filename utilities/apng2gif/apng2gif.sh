#!/bin/bash

PNG="$1"
PNG_LOC="$(readlink -f "${PNG}")"

UTIL="$(readlink -f "$(dirname -- "$(readlink -f "${BASH_SOURCE}")")/..")"
# Usage: apngdis anim.png [name]
APNGDIS="${UTIL}/apngdis/apngdis"
TMPDIR=$(mktemp -d 2>/dev/null || mktemp -d -t 'gifconvert')

cp "$PNG" "$TMPDIR"/
pushd "$TMPDIR"
$APNGDIS "$PNG" tmp
# $ cat tmp1.txt 
# delay=133/1000

# ex. command to build: convert -loop 0 -dispose previous -delay 13 apngframe1.png -delay 13 apngframe2.png test.gif

COMMAND="convert -loop 0 -dispose previous"
for FRAME in tmp*.png; do
  DELAY_FILE=$(echo $FRAME | sed 's#.png$#.txt#')
  [ -r "$DELAY_FILE" ] && source $DELAY_FILE && delay=$(($(echo $delay|sed 's#00$##')))
  echo Delay from frame $FRAME: $delay
  [ $delay -le 0 ] && delay=1

  COMMAND="$COMMAND -delay $delay $IMG"
  delay=-1
done

COMMAND="$COMMAND output.gif"

$COMMAND

cp -v output.gif "$(echo $PNG_LOC|sed 's#.png$#.gif#')"
#viu -1 "$(echo $PNG_LOC|sed 's#.png$#.gif#')"
