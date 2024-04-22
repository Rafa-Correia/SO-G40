.PHONY: all clean dir r

#ALL (CREATE DIR, CREATE CLIENT, CREATE SERVER)

all: dir client server
	@echo Done!

#CLIENT PROGRAM COMPILE

client: bin/client
	@echo Compiling client...
	
#SERVER PROGRAM COMPILE

server: bin/orquestrator
	@echo Compiling server...
	
#MAKE ALL NECESSARY DIRECTORIES

dir:
	@echo Creating directories...
	@mkdir -p bin obj
	

#CLEAN ALL

clean:
	@echo Cleaning up...
	@rm -f ./bin/*
	@rm -f ./obj/*
	@rmdir ./bin
	@rmdir ./obj
	@echo Done!

#REMAKES ENTIRE PROJECT (AFTER CLEAN)

r: clean all