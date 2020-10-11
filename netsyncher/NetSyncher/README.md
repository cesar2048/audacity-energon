

## References

### used references

**Programming resources**:

  * [VisualStudio: Remove CRT secure warnings](https://stackoverflow.com/questions/16883037/remove-secure-warnings-crt-secure-no-warnings-from-projects-by-default-in-vis)
  * [VisualStudio: Print to debug console](https://stackoverflow.com/questions/1333527/how-do-i-print-to-the-debug-output-window-in-a-win32-server)
  * [WxWidgets: wxSocketServer reference](https://docs.wxwidgets.org/trunk/classwx_socket_server.html#a043ecb02a352a27ef52389722e1f6f66)
  * [C++: pass argument by reference](https://stackoverflow.com/a/14548993)  
    This answers when to use the aperstand in the argument,  
    as in `void CTeam::ParseTeam(std::istream &in)`
  * [C++: how to check for null using shared_ptr](https://stackoverflow.com/questions/22220512/check-for-null-in-stdshared-ptr)
  * [Websocket interface example](https://github.com/TooTallNate/Java-WebSocket/wiki#server-example)

**System resources**:

  * [Windows APIs: Debugging a slow connection](https://github.com/golang/go/issues/23366#issuecomment-374397983)    
    My tests consistenly had ~500ms delay from postman to my C++ program in windows running locally.  
    Turns out in windows, `localhost` is resolved to `::1` before `127.0.0.1`, and since wxWidgets only supports listening on IPv4,  
    postman tests were internally resolving to `::1` on IPv6, timing out, and then connecting to `127.0.0.1` IPv4  
  * [Windows cmd: netstat to see listening ports](https://helpdeskgeek.com/how-to/use-netstat-to-see-listening-ports-and-pid-in-windows/)

### unused references
  * [Write custom input stream in C++](https://stackoverflow.com/questions/14086417/how-to-write-custom-input-stream-in-c)
  * [github.com - http parser in C](https://github.com/nodejs/http-parser)
  * [C++: iostream tutorial](https://www.cprogramming.com/tutorial/c++-iostreams.html)
  * [C++: custom ostream](https://stackoverflow.com/questions/13703823/a-custom-ostream)
  * [C++: custom input stream](https://stackoverflow.com/questions/14086417/how-to-write-custom-input-stream-in-c)
  * [C++: stringbuf reference](http://www.cplusplus.com/reference/sstream/stringbuf/)
  * [C++: inherit stdostream](https://stackoverflow.com/questions/772355/how-to-inherit-from-stdostream)
	