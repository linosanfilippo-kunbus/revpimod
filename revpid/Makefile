
CC?=gcc



revpid: revpid.o ../revpi_control/revpi_control_lib.o
	$(CC) -o revpid revpid.o ../revpi_control/revpi_control_lib.o -lpthread

revpid.o: revpid.c revpid.h ../revpi_control/revpi_control_lib.h
	$(CC) -c revpid.c -Wall -I../revpi_control/


clean:
	rm revpid *.o
	 
