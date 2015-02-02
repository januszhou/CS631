###CS631 sish —a simple shell

####The following options are supported by sish:
`−c command` Execute the given command.
`−x` Enable tracing: Write each command to standard error, preceeded by ’+’.

####Shell able to Handler

1. Control operators:
```c
&   BACKGROUND  
|   PIPE
```

2. Redirection operators:
```c
<   INPUT_REDIRECTION
>>  OUTPUT_APP
>   OUTPUT_REDIRECTION
```

####Builtins

sish supports the following builtins (which are taking precedence over any non-builtin commands):
1. cd [dir] Change the current working directory. If dir is not specified, change to the user’s home
directory.
2. echo [word] Print the given word, followed by a ’\n’. The following special values are supported:
$? The exit status of the last command.
$$ The current process ID.
3. exit Exit the current shell. or Ctrl + c

