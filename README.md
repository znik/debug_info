# DEBUG_INFO

### SUMMARY

DEBUG_INFO extracts debug information from the binary and provides information about C++/C types as follows:

For test_bin.cpp
```C++
01 struct test_struct_s { int fields[4]; };
02
03 int main(int argc, char *argv[]) {
04  test_struct_s str;
05  test_struct_s *const ptr = &str;
06  (void)str;
07  (void)ptr;
08 }
```
built as `g++ -g test_bin.cpp -o test_bin`

in the file where we want to get information about types that were used in test_bin.cpp:
```C++
VarInfo vi("/path/to/bin/test_bin");
const std::string& var_type = vi.type("/path/to/src/test_bin.cpp", 6, "ptr");
const std::string& field_name = vi.fieldname("/path/to/src/test_bin.cpp", 6, "ptr", sizeof(int));

<output var_type, field_name>
```

and get the following output:
```C++
type is:       "test_struct_s *const"
fieldname is:  "fields[1]"
```

### HOW TO USE DEBUG_INFO LIBRARY

1. Install libelf-dev and libdwarf-dev packages that are required for the library and build the static library.
```
% ./install.sh.
% ./make
```

2. Add reference to the library to your Makefile as follows:
```
  CXXLIBS += -L. -ldebug_info
```
3. Add debug_info dependencies to the Makefile *after* -ldebug_info:
```
  include debug_info.deps
```
4. Add '#include "varinfo.hpp"' to the file where variable type info is required.

5. The following code will do the job:
```C++
 #include "varinfo.hpp"
 ...
 VarInfo vi;
 if (!vi.init(path_to_binary)) { ... }
 ...
 const std::string& var_type = vi.type(src_file_path, line_in_file, var_name);
 const std::string& field_name = vi.fieldname(src_file_path, line_in_file, var_name, field_offset);
```

6. To see how everything works, see Makefile.main and run the test:

```
% make -f Makefile.test && make -f Makefile.main
% ./main test inst 13 0
```

### Paths

1. Path to the binary should be a full system path such as "/home/test/projects/debug_info/test".
2. Path to the source file is a full system path to the source file but its prefix is a full path to the Makefile that was used to create the binary.

For instance, for the following configuration:
```
% ls /home/test/project/my_app/src
my_app.cpp

%ls /home/test/project/my_app/build
Makefile
```

Source path should start with a path to the Makefile /home/test/project/my_app/build/, then goes the suffix ../src/my_app.cpp:
```C++
"/home/test/projects/my_app/build/../src/my_app.cpp"
```





