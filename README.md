# SimpleShell
A simple shell written in C


Firstly, we take the command as input and execute it in a new process with execl(). We write a function for this, which is run(). It takes the args array, background flag, which controls whether a process is background or not, and backgroundpid array, that stores the background processes’ pids, into it.

In the run() function, we find the path of the command that we will execute. Then we create a child process for executing our command with execl(). If there is a background process then we don’t wait for it to finish, but we write it’s pid into backgroundpid array.


Second of all, we create some built-in functions.
For the “alias” function, we remove the quote marks from the command line. We take the commands that we will set to an alias, which are between the quote marks, and store them in an array. Then we write the commands and their aliases into a linked list by append() function. If one of the alias is called then we get the real command of it by getRealCmds() function and then call the run function with return value of it.

If “alias -l” has been entered then we use display() method for showing our alias list.
For the “unalias” function, we delete the node which has the entered name from the linked list. We write a delete function for that.

For the “clr” function we clear the terminal. We use system function in that step.

For the “fg” function we take the pids of background processes from the backgroundpid array and we call the waitpid() function for all of them.

For the “exit” function, we checked if there is a background process by looking in the backgroundpid array. If there is no pid in that array we exit our shell program, if there is then we say that we cannot terminate because of them and do nothing.


For the last step, we make a function for IO-redirection. We take a flag for checking if there is a redirection. If there is, we fork a new child process and call our redirection function in it.

In the main function we control which redirection operation has been entered by our flag. In the redirection function, we take that flag and according to its value we call the appropriate open() function. We use dup2() function for redirecting the input or output.
 
