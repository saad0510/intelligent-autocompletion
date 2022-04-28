#include <iostream>
#include <set>
#include <time.h>
#include <windows.h>
#include "Indexer/Indexer.hpp"

using namespace std;

bool has_read{false};
Indexer indexer;

DWORD WINAPI readIndex(LPVOID lpParam)
{
    indexer.read_from("index.txt");
    has_read = true;
    ExitThread(0);
}

int main()
{
    // read index
    HANDLE hThread = CreateThread(0, 0, readIndex, 0, 0, 0);
	
    while (true)
    {
        system("cls");

        char in;
		string ctx;
        string query;
		printf("enter a line > ");
        
		while ((in = getchar()) != '\n')
        {
            if (in == ' ')
            {
                ctx = query;
                query.clear();
            }
            else
                query.push_back(in);
        }

        while (has_read == false)
			;

        set<string> results;

        if (!ctx.empty())
            indexer.complete_line(ctx, query, results);

        for (auto &result : results)
            puts(result.c_str());

        puts("\n\nPress 'e' to exit");
        fflush(stdin);
        if (getchar() == 'e')
			break;
    }
	
    CloseHandle(hThread);
    return 0;
}