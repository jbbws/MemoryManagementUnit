// MemoryManagmentUnit.cpp: определ€ет точку входа дл€ консольного приложени€.
//

#include "stdafx.h";
#include <bitset>;
#include <iostream>;
#include <stdio.h>;
#include <Windows.h>;
#include <fstream>;

using namespace std;
#pragma region  ќЌ—“јЌ“џ
const int VTABLE_SIZE = 256;
const int FTABLE_SIZE = 128;
const int FRAME_SIZE = 256;
const int TLB_SIZE = 16;
const int NOPAGE = -1000;
#pragma endregion 

#pragma region —трукутры
struct VTABLE_Rec
{
	bool inMemory;
	int block;
	int lastReq;
};
struct TLB_Rec
{
	int block;
	int page;
	bool active;
	int lastReq;
};
#pragma endregion

#pragma region »Ќ»÷»јЋ»«ј÷»я

VTABLE_Rec* initVirtualTable(int size)
{
	VTABLE_Rec* table = new VTABLE_Rec[size];
	for (int i = 0; i < size; i++)
	{
		table[i].inMemory = false;
		table[i].block = -1;
		table[i].lastReq = -1;
	}
	return table;
}

char** initFrameTable(int size)
{
	char** table = new char*[size];
	for (int i = 0; i < size; i++)
	{
		table[i] = new char[FRAME_SIZE];
	}
	return table;
}


TLB_Rec* initTLB(int size)
{
	TLB_Rec* tlb = new TLB_Rec[size];
	for (int i = 0; i < size; i++)
	{
		tlb[i].block = -1;
		tlb[i].page = -1;
		tlb[i].active = false;
		tlb[i].lastReq = -1;
	}
	return tlb;
}
#pragma endregion

#pragma region ‘”Ќ ÷»» ƒЋя –јЅќ“џ — ‘»«»„≈— ќ… ѕјћя“№ё
char getCharacter(char** table, int page, int offset)
{
	return table[page][offset];
}
#pragma endregion

#pragma region ‘”Ќ ÷»» ƒЋя –јЅќ“џ — “≈ —“ќ¬џћ ‘ј…Ћќћ —»ћ¬ќЋќ¬
void getDataFromDisk(const char* filename,int page, char** frame_table, int block) //перемещаем с диска на физическую пам€ть
{
	ifstream disk(filename, ios::in);
	if (disk.is_open())
	{
		disk.seekg(page,ios::beg);
		if(!disk.eof())
		{
			char temp[FRAME_SIZE];
			disk.read(temp, sizeof(char)*FRAME_SIZE);
			memcpy(frame_table[block],temp,sizeof(char)*FRAME_SIZE);
			disk.close();
		}
		else throw exception("END OF FILE");
	}
	else
	{
		cout << "ERROR: " << endl;
		throw exception("WRONG READ FILE");
	}
}
#pragma endregion

#pragma region ‘”Ќ ÷»» ƒЋя –јЅќ“џ — –≈«”Ћ№“»–”ёў»ћ ‘ј…Ћќћ
void pushCharToFile(char c, ofstream* out)
{
	if (out->is_open())
	{
		out->put(c);
	}
	else
	{
		throw exception("Can't open result file");
	}
}
void openResultFile(string filename,ofstream* o)
{
	o->open(filename, ostream::out);
}
void closeResultFile(ofstream* out)
{
	out->close();
}
#pragma endregion

#pragma region ‘”Ќ ÷»» ƒЋя –јЅќ“џ — Ѕ»Ќј–Ќџћ ‘ј…Ћќћ јƒ–≈—ќ¬
void getVirtualAdress(int* page, int* offset, ifstream* inp)
{
	int r = 0;
	inp->read((char*)&r, sizeof(r));
	*offset = r & 255;
	*page = (r >> 8) & 255;

	cout << "READ: " << r << endl;
	cout << "P: " << *page << endl;
	cout << "OFF: " << *offset << endl;
}
void openVirtualAdressFile(string filename, ofstream* inp)
{
	inp->open(filename, ios::binary);
}
void closeVirtualAdressFile(ofstream* out)
{
	out->close();
}
bool inputIsOpen(ofstream* out)
{
	return out->is_open();
}
#pragma endregion

#pragma region ‘”Ќ ÷»» ƒЋя –јЅќ“џ — TLB
int TLBHit(TLB_Rec* TLB, int page,int it) //возвращает номер блока
{
	int res = -1;
	for (int i = 0; i < TLB_SIZE; i++)
	{
		if (TLB[i].page == page)
		{
			TLB[i].lastReq = it;
			return TLB[i].block;
		}
	}
	return res;
}
int findPlaceInTLB(TLB_Rec* TLB,int it)
{
	int req = it,ind = -1;

	for (int i = 0; i < TLB_SIZE; i++)
	{
		if (TLB[i].lastReq < 0)
		{
			return i;
		}
		else if(TLB[i].lastReq < req)
		{
			req = TLB[i].lastReq;
			ind = i;
		}
	}
	return ind;
}
void writeToTLB(TLB_Rec* TLB, int index,TLB_Rec rec)
{
	TLB[index] = rec;
}
TLB_Rec fillTLBRecord(int it,int page,int block)
{
	TLB_Rec res;
	res.page = page;
	res.block = block;
	res.lastReq = it;
	res.active = true;
	return res;
}
int SynchTLB(TLB_Rec* TLB, TLB_Rec rec,int pushInd) //ищет место дл€ записи в TLB замен€€€ удаленную если она была или вытесн€€ самую ненужную
{
	int ind = -1;
	for (int i = 0; i < TLB_SIZE; i++)
	{
		if (TLB[i].page == pushInd)
		{
			ind = i;
		}
	}
	if (ind >= 0)
	{
		return ind;
	}
	else
	{
		int index = findPlaceInTLB(TLB, rec.lastReq);
		return index;
	}
}
#pragma endregion

#pragma region ‘”Ќ ÷»» ƒЋя –јЅќ“џ — “јЅЋ»÷≈… —“–јЌ»÷
void resetVTRecord(VTABLE_Rec* table, int index)
{
	table[index].block = -1;
	table[index].inMemory = false;
	table[index].lastReq = -1;
}
int VTableHit(VTABLE_Rec* table, int page,int iter)
{
	if (table[page].inMemory == true)
	{
		table[page].lastReq = iter;
		return table[page].block;
	}
	else return -1;
}
int findPlaceInVtable(VTABLE_Rec* table, int iter,int& actCount,bool& pushRec, int&indPush) //возвращает номер свободного блока
{
	int res = -1;
	if(actCount < FTABLE_SIZE) //есть свободное место в физической пам€ти
	{ 
		for (int i = 0,block = 0; i < VTABLE_SIZE; i++)
		{
			if (table[i].inMemory) //просматриваем только активные таблицы
			{
				if (table[i].block == block) //если блок зан€т то сначала ищем блок с номером i+1
				{
					i = -1;//или 0;
					block++;
					res = -1;
					continue;
				}
				else
				{
					res = block;
			//		indPush = i;
				}
			}
			else res = block;
		}
	}
	else  //если места нету и нижно вытесн€ть записи
	{
		int req = iter;
		int ind = -1;
		for (int i = 0; i < VTABLE_SIZE; i++)
		{
			if (table[i].inMemory == true) //просматривать только активные страницы
			{
				if (table[i].lastReq < req)
				{
					req = table[i].lastReq;
					ind = i;
				}							
			}
		}
		actCount--;
		res = table[ind].block;
		resetVTRecord(table, ind);
		pushRec = true;
		indPush = ind;
	}
	return res;
}
void writeToVtable(VTABLE_Rec* table, int ind, int block, int it,int& act)
{
	table[ind].block = block;
	table[ind].lastReq = it;
	table[ind].inMemory = true;
	act++;
}
#pragma endregion

int main()
{
	char **frameTable;
	VTABLE_Rec *pageTable;
	TLB_Rec* tlb;

	ifstream adrr;
	ofstream out;

	int adress, count,a, ad, page, offset, iter = 0,activePages = 0;
	

	pageTable = initVirtualTable(VTABLE_SIZE);
	frameTable = initFrameTable(FTABLE_SIZE);
	tlb = initTLB(TLB_SIZE);
	

	setlocale(LC_ALL, "Rus");
	openResultFile("res.txt", &out);
	adrr.open("v_adress.dat",ios::binary);
	if (adrr.is_open())
	{
		cout << "OPEN" << endl;
		while (!adrr.eof())
		{
			int  offs, page, frame;
			getVirtualAdress(&page, &offs, &adrr); //читать виртуальный адрес
			if ((frame = TLBHit(tlb,page,iter))>0) //если есть в TLB
			{
				char c = getCharacter(frameTable, frame, offs); //находим символ в таблице
				pushCharToFile(c, &out);						//пишем в результирующий файл
				cout << "SYMBOL: " << c << endl;
			}
			else // если нет в TLB
			{
				if ((frame = VTableHit(pageTable,page,iter))>0) //если есть в таблице страниц
				{
					int index = findPlaceInTLB(tlb, iter); //находим место в TLB
					TLB_Rec r = fillTLBRecord(iter,page,frame); //заполн€ем структуру записи в TLB
					writeToTLB(tlb, index, r); //пишем в TLB за место найденной 
					char c = getCharacter(frameTable, r.block, offs); //находим символ в таблице
					pushCharToFile(c, &out);			//пишем в результирующий файл
					cout << "SYMBOL: " << c << endl;
				}
				else //если нет в таблице страниц
				{
					int pushInd = -1;
					int &act = activePages,     //активных страниц
						&pInd=pushInd;			//вытесненна€ страница
					bool dislogeFlag = false;	//вытенс€ть запись из таблицы или нет;
					bool &push = dislogeFlag;

					int block = findPlaceInVtable(pageTable, iter, act,push,pInd);  //ищем страничный блок
					if (block < 0)			
						throw exception("Ќе удалось найти место в таблице");  
					else
					{
						getDataFromDisk("disk.txt", page, frameTable, block);
						writeToVtable(pageTable, page, block, iter, act);
						TLB_Rec r = fillTLBRecord(iter, page, block);
						if (push)
						{
							int ind = SynchTLB(tlb, r, pInd);
							writeToTLB(tlb, ind, r);
							char c = getCharacter(frameTable, r.block, offs); //находим символ в таблице
							pushCharToFile(c, &out);			//пишем в результирующий файл
							cout << "SYMBOL: " << c << endl;
						}
						else
						{
							int ind = SynchTLB(tlb, r, NOPAGE);
							writeToTLB(tlb, ind, r);
							char c = getCharacter(frameTable, r.block, offs); //находим символ в таблице
							pushCharToFile(c, &out);			//пишем в результирующий файл
							cout << "SYMBOL: " << c << endl;
						}
					}
				}
			}	
			iter++;
			getchar();
		}
	}
	else
	{
		throw invalid_argument("Wrong");
	}
}

