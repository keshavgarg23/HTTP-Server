#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>

#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include<fcntl.h>
#include<sys/sendfile.h>
#include<sys/stat.h>


#define err(x,val) if(x<0){printf("%s Error!\n",val);exit(0);}

char txt_header[]="HTTP/1.1 200 OK\nContent-Type:text/html\n\n";
char jpg_header[]="HTTP/1.1 200 OK\nContent-Type:image/jpg\n\n";
char png_header[]="HTTP/1.1 200 OK\nContent-Type:image/png\n\n";

int port = -1;
char static_url[1024] ="\0";
char html_path[1024] = "\0";
char temp_path[1024]="\0";

void get_configs(){
    FILE* cf = fopen("./config.txt","r");
    char buf[1024];
    while(fgets(buf,sizeof(buf),cf)){
        if(buf[0]=='#'){    //comment
            continue;
        }
        if(port==-1){
            port = atoi(buf);
        }else if(static_url[0]=='\0'){
            if(strlen(buf)>=1&&buf[0]!='\n'){
                strcpy(static_url,buf);
                if(static_url[strlen(static_url)-1]=='\n')
                    static_url[strlen(static_url)-1]='\0';
                if(static_url[strlen(static_url)-1]=='/'){
                    static_url[strlen(static_url)-1]='\0';
                }
            }
        }else if(html_path[0]=='\0'){
            if(strlen(buf)>=1&&buf[0]!='\n'){
                strcpy(html_path,buf);
                if(html_path[strlen(html_path)-1]=='\n')
                    html_path[strlen(html_path)-1]='\0';
                if(html_path[strlen(html_path)-1]=='/'){
                    html_path[strlen(html_path)-1]='\0';
                }
            }
        }else{
            if(strlen(buf)>=1&&buf[0]!='\n'){
                strcpy(temp_path,buf);
                if(temp_path[strlen(temp_path)-1]=='\n')
                    temp_path[strlen(temp_path)-1]='\0';
                if(temp_path[strlen(temp_path)-1]=='/'){
                    temp_path[strlen(temp_path)-1]='\0';
                }
                break;
            }
        }
    }
    fclose(cf);
}

int tokenizer(char* str,char* tokenized[],char delim,int get_last){
    char token[1024];
    int idx=0;
    int words=0;
    int n = strlen(str);
    for(int i = 0;i<n;i++){
        if(str[i]=='\n'&&delim!='\n')break;
        if(str[i]!=delim){
            token[idx++]=str[i];
        }else{
            token[idx++]='\0';
            char* word=(char*)malloc(strlen(token));

            strcpy(word,token);
            tokenized[get_last?0:words++]=word;
            // if(get_last==1)words--;
            if(words==3){
                break;
            }
            idx=0;
        }
    }
    if(words==3)return words;
    char* word=(char*)malloc(strlen(token));
    token[idx++]='\0';
    if(idx>1){
        strcpy(word,token);
        tokenized[get_last?0:words++]=word;
        idx=0;
    }
    
    return words;
}

void substr(char str[],int n){
    int idx=0;
    for(int i=n;i<=strlen(str);i++){
        str[idx++]=str[i];
    }
}

void add_details(char* message){
    char buf[1024];
    memset(buf,0,sizeof(buf));
    char* words[1];
    tokenizer(message,words,'\n',1);
    char* vals[3];
    tokenizer(words[0],vals,'&',0);
    substr(vals[0],5);
    substr(vals[1],6);
    substr(vals[2],3);
    
    FILE* fp = fopen("./student_db.txt","a");
    fprintf(fp,"%s,%s,%s\n",vals[0],vals[1],vals[2]);
    fclose(fp);
    
}

void get_message_name(char* message,char name[]){
    int idx=0;
    int i=0;
    if(!strncmp(message,"POST",4)){
        //add details
        add_details(message);
    }
    while(message[i]!='/'){
        i++;
    }
    while(message[i]!='\n'){
        if(message[i]==' ')break;
        name[idx]=message[i];
        i++;
        idx++;
    }
    name[idx]='\0';
    if(!strcmp(name,"/")){
        strcat(name,"index.html\0");
    }
}

size_t get_size(char* location){
    struct stat st;
    stat(location,&st);
    return st.st_size;
}

void send_page(char* path,int sock){
    char location[1024];
    memset(location,0,1024);
    strncpy(location,html_path,strlen(html_path));
    strncat(location,path,strlen(path));

    int resp = open("resp",O_WRONLY|O_TRUNC|O_CREAT);
    err(resp,"response write");
    write(resp,txt_header,strlen(txt_header));

    int page_desc = open(location,O_RDONLY);
    
    err(page_desc,"Page Not Found");

    sendfile(resp,page_desc,NULL,get_size(location));

    close(page_desc);
    close(resp);
    resp = open("resp",O_RDONLY);
    err(resp,"response read");
    sendfile(sock,resp,NULL,get_size("./resp"));

    close(resp);
}

int get_name_type(char* name){
    char type[10];
    int idx=0;
    memset(type,0,10);
    for(int i = strlen(name)-1;i>=0;i--){
        type[idx++]=name[i];
        if(name[i]=='.')break;
        if(idx==10){
            return -1;
        }
    }
    if(!strncmp(type,"pmet.",5))return -1;
    if(!strncmp(type,"lmth.",5))return 0;
    if(!strncmp(type,"gpj.",4))return 1;
    else if(!strncmp(type,"gnp.",4)||!strncmp(type,"oci.",4))return 2;

    return -2;
}

void send_image(char* path,int sock){
    char location[1024];
    strcpy(location,static_url);
    strcat(location,path);
    int imdesc = open(location,O_RDONLY);
    err(imdesc,"Image Not Found");

    int img_type = get_name_type(path);
    err(img_type,"Unknown Image Format");

    int imresp = open("resp",O_WRONLY|O_TRUNC|O_CREAT);   //generating image response
    err(imresp,"resp write");
    write(imresp,img_type==1?jpg_header:png_header,strlen(img_type==1?jpg_header:png_header));

    sendfile(imresp,imdesc,NULL,get_size(location));  //write content of image to imresp

    close(imdesc);
    close(imresp);
    imresp = open("resp",O_RDONLY);
    err(imresp,"resp read");
    sendfile(sock,imresp,NULL,get_size(location)+512);

    close(imresp);
}


void send_template(char* name,int sock){

    char location[1024];
    memset(location,0,sizeof(location));
    strcpy(location,temp_path);
    strcat(location,name);


    int resp = open("resp",O_WRONLY|O_TRUNC|O_CREAT);
    write(resp,txt_header,strlen(txt_header));
    write(resp,"<!DOCTYPE html>\n<html>\n<body>\n<h1>Students Details</h1>\n<hr>\n<br>\n<a href=\"add_student.html\">Add Student</a>\n<br>\n<hr>\n",119);



    FILE* student_db = fopen("student_db.txt","r");
    if(!student_db){
        printf("student_db.txt not found!\n");
        exit(1);
    }
    char buf[1024];
    char* words[3];
    memset(buf,0,sizeof(buf));
    while(fgets(buf,sizeof(buf),student_db)){
        if(buf[0]=='\n')continue;
        tokenizer(buf,words,',',0);

        memset(buf,0,sizeof(buf));

        int i = 0;
        char form_buf[1024];
        memset(form_buf,0,sizeof(form_buf));
        FILE* temp = fopen(location,"r");
        write(resp,"\n<div>\n",7);

        while(fgets(buf,sizeof(buf),temp)){
            sprintf(form_buf,buf,words[i]);
            write(resp,form_buf,strlen(form_buf));

            memset(buf,0,sizeof(buf));
            if(strlen(words[i])>0)
                free(words[i++]);
            else
                i++;
        }
        fclose(temp);
        
        write(resp,"\n</div>\n<hr>\n<br>\n",18);

    }
    write(resp,"\n</body>\n</html>\n",17);
    fclose(student_db);
    close(resp);
    resp = open("resp",O_RDONLY);
    sendfile(sock,resp,NULL,get_size("./resp"));
    close(resp);
    
}

int main(){
    get_configs(); //get configration from config 
    
    printf("port:%d\nStatic:%s\nhtml:%s\nTemplates:%s\n\n\n",port,static_url,html_path,temp_path);

    int sock;
    struct sockaddr_in serv,cli;
    socklen_t len;
    char buf[1024];

    sock = socket(AF_INET,SOCK_STREAM,0);
    err(sock,"Socket creation");

    serv.sin_family=AF_INET;
    serv.sin_addr.s_addr=htonl(INADDR_ANY);
    serv.sin_port = htons(port);

    err(bind(sock,(struct sockaddr*)&serv,sizeof(serv)),"bind");

    err(listen(sock,10),"listen");
    printf("Server started at port No: %d\nGoto 127.0.0.1:%d in web Browser\n\n",port,port);
    while(1){
        int comm = accept(sock,(struct sockaddr*)&cli,&len);
        err(comm,"accept");
        
        
        memset(buf,0,sizeof(buf));
        recv(comm,buf,sizeof(buf),0);
        printf("%s\n",buf);
        char name[1024];
        get_message_name(buf,name);
        int req_type = get_name_type(name);

        if(req_type==-2){
            send(comm,txt_header,sizeof(txt_header),0);
            close(comm);
            continue;
        }
        if(req_type==-1){   //template  (just for student db render)
            send_template(name,comm);
        }else if(req_type==0){  //page
            send_page(name,comm);
        }else{                  //image
            send_image(name,comm);
        }

        close(comm);

    }
    
    close(sock);
    return 0;
}