// MINIScript
// ---
// - Building
// [osx]
//   $ gcc -m32 miniscript.c mslib.c readme.c && ./a.out < sample.js
// [arm thumb size]
//   $ ~/gcc-arm-none-eabi-4_9-2015q2/bin/arm-none-eabi-gcc -c -mthumb -Os miniscript.c
//   $ ~/gcc-arm-none-eabi-4_9-2015q2/bin/arm-none-eabi-size -A miniscript.o
// - Changes
// [0.1] 2016.03.11
//   + code parser
//   + allocation: var
//   + operators: =,+,-
//   + statement: ;
//   + native function
// [0.2] 2016.03.16
//   + comment: //
//   + if-else: nested ok
//   + operators: *,/,==,!=,(,)
//   + scope: {}
//   + scoped var
// [0.3] 2016.03.23
//   + while: nested ok
//   + function: prams ok
// [0.4] 2016.04.04
//   + refactored
//   + object: new,.
// [0.5] 2016.0?.??
//   + stack,scope... overflow check
//   + refactored
//   + immediate value: +,-
//   + operators: %,<,>,<=,>=
//   + statement: break
//   ---
//   + multiple var
//   + operators: !
//   + try-catch, throw
//   + tuned for speed

#include <stdio.h>
#include <string.h>
#include "miniscript.config.h"
#include "miniscript.h"

#define SIZE_STACK  0x40
#define SIZE_SCOPE  0x40
#define SIZE_POOL   0x04
#define SIZE_POOLA  0x01



typedef struct{
    Stack base;
    Var vars[SIZE_STACK];
} MyStack;

typedef struct{
    Scope base;
    VarMap nvars[SIZE_SCOPE];
} MyScope;

typedef struct{
    Pool   base;
    ObjectForPool objs[SIZE_POOL];
} MyPool;

typedef struct{
    ArrayPool   base;
    ArrayForPool arrs[SIZE_POOLA];
} MyArrayPool;

Var Stack_OVERFLOW;

void gf_print(Thread* p){
    Var v = *Stack_pop(p->s);
    Var__(   Stack_pop(p->s));
    Var__(   Stack_pop(p->s));
    
    Var* pv = (v.vt == VT_Refer) ? v.ref : &v;
    if(pv->vt == VT_Number){
        printf("%d\n", pv->num);
    }else
    if(pv->vt == VT_CodeString){
        const char* b = pv->code;
        const char* p = b + 1;
        while(*p != *b) printf("%c", *(p++));
        printf("\n");
    }else{
        printf("VT: %d\n", pv->vt);
    }
    
    Var__(&v);
    Stack_push(p->s);//void
}



#define SIZE_SRC    0x1000

int main(){
    char buf[SIZE_SRC];
    size_t n = 0;
    
    size_t r;
    while( 0 < (r = read(0, buf+n, SIZE_SRC-n)) ) n += r;
    buf[n] = '\0';

    {
        MyPool pool;
        Pool_global( Pool_(&pool.base, SIZE_POOL) );
        
        MyArrayPool poola;
        ArrayPool_global( ArrayPool_(&poola.base, SIZE_POOLA) );

        MyStack stack;
        Stack_(&stack.base, SIZE_STACK);
        
        MyScope scope;
        Scope_(&scope.base, SIZE_SCOPE);
        
        Thread thread;
        Thread_(&thread, buf, &stack.base, &scope.base);
        
        Var* pv;
        pv = Scope_add(thread.o, "print", 5, Stack_push(thread.s));{
            pv->vt = VT_Function;
            pv->func = gf_print;
        }
        pv = Scope_add(thread.o, "Array", 5, Stack_push(thread.s));{
            pv->vt = VT_Function;
            pv->func = mslib_Array;
        }
        
        Stack_ground(thread.s);
        
        Error* e = Thread_run(&thread);
                                                                        if(e){
                                                                            char a[0x10];
                                                                            strncpy(a, e->code, e->len)[e->len] = '\0';
                                                                            printf("[ERROR!] %s('%s')\n", e->reason, a);
                                                                        }
        
        Thread__(&thread);
        Scope__(&scope.base);
        Stack__(&stack.base);
        ArrayPool__(&poola.base);
        Pool__(&pool.base);
    }
    
    return 0;
}


