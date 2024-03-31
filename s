#!/bin/sh

done=0
done_total=0
failed=0
count=0
delay=5
delay_after=120
timeout=60000
repeat=0

ext="tr.txt"

args=()

while [[ $# -gt 0 ]]; do
  case $1 in
    -c|--count)
      count="$2"
      shift # past argument
      shift # past value
      ;;
    -s|--sleep)
      delay="$2"
      shift # past argument
      shift # past value
      ;;
    -S|--sleep-after)
      delay_after="$2"
      shift # past argument
      shift # past value
      ;;
    -t|--timeout)
      timeout=$(($2 * 1000))
      shift # past argument
      shift # past value
      ;;
    -r|--repeat)
      repeat=1
      if [ $count -eq 0 ]; then
	count=2
      fi
      shift # past argument
      ;;
    -h|--help)
      echo -e "usege: s [-rh] [-c count] [-s delay] [-S dalay_after] [-t timeout] files
\t-c, --count\tПерекласти вказану кількість файлів. З -r скрипт перекладатиме вказаними порціями
\t-r, --repeat\tПерекласти порцію файлів, почекати 120с(змінити через -S) щоб запобігти бану й перейти до наступної порції. Якщо кількість файлів не вказана використовується 2 за замовчуванням
\t-s, --sleep\tЗасинати на вказану кількість секунд після перекладу файлу. 5с за замовчуванням
\t-S, --sleep-after\tЗасинати на вказану кількість секунд після перекладу порції файлів. 120с за замовчуванням
\t-t, --timeout\tСкільки чекати на результат перекладу. 60 секунд за замовчуванням
\t-h, --help\tПоказати цю довідку і завершити роботу"
      exit 0
      ;;
    -*|--*)
      echo "Unknown option $1"
      exit 1
      ;;
    *)
      args+=("$1") # save positional arg
      shift # past argument
      ;;
  esac
done

set -- "${args[@]}" # restore positional parameters

echo "running"

for arg in "$@"
do
	if [[ $arg == *.$ext ]]; then continue; fi
	
	if [ ! -f "$arg.$ext" ] || [ $(wc -c < "$arg.$ext") -eq 0 ]
	then
		if [ $(stat -c %s "$arg") -gt 1500 ]; then
			echo "file [$arg] too long, skip, 1500 chars max for DeepL Translator. Use: fix -n 1500 -T file"
			continue
		fi
		deepl --to uk --timeout=$timeout -f $arg > $arg.$ext 2> /dev/null
		if [ $? -eq 0 ]
		then
			((++done_total))
		else
			echo -e "\033[2K\rfailed [$arg] at [$(date +'%R %F')]"
			((++failed))
		fi

		((++done))
		if [ $count -ne 0 ]; then
			if [ "$done" -lt "$count" ]; then
				echo -ne "\rsleep $delay seconds [$(date +'%R %F')], [done $done_total] [$failed failed] [$# total]"

				sleep $delay
			else
				if [ "$repeat" -eq 1 ]; then
					done=0
					echo -ne "\rsleep $delay_after seconds [$(date +'%R %F')], [done $done_total] [$failed failed] [$# total]"

					sleep $delay_after
				else
					exit 0
				fi
			fi
		fi
		
		continue
	fi
done

echo -e "\nfinish"
