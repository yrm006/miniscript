#include <stdio.h>
#include <string.h>
#include "miniscript.config.h"
#include "miniscript.h"



// Array
static
void Array__(Array* p){
                                                                        // printf("%s\n", __FUNCTION__);
    int j = 0;
    while(j < ((Object*)p)->vars[0].var.num){
        Var__(&p->avars[j++]);
    }

    size_t i = 0;
    while( (i < ((Object*)p)->size) && ((Object*)p)->vars[i].name[0] ){
        Var__(&((Object*)p)->vars[i++].var);
    }
}

static
Array* Array_(Array* p, size_t len){
                                                                        // printf("%s: %lu\n", __FUNCTION__, sizeof(*p));
    ((Object*)p)->dctr = (void (*)(Object*))Array__;
    ((Object*)p)->size = 1;// for 0:length
    ((Object*)p)->c = 1;
    
    strncpy(((Object*)p)->vars[0].name, "length", 6)[6] = '\0';
    ((Object*)p)->vars[0].var.vt  = VT_Number;
    ((Object*)p)->vars[0].var.num = len;
    
    size_t i = 0;
    while(i < len){
        Var_(&p->avars[i++]);
    }
    
    return p;
}



// Pool for Array Allocation
ArrayPool* ArrayPool_(ArrayPool* p, size_t size){
                                                                        // printf("%s: %lu\n", __FUNCTION__, sizeof(*p));
    p->size = size;
    
    size_t i = 0;
    while(i < p->size){
        Object_release( (Object*)Array_((Array*)&p->arrs[i], 0) );
        ++i;
    }
    
    return p;
}

void ArrayPool__(ArrayPool* p){
                                                                        // printf("%s\n", __FUNCTION__);
    size_t i = 0;
    while(i < p->size){
        if(Object_retain((Object*)&p->arrs[i])){
                                                                        printf("!!!ArrLeak [%zu: %hu]\n", i, ((Object*)&p->arrs[i])->c);//###DBG
            while( Object_release((Object*)&p->arrs[i]) );
        }
        ++i;
    }
}

static ArrayPool* g_pArrayPool = NULL;

ArrayPool* ArrayPool_global(ArrayPool* p){
    return (g_pArrayPool = p);
}

static
Array* ArrayPool_new(ArrayPool* p, size_t len){
    size_t i = 0;
    while( (i < p->size) && Object_retain((Object*)&p->arrs[i]) ) Object_release((Object*)&p->arrs[i++]);
                                                                        // printf("%s: %zu\n", __FUNCTION__, i);
    
    return (i < p->size && len <= MS_SIZE_POOLEDARR) ? Array_((Array*)&p->arrs[i], len) : NULL;
}



void mslib_Array(Thread* p){
    int i = -1;
    while(!( Stack_tip(p->s, i)->vt == (VarType)VTX_Call )) --i;

    size_t len = MS_SIZE_POOLEDARR;
    if(i == -3){
        Var* pl = Stack_tip(p->s, -1);
        len = (pl->vt == VT_Refer) ? pl->ref->num : pl->num;
    }
    
    Var* pv = Stack_tip(p->s, i-1);
    Object_release(pv->obj);
    pv->obj = (Object*)ArrayPool_new(g_pArrayPool, len);
    
    while(!( Var__(Stack_pop(p->s)) == VTX_Call ));
    Stack_push(p->s);//void
}


