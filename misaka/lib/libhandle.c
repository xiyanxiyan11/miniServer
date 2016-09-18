#include "stdio.h"
#include "stdlib.h"
#include "dlfcn.h"  
 
int main(void)
{
         void *handle;
         int (*fcn)(int x, int y);
         const char *errmsg;
        
         /* open the library */   //打开库
         handle = dlopen("libsthc.so", RTLD_NOW);
         if(handle == NULL)
         {
                   fprintf(stderr, "Failed to load libsthc.so: %s\n", dlerror());
                   return 1;
         }
         dlerror();
 
         //*(void **)(&fcn) = dlsym(handle, "add");            //ok
         fcn = dlsym(handle, "add");                                   //ok
         if((errmsg = dlerror()) != NULL)
         {
                   printf("%s\n", errmsg);
                  return 1;
         }
         printf("%d\n", fcn(1, 5));
        
         dlclose(handle);
         return 0;
}



















