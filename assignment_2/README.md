# assignment_2

Assignment 2
EECS 3221E
November 12, 2019

Daniel Kozlovsky
Jada Jones
Ahaz Goraya
 

 
A modification of alarm_mutex.c that uses two extra parallel
threads to manage odd and even requests, thus dividing the work. 
Alarms are managed on an expiration basis.

To compile:

 -A makefile is included so just run "make" in the directory
 
 -Otherwise can use the command "cc -o assignment_2.out My_Alarm.c -D_POSIX_PTHREAD_SEMANTICS -lpthread -I."
 
To run: 

	-run "assignment_2.out"
	
To use:

At any time, it is possible to request a new alarm by entering the number of seconds desired followed by the message as a string. 

Ctrl+d to exit the program
