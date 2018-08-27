// MINIScript

#include <unistd.h>
#include <stdint.h>

// you have to define these symbols
// MS_SIZE_POOLEDOBJ : object member max
// MS_SIZE_POOLEDARR : array length max
// MS_SIZE_VARNAME   : var name length max
// MS_SIZE_PROPNAME  : property name length max



extern const char E_STACK_OVERFLOW[];
extern const char E_SCOPE_OVERFLOW[];
extern const char E_TOO_LONG_NAME[];
extern const char E_COULDNT_NEW[];
extern const char E_COULDNT_CREATE[];
extern const char E_NOT_FOUND[];



typedef enum{
    VT_Undefined = 0,
    VT_Refer,
    VT_Number,
    VT_CodeString,
    VT_Function,
    VT_CodeFunction,
    VT_Object,
    VT_Char,
    
    VTX_Frame = 0x20,
    VTX_Call,
    VTX_If,
    VTX_While,
    VTX_Try,
    VTX_Catch,
    VTX_Throw,
    VTX_CodeReturn,
    VTX_Params,
    VTX_Return,
    VTX_This,
} VarType;

typedef struct Var{
    // 1byte unused0;
    // 1byte unused1;
    // 1byte unused2;
    uint8_t vt;
    union{
        void (*func)();
        int32_t num;
        const char* code;
        struct Var* ref;
        struct Object* obj;
    };
} Var;  // __attribute__((__packed__))
Var* Var_(Var* p);
VarType Var__(Var* p);

typedef struct Error{
    const char* code;
    size_t      len;
    const char* reason;
} Error;

typedef struct Stack{
    size_t size;
    size_t n;
    Error* e;
    Var vars[0];
} Stack;
extern Var Stack_OVERFLOW;
Stack* Stack_(Stack* p, size_t size);
void Stack__(Stack* p);
Var* Stack_push(Stack* p);
Var* Stack_pop(Stack* p);
Var* Stack_tip(Stack* p, int n);
void Stack_ground(Stack* p);
Var* Stack_flatten(Stack* p);

typedef struct VarMap{
    Var* pv;
    char name[MS_SIZE_VARNAME];
} VarMap;
typedef struct Scope{
    size_t size;
    size_t n;
    struct Scope* s;
    Error* e;
    VarMap nvars[0];
} Scope;
Scope* Scope_(Scope* p, size_t size);
void Scope__(Scope* p);
Var* Scope_add(Scope* p, const char* name, size_t len, Var* pv);

typedef struct NamedVar{
    Var var;
    char name[MS_SIZE_PROPNAME];
} NamedVar;
typedef struct Object{
    void (*dctr)(struct Object*);
    uint16_t size;
    uint16_t c;
    NamedVar vars[0];
} Object;
Object* Object_retain(Object* p);
size_t Object_release(Object* p);

typedef struct ObjectForPool{
    Object base;
    NamedVar vars[MS_SIZE_POOLEDOBJ];
} ObjectForPool;
typedef struct Pool{
    size_t size;
    ObjectForPool objs[0];
} Pool;
Pool* Pool_(Pool* p, size_t size);
void Pool__(Pool* p);
Pool* Pool_global(Pool* p);

typedef struct Thread{
    const char*   c;
    void        (*l)(struct Thread*);
    Stack*        s;
    Scope*        o;
    void        (*f)(struct Thread*, const char*, size_t, int);
    Error         e;
} Thread;
Thread* Thread_(Thread* p, const char* c, Stack* s, Scope* o);
void Thread__(Thread* p);
int Thread_walk(Thread* p);
Error* Thread_run(Thread* p);
void Thread_setCodeFunction(Thread* p, Var* pcf, size_t np, Var* ap);
Var* Thread_getLastExp(Thread* p);



// library
typedef struct Array{
    Object base;
    NamedVar vars[1];// for 0:length
    Var avars[0];
} Array;

typedef struct ArrayForPool{
    Array base;
    Var avars[MS_SIZE_POOLEDARR];
} ArrayForPool;
typedef struct ArrayPool{
    size_t size;
    ArrayForPool arrs[0];
} ArrayPool;
ArrayPool* ArrayPool_(ArrayPool* p, size_t size);
void ArrayPool__(ArrayPool* p);
ArrayPool* ArrayPool_global(ArrayPool* p);

void mslib_Array(Thread* p);


