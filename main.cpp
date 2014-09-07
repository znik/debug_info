#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <libelf.h>
#include <libdwarf.h>
#include <gelf.h>

#include <cstdio>
#include <memory>
#include <vector>
#include <string>
#include <map>


static int die_stack_indent_level = 0;


static void print_attribute(Dwarf_Debug dbg, Dwarf_Die die,
	Dwarf_Half attr, Dwarf_Attribute attr_in, int die_indent_level,
	char **srcfiles, Dwarf_Signed cnt) {

	const char *v = 0;
	Dwarf_Error_s *err;

	int res = dwarf_get_AT_name(attr, &v);
	if (DW_DLV_OK != res) {
		printf("Failed to get attribute's name\n");
		return;
	}

	#define SEQ(s) (0== strcmp(v, s))

	bool to_print = false;	
	if (SEQ("DW_AT_name") || SEQ("DW_AT_MIPS_linkage_name") ||
		SEQ("DW_AT_decl_file") || SEQ("DW_AT_decl_line") ||
		SEQ("DW_AT_type")) {

		printf("%*s%s : ", die_indent_level, " ", v);
		to_print = true;
	}
	
	int sres = 0;
	if (SEQ("DW_AT_name") || SEQ("DW_AT_MIPS_linkage_name")) {
		char *name = 0;
		sres = dwarf_formstring(attr_in, &name, &err);
		if (DW_DLV_OK != sres) {
			printf("failed to read string attribute\n");
			return;
		}

		printf("\"%s\" ", name);
	} else if (SEQ("DW_AT_decl_file") || SEQ("DW_AT_decl_line")) {
		Dwarf_Signed val = 0;
		sres = dwarf_formsdata(attr_in, &val, &err);
		if (DW_DLV_OK != sres) {
			printf("failed to read data attribute\n");
			return;
		}
		
		if (SEQ("DW_AT_decl_file")) {
			printf("\"%s\" ", srcfiles[val - 1]);
		}
		else if (SEQ("DW_AT_decl_line")) {
			printf("\"%lli\" ", val);
		}
	}
	else if (SEQ("DW_AT_type")) {
		Dwarf_Off offset = 0;
		sres = dwarf_formref(attr_in, &offset, &err);
		if (DW_DLV_OK != sres) {
			printf("failed to read ref attribute\n");
			return;
		}
		
		printf("<0x%08llu> ", offset);
	}

	if (to_print)
		printf("\n");
}


static bool print_one_die(Dwarf_Debug dbg, Dwarf_Die die,
	int die_indent_level, char **srcfiles, Dwarf_Signed cnt) {

	Dwarf_Error_s *err;
	Dwarf_Half tag = 0;

	int tres = dwarf_tag(die, &tag, &err);
	if (DW_DLV_OK != tres) {
		printf("Failed to obtain the tag\n");
		return false;
	}

	const char * tagname = 0;
	int res = dwarf_get_TAG_name(tag, &tagname);
	if (DW_DLV_OK != res) {
		printf("Failed to get the name of the tag\n");
		return false;
	}

	printf("\n%*s%s ", die_indent_level, " ", tagname);

	// REF: print_die.c : 1028

	Dwarf_Off offset = 0;	
	res = dwarf_die_CU_offset(die, &offset, &err);
	if (DW_DLV_OK != res) {
		printf("Failed to get die CU offset\n");
		return false;
	}

	printf("<0x%08llu>", offset);
	printf("\r\n");

	Dwarf_Signed atcnt = 0;
	Dwarf_Attribute *atlist = 0;
	int atres = dwarf_attrlist(die, &atlist, &atcnt, &err);
	if (DW_DLV_ERROR == atres) {
		printf("Error while getting the attributes\n");
	}
	else if (DW_DLV_NO_ENTRY == atres) {
		atcnt = 0;
	}


	for (Dwarf_Signed i = 0; i < atcnt; ++i) {
		Dwarf_Half attr;
		int ares = dwarf_whatattr(atlist[i], &attr, &err);
		if (DW_DLV_OK != ares) {
			printf("<Cannot get attributes>\n");
			continue;
		}
		printf("%*s", die_indent_level + 1, " ");
		print_attribute(dbg, die, attr, atlist[i], die_indent_level,
			srcfiles, cnt);
	}

	for (Dwarf_Signed i = 0; i < atcnt; ++i) {
		dwarf_dealloc(dbg, atlist[i], DW_DLA_ATTR);
	}
	if (DW_DLV_OK == atres)
		dwarf_dealloc(dbg, atlist, DW_DLA_LIST);
	return true;
}


static void print_die_and_children(Dwarf_Debug dbg,
	Dwarf_Die in_die_in, Dwarf_Bool is_info, char **srcfiles,
	Dwarf_Signed cnt) {

	// REF print_die.c : 640	
	Dwarf_Die in_die = in_die_in;
	Dwarf_Error_s *err;
	Dwarf_Die child = 0;
	Dwarf_Die sibling = 0;

	int cdres = 0;

	for (;;) {
		print_one_die(dbg, in_die, die_stack_indent_level, srcfiles, cnt);
		
		cdres = dwarf_child(in_die, &child, &err);
		if (DW_DLV_OK == cdres) {
			++die_stack_indent_level;
			print_die_and_children(dbg, child, is_info, srcfiles, cnt);
			--die_stack_indent_level;
			dwarf_dealloc(dbg, child, DW_DLA_DIE);
			child = 0;
		}
		else if (DW_DLV_ERROR == cdres) {
			printf("Error while obtaining a child\n");
			return;
		}
		
		cdres = dwarf_siblingof_b(dbg, in_die, is_info, &sibling, &err);
		if (DW_DLV_ERROR == cdres) {
			printf("Failed to get a sibling\n");
			return;
		}

		if (in_die != in_die_in) {
			dwarf_dealloc(dbg, in_die, DW_DLA_DIE);
			in_die = 0;
		}
		if (DW_DLV_OK == cdres)
			in_die = sibling;
		else
			break;
	}
}


static int print_info(Dwarf_Debug &dbg, int is_info) {

	Dwarf_Error_s *err;
	Dwarf_Unsigned cu_header_length = 0;
	Dwarf_Half version_stamp = 0;
	Dwarf_Unsigned abbrev_offset = 0;
	Dwarf_Half address_size = 0;
	Dwarf_Half length_size = 0;
	Dwarf_Half extension_size = 0;
	Dwarf_Sig8 signature;
	Dwarf_Unsigned typeoffset = 0;
	Dwarf_Unsigned next_cu_offset = 0;

	if (is_info)
		printf("[[Section .debug_info]]\n");
	else
		printf("[[Section .debug_types]]\n");

	unsigned iteration = 0;
	int nres = 0;
	int sres = DW_DLV_OK;
	Dwarf_Die cu_die = 0;

	// REF print_die.c : 400	
	for (;;++iteration) {
		printf("*\n");

		nres = dwarf_next_cu_header_c(dbg, is_info, &cu_header_length,
			&version_stamp, &abbrev_offset, &address_size, &length_size,
			&extension_size, &signature, &typeoffset, &next_cu_offset,
			&err);

		if (DW_DLV_NO_ENTRY == nres || DW_DLV_OK != nres)
			return nres;

		sres = dwarf_siblingof_b(dbg, NULL, is_info, &cu_die, &err);
		if (DW_DLV_OK != sres) {
			printf("error in reading siblings");
			return sres;
		}
		
		Dwarf_Signed cnt = 0;
		char **srcfiles = 0;
		int srcf = dwarf_srcfiles(cu_die, &srcfiles, &cnt, &err);
		if (DW_DLV_OK != srcf) {
			srcfiles = 0;
			cnt = 0;
		}
		print_die_and_children(dbg, cu_die, is_info, srcfiles, cnt);
		if (DW_DLV_OK == srcf) {
			for (int si = 0; si < cnt; ++si)
				dwarf_dealloc(dbg, srcfiles[si], DW_DLA_STRING);
			dwarf_dealloc(dbg, srcfiles, DW_DLA_LIST);
		}
		dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
		cu_die = 0;
	}
};


static int collect_vars_info(Elf * elf) {
	Dwarf_Debug dbg;
	Dwarf_Error_s *err;
	int dres = dwarf_elf_init(elf, DW_DLC_READ, NULL, NULL, &dbg, &err);
	if (DW_DLV_NO_ENTRY == dres) {
		printf("No DWARF information.\n");
		return 0;
	}
	if (DW_DLV_OK != dres) {
		printf("error reading DWARF info\n");
		return 0;
	}
	
	print_info(dbg, 0);
	print_info(dbg, 1);	

	dwarf_finish(dbg, &err);
	return 1;
};


int parse_debug_info(int fd) {

	if (elf_version(EV_CURRENT) == EV_NONE) {
		printf("libelf.a is out of date\n");
	}

	Elf * elf = elf_begin(fd, ELF_C_READ, NULL);
	if (ELF_K_AR == elf_kind(elf)) {
		printf("the file is an archieve\n");
		close(fd);
		return 0;
	}
	Elf *f_elf = elf;
	// FIXME: check the there is an ELF32 or ELF64 header
	Elf_Cmd cmd = ELF_C_READ;
	while(0 != (elf = elf_begin(fd, cmd, elf))) {
		collect_vars_info(elf);
		cmd = elf_next(elf);
		elf_end(elf);
	}
	elf_end(f_elf);
	return 1;
};



int main(int argc, char *argv[]) {

	if (argc < 2) {
		printf("Usage: %s <file_compiled_with_dash_g>\n", argv[0]);
		return 0;
	}

	const char * file_name = argv[1];

	int fd = open(file_name, O_RDWR);
	if (-1 == fd) {
		printf("cannot find the file to open..\n");
		return 0;
	}

	struct stat elf_stats;
	if ((fstat(fd, &elf_stats))) {
		printf("cannot stat the file\n");
		return 0;
	}

	int e = parse_debug_info(fd);
	close(fd);
	return e;
}



