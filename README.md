# HTTP Server
The HTTP server is created using the concept of socket programming in C language. The server reads the port Number, static image directory and template directory from configs.txt which is present in the same directory as server. 


## Features
- The server can serve html pages along with embedded jpg or png images to any device in local network.
- Server also supports the hyperlinks.
- Any change in html code just require page refresh in browser, no need to restart server.
- Server also identifies and handles the POST requests (illustrated using add_student form).
- It works only on POSIX operating system.

**NOTE**: Templates are just hybrid of HTML and C that is used to render the details of the students.
