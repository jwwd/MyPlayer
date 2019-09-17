#include<stdio.h>
#include <stdlib.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <unistd.h>
#include<errno.h>
#include<ctype.h>
#include <wait.h>
#include <string.h>
#include <sys/types.h>
#include<dirent.h>
#include <signal.h>


typedef struct Words{

  struct Words *before; 
  int T;
  char info[100];
  struct Words *next;
}GC;

char *getname(char buff[50]);
int readfile(char path[100],GC *head);
int gettime(char buff[50],int *allTime);

int createList(char (*list)[50]);
void showList(char (*list)[50],int count,char *name);
int playSong(char *songNm,int pipefd);

int main(){

	char list[10][50]={};
	int count=createList(list);
	int nowSongId=0;

	int pipefd[2];
	if(pipe(pipefd)<0){
		perror("pipe");
    	return -1;
	}

	unlink ("./mymp.fifo"); 
	if(mkfifo("mymp.fifo",0666)<0){
		perror("mkfifo");	  //若文件已存在，返回-1
		return 0;
	}

	pid_t pid=fork();

	if(pid<0){
		perror("fork");
		return -1;
	}else if(pid>0){		//father
		close(pipefd[0]);
		pid_t pids2=fork();
		if(pids2<0){
			perror("fork2");
			return -1;
		}else if(pids2>0){			//father
			//close(pipefd[1]);
			int wrfd;		
			if((wrfd=open("mymp.fifo",O_WRONLY))<0){
	   			perror("open wrfd");
       	 		return -1;     
			}

			while(1){
				char command[20]={};
				gets(command);
				if(strcmp(command,"exit")==0){
					kill(pid,SIGKILL);
					kill(pids2,SIGKILL);
					wait(NULL);
					exit(1);
				}if(strcmp(command,"n")==0||strcmp(command,"l")==0){
					kill(pids2,SIGKILL);
					wait(NULL);
					if(strcmp(command,"n")==0){
						nowSongId++;
						if(nowSongId>=count) nowSongId=0;
					}else{
			            nowSongId--;
			            if(nowSongId<0) nowSongId=count-1;
			          }
					//printf("%s\n",list[nowSongId] );
					pids2=playSong(list[nowSongId],pipefd[1]);
					//exit(0);
					}
				else{
					//strcpy(command,"pause\n");
					//printf("cmd:%s\n",command);
					strcat(command,"\n");
					if(write(wrfd,command,strlen(command))<0){
						perror("write fifo");
       					return 0;
   	 				}   	 				   	 				
				}
			}						
		}else{						//son2
			char path[100]="../res/";
			strcat(path,list[0]);
			 if(dup2(pipefd[1],1)<0){
   				perror("fd1 dup2");
   				exit(-1);
			 }							
			execlp("mplayer","mplayer","-slave" ,"-idle","-input","file=./mymp.fifo",path,NULL);
		}
				
		
	}else{							//son1
		close(pipefd[1]);
/////////////////////////////////////////////////////////////
		  GC *first=(GC *)malloc(sizeof(GC));
		  GC *new=NULL;
		  GC *last=NULL;
		  //new=first;
		  //readfile(path,first);
		  first->before=NULL;
		  first->next=NULL;
		int nowtime;
		int allTime;

		char list[20][50]={0};
  		int count=createList(list);
  		int flag=0;
  		
  		char mp3_name[100];
		char lrc_path[100]="../lrc/";
		char p_name[100]={0};
		char *p;
		char info1[100]={};
		char info2[100]={};
//printf("ddddd\n");
				
		///////////////////////////////////////////////////////		
		while(1)
		{	
   		 	char buff[100]={};
			read(pipefd[0],buff,100);
			//printf("%s\n",buff);

			if(strstr(buff,"Playing"))
			{								
				p= getname(buff);
				strcpy(p_name,p);
				memset(lrc_path,100,0);
				strcpy(lrc_path,"../lrc/");
				strcat(lrc_path,p_name);
				strcat(lrc_path,".lrc");
				//printf("%s\n", p);
				strcpy(mp3_name,p);
				strcat(mp3_name,".mp3");

				new=first;
				if(new!=NULL)
				{	
				  last=new;
		          while(last!=NULL)//释放上一首歌词内存
		          {
		            last=last->next;
		            free(new);
		            new=last;
		          } 
		          first=(GC *)malloc(sizeof(GC));
		          first->before=NULL;
		  		  first->next=NULL;
		          new=first;

				}
		  		flag= readfile(lrc_path,first);
			}		

			if(buff[0]=='A'&&buff[1]==':')
				//printf("read:%s\n",buff);///////////////////////
			{
				//printf("read:%s\n",buff);
				//
				
				nowtime=gettime(buff,&allTime);
				//printf("%d\n",nowtime );
					 
				if(flag==1)
				{
					if((nowtime<(new->T))&&(new->before!=NULL))
				  	{
			  			last=new;
			  			new=new->before;
				  	}
				    if(nowtime>=(new->T)&&(new->next!=NULL))
				    {
			  			last=new;
			  			new=new->next;
			  			if(nowtime<(new->T))
			  			{
			  				printf("%s","\033[1H\033[2J");//刷新单行
						    showList(list,count,mp3_name);
						 
						    printf("\n正在播放：%s \n总时长：%d S 当前进度：%d S\n",p_name,allTime,nowtime); 
						    printf("\n\033[34m\t\t%s\033[0m",last->info );//字体背景wu，字是蓝色
						    printf("\n\033[37m\t\t\t%s\033[0m",last->next->info );//，	
						    strcpy(info1,last->info);
						    strcpy(info2,new->info);
						}
				    }
				    else
				    {
				    	printf("%s","\033[1H\033[2J");//刷新单行
					    showList(list,count,mp3_name);
					 
					    printf("\n正在播放：%s \n总时长：%d S 当前进度：%d S\n",p_name,allTime,nowtime); 
					    printf("\n\033[34m\t\t%s\033[0m",info1 );//字体背景wu，字是蓝色
					    printf("\n\033[37m\t\t\t%s\033[0m",info2 );//，
				    }
				}else
				{
					printf("%s","\033[1H\033[2J");//刷新单行
				    showList(list,count,mp3_name);
				    printf("\n正在播放：%s \n总时长：%d S 当前进度：%d S\n",p_name,allTime,nowtime);
				    printf("未搜索到歌词！\n");
				}

			  	

			    
			
			}

		}
	}	

	return 0;	
}


int readfile(char path[100],GC *head)
{
  FILE *fp;
  //GC *head=NULL;
  GC *new=NULL;
  GC *last=NULL;
  new=head;//(GC *)malloc(sizeof(GC));;
  
  int flag=0;
  char tmp[110];
  char *strp;
  char *token;
  char *tmin;
  char *tsec;
  fp=fopen(path,"r");
  if(fp==NULL){
  	// perror("fopen");
  	// exit(0);
  	return 0;
  }

  char a[100][110];
  int i=0;

  while(!feof(fp))
  {
      fgets(tmp,110,fp);
       token=strtok_r(tmp,"[]",&strp);
      
      
      if(strcmp(token,"offset:0")==0)
      {
        flag=1;
        continue;
      }
      //if(flag==1&&strcmp(strp,""))
      if(flag==1&&strcmp(strp,"\n"))
      {
        strcpy(new->info,strp);

        tmin=strtok_r(token,":",&strp);
        tsec=strtok(strp,".");
       
        new->T=atoi(tmin)*60+atoi(tsec);
      	last=new;
        new->next=(GC *)malloc(sizeof(GC));
        new=new->next;
        new->before=last;

        new->next=NULL;
       
      }
  }
  return 1;
}


int createList(char (*list)[50]){	
	DIR *dp=opendir("../res/");
	struct dirent *doneinfo;
	int count=0;

	while(doneinfo=readdir(dp)){
		if(strcmp(doneinfo->d_name,".")&&strcmp(doneinfo->d_name,"..")&&strstr(doneinfo->d_name,".mp3")){
			strcpy(list[count++],doneinfo->d_name);
			//printf("%d:%s",i++,doneinfo->d_name);
			//if((i-1)==num) printf("<-");
			//printf("\n");
		} 	 	
	}
	closedir(dp);
	return count;
}

void showList(char (*list)[50],int count,char *name){
  int i=0;
  for(i=0;i<count;i++){
    printf("%d:%s",i+1,list[i] );
    if(strcmp(list[i],name)==0){
      printf("<-");
    }
    printf("\n");
  }
  return;
}

// void showList(char (*list)[50],int count){
// 	int i=0;
// 	for(i=0;i<count;i++){
// 		printf("%d:%s\n",i+1,list[i] );
// 	}
// 	return;
// }

int playSong(char *songNm,int pipefd){
	pid_t pid=fork();
	char path[100]="../res/";
	strcat(path,songNm);
	if(pid<0){
		perror("fork son2");
	}else if(pid>0){
		return pid;
	}else{
		if(dup2(pipefd,1)<0){
   			perror("fd1 dup2");
   			exit(-1);
		}		
		execlp("mplayer","mplayer","-slave" ,"-idle","-input","file=./mymp.fifo",path,NULL);
	}	

}

///////////////////////////////////////////////////////////////////////////////////////////////

char *getname(char buff[100])
{
  char *token;
  char *strp1,*strp2; 
  
  token=strtok_r(buff," .",&strp1);
  
  token=strtok_r(strp1,"s",&strp2);
  token=strtok_r(strp2,"/",&strp1);
  
  return strtok(token,".");
  
}

int gettime(char buff[100],int *allTime)
{
	
	char *token;
	char *strp1,*strp2;
	int flag=0;
	int nowtime;
	int lengthtime;
	
	token=strtok_r(buff,"  ",&strp1);
	token=strtok_r(strp1,"(",&strp2);
	nowtime=atoi(token);
	//printf("time=%d\n",nowtime);
	if(flag==0)
	{
		token=strtok_r(strp2,"f",&strp1);
		token=strtok_r(strp1,"(",&strp2);
		*allTime=lengthtime=atoi(token);
	//	printf("lengthtime=%d\n",lengthtime);
		flag=1;
	}

	return nowtime;
}
