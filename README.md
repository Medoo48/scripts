#### Саморобні програми/скрипти які використовую для перекладів ранобе. Найкраще працюють з UNIX-подібною операційною системою, наприклад GNU/Linux або MacOS, на Windows у MSYS2.

[Як ці скрипти працюють разом](https://github.com/Medoo48/scripts/blob/main/how_it_works.md)

__s__ - переклад іншомовного тексту українською. Детальний опис у [файлі](https://github.com/Medoo48/scripts/blob/main/s_usage.md). У параметрах файли які треба перекласти. Переклад зберігається як `[імʼя файлу].tr.txt` (можна змінити). Наявні файли перекладу не перезаписуються. Розмір файлів які треба перекласти мусить бути до `1500` символів (обмеження `DeepL` для користувачів без облікового запису). Поділ великого файлу на менші відбувається за допомогою програми `fix`, з параметрами `-n 1500 -T`. `deepl` - це програма __deepl-cli__. s від sync, синхронізація перекладу)

__mp3__ - перетворює текст в mp3-файл. У параметрах — файли які треба озвучити. Скрипт запускає перетворення 5 файлів одночасно(можна збільшити/зменшити через змінну `maxjobs` у скрипті). Після виконання отримуємо файли `[імʼя файлу].mp3`. RHVoice - синтезатор мови з відкритим кодом який створює непоганий звук використовуючи мінімум ресурсів. У скрипті використовую голос `marianna`

__chg__ - виправити структуру перекладених файлів та замінити певні слова на інші. chg від change - змінити

__fix__ - Швейцарський ніж, програма на мові C++. Детальний опис у [файлі](https://github.com/Medoo48/scripts/blob/main/fix_usage.md). Використовується для:
1. поділу текстового файлу на окремі розділи по ключовому слову. Приклад: `fix -c 1 -C "\nРозділ " all.txt`
2. нарізання файлів за кількістю символів скільки максимально підтримує перекладач. Приклад: `fix -n 1500 -T розділ1 розділ2`
3. копіювання тексту у буфер обміну для перекладу і зберігання результату:
- за кількістю символів скільки максимально підтримує перекладач. Приклад: `fix -n 1500 -E розділ1 розділ2`.
- за ключовим словом. Приклад: `fix -e -C "\n\n" розділи1_645`.
4. поділу розділу на ділянки певної довжини. Приклад: `fix -n 1500 -t розділ1`
5. виправлення закінчень речень які були пошкоджені при перетворенні книги

__concat__ - обʼєднує файли в один. Якщо виконується без параметрів — всі файли у теці будуть поєднані у файлі `all.txt`, якщо у параметрах вказати файли — будуть поєднані лише вони у файлі `all.txt`. Файл `all.txt` не перезаписується, а доповнюється

__Makefile__ - скрипт побудови програми `fix` за допомогою `make` і `gcc`

__lightnovelworld_text.automa.json__ - отримати текст ранобе з `lightnovelworld.com`. Скрипт треба імпортувати у додаток [Automa](https://www.automa.site/), у ньому створити нову таблицю і підʼєднати до скрипта. Для використання перейдіть на перший розділ ранобе і запустіть скрипт. Експортуйте текст ранобе з таблиці як JSON(звичайний текст буде обʼєднано в один рядок) і використайте скрипт `chg` для отримання звичайного тексту: `chg json chapters.json`

У теці __story__ скрипти які більше не використовую

Ліцензія на ці програми GPL3+, якщо не зазначено іншого.

Дивіться інші цікаві програми [на сайті](https://cakestwix.github.io/WebUkrainianStuff/)
