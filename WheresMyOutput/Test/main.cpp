#include <Windows.h>
#include <iostream>

//
//Test program
//

using namespace std;

int main()
{
	while (1){
		cout << "cout message\n";
		wcout << "wcout message\n";
		cerr << "cerr message\n";
		wcerr << "wcerr message\n";
		printf("printf message\n");
		fprintf(stderr, "fprintf stderr messsage\n");
		OutputDebugStringA("OutputDebugStringA message\n");
		OutputDebugStringW(L"OutputDebugStringW message\n");
		Sleep(20000);
	}
	return 0;
}