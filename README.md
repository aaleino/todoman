# Todo manager

This program makes todo lists easily readable and editable and helps to divide tasks into subtasks. It also tracks the time spent on each task. 
This is an experimental hobby project that is under construction.

Demo
====

For example:

![demo](https://github.com/aaleino/todoman/blob/main/todoman.gif "Todo manager demo")

produces

     Work
          - [x] Write article
          - [x] Write software
     - [ ] Hobby


Explanation: "Work" was converted to a project that contains subitems. "Hobby" was not and can be tagged completed/uncompleted. "Work" was completed when all the subtasks were completed. Subprojects can be started within projects, i. e. one can always convert a to-do item to a project.

The bars keep track of the total time towards a goal (e. g. daily target) and suggest when to take a break. After quitting the program, the user can generate a time usage report on the tasks on an all-time or per session basis. This information is stored in the to-do file as comments and resets when requested.

Reading long to-do lists can be daunting. The file may grow long but should be relatively easy to read with the program if enough projects (subtasks) are used.

My vision is the program will allow users to create very detailed to-do lists. It should also help to organize large, more abstract tasks. A detailed to-do list could help in remembering the state the project was left last time. Time tracking could help identify bottlenecks in the daily routine and check for wise time usage.

See the changelog at the end of this file for a list of recent changes.

The timer
=========

The pause timer is automatically on.

Once you start working on any item, the program will count the time towards a goal that can be set at command line (see *Usage*).
The progress is shown in percentages:  (Current duration) / (Target time) * 100%.

The total time spent on each task is stored as comments. This data can be viewed using "-s 0" or "-s 1" options. 

If you change the task before the timer has been completed, the work timer will not reset. It will reset **only when** you press "stop working".
If task time tracking is used, the time worked so far will be deposited to the previous task.

The 'daily accumulation' counter increases **only when** working on a task. It will keep on increasing also after task progress reaches 100%.

The log
=======

A log is maintained in "todo_log.txt". The log functionality is still under development.

Caveat
======

**Do not open any other files with the program than todo lists made with the program.**
The files can be reorganized using any text editor, but the editor should use whitespace instead of tabs for indentation.
Linux command 'expand' works for getting rid of tabs.

Installation
============


Assuming recent enough CMake is installed:

	git clone https://github.com/aaleino/todoman
	cmake .
	cmake --build .


Target environment
==================

Tested in WSL+Windows terminal, Ubuntu+default terminal 

Usage
=====
```
Usage: ./todoman <filename.md> <options>
Options:
-t <tasktime> <pausetime> <worktime>
   tasktime: time in minutes to complete a task
   pausetime: time in minutes to pause between tasks
   worktime: time in hours to work on a task
   These options are automatically stored into the metadata of the file.
-v print version
-h print help
-r reset session
-e erase metadata (erases all timer information from the file and quits. Does a full timer/setting reset.)
-d disable metadata (disables storing timer information to the file. Erases existing timer information from the file.)
-s <option>
   generate a time usage report from the file

   option: set to "1" for session statistics, "0" for all time statistics
```

Metadata contains information on how much time has been spent on a task and other state varibles. 
It will be stored into the file enclosed between '<!-- ^ ' and ' ^--!>'.

Example:

	./todoman todo.md

Open todo.md for editing, use default intervals (40 min work 5 min pause)

Use up, down, right, left, enter, pgup and pgdown in the menu to navigate. 
Use 'space' to move note/task, 'insert' to create one between others and 'e' to edit one. 

	./todoman todo.md -t 40 10

Open todo.md for editing, use 40 minute interval for work, 10 minute interval for pause.
These settings will be stored in the file by default. This command is used for changing the intervals. 

	./todoman todo.md -t 40 10 7.25

As above, but set total working time goal as 7.25h

	./todoman todo.md -r

Restart working time.

	./todoman todo.md -s 1

Generate report on the time spent on each task during the current session (use -s 0 for a report about all sessions).


Credits
=======

(C) Aleksi Leino 2021

Uses FTXUI by Arthur Sonzogni
https://github.com/ArthurSonzogni/FTXUI
(I have forked my own version that reduces blinking in windows terminal)

Changelog
=========

12.12.2021:

Major changes related to time tracking. The program now allows tracking time and storing settings in the edited file. Beware of bugs!!

11.12.2021:

Support for space and 'e' -key. Pressing space one time will highlight one task/note for movement. The second press will move the note below the current highlight location. Pressing 'e' allows editing the task/note.

5.12.2021:

Support for insert key. Pressing insert in the list will move the highlight so that it is easy to add tasks or notes between others. 

17.10.2021:

The program can now also be used to write small notes. In addition, support for PgUp and PgDown has been added, which makes navigation slightly easier.

![demo](https://github.com/aaleino/todoman/blob/main/feature_notes.gif "Note demo")

Thanks to all who have given a star to the program. It has motivated me to continue working on it.

