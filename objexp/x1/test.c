
#include <dos.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <hw/dos/exeload.h>
#include <hw/dos/execlsg.h>

struct exeload_ctx      final_exe = exeload_ctx_INIT;

int main() {
    if (!exeload_load(&final_exe,"final.exe")) {
        fprintf(stderr,"Load failed\n");
        return 1;
    }
    if (!exeload_clsg_validate_header(&final_exe)) {
        fprintf(stderr,"EXE is not valid CLSG\n");
        exeload_free(&final_exe);
        return 1;
    }

    fprintf(stderr,"EXE image loaded to %04x:0000 residentlen=%lu\n",final_exe.base_seg,(unsigned long)final_exe.len_seg << 4UL);
    {
        unsigned int i,m=exeload_clsg_function_count(&final_exe);

        fprintf(stderr,"%u functions:\n",m);
        for (i=0;i < m;i++)
            fprintf(stderr,"  [%u]: %04x (%Fp)\n",i,exeload_clsg_function_offset(&final_exe,i),exeload_clsg_function_ptr(&final_exe,i));
    }

    /* let's call some! */
    {
        /* index 0:
           const char far * CLSG_EXPORT_PROC get_message(void); */
        const char far * (CLSG_EXPORT_PROC *get_message)(void) = exeload_clsg_function_ptr(&final_exe,0);
        const char far *msg;

        fprintf(stderr,"Calling entry 0 (get_message) now.\n");
        msg = get_message();
        fprintf(stderr,"Result: %Fp = %Fs\n",msg,msg);
    }

    {
        /* index 1:
           unsigned int CLSG_EXPORT_PROC callmemaybe(void); */
        unsigned int (CLSG_EXPORT_PROC *callmemaybe)(void) = exeload_clsg_function_ptr(&final_exe,1);
        unsigned int val;

        fprintf(stderr,"Calling entry 1 (callmemaybe) now.\n");
        val = callmemaybe();
        fprintf(stderr,"Result: %04x\n",val);
    }

    exeload_free(&final_exe);
    return 0;
}

