# nvidia-monitor-eventing

NVIDIA Device Monitoring and Eventing

## Build
### Environment Setup

OS | Build Tool Package | Code Support Package
--- | --- | ---
Ubuntu 18.04 | g++<br>autoconf<br>autoconf-archive<br>pkg-config<br>libtool-bin<br>doxygen<br>graphviz | OpenBMC SDK
Cygwin | g++<br>autoconf<br>autoconf-archive<br>pkg-config<br>libtool-bin<br>doxygen<br>graphviz | OpenBMC SDK
|

OpenBMC SDK Installation Instructions: [link](https://github.com/openbmc/docs/blob/master/development/dev-environment.md#download-and-install-sdk)

Instead of setting up those manually, run following script inside *source code folder* will help,
``` shell
$ sudo scripts/setup_bldenv
```
>**NOTE: Cygwin packages need to be installed manually with [Cygwin setup utility](https://www.cygwin.com/setup-x86_64.exe)!**

### How to build
The general build steps are as below, inside *source code folder* and run,
```
$ meson builddir # configure
$ ninja -C builddir # build
$ ninja -C builddir install # Optional
```


[Project Options](#tablebuildmode) are defined as below, any combinations of them are valid,
<a id="tablebuildmode"></a>


 ### How to Clean
To clean build cache,
``` shell
$ ninja -C builddir clean
$ ninja -C builddir uninstall #optional
```

To completely clean the workspace,
``` shell
$ rm -rf builddir
```

 ### Enable Project Options
 These commands would work for clean projects. If already configured, refer [section](#enable-project-options-for-configured-projects)


 #### Debug Log Level
 ``` shell
 $ meson builddir -Ddebug_log={0,1,2,3,4}
 $ ninja -C builddir
 $ ninja -C builddir install # Optional
 ```
 ### Enable Project Options for Configured Projects
 If the project is already configured using,
 ``` shell
 $ meson builddir
 ```
 , user can use `meson configure` to view/edit project options.


### Debug Logging Level Control
Debug Log Level supports to be configured on both build time and runtime.

There are 5 logging levels supported,
<a id="tabledbgloglevel"></a>

Level | Index
--- | ---
disabled | 0
error | 1 (default)
warning | 2
debug | 3
info | 4
|

By default, the logging level is 1 - error. To change the default log level before building, e.g. to 3 - debug, in openbmc repo, modify the recipe by changing the following line,
``` markdown
EXTRA_OEMESON += "-Ddebug_log=1"
```

To change the logging level during runtime, e.g. to 3 - debug, we need to pass following command line argument.
``` markdown
-l <level> 0 - None; 1 - +Error; 2 - +Warning; 3 - +Info
```

To change the target for log output during runtime, e.g. to /tmp/mon_evt_debug.log, we need to pass following command line argument.
``` markdown
-L <log_file> where to output the log. Output to screen if the arg not present.
```

### Docker Image for Building & Debugging
:warning: **Not maintained!** May need extensive work to work as described.

A docker image is used for code building and debugging. It needs to be created for the first time by,
``` shell
$ ./buildenv create
```
It creates an image named "bldenv:module" with all build support packages for this module.

After that, everytime to build code just start it by (*current folder* will be mounted into the container),
``` shell
$ ./buildenv
```
Or,
``` shell
$ ./buildenv start [src_dir] # src_dir is where you want to mount into the container, default is current folder.
```
It will start a container based on the image for code building by following the same steps listed [above](#how-to-build).
In this container, user could do '**ninja -C builddir install**' to install the code packages just like in real production environment, without messing up the OS.
By default, the container will keep it last states after exit. Do this to get a clean one,
``` shell
$ ./buildenv clean
$ ./buildenv
```
Or,
``` shell
$ ./buildenv clean
$ ./buildenv start [src_dir] # src_dir is where you want to mount into the container, default is current folder.
```