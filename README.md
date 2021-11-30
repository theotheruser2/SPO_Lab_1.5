# СПО Лабораторная работа №1.5

## Задание

Разработать способ организации данных в файле, позволяющий хранить, выбирать и гранулярно обновлять наборы записей общим объёмом от 10GB соответствующего варианту вида. Реализовать модуль или библиотеку для работы с ним в режиме курсора.

Используя данный способ сериализации, воспользоваться существующей библиотекой для описания схемы и реализации модуля, обеспечивающего функционирование протокола обмена запросами создания, выборки, модификации и удаления данных, и результатами их выполнения.

Использовать средство синтаксического анализа по выбору, реализовать модуль для разбора некоторого подмножества языка запросов по выбору в соответствии с вариантом формы данных. Должна быть обеспечена возможность описания команд создания, выборки, модификации и удаления данных.

Используя созданные модули разработать в виде консольного приложения две программы: клиентскую и серверную части. Серверная часть – получающая по сети запросы и операции описанного формата и выполняющая их над файлом, организованным в соответствии с разработанным способом. Имя файла данных для работы получать с аргументами командной строки, создавать новый в случае его отсутствия. Клиентская часть – получающая от пользователя команду, пересылающая её на сервер, получающая ответ и выводящая его в человекопонятном виде.

### Вариант 7

Форма данных - документное дерево

Протокол обмена - Protobuf

## Доступные команды

- `new <path>` (создать пустой элемент) или `new <path> = <value>` (создать элемент со значением)
- `set <path> = <value>`
- `get <path>` (`get` для чтения корня)
- `del <path>` (удалить элемент) или `del <path> =` (удалить значение элемента)

Элементы команд:
- `<path>` - разделённые точкой имена дочерних вершин, могут задаваться в одинарных или двойных кавычках для использования пробелов,
спецсимволов или цифры в качестве первого символа имени, возможно экранирование через обратный слеш (`\`) для использования кавычек
- `<value>` - любая строка в одинарных или двойных кавычках, возможно экранирование

Внимание! Функционал создания промежуточных элементов пути не реализован. Все элементы необходимо добавлять последовательно

## Инструкция по запуску:

1. `server [port] [file_name]`
2. `client [port]`
