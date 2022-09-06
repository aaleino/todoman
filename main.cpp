// version string
#define VERSION "0.5.0.1"

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
// strcmp
#include <cstring>
// map
#include <map>
#include <sstream>
// isnan
#include <cmath>

// setprecision
#include <iomanip>

#define ACCUMULATED_TIME "at"
#define SESSION_TIME "st"
#define PAUSE_STAMP "ps"

#define SESSION_STAMP "ss"
#define ALLTIME_STAMP "as"

#define SESSION_END_STAMP "sse"
#define ALLTIME_END_STAMP "ase"

#define TASK_TIME "tasktime"
#define PAUSE_TIME "pausetime"
#define WORK_TIME "worktime"

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
int elsetime = 0;
int menu_type = 0;

double tasktime = 60 * 40;
double worktime = 26100;
double pausetime = 60 * 5;

std::ofstream logfile;

std::string work_task_title = "Currently not working on a task";
std::string time_title;
std::string saved_message;
bool annotate = false;
bool annotation_begins = true;
time_t annotation_start;
std::string annotation;
std::string insertfile;

using namespace ftxui;
using namespace std;

using convert_t = std::codecvt_utf8<wchar_t>;
std::wstring_convert<convert_t, wchar_t> strconverter;

bool use_metadata = true;

// define metadata starting and ending string
std::string meta_start_string = "<!-- ^ ";
std::string meta_end_string = " ^--!>";
std::map<string, string> metadata;

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
  ifstream infile(filename.c_str(), ios::in);
  string line;
  if (use_metadata)
  {
    // skip metadata
    getline(infile, line);
  }

  while (!infile.eof())
  {
    getline(infile, line);
    if (line.length() > 0)
    {
      values.push_back(line);
    }
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

// erase substring from string
void erase_substring(std::string &str, const std::string &sub)
{
  size_t start = str.find(sub);
  if (start != std::string::npos)
  {
    str.erase(start, sub.length());
  }
}
// erase everything between two strings
void erase_between(std::string &str, const std::string &sub1, const std::string &sub2)
{
  size_t start = str.find(sub1);
  size_t end = str.find(sub2);
  if (start != std::string::npos && end != std::string::npos)
  {
    str.erase(start, end - start + sub2.length());
  }
}
// erase everything unless between two strings
void erase_unless_between(std::string &str, const std::string &sub1, const std::string &sub2)
{
  size_t start = str.find(sub1);
  size_t end = str.find(sub2);
  if (start != std::string::npos && end != std::string::npos)
  {
    str.erase(end, str.length());
    str.erase(0, start + sub1.length());
  }
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
int bookmark_counter=0;

struct task
{
  bool is_note;
  bool highlighted_for_copy;
  bool completed;
  double importance;
  int bookmark_id = -1;

  string name;
  string comment;
  std::list<task> subtasks;

  // parent task
  task *parent;
  std::map<string, string> metadata;
};

string print_parent_names(task &task) {
  if(task.parent == NULL) return "";
  return "<-"+task.name+print_parent_names(*task.parent);
}


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
    if (use_metadata)
    {
      string line = todolist_parsed[i][0];
      if (line.find(meta_start_string) != std::string::npos && line.find(meta_end_string) != std::string::npos)
      {
        // remove marker
        string metadata = line;
        erase_between(line, meta_start_string, meta_end_string);
        erase_unless_between(metadata, meta_start_string, meta_end_string);

        // read words from the line into variable value pairs
        std::istringstream iss(metadata);
        std::string key, value;
        while (iss >> key >> value)
        {
          newtask.metadata[key] = value;
          // cout << metadata << "\n";
        }
      }
      else
      {
        // task does not contain metadata
      }
      todolist_parsed[i][0] = line;
    }
    newtask.name = trim_to_title(remove_whitespace(todolist_parsed[i][0]));

    newtask.is_note = check_if_note(remove_whitespace(todolist_parsed[i][0]));

    if (!newtask.is_note)
    {
      newtask.completed =
          check_if_checked(remove_whitespace(todolist_parsed[i][0]));
    }
    else
    {
      newtask.completed = true;
    }
    newtask.highlighted_for_copy = false;
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
  }
  else
  {
    if (task.completed && task.subtasks.size() == 0)
    {
      file << "- [x] ";
    }

    if (!task.completed && task.subtasks.size() == 0)
    {
      file << "- [ ] ";
    }
  }
  file << task.name;
  // write metadata
  if (use_metadata)
  {
    // if there is metadata, write it
    if (task.metadata.size() > 0)
    {
      file << meta_start_string;
      for (auto it = task.metadata.begin(); it != task.metadata.end(); it++)
      {
        file << it->first << " " << it->second << " ";
      }
      file << meta_end_string;
    }
  }

  file << "\n";

  for (auto subtask : task.subtasks)
  {
    print_task_rec(file, subtask, level + 5);
  }
  return;
}



void update_parents(task &task)
{
  for (auto &subtask : task.subtasks)
  {
    subtask.parent = &task;
    update_parents(subtask);
  }
}
std::pair<int, int> get_total_time(task &task)
{
  int total_session_time = 0;
  int total_total_time = 0;
  if (task.subtasks.size() > 0)
  {
    for (auto &ctask : task.subtasks)
    {
      std::pair<int, int> tmp = get_total_time(ctask);
      total_session_time += tmp.first;
      total_total_time += tmp.second;
    }
  }
  if (task.metadata.find(SESSION_TIME) != task.metadata.end())
  {
    total_session_time += stoi(task.metadata[SESSION_TIME]);
  }
  if (task.metadata.find(ACCUMULATED_TIME) != task.metadata.end())
  {
    total_total_time += stoi(task.metadata[ACCUMULATED_TIME]);
  }

  return std::make_pair(total_session_time, total_total_time);
}

std::pair<int, int> get_total_time_in_subtasks(task &task, bool root)
{
  int total_session_time = 0;
  int total_total_time = 0;
  if (task.subtasks.size() > 0)
  {
    for (auto &ctask : task.subtasks)
    {
      std::pair<int, int> tmp = get_total_time(ctask);
      total_session_time += tmp.first;
      total_total_time += tmp.second;
    }
  }
  if (task.metadata.find(SESSION_TIME) != task.metadata.end())
  {
    if (!root)
    {
      total_session_time += stoi(task.metadata[SESSION_TIME]);
    }
  }
  if (task.metadata.find(ACCUMULATED_TIME) != task.metadata.end())
  {
    if (!root)
    {
      total_total_time += stoi(task.metadata[ACCUMULATED_TIME]);
    }
  }
  return std::make_pair(total_session_time, total_total_time);
}

void save_tasks(string filename, task &task)
{
  ofstream myfile;
  myfile.open(filename);
  if (use_metadata)
  {
    metadata[SESSION_END_STAMP] = to_string(time(0));
    metadata[ALLTIME_END_STAMP] = to_string(time(0));
    myfile << meta_start_string;
    for (auto &pair : metadata)
    {
      myfile << pair.first << " " << pair.second << " ";
    }
    myfile << meta_end_string;
  }
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

  MyInput(){};

  void set_colormode(int _colormode)
  {
    colormode = _colormode;
  }
  std::function<void()> on_pageup = []() {};
  std::function<void()> on_pagedown = []() {};
  std::function<void()> on_insert = []() {};

  void maxout_cursor()
  {
    cursor_position = content.size();
  }

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
    else if (event == Event::Special("\x1b[2~"))
    {
      on_insert();
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
      if (is_focused)
      {
        if (colormode == 1)
        {
          return text(placeholder) | focus | dim | inverted | main_decorator;
        }
        else
        {
          return text(placeholder) | focus | dim | color(Color::GreenLight) | inverted | main_decorator;
        }
      }
      else
      {
        if (colormode == 1)
        {
          return text(placeholder) | dim | main_decorator;
        }
        else
        {
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
    string timetitle = "";
    if(time_title.size() < 2)
    {
      timetitle = time_title;
    }
    else
    {
      timetitle = time_title + ": ";
    }
    return window(text(L"Summary"),
                  vbox({text(to_wstring(timetitle + work_task_title)), separator(), RenderGauge(1) | color(curcol),
                        separator(), RenderGauge(2) | color(dprogcol)}));
  };
  Color curcol = Color::BlueLight;
  Color dprogcol = Color::GrayDark;
};


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
  std::function<void()> on_insert = []() {};
  std::function<void()> on_space = []() {};
  std::function<void()> on_edit = []() {};
  std::function<void()> on_bookmark = []() {};
  std::function<void()> on_bookmark_next = []() {};
  std::function<void()> on_bookmark_prev = []() {};

  bool OnEvent(Event event) override {
    if(event == Event::Special("\x1b[5~")) {
      on_pageup();
    } else if(event == Event::Special("\x1b[6~")) {
      on_pagedown();
    } else if(event == Event::Special("\x1b[2~")) {
      on_insert();
    } else if(event == Event::Special(" ")) {
      on_space();
    } else if(event == Event::Special("e")) {
      on_edit();
    } else if(event == Event::Special("m")) {
      on_bookmark_next();
    } else if(event == Event::Special("n")) {
      on_bookmark_prev();
    } else if(event == Event::Special("b")) {
      on_bookmark();
    }else {
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
    } else if(itemcolors.size() == entries.size() && itemcolors[i] == 2) {
      elements.push_back(text(icon + entries[i]) | bgcolor(Color::YellowLight) | style | focused);
    } else if(itemcolors.size() == entries.size() && itemcolors[i] == 3) {
      elements.push_back(text(icon + entries[i]) | bgcolor(Color::YellowLight) | color(Color::GreenLight) | style | focused);
    } else if(itemcolors.size() == entries.size() && itemcolors[i] == 4) {
      elements.push_back(text(icon + entries[i]) | color(Color::YellowLight) | style | focused);
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
    move_mode= false;
    edit_mode = false;
    Add(&maincontainer);
    maincontainer.Add(&container);

    button_exit.on_click = [&] {quit_program=true; on_quit(); };

    button_go_back.on_click = [&] {
      // todoselection.go_back();
    };
    if(menu_type == 0) {
        menu.entries = {L"Go Back",
                        L"Add task above",
                        L"Add note above",
                        L"Convert to project",
                        L"Remove task / note",
                        L"Start working",
                        L"Stop working",
                        L"Select task at random",
                        L"Show command palette",
                        L"Exit"};
    } else {
        menu.entries = {L"Go Back",
                        L"Add task above",
                        L"Add note above",
                        L"Convert to project",
                        L"Remove recursive",
                        L"Start working",
                        L"Stop working",
                        L"Select task at random",
                        L"Insert others under selected",
                        L"Insert completed under selected",
                        L"Insert from file",
                        L"Exit"};
    }
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
      int i = 0;
      if(edit_mode) {
        (*edittask).name = to_string(input_1.content);
        edit_mode=false;
        input_1.content = L"";
        updateSelection();
        todomenu.TakeFocus();
        return;
      }

      task &ctit = current_active_task; 
/*
      task newtask;
      newtask.completed = false;
      newtask.is_note = false;
      newtask.name = to_string(input_1.content);
      ctit.subtasks.push_back(newtask);
*/
      bool added=false;
      for (auto it = ctit.subtasks.begin(); it != ctit.subtasks.end(); ++it)
      {
        if (i == todomenu.selected+1)
        {
          task newtask;
          newtask.is_note = false;
          newtask.completed = false;
          newtask.highlighted_for_copy = false;
          if(use_metadata) {
           // newtask.metadata[ACCUMULATED_TIME] = to_string(0);
           // newtask.metadata[SESSION_TIME] = to_string(0);
          }
          newtask.name = to_string(input_1.content);
          ctit.subtasks.insert(it, newtask);

          added=true;
        }
        i++;
      }
      if(!added) {
        task newtask;
        newtask.completed = false;
        newtask.is_note = false;
        newtask.name = to_string(input_1.content);
        newtask.highlighted_for_copy = false;
        if(use_metadata) {
        //  newtask.metadata[ACCUMULATED_TIME] = to_string(0);
        //  newtask.metadata[SESSION_TIME] = to_string(0);
        }

        ctit.subtasks.push_back(newtask);        
      }
      todomenu.selected++;
      input_1.content = L"";
      updateSelection();
    };

    input_2.on_enter = [&] {
      if(edit_mode) {
        (*edittask).name = to_string(input_2.content);
        input_2.content = L"";
    /*
        if(use_metadata) {
          (*edittask).metadata[ACCUMULATED_TIME] = to_string(0);
        }
    */
        updateSelection();
        todomenu.TakeFocus();
        edit_mode=false;
        return;
      }

      int i = 0;
      bool added = false;
      task &ctit = current_active_task;
      for (auto it = ctit.subtasks.begin(); it != ctit.subtasks.end(); ++it)
      {
        if (i == todomenu.selected+1)
        {
          task newtask;
          newtask.is_note = true;
          newtask.completed = true;
          newtask.highlighted_for_copy = false;
          newtask.name = to_string(input_2.content);
          ctit.subtasks.insert(it, newtask);
          added=true;
        }
        i++;
      }
      if(!added) {
        task newtask;
        newtask.completed = true;
        newtask.is_note = true;
        newtask.highlighted_for_copy = false;
        newtask.name = to_string(input_2.content);
        ctit.subtasks.push_back(newtask);        
      }
      updateSelection();
      input_2.content = L"";
      todomenu.selected++;

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
        updateTree();
        if (ctit.parent != NULL)
        {
          current_active_task = *ctit.parent;
          updateSelection();
//          previous_task.pop_back();
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
            newtask.highlighted_for_copy = false;
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
            newtask.highlighted_for_copy = false;
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
            //fix: previous_task.push_back(ctit);
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
              if(std::addressof(root_task.get()) == &ctit && ctit.subtasks.size() == 1) {
                  annotate=true;
                  annotation="Cannot delete the last item!";
                  return;
              } else { 
                // deposit times into parent
                task &rt = root_task;
                rt.parent = NULL;

                update_parents(ctit);
                // get total time spent on this task
                auto totaltime=get_total_time((*it));
                // deposit time into parent
                if(use_metadata) {
                  // if parent has ACCUMULATED_TIME metadata, add to it
                  if(ctit.metadata.find(ACCUMULATED_TIME) != ctit.metadata.end()) {
                    int old_time = stoi(ctit.metadata[ACCUMULATED_TIME]);
                    ctit.metadata[ACCUMULATED_TIME] = to_string(old_time + totaltime.second);
                  } else {
                    // if parent does not have metadata, add it
                    ctit.metadata[ACCUMULATED_TIME] = to_string(totaltime.second);
                  }
                  // same for SESSION_TIME
                  if(ctit.metadata.find(SESSION_TIME) != ctit.metadata.end()) {
                    int old_time = stoi(ctit.metadata[SESSION_TIME]);
                    ctit.metadata[SESSION_TIME] = to_string(old_time + totaltime.first);
                  } else {
                    ctit.metadata[SESSION_TIME] = to_string(totaltime.first);
                  }
                }
                ctit.subtasks.erase(it);
                updateSelection();
              }
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
        // store previous active_task
        std::list<task>::iterator prev_active = active_task;

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
            active_task = it;
            break;
          }
          i++;
        }
        string title_end;
        title_end += " (";
        //title_end += remove_whitespace_from_end(ctit.name);
        bool first = true;
        updateTree();
        if(ctit.parent != NULL) {
          title_end += (print_parent_names(ctit).erase(0,2));
        }
 #if 0        
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
#endif
        title_end += ")";
        if (title_end.compare(" ()") > 0)
          work_task_title += title_end;


        // if already working on task, do no reset counter
        if (working_on_a_task)
        {
          put_to_log("Switched task to " + work_task_title);
          if(use_metadata) {
            // bank the accumulated time to the previous task
            time_t time_add = time(NULL) - time_at_task_start - elsetime;
            // if metadata has ACCUMULATED_TIME
            int prevacc=0;
            if((*prev_active).metadata.find(ACCUMULATED_TIME) != (*prev_active).metadata.end()) {
              prevacc=stoi((*prev_active).metadata[ACCUMULATED_TIME]);
            }
            prevacc+=time_add;
            (*prev_active).metadata[ACCUMULATED_TIME]=to_string(prevacc);

            int prevsacc=0;
            if((*prev_active).metadata.find(SESSION_TIME) != (*prev_active).metadata.end()) {
              prevsacc=stoi((*prev_active).metadata[SESSION_TIME]);
            }
            prevsacc+=time_add;
            (*prev_active).metadata[SESSION_TIME]=to_string(prevsacc);
            // store how much time has been banked elsewhere
            elsetime+=time_add;
          }
        }
        else
        {
          // start working on a task
          elsetime = 0;
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
          time_t time_add = time(NULL) - time_at_task_start;
          accumulated_time += time_add;

          if(use_metadata) {
            // check if medata contains accumulated time
            int saved_time=0;
            if((*active_task).metadata.find(ACCUMULATED_TIME) != (*active_task).metadata.end()) {
              saved_time=stoi((*active_task).metadata[ACCUMULATED_TIME]);
            } 

            int saved_stime=0;
            if((*active_task).metadata.find(SESSION_TIME) != (*active_task).metadata.end()) {
              saved_stime=stoi((*active_task).metadata[SESSION_TIME]);
            } 

            metadata[ACCUMULATED_TIME] = to_string(accumulated_time);
            metadata[PAUSE_STAMP] = to_string(time_at_task_start);
            // write accumulated time to the task

            (*active_task).metadata[ACCUMULATED_TIME] = to_string(saved_time+time_add-elsetime);
            (*active_task).metadata[SESSION_TIME] = to_string(saved_stime+time_add-elsetime);

            elsetime = 0;
          }
          put_to_log("Ended working session. Total time " +
                    to_string((time(NULL) - time_at_task_start) / 60.0f) + " minutes.");
          put_to_log("Daily accumulation " + to_string(accumulated_time) +
                    " seconds.");
        }
      }
      else if (menu.selected == 8 && menu_type == 1)
      {
        // copy everything else under selected task
        // convert to project
        int i = 0;
        task &ctit = current_active_task;
        auto to_note = ctit.subtasks.begin();
        for (auto it = ctit.subtasks.begin(); it != ctit.subtasks.end(); ++it)
        {
          if (i == todomenu.selected)
          {
            if((*it).is_note) {
              annotate=true;
              annotation="Cannot move to a note!";
              return;
            }
            to_note = it;
          }
          i++;
        }
        i=0;
        for (auto it = ctit.subtasks.begin(); it != ctit.subtasks.end(); ++it)
        {
          if (i != todomenu.selected) {
            task newtask;
            newtask.name = (*it).name;
            newtask.is_note = (*it).is_note;
            newtask.highlighted_for_copy = false;
            newtask.subtasks = (*it).subtasks;
            newtask.completed = (*it).completed;
            if(use_metadata) {
              newtask.metadata = (*it).metadata;
            }
            (*to_note).subtasks.push_back(newtask);
            //ctit.subtasks.erase(it);
          }
          i++;
        }
        i=0;
        for (auto it = ctit.subtasks.begin(); it != ctit.subtasks.end();)
        {
          if (i == todomenu.selected) {
            ++it;
          } else {
            it=ctit.subtasks.erase(it);
          }
          i++;
        }
        updateSelection();
      }

      else if (menu.selected == 8 && menu_type == 1)
      {
        // copy everything else under selected task
        // convert to project
        int i = 0;
        task &ctit = current_active_task;
        auto to_note = ctit.subtasks.begin();
        for (auto it = ctit.subtasks.begin(); it != ctit.subtasks.end(); ++it)
        {
          if (i == todomenu.selected)
          {
            if((*it).is_note) {
              annotate=true;
              annotation="Cannot move to a note!";
              return;
            }
            to_note = it;
          }
          i++;
        }
        i=0;
        for (auto it = ctit.subtasks.begin(); it != ctit.subtasks.end(); ++it)
        {
          if (i != todomenu.selected) {
            task newtask;
            newtask.name = (*it).name;
            newtask.is_note = (*it).is_note;
            newtask.highlighted_for_copy = false;
            newtask.subtasks = (*it).subtasks;
            newtask.completed = (*it).completed;
            if(use_metadata) {
              newtask.metadata = (*it).metadata;
            }
            (*to_note).subtasks.push_back(newtask);
            //ctit.subtasks.erase(it);
          }
          i++;
        }
        i=0;
        for (auto it = ctit.subtasks.begin(); it != ctit.subtasks.end();)
        {
          if (i == todomenu.selected) {
            ++it;
          } else {
            it=ctit.subtasks.erase(it);
          }
          i++;
        }
        updateSelection();
      }
      else if (menu.selected == 9 && menu_type == 1)
      {
        // insert from file

      }
      else if (menu.selected == 8 && menu_type == 0)
      {
        // toggle between bookmarks
      }


      if (menu.selected == 9 && menu_type == 0 || menu.selected == 10 && menu_type == 1)
      {
        if(working_on_a_task == true) {
          work_task_title = "Currently not working on a task";
          time_at_task_stop = time(NULL);
          working_on_a_task = false;
          time_t time_add = time(NULL) - time_at_task_start;
          accumulated_time += time_add;
          if(use_metadata) {
            int saved_time=0;
            if((*active_task).metadata.find(ACCUMULATED_TIME) != (*active_task).metadata.end()) {
              saved_time=stoi((*active_task).metadata[ACCUMULATED_TIME]);
            }
            int saved_stime=0;
            if((*active_task).metadata.find(SESSION_TIME) != (*active_task).metadata.end()) {
              saved_stime=stoi((*active_task).metadata[SESSION_TIME]);
            }

            metadata[ACCUMULATED_TIME] = to_string(accumulated_time);
            metadata[PAUSE_STAMP] = to_string(time_at_task_start);
            // write accumulated time to the task

            (*active_task).metadata[ACCUMULATED_TIME] = to_string(saved_time+time_add-elsetime);
            (*active_task).metadata[SESSION_TIME] = to_string(saved_stime+time_add-elsetime);

            elsetime = 0;
          }
          put_to_log("Ended working session. Total time " +
                    to_string((time(NULL) - time_at_task_start) / 60.0f) + " minutes.");
          put_to_log("Daily accumulation " + to_string(accumulated_time) +
                    " seconds.");
        }
        on_quit();
      }
    };

    todomenu.on_pageup = [&] {
        todomenu.selected = 0;
    };

    todomenu.on_bookmark = [&] {
        task &ctit = current_active_task;
        if(ctit.bookmark_id == -1) {
          bookmarks.push_back(current_active_task);
          ctit.bookmark_id = bookmark_counter;
//          put_to_log("Added bookmark number " + to_string(bookmark_counter));
          bookmark_counter++;
          cur_bookmark_pos = bookmarks.begin();
        } else {
//        put_to_log("Erasing bookmark " + to_string(ctit.bookmark_id));
          auto deleteit=bookmarks.begin();
          for (auto it=bookmarks.begin(); it!=bookmarks.end(); ++it) {
            task &ait = *it;
            if(ait.bookmark_id = ctit.bookmark_id) {
                deleteit=it;
            }
          }
          if(bookmarks.size() != 0) {
            bookmarks.erase(deleteit);
            put_to_log("Succesfully erased ");
          }
          ctit.bookmark_id = -1;
    
          updateSelection();
        }

    };

    todomenu.on_bookmark_next = [&] {

        if(bookmarks.size() == 0) {
          return;
        }
        ++cur_bookmark_pos;
        bool has_position=false;

        for (auto it=bookmarks.begin(); it!=bookmarks.end(); ++it) {
          if(it == cur_bookmark_pos) {
            has_position=true;
          }
        }

        if(!has_position) {
          cur_bookmark_pos=bookmarks.begin();
        }

        current_active_task = (*cur_bookmark_pos);


        updateSelection();

        task &ctit = current_active_task;
        task &rt = root_task;
        
        rt.parent = NULL;
        update_parents(root_task);

    };

    todomenu.on_bookmark_prev = [&] {

        if(bookmarks.size() == 0) {
          return;
        }
        --cur_bookmark_pos;
        bool has_position=false;

        for (auto it=bookmarks.begin(); it!=bookmarks.end(); ++it) {
          if(it == cur_bookmark_pos) {
            has_position=true;
          }
        }

        if(!has_position) {
          if(bookmarks.size() > 1) {
                cur_bookmark_pos=bookmarks.end();
                --cur_bookmark_pos;
          } else {
            cur_bookmark_pos = bookmarks.begin();
          }
        }
        current_active_task = (*cur_bookmark_pos);

        updateSelection();

        task &ctit = current_active_task;
        task &rt = root_task;
        
        rt.parent = NULL;
        update_parents(root_task);

    };


    todomenu.on_pagedown = [&] {
        task ctask = current_active_task;
        input_2.TakeFocus();
        if(todomenu.selected == todomenu.entries.size() - 1) {
        } else{
          todomenu.selected = todomenu.entries.size()-1;
        }
    };

    todomenu.on_edit = [&] {
        // take focus

      edit_mode = true;
      // copy the content of the selected task to the input field
      int i = 0;
      task &ctit = current_active_task;
      for (auto it = ctit.subtasks.begin(); it != ctit.subtasks.end(); ++it)
      {
        if (i == todomenu.selected)
        {
          if((*it).is_note) {
            input_2.content = to_wstring((*it).name);
            input_2.maxout_cursor();
            input_2.TakeFocus();

          } else {
            input_1.content = to_wstring((*it).name);
            input_1.maxout_cursor();
            input_1.TakeFocus();
          }
          edittask = it;
          break;
        }
        i++;
      }
      updateSelection();
    };

    todomenu.on_space = [&] {
        if(!move_mode) {
          move_mode =true;
          task &ctit = current_active_task;
          int i = 0;
          for (auto it = ctit.subtasks.begin(); it != ctit.subtasks.end(); ++it)
          {
            if (i == todomenu.selected)
            {
              (*it).highlighted_for_copy = true;
              fromtask = it;
              active_task_for_move = current_active_task;
            }
            i++;
          }
        } else {
          move_mode = false;
          task &ctit = current_active_task;
          bool found = false;
          int i = 0;
          for (auto it = ctit.subtasks.begin(); it != ctit.subtasks.end(); ++it)
          {
            if (i == todomenu.selected+1)
            {
              totask = it;
              found = true;
            }
            i++;
          } 
          if(!found) {
            task newtask;
            newtask.name = (*fromtask).name;
            newtask.is_note = (*fromtask).is_note;
            newtask.highlighted_for_copy = false;
            newtask.subtasks = (*fromtask).subtasks;
            newtask.completed = (*fromtask).completed;
            if(use_metadata) {
              newtask.metadata = (*fromtask).metadata;
            }

            ctit.subtasks.push_back(newtask);
            task &mit = active_task_for_move;
            mit.subtasks.erase(fromtask);

            (*fromtask).highlighted_for_copy = false;
            updateSelection();
            return;
          }
          (*fromtask).highlighted_for_copy = false;
          if(fromtask != totask) {
            // create a new task and copy the content
            task newtask;
            newtask.name = (*fromtask).name;
            newtask.is_note = (*fromtask).is_note;
            newtask.highlighted_for_copy = false;
            newtask.completed = (*fromtask).completed;
            newtask.subtasks = (*fromtask).subtasks;
            if(use_metadata) {
              newtask.metadata = (*fromtask).metadata;
            }
           // (*totask).subtasks.push_back(newtask);
           // (*fromtask).subtasks.clear();
            ctit.subtasks.insert(totask, newtask);
            // delete the old task
            task &mit = active_task_for_move;
            mit.subtasks.erase(fromtask);

//            std::swap(*fromtask, *totask);
          }
        }
        updateSelection();
      //  input_2.TakeFocus();
    };


    todomenu.on_insert = [&] {
        //task ctask = current_active_task;
        input_2.TakeFocus();
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
            // prevent from moving to folder which is hightlighted for copy
            // otherwise user can try to move folder into itself
            if(!task.highlighted_for_copy) {
             // fix: previous_task.push_back(cat);
              current_active_task = task;
              updateSelection();
              return;
            }
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

//        if (previous_task.size() > 0)
 //       {
          cat.completed = true;
 //       }
      }
      else
      {
        cat.completed = false;
      }
      updateSelection();
    };

    current_active_task = _root_task;
    root_task = _root_task;

    updateSelection();
    input_2.TakeFocus();

  }

void updateTree() {
  task &root = root_task;
  root.parent = NULL;
  update_parents(root_task);
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
          if(task.highlighted_for_copy) {
            colors.push_back(2);
          } else if(task.bookmark_id >= 0) {
            colors.push_back(4);
          }
          else {
            colors.push_back(0);
          }

        if (task.completed)
        {
          taskname = checked;
        }
        else
        {
          taskname = unchecked;
        }
      } else {
          if(task.highlighted_for_copy) {
            colors.push_back(3);
          } else {
              colors.push_back(1);
          }
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
    if(cat.bookmark_id>=0) {
        return window(text(to_wstring(cat.name)) | bgcolor(Color::Yellow) | hcenter, vbox(
                                                                {hbox({vbox(todomenu.Render(), input_2.Render(), input_1.Render()) | frame, menu.Render() /*, input_1.Render()*/}) |
                                                                      flex,
                                                                  timergauge.Render() | size(HEIGHT, GREATER_THAN, 5)}));
    } else {
        return window(text(to_wstring(cat.name)) | hcenter, vbox(
                                                                {hbox({vbox(todomenu.Render(), input_2.Render(), input_1.Render()) | frame, menu.Render() /*, input_1.Render()*/}) |
                                                                      flex,
                                                                  timergauge.Render() | size(HEIGHT, GREATER_THAN, 5)}));

    }
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

  std::reference_wrapper<task> root_task = tmptask;
  std::list<task>::iterator active_task;

//  vector<std::reference_wrapper<task>> previous_task;
  vector<std::reference_wrapper<task>> parent_task;

  std::list<std::reference_wrapper<task>> bookmarks;
  std::list<std::reference_wrapper<task>>::iterator cur_bookmark_pos;
  std::list<std::reference_wrapper<task>>::iterator prev_bookmark_pos;

  bool move_mode;
  std::list<task>::iterator fromtask;
  std::list<task>::iterator totask;
  std::reference_wrapper<task> active_task_for_move = tmptask;

  bool edit_mode;
  std::list<task>::iterator edittask;

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

void reset_session_timers(task &task)
{
  // if metadata contains session time, reset it
  if (task.metadata.find(SESSION_TIME) != task.metadata.end())
  {
    auto it=task.metadata.find(SESSION_TIME);
    task.metadata.erase(it);
  }
  if (task.subtasks.size() > 0)
  {
    for (auto &ctask : task.subtasks)
    {
      reset_session_timers(ctask);
    }
  }
}

void erase_metadata(task &task)
{
  // if metadata contains session time, reset it
  task.metadata.erase ( task.metadata.begin(), task.metadata.end() );

  if (task.subtasks.size() > 0)
  {
    for (auto &ctask : task.subtasks)
    {
      erase_metadata(ctask);
    }
  }
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


void add_metadata(string filename) {
  std::ifstream file;
  file.open(filename, std::ifstream::in);
  std::string line;
  std::vector <string> lines;
  // read all lines from the file

  while (!file.eof()) 
  {
    std::getline(file, line);
    lines.push_back(line);
  } 
  file.close();

  // open the file again and write the lines back
  std::ofstream outfile(filename,std::ofstream::out);
  // write metadata to the file
  if(use_metadata)
  {
    outfile << meta_start_string;
    for(auto &pair : metadata)
    {
      outfile << pair.first << " " << pair.second << " ";
    }
     outfile << meta_end_string;
  }
  // write the lines back to the file
  for(auto &line : lines)
  {
    outfile << line << "\n";
  }
  // close the file
  outfile.close();
}

std::map <string, string> get_metadata(string filename) {
    // read the file
    std::ifstream file(filename, std::ifstream::in);
    // quit if the file does not exist
    std::string line;
    std::map <string, string> metadata;
    // define metadata starting string



    // check if the first line contains metadata
    // read first line
    std::getline(file, line);
    // check if the first line contains metadata
    if (line.find(meta_start_string) != std::string::npos && line.find(meta_end_string) != std::string::npos) {
       // remove marker
       erase_substring(line, meta_start_string);
       erase_substring(line, meta_end_string);
       // read words from the line into variable value pairs
        std::istringstream iss(line);
        std::string key, value;
        while (iss >> key >> value) {
            metadata[key] = value;
        } 
    } else {
       // file does not contain metadata
    }
    // close the file
    file.close();
    return metadata;
}

string format_time_diff(int seconds) {
  // include days, hours, minutes, seconds
  int days = seconds / 86400;
  int hours = (seconds % 86400) / 3600;
  int minutes = (seconds % 3600) / 60;
  int seconds_left = seconds % 60;
  stringstream ss;
  if (days > 0) {
    ss << days << "d ";
  }
  if (hours > 0) {
    ss << hours << "h ";
  }
  if (minutes > 0) {
    ss << minutes << "m ";
  }
  if (seconds_left > 0) {
    ss << seconds_left << "s";
  }
  return ss.str();
}

// print perenctages nicely with 2 decimal places
string format_percentage(double percentage) {
  stringstream ss;
  // if percentage is nan
  if (std::isnan(percentage)) {
    ss << "N/A";
  } else {
    ss << std::fixed << std::setprecision(2) << percentage*100 << "%";
  }
  return ss.str();
}

// format time stamp for printing
string format_time_stamp(time_t time) {
  stringstream ss;
  ss << std::put_time(std::localtime(&time), "%F %T");
  return ss.str();
}


// print statistics
void print_statistics(task &task, bool isroot, bool session_only)
{
  int csum = 0, usum = 0;
  auto total_time = get_total_time(task);
  int alltime = total_time.second;
  int sessiontime = total_time.first;
  string title;
  string ses;
  if(session_only) {
    ses = "Session";
  } else {
    ses = "All time";
  }

  if(isroot) {
//     cout << "[Task name] [Time used in working session] [Time used all time]" << endl;

     title= "=============== " + ses + " summary ================";
     cout << title << endl;
     // if metadata has timestamp for session start, print it
     if(session_only) {
      if (metadata.find(SESSION_STAMP) != metadata.end() && metadata.find(SESSION_END_STAMP) != metadata.end())
      {
        cout << "Session report from " << format_time_stamp(stoi(metadata[SESSION_STAMP]));
        cout << " to " << format_time_stamp(stoi(metadata[SESSION_END_STAMP])) << ". " << endl;
      } else {
        cout << "No session start time found from the file. It will be included on next session timer restart." << endl;
      }
     } else {
      if (metadata.find(ALLTIME_STAMP) != metadata.end() && metadata.find(ALLTIME_END_STAMP) != metadata.end())
      {
        cout << "All data dates from " << format_time_stamp(stoi(metadata[ALLTIME_STAMP]));
        cout << " to " << format_time_stamp(stoi(metadata[ALLTIME_END_STAMP])) << endl;
      } else {
        cout << "No start time found from the file. Erasing all metadata with \'-e\' -option and re-editing will add the time. " << endl;
      }

     }

  } else {
     title= "--------------- " + ses + " summary for subtask: " + task.name + "------------------";
     cout << title << endl;
//     cout << "---------------  for task " << task.name << " ------ " <<endl;
  }
  if(!session_only) {
    if(isroot) {
       cout << "Time spent on all tasks: " << format_time_diff(alltime) << endl;
    } else {
       cout << "Time spent on tasks: " << format_time_diff(alltime) << endl;
    }
  } else {
    cout << "Time spent on tasks during session: " << format_time_diff(get_total_time(task).first) << endl;
  }
  
  typedef struct  {
    string line;
    int time;
    double percent_time_session;
    double percent_time_all;
  } task_stats;
  
  std::list<task_stats> task_stat_list;

  if (task.subtasks.size() > 0)
  {
    for (auto &ctask : task.subtasks)
    {
      task_stats task_stat;
      auto tmp = get_total_time(ctask);
      task_stat.line = ctask.name;
      if(session_only) {
        task_stat.time = tmp.first;
      } else {
        task_stat.time = tmp.second;
      }
      task_stat.percent_time_session = (double)tmp.first / (double)sessiontime;
      task_stat.percent_time_all = (double)tmp.second / (double)alltime;
      task_stat_list.push_back(task_stat);
    }    
  }
  if(session_only) {
  // sort the list by percent time in session
    task_stat_list.sort([](task_stats a, task_stats b) {
      return a.percent_time_session > b.percent_time_session;
    });
    // print the list
    for (auto &task_stat : task_stat_list)
    {
        // if the time is 0, don't print it
        if(task_stat.time == 0) {
          continue;
        }
        cout << task_stat.line << ": " << " " << format_time_diff(task_stat.time) << " " << format_percentage(task_stat.percent_time_session) << endl;
    }
   } else {
  // sort the list by percent time in all time
    task_stat_list.sort([](task_stats a, task_stats b) {
      return a.percent_time_all > b.percent_time_all;
    });
    // print the list
    for (auto &task_stat : task_stat_list)
    {
        // if the time is 0, don't print it
        if(task_stat.time == 0) {
          continue;
        }
        cout << task_stat.line << ": " << " " << format_time_diff(task_stat.time) << " " << format_percentage(task_stat.percent_time_all) << endl;
    }

   }
   if(isroot) {
     // repeat '=' for the length of the title
      for(int i = 0; i < title.length(); i++) {
        cout << "=";
      }
      cout << "\n";
   } else {
      for(int i = 0; i < title.length(); i++) {
        cout << "-";
      }
      cout << "\n";
   }
    for (auto &ctask : task.subtasks)
    {
       if(ctask.subtasks.size() > 0) {
          auto tmp = get_total_time_in_subtasks(ctask,true);
          if(session_only) {
            if(tmp.first > 0) {
              print_statistics(ctask, false, session_only);
            }
          } else {
            if(tmp.second > 0) {
              print_statistics(ctask, false, session_only);
            }
          }
          
       }
    }
}

void print_help(const char *programname) {
    printf("Program version %s \n\n", VERSION);

    printf("Usage: %s <filename.md> <options>\n", programname);
    // printf options:
    cout << "Options:" << endl;
    cout << "-t <tasktime> <pausetime> <worktime>" << endl;
    cout << "   tasktime: time in minutes to complete a task" << endl;
    cout << "   pausetime: time in minutes to pause between tasks" << endl;
    cout << "   worktime: time in hours to work on a task" << endl;
    cout << "   These options are automatically stored into the metadata of the file." << endl;
    cout << "-v print version" << endl;
    cout << "-h print help" << endl;
    cout << "-r reset session" << endl;
    cout << "-m <option>" << endl;
    cout << "   select menu type" << endl;
    cout << "   option \"0\" short menu \"1\" extended menu" << endl;
    cout << "-f <filename.md>" << endl;
    cout << "   Inserts another todofile to current file to user given location. Works only with -m 1." << endl;
    cout << "-e erase metadata (erases all timer information from the file and quits. Does a full timer/setting reset.)" << endl;
    cout << "-d disable metadata (disables storing timer information to the file. Erases existing timer information from the file.)" << endl;
    cout << "-s <option>\n"; 
    cout << "   generate a time usage report from the file\n" << endl;
    cout << "   option: set to \"1\" for session statistics, \"0\" for all time statistics\n" << endl;
    // explain what metadata is
    cout << "Metadata contains information on how much time has been spent on a task and other state varibles. " << endl;
    cout << "It will be stored into the file enclosed between '" << meta_start_string << "' and '" << meta_end_string << "'. " << endl;
    // describe tasktime, pausetime, worktime
}

int main(int argc, const char *argv[])
{
  string todofile;
  bool need_help=false;
  bool reset_timers=false;
  bool erase_all_metadata=false;

  if (argc < 2)
  {
      print_help(argv[0]);
      return 0;
  }

  bool new_tasktime = false;
  bool new_pausetime = false;
  bool new_worktime = false;
  bool show_stats=false;
  bool show_stats_session=false;

     // check if any of the remaining args contain "-t"
  for(int i = 1; i < argc; i++) {
      if(strcmp(argv[i], "-t") == 0) {
        // check how many numbers follow after "-t"
        int num_args = 0;
        int maxlen = min(i+4, argc);
        for(int j = i+1; j < maxlen; j++) {
            if(isdigit(argv[j][0])) {
                num_args++;
            } else {
                break;
            }
        }
        cout << num_args << endl;
        if(num_args > 0 && i+1 < argc) {
            tasktime = stod(argv[i+1]) * 60;
            new_tasktime = true;
        }
        if(num_args > 1 && i+2 < argc) {
            pausetime = stod(argv[i+2]) * 60;
            new_pausetime = true;
        }
        if(num_args > 2 && i+3 < argc) {
            worktime = stod(argv[i+3]) * 60 * 60;
            new_worktime = true;
        }
        i+=num_args;
      } else if(strcmp(argv[i], "-r") == 0) {
            reset_timers = true;
            cout << "Session timer will be reset from the file. Cumulative timers will not reset. Continue? (y/n)" << endl;
            string answer;
            cin >> answer;
            if(answer == "y") {

            } else {
                cout << "Aborting..." << endl;
                return 1;
            }
      } else if(strcmp(argv[i], "-e") == 0) {
          // clear all metadata from the file
          // prompt for confirmation
          cout << "Are you sure you want to erase all metadata from the file? (y/n)" << endl;
          string answer;
          cin >> answer;
          if(answer == "y") {
              // delete metadata
              erase_all_metadata=true;
          } else {
              cout << "Aborting..." << endl;
              return 1;
          }
      } else if(strcmp(argv[i], "-h") == 0) {
          print_help(argv[0]);
          return 1;
      } else if(strcmp(argv[i], "-d") == 0) {
          // disable metadata
          use_metadata = false;
      } else if(strcmp(argv[i], "-v") == 0) {
          // print version
          printf("Program version %s \n\n", VERSION);
          return 1;
      } else if(strcmp(argv[i], "-s") == 0) {
          show_stats=true;
          // if the next argument is a number
          if(i+1 < argc && isdigit(argv[i+1][0])) {
              // set the option
              show_stats_session = (stoi(argv[i+1]) == 1);
          }
          i++;
          // show statistics
          // show_statistics(todofile);
      } else if(strcmp(argv[i], "-m") == 0) {
          if(i+1 < argc && isdigit(argv[i+1][0])) {
              // set the option
              menu_type = stoi(argv[i+1]);
          }
          i++;
      } else if(strcmp(argv[i], "-f") == 0) {
          if(i+1 < argc) {
              // set the option
              insertfile = argv[i+1];
          }
          if (!file_exists(insertfile)) {
            cerr << "Cannot find file " << insertfile << "\n"; 
            return 1;
          }
          i++;
      } else {
          if(argv[i][0] == '-') {
            cout << "Unknown option\n " << argv[i];
            return  1;
          } else { 
	    if(todofile.length() == 0) { 
              todofile = argv[i]; 
            } else {
              cout << "Unknown arguments given\n";
               return 1;
      	    }
          }
      }
  }    
  if(todofile.length() == 0) {
    cerr << "No filename given.\n";
    return 1;
  }  
  cout << "Opening " << todofile << "\n";
  metadata=get_metadata(todofile);
  // if there is no metadata, prompt user
  if(metadata.size() == 0 && !erase_all_metadata && use_metadata) {
      // check if the file exists
      if (!file_exists(todofile))
      {
        cout << "Creating a new todo file.\n";
      } else {
        cout << "No metadata was found in the file.\n";
      }
      cout << "Metadata is used to save timer information in the todo file." << endl;
      cout << "Without metadata, settings and time accumulation cannot restored after quitting the program." << endl;
      cout << "You can erase all metadata from the file later using the \"-e\" option." << endl;
      cout << "" << endl << endl;
      cout << "Do you want to add the metadata (recommended)? (y/n)" << endl;
      string answer;
      cin >> answer;
      // check if the user wants to add metadata  
      if(answer.compare("n") == 0) {
        use_metadata = false;
      } else if(answer.compare("y") == 0) {
        use_metadata = true;
      } else {
        cout << "Invalid answer. Exiting." << endl;
        return 1;
      }
      if(use_metadata) {
        metadata[ACCUMULATED_TIME] = "0";
        metadata[TASK_TIME] = to_string((int)tasktime);
        metadata[PAUSE_TIME] = to_string((int)pausetime);
        metadata[WORK_TIME] = to_string((int)(worktime));
        metadata[ALLTIME_STAMP] = to_string(time(NULL));
        metadata[SESSION_STAMP] = to_string(time(NULL));
        // add_metadata(todofile);
      }
  } else {
    // load settings from metadata
    if(!new_tasktime) {
      if(metadata.find(TASK_TIME) != metadata.end()) {
        tasktime = stoi(metadata[TASK_TIME]);
      }
    } else {
      metadata[TASK_TIME] = to_string((int)tasktime);
    }
    if(!new_pausetime) {
      if(metadata.find(PAUSE_TIME) != metadata.end()) {
        pausetime = stoi(metadata[PAUSE_TIME]);
      }
    } else {
      metadata[PAUSE_TIME] = to_string((int)pausetime);
    }
    if(!new_worktime) {
      if(metadata.find(WORK_TIME) != metadata.end()) {
        worktime = stod(metadata[WORK_TIME]);
      }
    } else {
      metadata[WORK_TIME] = to_string((int)worktime);
    }
  }
  srand(time(NULL));
      
  // if metadata contains accumulated time use it as accumulated time
  if(metadata.find(ACCUMULATED_TIME) != metadata.end()) {
      accumulated_time = stoi(metadata[ACCUMULATED_TIME]);
  } else {
      accumulated_time = 0;
  }

  // if metadata contains pause stamp
  if(metadata.find(PAUSE_STAMP) != metadata.end()) {
      time_at_task_stop = stoi(metadata[PAUSE_STAMP]);
  } else {
      time_at_task_stop = time(NULL);
  } 

  if(reset_timers) {
      accumulated_time = 0;
      time_at_task_stop = time(NULL);
      metadata[ACCUMULATED_TIME] = "0";
      metadata[PAUSE_STAMP] = to_string(time_at_task_stop);
      metadata[SESSION_STAMP] = to_string(time_at_task_stop);
  }

  auto time_at_last_todo_save = time(NULL);

  task root_task;

  if (!file_exists(todofile))
  {

  }
  else
  {
    vector<string> todolist_raw = read_lines(todofile);
    if(todolist_raw.size() > 0) {
      root_task.subtasks = extract_tasks(todolist_raw);
    }
    if(reset_timers) {
      reset_session_timers(root_task);	
    }
    if(erase_all_metadata) {
      erase_metadata(root_task);
      use_metadata = false;
      save_tasks(todofile, root_task);
      cout << "All metadata removed from " << todofile << endl;
      return 0;
    }
    if(show_stats) {
      print_statistics(root_task, true, show_stats_session);
      return 0;
    }
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
      if(working_on_a_task) {
        time_t time_add = time(NULL) - time_at_task_start - elsetime;    
        // format time difference to human readable format
        time_title = format_time_diff(time_add);
      } else {
        time_title = format_time_diff(0);
      }
      if (current_time - time_at_last_todo_save > 60)
      {
        save_tasks(todofile, root_task);
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

  if(new_tasktime) {
    cout << "Tasktime set to " << tasktime << " seconds" << endl; 
  }  
  if(new_pausetime) {
    cout << "Pausetime set to " << pausetime << " seconds " << endl;
  }
  if(new_worktime) {
    cout << "Worktime set to " << worktime << " seconds " << endl;
  }

  return 0;
}
