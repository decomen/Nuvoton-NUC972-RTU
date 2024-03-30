#!/bin/sh

make clean
fstr=`echo $1 | cut -d \. -f 1`
sstr=`echo $1 | cut -d \. -f 2`
vstr=""
if [ "$fstr"x = "0"x ]; then
  vstr=`grep '#define DAS_VER_VERCODE ' ./board/board.h | grep '('$sstr')'`
else
  vstr=`grep '#define DAS_VER_VERCODE ' ./board/board.h | grep '('$fstr$sstr')'`
fi

if [ "$fstr"x = "0"x ]; then
  if [ "$vstr"x = ""x ]; then
    sed -i 's/\(.*\)DAS_VER_VERCODE\(.*\)(\(.*\))\(.*\)/\1DAS_VER_VERCODE\2('$sstr')\4/g' ./board/board.h
  fi
else
  if [ "$vstr"x = ""x ]; then
    sed -i 's/\(.*\)DAS_VER_VERCODE\(.*\)(\(.*\))\(.*\)/\1DAS_VER_VERCODE\2('$fstr$sstr')\4/g' ./board/board.h
  fi
fi

make
mvstr="das_`date +%Y%m%d.0`.$1"
cp ./das /var/www/html/ota/ -a

echo "dopack:$1 $mvstr" >> ./pack.log

