
// g++ -std=c++11 boost_reg_tst2.cpp -o boost_reg_tst2 -I/usr/local/include/boost/ -L/usr/local/lib/boost/static -lboost_regex-gcc47-mt-1_56
// g++ -std=c++11 boost_reg_tst2.cpp -o boost_reg_tst2 -I/usr/local/include/boost1_54_0/ -L/usr/local/lib/boost1_54_0/ -lboost_regex-gcc47-mt-s-1_54

#include <iostream>
#include <fstream>
#include <string>

#include <cerrno>
#include <cstring>

#include <boost/regex.hpp>



int main (int argc, char *argv[]) {
	try {
		//std::string strDat1 ("notify-send -t 2000 -i skype \"Microsoft corporation\" \"<b>data () date</b>\"");
		std::string strDat1 ("ls -l");
		strDat1 += "    "; // at the end of a string must exist mandatory one space
		boost::regex regExp1 ("(?:\\d|\\w|[-.()])+(?=[ ])|\"(?:\\d|\\w|[ /\\<\\>!@#$%^&*()])+\"(?=[ ])");
		boost::match_results <std::string::const_iterator> resMatch;
		std::string::const_iterator it;
		
		it = strDat1.cbegin ();
		while (boost::regex_search (it, strDat1.cend (), resMatch, regExp1)) {
			std::cout << "Result: " << std::string (resMatch[0].first, resMatch[0].second) <<std::endl;
			std::cout << "Match 1: " << std::string (resMatch[1].first, resMatch[1].second) << std::endl;
			std::cout << "Match 2: " << std::string (resMatch[2].first, resMatch[2].second) << std::endl;
			std::cout << "Match 3: " << std::string (resMatch[3].first, resMatch[3].second) << std::endl;
			
			it = resMatch[0].second;
		}
		
		std::cout << "\n\n\n";
		/*
		boost::regex regExp2 ("\"([^\"]++|\"(?R)\")*\"");
		std::string strDat2 ("bash -c \"python -c \"import os; os.system (\"ls -al\")\"\" \"qwerty\"");

		it = strDat2.cbegin ();
		while (boost::regex_search (it, strDat2.cend (), resMatch, regExp2)) {
			std::cout << "Result: " << std::string (resMatch[0].first, resMatch[0].second) <<std::endl;
			std::cout << "Match 1: " << std::string (resMatch[1].first, resMatch[1].second) << std::endl;
			std::cout << "Match 2: " << std::string (resMatch[2].first, resMatch[2].second) << std::endl;
			std::cout << "Match 3: " << std::string (resMatch[3].first, resMatch[3].second) << std::endl;
			
			it = resMatch[0].second;
		}
		*/
	} catch (std::runtime_error & rntExc) {
		std::cout << "Runtime error: " << rntExc.what () << std::endl;
	}
	std::cout << "=====================================================================\n";
	
	
	return 0;
}












