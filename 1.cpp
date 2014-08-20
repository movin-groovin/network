
#include <iostream>


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
	
	
	
	return 0;
}
