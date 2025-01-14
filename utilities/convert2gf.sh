#!/bin/bash

GIF="$1"
#ALT_HOLD="$2"
MFRAMETIME="$2"
BG="rgb(0,0,0)"
ALT_BG="$3"
MINFT=4


TMPDIR=$(mktemp -d 2>/dev/null || mktemp -d -t 'gifconvert')

# Pull full frames from gif
# if we resize at this step frame animation doesn't work correctly
convert "$GIF" -coalesce $TMPDIR/%d.bmp
SIZE="$(identify -ping -format '%w x %h\n' "$GIF" | head -n 1)"
if [[ "$SIZE" != "16 x 16" ]]; then
  echo "$GIF: Incorrect Size ($SIZE)"
  #convert "$GIF" -resize 16x16\! -coalesce -type truecolor $TMPDIR/%d.bmp
  #exit 1
fi
WIDTH=$(echo $SIZE | awk '{print $1}')
HEIGHT=$(echo $SIZE | awk '{print $3}')

echo '#######' $(identify -format "%T " "$GIF")
frametime=($(identify -format "%T " "$GIF"))

if [ ! -z "$MFRAMETIME" ]; then
  for I in ${!frametime[@]}; do
    echo -n "Increasing frame $I time from ${frametime[$I]} to "
    if [ ${frametime[$I]} -le 0 ]; then
      [ -z "$MFRAMETIME" ] && MFRAMETIME=40
      frametime[$I]=$MFRAMETIME
    else
      frametime[$I]=$((${frametime[$I]}*$MFRAMETIME))
    fi
    echo ${frametime[$I]}
  done
fi

base_ft=100
base_ft_orig=$base_ft

cs=0
ds=0
try_higher=1
d=
while true; do
  for I in ${!frametime[@]}; do
    [ ! -z "$d" ] && echo cs: $((${frametime[$I]}%${base_ft}+${cs})) = ${frametime[$I])} '%' ${base_ft} '+' ${cs} 
    cs=$((${frametime[$I]}%${base_ft}+${cs}))
    [ ! -z "$d" ] && echo ds: $((${frametime[$I]}/${base_ft}+${ds})) = ${frametime[$I])} '/' ${base_ft} '+' ${ds}
    ds=$((${frametime[$I]}/${base_ft}+${ds}))
    [ ${base_ft} -eq $base_ft_orig ] && echo -n "${frametime[$I]}, "
  done
  [ ${base_ft} -eq $base_ft_orig ] && echo -e '\b\b'

  if [ $ds -eq 0 ] || [ $cs -eq 0 ] && [ $ds -gt 1 ] && [ $try_higher -eq 1 ]; then
    [ $cs -eq 0 ] && [ $ds -ne 1 ] && [ $try_higher -eq 1 ] && ds=0 && try_higher=0
    ft_max=${frametime[0]}
    for N in "${frametime[@]}" ; do
      ((N > ft_max)) && ft_max=$N
    done
    [ ! -z "$d" ] && echo ft_max: $ft_max

    if [ $ft_max -gt $MINFT ]; then
      [ ! -z "$d" ] && echo gt $MINFT

      # prevent reseting to a value we've already checked
      [ $ft_max -lt $base_ft_orig ] && continue
      base_ft=$ft_max
      echo Reset base frame time to $base_ft
      cs=0
      continue
    else
      base_ft=$MINFT
      echo Reset base frame time to miniumum: $base_ft
      cs=0
      break
    fi
  fi

  echo Frame time of ${base_ft}cs has a frame remainder of ${cs}cs
  if [ $cs -gt 0 ]; then
    if [ $base_ft -gt $MINFT ]; then
#      echo Shrinking frame time by 1cs
      base_ft=$(($base_ft-1))
    else
      echo At minimum frame time
      break
    fi
  fi
  [ $cs -ne 0 ] || break
  cs=0
done

# create copies of frames based on base_ft
fc=0
for frame in ${!frametime[@]}; do
  copies=$((${frametime[$frame]}/$base_ft))
  echo $GIF: Frame $frame should display ${frametime[$frame]}cs and needs $copies copies at a frame time of ${base_ft}cs

  for frame_copy in $(seq 0 $(($copies-1))); do
    if [ $frame_copy -lt 0 ]; then continue; fi
     # consider -scale or -sample instead of adaptive-resize
     # if one dimension is larger and one small -- there could be issues with a resize
    # if [[ check if width and height are >= 16 ]]; then
    #   # art is too large, scale down
    #   convert -sample 16x16 -gravity center -extent 16x16 -background "${ALT_BG:-$BG}" $TMPDIR/${frame}.bmp -layers flatten -type truecolor ./$((${frame_copy}+${fc})).bmp
    # else
    #   # art is too small, scale up
    #   if [[ test if dimensions are 2 or 4 or 8 (e.g. dividd into 16) ]]; then
    #   convert -scale 16x16 -gravity center -extent 16x16 -background "${ALT_BG:-$BG}" $TMPDIR/${frame}.bmp -layers flatten -type truecolor ./$((${frame_copy}+${fc})).bmp
    # else
    #   # art is too small dimensions don't scale well, pad
    #   convert -size 16x16 -gravity center -extent 16x16 -background "${ALT_BG:-$BG}" $TMPDIR/${frame}.bmp -layers flatten -type truecolor ./$((${frame_copy}+${fc})).bmp
    #   fi
    # fi
     # default action is to sample down to 16x16 -- smaller art can get fuzzy. Use one of the above scale up commands instead
     convert -sample 16x16 -gravity center -extent 16x16 -background "${ALT_BG:-$BG}" $TMPDIR/${frame}.bmp -layers flatten -type truecolor ./$((${frame_copy}+${fc})).bmp
  done
  if [ $frame_copy -lt 0 ]; then frame_copy=0; fi
  fc=$((${fc}+${frame_copy}+1))
done

# For use with ALT_HOLD variable. Replace hold = input below
# hold = ${ALT_HOLD:-$((${base_ft}*10))}

echo Writing out new config.ini
cat << EOF > config.ini
# All of these settings are optional.
# If this file doesn't exist, or if
# settings don't appear in this file,
# Game Frame will use default values.

[animation]

# milliseconds to hold each frame
# (1000 = 1 sec; lower is faster)
hold = $((${base_ft}*10))

# should the animation loop? If false,
# system will progress to next folder
# after the last frame is displayed.
loop = true

[translate]

# move the animation across the screen
# this many pixels per frame. Experiment
# with positive and negative numbers for
# different directions.
moveX = 0
moveY = false

# should the movement loop?
loop = true

# optionally dictate the next animation
# EXAMPLE: nextFolder = mspacman
# nextFolder = nextfolder
EOF

ls -l
