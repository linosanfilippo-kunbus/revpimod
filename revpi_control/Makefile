
CC?=gcc



revpi_control_test: revpi_control_test.o revpi_control_lib.o
	$(CC) -o revpi_control_test revpi_control_test.o revpi_control_lib.o

revpi_control_test.o: revpi_control_test.c revpi_control_lib.h
	$(CC) -c revpi_control_test.c -Wall

revpi_control_lib.o: revpi_control_lib.c revpi_control_lib.h ../revpi_user.h ../../revpid/revpid.h
	$(CC) -c revpi_control_lib.c -Wall -I../../revpid -I.


clean:
	rm revpi_control_test *.o
	 
