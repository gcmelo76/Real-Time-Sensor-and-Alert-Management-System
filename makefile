all:	home_iot sensor user_console
home_iot:	system_manager.o mainFunctions.o otherFunctions.o
	gcc -Wall -Wextra -pthread system_manager.o mainFunctions.o otherFunctions.o -o home_iot
sensor:	sensor.o commonFunctions.o
	gcc -Wall -Wextra sensor.o commonFunctions.o -o sensor
user_console: user_console.o commonFunctions.o
	gcc -Wall -Wextra -pthread user_console.o commonFunctions.o -o user_console
system_manager.o:	system_manager.c system_manager.h mainFunctions.h otherFunctions.h
	gcc -Wall -Wextra system_manager.c -c
mainFunctions.o: mainFunctions.c otherFunctions.h system_manager.h
	gcc -Wall -Wextra mainFunctions.c -c
otherFunctions.o: otherFunctions.c system_manager.h mainFunctions.h
	gcc -Wall -Wextra otherFunctions.c -c
sensor.o:	sensor.c commonFunctions.h
	gcc -Wall -Wextra sensor.c -c
user_console.o:	user_console.c commonFunctions.h
	gcc -Wall -Wextra user_console.c -c
commonFunctions.o: commonFunctions.c
	gcc -Wall -Wextra commonFunctions.c -c	
