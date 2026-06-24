#
# To compile, type "make" or make "all"
# To remove files, type "make clean"
#

# Object files for the server and client, including queue.o from queue.c
OBJS = server.o request.o segel.o client.o Queue.o

TARGET = server

CC = gcc
CFLAGS = -g -Wall

LIBS = -lpthread

# List of suffixes to handle
.SUFFIXES: .c .o

# Default target to build the server, client, and output.cgi, and copy necessary files
all: server client output.cgi
	-mkdir -p public
	-cp output.cgi favicon.ico home.html public

# Rule to build the server executable
server: server.o request.o segel.o Queue.o
	$(CC) $(CFLAGS) -o server server.o request.o segel.o Queue.o $(LIBS)

# Rule to build the client executable
client: client.o segel.o Queue.o
	$(CC) $(CFLAGS) -o client client.o segel.o Queue.o

# Rule to build the output CGI
output.cgi: output.c
	$(CC) $(CFLAGS) -o output.cgi output.c

# Generic rule for compiling .c files to .o object files
.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

# Rule to clean up all generated files
clean:
	-rm -f $(OBJS) server client output.cgi
	-rm -rf public
