#!/bin/sh

done=0
done_total=0
failed=0

delay=5
delay_after=120
translation_timeout=60000
try_again_after=3600

count=0
exit_count=0
exit_after_failed=0
last_failed_count=0

is_try_again=0
is_retry_on_failed=0

is_repeat=0
is_repeat_on_failed=0

ext="tr.txt"

args=()

show_help(){
      echo -e "usege: s [-rhE] [-c count] [-C exit_count] [-e failed_count] [-s delay] [-S dalay_after] [-t timeout] [-T retry_after] files
\t-c, --count\tПерекласти вказану кількість файлів. З -r скрипт перекладатиме вказаними порціями
\t-C, --exit-count\tЗавершити виконання після перекладу n-файлів
\t-e, --exit-after-failed\tЗавершити виконання після n-послідовних помилок
\t-E, --try-again\tПісля виникнення послідовних помилок перекладу зачекати 3600с(змінити чере -T) і спробувати ще раз, якщо знову невдало, то завершити виконання. Якщо кількість помилок не вказана, то 1 за замовчуванням. Якщо кількість файлів не вказана використовується 1 за замовчуванням
\t-r, --repeat\tПерекласти порцію файлів, почекати 120с(змінити через -S) щоб запобігти бану й перейти до наступної порції. Якщо кількість файлів не вказана використовується 1 за замовчуванням
\t-R, --repeat-on-failed\tТеж що й -r з -E, але не завершує виконання скрипта
\t-s, --sleep\tЗасинати на вказану кількість секунд після перекладу файлу. 5 секунд за замовчуванням
\t-S, --sleep-after\tЗасинати на вказану кількість секунд після перекладу порції файлів. 120 секунд за замовчуванням
\t-t, --timeout\tСкільки чекати на результат перекладу. 60 секунд за замовчуванням
\t-T, --try-again-after\tНа скільки секунд засинати перед повторною спробою перекладу після послідовних помилок які мали спричинити завершення виконання. 3600 секунд за замовчуванням. Не рекомендовано вказувати менш ніж годину(3600с), бо отримаєте повноцінний бан, а не обмеження на кількість перекладів
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
    -E|--try-again)
      is_try_again=1
      if [ $exit_after_failed -eq 0 ]; then
	exit_after_failed=1
      fi

      if [ $count -eq 0 ]; then
	count=1
      fi

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
      translation_timeout=$(($2 * 1000))
      shift # past argument
      shift # past value
      ;;
    -T|--try-again-after)
      try_again_after=$2
      shift # past argument
      shift # past value
      ;;
    -r|--repeat)
      is_repeat=1
      if [ $count -eq 0 ]; then
	count=1
      fi
      shift # past argument
      ;;
    -R|--repeat-on-failed)
      is_repeat=1
      is_try_again=1
      is_repeat_on_failed=1
      if [ $exit_after_failed -eq 0 ]; then
	exit_after_failed=1
      fi

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

		if [ $(stat -c %s "$arg") -eq 0 ]; then
			echo "file [$arg] is empty, skip"
			continue
		fi

		deepl --to uk --timeout=$translation_timeout -f $arg > $arg.$ext 2> /dev/null
		if [ $? -eq 0 ]
		#if ((1 + $RANDOM % 10 > 3)); # для тестів
		then
			((++done_total))
			if [ $exit_count -gt 0 ] && [ $exit_count -eq $done_total ]; then
				echo -ne "\rsleep $delay seconds [$(date +'%R %F')] [done $done_total] [$failed failed] [$# total] [$(date -u -d @"$SECONDS" +'%-Hh %-Mm %-Ss') elapsed]"
				echo -e "\nexit after $exit_count done files"
				exit 0
			fi

			last_failed_count=0
			is_retry_on_failed=0
		else
			((++failed))

			if [ $exit_after_failed -gt 0 ]; then
				if [ $is_retry_on_failed -eq 1 ]; then
					if [ $is_repeat_on_failed -eq 1 ]; then
						echo -e "\033[2K\rfailed [$arg] at [$(date +'%R %F')] sleep $try_again_after seconds, then try again"
						sleep $try_again_after
					else
						echo -e "\033[2K\rfailed [$arg] at [$(date +'%R %F') retry failed, exit"
						exit 2
					fi
				fi

				((++last_failed_count))

				if [ $exit_after_failed -eq $last_failed_count ]; then
					if [ $is_try_again -eq 1 ]; then
						if [ $is_repeat_on_failed -eq 1 ]; then
							echo -e "\033[2K\rfailed [$arg] at [$(date +'%R %F')] sleep $try_again_after seconds, then try again"
						else
							echo -e "\033[2K\rfailed [$arg] at [$(date +'%R %F')] sleep $try_again_after seconds, then try again or exit"
						fi
						sleep $try_again_after

						is_retry_on_failed=1
						continue
					else
						echo -e "\033[2K\rfailed [$arg] at [$(date +'%R %F')]\nexit after [$exit_after_failed] failed translation"
						exit 1
					fi
				else
					echo -e "\033[2K\rfailed [$arg] at [$(date +'%R %F')]"
				fi
			else
				echo -e "\033[2K\rfailed [$arg] at [$(date +'%R %F')]"
			fi
		fi
	
		((++done))
		if [ $count -ne 0 ]; then
			if [ "$done" -lt "$count" ]; then
				echo -ne "\rsleep $delay seconds [$(date +'%R %F')] [done $done_total] [$failed failed] [$# total] [$(date -u -d @"$SECONDS" +'%-Hh %-Mm %-Ss') elapsed]"

				sleep $delay
			else
				if [ "$is_repeat" -eq 1 ]; then
					done=0
					echo -ne "\rsleep $delay_after seconds [$(date +'%R %F')] [done $done_total] [$failed failed] [$# total] [$(date -u -d @"$SECONDS" +'%-Hh %-Mm %-Ss') elapsed]"

					sleep $delay_after
				else
					exit 0
				fi
			fi
			continue
		fi
		
		echo -ne "\rsleep $delay second [$(date +'%R %F')] [done $done_total] [$failed failed] [$# total] [$(date -u -d @"$SECONDS" +'%-Hh %-Mm %-Ss') elapsed]"
		sleep $delay
	fi
done

echo -e "\nfinish at [$(date +'%R %F')]"
