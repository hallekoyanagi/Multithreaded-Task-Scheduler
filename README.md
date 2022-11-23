# Multithreaded-Task-Scheduler
## Description:
Written in C. A task scheduler check-in system using threads. The scheduler includes two queues and four clerks to serve customers whos information is given in a text file provided at runtime. The two queues represent economy class and buisness class, where clerks will give service priority to any customer in in the buisness class line over the economy class line. Included in the provided text file is the total amount of customers and a unique customer ID, class type, service time, and arrival time for each customer. Average wait times are printed once all customers are served. Clerks and customers operate in parallel to eachother, and therefore are each represented by threads.

## To Run:

After extracting files, in terminal use 

make ACS

to compile and

./ACS customer.txt

where customer.txt is the customer file to be evaluated

## Known Issues:
Priority levels are rated out of 5, 5 being the highest priority

1. All clerks do not usually run. Often only two to three our of four will run in unison. It seems the clerk threads get held up waiting for signals from customers after sending a signal that was never recieved. **Priority = 5**
2. Due to issue 1, the program never terminates as some clerks never terminate. Therefore, the final customer to be served will print averages at the end of the thread and terminate the program. This is only a temporary fix for issue 1. **Priority = 2**
