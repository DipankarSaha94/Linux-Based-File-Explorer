# Linux Based File Explorer

## Functionalities of the Normal Mode are as follows:

* On executing main.cpp, the directory structure where this file is present would open. This is the root directory.
   Navigation could be done only in direction forward of the root.

* To scroll in the displayed list of files and directories, use UP and DOWN arrow keys.

* On pressing "Enter" on a directory, list of directories/files inside that directory opens up. In case of a file, "Enter" 
	would open that file.

* On pressing Backspace, navigate to parent directory of current directory. No effect is seen if current directory is root.

* On pressing Left Arrow key, previously visited directory is displayed.

* On pressing Right Arrow key, next directory among the visited directories stack is displayed.

* On pressing 'h' or 'H', Home(Root) directory is reached

* Press ':' to enter into Command Mode

* Press 'q' to exit from the application.


## Functionalities of the Command Mode are as follows:

* 'goto dir_path' will go to that directory,here path can be relative from root or complete path.

* 'copy_file filename dir_path' will copy file to given directory, here file will be in current_dir and path can be relative or full.

* 'copy_dir dirname dir_path' will copy directory to given directory, here directory will be in current_dir and path can be relative or full. 

* 'create_file filename dir_path' file will be created in given directory, path can be relative or full or current.

* 'create_dir dirname dir_path' dir will be created in given directory, path can be relative or full or current.

* 'search filename' will search file recursively in current_dir.

* 'search dirname' will dir file recursively in current_dir.

* 'rename old new' will rename the old file to new in current_dir

* 'delete_file file_path' will delete file from given path, here path can be relative or full.

* 'delete_dir dir_path' will delete dir from given path, here path can be relative or full.

* 'move filename dir_path' will move the file from current_dir to given path, here path can relative or full.

* 'move full_path_dire dir_path' will move the dir from given path to destination path.
