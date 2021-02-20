# Todo manager

Creates to-do lists in .md files. The program makes the lists easily readable and editable and helps to divide tasks into subtasks. It also helps to track time in a Pomodoro workflow. 

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

Use up down right left in the menu to navigate. 

!!ISSUE: Currently the focus might be lost, just move up or down to make the highlighted item visible again.!!


	./todoman todo.md 40 10

Open todo.md for editing, use 40 minute interval for work, 10 minute interval for pause

	./todoman todo.md 40 10 7.25

As above, but set total working time goal as 7.25h

	./todoman todo.md 40 10 7.25 restart

As above, but restart working time

Credits
=======

(C) Aleksi Leino 2021

Uses FTXUI by Arthur Sonzogni
https://github.com/ArthurSonzogni/FTXUI
(I have forked my own version that reduces blinking in windows terminal)
