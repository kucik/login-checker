CC     = gcc
OPTS   = -O2 -pipe
DEBUG  = -Wall
LIBS   = -ldl
CFLAGS  = -m32 -pipe -g -O2 -Wall $(DEBUG)

all: recvover.so login_checker

libs.o: recvover.c ro_logger.c ro_comm.c checker_srv.c
	$(CC) $(CFLAGS) -fPIC -o ro_logger.o  -c ro_logger.c
	$(CC) $(CFLAGS) -fPIC -o ro_comm.o  -c ro_comm.c

recvover.o: recvover.c
	$(CC) $(CFLAGS) -fPIC -o recvover.o  -c recvover.c

checker_srv.o: checker_srv.c
	$(CC) $(CFLAGS) -I/usr/include/mysql/ -fPIC -o checker_srv.o  -c checker_srv.c

recvover.so: libs.o recvover.o
	$(CC) $(CFLAGS) -fPIC -shared -o recvover.so recvover.o ro_logger.o ro_comm.o $(LIBS)

login_checker: libs.o checker_srv.o
	$(CC) $(CFLAGS) -o login_checker checker_srv.o ro_logger.o ro_comm.o -L/usr/lib/mysql -L/usr/lib -L/usr/lib/i386-linux-gnu -L/usr/lib/x86_64-linux-gnu $(LIBS) -lmysqlclient 

install: all
	/bin/cp -f recvover.so login_checker /home/nwn/

clean:
	/bin/rm -f *.o *.so login_checker
