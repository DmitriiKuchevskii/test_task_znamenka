# Requirements
Should work on any POSIX system with decent C++17 compiler  
Tested on:
 - Ubuntu 18.04.2 LTS (GNU/Linux 4.15.0-1044-aws x86_64) / g++ (Ubuntu 8.3.0-6ubuntu1) 8.3.0
 - Ubuntu 19.04 (GNU/Linux 5.0.0-25-generic x86_64) / g++ (Ubuntu 8.3.0-6ubuntu1) 8.3.0
 - Ubuntu 19.04 (GNU/Linux 5.0.0-25-generic x86_64) / clang++ 8.0.0-3 (tags/RELEASE_800/final)

# Build
The following command will build three executable: **server**, **client** and **test**
> sh build.sh

# Run
You can iether run server and clients manually or just use **test** to check whether or not eveyrhitng works fine.

 - If you would like to run automation tests just execute (default number of clients is 10):
> ./test <number_of_clients>

It will give you a small report about the results.
 - If you would like to run everything manually you need to follow the following steps:
> ./server <file_name_for_broadcasting>

&nbsp;&nbsp;&nbsp;&nbsp;Wait untill "Wating for clients..." appears signaling that server has been succesfully initialized.  
&nbsp;&nbsp;&nbsp;&nbsp;Connect whatever number of clients you'd like by executing.  
> ./client <optional_result_file_name> <optional_start>  

&nbsp;&nbsp;&nbsp;&nbsp;Note: Your last client must be run with **--start** option to start broadcasting.

# Example of report on my laptop

Test with 10 clients  
Broadcasting 100 MB  
Message size is 1 MB  

> Client's(PID: 12275) 90th percentile statistic:  
> Latecy_1 (FROM Server started read chunk of data TO Client finished write chunk of data): 857 microsec  
> Latecy_2 (FROM Server finished write into SHM TO Client started read from SHM): 58 microsec  
>  
> Client's(PID: 12273) 90th percentile statistic:  
> Latecy_1 (FROM Server started read chunk of data TO Client finished write chunk of data): 941 microsec  
> Latecy_2 (FROM Server finished write into SHM TO Client started read from SHM): 58 microsec  
>  
> Client's(PID: 12270) 90th percentile statistic:  
> Latecy_1 (FROM Server started read chunk of data TO Client finished write chunk of data): 831 microsec  
> Latecy_2 (FROM Server finished write into SHM TO Client started read from SHM): 47 microsec  
>  
> Client's(PID: 12278) 90th percentile statistic:  
> Latecy_1 (FROM Server started read chunk of data TO Client finished write chunk of data): 914 microsec  
> Latecy_2 (FROM Server finished write into SHM TO Client started read from SHM): 58 microsec  
>  
> Client's(PID: 12269) 90th percentile statistic:  
> Latecy_1 (FROM Server started read chunk of data TO Client finished write chunk of data): 941 microsec  
> Latecy_2 (FROM Server finished write into SHM TO Client started read from SHM): 59 microsec  
>  
> Client's(PID: 12277) 90th percentile statistic:  
> Latecy_1 (FROM Server started read chunk of data TO Client finished write chunk of data): 791 microsec  
> Latecy_2 (FROM Server finished write into SHM TO Client started read from SHM): 47 microsec  
>  
> Client's(PID: 12276) 90th percentile statistic:
> Latecy_1 (FROM Server started read chunk of data TO Client finished write chunk of data): 904 microsec  
> Latecy_2 (FROM Server finished write into SHM TO Client started read from SHM): 67 microsec  
>  
> Client's(PID: 12272) 90th percentile statistic:  
> Latecy_1 (FROM Server started read chunk of data TO Client finished write chunk of data): 881 microsec  
> Latecy_2 (FROM Server finished write into SHM TO Client started read from SHM): 67 microsec  
>  
> Client's(PID: 12274) 90th percentile statistic:  
> Latecy_1 (FROM Server started read chunk of data TO Client finished write chunk of data): 917 microsec  
> Latecy_2 (FROM Server finished write into SHM TO Client started read from SHM): 58 microsec  
>  
> Client's(PID: 12271) 90th percentile statistic:  
> Latecy_1 (FROM Server started read chunk of data TO Client finished write chunk of data): 945 microsec  
> Latecy_2 (FROM Server finished write into SHM TO Client started read from SHM): 58 microsec    
