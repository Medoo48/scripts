#!/bin/sh

done=0
done_total=0
failed=0
count=0
delay=5
delay_after=120
timeout=60000
repeat=0
exit_count=0
exit_after_failed=0

is_last_failed=0
last_failed=0


ext="tr.txt"

args=()

show_help(){
      echo -e "usege: s [-rh] [-c count] [-C exit_count] [-e failed_count] [-s delay] [-S dalay_after] [-t timeout] files
\t-c, --count\tПерекласти вказану кількість файлів. З -r скрипт перекладатиме вказаними порціями
\t-C, --exit-count\tЗавершити виконання після перекладу n-файлів
\t-e, --exit-after-failed\tЗавершити виконання після n-послідовних помилок
\t-r, --repeat\tПерекласти порцію файлів, почекати 120с(змінити через -S) щоб запобігти бану й перейти до наступної порції. Якщо кількість файлів не вказана використовується 1 за замовчуванням
\t-s, --sleep\tЗасинати на вказану кількість секунд після перекладу файлу. 5с за замовчуванням
\t-S, --sleep-after\tЗасинати на вказану кількість секунд після перекладу порції файлів. 120с за замовчуванням
\t-t, --timeout\tСкільки чекати на результат перекладу. 60 секунд за замовчуванням
\t-h, --help\tПоказати цю довідку і завершити роботу"
}

if [ $# -eq 0 ]; then
	show_help;
	exit 1
fi

while [[ $# -gt 0 ]]; do
  case $1 in
    -c|--count)
      count="$2"
      shift # past argument
      shift # past value
      ;;
    -C|--exit-count)
      exit_count="$2"
      shift # past argument
      shift # past value
      ;;
    -e|--exit-after-failed)
      exit_after_failed="$2"
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
	count=1
      fi
      shift # past argument
      ;;
    -h|--help)
      show_help;
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

echo "start at [$(date +'%R %F')]"

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
		
			if [ $exit_count -gt 0 ] && [ $exit_count -eq $done_total ]; then
				echo -ne "\rsleep $delay seconds [$(date +'%R %F')] [done $done_total] [$failed failed] [$# total]"
				echo -e "\nexit after $exit_count done files"
				exit 0
			fi

			is_last_failed=0
		else
			echo -e "\033[2K\rfailed [$arg] at [$(date +'%R %F')]"
			((++failed))

			if [ $exit_after_failed -gt 0 ]; then
				if [ $is_last_failed -eq 0 ]; then
					last_failed=1
				else
					((++last_failed))
				fi

				if [ $exit_after_failed -eq $last_failed ]; then
					echo -e "\nexit after [$exit_after_failed] failed translation"
					exit 0
				fi

				is_last_failed=1
			fi
		fi
	
		((++done))
		if [ $count -ne 0 ]; then
			if [ "$done" -lt "$count" ]; then
				echo -ne "\rsleep $delay seconds [$(date +'%R %F')] [done $done_total] [$failed failed] [$# total]"

				sleep $delay
			else
				if [ "$repeat" -eq 1 ]; then
					done=0
					echo -ne "\rsleep $delay_after seconds [$(date +'%R %F')] [done $done_total] [$failed failed] [$# total]"

					sleep $delay_after
				else
					exit 0
				fi
			fi
			continue
		fi
		
		echo -ne "\rsleep $delay sendond [$(date +'%R %F')] [done $done_total] [$failed failed] [$# total]"
		sleep $delay
	fi
done

echo -e "\nfinish at [$(date +'%R %F')]"
