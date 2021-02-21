# Todo manager

Creates to-do lists in .md files. The program makes the lists easily readable and editable and helps to divide tasks into subtasks. It also helps to track time. 

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
Subprojects can be started within projects, i.e. one can always convert a to-do -item to a project. The file may grow long but should be relatively easy to read 
if enough subtasks are used. Long to-lists can be daunting. 

My vision is that these kind of files could also be embedded in e.g. github projects to navigate issues that are too small to entitle their own github issue.
The to-do list could also act as a reminder in what state the project was left last time.

*The file "todo.md" contains a todolist that I use myself to complete this program.*


Installation
============

	git clone https://github.com/aaleino/todoman
	cd todoman
	git clone https://github.com/aaleino/FTXUI
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

Use up, down, right, left and enter in the menu to navigate. 

**ISSUE: Currently the highlight bar might be lost, just move up or down to make the highlight visible again.**


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
