# Unix-Shell
A command line interpreter with the ability to execute multiple commands, output redirection, piping, and loops.

**Built-in commands:** "exit", "cd", and "pwd" are the built-in commands support by this shell.

**Loops:** This shell supports the looping of built-in commands, commands with output redirected to files, pipelines, and multiple commands with loops.

**Multiple commands:** Multiple commands can be entered at once when seperated by a semicolon. These commands can consist of pipelined commands or commands with redirected output.

**Pipes:** Piping is supported for any number of pipelined commands greater than 1. Multiple pipelines seperated by a semicolon are also supported. Redirection of output to a file is supported for the last command in the pipeline. As stated above, pipelines can be looped.

**Redirected output to file:** This shell supports the redirection of a process's output to a specified file. The file will be created if it doesn't exist. Files are overwritten rather than appended to upon multiple executions.
