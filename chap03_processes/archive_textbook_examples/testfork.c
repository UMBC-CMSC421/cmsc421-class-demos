#include <stdio.h>


int main(int argc, char **argv)
{
	
	int pid;
	int pipefid[2];
	pipe(pipefid);


	pid = fork();
	if(pid == -1)
	{
		perror("FORK FAILED: ");
	}
	else if(pid == 0)
	{
		//child end write into the pipe
		//close(pipefid[0]);
		//char *str = "Hello Parent";
		//write(pipefid[1], str, 13);	
		//printf("Hello Child\n");
		//dup2(pipefid[1], 1);
		//printf("Hello Child\n");
		// /bin/ls
		char *arg[] = {"/bin/ls", "-l", (char *)0};
		char *env[] = {(char*)0};
		execve("/bin/ls",arg, env);
	
	}
	else
	{
		//close(pipefid[1]);
		//char buf[100];
		//dup2(pipefid[0], 0);
		//scanf("%s",buf);
		//read(pipefid[0],buf, 13);
		//printf("%s", buf);
		int status;
		wait(&status);
	}
}
