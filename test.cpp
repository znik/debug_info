#include "test.h"

inline void foo(int& a) {
	 a+=1;

}

int main(int argc, char *argv[]) {
	int a = infoo(2);
	{
		int b;
		foo(a);
	}
	return 1;
}
