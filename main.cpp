/*
MIT License

Copyright (c) 2021 Aleksi Leino

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <codecvt>
#include <fstream>
#include <iostream>
#include <list>
#include <locale>
#include <string>
#include <thread>
#include <vector>
#include <time.h>

#include "ftxui/component/button.hpp"
#include "ftxui/component/checkbox.hpp"
#include "ftxui/component/container.hpp"
#include "ftxui/component/input.hpp"
#include "ftxui/component/menu.hpp"
#include "ftxui/component/radiobox.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/toggle.hpp"
#include "ftxui/dom/elements.hpp"

bool quit_program = false;
int offset = 0;
time_t current_time;
bool working_on_a_task = false;
time_t accumulated_time = 0;
time_t time_at_task_start, time_at_task_stop;

double tasktime = 60 * 25;
double worktime = 26100;
double pausetime = 60 * 5;

std::ofstream logfile;

std::string work_task_title = "Currently not working on a task";

std::string saved_message;
bool annotate = false;
bool annotation_begins=true;
time_t annotation_start;
std::string annotation;


std::string accumulated_time_filename = "temp_accumulated_time.txt";

using namespace ftxui;
using namespace std;

using convert_t = std::codecvt_utf8<wchar_t>;
std::wstring_convert<convert_t, wchar_t> strconverter;

std::string to_string(std::wstring wstr)
{
  return strconverter.to_bytes(wstr);
}

std::wstring to_wstring(std::string str)
{
  return strconverter.from_bytes(str);
}

bool is_number(const std::string &s)
{
  std::string::const_iterator it = s.begin();
  while (it != s.end() && std::isdigit(*it))
    ++it;
  return !s.empty() && it == s.end();
}

// from https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
inline bool file_exists(const std::string &name)
{
  if (FILE *file = fopen(name.c_str(), "r"))
  {
    fclose(file);
    return true;
  }
  else
  {
    return false;
  }
}

vector<string> read_lines(string filename)
{
  vector<string> values;
  ifstream infile(filename.c_str());
  string line;
  while (getline(infile, line))
  {
    values.push_back(line);
  }
  if (infile.is_open())
    infile.close();
  return values;
}

int count_level(string instring)
{
  int ilevel = 0;
  for (int i = 0; i < instring.length(); i++)
  {
    if (iswspace(instring[i]))
    {
      ilevel++;
    }
    else
    {
      return ilevel;
    }
  }
  return ilevel;
}

bool line_is_empty(string line)
{
  if (line.find_first_not_of(' ') != string::npos)
  {
    return false;
  }
  return true;
}

// this returns the tasks in a two dimensional array so that the tasks on this
// level are on the first column and subtasks on the rest
vector<vector<string>> parse_tasks(vector<string> tasklist)
{
  // count the intendation level
  int curline = 0;
  int current_level = 0;
  bool first = true;

  vector<vector<string>> parsed_task_list;

  vector<string> current_task;

  for (int curline = 0; curline < tasklist.size(); curline++)
  {
    // don't do anything if the line is empty
    if (line_is_empty(tasklist[curline]))
    {
      continue;
    }

    // if this is the first non-empty line, check the indendation level
    // and set up as first entry in the array
    if (first)
    {
      current_task.push_back(tasklist[curline]);
      current_level = count_level(tasklist[curline]);
      // cout << current_level << ": " << tasklist[curline] << "\n";
      first = false;
      continue;
    }

    int line_level = count_level(tasklist[curline]);
    //    cout << line_level << ": " << tasklist[curline] << "\n";

    if (line_level > current_level)
    {
      // add this line as a subtask of the current task
      current_task.push_back(tasklist[curline]);
    }
    else
    {
      // add this line as a new task
      parsed_task_list.push_back(current_task);
      // clear all content from current task
      current_task.clear();
      current_task.push_back(tasklist[curline]);
    }
  }
  // add the last task
  parsed_task_list.push_back(current_task);

  return parsed_task_list;
}

string trim_to_filename(string input)
{
  string rvalue;
  for (int i = 0; i < input.length(); i++)
  {
    if (iswspace(input[i]))
      continue;
    if (input[i] == '|')
      break;
    rvalue.push_back(tolower(input[i]));
  }
  return rvalue;
}

string trim_to_title(string input)
{
  string rvalue;
  int i = 0;
  if (input.length() >= 5)
  {
    if (input[0] == '-' && input[1] == ' ' && input[2] == '[')
    {
      i = 6;
    }
    if (input[0] == '-' && input[1] == ' ' && input[2] != '[')
    {
      i = 2;
    }
  }
  for (; i < input.length(); i++)
  {
    if (input[i] == '|')
      break;
    rvalue.push_back(input[i]);
  }
  return rvalue;
}

double get_relative_size(string input)
{
  bool has_begun = false;
  string value;
  for (int i = 0; i < input.length(); i++)
  {
    if (input[i] == '|')
    {
      has_begun = true;
      continue;
    };
    if (has_begun)
      value.push_back(input[i]);
  }
  if (!has_begun)
    return 1;
  return stod(value);
}

struct task
{
  bool is_note;
  bool completed;
  double importance;
  string name;
  string comment;
  std::list<task> subtasks;
};

string remove_whitespace(string input)
{
  string value;
  bool has_begun = false;
  for (int i = 0; i < input.length(); i++)
  {
    if (!iswspace(input[i]) && !has_begun)
    {
      has_begun = true;
    }
    if (has_begun)
    {
      value.push_back(input[i]);
    }
  }
  return value;
}

string remove_whitespace_from_end(string input)
{
  int len = input.length();
  string rvalue = input;
  if (len == 0)
    return rvalue;
  if (isspace(input[input.length() - 1]))
  {
    rvalue.erase(rvalue.end() - 1, rvalue.end());
  }
  return rvalue;
}

bool check_if_checked(string input)
{
  string value;
  bool has_begun = false;
  if (input.length() >= 4)
  {
    if (input[0] == '-' && input[1] == ' ' && input[2] == '[' &&
        input[3] == 'x')
    {
      return true;
    }
    else
      return false;
  }
  return false;
}

bool check_if_note(string input)
{
  string value;
  bool has_begun = false;
  if (input.length() >= 4)
  {
    if (input[0] == '-' && input[1] == ' ' && input[2] != '[')
    {
      return true;
    }
    else
      return false;
  }
  return false;
}


std::list<task> extract_tasks(vector<string> todolist)
{
  auto todolist_parsed = parse_tasks(todolist);
  std::list<task> rvalue;
  for (int i = 0; i < todolist_parsed.size(); i++)
  {
    task newtask;

    newtask.name = trim_to_title(remove_whitespace(todolist_parsed[i][0]));

    newtask.is_note = check_if_note(remove_whitespace(todolist_parsed[i][0]));

    if(!newtask.is_note) {
        newtask.completed = 
            check_if_checked(remove_whitespace(todolist_parsed[i][0]));
    } else {
        newtask.completed = true;
    }

    newtask.importance = get_relative_size(todolist_parsed[i][0]);

    if (todolist_parsed[i].size() > 1)
    {
      vector<string> subtodolist;
      for (int j = 1; j < todolist_parsed[i].size(); j++)
      {
        subtodolist.push_back(todolist_parsed[i][j]);
      }
      newtask.subtasks = extract_tasks(subtodolist);
    }
    rvalue.push_back(newtask);
  }
  return rvalue;
}

void print_task_rec(ofstream &file, task &task, int level)
{
  for (int i = 0; i < level; i++)
  {
    file << " ";
  }

  if (task.is_note && task.subtasks.size() == 0)
  {
    file << "- ";
  } else {
    if (task.completed && task.subtasks.size() == 0)
    {
      file << "- [x] ";
    }

    if (!task.completed && task.subtasks.size() == 0)
    {
      file << "- [ ] ";
    }
  }

  file << task.name << "\n";

  for (auto subtask : task.subtasks)
  {
    print_task_rec(file, subtask, level + 5);
  }
  return;
}

void save_tasks(string filename, task &task)
{
  ofstream myfile;
  myfile.open(filename);
  print_task_rec(myfile, task, 0);
  myfile.close();
  return;
}

class NumberedCheckBox : public CheckBox
{
public:
  NumberedCheckBox() {}
  void give_number(int _number) { number = _number; }
  task default_task;
  std::reference_wrapper<task> my_reference = default_task;
  int number;
};

// quick and dirty
class MyInput : public Input
{
public:
  int colormode;

  MyInput() {};

  void set_colormode(int _colormode) {
    colormode=_colormode;
  }
  std::function<void()> on_pageup = []() {};
  std::function<void()> on_pagedown = []() {};
  
  Element Render()
  {
    return (vbox({hbox({Modified_Render() | size(HEIGHT, EQUAL, 1)}) |
                  size(WIDTH, EQUAL, 40)}));
  }

  bool OnEvent(Event event) override
  {
    if (event == Event::Special("\x1b[5~"))
    {
      on_pageup();
    }
    else if (event == Event::Special("\x1b[6~"))
    {
      on_pagedown();
    }
    else
    {
      return Input::OnEvent(event);
    }
  }

  Element Modified_Render()
  {
    cursor_position = std::max(0, std::min<int>(content.size(), cursor_position));
    auto main_decorator = flex | size(HEIGHT, EQUAL, 1);
    bool is_focused = Focused();

    // Placeholder.
    if (content.size() == 0)
    {
      if (is_focused) {
        if(colormode==1) {
          return text(placeholder) | focus | dim | inverted | main_decorator;
        } else {
          return text(placeholder) | focus | dim | color(Color::GreenLight) | inverted | main_decorator;
        }
      } else {
        if(colormode==1) {
          return text(placeholder) | dim | main_decorator;
        } else {
          return text(placeholder) | color(Color::Green) | main_decorator;
        }
      }
    }

    // Not focused.
    if (!is_focused)
      return text(content) | main_decorator;

    std::wstring part_before_cursor = content.substr(0, cursor_position);
    std::wstring part_at_cursor = cursor_position < (int)content.size()
                                      ? content.substr(cursor_position, 1)
                                      : L" ";
    std::wstring part_after_cursor = cursor_position < (int)content.size() - 1
                                         ? content.substr(cursor_position + 1)
                                         : L"";
    auto focused = is_focused ? focus : ftxui::select;

    // clang-format off
    return
      hbox(
        text(part_before_cursor),
        text(part_at_cursor) | underlined | focused,
        text(part_after_cursor)
      ) | flex | inverted | frame | main_decorator;
    // clang-format off
  }
};

class GaugeComponent : public Component
{
public:
  Element RenderGauge(int mode)
  {
    float progress = 0;
    wstring fronttext;
    wstring backtext;

    if (mode == 1)
    {
      // show
      if (working_on_a_task)
      {
        progress = (time(NULL) - time_at_task_start) / tasktime;
        fronttext = L"Task progress";
        curcol = Color::BlueLight;
        if (progress >= 1)
        {
          backtext = L"Time to consider a pause!";
          curcol = Color::Green;
        }
        else
        {
          backtext = L"Not complete";
          curcol = Color::BlueLight;
        }
      }
      else
      {
        progress = (time(NULL) - time_at_task_stop) / pausetime;
        fronttext = L"Pause progress";

        if (progress >= 1)
        {
          backtext = L"You should get back to work!";
          curcol = Color::Red;
        }
        else
        {
          backtext = L"Not complete";
          curcol = Color::BlueLight;
        }
      }
    }
    if (mode == 2)
    {
      if (working_on_a_task)
      {
        progress =
            ((time(NULL) - time_at_task_start) + accumulated_time) / worktime;
      }
      else
      {
        progress = accumulated_time / worktime;
      }
      fronttext = L"Daily accumulation";
      backtext = L"Not complete";

      if (progress >= 1)
      {
        backtext = L"Completed";
        dprogcol = Color::Green;
      }
      else
      {
        backtext = L"Not complete";
      }
    }

    if (progress >= 1)
      progress = 1;

    return hbox({
        text(fronttext) | size(WIDTH, EQUAL, 30),
        text(backtext) | size(WIDTH, EQUAL, 30),
        text(std::to_wstring(int(progress * 100)) + L"% ") |
            size(WIDTH, EQUAL, 5),
        gauge(progress),
    });
  }
  Element Render() override
  {
    return window(text(L"Summary"),
                  vbox({text(to_wstring(work_task_title)), separator(), RenderGauge(1) | color(curcol),
                        separator(), RenderGauge(2) | color(dprogcol)}));
  };
  Color curcol = Color::BlueLight;
  Color dprogcol = Color::GrayDark;
};

void save_accumulation()
{
  std::ofstream accfile(accumulated_time_filename, ios::out);
  if (working_on_a_task)
  {
    accfile << accumulated_time + (time(NULL) - time_at_task_start);
    accfile.close();
  }
  else
  {
    accfile << accumulated_time;
    accfile.close();
  }
}

void put_to_log(string entry)
{
  std::time_t now = std::time(NULL);
  std::tm *ptm = std::localtime(&now);
  char buffer[32];
  // Format: Mo, 15.06.2009 20:20:00
  std::strftime(buffer, 32, "%a, %d.%m.%Y %H:%M:%S", ptm);
  logfile.open("todo_log.txt", std::ios_base::app);
  logfile << buffer << ": " << entry << "\n";
  logfile.close();
}

class TodoMenu : public Menu {
public:
  TodoMenu() {};
  std::vector<int> itemcolors;
  std::function<void()> on_pageup = []() {};
  std::function<void()> on_pagedown = []() {};

  bool OnEvent(Event event) override {
    if(event == Event::Special("\x1b[5~")) {
      on_pageup();
    } else if(event == Event::Special("\x1b[6~")) {
      on_pagedown();
    } else {
      return  Menu::OnEvent(event);
    }
  }

  Element Render() {
  std::vector<Element> elements;
  bool is_focused = Focused();
  for (size_t i = 0; i < entries.size(); ++i) {
    auto style = (selected != int(i))
                     ? normal_style
                     : is_focused ? focused_style : selected_style;
    auto focused = (selected != int(i)) ? nothing : is_focused ? focus : ftxui::select;
    auto icon = (selected != int(i)) ? L"  " : L"> ";
    if(itemcolors.size() == entries.size() && itemcolors[i] == 1) {
      elements.push_back(text(icon + entries[i]) | color(Color::GreenLight) | style | focused);
    } else {
      elements.push_back(text(icon + entries[i]) | style | focused);
    }
  }
  return vbox(std::move(elements));
}


};

class TodoManager : public Component
{
public:
  std::function<void()> on_quit = [] {};

  TodoManager(task &_root_task)
  {
    input_1.set_colormode(1);
    input_2.set_colormode(2);

    Add(&maincontainer);
    maincontainer.Add(&container);

    button_exit.on_click = [&] {quit_program=true; on_quit(); };

    button_go_back.on_click = [&] {
      // todoselection.go_back();
    };

    menu.entries = {L"Go Back",
                    L"Add task above",
                    L"Add note above",
                    L"Convert to project",
                    L"Remove task / note",
                    L"Start working",
                    L"Stop working",
                    L"Select task at random",
                    L"Exit"};
    menu.selected = 0;
    timergauge.focusable = false;

    maincontainer.Add(&timergauge);

    container.Add(&left_container);
    container.Add(&right_container);
    input_1.placeholder = L"Enter new task here";
    input_2.placeholder = L"Enter new note here";

/*
    input_2.color = fgcolor(Color::Blue);
    input_1.color = bgcolor(Color::BlueLight);
*/
    right_container.Add(&menu);
    left_container.Add(&todomenu);
    left_container.Add(&input_2);
    left_container.Add(&input_1);

    todomenu.selected_style = bgcolor(Color::Blue);
    todomenu.focused_style = bgcolor(Color::BlueLight);

    input_1.on_enter = [&] {
      task &ctit = current_active_task;
      task newtask;
      newtask.completed = false;
      newtask.is_note = false;
      newtask.name = to_string(input_1.content);
      ctit.subtasks.push_back(newtask);
      input_1.content = L"";
      updateSelection();
    };

    input_2.on_enter = [&] {
      task &ctit = current_active_task;
      task newtask;
      newtask.completed = true;
      newtask.is_note = true;
      newtask.name = to_string(input_2.content);
      ctit.subtasks.push_back(newtask);
      input_2.content = L"";
      updateSelection();
    };

    menu.on_enter = [&] {
      if (menu.selected == 7)
      {
        int i = 0;
        task &ctit = current_active_task;

        vector<int> uncompleted;
        for (auto ctask : ctit.subtasks)
        {
          if (ctask.completed == false)
            uncompleted.push_back(i);
          i++;
        }
        if (uncompleted.size())
        {
          int rand_task = uncompleted[rand() % uncompleted.size()];
          todomenu.selected = rand_task;
          todomenu.TakeFocus();
        }
      }

      if (menu.selected == 0)
      {
        bool all_completed = true;
        task &ctit = current_active_task;
        for (auto ctask : ctit.subtasks)
        {
          if (ctask.completed == false)
            all_completed = false;
        }
        if (all_completed)
        {
          ctit.completed = true;
        }
        else
        {
          ctit.completed = false;
        }
        if (previous_task.size() > 0)
        {
          current_active_task = previous_task.back();
          updateSelection();
          previous_task.pop_back();
        }
      }
      else if (menu.selected == 1)
      {
        int i = 0;
        task &ctit = current_active_task;
        for (auto it = ctit.subtasks.begin(); it != ctit.subtasks.end(); ++it)
        {
          if (i == todomenu.selected)
          {
            task newtask;
            newtask.is_note = false;
            newtask.completed = false;
            newtask.name = to_string(input_1.content);
            ctit.subtasks.insert(it, newtask);
          }
          i++;
        }
        updateSelection();
      }
      else if (menu.selected == 2)
      {
        int i = 0;
        task &ctit = current_active_task;
        for (auto it = ctit.subtasks.begin(); it != ctit.subtasks.end(); ++it)
        {
          if (i == todomenu.selected)
          {
            task newtask;
            newtask.is_note = true;
            // all notes must be completed
            newtask.completed = true;
            newtask.name = to_string(input_2.content);
            ctit.subtasks.insert(it, newtask);
          }
          i++;
        }
        updateSelection();
      }
      else if (menu.selected == 3)
      {
        // convert to project
        int i = 0;
        task &ctit = current_active_task;

        for (auto it = ctit.subtasks.begin(); it != ctit.subtasks.end(); ++it)
        {

          if (i == todomenu.selected)
          {
            if((*it).is_note) {
              annotate=true;
              annotation="Cannot convert note to project!";
              return;
            }
            previous_task.push_back(ctit);
            current_active_task = *it;
            input_1.TakeFocus();
          }
          i++;
        }

        updateSelection();
      }
      else if (menu.selected == 4)
      {
        // remove task
        int i = 0;
        task &ctit = current_active_task;
        for (auto it = ctit.subtasks.begin(); it != ctit.subtasks.end(); ++it)
        {
          if (i == todomenu.selected)
          {
            if ((*it).subtasks.size() == 0)
            {
              ctit.subtasks.erase(it);
              updateSelection();
              return;
            };
          }
          i++;
        }
      }
      else if (menu.selected == 5)
      {
        int i = 0;
        task &ctit = current_active_task;
        for (auto it = ctit.subtasks.begin(); it != ctit.subtasks.end(); ++it)
        {
          if (i == todomenu.selected)
          {
            if((*it).is_note) {
              annotate=true;
              annotation="Cannot start working on a note!";
              return;
            }
            work_task_title = (*it).name;
            break;
          }
          i++;
        }
        string title_end;
        title_end += " (";
        title_end += remove_whitespace_from_end(ctit.name);
        bool first = true;
        for (auto ct : previous_task)
        {
          task &ctask = ct;

          if (ctask.name.length() > 0)
          {
            if (first)
            {
              title_end += ", ";
              first = false;
            }
            else
            {
              title_end += "->";
            }
            title_end += remove_whitespace_from_end(ctask.name);
          }
        }
        title_end += ")";
        if (title_end.compare(" ()") > 0)
          work_task_title += title_end;


        // if already working on task, do no reset counter
        if (working_on_a_task)
        {
          put_to_log("Switched task to " + work_task_title);
        }
        else
        {
          // start working on a task
          put_to_log("Started a working session. First task " + work_task_title);
          working_on_a_task = true;
          time_at_task_start = time(NULL);
        }
      }
      else if (menu.selected == 6)
      {
        // end working on a task
        if(working_on_a_task == true) {
          work_task_title = "Currently not working on a task";
          time_at_task_stop = time(NULL);
          working_on_a_task = false;
          accumulated_time += time(NULL) - time_at_task_start;
          put_to_log("Ended working session. Total time " +
                    to_string((time(NULL) - time_at_task_start) / 60.0f) + " minutes.");
          put_to_log("Daily accumulation " + to_string(accumulated_time) +
                    " seconds.");
        }
      }

      if (menu.selected == 8)
      {
        on_quit();
      }
    };

    todomenu.on_pageup = [&] {
        todomenu.selected = 0;
    };

    todomenu.on_pagedown = [&] {
        task ctask = current_active_task;
        input_2.TakeFocus();

        
        if(todomenu.selected == todomenu.entries.size() - 1) {
        } else{
          todomenu.selected = todomenu.entries.size()-1;
        }
    };

    menu.on_pageup = [&] {
        menu.selected = 0;
    };

    menu.on_pagedown = [&] {
        menu.selected = menu.entries.size()-1;
    };


    input_2.on_pageup = [&] {
      todomenu.TakeFocus();
      todomenu.selected = todomenu.entries.size()-1;
    };

    input_2.on_pagedown = [&] {
      input_1.TakeFocus();
      todomenu.selected = todomenu.entries.size()-1;
    };


    input_1.on_pageup = [&] {
      input_2.TakeFocus();
      todomenu.selected = todomenu.entries.size()-1;
    };
    todomenu.on_change = [&] {

    };
    todomenu.on_enter = [&] {
      int i = 0;

      task temptask;
      task &cat = current_active_task;
      bool all_tasks_completed = true;
      bool found_match = false;

      for (auto &task : cat.subtasks)
      {
        if (i == todomenu.selected)
        {
          if (task.subtasks.size() > 0)
          {
            previous_task.push_back(cat);
            current_active_task = task;
            updateSelection();
            return;
          }
          else
          {
            task.completed = task.completed ? false : true;
            if (task.completed)
            {
              put_to_log("Marked task \"" + task.name + "\" as complete");
            }
            else
            {
              put_to_log("Marked task \"" + task.name + "\" uncomplete");
            }
          }
        }
        if (task.completed == false)
        {
          all_tasks_completed = false;
        }
        i++;
      }

      if (all_tasks_completed)
      {
        if (previous_task.size() > 0)
        {
          cat.completed = true;
        }
      }
      else
      {
        cat.completed = false;
      }
      updateSelection();
    };

    current_active_task = _root_task;

    updateSelection();
    input_2.TakeFocus();

  }

  void updateSelection()
  {
#if defined(_WIN32)
    std::wstring checked = L"[X] ";   /// Prefix for  a "checked" state.
    std::wstring unchecked = L"[ ] "; /// Prefix for  an "unchecked" state.
#else
    std::wstring checked = L"▣ ";   /// Prefix for  a "checked" state.
    std::wstring unchecked = L"☐ "; /// Prefix for  a "unchecked" state.
#endif

    vector<wstring> items;
    int i = 0;
    task &cat = current_active_task;


    if (cat.subtasks.size() == 0)
    {
      todomenu.focusable = false;
    }
    else
    {
      todomenu.focusable = true;
    }

    vector <int> colors;
    for (auto &task : cat.subtasks)
    {
      wstring taskname;
      if(!task.is_note) {
        colors.push_back(0);
        if (task.completed)
        {
          taskname = checked;
        }
        else
        {
          taskname = unchecked;
        }
      } else {
          colors.push_back(1);
          taskname = to_wstring("  - ");
      }
      taskname += to_wstring(task.name);
      if (task.subtasks.size() > 0)
      {
        taskname += L"...";
      }
      items.push_back(taskname);
    }
    todomenu.entries = items;
    todomenu.itemcolors = colors;
  }

  Element Render() override
  {
    task &cat = current_active_task;
    return window(text(to_wstring(cat.name)) | hcenter, vbox(
                                                            {hbox({vbox(todomenu.Render(), input_2.Render(), input_1.Render()) | frame, menu.Render() /*, input_1.Render()*/}) |
                                                                 flex,
                                                             timergauge.Render() | size(HEIGHT, GREATER_THAN, 5)}));
  }

private:
  Container maincontainer = Container::Vertical();
  Container container = Container::Horizontal();

  Container left_container = Container::Vertical();
  Container right_container = Container::Vertical();

  //  Container lower_sub_container = Container::Vertical();
  TodoMenu menu;
  TodoMenu todomenu;
  MyInput input_1, input_2;
  GaugeComponent timergauge;

  Button button_go_back, button_add, button_remove, button_add_subtask,
      button_start_working, button_random, button_exit;
  //  TodoSelection todoselection;
  task tmptask;
  std::reference_wrapper<task> current_active_task = tmptask;
  vector<std::reference_wrapper<task>> previous_task;
  vector<std::reference_wrapper<task>> parent_task;
};

bool count_uncompleted_tasks(task &task)
{
  int sum = 0;
  if (task.subtasks.size() > 0)
  {
    for (auto &ctask : task.subtasks)
    {
      sum += count_uncompleted_tasks(ctask);
    }
  }
  else if (!task.completed)
    sum++;
  return sum;
}

// if a project with subtasks has any uncompleted tasks,
// mark it uncomplete
void check_completed_projects(task &task)
{

  bool all_completed = false;
  int utasks = 0;
  if (task.subtasks.size() > 0)
  {
    for (auto &ctask : task.subtasks)
    {
      utasks += count_uncompleted_tasks(ctask);
      check_completed_projects(ctask);
    }
    if (!utasks)
    {
      task.completed = true;
    }
  }

  return;
}

int main(int argc, const char *argv[])
{
  string todofile;
  srand(time(NULL));

  if (argc < 2)
  {
    printf("Usage: %s <filename.md>\n", argv[0]);
    printf("       %s <filename.md> <work interval duration minutes> <pause duration in minutes> <total (e.g. daily) work time hours> <keyword=restart>\n\n", argv[0]);
    printf("Examples:%s todo.md\n", argv[0]);
    printf("         Open todo.md for editing, use default intervals (25 min work 5 min pause)\n\n");
    printf("         %s todo.md 40 10\n", argv[0]);
    printf("         Open todo.md for editing, use 40 minute interval for work, 10 minute interval for pause\n\n");
    printf("         %s todo.md 40 10 7.25\n", argv[0]);
    printf("         As above, but set total working time goal as 7.25h \n\n");
    printf("         %s todo.md 40 10 7.25 restart\n", argv[0]);
    printf("         As above, but restart the accumulation in working time\n\n");
    return 0;
  }

  if (argc > 1)
  {
    todofile = argv[1];
  }
  if (argc > 2)
  {
    tasktime = stoi(argv[2]) * 60;
  }
  if (argc > 3)
  {
    pausetime = stoi(argv[3]) * 60;
  }
  if (argc > 4)
  {
    worktime = stod(argv[4]) * 60 * 60;
  }

  time_at_task_stop = time(NULL);
  auto time_at_last_todo_save = time(NULL);

  if (argc > 5 && !string(argv[5]).compare("restart"))
  {
    accumulated_time = 0;
  }
  else
  {
    std::ifstream accfile(accumulated_time_filename, std::ifstream::in);

    std::string line;
    if (getline(accfile, line))
    {
      accumulated_time = stoi(line);
    }
    accfile.close();
  }
  task root_task;

  if (!file_exists(todofile))
  {

  }
  else
  {
    vector<string> todolist_raw = read_lines(todofile);
    root_task.subtasks = extract_tasks(todolist_raw);
  }
  check_completed_projects(root_task);
  auto screen = ScreenInteractive::Fullscreen();
  bool should_quit = false;

  std::thread update([&screen, &todofile, &root_task, &should_quit, &time_at_last_todo_save]() {
    while (!should_quit)
    {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(0.3s);
      current_time = time(NULL);

      if (current_time - time_at_last_todo_save > 30)
      {
        save_tasks(todofile, root_task);
        save_accumulation();
        time_at_last_todo_save = time(NULL);
      }

      if(annotate) {
        if(annotation_begins==true) {
            annotation_start=time(NULL);
            annotation_begins=false;
            saved_message=work_task_title;
            work_task_title = annotation;
        }
        if(current_time - annotation_start > 3) {
            work_task_title=saved_message;
            annotate=false;
            annotation_begins=true;
        }
      }

      screen.PostEvent(Event::Custom);
    }
  });

  TodoManager component(root_task);


  component.on_quit = screen.ExitLoopClosure();
  screen.Loop(&component);
  should_quit = true;

  update.join();

  save_tasks(todofile, root_task);
  save_accumulation();

  return 0;
}
