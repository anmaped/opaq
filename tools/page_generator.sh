#!/bin/bash

echo "Converting files in folder \"websites\" to C Header file websites.h"

WEBSITE="process/"
CURRDIR="$(pwd)"
OUTFILE="$CURRDIR/websites.h"

#change into website folder
cd $WEBSITE

cat > $OUTFILE <<DELIMITER
//
// convert files into flash variables
//

DELIMITER

#macros
INDEX=0
for i in $(ls -1); do
  imod=${i//./_}
  echo "#define FILE_$imod $INDEX" >> $OUTFILE
  INDEX=$((INDEX+1))
done
echo "#define N_FILES $INDEX" >> $OUTFILE
echo >> $OUTFILE

#convert contents into array of bytes
INDEX=0
for i in $(ls -1); do
  CONTENT=$(cat $i | xxd -i)
  printf "static const char file_$INDEX[] PROGMEM = { \n$CONTENT\n };\n" >> $OUTFILE
  echo >> $OUTFILE
  INDEX=$((INDEX+1))
done

# write typedefinition
cat >> $OUTFILE <<DELIMITER
struct t_websitefiles {
  const char* filename;
  const char* mime;
  const uint16_t len;
  const __FlashStringHelper* content;
} files[] = {
DELIMITER

# add other data and create array
INDEX=0
for i in $(ls -1); do
  CONTENT=$(cat $i | xxd -i)
  CONTENT_LEN=$(echo $CONTENT | grep -o '0x' | wc -l)  
  MIMETYPE=$(file --mime-type -b $i)
  
  echo "  {" >> $OUTFILE
  echo "    .filename = \"/$i\"," >> $OUTFILE
  echo "    .mime = \"$MIMETYPE\"," >> $OUTFILE
  echo "    .len = $CONTENT_LEN," >> $OUTFILE
  echo "    .content = FPSTR(&file_$INDEX[0])" >> $OUTFILE
  echo "  }," >> $OUTFILE

  INDEX=$((INDEX+1))
done
echo "};" >> $OUTFILE

cd $CURRDIR

