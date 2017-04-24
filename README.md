# MisakaServer

## Brief 
 - Tiny Server frame

## DEPEND
 - libev
 - zlog     
 - lua-dev
 - python-dev

## Build 
 ```
    autoconf
    ./configure 
    make 
    
    ./app

 ```

## File
 - tool             tool code 
 - scripts          scripts
 - misaka           projct code
    - include       header files
    - lib           serve frame
    - link          link plugin
    - parse         protocol plugin
    - task          app code(c)
    - task-lua      app code(lua)
    - task-python   app code(python)
    - app           app Entry

# Support
 - thread pool
 - task distribute
 - tcp, udp 
 - link support register api
 - protocol support register api
 - dynamic load

## Notie
 - In rebuild

# Author 
    - name: hongwei mei
    - email:xiyanxiyan10@hotmail.com
    - weixin: xiyanxiyan10
