#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include<stdlib.h>
#include<dirent.h>
#include<time.h>
#include<string.h>
#include<pwd.h>
#include <grp.h>
#include<stack>
#include<vector>
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include<string>
#include<algorithm>
using namespace std;



char root[4096]; 
char current_dir[4096]; 
vector<string> directory_list; 
stack<string> backward_stack; 
stack<string> forward_stack; 
unsigned int cx; 
unsigned int cy; 
unsigned int terminal_row_no; 
unsigned int terminal_col_no; 
int current_window_size; 
struct winsize terminal; 
struct termios initSetting, newSetting; 
vector<string> command_vector;





void move_cursor(){
	printf("%c[%d;%dH", 27, cx, cy);
}

void move_cursor_top()
{
    cx = 1;
    current_window_size = 0;
    move_cursor();
}

string get_absolute_path(string path)
{
    string abs_path = "";
    if (path[0] == '~') {
        path = path.substr(1, path.length());
        abs_path = string(root) + path;
    }
    else if(path[0]=='.' && path[1]=='/'){
    	path = path.substr(1, path.length());
        abs_path = string(current_dir) + path;
    }
    else if(path[0]=='.'){
    	abs_path = string(current_dir);
    }
    else {
        abs_path = string(current_dir) + "/" + path;
    }
    return abs_path;
}

bool isDirectory(string path)
{
    struct stat sb;
    if (stat(path.c_str(), &sb) != 0) {
        //perror(path.c_str());
        return false;
    }
    if (S_ISDIR(sb.st_mode))
        return true;
    else
        return false;
}

void display(const char* dirName)
{
    cy = 0;
    struct stat sb;
    string abs_path = get_absolute_path(dirName);
    stat(abs_path.c_str(), &sb);
    printf((sb.st_mode & S_IRUSR) ? "r" : "-");
    printf((sb.st_mode & S_IWUSR) ? "w" : "-");
    printf((sb.st_mode & S_IXUSR) ? "x" : "-");
    printf((sb.st_mode & S_IRGRP) ? "r" : "-");
    printf((sb.st_mode & S_IWGRP) ? "w" : "-");
    printf((sb.st_mode & S_IXGRP) ? "x" : "-");
    printf((sb.st_mode & S_IROTH) ? "r" : "-");
    printf((sb.st_mode & S_IWOTH) ? "w" : "-");
    printf((sb.st_mode & S_IXOTH) ? "x" : "-");
    cy += 10;

    struct passwd* get_username;
    get_username = getpwuid(sb.st_uid);
    string uname = get_username->pw_name;
    cy += printf(" %10s ", uname.c_str());

    struct group* get_grpname;
    get_grpname = getgrgid(sb.st_gid);
    string gname = get_grpname->gr_name;
    cy += printf(" %10s ", gname.c_str());

    long long size=sb.st_size;
    char unit='B';
    if(size>=1024){
        size/=1024;
        unit='K';
    }
    if(size>=1024){
        size/=1024;
        unit='M';
    }if(size>=1024){
        size/=1024;
        unit='G';
    }    
    cy +=  printf("%4lld%c \t", size, unit); 
    
    string m_time = string(ctime(&sb.st_mtime));
    m_time = m_time.substr(4, 12);
    cy += printf(" %-12s ", m_time.c_str());

    printf(" %-20s\n", dirName);
    cy++;
}

void update_list()
{
    write(STDOUT_FILENO, "\x1b[2J", 4);
    cy = 1;
    move_cursor();

    int bound;
    if (directory_list.size() <= terminal_row_no)
        bound = directory_list.size() - 1;
    else
        bound = terminal_row_no + current_window_size;
        
    for (int i = current_window_size; i <= bound; i++) {
        display(directory_list[i].c_str());
    }
    return;
}

void listdir(const char* path)
{
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal);
    terminal_row_no = terminal.ws_row - 2;
    terminal_col_no = terminal.ws_col;
    write(STDOUT_FILENO, "\x1b[2J", 4);
    struct dirent* d;
    DIR* dp;
    dp = opendir(path);
    
    directory_list.clear();
    while ((d = readdir(dp))) {
        if (strcmp(path, root) == 0) {
            strcpy(current_dir, root);
            if (strcmp(d->d_name, "..") == 0)
                continue;
        }
        directory_list.push_back(d->d_name);
    }
    sort(directory_list.begin(), directory_list.end());
    update_list();
    cx = 1;
    move_cursor();
    closedir(dp);
    return;
}

void ScrollUp()
{
    if (cx > 1) {
        cx--;
        move_cursor();
    }
    else if (cx == 1 && cx + current_window_size > 1) {
        current_window_size--;
        update_list();
    }
}

void ScrollDown()
{
    if (cx <= terminal_row_no && cx < directory_list.size()) {
        cx++;
        move_cursor();
    }
    else if (cx > terminal_row_no && cx + current_window_size < directory_list.size()) {
        current_window_size++;
        update_list();
    } 
}

void GoRight()
{
    move_cursor_top();
    if (!forward_stack.empty()) {
        string path = forward_stack.top();
        forward_stack.pop();
        strcpy(current_dir, path.c_str());
        backward_stack.push(current_dir);
        listdir(current_dir);
    }
}

void GoLeft()
{
    move_cursor_top();
    if (backward_stack.size()>1) {
        string path = backward_stack.top();
        backward_stack.pop();
        forward_stack.push(path);
        path = backward_stack.top();
        strcpy(current_dir, path.c_str());
        listdir(current_dir);
    }
    else if(backward_stack.size()==1){
    	string path = backward_stack.top();
    	strcpy(current_dir, path.c_str());
        listdir(current_dir);
    }
}

void GoHome()
{
    strcpy(current_dir,root);
    while (!forward_stack.empty())
        forward_stack.pop();
    listdir(current_dir);
}

void GoUp()
{
    move_cursor_top();
    if ((strcmp(current_dir, root)) != 0) {
    	int pos = (string(current_dir)).find_last_of("/\\");
        string s_name = (string(current_dir)).substr(0, pos);
        strcpy(current_dir, s_name.c_str());
        backward_stack.push(current_dir);
        while (!forward_stack.empty())
            forward_stack.pop();
        listdir(current_dir);
    }
}

void EnterKey()
{
    if (directory_list[current_window_size + cx - 1] == ".") {
        move_cursor_top();
    }
    else if (directory_list[current_window_size + cx - 1] == "..") {
    	int pos = (string(current_dir)).find_last_of("/\\");
        string s_name = (string(current_dir)).substr(0, pos);
        strcpy(current_dir, s_name.c_str());
        backward_stack.push(current_dir);
        while (!forward_stack.empty())
            forward_stack.pop();
        move_cursor_top();
        listdir(current_dir);
    }
    else {
        char* p_path;
        char* f_path;
        
            string cur_d = "/" + directory_list[current_window_size + cx - 1];
            move_cursor();
            p_path = current_dir;
            f_path = new char[cur_d.length() + strlen(p_path) + 1];
            strcpy(f_path, p_path);
            strcat(f_path, cur_d.c_str());
            
        while (!forward_stack.empty())
            forward_stack.pop();
        
        if (isDirectory(f_path)) {
        	strcpy(current_dir, f_path);
                backward_stack.push(current_dir);
        	move_cursor_top();
        	listdir(current_dir);
        }
        else {
            pid_t pid = fork();
            if (pid == 0) {
                close(2);
                execlp("xdg-open", "xdg-open", f_path, NULL);
                exit(0);
            }
        }
    }
}



void CanonMode(){
	cx = directory_list.size() + 2;
	cy = 1;
	move_cursor();
        printf("\x1b[0K");
        printf(":");
        cout<<endl;
        fflush(0);
        cy++;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &initSetting);
	return;
}

void NonCanonMode(){
	tcgetattr(STDIN_FILENO,&initSetting);
	newSetting=initSetting;
	newSetting.c_lflag &= ~ICANON;
	newSetting.c_lflag &= ~ECHO;
	tcsetattr(STDIN_FILENO,TCSANOW,&newSetting);
	return;
}

void file_delete(string file){
	unlink(file.c_str());
	return;
}

void file_create(string filename, string dest){
	string file = dest+"/"+filename;
	open(file.c_str(),O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);
	return;
}
void dir_create(string dirname, string dest){
	string dir = dest+"/"+dirname;
	mkdir(dir.c_str(),S_IRUSR|S_IWUSR|S_IXUSR);
	return;
}

void dir_delete(string dir){
	DIR *dp;
	struct dirent *entry;
	
	if((dp = opendir(dir.c_str()))==NULL){
		fprintf(stderr, "Can't open the directory: %s\n",dir.c_str());
		return;
	}

	while((entry = readdir(dp))!=NULL){
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
       		continue;
        	else {
            		string path = dir + "/" + string(entry->d_name);
            		if (isDirectory(path)) {
                		dir_delete(path);
            		}
            		else {
                		unlink(path.c_str());
            		}
        	}
        }
    	remove(dir.c_str());
    	closedir(dp);
}
void File_copy(string filename, string dest){
	if(isDirectory(dest)){
			char block[1024];
			int infile,outfile;
			int nread;
			infile = open(filename.c_str(),O_RDONLY);
			outfile = open((dest+'/'+filename).c_str(),O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);
			while((nread = read(infile,block,sizeof(block)))>0){
				write(outfile,block,nread);
			}
		}
}
void Dir_copy(string source, string dest){
	if(isDirectory(source)){
		if(isDirectory(dest)){
			DIR *dp;
			struct dirent *entry;
			struct stat statbuf;
			dp = opendir(source.c_str());
			chdir(source.c_str());
    			while((entry = readdir(dp))!=NULL){
				lstat(entry->d_name,&statbuf);
				if(S_ISDIR(statbuf.st_mode)){
					if(strcmp(".",entry->d_name)==0 || strcmp("..",entry->d_name)==0){
						continue;
					}
					mkdir((dest+'/'+entry->d_name).c_str(),S_IRUSR|S_IWUSR|S_IXUSR);
					Dir_copy(entry->d_name,dest+'/'+entry->d_name);
				}
				else{
					File_copy(entry->d_name,dest);
				}
			}
			chdir("..");
			closedir(dp);
		}
	}
}

void gotodir(string path){
	chdir(path.c_str());
}

/*bool search(string str1, string dest){
	bool flag = false;
	if(isDirectory(str1)){
		if(isDirectory(dest)){
			DIR *dp;
			struct dirent *entry;
			struct stat statbuf;
			dp = opendir(dest.c_str());
			chdir(dest.c_str());
    			while((entry = readdir(dp))!=NULL){
				lstat(entry->d_name,&statbuf);
				if(S_ISDIR(statbuf.st_mode)){
					if(strcmp(".",entry->d_name)==0 || strcmp("..",entry->d_name)==0){
						continue;
					}
					if((dest+'/'+entry->d_name).c_str() == str1){
						flag = true;
						break;
					}
					flag = search(str1,dest+'/'+entry->d_name);
					if(flag)
						break;
				}
			}
			chdir("..");
			closedir(dp);
		}
	}
	else{
		if(isDirectory(dest)){
			DIR *dp;
			struct dirent *entry;
			struct stat statbuf;
			dp = opendir(dest.c_str());
			chdir(dest.c_str());
    			while((entry = readdir(dp))!=NULL){
				lstat(entry->d_name,&statbuf);
				if(S_ISDIR(statbuf.st_mode)){
					if(strcmp(".",entry->d_name)==0 || strcmp("..",entry->d_name)==0){
						continue;
					}
					flag = search(str1,dest+'/'+entry->d_name);
					if(flag)
						break;
				}
				 
				else{
					if(entry->d_name == str1){
						flag = true;
						break;
					}
				}
			}
			chdir("..");
			closedir(dp);
		}
	}
	return flag;
}*/
bool search(string str1, string dest){
	bool flag = false;
	if(isDirectory(dest)){
			//cout<<"inside if "<<endl;
			DIR *dp;
			struct dirent *entry;
			struct stat statbuf;
			dp = opendir(dest.c_str());
			chdir(dest.c_str());
    			while((entry = readdir(dp))!=NULL){
				lstat(entry->d_name,&statbuf);
				if(S_ISDIR(statbuf.st_mode)){
					if(strcmp(".",entry->d_name)==0 || strcmp("..",entry->d_name)==0){
						continue;
					}
					//cout<<" "<<str1<<" ";
					if(entry->d_name == str1){
						flag = true;
						break;
					}
					flag = search(str1,dest+'/'+entry->d_name);
					if(flag)
						break;
				}
				else {
					if(entry->d_name == str1){
						flag = true;
						break;
					}
				}
			}
			chdir("..");
			closedir(dp);
		}
	return flag;
}

void move_File_Dir(string str1, string dest){
	if(isDirectory(str1)){
		if(isDirectory(dest)){
			string temp;
			for(int i=str1.length()-1;i>0;i--){
				if(str1[i]=='/') break;
					temp += str1[i];	
				}		
			reverse(temp.begin(),temp.end());
			dir_create(temp,dest);
			dest += '/' + temp;
			Dir_copy(str1,dest);
			dir_delete(str1);
		}
	}
	else{
		
		if(isDirectory(dest)){
			File_copy(str1,dest);
			unlink((current_dir+'/'+str1).c_str());
		}
	}
	return;
}

void command_mode()
{
        CanonMode();
        gotodir(current_dir);
        string str;
	while(1){
	command_vector.clear();
	getline(cin,str);
	for(int i=0;i<str.size();i++){
		if(str[i]==' ') continue;
		string temp;
		while(i<str.size() && str[i] != ' '){
			temp += str[i];i++;
		}
		command_vector.push_back(temp);
	}		
	
	if(command_vector[0]=="copy_file"){
		string dest = get_absolute_path(command_vector[2]);
		File_copy(command_vector[1],dest);
	}
	else if(command_vector[0]=="copy_dir"){
		string source = get_absolute_path(command_vector[1]),dest = get_absolute_path(command_vector[2]);
		dir_create(command_vector[1].c_str(),dest);
		dest += '/' + command_vector[1];
		Dir_copy(source,dest);
	}
	else if(command_vector[0]=="move"){
		//string source = get_absolute_path(command_vector[1]);
		string dest = get_absolute_path(command_vector[2]);
		move_File_Dir(command_vector[1],dest);
	}
	else if(command_vector[0]=="delete_file"){
		string filepath = get_absolute_path(command_vector[1]);
		file_delete(filepath);
	}
	else if(command_vector[0]=="delete_dir"){
		string filepath = get_absolute_path(command_vector[1]);
		dir_delete(filepath);
	}
	else if(command_vector[0]=="goto"){
		string path = get_absolute_path(command_vector[1]);
		strcpy(current_dir,path.c_str());
		gotodir(current_dir);		
	}
	else if(command_vector[0]=="rename"){
		string oldstr = command_vector[1];
		string newstr = command_vector[2];
		rename(oldstr.c_str(),newstr.c_str());
	}
	else if(command_vector[0]=="create_file"){
		string dest = get_absolute_path(command_vector[2]);
		file_create(command_vector[1],dest);
	}
	else if(command_vector[0]=="create_dir"){
		string dest = get_absolute_path(command_vector[2]);
		dir_create(command_vector[1],dest);
	}
	else if(command_vector[0]=="search"){
		bool f = search(command_vector[1],current_dir);
		if(f)
			cout<<"True"<<endl;
		else
			cout<<"False"<<endl;
	}
	if(command_vector[0]=="exit")
		break;
	cout<<endl;	
	}
    return;
}

void Normar_Mode()
{
	NonCanonMode();
        char ch[3] = {0};
        while (1) {         
            move_cursor();
            fflush(0);
            if (read(STDIN_FILENO, ch, 3) == 0)
                continue;
            else if (ch[0] == 27 && ch[1] == '[' && ch[2] == 'A')
                ScrollUp();
            else if (ch[0] == 27 && ch[1] == '[' && ch[2] == 'B')
                ScrollDown();
            else if (ch[0] == 27 && ch[1] == '[' && ch[2] == 'C')
                GoRight();
            else if (ch[0] == 27 && ch[1] == '[' && ch[2] == 'D')
                GoLeft();
            else if (ch[0] == 'H' || ch[0] == 'h')
                GoHome();
            else if (ch[0] == 127)
                GoUp();
            else if (ch[0] == 10)
                EnterKey();
            else if (ch[0] == ':') {
                command_mode();
                printf("\033[H\033[J");
                cx = 1;
                cy = 1;
                move_cursor();
                exit(1);
            }
            else if (ch[0] == 'q') {
            	printf("\033[H\033[J");
            	write(STDOUT_FILENO, "\x1b[2J", 4);
        	tcsetattr(STDIN_FILENO, TCSAFLUSH, &initSetting);
                cx = 1;
                cy = 1;
                move_cursor();
                exit(1);
            }
        }
}

int main()
{
    getcwd(root, sizeof(root)); 
    strcpy(current_dir, root);
    backward_stack.push(root); 
    printf("%c[?1049h",27); 
    listdir(root); 
    Normar_Mode();
    return 0;
}
