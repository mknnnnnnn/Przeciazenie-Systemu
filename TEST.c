/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
*                           (c) Copyright 1992-2002, Jean J. Labrosse, Weston, FL
*                                           All Rights Reserved
*
*                                               EXAMPLE #1
*********************************************************************************************************
*/

#include "includes.h"

/*
*********************************************************************************************************
*                                               CONSTANTS
*********************************************************************************************************
*/

#define  TASK_STK_SIZE                 512       /* Size of each task's stacks (# of WORDs)            */
#define  N_TASKS                        30       /* Number of identical tasks                          */
#define  rozmiarKolejki                100       // Rozmiar kolejki

INT32U zmiennaGlobalna = 0; // Zmienna globalna
INT8U mailboxError[] = {100, 100, 100, 100, 100}; // Tablica globalna do przekazywania błędów skrzynki
INT8U kolejkaError[] = {100, 100, 100, 100, 100}; // Tablica globalna do przekazywania błędów kolejki
INT32U utraconaWartoscSkrzynki;
INT32U utraconaWartoscKolejki;

// Struktura odpowiedzialna za przesyłanie danych pomiędzy zadaniami
typedef struct{
	INT16U numerZadania;
	INT32U licznikZadania;
	INT32U wartoscObciazeniaZadania;
	char wpisywaneObciazenie[15];
}DANE;

// Struktura odpowiedzialna za obsługę kolejki
typedef struct{
	INT16U numerZadaniaKolejki;
	INT32U wartoscObciazeniaKolejki;
	INT8U kolejnoscWiadomosciKolejki;
}KOLEJKA;
/*
*********************************************************************************************************
*                                               VARIABLES
*********************************************************************************************************
*/

OS_STK        TaskStk[N_TASKS][TASK_STK_SIZE];        /* Tasks stacks                                  */
OS_STK        TaskStartStk[TASK_STK_SIZE];
char          TaskData[N_TASKS];                      /* Parameters to pass to each task               */

OS_MEM       *zarzadzaniePamiecia1; // Zmienna wskaźnikowa
OS_MEM       *zarzadzaniePamiecia2; // Zmienna wskaźnikowa

OS_EVENT     *mailboxInterpreter; // Utworzenie Mailboxa pomiędzy Input a Interpreter
OS_EVENT     *mailboxDisplay; // Utworzenie Mailboxa pomiędzy Interpreterem, MTask, QTask,STask, a Display
OS_EVENT     *mailboxObciazenie; // Utworzenie Mailboxa pomiędzy MTask, QTask,STask, a Interpreter
OS_EVENT     *mailbox[5]; // Mailbox przechowujący zmienna globalną
OS_EVENT     *kolejka; // Kolejka przechowująca zmienna globalną
OS_EVENT     *semafor; // Semafor chroniący zmienną globalną
void         *tablicaKolejki[rozmiarKolejki]; // Tablica kolejki
INT32U        tablicaZarzadzaniePamiecia1[100]; // Tablica do zarządzania pamięcia
KOLEJKA       tablicaZarzadzaniePamiecia2[100]; // Tablica do zarządzania pamięcia


/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

        void  Task(void *data);                       /* Function prototypes of tasks                  */
        void  TaskStart(void *data);                  /* Function prototypes of Startup task           */
		void  Input(void* data);
		void  Interpreter(void* data);
		void  Display(void* data);
		void  Obciazenie(void *data);
		void  STask(void* data);
		void  MTask(void* data);
		void  QTask(void* data);
static  void  TaskStartCreateTasks(void);
static  void  TaskStartDispInit(void);
static  void  TaskStartDisp(void);

/*$PAGE*/
/*
*********************************************************************************************************
*                                                MAIN
*********************************************************************************************************
*/

void  main (void)
{
	INT8U i;
	INT8U err;
    PC_DispClrScr(DISP_FGND_WHITE + DISP_BGND_BLACK);      /* Clear the screen                         */

    OSInit();                                              /* Initialize uC/OS-II                      */
    PC_DOSSaveReturn();                                    /* Save environment to return to DOS        */
    PC_VectSet(uCOS, OSCtxSw);                             /* Install uC/OS-II's context switch vector */
	
	mailboxDisplay = OSMboxCreate(0);
	mailboxInterpreter = OSMboxCreate(0);
	mailboxObciazenie = OSMboxCreate(0);
	
	zarzadzaniePamiecia1 = OSMemCreate(&tablicaZarzadzaniePamiecia1[0], 100, sizeof(INT32U), &err);
	zarzadzaniePamiecia2 = OSMemCreate(&tablicaZarzadzaniePamiecia2[0], 100, sizeof(KOLEJKA), &err);
	
	// Utworzenie semafora, utworzenie skrzynek obciążenia oraz kolejki obciążenia
	semafor = OSSemCreate(1);
	
	for (i = 0; i < 5; i++)
		mailbox[i] = OSMboxCreate(0);
	
	kolejka = OSQCreate(&tablicaKolejki[0], rozmiarKolejki);
	
    OSTaskCreate(TaskStart, 0, &TaskStartStk[TASK_STK_SIZE - 1], 0);
    OSStart();                                             /* Start multitasking                       */
}


/*
*********************************************************************************************************
*                                              STARTUP TASK
*********************************************************************************************************
*/
void  TaskStart (void *pdata)
{
#if OS_CRITICAL_METHOD == 3                                /* Allocate storage for CPU status register */
    OS_CPU_SR  cpu_sr;
#endif

	DANE dane;

    pdata = pdata;                                         /* Prevent compiler warning                 */

    TaskStartDispInit();                                   /* Initialize the display                   */

    OS_ENTER_CRITICAL();
    PC_VectSet(0x08, OSTickISR);                           /* Install uC/OS-II's clock tick ISR        */
    PC_SetTickRate(OS_TICKS_PER_SEC);                      /* Reprogram tick rate                      */
    OS_EXIT_CRITICAL();

    OSStatInit();                                          /* Initialize uC/OS-II's statistics         */

    TaskStartCreateTasks();                                /* Create all the application tasks         */

	dane.numerZadania = 16; // Jeżeli numer zadania = 16 to upływa sekunda

    for (;;) {
        TaskStartDisp();                                  /* Update the display                        */
        OSCtxSwCtr = 0;                                    /* Clear context switch counter             */
        OSTimeDlyHMSM(0, 0, 1, 0);                         /* Wait one second                          */
		OSMboxPost(mailboxDisplay, &dane);
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                        INITIALIZE THE DISPLAY
*********************************************************************************************************
*/

static  void  TaskStartDispInit (void)
{
/*                                1111111111222222222233333333334444444444555555555566666666667777777777 */
/*                      01234567890123456789012345678901234567890123456789012345678901234567890123456789 */
    PC_DispStr( 0,  0, "                         uC/OS-II, The Real-Time Kernel                         ", DISP_FGND_WHITE + DISP_BGND_RED + DISP_BLINK);
    PC_DispStr( 0,  1, "                      Maksymilian Gerlach Franciszek Fornal                               ", DISP_FGND_YELLOW + DISP_BGND_BLUE);
    PC_DispStr( 0,  2, " Licznik        Obciazenie      Delta                                                          ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  3, "                                              1  QTask                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  4, "                                              2                                     ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  5, "                                              3                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  6, "                                              4                                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  7, "                                              5                                     ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  8, "                                              6  MTask                                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  9, "                                              7                                 ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 10, "                                              8                                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 11, "                                              9                                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 12, "                                              10                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 13, "                                              11 STask                                  ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 14, "                                              12                                   ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 15, "                                              13                                  ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 16, "                                              14                                  ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 17, "                                              15                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 18, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 19, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 20, "Wpisywane obciazenie:                                                                               ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 21, "Zapisane  obciazenie:                                                                               ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 22, "#Tasks          :        CPU Usage:     % System freq:       Hz                                       ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 23, "#Task switch/sec:                                                               ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 24, "                            <-PRESS 'ESC' TO QUIT->                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
/*                                1111111111222222222233333333334444444444555555555566666666667777777777 */
/*                      01234567890123456789012345678901234567890123456789012345678901234567890123456789 */
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                           UPDATE THE DISPLAY
*********************************************************************************************************
*/

static  void  TaskStartDisp (void)
{
    char   s[80];

    sprintf(s, "%5d", OSTaskCtr);                                  /* Display #tasks running               */
    PC_DispStr(18, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);
	sprintf(s, "%5d", OS_TICKS_PER_SEC);
	PC_DispStr(55, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);
#if OS_TASK_STAT_EN > 0
    sprintf(s, "%3d", OSCPUUsage);                                 /* Display CPU usage in %               */
    PC_DispStr(36, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);
#endif

    sprintf(s, "%5d", OSCtxSwCtr);                                 /* Display #context switches per second */
    PC_DispStr(18, 23, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

    sprintf(s, "V%1d.%02d", OSVersion() / 100, OSVersion() % 100); /* Display uC/OS-II's version number    */
    PC_DispStr(75, 24, s, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);

    switch (_8087) {                                               /* Display whether FPU present          */
        case 0:
             PC_DispStr(71, 22, " NO  FPU ", DISP_FGND_YELLOW + DISP_BGND_BLUE);
             break;

        case 1:
             PC_DispStr(71, 22, " 8087 FPU", DISP_FGND_YELLOW + DISP_BGND_BLUE);
             break;

        case 2:
             PC_DispStr(71, 22, "80287 FPU", DISP_FGND_YELLOW + DISP_BGND_BLUE);
             break;

        case 3:
             PC_DispStr(71, 22, "80387 FPU", DISP_FGND_YELLOW + DISP_BGND_BLUE);
             break;
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                             CREATE TASKS
*********************************************************************************************************
*/

static  void  TaskStartCreateTasks (void)
{
	INT8U  i;
	
	OSTaskCreate(Input, 0, &TaskStk[0][TASK_STK_SIZE - 1], 1);     
	OSTaskCreate(Interpreter, 0, &TaskStk[1][TASK_STK_SIZE - 1], 2);
	OSTaskCreate(Display, 0, &TaskStk[2][TASK_STK_SIZE - 1], 3);
	OSTaskCreate(Obciazenie, 0, &TaskStk[3][TASK_STK_SIZE - 1], 4);


	for (i = 1; i <= 15; i++) {
		
		TaskData[i] = i;
		
		if(i <= 5)
			OSTaskCreate(QTask, &TaskData[i], &TaskStk[i+3][TASK_STK_SIZE - 1], i+4); 
		
		if(i > 5 && i <= 10)
			OSTaskCreate(MTask, &TaskData[i], &TaskStk[i+3][TASK_STK_SIZE - 1], i+4);
		
		if(i > 10)
			OSTaskCreate(STask, &TaskData[i], &TaskStk[i+3][TASK_STK_SIZE - 1], i+4);
	}
}

/*
*********************************************************************************************************
*                                                  TASKS
*********************************************************************************************************
*/

void Input (void *pdata){
	INT16S klawisz;
	pdata = pdata;
	
	for(;;){
		if(PC_GetKey(&klawisz) == TRUE) // Odczyt klawisza
			OSMboxPost(mailboxInterpreter, &klawisz);
		OSTimeDly(19);
	}	
}

void Interpreter (void *pdata){
	DANE dane;
	INT8U kursor = 0;
	INT8U err;
	INT8U i;
	INT16S *wsk; // Wskaźnik pomocniczy
	INT16S klawisz;
	INT32U wartoscObciazenia;
	char lancuch[15];
	dane.numerZadania = 0; // Jeżeli numer zadania = 0 oznacza to, że są to dane wpisywane z klawiatury
	dane.wartoscObciazeniaZadania = 0; // Wartość początkowa obciążenia
	pdata = pdata;
	
	for(;;){
		wsk = OSMboxPend(mailboxInterpreter, 0, &err); // Odebranie danych klawisza
		klawisz = *wsk;
		
		switch(klawisz){
			case 0x1B: // ESCAPE
				PC_DOSReturn();
			break;
			
			case 0x08: // Backspace
				if(kursor > 0 ){
					kursor = kursor - 1;
					lancuch[kursor] = ' ';
				}
			break;
			
			case 0x0D: // Enter
				kursor = 0;
				wartoscObciazenia = strtoul(lancuch, &lancuch[10], 10); // Konswersja łańcucha na unsigned long
				OSMboxPost(mailboxObciazenie, &wartoscObciazenia); // Wysłanie danych obciążenia
				dane.wartoscObciazeniaZadania = wartoscObciazenia; // Zapisane obciążenie DISPLAY
				for (i = 0; i < 15; i++) // Wyczyszczenie łańcucha znaków
					lancuch[i] = ' ';
			break;
			
			default:
				if((klawisz >= 0x30) && (klawisz <= 0x39)){ // Odczyt cyfr od 0 do 9
					if (kursor < 10 ){
						lancuch[kursor] = klawisz;
						kursor = kursor + 1;
					}
				}
			break;
		}
		
		for(i = 0; i < 15; i++)
			dane.wpisywaneObciazenie[i] = lancuch[i];
		
		OSMboxPost(mailboxDisplay, &dane); // Wpisywane obciążenie DISPLAY
		}
}

void Obciazenie (void *pdata){
	DANE dane;
	KOLEJKA *wskKolejka;
	INT8U err;
	INT8U errSkrzynki;
	INT8U errKolejki;
	INT8U i;
	INT32U *wsk; // Wskaźnik pomocniczy
	INT32U *wskSkrzynka; // Wskaźnik pomocniczy
	INT32U wartoscObciazenia = 0;
	INT32U kolejnoscWiadomosci = 0;
	
	pdata = pdata;
	
	for (;;) {
		wsk = OSMboxPend(mailboxObciazenie, 0, &err); // Odebranie danych obciążenia
		wartoscObciazenia = *wsk;
		
		if (err == OS_NO_ERR){ // Sprawdzanie czy nie wystąpiły błędy
			OSSemPend(semafor, 0, &err); // Oczekiwanie na semafor
			zmiennaGlobalna = wartoscObciazenia; // Przypisanie obciążenia do zmienna globalnej
			OSSemPost(semafor); // Sygnalizowanie semafora
			
			for (i = 0; i < 5; i++){
				wskSkrzynka = OSMemGet(zarzadzaniePamiecia1, &err); // Pobranie bloku pamięci
				*wskSkrzynka = wartoscObciazenia; // Przypisanie obciążenia
				utraconaWartoscSkrzynki = wartoscObciazenia; // Poprzednia wartość obciążenia
				errSkrzynki = OSMboxPost(mailbox[i], wskSkrzynka); // Zwrot błędu
				if(errSkrzynki == OS_MBOX_FULL)
					mailboxError[i] = i + 10; // Tablica z informacją o błędzie poszczególnego zadania
			}
			
			kolejnoscWiadomosci++;
			
			for (i = 0; i < 5; i++){
				wskKolejka = OSMemGet(zarzadzaniePamiecia2, &err); // Pobranie bloku pamięci
				wskKolejka-> numerZadaniaKolejki = i + 1; // Ustawienie numeru zadania, które ma być odbiorcą konkretnej wiadomości
				wskKolejka-> wartoscObciazeniaKolejki = wartoscObciazenia; // Przypisanie obciążenia
				wskKolejka-> kolejnoscWiadomosciKolejki = kolejnoscWiadomosci;
				utraconaWartoscKolejki = wartoscObciazenia; // Poprzednia wartość obciążenia
				errKolejki = OSQPost(kolejka, wskKolejka); // Zwrot błędu
				if(errKolejki == OS_Q_FULL)
					kolejkaError[i] = i + 10; // Tablica z informacją o błędzie poszczególnego zadania
			}
		}
	}
}

void Display (void *pdata){
	DANE *dane;
	INT8U err;
	INT8U i;
	INT32U deltaLiczniki[15]; // Tablica do przechowywania różnicy liczników
	INT32U wartoscLiczniki[2][15]; // Tablica do przechowywania ilości aktualnych oraz poprzednich wartości
	INT32U licznikWejsciowy;
	INT32U wartoscObciazenia;
	INT8U numerZadania;
	INT32U zapisaneObciazenie;
	char zapisaneObciazenieString[15];
	char wpisywaneObciazenieString[15];
	char licznikWejsciowyString[15];
	char wartoscObciazeniaString[15];
	char deltaLicznikiString[15];
	char utraconaWartoscSkrzynkiString[15];
	char utraconaWartoscKolejkiString[15];
	pdata = pdata;
	
	for(i = 0; i < 15; i++) // Ustawienie 0 dla początkowych wartości
		wartoscLiczniki[0][i] = 0;
	
	for(;;){
		dane = OSMboxPend(mailboxDisplay, 0, &err);
		numerZadania = dane -> numerZadania;
		if(numerZadania > 0 && numerZadania < 16){ // Obciążenia poszczególnych zadań
			licznikWejsciowy = dane -> licznikZadania; // Odbiór wartości licznika poszczególnego zadania
			wartoscObciazenia = dane -> wartoscObciazeniaZadania; // Odbiór wartości obciążenia poszczególnego zadania
			wartoscLiczniki[1][numerZadania - 1] = licznikWejsciowy; // Zapisanie pobranej wartości licznika
			ultoa(licznikWejsciowy, licznikWejsciowyString, 10); // Konwersja na string
			ultoa(wartoscObciazenia, wartoscObciazeniaString, 10); // Konwersja na string
			PC_DispStr(1, numerZadania + 2, licznikWejsciowyString, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY); // Wyswietlanie licznika poszczególnych zadań
			PC_DispStr(16, numerZadania + 2, "               ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY); // Wyczyszczenie poprzedniego wyswietlanego obciążenia poszczególnych zadań
			PC_DispStr(16, numerZadania + 2, wartoscObciazeniaString, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY); // Wyswietlanie obciążenia poszczególnych zadań
			
		}else if(numerZadania == 0){ // Obciążenie wpisywane z klawiatury
			zapisaneObciazenie = dane -> wartoscObciazeniaZadania;
			
			if(zapisaneObciazenie != 0)
				ultoa(zapisaneObciazenie, zapisaneObciazenieString, 10); // Konwersja na string
			
			PC_DispStr(22, 21, "               ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY); // Wyczyszczenie poprzedniego zapisanego obciążenia
			
			PC_DispStr(22, 21, zapisaneObciazenieString, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY); // Zapisane obciążenie
			
			for(i = 0; i < 15; i ++)
				wpisywaneObciazenieString[i] = dane -> wpisywaneObciazenie[i]; // Wpisywane obciążenie
			
			PC_DispStr(22,  20, wpisywaneObciazenieString, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY); // Wpisywane obciążenie
			
		}else if(numerZadania == 16){
			for(i = 0; i < 15; i++){
				deltaLiczniki[i] = wartoscLiczniki[1][i] - wartoscLiczniki[0][i]; // Obliczanie delty
				wartoscLiczniki[0][i] = wartoscLiczniki[1][i]; // Zachowanie aktualnego stanu w celu wykorzystania w następnej sekundzie
				ultoa(deltaLiczniki[i], deltaLicznikiString, 10); // Konwersja na string
				PC_DispStr(32, i + 3, "    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY); // Wyczyszczenie poprzedniej wartość obliczonej delty
				PC_DispStr(32, i + 3, deltaLicznikiString, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY); // Wyświetlenie obliczonej delty
			}
			
			// Wyświetlanie błędu skrzynki
			for(i = 0; i < 5; i++){
				PC_DispStr( 55, 8 + i, "                        ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY); // Wyczyszczenie pozycji
				if(mailboxError[i] == 10 + i){ // Sprawdzenie czy do tablicy został przypisany błąd, taki jak w obciążeniu
					PC_DispStr( 55, 8 + i, "Utr wartosc -", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY); 
					ultoa(utraconaWartoscSkrzynki, utraconaWartoscSkrzynkiString, 10);
					PC_DispStr( 69, 8 + i, utraconaWartoscSkrzynkiString, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY); 
					mailboxError[i] = 100; // Przypisanie tablicy inne wartości, aby poprzednia wartość nie spełniała warunku if
				}
			}
			// Wyświetlanie błędu kolejki
			for(i = 0; i < 5; i++){
				PC_DispStr( 55, 3 + i, "                        ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY); // Wyczyszczenie pozycji
				if(kolejkaError[i] == 10 + i){ // Sprawdzenie czy do tablicy został przypisany błąd, taki jak w obciążeniu
					PC_DispStr( 55, 3 + i, "Utr wartosc -", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
					ultoa(utraconaWartoscKolejki, utraconaWartoscKolejkiString, 10);
					PC_DispStr( 69, 3 + i, utraconaWartoscKolejkiString, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
					kolejkaError[i] = 100; // Przypisanie tablicy inne wartości, aby poprzednia wartość nie spełniała warunku if
				}
			}
		}
	}
}

void MTask (void* pdata){
	DANE dane;
	INT32U i;
	INT32U wartoscObciazenia = 0;
	INT32U obciazeniePetli = 0;
	INT32U licznikWejsciowy = 0;
	INT32U *wsk; // Wskaźnik pomocniczy
	INT8U p = *(INT8U*)pdata;
	dane.numerZadania = p; // Przypisanie takiego samego numeru jak przy tworzeniu zadania
	dane.wartoscObciazeniaZadania = 0;
	
	for(;;){
		wsk = OSMboxAccept(mailbox[p-6]); // Sprawdzenie skrzynki
		
		if(wsk != 0){
			wartoscObciazenia = *wsk;
			OSMemPut(zarzadzaniePamiecia1, wsk); // Zwrot bloku do partycji pamięci
		}
		
		licznikWejsciowy++;
		dane.licznikZadania = licznikWejsciowy; // Do obliczania delty
		dane.wartoscObciazeniaZadania = wartoscObciazenia;
		OSMboxPost(mailboxDisplay, &dane);
		
		for(i = 0; i < wartoscObciazenia; i++) // Pętla obciążająca
			obciazeniePetli++;
		OSTimeDly(1);
	}
}

void QTask (void* pdata){
	DANE dane;
	KOLEJKA wartoscObciazeniaKolejki;
	KOLEJKA *wskKolejka;
	INT8U kolejnoscWiadomosci = 1;
	INT32U licznikWejsciowy = 0;
	INT32U i;
	INT32U wartoscObciazenia = 0;
	INT32U obciazeniePetli = 0;
	INT8U p = *(INT8U*)pdata;
	dane.numerZadania = p; // Przypisanie takiego samego numeru jak przy tworzeniu zadania
	dane.wartoscObciazeniaZadania = 0;
	
	for(;;){
		for(i = 0; i < 5; i++){ // Pętla odbierająca wartość obciążenia dla poszczególnego zadania z kolejki oraz sprawdzająca czy numer zadania oraz kolejność wiadomości zgadza się
			wskKolejka = OSQAccept(kolejka); // Sprawdzenie kolejki
			
			if(wskKolejka != 0){
				wartoscObciazeniaKolejki = *wskKolejka; // Pobranie obciążenia z kolejki
				if(p == wartoscObciazeniaKolejki.numerZadaniaKolejki && kolejnoscWiadomosci == wartoscObciazeniaKolejki.kolejnoscWiadomosciKolejki){ // Sprawdzenie czy zadanie jest właściwym odbiorcą
					wartoscObciazenia = wartoscObciazeniaKolejki.wartoscObciazeniaKolejki;
					OSMemPut(zarzadzaniePamiecia2, wskKolejka); // Zwrot bloku do partycji pamięci
					kolejnoscWiadomosci++;
					break;
				}else
					OSQPost(kolejka, wskKolejka); // Zwrocenie wiadomości do kolejki, jeżeli warunki numeru zadania oraz kolejności wiadomości nie zostały spełnione
			}else
				break;
		}
		
		licznikWejsciowy++; 
		dane.licznikZadania = licznikWejsciowy; // Do obliczania delty
		dane.wartoscObciazeniaZadania = wartoscObciazenia;
		OSMboxPost(mailboxDisplay, &dane); 
		
		for(i = 0; i < wartoscObciazenia; i++) // Pętla obciążająca
			obciazeniePetli++;
		
		OSTimeDly(1);
	}
}

void STask (void *pdata){
	DANE dane;
	INT32U licznikWejsciowy = 0;
	INT32U i;
	INT8U err;
	INT32U wartoscObciazenia = 0;
	INT32U obciazeniePetli = 0;
	INT8U p = *(INT8U*)pdata;
	dane.numerZadania = p; // Przypisanie takiego samego numeru jak przy tworzeniu zadania
	dane.wartoscObciazeniaZadania = 0;
	
	for(;;){
		err = OSSemAccept(semafor); // Odczytanie wartości zmiennej z semafora
		
		if (err > 0){
			wartoscObciazenia = zmiennaGlobalna;
			OSSemPost(semafor);
		}
		
		licznikWejsciowy++;
		dane.licznikZadania = licznikWejsciowy; // Do obliczania delty
		dane.wartoscObciazeniaZadania = wartoscObciazenia;
		OSMboxPost(mailboxDisplay, &dane);
		
		for(i = 0; i < wartoscObciazenia; i++) // Pętla obciążająca
			obciazeniePetli++;
		
		OSTimeDly(1);
	}
}