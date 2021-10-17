# Todo manager

This program makes todo lists easily readable and editable and helps to divide tasks into subtasks. It also helps to track time. 
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


Explanation: "Work" was converted to a project, and it contains subitems. "Hobby" was not and it can be tagged completed/uncompleted.
Subprojects can be started within projects, i.e. one can always convert a to-do item to a project. 

Reading long to-do lists can be daunting. The file may grow long but should be relatively easy to read with the program if enough projects (subtasks) are used.  

My vision is the program will allow users to create very detailed to-do lists and help them to organize large and more abstract tasks.
A detailed to-do list could help in remembering the state in which the project was left last time.

*The file "todo.md" contains a todolist that I use myself to complete this program.*

Update 17.10: New feature
=========================

Thanks to all who have given a star to the program! It has motivated me to continue working on it! :) The program can now also be used to write small notes. In addition, support for PgUp and PgDown has been added, which makes navigation slightly easier.

![demo](https://github.com/aaleino/todoman/blob/main/feature_notes.gif "Note demo")

The timer
=========

The pause timer is automatically on.

Once you start working on any item, the program will count the time towards a goal that can be set at command line (see *Usage*).
The progress is shown in percentages:  (Current duration) / (Target time) * 100%

If you change the task before the timer has been completed, the work timer will not reset. It will reset **only when** you press "stop working".

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

	git clone https://github.com/aaleino/todoman
	cmake .
	cmake --build .
	

Target environment
==================

Tested in WSL+Windows terminal, Ubuntu+default terminal 

Usage
=====


Example:

	./todoman todo.md

Open todo.md for editing, use default intervals (25 min work 5 min pause)

Use up, down, right, left, pgup, pgdown and enter in the menu to navigate. 

	./todoman todo.md 40 10

Open todo.md for editing, use 40 minute interval for work, 10 minute interval for pause

	./todoman todo.md 40 10 7.25

As above, but set total working time goal as 7.25h

	./todoman todo.md 40 10 7.25 restart

As above, but restart working time.

Credits
=======

(C) Aleksi Leino 2021

Uses FTXUI by Arthur Sonzogni
https://github.com/ArthurSonzogni/FTXUI
(I have forked my own version that reduces blinking in windows terminal)
