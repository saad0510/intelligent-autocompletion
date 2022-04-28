#include <iostream>
#include <time.h>
#include <vector>
#include <windows.h>
#include <filesystem>
#include "Indexer/Indexer.hpp"

using namespace std;

unsigned countFiles{0};
unsigned total_files{0};
bool completed{false};

// thread which prints the indexing results on screen
DWORD WINAPI printCount(LPVOID lpParam)
{
    while (completed == false)
	{
        printf("reading... %.1f percent\r", (100.0 * countFiles) / total_files);
		Sleep(1000);
	}
	ExitThread(0);
}

int main()
{
    const char *datasetDir = "./dataset/";
    const char *resultFile = "./index.txt";

    // start timer:
    time_t begin, end;
    time(&begin);
		
	// read metadeta about dataset:
	vector<pair<string, int>> files(10); 	// (filename, file_id)
	files.clear();
	
	// files should have no extension
	for (const auto &entry : filesystem::directory_iterator(datasetDir))
		files.push_back(
			pair<string, int>	(
				entry.path().string(),							// full path to file
				stoi(entry.path().filename().stem().string())	// file id
			)
		);
	
	files.shrink_to_fit();		// remove empty indexes
	total_files = files.size();
	
	// start printing thread:
    HANDLE hThread = CreateThread(0, 0, printCount, 0, 0, 0);
    
	// index all files:
    Indexer indexer(total_files);
    
	for (const auto& file : files)
    {
        indexer.index(file.first.c_str(), file.second);
		countFiles++;
    }

    // stop timer:
    time(&end);
    printf("\nTime measured: %d seconds.\n", end - begin);
    completed = true;

    // write the inverted index on given file
    indexer.write_on(resultFile);

    // exit:
    CloseHandle(hThread);
    fflush(stdin);
    return 0;
}
