#include "swigmod.h"
#include <ctype.h>

class OBJECTIVEC:public Language {
private:
  /* Files and file sections containing generated code. */
  File *f_wrap_h;		// Wrapper header file
  File *f_wrap_mm;		// Wrapper source file
  File *f_proxy_h;		// Proxy header file
  File *f_proxy_mm;		// Proxy source file
  File *f_runtime;
  File *f_header;		/* General DOH objects used for holding the sections of wrapper source */
  File *f_wrappers;
  File *f_init;

  /* Strings temporarily holding the generated C++ code. */
  String *wrap_h_code;		// Code to be written in wrap_h.
  String *wrap_mm_code;		// Code to be written in wrap_mm.
  String *proxy_h_code;		// Code to be written in proxy_h.
  String *proxy_mm_code;	// Code to be written in proxy_mm.
  String *swigtypes_h_code;	// Code for Objective-C typewrapper classes header.
  String *swigtypes_mm_code;	// Code for Objective-C typewrapper classes implementation.


  /* Various flags controlling the code generation. */
  bool proxy_flag;		// flag: determine should the proxy files be generated or not

  /* State variables which indicate what is being wrapped at the moment. */
  bool member_variable_flag;	// flag: wrapping member variables
  bool static_member_variable_flag;	// flag: wrapping static member variables
  bool global_variable_flag;	// flag: wrapping global variables  
  bool global_func_flag;	// flag: wrapping global functions
  bool static_member_func_flag;	// flag: wrapping static member functions
  bool member_func_flag;	// flag: wrapping member functions
  bool member_constant_flag;	// flag: wrapping member constants

  /* Helper strings used in the code */
  String *variable_name;	// String representing the current variable name.
  String *proxyfuncname; // String representing the current memberfunction name.

  /* ObjectiveC data for the current proxy class:
   * These strings are mainly used to temporarily accumulate code from the
   * various member handling functions while a single class is processed and are
   * no longer relevant once that class has been finished, i.e. after
   * classHandler() has returned. */
  String *proxy_class_name;	// The unqualified name of the current proxy class.
  String *proxy_class_qname;	// The name of the current proxy class, qualified with the name of the namespace it is in, if any.
  // TODO: Add this when nspaces are handled. Not now!

  String *proxy_class_imports_code;	// The proxy imports code.This goes in the proxy_h file if proxy_flag is true.
  String *proxy_class_defns_code;	// The proxy class declaration code.This goes in the proxy_h file if proxy_flag is true.
  String *proxy_class_itfc_code;	// The proxy class interface code.This goes in the proxy_h file if proxy_flag is true.
  String *proxy_class_impl_code;	// The proxy class implementation code.This goes in the proxy_mm file if proxy_flag is true.
  String *proxy_class_imports;	// The import directives for the current proxy class. This goes in the proxy_h file if proxy_flag is true.

  String *proxy_class_enums_code;	// Code for enumerations nested in the current proxy class. Is emitted globally and earlier
  // than the rest of the body to work around forward referencing-issues.

  String *proxy_class_function_decls;	// Code for the proxy class member functions declaration.
  String *proxy_class_function_defns;	// Code for the proxy class member functions definition.
  String *proxy_global_function_decls;	// Code for the proxy class member functions declaration.
  String *proxy_global_function_defns;	// Code for the proxy class member functions definition.
  String *proxy_constant_decls;	        // Code for the proxy constant declaration.
  String *destrcutor_call;	// Contains an ObjectiveC call to the function wrapping the C++  destructor of the
  // current class (if there is a public C++ destructor).
  
  List *proxy_class_function_decls_list; // List of Objective-C class method declaration


  /* SWIG types data: Collects information about encountered types SWIG does not know about (e.g.
   * incomplete types). This is used later to generate type wrapper proxy.
   * classes for the unknown types. */
  Hash *unknown_types;

  /* Strings used at different places in the code. */
  static const char *const usage;	// Usage message
  const String *empty_string;	// Empty string used at different places in the code


public:
   OBJECTIVEC():f_wrap_h(NULL),
      f_wrap_mm(NULL),
      f_proxy_h(NULL),
      f_proxy_mm(NULL),
      f_runtime(NULL),
      f_header(NULL),
      f_wrappers(NULL),
      f_init(NULL),
      wrap_h_code(NULL),
      wrap_mm_code(NULL),
      proxy_h_code(NULL),
      proxy_mm_code(NULL),
      swigtypes_h_code(NULL),
      swigtypes_mm_code(NULL),
      proxy_flag(true),
      member_variable_flag(false),
      static_member_variable_flag(false),
      global_variable_flag(false),
      global_func_flag(false),
      static_member_func_flag(false),
      member_func_flag(false),
      member_constant_flag(false),
      variable_name(NULL),
      proxyfuncname(NULL),
      proxy_class_name(NULL),
      proxy_class_qname(NULL),
      proxy_class_imports_code(NULL),
      proxy_class_defns_code(NULL),
      proxy_class_itfc_code(NULL),
      proxy_class_impl_code(NULL),
      proxy_class_imports(NULL),
      proxy_class_enums_code(NULL),
      proxy_class_function_decls(NULL),
      proxy_class_function_defns(NULL),
      proxy_global_function_decls(NULL), 
      proxy_global_function_defns(NULL), 
      proxy_constant_decls(NULL),
      destrcutor_call(NULL), 
      proxy_class_function_decls_list(NULL),
      unknown_types(NULL), 
      empty_string(NewStringEmpty()) {
  } 
  
  /* ------------------------------------------------------------
   * main()
   * ------------------------------------------------------------ */
   
  virtual void main(int argc, char *argv[]) {
	  // Set language-specific subdirectory in SWIG library
	  SWIG_library_directory("objc");

	  // Process command line options
	  for (int i = 1; i < argc; i++) {
		if (argv[i]) {
		  if (strcmp(argv[i], "-noproxy") == 0) {
		Swig_mark_arg(i);
		proxy_flag = false;
		  } else if (strcmp(argv[i], "-help") == 0) {
		Printf(stdout, "%s\n", usage);
		  }
		}
	  }

	  // Set language-specific preprocessing symbol
	  Preprocessor_define("SWIGOBJECTIVEC 1", 0);

	  // Set language-specific configuration file 
	  SWIG_config_file("objc.swg");

	  // Set typemap language (historical) 
	  SWIG_typemap_lang("objc");
	  
	  allow_overloading();
  }
  
  /* ---------------------------------------------------------------------
   * top()
   * --------------------------------------------------------------------- */
   
  virtual int top(Node *n) {
	/* Get the module name */
	String *module = Getattr(n, "name");

	/* Initialize I/O */

	// Create the _wrap files
	String *wrapfile_h = NewStringf("%s_wrap.h", module);
	f_wrap_h = NewFile(wrapfile_h, "w", SWIG_output_files());
	if (!f_wrap_h) {
		FileErrorDisplay(wrapfile_h);
		SWIG_exit(EXIT_FAILURE);
	}
	Delete(wrapfile_h);

	String *wrapfile_mm = NewStringf("%s_wrap.mm", module);
	f_wrap_mm = NewFile(wrapfile_mm, "w", SWIG_output_files());
	if (!f_wrap_mm) {
		FileErrorDisplay(wrapfile_mm);
		SWIG_exit(EXIT_FAILURE);
	}
	Delete(wrapfile_mm);

	// Create the _proxy files if proxy flag is true
	if (proxy_flag) {
		String *proxyfile_h = NewStringf("%s_proxy.h", module);
		f_proxy_h = NewFile(proxyfile_h, "w", SWIG_output_files());
		if (!f_proxy_h) {
	  		FileErrorDisplay(proxyfile_h);
	  		SWIG_exit(EXIT_FAILURE);
		}
		Delete(proxyfile_h);
		
		String *proxyfile_mm = NewStringf("%s_proxy.mm", module);
		f_proxy_mm = NewFile(proxyfile_mm, "w", SWIG_output_files());
		if (!f_proxy_mm) {
	  		FileErrorDisplay(proxyfile_mm);
	  		SWIG_exit(EXIT_FAILURE);
		}
		Delete(proxyfile_mm);
	}

	f_runtime 	= NewStringEmpty();
	f_init 		= NewStringEmpty();
	f_header 	= NewStringEmpty();
	f_wrappers 	= NewStringEmpty();

	// Register file targets with the SWIG file handler
	Swig_register_filebyname("begin", f_wrap_mm);
	Swig_register_filebyname("header", f_header);
	Swig_register_filebyname("wrapper", f_wrappers);
	Swig_register_filebyname("runtime", f_runtime);
	Swig_register_filebyname("init", f_init);

	// Output module initialization code
	Swig_banner(f_wrap_h);
	Printf(f_wrap_h, "\n");
	Printf(f_wrap_h, "#ifndef %(upper)s_WRAP_H\n", module);
	Printf(f_wrap_h, "#define %(upper)s_WRAP_H\n", module);
	Printf(f_wrap_h, "\n#ifdef __cplusplus\n");
	Printf(f_wrap_h, "extern \"C\" {\n");
	Printf(f_wrap_h, "#endif\n\n");

	Swig_banner(f_wrap_mm);
	Printf(f_header, "#include \"%s_wrap.h\"\n", module);

	if (proxy_flag) {
		Swig_banner(f_proxy_h);

		Swig_banner(f_proxy_mm);
		Printf(f_proxy_mm, "#include \"%s_proxy.h\"\n", module);
		Printf(f_proxy_mm, "#include \"%s_wrap.h\"\n\n", module);
	}
	// Create strings for holding the generated code. These will be dumped
	// to the generated files at the end of the top function.
	wrap_h_code = NewStringEmpty();
	wrap_mm_code = NewStringEmpty();
	if (proxy_flag) {
		proxy_h_code 		= NewStringEmpty();
		proxy_mm_code 		= NewStringEmpty();
		swigtypes_h_code 	= NewStringEmpty();
		swigtypes_mm_code 	= NewStringEmpty();

        proxy_class_imports_code	= NewString("\n#import <Foundation/Foundation.h>\n");
        proxy_class_defns_code   	= NewStringEmpty();
		proxy_class_itfc_code 		= NewStringEmpty();
		proxy_class_impl_code 		= NewStringEmpty();
		proxy_class_enums_code 		= NewStringEmpty();
		proxy_class_function_decls 	= NewStringEmpty();
		proxy_class_function_defns 	= NewStringEmpty();
		proxy_global_function_decls = NewStringEmpty();
		proxy_global_function_defns = NewStringEmpty();
		proxy_constant_decls        = NewStringEmpty();
		proxy_class_imports 		= NewStringEmpty();
		
		proxy_class_function_decls_list = NewList();

		destrcutor_call = NewStringEmpty();
		unknown_types 	= NewHash();
	}

	/* Emit code for children */
	Language::top(n);

	// Write to the wrap_h
	Dump(wrap_h_code, f_wrap_h);
	Printf(f_wrap_h, "\n#ifdef __cplusplus\n");
	Printf(f_wrap_h, "}\n");
	Printf(f_wrap_h, "#endif\n");
	Printf(f_wrap_h, "\n#endif\n");
	// Write to the wrap_mm
	Dump(f_runtime, f_wrap_mm);
	Dump(f_header, f_wrap_mm);
	Printf(f_wrap_mm, "\n#ifdef __cplusplus\n");
	Printf(f_wrap_mm, "extern \"C\" {\n");
	Printf(f_wrap_mm, "#endif\n\n");
	Dump(f_wrappers, f_wrap_mm);
	Dump(wrap_mm_code, f_wrap_mm);
	Wrapper_pretty_print(f_init, f_wrap_mm);
	Printf(f_wrap_mm, "#ifdef __cplusplus\n");
	Printf(f_wrap_mm, "}\n");
	Printf(f_wrap_mm, "#endif\n");

	// Write to the proxy.h, if required
	if (proxy_flag) {
	
	    Printf(proxy_class_imports_code, "\n");
	    Dump(proxy_class_imports_code, f_proxy_h);

		// ObjectiveC will understand the C code.
		Printf(f_proxy_h, "#ifdef __cplusplus\n");
		Printf(f_proxy_h, "extern \"C\" {\n");
		Printf(f_proxy_h, "#endif\n\n");

		for (Iterator swig_type = First(unknown_types); swig_type.key; swig_type = Next(swig_type)) {
	  		emitTypeWrapperClass(swig_type.key, swig_type.item);
		}
		
        if (Len(proxy_class_defns_code) != 0) {
          Printf(proxy_class_defns_code, "\n");
          Dump(proxy_class_defns_code, f_proxy_h);
        }
        if (Len(proxy_class_enums_code) != 0) {
          Dump(proxy_class_enums_code, f_proxy_h);
        }
        if (Len(proxy_constant_decls) != 0) {
          Printf(proxy_constant_decls, "\n");
          Dump(proxy_constant_decls, f_proxy_h);
        }
        if (Len(proxy_global_function_decls) != 0) {
          Printf(proxy_global_function_decls, "\n");
          Dump(proxy_global_function_decls, f_proxy_h);
        }
        if (Len(swigtypes_h_code) != 0) {
          Printf(swigtypes_h_code, "\n");
		  Dump(swigtypes_h_code, f_proxy_h);
		}
		
		Dump(proxy_h_code, f_proxy_h);
		Printf(f_proxy_h, "#ifdef __cplusplus\n");
		Printf(f_proxy_h, "}\n");
		Printf(f_proxy_h, "#endif\n\n");
	}
	
	// Write to proxy.mm, if required
	if (proxy_flag) {
	    Dump(proxy_global_function_defns, f_proxy_mm);
		Dump(swigtypes_mm_code, f_proxy_mm);
		Dump(proxy_mm_code, f_proxy_mm);
	}
	
	// Cleanup
	Delete(wrap_h_code);
	Delete(wrap_mm_code);
	Delete(f_runtime);
	Delete(f_init);
	Delete(f_header);
	Delete(f_wrappers);
	Delete(f_wrap_h);
	Delete(f_wrap_mm);

	if (proxy_flag) {
		Delete(proxy_class_imports_code);
	    Delete(proxy_class_defns_code);
		Delete(proxy_class_itfc_code);
		Delete(proxy_class_impl_code);
		Delete(proxy_class_enums_code);
		Delete(proxy_class_function_decls);
		Delete(proxy_class_function_defns);
		Delete(proxy_global_function_decls);
		Delete(proxy_global_function_defns);
		Delete(proxy_class_imports);
		Delete(proxy_constant_decls);
		Delete(destrcutor_call);
		Delete(proxy_class_function_decls_list);
		Delete(unknown_types);

		Delete(swigtypes_h_code);
		Delete(swigtypes_mm_code);
		Delete(proxy_h_code);
		Delete(proxy_mm_code);
		Delete(f_proxy_h);
		Delete(f_proxy_mm);
	}

	return SWIG_OK;
  }
/* ----------------------------------------------------------------------
 * Language::importDirective()
 * ---------------------------------------------------------------------- */

  virtual int importDirective(Node *n) {
    String *module = Getattr(n, "module");
    if (module)
	  Printf(proxy_class_imports_code, "#import \"%s_proxy.h\"\n", module);

    return Language::importDirective(n);
  }

/* ---------------------------------------------------------------------
 * globalfunctionHandler()
 * --------------------------------------------------------------------- */
  virtual int globalfunctionHandler(Node *n) {
  	global_func_flag = true;
  	Language::globalfunctionHandler(n);
  	global_func_flag = false;

  	return SWIG_OK;
  }

/* ---------------------------------------------------------------------
 * memberfunctionHandler()
 * --------------------------------------------------------------------- */  
  virtual int memberfunctionHandler(Node *n)  {
  	String *symname = Getattr(n, "sym:name");
  	proxyfuncname = symname;
  	member_func_flag = true;
  	int result = Language::memberfunctionHandler(n);
  	member_func_flag = false;

  	return result;
  }
  
/* ---------------------------------------------------------------------
 * staticmemberfunctionHandler()
 * --------------------------------------------------------------------- */
  virtual int staticmemberfunctionHandler(Node *n) {
  	String *symname = Getattr(n, "sym:name");
  	proxyfuncname = symname;
  	static_member_func_flag = true;
  	int result = Language::staticmemberfunctionHandler(n);
  	static_member_func_flag = false;

  	return result;
  }
  
/* ---------------------------------------------------------------------
 * globalvariableHandler()
 * --------------------------------------------------------------------- */
  virtual int globalvariableHandler(Node *n) {
  	String *symname = Getattr(n, "sym:name");
  	variable_name = symname;
  	global_variable_flag = true;
  	int result = Language::globalvariableHandler(n);
  	global_variable_flag = false;
  	
  	return result;
  }
  
/* ---------------------------------------------------------------------
 * membervariableHandler()
 * --------------------------------------------------------------------- */
  virtual int membervariableHandler(Node *n) {
  	String *symname = Getattr(n, "sym:name");
  	variable_name = symname;
  	member_variable_flag = true;
  	int result = Language::membervariableHandler(n);
  	member_variable_flag = false;
  	
  	return result;
  }

/* ---------------------------------------------------------------------
 * staticmembervariableHandler()
 * --------------------------------------------------------------------- */
  virtual int staticmembervariableHandler(Node *n) {
  	String *symname = Getattr(n, "sym:name");
  	variable_name = symname;
  	static_member_variable_flag = true;
  	int result = Language::staticmembervariableHandler(n);
  	static_member_variable_flag = false;
  	
  	return result;
  }

/* ---------------------------------------------------------------------
 * memberconstantHandler() 
 * --------------------------------------------------------------------- */
  virtual int memberconstantHandler(Node *n) {
  	String *symname = Getattr(n, "sym:name");
  	variable_name = symname;
  	member_constant_flag = true;
  	int result = Language::memberconstantHandler(n);
  	member_constant_flag = false;
  	
  	return result;
  }
  
/* ---------------------------------------------------------------------
 * destructorHandler()
 * --------------------------------------------------------------------- */
  virtual int destructorHandler(Node *n) {
  	Language::destructorHandler(n);
  	String *symname = Getattr(n, "sym:name");
  	Printv(destrcutor_call, Swig_name_wrapper(Swig_name_destroy(getNSpace(), symname)), "((void*)self.swigCPtr)", NIL);
  	return SWIG_OK;
  }

/* ---------------------------------------------------------------------
 * classHandler()
 * --------------------------------------------------------------------- */
  virtual int classHandler(Node *n) {
  	if (proxy_flag) {
    	proxy_class_name = Copy(Getattr(n, "sym:name"));

    	if (!addSymbol(proxy_class_name, n))
      		return SWIG_ERROR;

    	Clear(proxy_class_imports);
    	Clear(proxy_class_function_decls);
    	Clear(proxy_class_function_defns);
    	Clear(proxy_class_itfc_code);
    	Clear(proxy_class_impl_code);
    	Clear(destrcutor_call);
    	Clear(proxy_class_function_decls_list);
  	}

  	Language::classHandler(n);

  	if (proxy_flag) {
    	// Write the code for proxy class
    	emitProxyClass(n);

    	// Apply the necessary substitutions    
    	Replaceall(proxy_class_defns_code, "$objcclassname", proxy_class_name);    
    	Replaceall(proxy_class_itfc_code, "$objcclassname", proxy_class_name);
    	Replaceall(proxy_class_impl_code, "$objcclassname", proxy_class_name);

    	// And, dump everything to the proxy files
    	Printv(proxy_h_code, proxy_class_itfc_code, NIL);
    	Printv(proxy_mm_code, proxy_class_impl_code, NIL);
    
    	// Tidy up
    	Delete(proxy_class_qname);
    	proxy_class_qname = NULL;
    	Delete(proxy_class_name);
    	proxy_class_name = NULL;
  	}

  	return SWIG_OK;
  }

/* ----------------------------------------------------------------------
 * enumDeclaration()
 * ---------------------------------------------------------------------- */
  virtual int enumDeclaration(Node *n) {
  	if (ImportMode)
    	return SWIG_OK;

  	if (getCurrentClass() && (cplus_mode != PUBLIC))
    	return SWIG_NOWRAP;

  	String *typemap_lookup_type = Getattr(n, "name");
  	String *symname = Getattr(n, "sym:name");
  	String *enumname;

  	Node *p = parentNode(n);
  	
  	if (p && !Cmp(nodeType(p), "class")) {	// This is a nested enum, prefix the class name
    	String *parentname = Getattr(p, "sym:name");
    	enumname = NewStringf("%s_%s", parentname, symname);
  	}
  	else {
   	 	enumname = Copy(symname);
  	}

  	if (proxy_flag) {
    	if (typemap_lookup_type) {
     		// Copy-paste the C/C++ enum as an Objective-C enum
            Printv(proxy_class_enums_code, "typedef NS_ENUM(NSInteger, ", enumname, ") {\n", NIL);
    	} 
    	else {
      		// Handle anonymous enums.
      		Printv(proxy_class_enums_code, "\nenum {\n", NIL);
    	}
  	}
  	// Emit each enum item
  	Language::enumDeclaration(n);
  	
  	if (!GetFlag(n, "nonempty")) {
    	// Default value for empty enums;
    	Printv(proxy_class_enums_code,"  ", enumname, "None = 0", NIL);
  	}

  	if (proxy_flag) {
    	if (typemap_lookup_type) {
      	// Finish the enum declaration
      	Printv(proxy_class_enums_code, "\n};\n\n", NIL);
    	}
    	else {
      	// Handle anonymous enums.
      	Printv(proxy_class_enums_code, "\n};\n\n", NIL);
    	}
  	}

  	Delete(enumname);
  	return SWIG_OK;
}

/* ---------------------------------------------------------------------------
 * enumvalueDeclaration()
 * --------------------------------------------------------------------------- */
  virtual int enumvalueDeclaration(Node *n) {
  	if (getCurrentClass() && (cplus_mode != PUBLIC))
    	return SWIG_NOWRAP;

  	Swig_require("enumvalueDeclaration", n, "*name", "?value", NIL);
  	String *symname = Getattr(n, "sym:name");
  	String *value = Getattr(n, "feature:objc:constvalue");
  	Node *parent = parentNode(n);
  	Node *pparent = parentNode(parent);
  	String *enumname;
  	
  	if (pparent && !Cmp(nodeType(pparent), "class"))	// This is a nested enum, prefix the class name
    	enumname = NewStringf("%s_%s", Getattr(pparent, "sym:name"), symname);
  	else
    	enumname = Copy(symname);

  	if (proxy_flag) {		// Emit the enum item
  	  String *valuedecl = NewStringEmpty();
  	  
    	if (!GetFlag(n, "firstenumitem"))
      		Printf(proxy_class_enums_code, ",\n");
      		
    	Printf(valuedecl, "  %s", enumname);
    	  
        // The %objcconst feature determines how the constant value is obtained
    	if (!value && GetFlag(n, "feature:objc:const")) {
    	
    	  // Deal with enum values that are not int
          int swigtype = SwigType_type(Getattr(n, "type"));
          if (swigtype == T_BOOL) {
            const char *val = Equal(Getattr(n, "enumvalue"), "true") ? "1" : "0";
            Setattr(n, "enumvalue", val);
          } else if (swigtype == T_CHAR) {
            String *val = NewStringf("'%s'", Getattr(n, "enumvalue"));
            Setattr(n, "enumvalue", val);
            Delete(val);
          }
          value = Getattr(n, "enumvalue");// ? Copy(Getattr(n, "enumvalue")) : Copy(Getattr(n, "enumvalueex"));
    	}
    	
    	if (value)
    	  Printf(valuedecl, " = %s", value);
    	  
    	Printf(proxy_class_enums_code, "  %s", valuedecl);
    	
    	Delete(valuedecl);
  	}
  	// Keep track that the currently processed enum has at least one value
	SetFlag(parent, "nonempty");

  	Swig_restore(n);
  	Delete(enumname);
  	return SWIG_OK;
  }

  /* Low-level code generation */

/* ---------------------------------------------------------------------
 * constantWrapper()
 * --------------------------------------------------------------------- */
  virtual int constantWrapper(Node *n) {
  	SwigType *type = Getattr(n, "type");
  	String *symname = Getattr(n, "sym:name");
  	
  	String *typestring = SwigType_str(type, 0);
  	String *tm;

  	String *crettype = NewStringEmpty();
  	String *constants_h_code = NewStringEmpty();
  	String *constants_mm_code = NewStringEmpty();
  	bool is_func_ptr = SwigType_isfunctionpointer(type);
  	// The %objcconst feature determines how the constant value is obtained
    int const_feature_flag = GetFlag(n, "feature:objc:const");
    // Check for the %objcconstvalue feature
    String *value = Getattr(n, "feature:objc:constvalue");

  	if (!addSymbol(symname, n))
    	return SWIG_ERROR;
    	
    Swig_save("constantWrapper", n, "value", NIL);

  	// Get the corresponding ObjectiveC type or the intermediate type. "imtype" if no proxy and "objctype" if proxy_flag is true.
  	if (!is_func_ptr) {
    	if (proxy_flag) {
      		if ((tm = Swig_typemap_lookup("objctype", n, "", 0))) {
				substituteClassname(tm, type);
			    Printf(crettype, "const %s", tm);
      		}
      		else
				Swig_warning(WARN_OBJC_TYPEMAP_OBJCTYPE_UNDEF, input_file, line_number, "No objctype typemap defined for %s\n", typestring);
    	}
    	else {
      		if ((tm = Swig_typemap_lookup("imtype", n, "", 0)))
				  Printf(crettype, "const %s", tm);
      		else
				Swig_warning(WARN_OBJC_TYPEMAP_IMTYPE_UNDEF, input_file, line_number, "No imtype typemap defined for %s\n", typestring);
    	}
  	}
  	else {
    	if (proxy_flag) {
      		if ((tm = Swig_typemap_lookup("objctype", n, "", 0))) {
				substituteClassname(tm, type);
				Printf(crettype, "%s", tm);
      		}
      		else
				Swig_warning(WARN_OBJC_TYPEMAP_OBJCTYPE_UNDEF, input_file, line_number, "No objctype typemap defined for %s\n", typestring);
    	}
    	else
      		Printf(crettype, "%s", SwigType_str(type, symname));
  	}
      
    if (is_func_ptr) {
        
    	Printf(constants_h_code, "extern %s %s;\n", crettype, symname);
    	//Printf(constants_mm_code, "%s %s = (%s)%s();\n", crettype, symname, typestring, Swig_name_wrapper(Swig_name_get(getNSpace(), symname)));
        // Transform return type used in low level accessor to the type used in Objective-C constant definition
    	String *imcall = NewStringf("%s()", Swig_name_wrapper(Swig_name_get(getNSpace(), symname)));
    	if ((tm = Swig_typemap_lookup("objcvarout", n, "", 0))) {
      		substituteClassname(tm, type);
      		Replaceall(tm, "$objcvarname", symname);
      		Replaceall(tm, "$imcall", imcall);
    	}
    	else
      		Swig_warning(WARN_OBJC_TYPEMAP_OBJCOUT_UNDEF, input_file, line_number, "No objcout typemap defined for %s\n", crettype);
      		
    	Printf(constants_mm_code, "%s\n", tm);
    	
    	SetFlag(n, "feature:immutable");
    	variableWrapper(n);
    	
    	Delete(imcall);
    	
  	} else if(value) {
  	
      Printf(constants_h_code, "extern %s %s;\n", crettype, symname);
      Printf(constants_mm_code, "%s %s = %s;\n", crettype, symname, value);
      
  	} else if (!const_feature_flag) {
  	
  	  // Add the stripped quotes back in
      if (SwigType_type(type) == T_STRING) {
        value = NewStringf("@\"%s\"", Copy(Getattr(n, "value")));
         Printf(constants_mm_code, "%s %s = %s;\n", crettype, symname, value);
      } else if (SwigType_type(type) == T_CHAR) {
        value = NewStringf("\'%s\'", Copy(Getattr(n, "value")));
         Printf(constants_mm_code, "%s %s = %s;\n", crettype, symname, value);
      } else {
  	    value = Swig_name_wrapper(Swig_name_get(getNSpace(), symname));
  	    Printf(constants_mm_code, "%s %s = (%s) %s();\n", crettype, symname, crettype, value);
  	    
  	    SetFlag(n, "feature:immutable");
  	    variableWrapper(n);
  	  }
      
      Printf(constants_h_code, "extern %s %s;\n", crettype, symname);
      Delete(value);
  	  
  	} else if (Getattr(n, "wrappedasconstant")) {
  		if (SwigType_type(type) == T_CHAR)
    		Printf(constants_mm_code, "%s %s = \'%s\';\n", crettype, symname, Getattr(n, "staticmembervariableHandler:value"));
		else if (SwigType_type(type) == T_STRING)
   		 	Printf(constants_mm_code, "%s const %s = @\"%s\";\n", crettype, symname, Getattr(n, "staticmembervariableHandler:value"));
  		else
    		Printf(constants_mm_code, "%s %s = %s;\n", crettype, symname, Getattr(n, "value")); 
  	}
  	else {
 	  Printf(constants_h_code, "extern %s %s;\n", crettype, symname);
      Printf(constants_mm_code, "%s %s = %s;\n", crettype, symname, Getattr(n, "value"));
   	}

  	// Dump to generated files
  	if (proxy_flag) {		// write to the proxy files
    	Printv(proxy_constant_decls, constants_h_code, NIL);
    	Printv(proxy_mm_code, constants_mm_code, NIL);
  	}
  	else {			// write to the wrap files
    	Printv(wrap_h_code, constants_h_code, NIL);
    	Printv(wrap_mm_code, constants_mm_code, NIL);
  	}

  	// Cleanup
  	Delete(crettype);
  	Delete(constants_h_code);
  	Delete(constants_mm_code);
  	return SWIG_OK;
  }
  
/* ---------------------------------------------------------------------
 * functionWrapper()
 * --------------------------------------------------------------------- */
  virtual int functionWrapper(Node *n) {
      
	// Get some useful attributes of this function
	String *symname = Getattr(n, "sym:name");
	String *overloaded_name = getOverloadedName(n);
	SwigType *type = Getattr(n, "type");
	ParmList *parmlist = Getattr(n, "parms");
	String *crettype = SwigType_str(type, 0);
	String *imrettype = NewStringEmpty();
	String *tm;
	bool is_void_return = (Cmp(crettype, "void") == 0);
	bool is_constructor = (Cmp(nodeType(n), "constructor") == 0);
	bool is_destructor = (Cmp(nodeType(n), "destructor") == 0);

	if (!Getattr(n, "sym:overloaded") && !addSymbol(symname, n))
		return SWIG_ERROR;
	
	// Create the function's wrappered name
	String *wname = Swig_name_wrapper(overloaded_name);

	// Create the wrapper function object
	Wrapper *wrapper = NewWrapper();

	// Retrieve the intermediate return type for this function
	if ((tm = Swig_typemap_lookup("imtype", n, "", 0)))
		Printf(imrettype, "%s", tm);
	else
		Swig_warning(WARN_OBJC_TYPEMAP_IMTYPE_UNDEF, input_file, line_number, "No imtype typemap defined for %s\n", crettype);

	// Deal with overloading

	/* Write the wrapper function declaration and definition */

	// Begin the first line of the function definition
	Printv(wrapper->def, imrettype, " ", wname, "(", NIL);
	
	Setattr(n, "wrap:parms", parmlist);

	// Make the param list with the intermediate parameter types 
	makeParameterList(parmlist, wrapper);

	// End the first line of the function definition
	Printv(wrapper->def, ")", NIL);

	// Write the function declaration to the wrap_h
	Printv(wrap_h_code, wrapper->def, ";\n", NIL);

	// Now write the function definition to the wrap_cpp
	Printv(wrapper->def, " {", NIL);

	// If any additional local variable needed, add them now
	if (!is_void_return)
		Wrapper_add_localv(wrapper, "imresult", imrettype, "imresult = 0", NIL);

	// Now walk the function parameter list and generate code to get arguments.
	marshalInputArgs(parmlist, wrapper);

	Setattr(n, "wrap:name", wname);
	String *actioncode = emit_action(n);

	// Write typemaps(out) and return value if necessary.
	marshalOutput(n, actioncode, wrapper);

	// Close the function
	if (!is_void_return)
		Printv(wrapper->code, "return imresult;\n", NIL);
		
	Printv(wrapper->code, "}", NIL);

	// Final substititions if applicable.

	/* Contract macro modification */
	Replaceall(wrapper->code, "SWIG_contract_assert(", "SWIG_contract_assert($null, ");

	if (!is_void_return)
		Replaceall(wrapper->code, "$null", "0");
	else
		Replaceall(wrapper->code, "$null", "");

	// Dump the function out.
	Wrapper_print(wrapper, wrap_mm_code);

	/* Create the proxy functions if proxy_flag is true. */
	if (proxy_flag && is_constructor) {	// Handle constructor
		Setattr(n, "imfunctionname", wname);
		emitProxyClassConstructor(n);
	}
	else if (proxy_flag && is_destructor) {	// Handle destructor
	// TODO: Do it here instead in emitProxyClass function
	}
	
	// globalFunctionHandler is called for static member functions as well hence setting global_func_flag to true. 
	// To route the call to the appropriate proxy generator, we check for !static_member_func_flag here.
	if (proxy_flag && (global_variable_flag || global_func_flag) && !static_member_func_flag) {	// Handle globals
		Setattr(n, "imfunctionname", wname);
		emitProxyGlobalFunctions(n);
	}
	else if (proxy_flag && (member_variable_flag || static_member_variable_flag || member_constant_flag || member_func_flag || static_member_func_flag)) {	// Handle members
		Setattr(n, "imfunctionname", wname);
		emitProxyClassFunction(n);
	}
	
	// Tidy up
	Delete(imrettype);
	Delete(wname);
	Delete(overloaded_name);
	DelWrapper(wrapper);
	
	return SWIG_OK;
  }
  
/* ---------------------------------------------------------------------
 * nativeWrapper()
 * --------------------------------------------------------------------- */
  virtual int nativeWrapper(Node *n) {
  	return Language::nativeWrapper(n);
  }

/* -----------------------------------------------------------------------------
* getOverloadedName()
* ----------------------------------------------------------------------------- */
  String *getOverloadedName(Node *n) {

    String *overloaded_name = NewStringf("%s", Getattr(n, "sym:name"));

    if (Getattr(n, "sym:overloaded")) {
      Printv(overloaded_name, Getattr(n, "sym:overname"), NIL);
    }

    return overloaded_name;
  }
  
/* -----------------------------------------------------------------------------
* checkDuplicateDeclaration()
* ----------------------------------------------------------------------------- */
  void checkDuplicateDeclaration(String *decl, String *name) {
  	String *ori_decl = Copy(decl);
  	
  	// if the method already exist, add an underscore
    for (int i=0; i<Len(proxy_class_function_decls_list); i++) {
      String *item = Getitem(proxy_class_function_decls_list, i);
      if( Cmp(decl, item) == 0) {
        Push(decl, "_"); 
        Push(name, "_");
      }
    }
    
    if(Cmp(ori_decl, decl) != 0)
      Swig_warning(WARN_OBJC_DUPLICATE_DECL, input_file, line_number, "duplicate declaration of objective-c method '%s', renaming to '%s'\n",ori_decl,decl);
    
    Append(proxy_class_function_decls_list,decl);

    Delete(ori_decl);
  }

  /* Proxy class code generators */
  void emitProxyGlobalFunctions(Node *n);
  void emitProxyClassFunction(Node *n);
  void emitProxyClassConstructor(Node *n);
  void emitProxyClass(Node *n);
  void emitTypeWrapperClass(String *classname, SwigType *type);

  /* Helper functions */
  bool substituteClassname(String *tm, SwigType *pt);
  void substituteClassnameVariable(String *tm, const char *classnamevariable, SwigType *type);
  bool skipIgnoredArgs(Parm **p);
  void marshalInputArgs(ParmList *parmlist, Wrapper *wrapper);
  void makeParameterList(ParmList *parmlist, Wrapper *wrapper);
  void marshalOutput(Node *n, String *actioncode, Wrapper *wrapper);
  String *createProxyName(SwigType *t);
  const String *typemapLookup(Node *n, const_String_or_char_ptr tmap_method, SwigType *type, int warning, Node *typemap_attributes = 0);
};

/* -----------------------------------------------------------------------------
 * emitProxyGlobalFunctions()
 *
 * C/C++ global functions and global variables are mapped to ObjectiveC global functions.
 * C/C++ types will be replaced with their equivalent ObjectiveC types (eg: char* --> NSString*)
 * A prefix 'Objc' will be added to counter linker redifiniton complains. 
 *
 * ----------------------------------------------------------------------------- */

void OBJECTIVEC::emitProxyGlobalFunctions(Node *n) {
  if (!proxy_flag)
    return;

  // Get some useful attributes of this function
  String *symname = Getattr(n, "sym:name");
  String *ccasesymname = Swig_string_ccase(symname);
  SwigType *type = Getattr(n, "type");
  ParmList *parmlist = Getattr(n, "parms");
  String *crettype = SwigType_str(type, 0);
//   String *storage = Getattr(n, "storage");
  String *objcrettype = NewStringEmpty();
  String *imcall = NewStringEmpty();
  String *function_defn = NewStringEmpty();
  String *function_decl = NewStringEmpty();
  String *tm;
  String *imfunctionname = Getattr(n, "imfunctionname");
  String *proxyfunctionname = NewStringEmpty();
  bool setter_flag = false;

  // Retrieve the ObjectiveC return type for this function
  if ((tm = Swig_typemap_lookup("objctype", n, "", 0))) {
    substituteClassname(tm, type);
    Printf(objcrettype, "%s", tm);
  } else {
    Swig_warning(WARN_OBJC_TYPEMAP_OBJCTYPE_UNDEF, input_file, line_number, "No objctype typemap defined for %s\n", crettype);
  }   

  // Change the function name for global variables
  if (proxy_flag && global_variable_flag) {
    setter_flag = (Cmp(symname, Swig_name_set(getNSpace(), variable_name)) == 0);
    String *ccvariablename = Swig_string_ccase(variable_name);
    Printf(proxyfunctionname, (setter_flag)? "set%s" : "get%s", ccvariablename);

    Delete(ccvariablename);
  } else {
    Printf(proxyfunctionname, "Objc%s", ccasesymname);
  }

  /* Write the proxy global function declaration and definition */
  
  // Deal with overloading 
  if (Getattr(n, "sym:overloaded"))
      Printv(proxyfunctionname, Getattr(n, "sym:overname"), NIL);

  // Begin the first line of the function
  Printv(function_decl, objcrettype, " ", proxyfunctionname, "(", NIL);

  // Prepare the call to intermediate function
  Printf(imcall, "%s(", imfunctionname);

  // Attach the non-standard typemaps to the parameter list
  Swig_typemap_attach_parms("in", parmlist, NULL);
  Swig_typemap_attach_parms("objctype", parmlist, NULL);
  Swig_typemap_attach_parms("objcin", parmlist, NULL);

  Parm *p;
  int i = 0;
  int gencomma = 0;
  
  emit_mark_varargs(parmlist);

  for (p = parmlist; p; i++) {
    if(skipIgnoredArgs(&p))
      continue;
    
    SwigType *pt = Getattr(p, "type");
    
    String *objcparmtype = NewStringEmpty();

    // Get the ObjectiveC parameter type for this parameter
    if ((tm = Getattr(p, "tmap:objctype"))) {
      substituteClassname(tm, pt);
      Printf(objcparmtype, "%s", tm);
    } else {
      Swig_warning(WARN_OBJC_TYPEMAP_OBJCTYPE_UNDEF, input_file, line_number, "No objctype typemap defined for %s\n", SwigType_str(pt, 0));
    }

    if (gencomma)
      Printf(imcall, ", ");

    String *arg = makeParameterName(n, p, i, setter_flag);

    // Use typemaps to transform the type used in Objective-C proxy function to the one used in intermediate code.
    if ((tm = Getattr(p, "tmap:objcin"))) {
      substituteClassname(tm, pt);
      Replaceall(tm, "$objcinput", arg);
      Printv(imcall, tm, NIL);
    } else {
      Swig_warning(WARN_OBJC_TYPEMAP_OBJCIN_UNDEF, input_file, line_number, "No objcin typemap defined for %s\n", SwigType_str(pt, 0));
    }

    // Add parameter to proxy function
    if (gencomma >= 2)
      Printf(function_decl, ", ");
    gencomma = 2;
    Printf(function_decl, "%s %s", objcparmtype, arg);
    
    p = Getattr(p, "tmap:in:next");

    Delete(arg);
    Delete(objcparmtype);
  }

  // End the first line of the function
  Printv(function_decl, ")", NIL);	// or 
  //Printv(function_decl, paramstring, ")", NIL);

  Printv(function_defn, function_decl, " {\n", NIL);
  Printf(function_decl, ";");

  // End the call to the intermediate function
  Printv(imcall, ")", NIL);

  // Transform return type used in low level accessor to type used in Objective-C proxy function 
  if ((tm = Swig_typemap_lookup("objcout", n, "", 0))) {
    substituteClassname(tm, type);
    Replaceall(tm, "$imcall", imcall);
  }
  else {
    Swig_warning(WARN_OBJC_TYPEMAP_OBJCOUT_UNDEF, input_file, line_number, "No objcout typemap defined for %s\n", crettype);
  }

  Printf(function_defn, "  %s\n}\n", tm ? (const String *) tm : empty_string);

  Printv(proxy_global_function_decls, function_decl, "\n", NIL);
  Printv(proxy_global_function_defns, function_defn, "\n", NIL);

  //Delete(paramstring);
  Delete(ccasesymname);
  Delete(proxyfunctionname);
  Delete(objcrettype);
  Delete(imcall);
  Delete(function_decl);
  Delete(function_defn);
}

/* -----------------------------------------------------------------------------
 * emitProxyClassFunction()
 *
 * Function called for writing an Objective-C proxy function for static and non-static 
 * C++ class functions, static and non-static member variables, member constants/enums.
 * 
 * C++ class static functions map to Objective-C "+" functions.
 * C++ class non-static functions map to Objective-C "-" functions.
 * ----------------------------------------------------------------------------- */

void OBJECTIVEC::emitProxyClassFunction(Node *n) {
  if (!proxy_flag)
    return;

  // Get some useful attributes of this function
  String *symname = Getattr(n, "sym:name");
  SwigType *type = Getattr(n, "type");
  ParmList *parmlist = Getattr(n, "parms");
  String *crettype = SwigType_str(type, 0);
  String *objcrettype = NewStringEmpty();
  String *imcall = NewStringEmpty();
  String *function_defn = NewStringEmpty();
  String *function_decl = NewStringEmpty();
  String *tm;

  String *imfunctionname = Getattr(n, "imfunctionname");
  String *proxyfunctionname;
 
  bool variable_flag = member_variable_flag || static_member_variable_flag || member_constant_flag;
  bool setter_flag = false;
  bool static_flag = (static_member_func_flag || static_member_variable_flag);

  // Retrieve the ObjectiveC return type for this function
  if ((tm = Swig_typemap_lookup("objctype", n, "", 0))) {
    substituteClassname(tm, type);
    Printf(objcrettype, "%s", tm);
  } else {
    Swig_warning(WARN_OBJC_TYPEMAP_OBJCTYPE_UNDEF, input_file, line_number, "No objctype typemap defined for %s\n", crettype);
  }

  if (variable_flag) {
    proxyfunctionname = NewStringEmpty();
    setter_flag = Cmp(symname, Swig_name_set(getNSpace(), Swig_name_member(0, proxy_class_name, variable_name))) == 0;
    String *ccvariablename = Swig_string_ccase(variable_name);
    Printf(proxyfunctionname, (setter_flag)? "set%s" : "get%s", ccvariablename);
    Delete(ccvariablename);
  } else {
    proxyfunctionname = Swig_string_lccase(proxyfuncname);
    
    // if the method fist char is numerical, add an underscore
    char c = *Char(proxyfunctionname);
    if (c > 47 && c < 58)
      Push(proxyfunctionname , "_");
  } 

  /* Write the proxy function declaration and definition */

  // Begin the first line of the function declaration
  String *objcfuncdecl = Swig_string_lccase(proxyfunctionname);

  // Prepare the call to intermediate function
  Printv(imcall, imfunctionname, "(", NIL);

  // Attach the non-standard typemaps to the parameter list
  Swig_typemap_attach_parms("in", parmlist, NULL);
  Swig_typemap_attach_parms("objctype", parmlist, NULL);
  Swig_typemap_attach_parms("objcin", parmlist, NULL);

  Parm *p;
  int i = 0;
  int gencomma = 0;
  
  emit_mark_varargs(parmlist);

  for (p = parmlist; p;/* p = nextSibling(p),*/ i++) {
    if(skipIgnoredArgs(&p))
      continue;
    
    SwigType *pt = Getattr(p, "type");
    String *objcparmtype = NewStringEmpty();

    // Get the ObjectiveC parameter type for this parameter
    if ((tm = Getattr(p, "tmap:objctype"))) {
      substituteClassname(tm, pt);
      Printf(objcparmtype, "%s", tm);
    } else {
      Swig_warning(WARN_OBJC_TYPEMAP_OBJCTYPE_UNDEF, input_file, line_number, "No objctype typemap defined for %s\n", SwigType_str(pt, 0));
    }

    if (gencomma)
      Printf(imcall, ", ");
    
    String *objcparmname = (!static_flag && !gencomma)? NewString("self") : makeParameterName(n, p, i, setter_flag);

    // Use typemaps to transform type used in Objective-C proxy function to the one used in intermediate code.
    if ((tm = Getattr(p, "tmap:objcin"))) {
      substituteClassname(tm, pt);
      Replaceall(tm, "$objcinput", objcparmname);
      Printv(imcall, tm, NIL);
    } else {
      Swig_warning(WARN_OBJC_TYPEMAP_OBJCIN_UNDEF, input_file, line_number, "No objcin typemap defined for %s\n", SwigType_str(pt, 0));
    }
 
    if (gencomma == (static_flag?0:1)) {
      String *objcparmdecl;
      if (!variable_flag && Strncmp(proxyfunctionname, "set", 3) && Strncmp(proxyfunctionname, "get", 3) ) {
        objcparmdecl = Swig_string_ccase(objcparmname);
        Push(objcparmdecl, "With");
      } else {
        objcparmdecl = NewStringEmpty();
      }
      
      Printv(proxyfunctionname, objcparmdecl,":(", objcparmtype, ")", objcparmname, NIL);
      Printf(objcfuncdecl, "%s:", objcparmdecl);
      
      Delete(objcparmdecl);
    }
    else if (gencomma >= (static_flag?1:2)) {
      String *objcparmdecl = Swig_string_lccase(objcparmname);
	  Printf(proxyfunctionname, " %s:(%s)%s", objcparmdecl, objcparmtype, objcparmname);
	  Printf(objcfuncdecl, "%s:", objcparmdecl);
	  Delete(objcparmdecl);
	}
		
    gencomma++;
    
    p = Getattr(p, "tmap:in:next");
    
    Delete(objcparmname);
    Delete(objcparmtype);
  }
  
  // Deal with overloading  
  checkDuplicateDeclaration(objcfuncdecl, proxyfunctionname);
  Printv(function_decl, (static_flag)? "+":"-", " (", objcrettype, ")", proxyfunctionname, NIL);

  // First line of function definition
  Printv(function_defn, function_decl, " {\n", NIL);
  Printf(function_decl, ";");

  // End the call to the intermediate function
  Printv(imcall, ")", NIL);

  // Transform return type used in low level accessor to type used in Objective-C proxy function 
  if ((tm = Swig_typemap_lookup("objcout", n, "", 0))) {
    substituteClassname(tm, type);
    Replaceall(tm, "$imcall", imcall);
  } else {
    Swig_warning(WARN_OBJC_TYPEMAP_OBJCOUT_UNDEF, input_file, line_number, "No objcout typemap defined for %s\n", crettype);
  }

  Printf(function_defn, "  %s\n}\n", tm ? (const String *) tm : empty_string);

  /* Write the function declaration to the proxy_class_function_decls 
     and function definition to the proxy_class_function_defns */
  Printv(proxy_class_function_decls, function_decl, "\n", NIL);
  Printv(proxy_class_function_defns, function_defn, "\n", NIL);

  //Delete(paramstring);
  Delete(proxyfunctionname);
  Delete(objcrettype);
  Delete(imcall);
  Delete(objcfuncdecl);
  Delete(function_decl);
  Delete(function_defn);
}

/* -----------------------------------------------------------------------------
 * emitProxyClassConstructor()
 *
 * Function called for writing an Objective-C proxy class constructor a.k.a 
 * init function(s).
 *
 * ----------------------------------------------------------------------------- */

void OBJECTIVEC::emitProxyClassConstructor(Node *n) {
  if (!proxy_flag)
    return;

  // Get some useful attributes of the constructor
  String *name = Getattr(n, "name");
  ParmList *parmlist = Getattr(n, "parms");
  String *imcall = NewStringEmpty();
  String *constructor_defn = NewStringEmpty();
  String *constructor_decl = NewStringEmpty();
  String *tm;
  String *objcrettype = NewString("id");
  String *imfunctionname = Getattr(n, "imfunctionname");
  String *proxyfunctionname = NewString("init");
  String *objcfuncdecl = Copy(proxyfunctionname);  

  /* Write the proxy class constructor declaration and definition */

  // Prepare the call to intermediate function
  Printv(imcall, imfunctionname, "(", NIL);

  // Attach the non-standard typemaps to the parameter list
  Swig_typemap_attach_parms("in", parmlist, NULL);
  Swig_typemap_attach_parms("objctype", parmlist, NULL);
  Swig_typemap_attach_parms("objcin", parmlist, NULL);

  Parm *p;
  int i = 0;
  int gencomma = 0;
  
  emit_mark_varargs(parmlist);

  for (p = parmlist; p; i++) {
    if(skipIgnoredArgs(&p))
      continue;
    
    SwigType *pt = Getattr(p, "type");
    String *objcparmtype = NewStringEmpty();

    // Get the ObjectiveC parameter type for this parameter
    if ((tm = Getattr(p, "tmap:objctype"))) {
      substituteClassname(tm, pt);
      Printf(objcparmtype, "%s", tm);
    } else {
      Swig_warning(WARN_OBJC_TYPEMAP_OBJCTYPE_UNDEF, input_file, line_number, "No objctype typemap defined for %s\n", SwigType_str(pt, 0));
    }

    if (gencomma)
      Printf(imcall, ", ");

    String *objcparmname = makeParameterName(n, p, i, false);

    // Use typemaps to transform type used in Objective-C proxy function to the one used in intermediate code.
    if ((tm = Getattr(p, "tmap:objcin"))) {
      substituteClassname(tm, pt);
      Replaceall(tm, "$objcinput", objcparmname);
      Printv(imcall, tm, NIL);
    } else {
      Swig_warning(WARN_OBJC_TYPEMAP_OBJCIN_UNDEF, input_file, line_number, "No objcin typemap defined for %s\n", SwigType_str(pt, 0));
    }

    // Add parameter to proxy function
    if (gencomma >= 1) {	// Subsequent arguments.
      String *objcparmdecl = Swig_string_lccase(objcparmname);
      Printv(proxyfunctionname, " ", objcparmdecl,":(", objcparmtype, ")", objcparmname, NIL);
      Printf(objcfuncdecl, "%s:", objcparmdecl);
      Delete(objcparmdecl);
    } else {// First valid argument, prefix it with 'With' and capitalize the first letter
      String *objcparmdecl = Swig_string_ccase(objcparmname);
      Printv(proxyfunctionname, "With", objcparmdecl,":(", objcparmtype, ")", objcparmname, NIL);
      Printf(objcfuncdecl, "With%s:", objcparmdecl);
      Delete(objcparmdecl);
    }
    
    gencomma++;
    
    p = Getattr(p, "tmap:in:next");
    
    Delete(objcparmname);
    Delete(objcparmtype);
  }

  // Deal with overloading  
  checkDuplicateDeclaration(objcfuncdecl, proxyfunctionname);
  Printv(constructor_decl, "- (", objcrettype, ")", proxyfunctionname, NIL);
  
  // First line of function definition
  Printv(constructor_defn, constructor_decl, " {\n", NIL);
  Printf(constructor_decl, ";");

  // End the call to the intermediate function
  Printv(imcall, ")", NIL);

  // Insert the objcconstructor typemap
  Hash *attributes = NewHash();
  String *constructor_code = NewStringEmpty();
  const String *construct_tm = typemapLookup(n, "objcconstructor", name, WARN_NONE, attributes);
  if (construct_tm) {
    Printv(constructor_code, construct_tm, NIL);
    Replaceall(constructor_code, "$imcall", imcall);
  }
  Printf(constructor_defn, " %s\n}\n", constructor_code);

  Delete(attributes);
  Delete(constructor_code);

  /* Write the function declaration to the proxy_class_function_decls 
     and function definition to the proxy_class_function_defns */
  Printv(proxy_class_function_decls, constructor_decl, "\n", NIL);
  Printv(proxy_class_function_defns, constructor_defn, "\n", NIL);

  //Delete(paramstring);
  Delete(proxyfunctionname);
  Delete(objcrettype);
  Delete(imcall);
  Delete(constructor_decl);
  Delete(constructor_defn);
}


/* ---------------------------------------------------------------------------
 * emitProxyClass()
 *
 * Collects all the code fragments generated by the handler function while
 * traversing the tree from the proxy_class_* variables and writes the
 * class definition (including any additional code) to proxy_h and proxy_mm files.
 *
 * Inputs:
 *  n – The class node currently processed.
 * --------------------------------------------------------------------------- */
void OBJECTIVEC::emitProxyClass(Node *n) {
  if (!proxy_flag)
    return;

  SwigType *typemap_lookup_type = Getattr(n, "classtypeobj");

  /*
   * Handle inheriting from ObjectiveC and C++ classes.
   */
  String *baseclass = NULL;
  String *c_baseclassname = NULL;

  // Inheritance from pure ObjectiveC classes.
  Node *attributes = NewHash();
  const String *pure_baseclass = typemapLookup(n, "objcbase", typemap_lookup_type, WARN_NONE, attributes);
  bool purebase_replace = GetFlag(attributes, "tmap:objcbase:replace") ? true : false;
  bool purebase_notderived = GetFlag(attributes, "tmap:objcbase:notderived") ? true : false;
  Delete(attributes);

  // C++ inheritance.
  if (!purebase_replace) {
    List *baselist = Getattr(n, "bases");
    if (baselist) {
      Iterator base = First(baselist);
      while (base.item && GetFlag(base.item, "feature:ignore")) {
	base = Next(base);
      }
      if (base.item) {
	c_baseclassname = Getattr(base.item, "name");
	baseclass = Copy(createProxyName(c_baseclassname));
	base = Next(base);
	/* Warn about multiple inheritance for additional base class(es) */
	while (base.item) {
	  if (GetFlag(base.item, "feature:ignore")) {
	    base = Next(base);
	    continue;
	  }
	  String *proxyclassname = SwigType_str(Getattr(n, "classtypeobj"), 0);
	  String *baseclassname = SwigType_str(Getattr(base.item, "name"), 0);
	  Swig_warning(WARN_OBJC_MULTIPLE_INHERITANCE, Getfile(n), Getline(n),
		       "Base %s of class %s ignored: multiple inheritance is not supported in objective-c.\n", baseclassname, proxyclassname);
	  base = Next(base);
	}
      }
    }
  }

  bool derived = baseclass && createProxyName(c_baseclassname);
  if (derived && purebase_notderived) {
    pure_baseclass = empty_string;
  }
  const String *wanted_base = baseclass ? baseclass : pure_baseclass;

  if (purebase_replace) {
    wanted_base = pure_baseclass;
    derived = false;
    Delete(baseclass);
    baseclass = NULL;
    if (purebase_notderived) {
      Swig_error(Getfile(n), Getline(n),
		 "The objcbase typemap for proxy %s must contain just one of the 'replace' or 'notderived' attributes.\n", typemap_lookup_type);
    }
  } else if (Len(pure_baseclass) > 0 && Len(baseclass) > 0) {
    Swig_warning(WARN_OBJC_MULTIPLE_INHERITANCE, Getfile(n), Getline(n),
		 "Warning for %s proxy: Base class %s ignored. Multiple inheritance is not supported in objective-c. ",
		 "Perhaps you need one of the 'replace' or 'notderived' attributes in the objcbase typemap?\n", typemap_lookup_type, pure_baseclass);
  }

  /* Destructor handling */
  // If the C++ destructor is accessible (public), it is wrapped by the
  // dealloc() method in ObjectiveC. If it is not accessible, dealloc() method is written and throws an exception.
  String *destructor_decl = NewStringEmpty();
  String *destructor_defn = NewStringEmpty();
  const String *tm = NULL;

  String *destructor_methodname;
  String *destructor_methodmodifiers;
  attributes = NewHash();

  tm = typemapLookup(n, "objcdestructor", typemap_lookup_type, WARN_NONE, attributes);
  destructor_methodname = Getattr(attributes, "tmap:objcdestructor:methodname");
  destructor_methodmodifiers = Getattr(attributes, "tmap:objcdestructor:methodmodifiers");

  if (tm && *Char(tm)) {
    if (!destructor_methodname) {
      Swig_error(Getfile(n), Getline(n), "No methodname attribute defined in the objcdestructor typemap for %s\n", proxy_class_name);
    }
    if (!destructor_methodmodifiers) {
      Swig_error(Getfile(n), Getline(n), "No methodmodifiers attribute defined in objcdestructor typemap for %s.\n", proxy_class_name);
    }
  }

  if (tm) {
    // Write the dealloc() method declaration for the proxy interface
    Printv(destructor_decl, "- (void)", destructor_methodname, ";", NIL);

    // And, the dealloc() method definition for the proxy implementation
    String *destructor_code = NewStringEmpty();
    Printv(destructor_code, tm, NIL);

    if (*Char(destrcutor_call)) {
      Replaceall(destructor_code, "$imcall", destrcutor_call);
    } else {
      Replaceall(destructor_code, "$imcall", "[NSException raise:@\"UnsupportedOperationException\" format:@\"%@\", @\"C++ destructor does not have public access\"]");
    }

    if (*Char(destructor_code)) {
      Printv(destructor_defn, "- (void)", destructor_methodname, " {", destructor_code, "\n}\n", NIL);
    }

    Delete(destructor_code);
  }


  /* Write the proxy class declaration */
  // Class modifiers.
  const String *objcinterfacemodifier = typemapLookup(n, "objcinterfacemodifier", typemap_lookup_type, WARN_OBJC_INTERFACE_MOD);

  // User-defined protocols.
  const String *protocols = typemapLookup(n, derived ? "objcprotocols_derived" : "objcprotocols", typemap_lookup_type, WARN_NONE);

  // Default interface code
  const String *objcinterfacecode;
  if (derived) {
    objcinterfacecode = typemapLookup(n, "objcinterfacecode_derived", typemap_lookup_type, WARN_NONE);
  } else {
    objcinterfacecode = typemapLookup(n, "objcinterfacecode", typemap_lookup_type, WARN_NONE);
  }
  
  // the class declaration
  Printv(proxy_class_defns_code, "@class $objcclassname;\n", NIL);

  // the class interface
  Printv(proxy_class_itfc_code, proxy_class_imports,
	 objcinterfacemodifier, " $objcclassname",
	 (*Char(wanted_base) || *Char(protocols)) ? " : " : "", wanted_base,
	 (*Char(wanted_base) && *Char(protocols)) ? ", " : "", protocols,
	 objcinterfacecode, proxy_class_function_decls, destructor_decl, "\n", typemapLookup(n, "objcclassclose", typemap_lookup_type, WARN_NONE), "\n\n", NIL);


  /* Write the proxy class definition */
  // Class modifiers.
  const String *objccimplementationmodifier = typemapLookup(n, "objcimplementationmodifier", typemap_lookup_type, WARN_OBJC_IMPLEMENTATION_MOD);

  // Default implementationcode code
  const String *objcimplementationcode;
  if (derived) {
    objcimplementationcode = typemapLookup(n, "objcimplementationcode_derived", typemap_lookup_type, WARN_NONE);
  } else {
    objcimplementationcode = typemapLookup(n, "objcimplementationcode", typemap_lookup_type, WARN_NONE);
  }

  // the class implementation
  Printv(proxy_class_impl_code, "\n", objccimplementationmodifier, " $objcclassname", objcimplementationcode, "\n",
	 proxy_class_function_defns, destructor_defn, "\n", typemapLookup(n, "objcclassclose", typemap_lookup_type, WARN_NONE), "\n\n", NIL);

  Replaceall(proxy_class_itfc_code, "$objcbaseclass", proxy_class_name);
  Replaceall(proxy_class_impl_code, "$objcbaseclass", proxy_class_name);

  Delete(baseclass);
  Delete(destructor_decl);
  Delete(destructor_defn);
}


/* ---------------------------------------------------------------------------
 * emitTypeWrapperClass()
 * --------------------------------------------------------------------------- */
void OBJECTIVEC::emitTypeWrapperClass(String *classname, SwigType *type) {
  Node *n = NewHash();
  Setfile(n, input_file);
  Setline(n, line_number);

  // Pure ObjectiveC baseclass and interfaces.
  const String *pure_baseclass = typemapLookup(n, "objcbase", type, WARN_NONE);
  const String *pure_interfaces = typemapLookup(n, "objcprotocols", type, WARN_NONE);

  /* Write the type wrapper class declaration */
  // Class modifiers.
  const String *objcinterfacemodifier = typemapLookup(n, "objcinterfacemodifier", type, WARN_OBJC_INTERFACE_MOD);

  // Default interface code
  const String *objcinterfacecode = typemapLookup(n, "objcinterfacecode", type, WARN_NONE);

  Printv(swigtypes_h_code, objcinterfacemodifier, " $objcclassname",
	 (*Char(pure_baseclass) || *Char(pure_interfaces)) ? " : " : "", pure_baseclass,
	 (*Char(pure_baseclass) && *Char(pure_interfaces)) ? ", " : "", pure_interfaces,
	 objcinterfacecode, "\n", typemapLookup(n, "objcclassclose", type, WARN_NONE), "\n\n", NIL);

  /* Write the type wrapper class definition */
  // Class modifiers.
  const String *objccimplementationmodifier = typemapLookup(n, "objcimplementationmodifier", type, WARN_OBJC_IMPLEMENTATION_MOD);

  // Default implementationcode code
  const String *objcimplementationcode = typemapLookup(n, "objcimplementationcode", type, WARN_NONE);

  Printv(swigtypes_mm_code, "\n", objccimplementationmodifier, " $objcclassname", objcimplementationcode,
	 "\n", typemapLookup(n, "objcclassclose", type, WARN_NONE), "\n\n", NIL);

  Printf(proxy_class_defns_code, "@class %s;\n", classname);
  Replaceall(swigtypes_h_code, "$objcclassname", classname);
  Replaceall(swigtypes_mm_code, "$objcclassname", classname);

  Delete(n);
}


/* -----------------------------------------------------------------------------
 * substituteClassname()
 *
 * Substitute $objcclassname with the proxy class name for classes/structs/unions that SWIG knows about.
 * Also substitutes enums with enum name.
 * Otherwise use the $descriptor name for the Objective-C class name. Note that the $&objcclassname substitution
 * is the same as a $&descriptor substitution, ie one pointer added to descriptor name.
 *
 * Inputs:
 *   tm - String to perform the substitution at (will usually come from a
 *        typemap.
 *   pt - The type to substitute for the variables.
 * Outputs:
 *   tm - String with the variables substituted.
 * Return:
 *   substitution_performed - flag indicating if a substitution was performed
 * ----------------------------------------------------------------------------- */

bool OBJECTIVEC::substituteClassname(String *tm, SwigType *pt) {
  bool substitution_performed = false;
  SwigType *type = Copy(SwigType_typedef_resolve_all(pt));
  SwigType *strippedtype = SwigType_strip_qualifiers(type);

  if (Strstr(tm, "$objcclassname")) {
    SwigType *type = Copy(strippedtype);
    substituteClassnameVariable(tm, "$objcclassname", type);
    substitution_performed = true;
    Delete(type);
  }
  if (Strstr(tm, "$*objcclassname")) {
    SwigType *type = Copy(strippedtype);
    Delete(SwigType_pop(type));
    substituteClassnameVariable(tm, "$*objcclassname", type);
    substitution_performed = true;
    Delete(type);
  }
  if (Strstr(tm, "$&objcclassname")) {
    SwigType *type = Copy(strippedtype);
    SwigType_add_pointer(type);
    substituteClassnameVariable(tm, "$&objcclassname", type);
    substitution_performed = true;
    Delete(type);
  }

  Delete(strippedtype);
  Delete(type);

  return substitution_performed;
}

/* -----------------------------------------------------------------------------
 * substituteClassnameVariable()
 * ----------------------------------------------------------------------------- */

void OBJECTIVEC::substituteClassnameVariable(String *tm, const char *classnamevariable, SwigType *type) {
  String *type_name;

  if (SwigType_isenum(type)) {
    Node *n = enumLookup(type);
    String *enum_name = Getattr(n, "sym:name");
    Node *p = parentNode(n);
    
    if (p && !Cmp(nodeType(p), "class")) {
      // This is a nested enum.
      String *parent_name = Getattr(p, "sym:name");
      type_name = NewStringf("%s_%s", parent_name, enum_name);
    } else {
      type_name = Copy(enum_name);
    }
  } else {
    String *class_name = createProxyName(type);
    if (class_name) {
      type_name = Copy(class_name);
    } else {
      // SWIG does not know anything about the type (after resolving typedefs).
      // Just mangle the type name string like $descriptor(type) would do.
      String *descriptor = NewStringf("SWIGTYPE%s", SwigType_manglestr(type));
      type_name = Copy(descriptor);

      // Add to hash table so that the type wrapper classes can be created later
      Setattr(unknown_types, descriptor, type);
      Delete(descriptor);
    }
  }
  Replaceall(tm, classnamevariable, type_name);
  Delete(type_name);
}

/* ---------------------------------------------------------------------
 * skipIgnoredArgs()
 *
 * --------------------------------------------------------------------- */
bool OBJECTIVEC::skipIgnoredArgs(Parm **p) {
  /* Ignored varargs */
  if (checkAttribute(*p, "varargs:ignore", "1")) {
    *p = nextSibling(*p);
	return true;
  }

  /* Ignored parameters */
  if (checkAttribute(*p, "tmap:in:numinputs", "0")) {
    *p = Getattr(*p, "tmap:in:next");
	return true;
  }
  return false;
}

/* ---------------------------------------------------------------------
 * marshalInputArgs()
 *
 * Process all of the arguments passed and convert them into C/C++ 
 * function arguments using the supplied typemaps.
 * --------------------------------------------------------------------- */

void OBJECTIVEC::marshalInputArgs(ParmList *parmlist, Wrapper *wrapper) {
  String *tm;
  Parm *p;
  int i = 0;

  for (p = parmlist; p; i++) {
	if(skipIgnoredArgs(&p))
      continue;
    
    String *arg = NewStringEmpty();
    Printf(arg, "imarg%d", i + 1);

    // Get the "in" typemap for this argument and add to the wrapper->code
    if ((tm = Getattr(p, "tmap:in"))) {
      Replaceall(tm, "$input", arg);
      Setattr(p, "emit:input", arg);
      Printf(wrapper->code, "%s\n", tm);
    } 
    
    p = Getattr(p, "tmap:in:next");
    
    Delete(arg);
  }

}

/* ---------------------------------------------------------------------
 * makeParameterList()
 *
 * --------------------------------------------------------------------- */

void OBJECTIVEC::makeParameterList(ParmList *parmlist, Wrapper *wrapper) {

  /* Emit all of the local variables for holding arguments. */
  emit_parameter_variables(parmlist, wrapper);

  /* Attach the standard typemaps */
  emit_attach_parmmaps(parmlist, wrapper);

  // Attach the non-standard typemaps to the parameter list
  Swig_typemap_attach_parms("imtype", parmlist, wrapper);

  String *tm;
  Parm *p;
  int i = 0;
  int gencomma = 0;

  for (p = parmlist; p; i++) {
    if(skipIgnoredArgs(&p))
      continue;
    
    SwigType *pt = Getattr(p, "type");
    
    String *imparmtype = NewStringEmpty();
    String *arg = NewStringEmpty();

    Printf(arg, "imarg%d", i + 1);

    // Get the intermediate parameter type for this parameter
    if ((tm = Getattr(p, "tmap:imtype"))) {
      Printv(imparmtype, tm, NIL);
      if (gencomma)
	  Printf(wrapper->def, ", ");
      Printv(wrapper->def, imparmtype, " ", arg, NIL);	// Add parameter to the function signature (wrapper->def)
      ++gencomma;
    } else
      Swig_warning(WARN_OBJC_TYPEMAP_IMTYPE_UNDEF, input_file, line_number, "No imtype typemap defined for %s\n", SwigType_str(pt, 0));

    p = Getattr(p, "tmap:in:next");
    
    Delete(imparmtype);
    Delete(arg);
  }
}

/* ---------------------------------------------------------------------
 * marshalOutput()
 *
 * Process the return value of the C/C++ function call
 * and convert it into the intermediate type using the
 * supplied typemaps.
 * --------------------------------------------------------------------- */

void OBJECTIVEC::marshalOutput(Node *n, String *actioncode, Wrapper *wrapper) {
  SwigType *type = Getattr(n, "type");
  // Emit the out typemap
  String *tm;
  if ((tm = Swig_typemap_lookup_out("out", n, "result", wrapper, actioncode))) {
    Replaceall(tm, "$result", "imresult");
    Printf(wrapper->code, "%s", tm);
    if (Len(tm))
      Printf(wrapper->code, "\n");
  } else {
    Swig_warning(WARN_TYPEMAP_OUT_UNDEF, input_file, line_number, "Unable to use return type %s in function %s.\n", SwigType_str(type, 0), Getattr(n, "name"));
  }
  // Emits a variable declaration for a function return value
  emit_return_variable(n, type, wrapper);
}

/* -----------------------------------------------------------------------------
 * createProxyName()
 *
 * Returns the ObjectiveC class name if a type corresponds to something wrapped with a
 * proxy class, NULL otherwise.
 * ----------------------------------------------------------------------------- */

String *OBJECTIVEC::createProxyName(SwigType *t) {	// TODO: See this once more for nspaces.
  if (proxy_flag) {
    Node *n = classLookup(t);
    if (n) {
      return Getattr(n, "sym:name");
    }
  }
  return NULL;
}

/* -----------------------------------------------------------------------------
 * typemapLookup()
 * n - for input only and must contain info for Getfile(n) and Getline(n) to work
 * tmap_method - typemap method name
 * type - typemap type to lookup
 * warning - warning number to issue if no typemaps found
 * typemap_attributes - the typemap attributes are attached to this node and will 
 *   also be used for temporary storage if non null
 * return is never NULL, unlike Swig_typemap_lookup()
 * ----------------------------------------------------------------------------- */

const String *OBJECTIVEC::typemapLookup(Node *n, const_String_or_char_ptr tmap_method, SwigType *type, int warning, Node *typemap_attributes) {
  Node *node = !typemap_attributes ? NewHash() : typemap_attributes;
  Setattr(node, "type", type);
  Setfile(node, Getfile(n));
  Setline(node, Getline(n));
  const String *tm = Swig_typemap_lookup(tmap_method, node, "", 0);
  if (!tm) {
    tm = empty_string;
    if (warning != WARN_NONE)
      Swig_warning(warning, Getfile(n), Getline(n), "No %s typemap defined for %s\n", tmap_method, SwigType_str(type, 0));
  }
  if (!typemap_attributes)
    Delete(node);
  return tm;
}


/* -----------------------------------------------------------------------------
 * swig_objectivec()    - Instantiate module
 * ----------------------------------------------------------------------------- */

static Language *new_swig_objectivec() {
  return new OBJECTIVEC();
}
extern "C" Language *swig_objectivec(void) {
  return new_swig_objectivec();
}

/* -----------------------------------------------------------------------------
 * Static member variables
 * ----------------------------------------------------------------------------- */

// Usage message.
const char *const OBJECTIVEC::usage = (char *) "\
ObjectiveC options (available with -objc)\n\
    -noproxy    - Do not generate proxy files (Only C wrappers will be generated) \n\
    -help       - This message \n\
\n";
