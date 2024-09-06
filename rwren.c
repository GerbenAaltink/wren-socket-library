
#include <rlib.h>
#include "net.h"
#ifndef wren_h
#include "wren.h"
#endif

bool printEnabled = true;

void errorHandler(WrenVM* vm, WrenErrorType type, const char* module, int line,
    const char* message){
switch (type)
    {
        case WREN_ERROR_COMPILE:
            printf("[Compile Error] %s:%d: %s\n", module, line, message);
            break;

        case WREN_ERROR_RUNTIME:
            printf("[Runtime Error] %s\n", message);
            break;

        case WREN_ERROR_STACK_TRACE:
            printf("[Stack Trace] %s:%d: %s\n", module, line, message);
            break;
    }

    }



void wrenWrite(WrenVM * vm, const char * content){
    if(printEnabled)
    printf("%s", content);
}


void wrenExecString(WrenVM * vm,const char *str)
{
  WrenInterpretResult result = wrenInterpret(vm, "main", str);
  switch (result)
  {
  case WREN_RESULT_COMPILE_ERROR:
  {
    printf("Compile Error!\n");
    printf(str);

  //wrenDebugPrintStackTrace(vm);
  
  printf("\n");;
    exit(2);
  }
  break;
  case WREN_RESULT_RUNTIME_ERROR:
  {
    printf("Runtime Error!\n");
    printf(str);
    
  //wrenDebugPrintStackTrace(vm);
  printf("\n");
    exit(2);
  }
  break;
  case WREN_RESULT_SUCCESS:
  {
  }
  break;
  }
      wrenEnsureSlots(vm,100);
 
}

void wrenExecFile(WrenVM * vm, char *path)
{
  size_t file_length = rfile_size(path);
  char file_content[file_length + 1];
  rfile_readb(path, file_content, file_length);
  file_content[file_length] = 0;
  wrenExecString(vm, file_content);
}

const char * generateArgumentArray(int argc, char ** argv){
    char arrayString[4096] = {0};
    strcat(arrayString,"var Argv = [");
    for(int i = 0; i < argc; i++){
        char argument[1034] = {0};
        sprintf(argument,"\"%s\"",argv[i]);
        if(i != argc - 1){
            strcat(argument,",");
        }
        strcat(arrayString,argument);
    }
    strcat(arrayString,"]");
    static char escaped[4096];
    escaped[0] = 0;
    rstraddslashes(arrayString,escaped);
    strcat(escaped,"\n");
    return escaped;
}
void wrenExecMerge(WrenVM * vm, const char * str, char * path){
    size_t file_size = rfile_size(path);
    char file_content[file_size + 1];
    rfile_readb(path,file_content,file_size);
    file_content[file_size ] = 0;
    char combined[file_size + strlen(str) + 2];
    combined[0] = 0;
    strcat(combined,str);
    strcat(combined,file_content);
    wrenExecString(vm,combined);
}

int main(int argc, char *argv[]) {
   // setbuf(stdout,NULL);
    WrenConfiguration config;
    wrenInitConfiguration(&config);
    config.bindForeignMethodFn = bindNetModuleClass;
    config.loadModuleFn = loadNetModule;
    config.writeFn = wrenWrite;
    config.errorFn = errorHandler;
    WrenVM * vm = wrenNewVM(&config);
 
    if(argc == 1){
        printf("Usage: rwren <script.wren>\n");
        return 0;
    }
    char* sourcePath = argv[1];
    if(!rfile_exists(sourcePath)){
        printf("File \"%s\" does not exist.\n",sourcePath);
        return 0;
    }
    //if(!strcmp(sourcePath,"lib.wren")){
        printEnabled = true;
    //}
   // wrenExecString(vm,"import \"net\"");
    //wrenExecString(vm, createClassString());
    wrenExecString(vm,generateArgumentArray(argc,argv));
    wrenExecFile(vm,sourcePath);
    //wrenExecMerge(vm,  generateArgumentArray(argc,argv),sourcePath);

    return 0;

}
