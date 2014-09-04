
// g++ boost_reg_tst1.cpp -o boost_reg_tst1 -I/usr/local/include/boost-1_54/ -L/usr/local/lib/boost1_54_0/ -lboost_regex-gcc47-mt-s-1_54

#include <iostream>
#include <fstream>
#include <string>

#include <cerrno>
#include <cstring>

#include <boost/regex.hpp>



int main (int argc, char *argv[]) {
	try {
		//std::string strDat ("asuser:root ps lax");
		std::string strDat ("user2 = gangnam:coolman1983\n");
		//boost::regex regExp ("(?<=asuser:)\\w{1,100}");
		boost::regex regExp ("(?:[^#]|^)user\\d+([ ]|\\t)*=(?1)*((\\w|\\d)*):((?3)*)\\n");
		boost::match_results <std::string::iterator> resMatch;
		std::string::iterator it = strDat.begin ();
		
		while (boost::regex_search (it, strDat.end (), resMatch, regExp)) {
			std::cout << "Size: " << resMatch.size () << ", max size: " << resMatch.max_size () << "\n";
			std::cout << "Result: " << std::string (resMatch[0].first, resMatch[0].second) <<std::endl;
			std::cout << "Match 1: " << std::string (resMatch[1].first, resMatch[1].second) << std::endl;
			std::cout << "Match 2: " << std::string (resMatch[2].first, resMatch[2].second) << std::endl;
			std::cout << "Match 3: " << std::string (resMatch[3].first, resMatch[3].second) << std::endl;
			std::cout << "Match 4: " << std::string (resMatch[4].first, resMatch[4].second) << std::endl;
			
			it = resMatch[0].second;
		}
		
		std::cout << "\n\n\n";
		
		boost::regex regExp1 ("[ ]+/+");
		std::cout << "Second ret: " << boost::regex_search (strDat.begin (), strDat.end (), regExp1) << std::endl;
	} catch (std::runtime_error & rntExc) {
		std::cout << "Runtime error: " << rntExc.what () << std::endl;
	}
	std::cout << "=====================================================================\n";
	
	
	return 0;
}












