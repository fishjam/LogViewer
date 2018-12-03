# 中文用户请点击 [中文说明](README_CN.md)

# LogViewer
  - This is a general log viewer. It can parse the log files by provide  regular expression configuration.
  - Support several log format, such as spring-boot, Android, iOS, Visual studio, etc.
  -  ***Theoretically, this tool can be used to analyze any log format as long as you write regular expressions for it***.
 
# Function Point
  - 1.After config the regular expression(REGULAR) in ini configuration files,  and set the result map in REGMAP, then can analyze and display the log files.
  - 2.Can filter the log according machine/Process Id/Thread Id, log level, log text etc.
  - 3.Can calculate and get the elapse from the same thread of adjacent logs, so can  identify the performance issue quickly.
  - 4.Can sort according any column .
  - 5.Can open Visual Studio and identify the source code by double click the log item,  if there is source filename and line number in logs.
  
# Log Configuration
  - 1.Sample file [Standard-SpringBoot.ini](x64/Release/Dsh-SpringBoot.ini) , the log file is [spring log demo](demos/dsh-springdemo.log), can get logs from multi servers by [distributed shell](https://github.com/fishjam/dsh) .
  - 2.COMMON 
    - REGULAR :  defines the regular expression for a log. [Regex Match Tracer](http://www.regex-match-tracer.com/) is recommended for writing and validating regular expressions.
    - TIME_FORMAT : define the date and time format, now only support 4 format(ref sample). 
  - 3.REGMAP : define the regular result and log part's correspondence. 
  - 4.LEVELMAP: define the log level's correspondence. 

# TODO
 - [ ] Support pipe source, so can support real-time log(tail -f xxx | adb logcat | idevicelogsys | etc.)

# Main UI
![main](doc/main.png)
