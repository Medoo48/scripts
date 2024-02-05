#!/bin/sh

ext="tr.txt"

for arg in "$@"
do
	if [ ! -f "$arg.$ext" ] || [ $(wc -c < "$arg.$ext") -eq 0 ]
	then
		deepl --to uk --timeout=20000 -f $arg > $arg.$ext 2> /dev/null
		if [ $? -eq 0 ]
		then
			echo "done $arg" 
		else
			echo $arg >> fix_list.txt
			echo "need fix $arg"
		fi
	fi
done
echo "finish"
