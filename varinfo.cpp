///
#ifdef __linux
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif // __linux

#include <stdlib.h>
#include <string.h>

#ifdef __linux
#include <fcntl.h>
#include <libelf.h>
#include <libdwarf.h>
#include <gelf.h>
#endif // __linux

#include <cstdio>
#include <memory>
#include <vector>
#include <string>
#include <cassert>
#include <algorithm>
#include <map>

#include "varinfo.hpp"


namespace {
	// BaseTypes describe build-in system data types like "int", "char", etc.
	// Base types are identified by an offset in .debug_info section.

	typedef std::map<size_t, std::string> BaseTypes_t;

	// SrcFiles describe source files described in .debug_info section.
	typedef std::map<size_t, std::string> SrcFiles_t;

	// Variables describe every variable declared in a program
	struct Variable {
		Variable(SrcFiles_t *const srcfiles, BaseTypes_t *const basetypes) :
			_srcfiles(srcfiles), _basetypes(basetypes),
			_line(-1), _vis_ended_line(-1),
			_file_id(-1), _type_offset(-1) {};

		void setLine(size_t line) { _line = line; }
		void setFile(const std::string& file) {
			auto it = _srcfiles->end();
			for (it = _srcfiles->begin(); _srcfiles->end() != it; ++it) {
				if (file == it->second) {
					_file_id = it->first;
					break;
				}
			}
			if (_srcfiles->end() == it) {
				(*_srcfiles)[_srcfiles->size()] = file;
				_file_id = _srcfiles->size() - 1;
			}
		}
		void setVisEndLine(size_t vis_end_line) { _vis_ended_line = vis_end_line; };
		void setName(const std::string& name) { _name = name; }
		void setTypeOffset(size_t type_offset) { _type_offset = type_offset; }

		size_t line() const { return _line; }
		size_t visEndsLine() const { return _vis_ended_line; }
		const std::string& file() {
			return (*_srcfiles)[_file_id];
		}
		const std::string& name() const { return _name; }
		const std::string type() const { return (*_basetypes)[_type_offset]; }

	private:
		SrcFiles_t*		_srcfiles;
		BaseTypes_t*	_basetypes;

		size_t		_line;			// declaration line
		size_t		_vis_ended_line;// line where local visibility of the var ends
		size_t		_file_id;		// declaration file id (@sa SrcFiles_t::first)
		std::string	_name;			// variable name
		size_t		_type_offset;	// type description offset (@sa BaseTypes_t::first)
	};

	typedef std::vector<Variable> Vars_t;
};


class VarInfo::Imp {
public:
	bool init(const std::string&);
	const std::string type(const std::string& file, const size_t line,
		const std::string& name) const {
		
		std::map<size_t, Variable*> variants;
		for (unsigned i = 0; i < _vars.size(); ++i) {
			Variable *const v = const_cast<Variable *const>(&_vars[i]);
			if (v->line() <= line && line <= v->visEndsLine() &&
				 name == v->name() && file == v->file()) {
				
				variants[v->line()] = v;
			}
		}
#ifndef __linux
#pragma warning(suppress : 4172)
#endif
		// FIXME last element is required !!!
		if (0 != variants.size())
			return std::max_element(variants.begin(), variants.end())
				->second->type();
		return "<Unknown>";
	}

private:
	Variable& newVar() {
		_vars.push_back(Variable(&_src_files, &_base_types));
		return _vars[_vars.size() - 1];
	}

	std::string& newBaseType(const size_t offset) {
		return _base_types[offset];
	}

private:
	Vars_t		_vars;
	SrcFiles_t	_src_files;
	BaseTypes_t	_base_types;

#ifdef __linux
private:
	typedef std::map<Dwarf_Addr, Dwarf_Unsigned> addr2line_mapper_t;
	std::map<std::string, addr2line_mapper_t> _pcaddr2line;

	int _die_stack_indent_level;
	int _vis_end_line;
	
	void print_attribute(Dwarf_Debug dbg, Dwarf_Die die,
		Dwarf_Half attr, Dwarf_Attribute attr_in, int die_indent_level,
		char **srcfiles, const char *cfile, Dwarf_Signed cnt, Variable *const var = 0,
		std::string *const basetype = 0) {

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
			SEQ("DW_AT_type") || SEQ("DW_AT_low_pc") || SEQ("DW_AT_high_pc")) {

		printf("%*s%s : ", 2 * die_indent_level, " ", v);
		to_print = true;
	}
	
	int sres = 0;
	if (SEQ("DW_AT_name")) {
		char *name = 0;
		sres = dwarf_formstring(attr_in, &name, &err);
		if (DW_DLV_OK != sres) {
			printf("failed to read string attribute\n");
			return;
		}
		printf("\"%s\" ", name);
		if (!!var) {
			var->setName(name);	
			var->setVisEndLine(_vis_end_line);
		} else {
			 if (!!basetype)
				*basetype = name;
		}

	} else if (SEQ("DW_AT_decl_file") || SEQ("DW_AT_decl_line")) {
		Dwarf_Signed val = 0;
		Dwarf_Unsigned uval = 0;
		sres = dwarf_formudata(attr_in, &uval, &err);

		if (DW_DLV_OK != sres) {
			sres = dwarf_formsdata(attr_in, &val, &err);
			if (DW_DLV_OK != sres) {
				printf("failed to read data attribute\n");
				return;
			}
			if (SEQ("DW_AT_decl_file")) {
				cfile = srcfiles[val - 1];
				if (!!var)
					var->setFile(cfile);
				printf("\"%s\" ", cfile);
			}
			else if (SEQ("DW_AT_decl_line")) {
				printf("\"%lli\" ", val);
				if (!!var)
					var->setLine(val);
			}
		}
		
		if (SEQ("DW_AT_decl_file")) {
			cfile = srcfiles[uval - 1];
			if (!!var)
				var->setFile(cfile);
			printf("\"%s\" ", cfile);
		}
		else if (SEQ("DW_AT_decl_line")) {
			printf("\"%lli\" ", uval);
			if (!!var)
				var->setLine(uval);
		}
	}
	else if (SEQ("DW_AT_low_pc") || SEQ("DW_AT_high_pc")) {
		Dwarf_Addr addr = 0;
		sres = dwarf_formaddr(attr_in, &addr, &err);
		if (DW_DLV_OK != sres) {
			printf("failed to read address attribute\n");
		}
		_vis_end_line = _pcaddr2line[cfile][addr];
		printf("line:%llu \"0x%08llx\" ", _vis_end_line, addr);
	}
	else if (SEQ("DW_AT_type")) {
		Dwarf_Off offset = 0;
		sres = dwarf_formref(attr_in, &offset, &err);
		if (DW_DLV_OK != sres) {
				printf("failed to read ref attribute\n");
				return;
			}
			if (!!var)
				var->setTypeOffset(offset);
			printf("<0x%08llu> ", offset);
		}

		if (to_print)
			printf("\n");
	}

	bool print_one_die(Dwarf_Debug dbg, Dwarf_Die die,
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

		printf("\n%*s[%d]%s ", 2 * die_indent_level, " ", die_indent_level, tagname);

		Variable *var = 0;
		std::string *basetype = 0;

		Dwarf_Off offset = 0;	
		res = dwarf_die_CU_offset(die, &offset, &err);
		if (DW_DLV_OK != res) {
			printf("Failed to get die CU offset\n");
			return false;
		}

		if (0 == strcmp(tagname, "DW_TAG_variable")) {
			var = &newVar();
		} else if (0 == strcmp(tagname, "DW_TAG_base_type")) {
			basetype = &newBaseType(offset);
		}

		// REF: print_die.c : 1028

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
			printf("%*s", 2 * die_indent_level + 1, " ");
			print_attribute(dbg, die, attr, atlist[i], die_indent_level,
				srcfiles, srcfiles[0], cnt, var, basetype);
		}

		for (Dwarf_Signed i = 0; i < atcnt; ++i) {
			dwarf_dealloc(dbg, atlist[i], DW_DLA_ATTR);
		}
		if (DW_DLV_OK == atres)
			dwarf_dealloc(dbg, atlist, DW_DLA_LIST);
		return true;
	}

	void print_die_and_children(Dwarf_Debug dbg,
		Dwarf_Die in_die_in, Dwarf_Bool is_info, char **srcfiles,
		Dwarf_Signed cnt) {

		// REF print_die.c : 640	
		Dwarf_Die in_die = in_die_in;
		Dwarf_Error_s *err;
		Dwarf_Die child = 0;
		Dwarf_Die sibling = 0;

		int cdres = 0;

		for (;;) {
			print_one_die(dbg, in_die, _die_stack_indent_level, srcfiles, cnt);
		
			cdres = dwarf_child(in_die, &child, &err);
			if (DW_DLV_OK == cdres) {
				++_die_stack_indent_level;
				print_die_and_children(dbg, child, is_info, srcfiles, cnt);
				--_die_stack_indent_level;
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

	
	void print_line_numbers_info(Dwarf_Debug dbg, Dwarf_Die cu_die) {

		Dwarf_Error_s *err;
		Dwarf_Line *linebuf = NULL;
		Dwarf_Signed linecount = 0;

		int lres = dwarf_srclines(cu_die, &linebuf,
			&linecount, &err);
		switch(lres) {
		case DW_DLV_ERROR:
			printf("Error in reading line numbers information\n");
			return;
		case DW_DLV_NO_ENTRY:
			printf("No line numbers information\n");
			return;
		default:;
		}

		int ares = 0, sres = 0;
		Dwarf_Unsigned old_lineno = 0;
		printf("\n\n**PC-ADDRESSES ASSOCIATION\n");
		Dwarf_Addr pc = 0;
		Dwarf_Unsigned lineno = 0;
		char * filename = 0;
		for (Dwarf_Signed i = 0; i < linecount; ++i) {
			Dwarf_Line line = linebuf[i];
			filename = (char *)"<unknown>";
			sres = dwarf_linesrc(line, &filename, &err);
			if (DW_DLV_ERROR == sres) {
				printf("cannot read a source file that corresponds " 
					"to the line\n");
			} else {
				ares = dwarf_lineaddr(line, &pc, &err);
			}
			if (DW_DLV_ERROR == ares) {
				printf("failed to obtain source - pc association\n");
				continue;
			}
			ares = dwarf_lineno(line, &lineno, &err);

			// This is a dirty huck! It seems that .debug_info
			// actually contains incoorect high_pc for nested
			// lexical scopes (dwarfdump -l <bin> does the same
			// mistake).

			if (lineno < old_lineno)
				lineno = old_lineno + 1;
			old_lineno = lineno;

			if (DW_DLV_ERROR == ares) {
				printf("failed to get a line number for the pc addr\n");
				continue;
			}
			if (DW_DLV_NO_ENTRY == ares)
				continue;
			
			printf("\t0x%08llx %llu %s\n", pc, lineno, filename);
			_pcaddr2line[filename][pc] = lineno;

			if (DW_DLV_OK == sres)
				dwarf_dealloc(dbg, filename, DW_DLA_STRING);
		}
		dwarf_srclines_dealloc(dbg, linebuf, linecount);
	} 

	int print_info(Dwarf_Debug &dbg, int is_info) {

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

			_pcaddr2line.clear();
			print_line_numbers_info(dbg, cu_die);
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

	int collect_vars_info(Elf * elf) {
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

	bool read_file_debug(const char * file) {	
		int fd = open(file, O_RDWR);
		if (-1 == fd) {
			printf("cannot find the file to open..\n");
			return false;
		}

		struct stat elf_stats;
		if ((fstat(fd, &elf_stats))) {
			printf("cannot stat the file\n");
			return false;
		}

		int e = parse_debug_info(fd);
		close(fd);
		return 1 == e;
	}
#endif // __linux
};


bool VarInfo::Imp::init(const std::string& file) {
#ifdef __linux
	_die_stack_indent_level = 0;
	return read_file_debug(file.c_str());
#else // __linux
	return false; // NOT_IMPLEMENTED
#endif // __linux
};


VarInfo::VarInfo() : _imp(new VarInfo::Imp) {}

const std::string VarInfo::type(const std::string& file, const size_t line, const std::string& name) const {
	return _imp->type(file, line, name);
}

bool VarInfo::init(const std::string& file) {
	_file = file;
	return _imp->init(_file);
}
