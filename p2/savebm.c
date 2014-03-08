#include <stdio.h>
#include <memory.h>
#include <assert.h>

char *make_bmp_head(char *p, long w, long h){
#define PUT( T, X ) *(T*)p = (X); p+=sizeof(T);
    *p++ = 'B';
    *p++ = 'M';
    PUT(long,0x3E + w*h/8);
    PUT(short,0);
    PUT(short,0);
    PUT(long,0x3E);

    PUT(long,0x28);
    PUT(long,w);
    PUT(long,h);
    PUT(short,1);
    PUT(short,1);
    PUT(long,0);
    PUT(long,w*h/8);
    memset(p,0,0x14);
    p+=0x14;
    PUT(long,0xFFFFFF);
    return p;
}
char buf[1048576];
int main(){
    FILE *f = fopen("testbmp.bin","wb");
    char *r = make_bmp_head(buf,256,127);
    assert(r-buf==0x3E);
    fwrite(buf,1,0x3E + 256*127/8,f);
    fclose(f);
    return 0;
}
