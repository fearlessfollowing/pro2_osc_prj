#include <iostream>
#include <vector>
#include <string.h>

using namespace std;


typedef unsigned int u32;

std::string extraAbsDirFromUri(std::string fileUrl)

{

	const char *delim = "/";

	std::vector<std::string> vUris;

	char cUri[1024] = {0};



	strncpy(cUri, fileUrl.c_str(), (fileUrl.length() > 1024)? 1024: fileUrl.length());

    char* p = strtok(cUri, delim);

    while(p) {

		std::string tmpStr = p;

		vUris.push_back(tmpStr);

        p = strtok(NULL, delim);

    }



	if (vUris.size() < 3) {

		return "none";

	} else {

		std::string result = "/";

		u32 i = 2;

		for (i = 2; i < vUris.size() - 1; i++) {

			result += vUris.at(i);
			if (i != vUris.size() -2)
				result += "/";
		}

		return result;

	}

}

 
int main() {
	
	std::string fileUri = "http://192.168.1.1/files/150100525831424d4207a52390afc300/100RICOH/R0012284.JPG";
	std::string dirPath = extraAbsDirFromUri(fileUri);
 	std::cout << dirPath << std::endl;
	return 0;
}

