# System-Load

Głównym celem napisanego programu było obciążenie systemu operacyjnego. W tym celu przygotowano 15 zadań obciążających. Pięć zadań, które wartość obciążenia otrzymywały za pomocą dostępu do zmiennej globalnej z użyciem semafora, pięć zadań otrzymujących wartość obciążenia za pomocą Mailboxa oraz pięć zadań otrzymujących wartość obciążenie za pomocą kolejki.

W celu realizacji założeń programu zostało utworzone siedem współpracujących ze sobą zadań:

•	Input - Zadanie realizujące zapis znaków wpisanych przez użytkownika, pobranie wartości z klawiatury.

•	Interpreter - Zadanie realizujące funkcje klawiszy specjalnych.

•	Display - Zadanie realizujące wyświetlanie tekstu na ekranie.

•	Obciazenie – Zadanie realizujące rozdzielenie obciążenia pomiędzy zadaniami obciążającymi.

•	QTask – zadania obciążające, dostęp do wartości obciążenia za pomocą kolejki.

•	MTask – zadania obciążające, dostęp do wartości obciążenia za pomocą Mailboxu.

•	STask – zadania obciążające, dostęp do wartości obciążenia za pomocą Semafora.

