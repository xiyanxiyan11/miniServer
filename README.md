# embeddedServer

## Brief 
 - Tiny Server frame

# Feature
 - thread pools
 - task distribute
 - tcp, udp ...
 - register api for more link to support
 - register api for more protocol to support
 - dynamic load


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
    - task-lua      app code(lua, in build)
    - task-python   app code(python, in build)
    - app           app Entry

## Notie
 - In rebuild

# Author 
    - name: hongwei mei
    - email:xiyanxiyan10@hotmail.com
    - weixin: xiyanxiyan10
