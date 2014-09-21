int main(int argc, char *argv[]) {

	int v1;
	int *const v2 = &v1;
	const char * v3 = 0;
	int &v4 = v1;
	const int &v5 = v1;
	volatile char v6;
	static double *	v7;
	volatile char **const v8 = 0;
	
	return 1;
}
