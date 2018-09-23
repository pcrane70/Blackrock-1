![Alt text](resources/blackrock-title.png?raw=true "Blackrock")

[About](#about)  
[Current Features](#current-features)  
[Upcoming Features](#upcoming-features)  
[Installing](#installing)  
[Contributing](#contributing)  
[Copyright & Licensing](#copyright--licensing)
[Contact](#contact)

## About

**Blackrock** is currently a dungeon exploration rouguelike heavily inspired by the great **Nethack**, nut it is intended to become a much complex game featuring multiplayer and some bigger maps to explore with your friends.

**Blackrock** first started as a hobby program just to test my skills working with C, but as time went one, it became one of my biggest projects and I have to say that I have had a lot of fun working on it and also I have learned a lot.

This game is also intended to serve as a much more complex tutorial for anyone that would like to sharpen his or her skills in C. It includes many topics that I have had a hard time finding out how to implement them in C such as a database using Sqlite or some object pooling and an ECS. In the future it will feature a full wiki for any topic related to the game and its development. For now the code is heavily commented and I will try to keep that way trough its development. If you have any question please contact me.

## Current Features
  
- ECS  (Entity Component System)
- Object Pooling  
- Simple dungeon generation  
- Ascii characters  
- Simple random enemy spawner  
- Loot and currency  
- Weapons and armor  
- Some items  
- Shops  
- Simple inventory system  
- Simple UI  
- Only one class -> warrior  
- Only one race -> human  
- Leaderboards
- Support for Linux

## Upcoming Features
  
- Better UI
- New player profile    
- Bosses!  
- Crafting system and resources gathering 
- More classes
- Adding sounds!  
- Adding new and better graphics  
- Support for Mac and Windows
- Bigger open world  
- Multiplayer
- Achievements  

## Installing

### Linux

_Build it yourself_ -> You need to install the following dependencies first:

- Sdl 2.0 - libsdl2-dev
- Sqlite3 - libsqlite3-dev
- Pthread - libpthread-stubs0-dev

If you are using Ubuntu, you can run the following commands: 

``` sudo apt-get install libsdl2-dev ```

``` sudo apt-get install sqlite3 libsqlite3-dev ```

``` sudo apt-get install libpthread-stubs0-dev ```

Note: You also need to have installed:

- Make

_Compiling_ -> Just use 'make' in the project folder.

_Running_ ->  Use 'make run'.

**_Just give me the exe_** -> If you just want to download the executable, check the releases tab in the top of this page and select the one you want.

## Contributing

Anyone can make a pull request and I will try to check it, and if it seems fine with the flow of the game, I will include it on the project.

## Copyright & Licensing

Pending...

## Contact

You can contact me for whatever you need at erick_salas@ermiry.com.