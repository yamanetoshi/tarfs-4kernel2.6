#!/bin/sh

# comp.sh dir1 dir2

{
for i in `(cd $1;find . -type f)`
do
    if ! diff $1/$i $2/$i >& /dev/null
    then
	echo "File $1/$i and $2/$i not match!"
    fi
done
}
