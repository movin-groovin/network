
// g++ -std=c++11 boost_reg_tst2.cpp -o boost_reg_tst2 -I/usr/local/include/boost/ -L/usr/local/lib/boost/static -lboost_regex-gcc47-mt-1_56

#include <iostream>
#include <fstream>
#include <string>

#include <cerrno>
#include <cstring>

#include <boost/regex.hpp>



int main (int argc, char *argv[]) {
	try {
		//std::string strDat ("asuser:root ps lax");
		std::string strDat ("cmd par1 \"par2\" par3 \"par4\"");
		//boost::regex regExp ("(?<=asuser:)\\w{1,100}");
		//std::string cmd ("(?:\\w|\\d|[\\-])+");
		//std::string spcs ("(?:\"[ ]*|\\t*\")");
		boost::regex regExp1 ("((?:\\w|\\d|[\\-])+)");
		boost::regex regExp2 ("\"(?>[^\"])\"");
		boost::match_results <std::string::const_iterator> resMatch;
		std::string::const_iterator it;
		
		
		if (boost::regex_search (strDat.cbegin (), strDat.cend (), resMatch, regExp1)) {
			std::cout << "Size: " << resMatch.size () << ", max size: " << resMatch.max_size () << "\n";
			std::cout << "Result: " << std::string (resMatch[0].first, resMatch[0].second) << std::endl;
			std::cout << "Match 1: " << std::string (resMatch[1].first, resMatch[1].second) << "\n\n";

			it = resMatch[0].second;
			while (true) {
				char ch = *it;
				if (ch != ' ' && ch != '\t') break;
				else ++it;
			}
		}
		else {
			std::cout << "Matches haven't found\n";
			return 1;
		}
		
		while (boost::regex_search (it, strDat.cend (), resMatch, regExp2)) {
			std::cout << "Result: " << std::string (resMatch[0].first, resMatch[0].second) <<std::endl;
			std::cout << "Match 1: " << std::string (resMatch[1].first, resMatch[1].second) << std::endl;
			std::cout << "Match 2: " << std::string (resMatch[2].first, resMatch[2].second) << std::endl;
			std::cout << "Match 3: " << std::string (resMatch[3].first, resMatch[3].second) << std::endl;
			
			it = resMatch[0].second;
			while (true) {
				char ch = *it;
				if (ch != ' ' && ch != '\t') break;
				else ++it;
			}
		}
		
		std::cout << "\n\n\n";
	} catch (std::runtime_error & rntExc) {
		std::cout << "Runtime error: " << rntExc.what () << std::endl;
	}
	std::cout << "=====================================================================\n";
	
	
	return 0;
}












