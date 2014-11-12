typedef int my_type_t;

struct my_struct_s {
	double x, y, z;
};

class my_class_s {

};

int main(int argc, char *argv[]) {

	my_class_s v1;
	int *const v2 = 0;
	const char * v3 = 0;
	const int &v4 = 2;
	volatile char v6;
	static double *	v7;
	volatile char **const v8 = 0;
	const my_type_t & v9 = my_type_t();
	my_struct_s* v10;
	auto v11 = v10->x;
	
	return 1;
}
