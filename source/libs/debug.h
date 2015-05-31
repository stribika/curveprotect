#ifndef _DEBUG_H____
#define _DEBUG_H____

extern void debug_exec(int e, const char *message1, const char *message2, char **s);

extern void debug_9(int,const char *,const char *,const char *,const char *,const char *,const char *,const char *,const char *,const char *);

#define debug_8(x,a,b,c,d,e,f,g,h) debug_9(x,a,b,c,d,e,f,g,h,0)
#define debug_7(x,a,b,c,d,e,f,g) debug_8(x,a,b,c,d,e,f,g,0)
#define debug_6(x,a,b,c,d,e,f) debug_7(x,a,b,c,d,e,f,0)
#define debug_5(x,a,b,c,d,e) debug_6(x,a,b,c,d,e,0)
#define debug_4(x,a,b,c,d) debug_5(x,a,b,c,d,0)
#define debug_3(x,a,b,c) debug_4(x,a,b,c,0)
#define debug_2(x,a,b) debug_3(x,a,b,0)
#define debug_1(x,a) debug_2(x,a,0)
#define debug_0(x) debug_1(x,0)

#endif
