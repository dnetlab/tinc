To modify the dynamic library path:
vi src/Makefile
Change "CFLAGS = -g -O2 -Wall"  To  "CFLAGS = -g -O2 -Wall -Wl,-rpath=./lib"  
"./lib" represents the "lib" directory in the program's working directory

为适配其他机器不同版本openssl的环境，将动态库和软件一起打包，并指定程序运行时加载的动态库
的路径为当前目录下的lib目录，可通过修改src/Makefile来实现：
	将 "CFLAGS = -g -O2 -Wall"  改为  "CFLAGS = -g -O2 -Wall -Wl,-rpath=./lib"  
