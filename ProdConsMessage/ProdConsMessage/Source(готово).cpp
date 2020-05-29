#include <windows.h>
#include <conio.h>
#include <random>
#include <time.h>
#include <iostream>


using namespace std;

#define N 5 
CRITICAL_SECTION cs;									// ����������� ������ ��� ����������� ������ �� �����
HANDLE ConsEmpty, ConsFull, ProdEmpty, ProdFull;		// ����������� ������� ��� ��������� �������
int ProdID = 1;	// ��� Producer ��� ���r��� RecvMess � SendMess
int ConsID = 2; // ��� Consumer ��� ������� RecvMess � SendMess

class MessagesQueue // ��� ��� ���������� ������� ���������
{
public:
	MessagesQueue(){ count = 0;}

	bool isEmpty()// �������� ������� �� �������
	{
		if (count == 0)return true;
		else return false;
	}
	bool isFull()// �������� ������� �� �������������
	{
		if (count+1 >= N)return true;
		else return false;
	}

	int GetFirstMsg()// ���������� ������ ��������� � �������
	{
		int _data = messages[0];
		count = count - 1;
		for (int i = 0; i < count; i++) messages[i] = messages[i + 1];
		return _data;
	}

	void SetMsg(int _data)// ��������� ��������� � ����� �������
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

MessagesQueue messages_for_prod; //������� ��� Producer
MessagesQueue messages_for_cons; //������� ��� Consumer
 

void SendMess(int addres, int data)  //������� �������� ��������� � ������� 
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

void RecvMess(int addres, int * mes) //������� ������ ��������� �� �������
{
	if (addres == 1) // �� producer
	{
		if (mes != NULL)* mes = messages_for_cons.GetFirstMsg();
	}
	if (addres == 2) // �� consumer
	{
		if (mes != NULL)*mes = messages_for_prod.GetFirstMsg();
	}
}


void Consumer(void* IDTProducer) // ������� ������ Consumer
{
	srand(time(0));
	int data = 0;

	for (int i = 0; i < N; i++) // ��������� �������� ������ ��������� Producer'�
	{
		SendMess(ProdID, 0);	
	}
	PulseEvent(ProdEmpty); // ���������� Producer'� ��� ������ ��� ������� ��������� �� ������


	while (true) 
	{

		if (messages_for_cons.isEmpty()) WaitForSingleObject(ConsEmpty, INFINITE); // ���� ������� Consumer'� ������, ����, ���� � ��� ���-�� ��������
		RecvMess(ProdID, &data);// �������� �� Producer'� ���������
		PulseEvent(ConsFull);//���������� Producer'� ��� � ������� Consumer'� ���� �����
		
		//-------------------��������� ����������� ���������-----------------------
		EnterCriticalSection(&cs);// � ������� ����������� ������
		cout << "Consumer took: " << data << endl;
		cout << "Bufer: ";
		messages_for_cons.PrintBuf();
		cout << endl << endl;
		cout.flush();
		LeaveCriticalSection(&cs);
		//--------------------------------------------------------------------------


		if(messages_for_prod.isFull()) WaitForSingleObject(ProdFull, INFINITE); // ���� ������� Producer'� ���������, ����, ����� ������������ �����
		SendMess(ProdID, 0); // ���������� ������ ��������� Producer'�
		PulseEvent(ProdEmpty); // ���������� Producer'� ��� ��� ������� ��������� �� ������

		//���������������� ������� �� ��������� �����
		int n = rand() % 3 + 1;
		Sleep(n * 100);
	}
}

void Producer(void* IDTConsumer)// ������� ������ Producer
{
	srand(time(0));
	int data_new = 0;
	

	while (true)
	{
		data_new = rand() % 100+1;

		if (messages_for_prod.isEmpty())WaitForSingleObject(ProdEmpty, INFINITE); // ���� ������� Producer'� ������, ����, ���� � ��� ���-�� ��������
		RecvMess(ConsID, NULL);// �������� �� Consumer'� ������ ���������
		PulseEvent(ProdFull);//���������� Consumer'� ��� � ������� Producer'� ���� �����

		//-------------------��������� ������������� ���������-----------------------
		EnterCriticalSection(&cs);
		cout << "Produser put: " << data_new << endl;
		cout << "Bufer: ";
		messages_for_cons.PrintBuf();
		cout << data_new << endl<< endl;
		cout.flush();
		LeaveCriticalSection(&cs);
		//--------------------------------------------------------------------------


		if (messages_for_cons.isFull()) WaitForSingleObject(ConsFull, INFINITE); // ���� ������� Consumer'� ���������, ����, ����� ������������ �����
		SendMess(ConsID, data_new);// ���������� ��������� Consumer'�
		PulseEvent(ConsEmpty);// ���������� Consumer'� ��� ��� ������� ��������� �� ������

		//���������������� ������� �� ��������� �����
		int n = rand() % 3 + 1;
		Sleep(n * 100);
	}

}


int main()
{
	srand(time(0));

	// ������������� ����������� ������ � �������
	InitializeCriticalSection(&cs);
	ConsEmpty = CreateEvent(NULL, FALSE, FALSE, NULL);
	ConsFull = CreateEvent(NULL, FALSE, FALSE, NULL);
	ProdEmpty = CreateEvent(NULL, FALSE, FALSE, NULL);
	ProdFull = CreateEvent(NULL, FALSE, FALSE, NULL);
	//--------------------------------------------

	//�������� ������������ �������
	HANDLE TConsumer;
	DWORD dTConsumer;

	HANDLE TProducer;
	DWORD dTProducer;

	//������� ������ Producer � Consumer
	TProducer = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Producer, &dTConsumer, 0, &dTProducer);
	TConsumer = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Consumer, &dTProducer, 0, &dTConsumer);


	if (TProducer == NULL || TConsumer == NULL)//���� �� ���������, �� ������
	{
		cout << "Error of creating new thread..." << endl;
		return GetLastError();
	}

	_getch(); // ���� ����� �� ������������(��� ������� �� ����� ������ ��������� �����������)
	cout << "Successful completion" << endl;

	return 0;
}