# Build
The following command will build three executable: **server**, **client** and **test**
> sh build.sh

# Run
You can iether run server and clients manually or just use **test** to check whether or not eveyrhitng works fine.

 - If you would like to run automation tests just execute:
> ./test

It will give you a small report about the results.
 - If you would like to run everything manually you need to follow the following steps:
> ./server <file_name_for_broadcasting>

&nbsp;&nbsp;&nbsp;&nbsp;Wait untill "Wating for clients..." appears signaling that server has been succesfully initialized.  
&nbsp;&nbsp;&nbsp;&nbsp;Connect whatever number of clients you'd like by executing.  
> ./client <optional_result_file_name> <optional_start>  

&nbsp;&nbsp;&nbsp;&nbsp;Note: Your last client must be run with **--start** option to start broadcasting.
