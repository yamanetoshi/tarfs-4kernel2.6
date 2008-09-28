#!/bin/sh

# test.sh test_dir

if echo $0 | grep '^/' > /dev/null
then
    prog=$0
else
    prog="$PWD/$0"
fi

echo "Changing directory to $1";
cd $1;
ents=`echo *`
for i in $ents
do
    if [ -d $i ]
    then
	echo "Down to $i"
	$prog $1/$i
    fi
    if [ -f $i ]
    then
	echo "Accessing $i"
	cat $i > /dev/null
    fi
done
