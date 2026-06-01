#ifndef __JEXCEPTION_H__
#define __JEXCEPTION_H__

#include <stdint.h>
#include <setjmp.h>

#define INIT_EXCEPTION_SYSTEM   jmp_buf __j_exception_buff; volatile uint32_t __last_error_code = 0;
#define INIT_EXCEPTION_CUSTOM(ex)	/*jmp_buf __j_exception_buff_##ex; volatile uint32_t __last_error_code_##ex = 0*/;
#define EXTERN_EXCEPTION_CUSTOM(ex)	/*extern jmp_buf __j_exception_buff_##ex; extern volatile uint32_t __last_error_code_##ex*/;

extern jmp_buf __j_exception_buff;
extern volatile uint32_t __last_error_code;

#define GetLastError()  (__last_error_code)

#define try if(0 == setjmp(__j_exception_buff))
#define catch else
#define catchex(ex) else if(__last_error_code == ex)
#define catchgo(lbl) else{goto lbl;}

#define throw(n) {__last_error_code=n; longjmp(__j_exception_buff,1);}


#define tryb(ex) /*if(0 == setjmp(__j_exception_buff_##ex))*/;
#define catchb(ex) /*else*/;
#define throwb(ex,n) /*{__last_error_code_##ex=n; longjmp(__j_exception_buff_##ex,1);}*/;

#endif
