Use FTXui c++ library to create a very simple, running tui that I can use to bootstrap my own application.
#### Goals
1. put the correct add_requires statement in xmake.lua so that xmake downloads, installs and builds the package.
2. create the cpp file tui_example.cpp to hold the function main and all of the other functions you need to create.
3. modify the "this" xmake.lua target to build the tui.
4. use the commands and descriptions in commands.md for the commands and 1 line descriptions to create a menu accessible with /.
5. for help, use show_help.cpp:
   - use the help_map keys for topics in the menu
   - use the string_view constants for the text of each topic
   - use function show_help or write something more suitable for tui this will be the implemetation of the help command.
6. each of the other commands has a trivial stub implemented in stubs.cpp files that can be wired into executing each command. You can modify stubs.cpp as needed, but do not modify any file in the src directory.
7. Follow the style of the TUI screen shots I have shown in the .png files in the scratch/tui directory.

#### Task completion
- make sure the code builds with xmake successfully
- run the code and check the screen display is correct (if you can...)

Here are some details for the screen shots:

#### Opening Screen
- Don't use graphic block characters for the title:  just centered bold text:  epi_sim\n Epidemic Simulation Model
- in the center prompt box the intial text should be Press slash for commands to get started adn below that "Suggestion: choose /help followed by selecting Get started...
- nothing immediately below the prompt box
- no tip
- at bottom left: the working directory
- at bottom right: we will display the case tag, but we don't hsve one set so the default "null" text is:  No case selected...

#### Post command output
- pretty much as you see it:  the  output scrolls continuously
- the prompt box is always at the bottom and displays prompts produced by the running code or just put Press slash to choose a commmand...  in grayed text
- nothing below the prompt box
- create the sidebar even though we don't have anything much to put in it yet
  - Title: Case <>
  - first heading Population (later application code will provide things to fill in)
  - second heading:  Output (nothing until application code fills it in)
  - third heading: Plots (ditto)
  - footer text:  working directory

#### Menu for command at intro screen
- pretty much as shown 
- line below menu should say either "Available commands..." if the / shows the top level menu    or   it should show the previous command: for example if help was the command chosen then line below the menu should say "Available help topics..."  this strint should be a variable, possibly part of a struct MenuDisplay
- footer as before