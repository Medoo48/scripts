#!/bin/sh

ext="tr.txt"

for arg in "$@"
do
	if [[ $arg == *.$ext ]]; then continue; fi
	
	if [ ! -f "$arg.$ext" ] || [ $(wc -c < "$arg.$ext") -eq 0 ]
	then
		if [ $(stat -c %s "$arg") -gt 1500 ]; then
			echo "file [$arg] too long, skip, 1500 chars max for DeepL Translator. Use: fix -n 1500 -T file"
			echo $arg >> fix_list.txt
			continue
		fi

		deepl --to uk --timeout=20000 -f $arg > $arg.$ext 2> /dev/null
		if [ $? -eq 0 ]
		then
			echo "done $arg" 
		else
			echo $arg >> fix_list.txt
			echo "need fix $arg"
		fi
		continue
	fi

done
echo "finish"
