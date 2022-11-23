# Multithreaded-Task-Scheduler
A task scheduler check-in system using threads. The scheduler includes two queues and four clerks to serve customers whos information is given in a text file provided at runtime. The two queues represent economy class and buisness class, where clerks will give service priority to any customer in in the buisness class line over the economy class line. Included in the provided text file is the total amount of customers and a unique customer ID, class type, service time, and arrival time for each customer.

After extracting files, in terminal use 

make ACS

to compile and

./ACS customer.txt

where customer.txt is the customer file to be evaluated
