#pragma once
#ifndef INDEXOR_HPP
#define INDEXOR_HPP

#include <set>
#include <cmath>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include "Tries/Trie.hpp"

#define TOTAL_DOCS (248153) // total number of docs
using namespace std;

// inverted index class 
class Indexer
{	
	using DocScore = pair<Document*, float>;
	const char* datasetDir = "./dataset/";
	
private:
    Trie dictionary;
	unsigned total_docs;		
	
public:
	// constructor:
    Indexer(unsigned total_docs = 1) : total_docs(total_docs) {}
	
	// add the given document in index
    void index(const char *filename, unsigned doc_ID = 0)
    {	
		string word;
        bool go{true};
        unsigned line_no{0};
		char c1, c2, findChar{0};
		HashEntry *target{nullptr};
		
        FILE *file = fopen(filename, "r"); // open file in read mode
    
	    while (true)
        {
			// read 2 characters:
			c1 = fgetc(file);
			if (c1 == EOF)
				break;	// reading completed?
			
            c2 = fgetc(file);
            ungetc(c2, file);	// move file pointer back

            if (c1 == '\n')		// a new line?
                line_no++;

            if (go == false)	// skip till findchar
            {
                if (c1 == findChar)
                {
                    if (findChar != '*')	// end of multiline comment
                    {
                        go = true;
                        word.clear();
                    }
                    else if (c2 == '/')		// end of inline comment
                    {
                        go = true;
                        word.clear();
                        fgetc(file);
                    }
                }
                continue;
            }
			
			// remove strings enclosed in '' "":
            if (c1 == '\"' || c1 == '\'')
            {
                findChar = c1;
                go = false;
                continue;
            }

			// remove comments:
            if (c1 == '/')
            {
                if (c2 == '/')			// inline comment?
				{
					findChar = '\n';	// skip till new line
					go = false;
					continue;					
				}
				else if (c2 == '*')		// multi line comment?
				{
					findChar = '*';		// skip till * of */
					go = false;
					continue;					
				}
            }

            c1 |= 32;	// convert to lowecase
            
			if (c1 >= 'a' && c1 <= 'z')		// push all alphabets
                word.push_back(c1);
            else							// a break-point? like ' '
            {
                if (word.length() > 3)		//  minimum length of word 
                {
                    target = dictionary.insert(word);	// add the word in index
                    if (target->posting)				// posting already exits?
                        target->posting->push_directly(doc_ID, line_no);	// update posting
                    else								// no posting?
                        target->posting = new Posting(doc_ID, line_no);	// add a new posting
                }
                word.clear();
            }
        }
        fclose(file);
    }
	
	// write the index on given file
    void write_on(const char *filename)
    {
        ofstream file;
        file.open(filename, ios::out);		// open in write mode
        dictionary.write(file);				// write index
        file.close();
    }
	
	// read and store the index from a file
	// opposite of write_on()
    void read_from(const char *filename)
    {
        string token;
        unsigned doc_count{0};
        unsigned doc_ID{0};
        unsigned term_freq{0};
        unsigned line_no{0};
        HashEntry *target;

        dictionary.deleteTrie();	// delete previous index

        ifstream file;
        file.open(filename, ios::in);	// open in read mode
		
        while (!file.eof())		// EOF is false?
        {
            file >> token;		// read a word
            if (file.eof())
                break;

            file >> doc_count;	// read number of documents in which the token has appeared
            if (file.eof())
                break;
			
			// read all document ids
            for (unsigned i = 0; i < doc_count; i++)
            {
                file >> doc_ID;
                if (file.eof())
                    break;
				
                file >> term_freq;	// number of times the token has appeared in this document
                if (file.eof())
                    break;
				
				// read all number of lines of doc in which the token has appeared
                for (unsigned j = 0; j < term_freq; j++)
                {
                    file >> line_no;
                    if (file.eof())
                        break;

                    target = dictionary.insert(token);	// get posting of token
                    if (target->posting)				// positing alread exits?
                        target->posting->push_directly(doc_ID, line_no);	// update posting
                    else		// posting is new?
                        target->posting = new Posting(doc_ID, line_no);		// add posting
                }
            }
        }
        file.close();
    }
	
	// return the pointer to HashEntry of last character of given word
    HashEntry *search(const string &token)
    {
        return dictionary.search(token);
    }

    // seeks to the given line of a text file
    void goToLine(ifstream &file, unsigned line)
    {
        file.seekg(0);
        
		if (line == 0)
            return;
		
        for (unsigned i = 0; i < line - 1; ++i)
            file.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    // returns tf-idf score:
    float getTfIdf(unsigned term_freq, unsigned doc_count)
    {
        return ((float)(1.0 + log10(term_freq)) * log10(total_docs / doc_count));
    }
	
	// rank top relevant documents wrf TD-IDF
    void rankDocs(const Posting *posting, vector<DocScore> &rel_docs)
    {
		// get all documents with their tfidf scores:
        for (auto doc = posting->documents.begin(); doc != nullptr; doc = doc->next)
            rel_docs.push_back(
				DocScore(
					&(doc->data),	// doc
					getTfIdf(doc->data.term_freq, posting->doc_count)	// score
				)
			);
			
		// sort wrt highest score:
        sort(rel_docs.begin(), rel_docs.end(),
            []	(const DocScore &lhs, const DocScore &rhs)
				{
					return lhs.second > rhs.second;
				}
			);
		// top 10 docs only:
        if (rel_docs.size() > 10)
            rel_docs.resize(10);
    }
	
	// get the top code snippets of the given word (posting)
    void scoreWords(const Posting *posting, set<string> &dest)
    {
        float highest = -1;
        vector<DocScore> top_docs; // holds highest ranking docs
        rankDocs(posting, top_docs);
		
        // Traverse the highest ranking docs
        for (auto doc : top_docs)
        {
            printf("%ld: \n", doc.first->ID);

            ifstream code;
            code.open(datasetDir + to_string(doc.first->ID), ios::in);

            for (auto line = doc.first->lines.begin(); line != nullptr; line = line->next)
            {
                goToLine(code, line->data + 1);

                string snippet;
                getline(code, snippet);

                auto pos = snippet.find_first_not_of(" \t"); // Trim out indents
                
				if (pos != string::npos)
                    snippet = snippet.substr(pos); // from the line of code
                if (!snippet.empty())
                    dest.insert(snippet);
            }
            code.close();
        }
    }
	
	// print the top code snippets of the incomplete string
    void searchWord(string &partialStr, const unsigned &count)
    {
        vector<Trie::Result> results;
		// top results
        dictionary.searchTopWords(partialStr, count, results);

        if (results.empty())
        {
            puts("No results were found");
            return;
        }

        set<string> words;
        for (auto &pair : results)
        {
            words.clear();
            printf("%s ", pair.first.c_str());
            printf("%ld ", pair.second->doc_count);
            printf("%ld\n", pair.second->total_count);
			// get code snippet of current word
            scoreWords(pair.second, words);
			// print
            for (auto &word : words)
                puts(word.c_str());
            puts("\n");
        }
    }

    void complete_line(string &context, string &incomplete, set<string> &dest)
    {
		// top 5 results containing the query
        vector<Trie::Result> query_res;
        dictionary.searchTopWords(incomplete, 5, query_res);
		
		if (query_res.empty())
            puts("No query results were found");

		// top 5 results containg the context
        vector<Trie::Result> ctx_res;
        dictionary.searchTopWords(context, 5, ctx_res);
		
        if (ctx_res.empty())
            puts("No context results were found");

		// get the documents which are in common between query and context
        vector<tuple<Document *, Document *, float>> final_res;		// query_doc, ctx_doc, tdidf score
        bool doc_intersected{false};
		
		// go in each query result:
        for (auto &pair : query_res)
        {
            // go in each query doc:
            for (auto query_doc = pair.second->documents.begin(); query_doc; query_doc = query_doc->next)
            {
                bool go = true;
                // go in each context result:
                for (auto &ctx_pair : ctx_res)
                {
                    // go in each context doc:
                    for (auto ctx_doc = ctx_pair.second->documents.begin(); ctx_doc; ctx_doc = ctx_doc->next)
                    {
                        // if an intersection exits:
                        if (ctx_doc->data.ID == query_doc->data.ID)
                        {
                            doc_intersected = true;

                            // tf-idf of query in common doc:
                            float score1 = getTfIdf(query_doc->data.term_freq, pair.second->doc_count);
                            // tf-idf of ctx in common doc:
                            float score2 = getTfIdf(ctx_doc->data.term_freq, ctx_pair.second->doc_count);
                            // final score:
							float final_score = score1 + score2;

                            final_res.push_back(
								tuple<Document *, Document *, float> (
									&(query_doc->data),
									&(ctx_doc->data),
									final_score
								)
							);
                            go = false;
							break;
                        }
                    }
                    if (go == false)
                        break;
                }
            }
        }
		
        if (doc_intersected == false)
        {
			puts("the words are not found together in any document");
            return;
        }
		
		// sort wrt score
        sort(final_res.begin(), final_res.end(),
             [] (
					const tuple<Document *, Document *, float> &lhs,
					const tuple<Document *, Document *, float> &rhs
				)
				{
					return get<2>(lhs) > get<2>(rhs); 
				}
			);
		
		bool line_intersected{false};
        for (auto final_doc : final_res)
        {
            printf("%ld - %ld - %.1lf\n", get<0>(final_doc)->ID, get<1>(final_doc)->ID, get<2>(final_doc));
            /// go in each query doc
			for (auto query_line = get<0>(final_doc)->lines.begin(); query_line; query_line = query_line->next)
            {
				// go in each context result
                bool go = true;
                for (auto ctx_line = get<1>(final_doc)->lines.begin(); ctx_line; ctx_line = ctx_line->next)
                {
					// if both exits on a common line?
                    if (query_line->data == ctx_line->data)
                    {
                        line_intersected = true;
						
                        ifstream code;
                        code.open(datasetDir + to_string(get<0>(final_doc)->ID), ios::in);

                        goToLine(code, query_line->data + 1);

                        string snippet;
                        getline(code, snippet);

                        auto pos = snippet.find_first_not_of(" \t"); // Trim out indents
                        if (pos != string::npos)
                            snippet = snippet.substr(pos); // from line of code

                        if (!snippet.empty())
                        {
                            dest.insert(snippet);
                        }
                        code.close();
                        go = false;
						break;
                    }
                }
            }
        }
		
		// if no they donot exits on a common line, show context resutls
        if (line_intersected == false)
        {
            for (auto &final_doc : final_res)
            {
                for (auto ctx_line = get<1>(final_doc)->lines.begin(); ctx_line; ctx_line = ctx_line->next)
                {
                    ifstream code;
                    code.open(datasetDir + to_string(get<0>(final_doc)->ID), ios::in);

                    goToLine(code, ctx_line->data + 1);

                    string snippet;
                    getline(code, snippet);

                    auto pos = snippet.find_first_not_of(" \t"); // Trim out indents
                    if (pos != string::npos)
                        snippet = snippet.substr(pos); // from line of code

                    if (!snippet.empty())
                    {
                        dest.insert(snippet);
                    }
                    code.close();
                }
            }
            return;
        }
    }
};
#endif
