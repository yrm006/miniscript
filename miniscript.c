// MINIScript

#include <stdio.h>  // for debug
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "miniscript.config.h"
#include "miniscript.h"

                                                                        #if DEBUG
                                                                          #define debug printf
                                                                        #else
                                                                          #define debug(...) ;
                                                                          #define dump(...) ;
                                                                        #endif



const char E_STACK_OVERFLOW[]   = "stack overflow.";
const char E_SCOPE_OVERFLOW[]   = "scope overflow.";
const char E_TOO_LONG_NAME[]    = "too long name.";
const char E_COULDNT_NEW[]      = "couldn't new.";
const char E_COULDNT_CREATE[]   = "couldn't create.";
const char E_NOT_FOUND[]        = "not found.";



enum{
    TW_NONE,
    TW_EQ,      // ==
    TW_NEQ,     // !=
    TW_LTEQ,    // <=
    TW_GTEQ,    // >=
    TW_SHRI,    // >>
    TW_SHLE,    // <<
    TW_PARL,    // (
    TW_PARR,    // )
    TW_BRTL,    // [
    TW_BRTR,    // ]
    TW_BRCL,    // {
    TW_BRCR,    // }
    TW_SCLN,    // ;
    TW_COMM,    // ,
    TW_DOT,     // .
    TW_COPY,    // =
    TW_PLUS,    // +
    TW_MINS,    // -
    TW_MULT,    // *
    TW_DIV,     // /
    TW_MOD,     // %
    TW_NOT,     // !
    TW_LT,      // <
    TW_GT,      // >
    TW_AND,     // &
    TW_OR,      // |
    TW_XOR,     // ^
    TW_CHIL,    // ~
};



// Var
Var* Var_(Var* p){
                                                                        // debug("%s: %lu\n", __FUNCTION__, sizeof(*p));
    p->vt = VT_Undefined;
    return p;
}

VarType Var__(Var* p){
    if((p->vt == VT_Object || p->vt == VTX_This) && p->obj){
        Object_release(p->obj);
    }
    return p->vt;
}



// Stack
Stack* Stack_(Stack* p, size_t size){
    p->size = size;
    p->n = 0;
    p->e = NULL;
    return p;
}

void Stack__(Stack* p){
                                                                        debug("%s: %02lx\n", __FUNCTION__, p->n);
    while(p->n) Var__(p->vars + --p->n);
}

Var* Stack_push(Stack* p){
                                                                        // debug("%s: %02lx\n", __FUNCTION__, p->n);
                                                                        if(p->size <= p->n){
                                                                            p->e->reason = E_STACK_OVERFLOW;
                                                                            return Var_(&Stack_OVERFLOW);
                                                                        }
    return Var_(p->vars + (p->n++));
}

Var* Stack_pop(Stack* p){
                                                                        // debug("%s: %02lx(", __FUNCTION__, p->n-1); printv(p->vars+(p->n-1)); printf(")\n");
    return p->vars + (--p->n);
}

Var* Stack_tip(Stack* p, int n){
    return p->vars + (0 <= n ? 0 : p->n) + n;
}

static
size_t Stack_into(Stack* p){
                                                                        debug("%s: %02lx\n", __FUNCTION__, p->n);
    Var* pv = Stack_push(p);{
        pv->vt = VTX_Frame;
        pv->ref = pv;
    }
    
    return p->n;
}

static
size_t Stack_out(Stack* p){
                                                                        debug("%s: %02lx\n", __FUNCTION__, p->n);
    while(Var__(Stack_pop(p)) != VTX_Frame);
    return p->n;
}

void Stack_ground(Stack* p){
                                                                        debug("%s: %02lx\n", __FUNCTION__, p->n);
    size_t i = 0;
    Var* pv;
    while( (pv = Stack_tip(p, -(++i)))->vt != VTX_Frame );
    
    pv->ref = Stack_tip(p, -1);
}

Var* Stack_flatten(Stack* p){
                                                                        debug("%s: %02lx\n", __FUNCTION__, p->n);
    size_t i = 0;
    Var* pv;
    while( (pv = Stack_tip(p, -(++i)))->vt != VTX_Frame );
    
    return (pv->ref == Stack_tip(p, -1)) ? NULL : Stack_pop(p);
}

int  Stack_isground(Stack* p){
                                                                        debug("%s: %02lx\n", __FUNCTION__, p->n);
    size_t i = 0;
    Var* pv;
    while( (pv = Stack_tip(p, -(++i)))->vt != VTX_Frame );
    
    return (pv->ref == Stack_tip(p, -1));
}
                                                                        #if DEBUG
                                                                        struct Thread;
                                                                        static void Thread_op_new(struct Thread*);
                                                                        static void Thread_op_property(struct Thread*);
                                                                        static void Thread_op_copy(struct Thread*);
                                                                        
                                                                        void printv(Var* pv){
                                                                            if(pv->vt == VT_Refer){
                                                                                printf("[R] ");
                                                                                pv = pv->ref;
                                                                            }

                                                                            if(pv->vt == VT_Number){
                                                                                printf("%d", pv->num);
                                                                            }else
                                                                            if(pv->vt == VT_CodeString){
                                                                                const char* b = pv->code;
                                                                                const char* p = b + 1;
                                                                                while(*p != *b) printf("%c", *(p++));
                                                                            }else
                                                                            if(pv->vt == VTX_Call){
                                                                                printf("VTX_Call (%s)", 
                                                                                    pv->func==Thread_op_new      ? "op_new"      :
                                                                                    pv->func==Thread_op_property ? "op_property" :
                                                                                    pv->func==Thread_op_copy     ? "op_copy"     :
                                                                                    "?");
                                                                            }else
                                                                            {
                                                                                char* pvt = NULL;
                                                                                if(pv->vt < VTX_Frame){
                                                                                    static char* ps[] = {
                                                                                        "VT_Undefined",
                                                                                        "VT_Refer",
                                                                                        "VT_Number",
                                                                                        "VT_CodeString",
                                                                                        "VT_Function",
                                                                                        "VT_CodeFunction",
                                                                                        "VT_Object",
                                                                                    };
                                                                                    pvt = ps[pv->vt];
                                                                                }else{
                                                                                    static char* ps[] = {
                                                                                        "VTX_Frame",
                                                                                        "VTX_Call",
                                                                                        "VTX_If",
                                                                                        "VTX_While",
                                                                                        "VTX_Try",
                                                                                        "VTX_Catch",
                                                                                        "VTX_Throw",
                                                                                        "VTX_CodeReturn",
                                                                                        "VTX_Params",
                                                                                        "VTX_Return",
                                                                                        "VTX_This",
                                                                                    };
                                                                                    pvt = ps[pv->vt-VTX_Frame];
                                                                                }
                                                                                printf("%s", pvt);
                                                                            }
                                                                        }

                                                                        char* strcut(const char* c, size_t len){
                                                                            static char a[0x40];
                                                                            strncpy(a, c, len)[len] = '\0';
                                                                            return a;
                                                                        }
                                                                        
                                                                        void dump(Stack* s){
                                                                            printf("---\n");
                                                                            size_t i = 1;
                                                                            while(i <= s->n){
                                                                                printf("%02lx| ", s->n-i);
                                                                                printv(Stack_tip(s, -i));
                                                                                printf("\n");
                                                                                ++i;
                                                                            }
                                                                        }
                                                                        
                                                                        void dumpo(Scope* o){
                                                                            printf("===\n");
                                                                            size_t i = o->n-1;
                                                                            while(i < o->n){
                                                                                printf("%02lx| %s, ", i, o->nvars[i].name);
                                                                                o->nvars[i].pv ? printv(o->nvars[i].pv) : 0;
                                                                                printf("\n");
                                                                                --i;
                                                                            }
                                                                        }
                                                                        #endif
                                                                        


// Scope
Scope* Scope_(Scope* p, size_t size){
    p->size = size;
    p->n = 0;
    p->s = NULL;
    p->e = NULL;
    return p;
}

void Scope__(Scope* p){
    (void)p;
                                                                        debug("%s: %zu\n", __FUNCTION__, p->n);
    // none
}

Var* Scope_add(Scope* p, const char* name, size_t len, Var* pv){
                                                                        debug("%s: %zu\n", __FUNCTION__, p->n);
                                                                        if(p->size <= p->n){
                                                                            p->e->reason = E_SCOPE_OVERFLOW;
                                                                            return pv;
                                                                        }
                                                                        if(MS_SIZE_VARNAME < len+1){
                                                                            p->e->reason = E_TOO_LONG_NAME;
                                                                            return pv;
                                                                        }
    
    strncpy(p->nvars[p->n].name, name, len)[len] = '\0';
    p->nvars[p->n].pv = pv;
    ++p->n;
    
    return pv;
}

static
Var* Scope_find(Scope* p, const char* name, size_t len){
                                                                        // debug("%s: %s, %zu\n", __FUNCTION__, name, len);
    size_t i = p->n;
    while( (--i < p->n) && (p->nvars[i].name[len] || strncmp(p->nvars[i].name, name, len) != 0) );

    // return (i < p->n) ? p->nvars[i].pv : NULL;
    return (i < p->n) ? p->nvars[i].pv : (p->s) ? Scope_find(p->s, name, len) : NULL;
}

static
size_t Scope_into(Scope* p, Var* mark){
                                                                        debug("%s: %p\n", __FUNCTION__, mark);
    Scope_add(p, "", 0, mark);
    
    return p->n;
}

static
size_t Scope_out(Scope* p, Var* mark){
                                                                        debug("%s: %p\n", __FUNCTION__, mark);
    while( p->nvars[--p->n].name[0] || p->nvars[p->n].pv!=mark );

    return p->n;
}



// Object
static
void Object__(Object* p){
                                                                        debug("%s\n", __FUNCTION__);
    size_t i = 0;
    while( (i < p->size) && p->vars[i].name[0] ){
        Var__(&p->vars[i++].var);
    }
}

Object* Object_(Object* p, uint16_t size){
                                                                        debug("%s: %lu\n", __FUNCTION__, sizeof(*p));
    p->dctr = Object__;
    p->size = size;
    p->c = 1;
    
    size_t i = 0;
    while(i < p->size){
        p->vars[i++].name[0] = '\0';
    }
    
    return p;
}

Object* Object_retain(Object* p){
                                                                        debug("%s: %d\n", __FUNCTION__, (p->c + (p->c ? 1 : 0)));
	if(p->c == 0xffff) return p;

	return (p->c += p->c ? 1 : 0) ? p : NULL;
}

size_t Object_release(Object* p){
                                                                        debug("%s: %d\n", __FUNCTION__, (p->c - (p->c ? 1 : 0)));
	if(p->c == 0xffff) return p->c;
	
    if(p->c && !--p->c){
        p->dctr(p);
    }
    return p->c;
}

static
Var* Object_property(Object* p, const char* name, size_t len){
                                                                        debug("%s: %zu\n", __FUNCTION__, (size_t)p->c);
    Var* pv = NULL;
    
    size_t i = 0;
    while(i < p->size){
        if(!p->vars[i].name[0]){
                                                                        if(MS_SIZE_PROPNAME < len+1) return pv;
            strncpy(p->vars[i].name, name, len)[len] = '\0';
            pv = Var_(&p->vars[i].var);
                                                                        debug("[new prop]\n");
            break;
        }else
        if(!p->vars[i].name[len] && strncmp(p->vars[i].name, name, len) == 0){
            pv = &p->vars[i].var;
                                                                        debug("[found prop]\n");
            break;
        }
        ++i;
    }
    
    return pv;
}

static
Var* Object_index(Object* p, size_t i){
                                                                        debug("%s: %zu\n", __FUNCTION__, (size_t)p->c);
    return &((Var*)&p->vars[p->size])[i];
}



// Pool for Object Allocation
Pool* Pool_(Pool* p, size_t size){
                                                                        debug("%s: %lu\n", __FUNCTION__, sizeof(*p));
    p->size = size;
    
    size_t i = 0;
    while(i < p->size){
        Object_release( Object_((Object*)&p->objs[i], 0) );
        ++i;
    }
    
    return p;
}

void Pool__(Pool* p){
                                                                        debug("%s\n", __FUNCTION__);
    size_t i = 0;
    while(i < p->size){
        if(Object_retain((Object*)&p->objs[i])){
                                                                        printf("!!!Leak [%zu: %hu]\n", i, ((Object*)&p->objs[i])->c);//###DBG
            while( Object_release((Object*)&p->objs[i]) );
        }
        ++i;
    }
}

static Pool* g_pPool = NULL;

Pool* Pool_global(Pool* p){
    return (g_pPool = p);
}

static
Object* Pool_new(Pool* p){
    size_t i = 0;
    while( (i < p->size) && Object_retain((Object*)&p->objs[i]) ) Object_release((Object*)&p->objs[i++]);
                                                                        debug("%s: %zu\n", __FUNCTION__, i);
    
    return (i < p->size) ? Object_((Object*)&p->objs[i], MS_SIZE_POOLEDOBJ) : NULL;
}



// Error
static
Error* Error_(Error* p){
    p->code = NULL;
    p->len = 0;
    p->reason = NULL;
    return p;
}

// Thread
static int op__high(void (*op1)(), void (*op2)());

static void Thread_stride(Thread* p, const char* c, size_t len, int tw);
static void Thread_stride_var(Thread* p, const char* c, size_t len, int tw);
static void Thread_stride_function(Thread* p, const char* c, size_t len, int tw);
static void Thread_stride_function_skiptoend(Thread* p, const char* c, size_t len, int tw);
static void Thread_stride_function_call(Thread* p, const char* c, size_t len, int tw);
static void Thread_stride_if_trueended(Thread* p, const char* c, size_t len, int tw);
static void Thread_stride_if_skiptoelse(Thread* p, const char* c, size_t len, int tw);
static void Thread_stride_if_skiptoend(Thread* p, const char* c, size_t len, int tw);
static void Thread_stride_while_skiptoend(Thread* p, const char* c, size_t len, int tw);
#ifdef MS_TRY
static void Thread_stride_try_skiptocatch(Thread* p, const char* c, size_t len, int tw);
static void Thread_stride_try_succeeded(Thread* p, const char* c, size_t len, int tw);
#endif
static void Thread_stride_object_new(Thread* p, const char* c, size_t len, int tw);
static void Thread_stride_object_property(Thread* p, const char* c, size_t len, int tw);

Thread* Thread_(Thread* p, const char* c, Stack* s, Scope* o){
                                                                        debug("%s: %lu\n", __FUNCTION__, sizeof(*p));
    p->c = c;
    p->l = NULL;
    p->s = s;
    p->o = o;
    p->f = Thread_stride;
    Error_(&p->e);
    
    s->e = &p->e;
    o->e = &p->e;

    Stack_into(p->s);
    Scope_into(p->o, NULL);
    
    return p;
}

void Thread__(Thread* p){
                                                                        debug("%s\n", __FUNCTION__);
    Scope_out(p->o, NULL);
    Stack_out(p->s);
}

static
void* Thread_stack_call(Thread* p, void(*pf)()){
    void (*f)() = NULL;
    
    size_t i = 0;
    Var* pv;
    while( ((pv = Stack_tip(p->s, -(++i)))->vt != VTX_Call) && (pv->vt != VTX_Frame) );
    
    if( (pv->vt == VTX_Call) && (!pf || op__high(pv->func, pf)) ){
        (f = pv->func)(p);
    }
    
                                                                        debug("%s: %p\n", __FUNCTION__, f);
    return f;
}

static
void Thread_op_new(Thread* p){
    Var__(    Stack_pop(p->s));  // function returns
    Var vr = *Stack_pop(p->s);   // this object(VTX_This)
    Var__(    Stack_pop(p->s));  // op_(this)
                                                                        debug("%s: VT:%d\n", __FUNCTION__, vr.vt);
   
    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Object;
        pv->obj = vr.obj;
    }
}

static
void Thread_op_thiscall(Thread* p){
    Var vr = *Stack_pop(p->s);   // function returns
    Var__(    Stack_pop(p->s));  // this object(VTX_This)
    Var__(    Stack_pop(p->s));  // op_(this)
                                                                        debug("%s: VT:%d\n", __FUNCTION__, vr.vt);
   
    *Stack_push(p->s) = vr;
}

static
void Thread_op_property(Thread* p){
    Var vr = *Stack_pop(p->s);   // property
    Var__(    Stack_pop(p->s));  // op_(this)
    Var__(    Stack_pop(p->s));  // this object(VT_Object)
                                                                        debug("%s: VT:%d\n", __FUNCTION__, vr.vt);
   
    *Stack_push(p->s) = vr;
}

static
void Thread_op_copy(Thread* p){
    Var v2 = *Stack_pop(p->s);   // right
    Var__(    Stack_pop(p->s));  // op_(this)
    Var v1 = *Stack_pop(p->s);   // left
                                                                        debug("%s: VT:%d\n", __FUNCTION__, v2.vt);
   
    Var__(v1.ref);
    *v1.ref = (v2.vt == VT_Refer) ? *v2.ref : v2;
    if(v1.ref->vt == VT_Object && v1.ref->obj){
        Object_retain(v1.ref->obj);
    }
    
    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Refer;
        pv->ref = v1.ref;
    }
    
    Var__(&v1);
    Var__(&v2);
}

static
void Thread_op_not(Thread* p){
    Var v = *Stack_pop(p->s);  // value
    Var__(   Stack_pop(p->s)); // op_(this)
   
    int32_t num = !((v.vt==VT_Refer)?v.ref->num:v.num);
                                                                        debug("%s: %d\n", __FUNCTION__, num);

    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Number;
        pv->num = num;
    }
    
    Var__(&v);
}

static
void Thread_op_plus(Thread* p){
    Var v2 = *Stack_pop(p->s);   // right
    Var__(    Stack_pop(p->s));  // op_(this)
    Var v1 = *Stack_pop(p->s);   // left
   
    int32_t num = ((v1.vt==VT_Refer)?v1.ref->num:v1.num) + ((v2.vt==VT_Refer)?v2.ref->num:v2.num);
                                                                        debug("%s: %d\n", __FUNCTION__, num);

    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Number;
        pv->num = num;
    }
    
    Var__(&v1);
    Var__(&v2);
}

static
void Thread_op_plus1(Thread* p){
    Var v = *Stack_pop(p->s);   // right
    Var__(   Stack_pop(p->s));  // op_(this)
   
    int32_t num = +((v.vt==VT_Refer)?v.ref->num:v.num);
                                                                        debug("%s: %d\n", __FUNCTION__, num);

    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Number;
        pv->num = num;
    }
    
    Var__(&v);
}

static
void Thread_op_minus(Thread* p){
    Var v2 = *Stack_pop(p->s);   // right
    Var__(    Stack_pop(p->s));  // op_(this)
    Var v1 = *Stack_pop(p->s);   // left
    
    int32_t num = ((v1.vt==VT_Refer)?v1.ref->num:v1.num) - ((v2.vt==VT_Refer)?v2.ref->num:v2.num);
                                                                        debug("%s: %d\n", __FUNCTION__, num);

    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Number;
        pv->num = num;
    }
    
    Var__(&v1);
    Var__(&v2);
}

static
void Thread_op_minus1(Thread* p){
    Var v = *Stack_pop(p->s);   // right
    Var__(   Stack_pop(p->s));  // op_(this)
    
    int32_t num = -((v.vt==VT_Refer)?v.ref->num:v.num);
                                                                        debug("%s: %d\n", __FUNCTION__, num);

    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Number;
        pv->num = num;
    }
    
    Var__(&v);
}

static
void Thread_op_multi(Thread* p){
    Var v2 = *Stack_pop(p->s);   // right
    Var__(    Stack_pop(p->s));  // op_(this)
    Var v1 = *Stack_pop(p->s);   // left
   
    int32_t num = ((v1.vt==VT_Refer)?v1.ref->num:v1.num) * ((v2.vt==VT_Refer)?v2.ref->num:v2.num);
                                                                        debug("%s: %d\n", __FUNCTION__, num);

    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Number;
        pv->num = num;
    }
    
    Var__(&v1);
    Var__(&v2);
}

static
void Thread_op_div(Thread* p){
    Var v2 = *Stack_pop(p->s);   // right
    Var__(    Stack_pop(p->s));  // op_(this)
    Var v1 = *Stack_pop(p->s);   // left
   
    int32_t num = ((v1.vt==VT_Refer)?v1.ref->num:v1.num) / ((v2.vt==VT_Refer)?v2.ref->num:v2.num);
                                                                        debug("%s: %d\n", __FUNCTION__, num);

    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Number;
        pv->num = num;
    }
    
    Var__(&v1);
    Var__(&v2);
}

static
void Thread_op_mod(Thread* p){
    Var v2 = *Stack_pop(p->s);   // right
    Var__(    Stack_pop(p->s));  // op_(this)
    Var v1 = *Stack_pop(p->s);   // left
   
    int32_t num = ((v1.vt==VT_Refer)?v1.ref->num:v1.num) % ((v2.vt==VT_Refer)?v2.ref->num:v2.num);
                                                                        debug("%s: %d\n", __FUNCTION__, num);

    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Number;
        pv->num = num;
    }
    
    Var__(&v1);
    Var__(&v2);
}

static
void Thread_op_equal(Thread* p){
    Var v2 = *Stack_pop(p->s);   // right
    Var__(    Stack_pop(p->s));  // op_(this)
    Var v1 = *Stack_pop(p->s);   // left
   
    int32_t num = ((v1.vt==VT_Refer)?v1.ref->num:v1.num) == ((v2.vt==VT_Refer)?v2.ref->num:v2.num);
                                                                        debug("%s: %d\n", __FUNCTION__, num);

    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Number;
        pv->num = num;
    }
    
    Var__(&v1);
    Var__(&v2);
}

static
void Thread_op_noteq(Thread* p){
    Var v2 = *Stack_pop(p->s);   // right
    Var__(    Stack_pop(p->s));  // op_(this)
    Var v1 = *Stack_pop(p->s);   // left
   
    int32_t num = ((v1.vt==VT_Refer)?v1.ref->num:v1.num) != ((v2.vt==VT_Refer)?v2.ref->num:v2.num);
                                                                        debug("%s: %d\n", __FUNCTION__, num);

    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Number;
        pv->num = num;
    }
    
    Var__(&v1);
    Var__(&v2);
}

static
void Thread_op_lthaneq(Thread* p){
    Var v2 = *Stack_pop(p->s);   // right
    Var__(    Stack_pop(p->s));  // op_(this)
    Var v1 = *Stack_pop(p->s);   // left
   
    int32_t num = ((v1.vt==VT_Refer)?v1.ref->num:v1.num) <= ((v2.vt==VT_Refer)?v2.ref->num:v2.num);
                                                                        debug("%s: %d\n", __FUNCTION__, num);

    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Number;
        pv->num = num;
    }
    
    Var__(&v1);
    Var__(&v2);
}

static
void Thread_op_gthaneq(Thread* p){
    Var v2 = *Stack_pop(p->s);   // right
    Var__(    Stack_pop(p->s));  // op_(this)
    Var v1 = *Stack_pop(p->s);   // left
   
    int32_t num = ((v1.vt==VT_Refer)?v1.ref->num:v1.num) >= ((v2.vt==VT_Refer)?v2.ref->num:v2.num);
                                                                        debug("%s: %d\n", __FUNCTION__, num);

    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Number;
        pv->num = num;
    }
    
    Var__(&v1);
    Var__(&v2);
}

static
void Thread_op_lthan(Thread* p){
    Var v2 = *Stack_pop(p->s);   // right
    Var__(    Stack_pop(p->s));  // op_(this)
    Var v1 = *Stack_pop(p->s);   // left
   
    int32_t num = ((v1.vt==VT_Refer)?v1.ref->num:v1.num) < ((v2.vt==VT_Refer)?v2.ref->num:v2.num);
                                                                        debug("%s: %d\n", __FUNCTION__, num);

    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Number;
        pv->num = num;
    }
    
    Var__(&v1);
    Var__(&v2);
}

static
void Thread_op_gthan(Thread* p){
    Var v2 = *Stack_pop(p->s);   // right
    Var__(    Stack_pop(p->s));  // op_(this)
    Var v1 = *Stack_pop(p->s);   // left
   
    int32_t num = ((v1.vt==VT_Refer)?v1.ref->num:v1.num) > ((v2.vt==VT_Refer)?v2.ref->num:v2.num);
                                                                        debug("%s: %d\n", __FUNCTION__, num);

    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Number;
        pv->num = num;
    }
    
    Var__(&v1);
    Var__(&v2);
}

static
void Thread_op_bitshiftr(Thread* p){
    Var v2 = *Stack_pop(p->s);   // right
    Var__(    Stack_pop(p->s));  // op_(this)
    Var v1 = *Stack_pop(p->s);   // left
   
    int32_t num = ((v1.vt==VT_Refer)?v1.ref->num:v1.num) >> ((v2.vt==VT_Refer)?v2.ref->num:v2.num);
                                                                        debug("%s: %d\n", __FUNCTION__, num);

    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Number;
        pv->num = num;
    }
    
    Var__(&v1);
    Var__(&v2);
}

static
void Thread_op_bitshiftl(Thread* p){
    Var v2 = *Stack_pop(p->s);   // right
    Var__(    Stack_pop(p->s));  // op_(this)
    Var v1 = *Stack_pop(p->s);   // left
   
    int32_t num = ((v1.vt==VT_Refer)?v1.ref->num:v1.num) << ((v2.vt==VT_Refer)?v2.ref->num:v2.num);
                                                                        debug("%s: %d\n", __FUNCTION__, num);

    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Number;
        pv->num = num;
    }
    
    Var__(&v1);
    Var__(&v2);
}

static
void Thread_op_bitand(Thread* p){
    Var v2 = *Stack_pop(p->s);   // right
    Var__(    Stack_pop(p->s));  // op_(this)
    Var v1 = *Stack_pop(p->s);   // left
   
    int32_t num = ((v1.vt==VT_Refer)?v1.ref->num:v1.num) & ((v2.vt==VT_Refer)?v2.ref->num:v2.num);
                                                                        debug("%s: %d\n", __FUNCTION__, num);

    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Number;
        pv->num = num;
    }
    
    Var__(&v1);
    Var__(&v2);
}

static
void Thread_op_bitor(Thread* p){
    Var v2 = *Stack_pop(p->s);   // right
    Var__(    Stack_pop(p->s));  // op_(this)
    Var v1 = *Stack_pop(p->s);   // left
   
    int32_t num = ((v1.vt==VT_Refer)?v1.ref->num:v1.num) | ((v2.vt==VT_Refer)?v2.ref->num:v2.num);
                                                                        debug("%s: %d\n", __FUNCTION__, num);

    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Number;
        pv->num = num;
    }
    
    Var__(&v1);
    Var__(&v2);
}

static
void Thread_op_bitxor(Thread* p){
    Var v2 = *Stack_pop(p->s);   // right
    Var__(    Stack_pop(p->s));  // op_(this)
    Var v1 = *Stack_pop(p->s);   // left
   
    int32_t num = ((v1.vt==VT_Refer)?v1.ref->num:v1.num) ^ ((v2.vt==VT_Refer)?v2.ref->num:v2.num);
                                                                        debug("%s: %d\n", __FUNCTION__, num);

    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Number;
        pv->num = num;
    }
    
    Var__(&v1);
    Var__(&v2);
}

static
void Thread_op_bitnot(Thread* p){
    Var v = *Stack_pop(p->s);  // value
    Var__(   Stack_pop(p->s)); // op_(this)
   
    int32_t num = ~((v.vt==VT_Refer)?v.ref->num:v.num);
                                                                        debug("%s: %d\n", __FUNCTION__, num);

    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Number;
        pv->num = num;
    }
    
    Var__(&v);
}

static
void Thread_op_exp(Thread* p){
    Var v = *Stack_pop(p->s);  // value
    Var__(   Stack_pop(p->s)); // op_(this)
                                                                        debug("%s: %d\n", __FUNCTION__, v.num);
    
    *Stack_push(p->s) = v;
}

static
int op__high(void (*op1)(), void (*op2)()){
    static void (*ops[])() = {
        Thread_op_new,
        Thread_op_thiscall,
        Thread_op_property,
        Thread_op_minus1,
        Thread_op_plus1,
        Thread_op_not,
        Thread_op_bitnot,
        Thread_op_multi,
        Thread_op_div,
        Thread_op_mod,
        Thread_op_minus,
        Thread_op_plus,
        Thread_op_bitshiftr,
        Thread_op_bitshiftl,
        Thread_op_lthan,
        Thread_op_gthan,
        Thread_op_lthaneq,
        Thread_op_gthaneq,
        Thread_op_equal,
        Thread_op_noteq,
        Thread_op_bitand,
        Thread_op_bitxor,
        Thread_op_bitor,
        Thread_op_copy,
        NULL
    };
    
    static uint8_t opp[] = {
        0, // Thread_op_new,
        0, // Thread_op_thiscall,
        0, // Thread_op_property,
        1, // Thread_op_minus1,
        1, // Thread_op_plus1,
        1, // Thread_op_not,
        1, // Thread_op_bitnot,
        2, // Thread_op_multi,
        2, // Thread_op_div,
        2, // Thread_op_mod,
        3, // Thread_op_minus,
        3, // Thread_op_plus,
        4, // Thread_op_bitshiftr,
        4, // Thread_op_bitshiftl,
        5, // Thread_op_lthan,
        5, // Thread_op_gthan,
        5, // Thread_op_lthaneq,
        5, // Thread_op_gthaneq,
        6, // Thread_op_equal,
        6, // Thread_op_noteq,
        7, // Thread_op_bitand,
        8, // Thread_op_bitxor,
        9, // Thread_op_bitor,
       10  // Thread_op_copy,
           // NULL
    };

    uint8_t p1 = UINT8_MAX;
    uint8_t p2 = UINT8_MAX;
    int i = 0;
    while(ops[i]){
        if(ops[i] == op1) p1 = opp[i];
        if(ops[i] == op2) p2 = opp[i];
        ++i;
    }

                                                                        debug("%s: %d\n", __FUNCTION__, (p1 <= p2));
    return (p1 <= p2);
}

static
void Thread_oth_codecall(Thread* p){
                                                                        debug("%s: %p\n", __FUNCTION__, p);
    size_t i = 0;
    Var* pv;
    while( (pv = Stack_tip(p->s, -(++i)))->vt != VTX_Call );
    
    pv->vt = VTX_CodeReturn;
    pv->code = p->c;
    
    pv = Stack_tip(p->s, -i+1);
    p->c = (pv->vt == VT_Refer) ? pv->ref->code : pv->code;
    p->f = Thread_stride_function_call;
    
    pv = Stack_push(p->s);{
        pv->vt = VTX_Params;
        pv->num = i - 3;
    }
}



static
void Thread_stride_TW_EQ(Thread* p){// ==
    Thread_stack_call(p, Thread_op_equal);
    
    Var* pv = Stack_push(p->s);{
        pv->vt = VTX_Call;
        pv->func = Thread_op_equal;
    }
}

static
void Thread_stride_TW_NEQ(Thread* p){// !=
    Thread_stack_call(p, Thread_op_noteq);
    
    Var* pv = Stack_push(p->s);{
        pv->vt = VTX_Call;
        pv->func = Thread_op_noteq;
    }
}

static
void Thread_stride_TW_LTEQ(Thread* p){// <=
    Thread_stack_call(p, Thread_op_lthaneq);
    
    Var* pv = Stack_push(p->s);{
        pv->vt = VTX_Call;
        pv->func = Thread_op_lthaneq;
    }
}

static
void Thread_stride_TW_GTEQ(Thread* p){// >=
    Thread_stack_call(p, Thread_op_gthaneq);
    
    Var* pv = Stack_push(p->s);{
        pv->vt = VTX_Call;
        pv->func = Thread_op_gthaneq;
    }
}

static
void Thread_stride_TW_SHRI(Thread* p){// >>
    Thread_stack_call(p, Thread_op_bitshiftr);
    
    Var* pv = Stack_push(p->s);{
        pv->vt = VTX_Call;
        pv->func = Thread_op_bitshiftr;
    }
}

static
void Thread_stride_TW_SHLE(Thread* p){// <<
    Thread_stack_call(p, Thread_op_bitshiftl);
    
    Var* pv = Stack_push(p->s);{
        pv->vt = VTX_Call;
        pv->func = Thread_op_bitshiftl;
    }
}

static
void Thread_stride_TW_PARL(Thread* p){// (
    Var* pp = Stack_tip(p->s, -1);
    if(pp->vt == VT_Refer) pp = pp->ref;

    int gnd = Stack_isground(p->s);

    if(!gnd && pp->vt == VT_Function){
        Var prev = *Stack_pop(p->s);
        Var* pv = Stack_push(p->s);{
            pv->vt = VTX_Call;
            pv->func = (prev.vt == VT_Refer) ? prev.ref->func : prev.func;
        }

        Stack_into(p->s);
    }else
    if(!gnd && pp->vt == VT_CodeFunction){
        Var prev = *Stack_pop(p->s);
        Var* pv = Stack_push(p->s);{
            pv->vt = VTX_Call;
            pv->func = Thread_oth_codecall;
        }
        *Stack_push(p->s) = prev;

        Stack_into(p->s);
    }else
    {
        Var* pv = Stack_push(p->s);{
            pv->vt = VTX_Call;
            pv->func = Thread_op_exp;
        }
    }
}

static
void Thread_stride_TW_PARR(Thread* p){// )
    void* f;
    while((f = Thread_stack_call(p, NULL)) && (f != Thread_op_exp) && op__high(f, NULL));

    if(!f){
        size_t i = 0;
        Var* pv;
        while( (pv = Stack_tip(p->s, -(++i)))->vt != VTX_Frame );
        
        pv->vt = VT_Undefined;
        
        Thread_stack_call(p, NULL);
    }else
    if(f == Thread_op_exp){
        if(Stack_tip(p->s, -2)->vt == VTX_If){
            Var* pc = Stack_pop(p->s);
            if(pc->vt == VT_Refer ? pc->ref->num : pc->num){
                // none
            }else{
                p->f = Thread_stride_if_skiptoelse;
            }
        }else
        if(Stack_tip(p->s, -2)->vt == VTX_While){
            Var* pc = Stack_pop(p->s);
            if(pc->vt == VT_Refer ? pc->ref->num : pc->num){
                // none
            }else{
                Stack_tip(p->s, -1)->num = 0;
                p->f = Thread_stride_while_skiptoend;
            }
        }
    }
}

static
void Thread_stride_TW_BRTL(Thread* p){// [
    Var* pv = Stack_push(p->s);{
        pv->vt = VTX_Call;
        pv->func = Thread_op_exp;
    }
}

static
void Thread_stride_TW_BRTR(Thread* p){// ]
    while(!( Thread_stack_call(p, NULL) == Thread_op_exp ));

    Var vi = *Stack_pop(p->s);
    Var* pi = (vi.vt == VT_Refer) ? vi.ref : &vi;
    Var vo = *Stack_pop(p->s);
    Var* po = (vo.vt == VT_Refer) ? vo.ref : &vo;
    
    Var* pr = Stack_push(p->s);{
        pr->vt = VT_Refer;
        pr->ref = Object_index(po->obj, pi->num);
    }
}

static
void Thread_stride_TW_BRCL(Thread* p){// {
    Stack_into(p->s);
    Scope_into(p->o, NULL);
}

static
void Thread_stride_TW_BRCR(Thread* p){// }
    Stack_out(p->s);
    Scope_out(p->o, NULL);
    
    if(Stack_tip(p->s, -1)->vt == VTX_If){
        p->f = Thread_stride_if_trueended;
    }else
    if(Stack_tip(p->s, -1)->vt == VTX_While){
        p->c = Stack_pop(p->s)->code;
    }else
#ifdef MS_TRY
    if(Stack_tip(p->s, -1)->vt == VTX_Try){
        p->f = Thread_stride_try_succeeded;
    }else
    if(Stack_tip(p->s, -2)->vt == VTX_Catch){
        Var__(Stack_pop(p->s));
        Scope_out(p->o, Stack_pop(p->s));
    }else
#endif
    if(Stack_tip(p->s, -1)->vt == VTX_Params){
        Var* pv;
        while( (pv = Stack_pop(p->s))->vt != VTX_CodeReturn ) Var__(pv);
        
        Scope_out(p->o, pv);
        p->c = pv->code;
        
        Stack_push(p->s);//void
                                                                    debug("CodeReturn: %d\n", __LINE__);
    }
}

static
void Thread_stride_TW_SCLN(Thread* p){// ;
    while( Thread_stack_call(p, NULL) );

    if(Stack_tip(p->s, -1)->vt == VTX_If){
        Stack_push(p->s);//void
    }else
    if(Stack_tip(p->s, -1)->vt == VTX_While){
        Stack_push(p->s);//void
    }else
    if(Stack_tip(p->s, -1)->vt == VTX_Return){
        Stack_push(p->s);//void
    }else
#ifdef MS_TRY
    if(Stack_tip(p->s, -1)->vt == VTX_Throw){
        Stack_push(p->s);//void
    }else
#endif
	{}

    Var* pf = Stack_flatten(p->s);
    
    Var vf;
    if(pf){
        vf = *pf;
    }else{
        Var_(&vf);
    }

    if(Stack_tip(p->s, -1)->vt == VTX_If){
        p->f = Thread_stride_if_trueended;
    }else
    if(Stack_tip(p->s, -1)->vt == VTX_While){
        p->c = Stack_pop(p->s)->code;
    }else
    if(Stack_tip(p->s, -1)->vt == VTX_Return){
        Var* pv;
        while( (pv = Stack_pop(p->s))->vt != VTX_CodeReturn ) Var__(pv);
        
        Scope_out(p->o, pv);
        p->c = pv->code;
        
        *Stack_push(p->s) = vf;
        vf.vt = VT_Undefined;
                                                                    debug("CodeReturn: %d\n", __LINE__);
    }else
#ifdef MS_TRY
    if(Stack_tip(p->s, -1)->vt == VTX_Throw){
        Var* pv;
        while(!( (pv = Stack_tip(p->s, -1))->vt == VTX_Try )){
            if(pv->vt == VTX_Frame){
                Scope_out(p->o, NULL);
            }else
            if(pv->vt == VTX_CodeReturn){
                Scope_out(p->o, pv);
            }
            Var__(Stack_pop(p->s));
        }
        
        p->c = pv->code;
        
        *Stack_push(p->s) = vf;
        vf.vt = VT_Undefined;
                                                                    debug("Throw: %d\n", __LINE__);
    }else
#endif
	{}

    Var__(&vf);
}

static
void Thread_stride_TW_COMM(Thread* p){// ,
    while( Thread_stack_call(p, NULL) );
    
    Var* pv1 = Stack_tip(p->s, -1);
    Var* pv2 = Stack_tip(p->s, -2);
    if( (pv1->vt == VT_Refer) && (pv1->ref == pv2) ){
        Var__(Stack_pop(p->s));
        p->f = Thread_stride_var;
    }
}

static
void Thread_stride_TW_DOT(Thread* p){// .
    Thread_stack_call(p, Thread_op_property);
    
    Var* pv = Stack_push(p->s);{
        pv->vt = VTX_Call;
        pv->func = Thread_op_property;
    }
    
    p->f = Thread_stride_object_property;
}

static
void Thread_stride_TW_COPY(Thread* p){// =
    Thread_stack_call(p, Thread_op_copy);

    Var* pv = Stack_push(p->s);{
        pv->vt = VTX_Call;
        pv->func = Thread_op_copy;
    }
}

static
void Thread_stride_TW_PLUS(Thread* p){// +
    Var* pv1 = Stack_tip(p->s, -1);

    if(p->l == Thread_stride_TW_COMM || VTX_Frame <= pv1->vt){
        Thread_stack_call(p, Thread_op_plus1);
        
        Var* pv = Stack_push(p->s);{
            pv->vt = VTX_Call;
            pv->func = Thread_op_plus1;
        }
    }else{
        while( Thread_stack_call(p, Thread_op_plus) );//#!#
        
        Var* pv = Stack_push(p->s);{
            pv->vt = VTX_Call;
            pv->func = Thread_op_plus;
        }
    }
}

static
void Thread_stride_TW_MINS(Thread* p){// -
    Var* pv1 = Stack_tip(p->s, -1);

    if(p->l == Thread_stride_TW_COMM || VTX_Frame <= pv1->vt){
        Thread_stack_call(p, Thread_op_minus1);
        
        Var* pv = Stack_push(p->s);{
            pv->vt = VTX_Call;
            pv->func = Thread_op_minus1;
        }
    }else{
        while( Thread_stack_call(p, Thread_op_minus) );//#!#
        
        Var* pv = Stack_push(p->s);{
            pv->vt = VTX_Call;
            pv->func = Thread_op_minus;
        }
    }
}

static
void Thread_stride_TW_MULT(Thread* p){// *
    Thread_stack_call(p, Thread_op_multi);
    
    Var* pv = Stack_push(p->s);{
        pv->vt = VTX_Call;
        pv->func = Thread_op_multi;
    }
}

static
void Thread_stride_TW_DIV(Thread* p){// /
    Thread_stack_call(p, Thread_op_div);
    
    Var* pv = Stack_push(p->s);{
        pv->vt = VTX_Call;
        pv->func = Thread_op_div;
    }
}

static
void Thread_stride_TW_MOD(Thread* p){// %
    Thread_stack_call(p, Thread_op_mod);
    
    Var* pv = Stack_push(p->s);{
        pv->vt = VTX_Call;
        pv->func = Thread_op_mod;
    }
}

static
void Thread_stride_TW_NOT(Thread* p){// !
    Thread_stack_call(p, Thread_op_not);
    
    Var* pv = Stack_push(p->s);{
        pv->vt = VTX_Call;
        pv->func = Thread_op_not;
    }
}

static
void Thread_stride_TW_LT(Thread* p){// <
    Thread_stack_call(p, Thread_op_lthan);
    
    Var* pv = Stack_push(p->s);{
        pv->vt = VTX_Call;
        pv->func = Thread_op_lthan;
    }
}

static
void Thread_stride_TW_GT(Thread* p){// >
    Thread_stack_call(p, Thread_op_gthan);
    
    Var* pv = Stack_push(p->s);{
        pv->vt = VTX_Call;
        pv->func = Thread_op_gthan;
    }
}

static
void Thread_stride_TW_AND(Thread* p){// &
    Thread_stack_call(p, Thread_op_bitand);
    
    Var* pv = Stack_push(p->s);{
        pv->vt = VTX_Call;
        pv->func = Thread_op_bitand;
    }
}

static
void Thread_stride_TW_OR(Thread* p){// |
    Thread_stack_call(p, Thread_op_bitor);
    
    Var* pv = Stack_push(p->s);{
        pv->vt = VTX_Call;
        pv->func = Thread_op_bitor;
    }
}

static
void Thread_stride_TW_XOR(Thread* p){// ^
    Thread_stack_call(p, Thread_op_bitxor);
    
    Var* pv = Stack_push(p->s);{
        pv->vt = VTX_Call;
        pv->func = Thread_op_bitxor;
    }
}

static
void Thread_stride_TW_BNOT(Thread* p){// ~
    Thread_stack_call(p, Thread_op_bitnot);
    
    Var* pv = Stack_push(p->s);{
        pv->vt = VTX_Call;
        pv->func = Thread_op_bitnot;
    }
}

static
void Thread_stride_TW_OTHER(Thread* p){// dummy(reserved words) for p->l
    (void)p;
}

static
void Thread_stride_TW_LITERAL(Thread* p){// dummy(literal) for p->l
    (void)p;
}

static
void Thread_stride(Thread* p, const char* c, size_t len, int tw){
                                                                        debug("%s: %s\n", __FUNCTION__, strcut(c, len));
    static void (* const tws[])(Thread*) = {
        NULL,
        Thread_stride_TW_EQ,
        Thread_stride_TW_NEQ,
        Thread_stride_TW_LTEQ,
        Thread_stride_TW_GTEQ,
        Thread_stride_TW_SHRI,
        Thread_stride_TW_SHLE,
        Thread_stride_TW_PARL,
        Thread_stride_TW_PARR,
        Thread_stride_TW_BRTL,
        Thread_stride_TW_BRTR,
        Thread_stride_TW_BRCL,
        Thread_stride_TW_BRCR,
        Thread_stride_TW_SCLN,
        Thread_stride_TW_COMM,
        Thread_stride_TW_DOT,
        Thread_stride_TW_COPY,
        Thread_stride_TW_PLUS,
        Thread_stride_TW_MINS,
        Thread_stride_TW_MULT,
        Thread_stride_TW_DIV,
        Thread_stride_TW_MOD,
        Thread_stride_TW_NOT,
        Thread_stride_TW_LT,
        Thread_stride_TW_GT,
        Thread_stride_TW_AND,
        Thread_stride_TW_OR,
        Thread_stride_TW_XOR,
        Thread_stride_TW_BNOT,
    };

    void (*ls)(Thread*) = Thread_stride_TW_OTHER;

    if(tws[tw]){
        tws[tw](p);

        ls = tws[tw];
    }else    
    if(len == 3 && strncmp("var", c, len) == 0){
        p->f = Thread_stride_var;
    }else
    if(len == 8 && strncmp("function", c, len) == 0){
        p->f = Thread_stride_function;
    }else
    if(len == 6 && strncmp("return", c, len) == 0){
        Var* pv = Stack_push(p->s);{
            pv->vt = VTX_Return;
        }
    }else
    if(len == 3 && strncmp("new", c, len) == 0){
        Thread_stack_call(p, Thread_op_new);
        
        Var* pv = Stack_push(p->s);{
            pv->vt = VTX_Call;
            pv->func = Thread_op_new;
        }
        
        p->f = Thread_stride_object_new;
    }else
    if(len == 2 && strncmp("if", c, len) == 0){
        Var* pv = Stack_push(p->s);{
            pv->vt = VTX_If;
            pv->num = 0;
        }
                                                                        debug("VTX_If: %d\n", __LINE__);
    }else
    if(len == 5 && strncmp("while", c, len) == 0){
        Var* pv = Stack_push(p->s);{
            pv->vt = VTX_While;
            pv->code = c;
        }
                                                                        debug("VTX_While: %d\n", __LINE__);
    }else
    if(len == 5 && strncmp("break", c, len) == 0){
        Var* pv;
        while(!( (pv = Stack_tip(p->s, -1))->vt == VTX_While )){
            if( Var__( Stack_pop(p->s) ) == VTX_Frame ){
                Scope_out(p->o, NULL);
            }
        }
        
        p->c = pv->code;
        pv->num = 0;
        p->f = Thread_stride_while_skiptoend;
    }else
#ifdef MS_TRY
    if(len == 3 && strncmp("try", c, len) == 0){
        Var* prev2 = Stack_tip(p->s, -2);
        if( (prev2->vt==VTX_Try) && (prev2->code==c) ){
            prev2->vt = VTX_Catch;
            prev2->num = 0;
            p->f = Thread_stride_try_skiptocatch;
        }else{
            Var* pv = Stack_push(p->s);{
                pv->vt = VTX_Try;
                pv->code = c;
            }
        }
    }else
    if(len == 5 && strncmp("throw", c, len) == 0){
        Var* pv = Stack_push(p->s);{
            pv->vt = VTX_Throw;
        }
    }else
#endif
    {
        if( '0'<=c[0] && c[0]<='9' ){
            Var* pv = Stack_push(p->s);{
                pv->vt = VT_Number;
                pv->num = atoi(c);
            }
        }else
        if(c[0] == '\"'){
            Var* pv = Stack_push(p->s);{
                pv->vt = VT_CodeString;
                pv->code = c;
            }
        }else
        {
            Var* pf = Scope_find(p->o, c, len);
                                                                        if(!pf){ p->e.reason=E_NOT_FOUND; return; }
            
            Var* pv = Stack_push(p->s);{
                pv->vt = VT_Refer;
                pv->ref = (pf->vt == VT_Refer) ? pf->ref : pf;
            }
        }

        ls = Thread_stride_TW_LITERAL;
    }

    p->l = ls;
}

static
void Thread_stride_var(Thread* p, const char* c, size_t len, int tw){
                                                                        debug("%s: %s\n", __FUNCTION__, strcut(c, len));
    (void)tw;
    Var* pa = Scope_add(p->o, c, len, Stack_push(p->s));
    
    Stack_ground(p->s);
    
    Var* pv = Stack_push(p->s);{
        pv->vt = VT_Refer;
        pv->ref = pa;
    }
    
    p->f = Thread_stride;
}

static
void Thread_stride_function(Thread* p, const char* c, size_t len, int tw){
                                                                        debug("%s: %s\n", __FUNCTION__, strcut(c, len));
    (void)tw;
    if(len == 1 && strncmp("(", c, len) == 0){
        Var* pp = Stack_tip(p->s, -1);
        if(pp->vt != VT_CodeFunction){
            pp = Stack_push(p->s);{
                pp->vt = VT_CodeFunction;
                pp->code = NULL;
            }
        }
        
        pp->code = c;
        
        Var* pv = Stack_push(p->s);{
            pv->vt = VT_CodeFunction;
            pv->num = 0;// for '{' '}' skip count.
        }
        
        p->f = Thread_stride_function_skiptoend;
    }else
    {
        Var* pv = Scope_add(p->o, c, len, Stack_push(p->s));{
            pv->vt = VT_CodeFunction;
            pv->code = NULL;                                             // will be set when '(' is appeared.
        }
        
        Stack_ground(p->s);
    }
}

static
void Thread_stride_function_skiptoend(Thread* p, const char* c, size_t len, int tw){
                                                                        debug("%s: %s\n", __FUNCTION__, strcut(c, len));
    (void)tw;
    if(len == 1 && strncmp("{", c, len) == 0){
        ++Stack_tip(p->s, -1)->num;
    }else
    if(len == 1 && strncmp("}", c, len) == 0){
        if(--Stack_tip(p->s, -1)->num == 0){
            Stack_pop(p->s);
            p->f = Thread_stride;
        }
    }else
    {
        // skip
    }
}

static
void Thread_stride_function_call(Thread* p, const char* c, size_t len, int tw){
                                                                        debug("%s: %s\n", __FUNCTION__, strcut(c, len));
    (void)tw;
    if(len == 1 && strncmp("(", c, len) == 0){
        size_t i = 0;
        Var* pv;
        while( (pv = Stack_tip(p->s, -(++i)))->vt != VTX_CodeReturn );
        
        Scope_into(p->o, pv);
        
        Var* pc = Stack_tip(p->s, -i-1);
        if(pc->vt == VTX_This){
            Var* pthis = Stack_tip(p->s, -i+2);{
                pthis->vt = VT_Object;
                pthis->obj = Object_retain(pc->obj);
            }
            Scope_add(p->o, "this", 4, pthis);
        }
    }else
    if(len == 1 && strncmp(")", c, len) == 0){
        p->f = Thread_stride;
    }else
    if(len == 1 && strncmp(",", c, len) == 0){
        // none
    }else
    {
        Var* pv = Stack_tip(p->s, -1);
        if(pv->num){
            Var* pa = Scope_add(p->o, c, len, Stack_tip(p->s, -1-(pv->num--)));
            if(pa->vt == VT_Refer){
                *pa = *pa->ref;
                if(pa->vt == VT_Object && pa->obj){
                    Object_retain(pa->obj);
                }
            }
        }
    }
}

static
void Thread_stride_if_trueended(Thread* p, const char* c, size_t len, int tw){
                                                                        debug("%s: %s\n", __FUNCTION__, strcut(c, len));
    (void)tw;
    if(len == 4 && strncmp("else", c, len) == 0){
        p->f = Thread_stride_if_skiptoend;
    }else
    {
        Stack_pop(p->s);
                                                                        debug("___If: %d\n", __LINE__);
        (p->f = Thread_stride)(p, c, len, tw);
    }
}

static
void Thread_stride_if_skiptoelse(Thread* p, const char* c, size_t len, int tw){
                                                                        debug("%s: %s\n", __FUNCTION__, strcut(c, len));
    (void)tw;
    if(Stack_tip(p->s, -1)->num < 0){
        Stack_pop(p->s);
        if(len == 4 && strncmp("else", c, len) == 0){
                                                                        debug("___If: %d\n", __LINE__);
            p->f = Thread_stride;
        }else{
                                                                        debug("___If: %d\n", __LINE__);
            (p->f = Thread_stride)(p, c, len, tw);
        }
    }else
    if(len == 1 && strncmp("{", c, len) == 0){
        ++Stack_tip(p->s, -1)->num;
    }else
    if(len == 1 && strncmp("}", c, len) == 0){
        if(--Stack_tip(p->s, -1)->num == 0){
            --Stack_tip(p->s, -1)->num;
        }
    }else
    if(len == 1 && strncmp(";", c, len) == 0){
        if(Stack_tip(p->s, -1)->num == 0){
            --Stack_tip(p->s, -1)->num;
        }
    }
}

static
void Thread_stride_if_skiptoend(Thread* p, const char* c, size_t len, int tw){
                                                                        debug("%s: %s\n", __FUNCTION__, strcut(c, len));
    (void)tw;
    if(len == 1 && strncmp("{", c, len) == 0){
        ++Stack_tip(p->s, -1)->num;
    }else
    if(len == 1 && strncmp("}", c, len) == 0){
        if(--Stack_tip(p->s, -1)->num == 0){
            Stack_pop(p->s);
                                                                        debug("___If: %d\n", __LINE__);
            p->f = Thread_stride;
        }
    }else
    if(len == 1 && strncmp(";", c, len) == 0){
        if(Stack_tip(p->s, -1)->num == 0){
            Stack_pop(p->s);
                                                                        debug("___If: %d\n", __LINE__);
            p->f = Thread_stride;
        }
    }else
    {
        // skip
    }
}

static
void Thread_stride_while_skiptoend(Thread* p, const char* c, size_t len, int tw){
                                                                        debug("%s: %s\n", __FUNCTION__, strcut(c, len));
    (void)tw;
    if(len == 1 && strncmp("{", c, len) == 0){
        ++Stack_tip(p->s, -1)->num;
    }else
    if(len == 1 && strncmp("}", c, len) == 0){
        if(--Stack_tip(p->s, -1)->num == 0){
            Stack_pop(p->s);
                                                                        debug("___While: %d\n", __LINE__);
            p->f = Thread_stride;
        }
    }else
    if(len == 1 && strncmp(";", c, len) == 0){
        if(Stack_tip(p->s, -1)->num == 0){
            Stack_pop(p->s);
                                                                        debug("___While: %d\n", __LINE__);
            p->f = Thread_stride;
        }
    }else
    {
        // skip
    }
}

#ifdef MS_TRY
static
void Thread_stride_try_catched(Thread* p, const char* c, size_t len, int tw){
                                                                        debug("%s: %s\n", __FUNCTION__, strcut(c, len));
    (void)tw;
    if(len == 1 && strncmp("(", c, len) == 0){
        Scope_into(p->o, Stack_tip(p->s, -2));
    }else
    if(len == 1 && strncmp(")", c, len) == 0){
        p->f = Thread_stride;
    }else
    {
        Scope_add(p->o, c, len, Stack_tip(p->s, -1));
    }
}

static
void Thread_stride_try_skiptocatch(Thread* p, const char* c, size_t len, int tw){
                                                                        debug("%s: %s\n", __FUNCTION__, strcut(c, len));
    (void)tw;
    if(Stack_tip(p->s, -2)->num < 0){
        if(len == 5 && strncmp("catch", c, len) == 0){
                                                                        debug("___Try: %d\n", __LINE__);
            p->f = Thread_stride_try_catched;
        }else{
                                                                        debug("___Try: %d\n", __LINE__);
            Var__(Stack_pop(p->s));// throwed
            Var__(Stack_pop(p->s));// catch
            
            (p->f = Thread_stride)(p, c, len, tw);
        }
    }else
    if(len == 1 && strncmp("{", c, len) == 0){
        ++Stack_tip(p->s, -2)->num;
    }else
    if(len == 1 && strncmp("}", c, len) == 0){
        if(--Stack_tip(p->s, -2)->num == 0){
            --Stack_tip(p->s, -2)->num;
        }
    }else
    {
        // skip
    }
}

static
void Thread_stride_try_skiptoend(Thread* p, const char* c, size_t len, int tw){
                                                                        debug("%s: %s\n", __FUNCTION__, strcut(c, len));
    (void)tw;
    if(len == 1 && strncmp("{", c, len) == 0){
        ++Stack_tip(p->s, -1)->num;
    }else
    if(len == 1 && strncmp("}", c, len) == 0){
        if(--Stack_tip(p->s, -1)->num == 0){
            Stack_pop(p->s);// try
                                                                        debug("___Try: %d\n", __LINE__);
            p->f = Thread_stride;
        }
    }else
    {
        // skip
    }
}

static
void Thread_stride_try_succeeded(Thread* p, const char* c, size_t len, int tw){
                                                                        debug("%s: %s\n", __FUNCTION__, strcut(c, len));
    (void)tw;
    if(len == 5 && strncmp("catch", c, len) == 0){
        Stack_tip(p->s, -1)->num = 0;
        p->f = Thread_stride_try_skiptoend;
    }else
    {
        Stack_pop(p->s);// try
                                                                        debug("___Try: %d\n", __LINE__);
        (p->f = Thread_stride)(p, c, len, tw);
    }
}
#endif

static
void Thread_stride_object_new(Thread* p, const char* c, size_t len, int tw){
                                                                        debug("%s: %s\n", __FUNCTION__, strcut(c, len));
    (void)tw;
    Var* pf = Scope_find(p->o, c, len);
                                                                        if(!pf){ p->e.reason=E_NOT_FOUND; return; }
    
    Var* pv = Stack_push(p->s);{
        pv->vt = VTX_This;
        pv->obj = Pool_new(g_pPool);
                                                                        if(!pv->obj){ p->e.reason=E_COULDNT_NEW; return; };
    }
    
    pv = Stack_push(p->s);{
        pv->vt = VT_Refer;
        pv->ref = (pf->vt == VT_Refer) ? pf->ref : pf;
    }
    
    p->f = Thread_stride;
}

static
void Thread_stride_object_property(Thread* p, const char* c, size_t len, int tw){
                                                                        debug("%s: %s\n", __FUNCTION__, strcut(c, len));
    (void)tw;
    Var* pp = Stack_tip(p->s, -1);
    if(pp->vt == VTX_Call && pp->func == Thread_op_property){
        Var* po = Stack_tip(p->s, -2);
        if(po->vt == VT_Refer) po = po->ref;
        
        Var* pr = Object_property(po->obj, c, len);
                                                                        if(!pr){ p->e.reason=E_COULDNT_CREATE; return; }
        
        Var* pv = Stack_push(p->s);{
            pv->vt = VT_Refer;
            pv->ref = pr;
        }
    }else{
        if(len == 1 && strncmp("(", c, len) == 0){
            // set to thiscall
            Var* po = Stack_tip(p->s, -3);
            if(po->vt == VT_Refer) po = po->ref;
            
            Object* o = po->obj;
            
            Thread_stack_call(p, NULL);
            
            Var vf = *Stack_pop(p->s);
            
            Var* pv = Stack_push(p->s);{
                pv->vt = VTX_Call;
                pv->func = Thread_op_thiscall;
            }
            pv = Stack_push(p->s);{
                pv->vt = VTX_This;
                pv->obj = Object_retain(o);
            }
            *Stack_push(p->s) = vf;
        }else{
            Thread_stack_call(p, NULL);
        }
        
        (p->f = Thread_stride)(p, c, len, tw);
    }
}

inline
int Thread_walk(Thread* p){
    static const uint8_t namechar[] = {
        0b00000000,0b00000000, // 0
        0b00000000,0b00000000, // 1
        0b00000000,0b00000000, // 2
        0b11111111,0b11000000, // 3
        0b01111111,0b11111111, // 4
        0b11111111,0b11100001, // 5
        0b01111111,0b11111111, // 6
        0b11111111,0b11100000, // 7
        0b11111111,0b11111111, // 8
        0b11111111,0b11111111, // 9
        0b11111111,0b11111111, // a
        0b11111111,0b11111111, // b
        0b11111111,0b11111111, // c
        0b11111111,0b11111111, // d
        0b11111111,0b11111111, // e
        0b11111111,0b11111111, // f
    };
    
    static const uint8_t twvalue[] = {
        //    0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f
        TW_NONE,TW_NOT ,TW_NONE,TW_NONE,TW_NONE,TW_MOD ,TW_AND ,TW_NONE,TW_PARL,TW_PARR,TW_MULT,TW_PLUS,TW_COMM,TW_MINS,TW_DOT ,TW_DIV , // 2
        TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_SCLN,TW_LT  ,TW_COPY,TW_GT  ,TW_NONE, // 3
        TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE, // 4
        TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_BRTL,TW_NONE,TW_BRTR,TW_XOR ,TW_NONE, // 5
        TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE, // 6
        TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_NONE,TW_BRCL,TW_OR  ,TW_BRCR,TW_CHIL,TW_NONE, // 7
    };
    
    while(*p->c==' ' || *p->c=='\t' || *p->c=='\r' || *p->c=='\n') ++p->c;
    
    if(*p->c == '\0'){
        return 0;
    }
    
    const char* c0 = p->c++;
    
    int tw = TW_NONE;
    if(*c0=='/' && *p->c=='/'){
        while(*p->c!='\n') ++p->c;
        ++p->c;
        return 1;
    }else
    if(*c0=='=' && *p->c=='='){
        ++p->c;
        tw = TW_EQ;
    }else
    if(*c0=='!' && *p->c=='='){
        ++p->c;
        tw = TW_NEQ;
    }else
    if(*c0=='<' && *p->c=='='){
        ++p->c;
        tw = TW_LTEQ;
    }else
    if(*c0=='>' && *p->c=='='){
        ++p->c;
        tw = TW_GTEQ;
    }else
    if(*c0=='>' && *p->c=='>'){
        ++p->c;
        tw = TW_SHRI;
    }else
    if(*c0=='<' && *p->c=='<'){
        ++p->c;
        tw = TW_SHLE;
    }else
    if( (tw = twvalue[*c0-0x20]) ){
        // none
    }else
    if(*c0=='\"'){
        while(*p->c!='\"'){
            if(*p->c=='\\') ++p->c;
            ++p->c;
        }
        ++p->c;
    }else
    {
        while( namechar[*p->c/8] & (0b10000000>>(*p->c%8)) ) ++p->c;
    }
    
    p->f(p, c0, p->c-c0, tw);
    
    if(p->e.reason){
        p->e.code   = c0;
        p->e.len    = p->c - c0;
        return 0;
    }
    
    return 1;
}

Error* Thread_run(Thread* p){
                                                                        debug("%s\n", __FUNCTION__);
    while( Thread_walk(p) );
    
    return p->e.reason ? &p->e : NULL;
}

void Thread_setCodeFunction(Thread* p, Var* pcf, size_t np, Var* ap){
    pcf = (pcf->vt == VT_Refer) ? pcf->ref : pcf;

    Var* pv;
    pv = Stack_push(p->s);{
        pv->vt = VTX_CodeReturn;
        pv->code = ";";
    }

    pv = Stack_push(p->s);{
        pv->vt = VT_Refer;
        pv->ref = pcf;
    }

    pv = Stack_push(p->s);{
        pv->vt = VT_Undefined;
    }

    size_t i = 0;    
    while(i < np){
        *Stack_push(p->s) = ap[i++];
    }
    
    pv = Stack_push(p->s);{
        pv->vt = VTX_Params;
        pv->num = np;
    }
    
    p->c = pcf->code;
    p->f = Thread_stride_function_call;
}

Var* Thread_getLastExp(Thread* p){
    while( Thread_stack_call(p, NULL) );
    
    return Stack_flatten(p->s);
}


