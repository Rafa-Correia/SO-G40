#EMPTY MAKEFILE
.PHONY: all clean dir r

all: dir client server
	@echo Done!

client:
	@echo Compiling client...
	

server:
	@echo Compiling server...
	

dir:
	@echo Creating directories...
	@mkdir -p bin obj
	

clean:
	@echo Cleaning up...
	@rm -f ./bin/*
	@rm -f ./obj/*
	@rmdir ./bin
	@rmdir ./obj
	@echo Done!

r: clean all