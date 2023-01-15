Archive of art on LedSeq.com

Shell script included to convert 16x16 GIFs into BMPs under utilities directory

Convert list.txt into links for current directory:
$ while read LINK; do ln -vnsf ../archive/$LINK "$(echo $LINK | tr '/' '-')"; done < list.txt

Clean up current directory symlinks:
$ find . -maxdepth 1 -type l -delete

Copy over current artwork
$ cp -prvH * /Volumes/NO\ NAME/

Debug tips:
1. Color depth should be 24 bits, if it is 32 turn off alpha channel. With ImageMagick:
  $ convert 0.bmp -alpha off -depth 24 0-new.bmp; file 0-new.bmp; mv 0-new.bmp 0.bmp 

2. GameFrame will not search non-numeric nested directories. If you copy over art make sure the
   animation directory is at the top level and not nested.
  2a. GameFrame will search sub-directories named 0, 1, 2, etc in order

3. Leading 0s can be stripped with something like:
  $ for I in {0..9}; do mv ./$(printf '%02d.bmp' $I) ./$I.bmp; done # 1 leading 0
  $ for I in {0..99}; do mv ./$(printf '%03d.bmp' $I) ./$I.bmp; done # 22leading 0

4. Art directories on Game Frame can not start with 0 or they will not load

5. SD Card needs to be formatted FAT32 to handle long filenames
