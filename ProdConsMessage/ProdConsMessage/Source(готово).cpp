#include <windows.h>
#include <conio.h>
#include <random>
#include <time.h>
#include <iostream>


using namespace std;

#define N 5 
CRITICAL_SECTION cs;									// Критическая секция для безопасного вывода на экран
HANDLE ConsEmpty, ConsFull, ProdEmpty, ProdFull;		// Дескрипторы событий для остановки потоков
int ProdID = 1;	// код Producer для фунrций RecvMess и SendMess
int ConsID = 2; // код Consumer для функций RecvMess и SendMess

class MessagesQueue // для для реализации очереди сообщений
{
public:
	MessagesQueue(){ count = 0;}

	bool isEmpty()// проверка очереди на пустоту
	{
		if (count == 0)return true;
		else return false;
	}
	bool isFull()// проверка очереди на заполненность
	{
		if (count+1 >= N)return true;
		else return false;
	}

	int GetFirstMsg()// возвращает первое сообщение в очереди
	{
		int _data = messages[0];
		count = count - 1;
		for (int i = 0; i < count; i++) messages[i] = messages[i + 1];
		return _data;
	}

	void SetMsg(int _data)// добавляет сообщение в конец очереди
	{
		messages[count] = _data;
		count++;
	}
	void PrintBuf()
	{
		for (int i = 0; i < count; i++)
		{
			cout << messages[i] << " ";
		}
	}

private:
	int messages[N] = {};
	int count;
};

MessagesQueue messages_for_prod; //очередь для Producer
MessagesQueue messages_for_cons; //очередь для Consumer
 

void SendMess(int addres, int data)  //функция отправки сообщения в очередь 
{
	if (addres == 1) //producer
	{
		messages_for_prod.SetMsg(0);
	}
	if (addres == 2) //consumer
	{
		messages_for_cons.SetMsg(data);
	}
}

void RecvMess(int addres, int * mes) //функция взятия сообщения из очереди
{
	if (addres == 1) // от producer
	{
		if (mes != NULL)* mes = messages_for_cons.GetFirstMsg();
	}
	if (addres == 2) // от consumer
	{
		if (mes != NULL)*mes = messages_for_prod.GetFirstMsg();
	}
}


void Consumer(void* IDTProducer) // функция потока Consumer
{
	srand(time(0));
	int data = 0;

	for (int i = 0; i < N; i++) // начальная отправка пустых сообщений Producer'у
	{
		SendMess(ProdID, 0);	
	}
	PulseEvent(ProdEmpty); // Показываем Producer'у что теперь его очередь сообщений не пустая


	while (true) 
	{

		if (messages_for_cons.isEmpty()) WaitForSingleObject(ConsEmpty, INFINITE); // Если очередь Consumer'а пустая, ждем, пока в ней что-то появится
		RecvMess(ProdID, &data);// получаем от Producer'а сообщение
		PulseEvent(ConsFull);//показываем Producer'у что в очереди Consumer'а есть место
		
		//-------------------Обработка полученного сообщения-----------------------
		EnterCriticalSection(&cs);// с помощью критической секции
		cout << "Consumer took: " << data << endl;
		cout << "Bufer: ";
		messages_for_cons.PrintBuf();
		cout << endl << endl;
		cout.flush();
		LeaveCriticalSection(&cs);
		//--------------------------------------------------------------------------


		if(messages_for_prod.isFull()) WaitForSingleObject(ProdFull, INFINITE); // если очередь Producer'а заполнена, ждем, когда освободиться место
		SendMess(ProdID, 0); // отправляем пустое сообщение Producer'у
		PulseEvent(ProdEmpty); // Показываем Producer'у что его очередь сообщений не пустая

		//Приостанавливаем процесс на случайное время
		int n = rand() % 3 + 1;
		Sleep(n * 100);
	}
}

void Producer(void* IDTConsumer)// функция потока Producer
{
	srand(time(0));
	int data_new = 0;
	

	while (true)
	{
		data_new = rand() % 100+1;

		if (messages_for_prod.isEmpty())WaitForSingleObject(ProdEmpty, INFINITE); // Если очередь Producer'а пустая, ждем, пока в ней что-то появится
		RecvMess(ConsID, NULL);// получаем от Consumer'а пустое сообщение
		PulseEvent(ProdFull);//показываем Consumer'у что в очереди Producer'а есть место

		//-------------------Обработка отправляемого сообщения-----------------------
		EnterCriticalSection(&cs);
		cout << "Produser put: " << data_new << endl;
		cout << "Bufer: ";
		messages_for_cons.PrintBuf();
		cout << data_new << endl<< endl;
		cout.flush();
		LeaveCriticalSection(&cs);
		//--------------------------------------------------------------------------


		if (messages_for_cons.isFull()) WaitForSingleObject(ConsFull, INFINITE); // если очередь Consumer'а заполнена, ждем, когда освободиться место
		SendMess(ConsID, data_new);// отправляем сообщение Consumer'у
		PulseEvent(ConsEmpty);// Показываем Consumer'у что его очередь сообщений не пустая

		//Приостанавливаем процесс на случайное время
		int n = rand() % 3 + 1;
		Sleep(n * 100);
	}

}


int main()
{
	srand(time(0));

	// Инициальзация критической секции и событий
	InitializeCriticalSection(&cs);
	ConsEmpty = CreateEvent(NULL, FALSE, FALSE, NULL);
	ConsFull = CreateEvent(NULL, FALSE, FALSE, NULL);
	ProdEmpty = CreateEvent(NULL, FALSE, FALSE, NULL);
	ProdFull = CreateEvent(NULL, FALSE, FALSE, NULL);
	//--------------------------------------------

	//Создание дескрипторов потоков
	HANDLE TConsumer;
	DWORD dTConsumer;

	HANDLE TProducer;
	DWORD dTProducer;

	//Создаем потоки Producer и Consumer
	TProducer = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Producer, &dTConsumer, 0, &dTProducer);
	TConsumer = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Consumer, &dTProducer, 0, &dTConsumer);


	if (TProducer == NULL || TConsumer == NULL)//если не создались, то ошибка
	{
		cout << "Error of creating new thread..." << endl;
		return GetLastError();
	}

	_getch(); // ждем ввода от пользователя(при нажатии на любую кнопку программа закрывается)
	cout << "Successful completion" << endl;

	return 0;
}