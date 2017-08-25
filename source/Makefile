myshell: myshell.o utility.o process.o string.o variable.o control.o io.o
	gcc myshell.o utility.o process.o string.o variable.o control.o io.o -o myshell
myshell.o:  myshell.h myshell.c
	gcc myshell.h  myshell.c -c
utility.o:  myshell.h utility.c
	gcc myshell.h utility.c -c
process.o:  myshell.h process.c
	gcc myshell.h process.c -c
string.o: myshell.h string.c
	gcc myshell.h string.c -c
variable.o: myshell.h variable.c
	gcc myshell.h variable.c -c
control.o: myshell.h control.c
	gcc myshell.h control.c -c
io.o: myshell.h io.c
	gcc myshell.h io.c -c
clear:
	rm -f *.o
