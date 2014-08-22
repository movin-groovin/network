
#include <iostream>
#include <sstream>


void func1 (int);


class COne {
public:
	//void func1 ();
	friend void func1 (int a) {std::cout << "Friend function\n";}
	
public:
	void f () {func1 (1);}
	
};



int main () {
	COne obj;
	
	
	obj.f ();
	std::string str1 ("12345");
	std::istringstream issCnv;
	
	issCnv.str (str1);
	int var;
	issCnv >> var;
	
	std::cout << "Var: " << var << std::endl;
	
	
	return 0;
}
